"""Validate authored upgrade data and render its review documents. No combat simulation."""
from __future__ import annotations

import argparse
from collections import Counter
import json
from pathlib import Path

DESIGN = Path(__file__).resolve().parents[1]
ROSTER_FILES = [DESIGN / 'data/shared-upgrades.json', DESIGN / 'data/character-mayor-upgrades.json']
REFERENCE = DESIGN / 'data/sts2-relic-reference.json'
MERCENARIES = ['Mara', 'Ivo', 'Ada', 'Noor']
RARITIES = ['Common', 'Uncommon', 'Rare', 'Legendary']


def load_rows(path: Path, key: str) -> list[dict]:
    data = json.loads(path.read_text(encoding='utf-8-sig'))
    return data if isinstance(data, list) else data[key]


def read_all() -> tuple[list[dict], list[dict]]:
    return [row for path in ROSTER_FILES for row in load_rows(path, 'upgrades')], load_rows(REFERENCE, 'relics')


def mayor(row: dict) -> bool:
    return 'Mayor' in row['acquisition']


def validate(rows: list[dict], refs: list[dict]) -> dict:
    errors = []
    required = ['id', 'name', 'mercenary', 'rarity', 'source_rarity', 'rarity_basis', 'acquisition',
                'min_city', 'max_city', 'effect', 'strategy', 'default_exception', 'balance_risk',
                'inspiration', 'implementation_tags', 'status']
    source_ids = {r['id'] for r in refs}
    source_by_id = {r['id']: r for r in refs}
    for field in ['id', 'name']:
        duplicates = [x for x, count in Counter(str(r.get(field, '')).casefold() for r in rows).items() if count > 1]
        if duplicates:
            errors.append(f'Duplicate {field}: {duplicates}')
    if len(rows) != 288:
        errors.append(f'Expected 288 upgrades, found {len(rows)}')
    if len(refs) != 298 or len(source_ids) != 298:
        errors.append(f'Expected 298 distinct source records; found {len(refs)} rows / {len(source_ids)} IDs')
    for ref in refs:
        if not all(ref.get(key) for key in ['id', 'name', 'rarity', 'mechanics', 'branch', 'source_url']):
            errors.append(f"{ref.get('id', '?')}: incomplete reference definition or provenance")
    for r in rows:
        label = r.get('id', '?')
        missing = [key for key in required if key not in r or r[key] is None or r[key] == '']
        if missing:
            errors.append(f'{label}: missing {missing}')
            continue
        if r['mercenary'] not in ['Shared'] + MERCENARIES or r['rarity'] not in RARITIES:
            errors.append(f'{label}: invalid mercenary/rarity')
        if not 1 <= r['min_city'] <= r['max_city'] <= 3:
            errors.append(f'{label}: invalid city range')
        if r['status'] != 'proposal':
            errors.append(f'{label}: must distinguish draft effects from owner approval')
        if not r['inspiration']:
            errors.append(f'{label}: no inspiration reference')
        for source in r['inspiration']:
            if source['id'] not in source_ids:
                errors.append(f"{label}: missing source {source['id']}")
            if not source.get('url', '').startswith('https://'):
                errors.append(f'{label}: source URL absent')
        if r['source_rarity'] in ['Common', 'Uncommon', 'Rare'] and r['rarity'] != r['source_rarity']:
            errors.append(f'{label}: ordinary rarity parity violated')
        known_rarities = {source_by_id[s['id']]['rarity'] for s in r['inspiration'] if s['id'] in source_ids}
        if r['source_rarity'] not in known_rarities:
            errors.append(f'{label}: source rarity does not match its references: {known_rarities}')
        if mayor(r):
            if r['acquisition'] != ['Mayor'] or r['min_city'] != r['max_city']:
                errors.append(f'{label}: Mayor tier can leak through another source/stage')
            if r['rarity'] not in ['Rare', 'Legendary'] or r['source_rarity'] != 'Ancient':
                errors.append(f'{label}: Mayor must be Rare/Legendary and Ancient-inspired')
    if sum(mayor(r) for r in rows) != 102:
        errors.append('Expected 102 Mayor entries')
    for merc in MERCENARIES:
        normal = [r for r in rows if r['mercenary'] == merc and not mayor(r)]
        if len(normal) != 9:
            errors.append(f'{merc}: expected 9 normal exclusive upgrades, found {len(normal)}')
    availability = {}
    for city in range(1, 4):
        tier = [r for r in rows if mayor(r) and r['min_city'] == city]
        if len(tier) != 34:
            errors.append(f'City {city}: expected 34 Mayor entries, found {len(tier)}')
        if sum(r['mercenary'] == 'Shared' for r in tier) != 22:
            errors.append(f'City {city}: expected 22 shared Mayor entries')
        for merc in MERCENARIES:
            if sum(r['mercenary'] == merc for r in tier) != 3:
                errors.append(f'{merc} / City {city}: expected three exclusive Mayor entries')
            eligible = [r for r in tier if r['mercenary'] in ['Shared', merc]]
            availability[f'{merc} / City {city}'] = len(eligible)
            if len(eligible) < 3:
                errors.append(f'{merc} / City {city}: fewer than three eligible Mayor IDs')
    if errors:
        raise ValueError('\n'.join(errors))
    return {'upgrade_count': len(rows), 'source_count': len(refs),
            'rarity': dict(Counter(r['rarity'] for r in rows)),
            'mercenary': dict(Counter(r['mercenary'] for r in rows)),
            'mayor_eligible_before_item_conditions': availability,
            'scope': 'Authored data, source links and pool structure only; not combat/economy balance or runtime tests.'}


def md_text(value: object) -> str:
    return str(value).replace('|', '\\|').replace('\n', ' ')


def render_reference(refs: list[dict]) -> str:
    metadata = json.loads(REFERENCE.read_text(encoding='utf-8-sig'))
    lines = ['# STS2 relic reference inventory', '', 'Researched 18 September 2026. **298 entries with mechanical summaries.**', '',
             'This reference gathers the full 296-entry stable data snapshot and the two additional beta relics in the indexed wiki list. '
             'It records gameplay facts in compact form; source flavor text and artwork are not reproduced. '
             'The upgrade catalogue contains our independently authored adaptations, not these source items.', '',
             'The [wiki relic list](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Relics_List) reports 298 entries. '
             'Direct wiki requests returned 403, so indexed wiki pages were used alongside the '
             '[pinned Spire Codex stable data](https://raw.githubusercontent.com/ptrlrd/spire-codex/05dbf2eb917d754ab28360b598a0c82931a309ef/data/eng/relics.json). '
             'That community snapshot is version 0.107.1, from 19 June 2026. Dowsing Rod and Neow’s Sacrifice were added in the '
             '[0.109.0 beta](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:V0.109.0_-_Beta_Patch). '
             'Their presence explains the two-entry difference; later beta versions also alter some existing effects.', '',
             '**Coverage limit:** all 298 names have a functional reference summary, but the 296 stable definitions are not represented as '
             'exhaustively checked against the latest beta. Branch/version metadata and known variants are preserved in the '
             '[structured inventory](../data/sts2-relic-reference.json). Source categories include the placeholder and deliberately weak/event-only variants; '
             'the breadth calculation excludes Circlet and adjusts five source characters to our four.', '',
             '## Category counts', '', '| Source category | Count |', '| --- | ---: |']
    lines += [f'| {rarity} | {count} |' for rarity, count in sorted(Counter(r['rarity'] for r in refs).items())]
    lines += ['', '## Definitions', '', '| Relic / source | Category | Character | Mechanical summary | Branch |', '| --- | --- | --- | --- | --- |']
    for r in sorted(refs, key=lambda x: x['name'].casefold()):
        mechanics = r['mechanics']
        if isinstance(mechanics, list):
            mechanics = '; '.join(map(str, mechanics))
        elif isinstance(mechanics, dict):
            mechanics = '; '.join(f'{k}: {v}' for k, v in mechanics.items())
        url = r.get('wiki_url') or r['source_url']
        lines.append(f"| [{md_text(r['name'])}]({url}) | {md_text(r['rarity'])} | {md_text(r.get('character', 'Shared'))} | {md_text(mechanics)} | {md_text(r.get('branch', 'Stable snapshot'))} |")
    lines += ['', '## Provenance and observed variants', '',
              'The structured record below preserves the research metadata. A wiki URL identifies the corresponding relic page; '
              'it does not imply that every individual page was directly fetched successfully.', '', '```json',
              json.dumps({k: v for k, v in metadata.items() if k != 'relics'}, ensure_ascii=False, indent=2), '```', '']
    return '\n'.join(lines)


def render_markdown(rows: list[dict], refs: list[dict], report: dict) -> str:
    ref_by_id = {r['id']: r for r in refs}
    lines = ['# Permanent upgrade catalogue', '', '18 September 2026 · **288 proposed upgrades** · Four mercenaries · Three city tiers', '',
             'Use the [searchable catalogue](UPGRADE-CATALOGUE.html) to filter by rarity, mercenary, source and city. '
             'Read the [system contract](UPGRADE-SYSTEM.md) for shared timing, part types, sale prices and stacking; '
             '[Mayors](MAYORS.md) explains city-entry offers. The data files are authoritative for these draft entries; '
             'this document is generated by `analysis/upgrade_catalogue.py`.', '',
             'The owner selected relic-like upgrades, scoped rule exceptions, rarity parity and increasingly strong Mayor gifts. '
             'Every individual effect and number below remains a proposal awaiting gameplay balancing. '
             'Nothing here replaces an existing innate mercenary ability or changes a recipe row.', '',
             '## Pool size and rarity', '',
             '| Pool | Upgrades |', '| --- | ---: |',
             '| Shared ordinary / shop / Mystery | 150 |', '| Ordinary mercenary-specific | 36 — 9 each |',
             '| Mayor gifts | 102 — 34 per city tier |', '| Total | **288** |', '',
             'Mayor pools also contain mercenary-specific entries. Eligibility and city bounds are printed on every row. '
             'The new pool counts acquired items; the four innate character kits remain separate.', '',
             '| Power rarity | Count |', '| --- | ---: |']
    lines += [f"| {rarity} | {report['rarity'].get(rarity, 0)} |" for rarity in RARITIES]
    lines += ['', 'Common/Uncommon/Rare source tiers map directly. Shop/Event/Starter are retained as source categories with an explicit adapted rarity rationale. '
              'Ancient sources become Rare/Legendary Mayor gifts. Rarity is distinct from acquisition route and price.', '',
              '## Source coverage', '',
              'The reference inventory contains **298 named entries**: a complete 296-entry stable snapshot plus two indexed wiki beta additions. '
              'Scaling removes the one nonfunctional placeholder and nine slots for the fifth source character: **298 − 1 − 9 = 288**. '
              'This is a breadth target, not a claim that four characters require proportionally less shared content. '
              'See the [reference inventory](research/2026-09-18-relic-reference.md) for version boundaries, mechanical summaries and links. '
              'Direct wiki fetching returned 403; indexed wiki material and the pinned public data mirror were used. '
              'We do not claim that every stable definition was reverified against every later beta patch.', '',
              'The two agents reviewed shared effects, character effects and Ancient adaptations together. Their '
              '[shared-pool study](research/2026-09-18-shared-relics.md) and '
              '[character/Mayor study](research/2026-09-18-character-mayor-relics.md) record source coverage, decisions and revisions.', '']
    groups = [('Shared upgrades', [r for r in rows if not mayor(r) and r['mercenary'] == 'Shared'])]
    groups += [(f'{merc} upgrades', [r for r in rows if not mayor(r) and r['mercenary'] == merc]) for merc in MERCENARIES]
    groups += [(f'City {city} Mayor upgrades', [r for r in rows if mayor(r) and r['min_city'] == city]) for city in range(1, 4)]
    for title, group in groups:
        lines += [f'## {title}', '']
        for r in group:
            city = str(r['min_city']) if r['min_city'] == r['max_city'] else f"{r['min_city']}–{r['max_city']}"
            lines += [f"### {r['id']} — {r['name']}", '',
                      f"**{r['rarity']} · {r['mercenary']} · {' / '.join(r['acquisition'])} · City {city}**", '',
                      r['effect'], '', f"**Purpose:** {r['strategy']}", '',
                      f"**Rule exception:** {r['default_exception']}", '',
                      f"**Rarity:** {r['source_rarity']} source. {r['rarity_basis']}", '',
                      f"**Balance watch:** {r['balance_risk']}", '']
            sources = ', '.join(f"[{ref_by_id[s['id']]['name']}]({s['url']})" for s in r['inspiration'])
            lines += [f'**Inspiration:** {sources}.', '']
    return '\n'.join(lines).rstrip() + '\n'


def render_html(rows: list[dict], report: dict) -> str:
    payload = json.dumps(rows, ensure_ascii=False).replace('<', '\\u003c')
    return '''<!doctype html>
<html lang="en"><meta charset="utf-8"><meta name="viewport" content="width=device-width,initial-scale=1">
<title>Overkill Foundry · Upgrades</title>
<style>
:root{color-scheme:dark;--bg:#11171a;--panel:#1c2529;--text:#eaf0ee;--muted:#b5c2c4;--line:#35464c;--accent:#edba64}
*{box-sizing:border-box}body{margin:0;background:var(--bg);color:var(--text);font:16px/1.55 system-ui,sans-serif}
header,main{max-width:1440px;margin:auto;padding:30px 36px}header{border-bottom:1px solid var(--line)}
.eyebrow{color:var(--accent);font-size:12px;letter-spacing:.18em;text-transform:uppercase}h1{font-size:clamp(30px,4vw,50px);line-height:1.1;margin:12px 0}p{max-width:950px}.muted{color:var(--muted)}a{color:var(--accent)}
.numbers{display:flex;gap:28px;flex-wrap:wrap;margin-top:24px}.numbers strong{display:block;font-size:28px;color:var(--accent)}
.filters{display:grid;grid-template-columns:2fr repeat(4,1fr);gap:12px;position:sticky;top:0;background:var(--bg);padding:16px 0;z-index:2}
label{font-size:12px;color:var(--muted)}input,select{display:block;width:100%;margin-top:5px;padding:12px;background:var(--panel);color:var(--text);border:1px solid var(--line);border-radius:6px;font:inherit;font-size:15px}
input:focus,select:focus{outline:2px solid var(--accent)}#count{margin:8px 0 22px;color:var(--muted)}
#grid{display:grid;grid-template-columns:repeat(3,minmax(0,1fr));gap:18px}.card{padding:22px;background:var(--panel);border:1px solid var(--line);border-top:3px solid var(--accent);border-radius:8px}
.card h2{font-size:21px;line-height:1.2;margin:8px 0 15px}.meta{font-size:12px;color:var(--accent);letter-spacing:.03em}.id{color:var(--muted);font-size:11px}.card p{font-size:14px;margin:14px 0}.purpose{color:var(--muted)}details{font-size:13px;border-top:1px solid var(--line);padding-top:12px}summary{cursor:pointer;color:var(--accent)}.empty{padding:30px;border:1px solid var(--line);grid-column:1/-1}footer{color:var(--muted);font-size:12px;padding:30px 0}
@media(max-width:1100px){#grid{grid-template-columns:repeat(2,minmax(0,1fr))}.filters{grid-template-columns:2fr 1fr 1fr}.filters label:first-child{grid-column:1/-1}}
@media(max-width:650px){header,main{padding:22px 18px}#grid{grid-template-columns:1fr}.filters{position:static;grid-template-columns:1fr 1fr}}
</style>
<header><div class="eyebrow">Overkill Foundry / Design library / 18 September 2026</div>
<h1>Permanent upgrades</h1><p class="muted">High-tech equipment, build engines and deliberate exceptions to the rules. Every effect and number here is a proposal for review and beta balancing.</p>
<div class="numbers"><div><strong>288</strong>upgrade designs</div><div><strong>4</strong>mercenaries</div><div><strong>102</strong>Mayor gifts</div><div><strong>3</strong>city tiers</div></div>
<p><a href="UPGRADE-SYSTEM.md">System rules</a> · <a href="MAYORS.md">Mayor progression</a> · <a href="research/2026-09-18-relic-reference.md">298-entry source inventory</a></p>
<p class="muted">Normal rarities follow their source tier. Ancient-inspired Mayor gifts are Rare or Legendary. Existing innate abilities remain separate.</p></header>
<main><div class="filters"><label>Search name, effect or reference<input id="query" type="search" placeholder="Shield retention, claw, cooling…"></label>
<label>Mercenary<select id="merc"><option value="">All characters</option><option>Shared</option><option>Mara</option><option>Ivo</option><option>Ada</option><option>Noor</option></select></label>
<label>Rarity<select id="rarity"><option value="">All rarities</option><option>Common</option><option>Uncommon</option><option>Rare</option><option>Legendary</option></select></label>
<label>Source<select id="source"><option value="">All sources</option><option>Mayor</option><option>Officer</option><option>Shop</option><option>Mystery</option></select></label>
<label>City tier<select id="city"><option value="">All cities</option><option value="1">City 1</option><option value="2">City 2</option><option value="3">City 3</option></select></label></div>
<div id="count" aria-live="polite"></div><section id="grid" aria-label="Upgrade catalogue"></section>
<footer>Design evidence only. No gameplay build, price calibration or playtested balance is claimed. Source inventory separates a pinned stable snapshot from wiki beta additions. Selecting a character includes shared upgrades.</footer></main>
<script>const rows=''' + payload + ''';
const $=id=>document.getElementById(id);const esc=s=>String(s).replace(/[&<>"']/g,c=>({'&':'&amp;','<':'&lt;','>':'&gt;','"':'&quot;',"'":'&#39;'}[c]));
function draw(){const q=$('query').value.toLowerCase().trim(),m=$('merc').value,r=$('rarity').value,s=$('source').value,c=Number($('city').value);
const found=rows.filter(x=>(!q||JSON.stringify(x).toLowerCase().includes(q))&&(!m||x.mercenary===m||(m!=='Shared'&&x.mercenary==='Shared'))&&(!r||x.rarity===r)&&(!s||x.acquisition.includes(s))&&(!c||(x.min_city<=c&&x.max_city>=c)));
$('count').textContent=found.length+' of '+rows.length+' upgrades'+(m&&m!=='Shared'?' · includes shared items':'');
$('grid').innerHTML=found.map(x=>`<article class="card"><div class="id">${esc(x.id)}</div><div class="meta">${esc(x.rarity)} · ${esc(x.mercenary)} · City ${x.min_city}${x.max_city!==x.min_city?'–'+x.max_city:''}</div><h2>${esc(x.name)}</h2><div class="id">${esc(x.acquisition.join(' / '))}</div><p>${esc(x.effect)}</p><p class="purpose">${esc(x.strategy)}</p><details><summary>Rule exception, rarity &amp; source</summary><p><b>Exception:</b> ${esc(x.default_exception)}</p><p><b>Rarity:</b> ${esc(x.source_rarity)} source. ${esc(x.rarity_basis)}</p><p><b>Balance watch:</b> ${esc(x.balance_risk)}</p><p>${x.inspiration.map(z=>`<a href="${esc(z.url)}" target="_blank" rel="noopener">${esc(z.id.replaceAll('_',' '))}</a>`).join(' · ')}</p></details></article>`).join('')||'<div class="empty">No upgrades match. Try clearing a filter.</div>';}
['query','merc','rarity','source','city'].forEach(id=>$(id).addEventListener('input',draw));draw();</script></html>'''


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument('--write', action='store_true', help='Regenerate Markdown and HTML after validation')
    args = parser.parse_args()
    rows, refs = read_all()
    report = validate(rows, refs)
    if args.write:
        (DESIGN / 'UPGRADE-CATALOGUE.md').write_text(render_markdown(rows, refs, report), encoding='utf-8')
        (DESIGN / 'UPGRADE-CATALOGUE.html').write_text(render_html(rows, report), encoding='utf-8')
        (DESIGN / 'research/2026-09-18-relic-reference.md').write_text(render_reference(refs), encoding='utf-8')
    print(json.dumps(report, indent=2))


if __name__ == '__main__':
    main()
