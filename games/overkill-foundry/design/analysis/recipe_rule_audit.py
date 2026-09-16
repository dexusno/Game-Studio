"""Render a manually curated, row-complete recipe audit; never edits recipes.

The assistant read all five pools. Membership below is that semantic review,
not the result of treating keyword matches as rule violations. This program
checks coverage, pins the reviewed source, and renders evidence for discussion.
"""

from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import subprocess


DESIGN = Path(__file__).resolve().parents[1]
ROOT = Path(__file__).resolve().parents[4]
SOURCE = DESIGN / "RECIPE-CATALOGUE.md"
SOURCE_COMMIT = "a0b3e105f93a2a5892aafab6137b13aefc3890c2"
SOURCE_REL = SOURCE.relative_to(ROOT).as_posix()


def finding(key, status, title, rule, reason, ids, focus, review):
    return dict(id=key, status=status, title=title, rule=rule, reason=reason,
                recipes=ids.split(), excerpt_focus=focus, review=review)


FINDINGS = [
    finding("C01", "resolved", "Recipe-granted Shield retention", "R02",
            "The owner authorized analogous fresh-part replacements after approving Spare Metal Brace. These twelve rows now schedule regular Shield parts instead of retaining active protection. Costs, cooldowns and meaningful conditions are preserved. Saved source parts still use their normal activation clock. Last Wall checks leftover Shield before reset for its conditional cooling reward; its future Shield part is a separate fixed grant.",
            "SH057 SH092 SH113 MA048 MA099 IV019 IV057 IV061 IV090 AD074 AD092 NO087",
            r"next round|next two rounds|beginning",
            "Retention conflicts are resolved. Q09 retains standard one-part valuation mapping for the added outputs; numerical balance remains untested."),
    finding("C02", "conflict", "Utilities create or copy physical parts", "R01",
            "These Utilities manufacture or copy arbitrary inventory parts, including delayed production. That output remains unreconciled with the Utility boundary; the owner-approved ordinary Shield-value grant does not by itself approve generic part copying or other manufactured outputs.",
            "SH071 SH076 SH102 SH118 MA107 MA119 IV118 NO107 NO120",
            r"receive|make one|gain 2",
            "Review the intended recipe category and output; no conversion has been applied."),
    finding("C03", "conflict", "Saved, split or sacrificed Utility items", "R01",
            "The effect requires an inventory Utility item, its age, or separate stored activations. Utilities activate on recipe Use and do not create those items.",
            "IV071 IV099 IV105 AD040 AD062 AD097 NO055",
            r"saved|earlier turn|Utility parts|Service Tab|Small Cell",
            "Determine the intended replacement dependency or output. AD062 is newly identified beyond the existing superseded markers."),
    finding("C04", "resolved", "Shield granted during preparation, collection or after Fire", "R03",
            "Resolved by the owner's 16 September clarification: a grant automatically loads an ordinary Shield part worth that amount. It may be removed and saved like any part. End Turn activates parts left loaded; active Shield resets at enemy-turn end without an automatic grant next round. The original audit incorrectly read these grants as immediate active protection.",
            "SH033 SH045 SH064 SH065 SH095 SH104 SH108 SH111 MA004 MA047 MA078 MA101 MA113 MA116 IV011 IV036 IV038 IV066 IV079 IV098 AD024 AD042 AD049 AD054 AD056 AD060 AD068 AD078 AD082 AD103 AD107 AD114 AD116 AD119 AD120 NO036 NO041 NO054 NO092 NO095 NO111",
            r"gain.*Shield|Shield equal|Each trigger grants",
            "MA004 Quick Vent is resolved: automatically load its 5-Shield part, with ordinary removal/storage available. Other findings on these rows remain separate; AD103/AD107/AD119 also have enemy-phase triggers covered by Q01."),
    finding("C05", "resolved", "Explicit future-turn Shield schedules", "R03",
            "Resolved at the timing-rule level by the owner's Folding Brace revision: a recipe may schedule a new ordinary Shield part for the beginning of the next round. It loads then, remains removable/saveable and activates at End Turn if left loaded. This is neither active-Shield retention nor immediate active protection. SH034's expiry wording is now corrected in C10; MA118's active-Shield payment remains in Q02. Other printed values are still draft balance.",
            "SH034 SH096 SH115 MA016 MA040 MA092 MA118 IV059 IV097 IV112 IV120 AD063 AD066 AD098 NO018 NO034 NO056 NO098 NO113",
            r"start|next turn",
            "Interpret an explicitly scheduled Shield grant as a new ordinary part at its stated trigger. An immediate grant alone does not repeat next round. Preserve each row's stated conditions and other unresolved findings."),
    finding("C06", "conflict", "Shield-part activation expected before a shot", "R03",
            "The effect expects Shield parts to have activated in preparation, or arms a later shot trigger only when the Shield part activates. Under End-Turn-only activation there is no later ordinary Fire in that player turn.",
            "SH044 SH062 MA034 IV107 AD076",
            r"used at least|next main shot|After this round|planning phase",
            "Review which event these bonuses should measure. Staging a part and activating it are already different events."),
    finding("C07", "conflict", "Shield conversion at the obsolete reset point", "R02",
            "NO060 schedules a next-turn action 'before clearing or retention', although the default reset has already occurred at enemy-turn end. It converts leftover protection into Charge rather than granting a fresh Shield part, so the authorized fresh-part replacement does not automatically select its conversion timing.",
            "NO060", r"before clearing",
            "Review NO060's conversion trigger against the settled enemy-turn-end reset. SH034's former expiry issue is resolved in C10."),
    finding("C08", "conflict", "Removed Charged Barrel payment", "R04",
            "The row refers to a separate barrel Charge payment at Fire, which belonged to the superseded independent gun-damage trait. Positive-payment branches lose their trigger, no-payment branches become automatic, and 'after payment' or exclusion clauses refer to a removed operation.",
            "NO005 NO006 NO017 NO025 NO028 NO035 NO039 NO048 NO058 NO075 NO081 NO106 NO110",
            r"barrel|Charged Barrel",
            "Review the affected condition or refund without restoring an independent gun-damage payment. This does not depend on accepting Residual Current's proposed numbers."),
    finding("C09", "resolved", "Folding Brace owner-selected replacement", "R03",
            "SH026's old 6 Shield plus retention of up to 4 is replaced by 4 Shield loaded on recipe Use and a new 6-Shield part loaded at the beginning of the next round. Both are ordinary parts. The second delivery is scheduled by recipe Use, regardless of whether the first part is activated or saved. Cost and cooldown are unchanged; the proposed flat 10 Shield was rejected.",
            "SH026", r"On recipe Use",
            "The retention conflict is resolved. Q09 retains the normal one-part resale-basis mapping work for the two generated outputs; no price has been invented."),
    finding("C10", "resolved", "Early Cover uses ordinary Shield-part expiry", "R03",
            "SH034 now schedules a regular 8-Shield part for the beginning of the next round. It loads then and may be removed or saved; if left loaded, End Turn activates it and active Shield resets at enemy-turn end. The obsolete following-turn expiry clause was removed without changing its cost, cooldown or Shield amount.",
            "SH034", r"On recipe Use",
            "The old expiry conflict is resolved under the same ordinary-part rule."),
    finding("Q01", "clarification", "Shield replenishment during enemy actions", "R03",
            "These gains can happen after End Turn's Shield activation, through an ongoing effect or a Bolt-disable trigger during enemy actions. The ordinary-part grant rule does not yet specify activation when End Turn has already resolved. Do not silently make such a grant active protection or a fresh automatic next-round Shield balance.",
            "MA044 MA071 MA090 MA115 IV021 NO042 NO083 NO117 AD103 AD107 AD119",
            r"gain 6 Shield after|gain 12 Shield|after|restore",
            "Review secondary Shield replenishment as one group, keeping it separate from preparation-time grants and next-turn retention."),
    finding("Q02", "clarification", "Preparation-time active-Shield dependencies", "R03",
            "The recipe spends or checks active Shield while preparing a shot. Ordinary staged Shield cannot supply it. An explicit permanent-upgrade grant/retention can make the effect legal, so this is a build dependency to review, not an automatic rule violation.",
            "SH085 MA027 MA037 MA061 MA068 MA081 MA112 MA118 AD095 NO010 NO079 NO116",
            r"Shield",
            "Check whether the intended recipe should require an active-Shield upgrade. Do not confuse pending parts with an available Shield balance."),
    finding("Q03", "clarification", "Order within End Turn's Shield activation", "R03",
            "The value or legality depends on another Shield part having activated, or on Shield accumulated so far. Values add at End Turn, but the owner has not selected ordering/payment validation for these interdependent Shield parts.",
            "MA028 MA075 IV094 NO029 NO038",
            r"already|current Shield|other Shield",
            "Establish the activation-order reading needed by these five rows; this also informs Heat/Charge payments across a staged Shield build."),
    finding("Q04", "clarification", "Remaining Shield sampled at enemy-phase end", "R02",
            "The bonus reads or spends remaining Shield at/after enemy-phase end, where the selected reset also occurs. A precise snapshot/trigger ordering is needed; the recipe does not necessarily retain Shield.",
            "SH066 SH091 MA055 IV092 NO044",
            r"end of the enemy|After this enemy|remaining Shield|end of this enemy",
            "Review the timing of the read/payment relative to reset without changing the reset itself."),
    finding("Q05", "clarification", "Utility item wording left after conversion", "R01",
            "The immediate effect is usable without an inventory item, but leftover 'pack crafted' or 'copies of its part' wording describes the former Utility-item model. Unlike C03, the main effect does not require storing the Utility.",
            "SH029 SH030 SH031 SH074", r"crafted|copies of its part",
            "Review wording/cap ownership; no numerical retuning is implied by this finding."),
    finding("Q06", "clarification", "Recipe crafting versus immediate Utility use", "R01",
            "The effect counts crafting, crafting costs or a crafted numeric-cooldown recipe. It does not explicitly say whether immediate Utility recipe Uses count. Part-producing crafts and all recipe Uses are different sets after the Utility conversion.",
            "SH023 SH070 MA069 NO062 NO071 NO109", r"craft",
            "Make the counted event explicit. Existing clauses that explicitly count physical parts can remain narrower."),
    finding("Q07", "clarification", "Cooling rewards restricted to parts", "R01",
            "The reward counts cooling performed by a part rather than by a Utility. That can be intentional: NO057 is an Ammo part that really cools recipes. Consequently NO094's old superseded marker is not enough to prove a contradiction.",
            "NO080 NO094", r"parts this round|with a part",
            "Review intended eligible sources. Do not automatically broaden the reward to Utilities or mistake this for non-global cooling."),
    finding("Q08", "clarification", "Status recipient implied rather than stated", "R05",
            "The effect names a main hit/target condition but omits the recipient of one status application. The owner requires recipe-defined status scope, especially when a shot has extra targets. This is a wording finding, not a proposal that the status should affect everyone.",
            "SH013 SH014 SH016 SH055 SH086 IV006 IV008 IV015 IV016 IV041 IV047 IV084 IV085 AD038",
            r"apply",
            "Make the intended recipient explicit when reviewing these rows; no blanket main-target or all-hit default has been applied."),
    finding("Q09", "clarification", "Canonical sale basis for batches and generated subparts", "R06",
            "The recipe creates multiple parts or a generated subpart without a recorded standard one-part production requirement for every resulting type. Preserve main-recipe-based resale; do not substitute the discounted producer's cost or divide a mixed batch arbitrarily.",
            "SH026 SH057 SH076 SH092 SH113 SH126 MA038 MA048 MA099 MA107 MA119 IV019 IV057 IV061 IV088 IV090 IV118 AD017 AD074 AD083 AD092 AD117 NO031 NO063 NO087 NO107 NO120",
            r"Each|receive|Make one|gain 2",
            "Record canonical one-part valuation references/requirements for these outputs. Known fresh copies inherit their original type's basis; generic copying alone is not a pricing conflict."),
    finding("Q10", "clarification", "Spread Modifier lifecycle and placement order", "R07",
            "The row is a Modifier used/consumed in planning, while the selected spread rule resolves contributions in their parts' bullet-placement order and Load remains reversible. The catalogue still leaves non-Ammo lifecycle/accounting open; the ordering record and reservation behaviour need specifying.",
            "SH005 SH051 SH082 SH110 MA041 MA111 IV076 NO050 NO082 NO115",
            r"shot|Fire|extra enemy",
            "Define how these consumed Modifier effects retain the player's chosen bullet order through Load/unload. This is not permission to refund resolved effects."),
]

RULES = {
    "R01": "Utilities activate on recipe Use without a stored Utility item. The owner explicitly allows Shield grants to create automatically loaded ordinary Shield parts, removable and saveable like any other part. This does not approve arbitrary reserve-part manufacturing or saved Utility items.",
    "R02": "Active Shield resets at enemy-turn end by default; only explicit permanent upgrades provide retention exceptions.",
    "R03": "Only End Turn activates prepared Shield. Recipe grants automatically load ordinary Shield parts worth their granted amount; they may be removed and saved. An explicitly scheduled future grant loads a new part at its stated trigger, as selected for Folding Brace. Active Shield resets after the enemy turn; an immediate grant does not automatically repeat. Unused-part storage differs from active-Shield retention. Explicit upgrade exceptions keep their timing; enemy-phase secondary-effect accounting remains partly draft.",
    "R04": "The gun contributes zero innate damage/effects. Old Hot Barrel/Charged Barrel independent damage grants are superseded. Recipe-authored effects and explicit Utility bonuses are distinct from gun base damage.",
    "R05": "Each recipe specifies its own status recipients; there is no blanket main-target-only or all-hit-target default.",
    "R06": "A part's sale value uses the main recipe's normal one-part ingredient requirement, current resource prices, a 50% factor and whole-credit floor. Discounts/copies do not change that basis.",
    "R07": "Spread contributions are separate hits resolved in contributing-part placement order. Load can be undone before Fire; resolved effects/crafting are not automatically refunded.",
}

CHECKED_DIMENSIONS = [
    "output kind and Utility lifecycle", "Shield activation, reset and grants",
    "per-copy cooldown, global cooling and no-cooldown uses",
    "part-based damage, multiple shots and modifier duration",
    "status recipients, hit order, explicit copy/stack limits",
    "whole-number arithmetic and player HP costs/death",
    "resource/part persistence, collection count and Precision protection",
    "part consumption, saved-part age and canonical sale basis",
    "removed character dependencies; proposed traits are not owner rules",
]


def read_rows():
    text = SOURCE.read_text(encoding="utf-8")
    original = subprocess.check_output(
        ["git", "show", f"{SOURCE_COMMIT}:{SOURCE_REL}"], cwd=ROOT
    ).decode("utf-8").replace("\r\n", "\n")
    row_pattern = r"^\| (?:SH|MA|IV|AD|NO)\d{3}\s*\|.*$"
    if re.findall(row_pattern, text, re.M) != re.findall(row_pattern, original, re.M):
        raise SystemExit("Recipe rows changed since semantic review. Re-audit affected rows before regenerating.")
    rows = []
    for number, line in enumerate(text.splitlines(), 1):
        if not re.match(r"^\| (SH|MA|IV|AD|NO)\d{3}\s*\|", line):
            continue
        cells = [cell.strip() for cell in line.strip("|").split("|")]
        assert len(cells) == 7
        keys = ("id", "name", "output", "kind", "cost", "cooldown", "effect")
        rows.append(dict(zip(keys, cells), source_line=number, source_row=line))
    expected = {f"{pool}{i:03}" for pool, n in (("SH", 126), ("MA", 120),
                ("IV", 120), ("AD", 120), ("NO", 120)) for i in range(1, n + 1)}
    assert len(rows) == 606 and {r["id"] for r in rows} == expected
    return text, rows


def excerpt(row, group):
    effect = re.sub(r"^\*\*Superseded[^*]+\*\*\s*", "", row["effect"])
    sentences = re.split(r"(?<=[.!?])\s+", effect)
    chosen = next((s for s in sentences if re.search(group["excerpt_focus"], s, re.I)), effect)
    # Quotes are verbatim; shortening is visibly indicated.
    if len(chosen) > 280:
        chosen = chosen[:280].rsplit(" ", 1)[0] + "…"
    assert chosen.rstrip("…") in row["effect"]
    return chosen


def render():
    text, rows = read_rows()
    by_id = {r["id"]: r for r in rows}
    group_by_id = {g["id"]: g for g in FINDINGS}
    assert len(group_by_id) == len(FINDINGS)
    for g in FINDINGS:
        assert len(g["recipes"]) == len(set(g["recipes"]))
        assert set(g["recipes"]) <= by_id.keys() and g["rule"] in RULES
    for row in rows:
        ids = [g["id"] for g in FINDINGS if row["id"] in g["recipes"]]
        row["finding_ids"] = ids
        row["resolved_finding_ids"] = [i for i in ids if group_by_id[i]["status"] == "resolved"]
        row["status"] = ("conflict" if any(group_by_id[i]["status"] == "conflict" for i in ids)
                         else "clarification" if any(group_by_id[i]["status"] == "clarification" for i in ids)
                         else "no_direct_conflict_identified")
        row["review"] = "Reviewed against the recorded rule dimensions; not an approval or balance result."
    totals = Counter(r["status"] for r in rows)
    ledger = {
        "audit_date": "2026-09-17", "initial_audit_date": "2026-09-16", "source_commit": SOURCE_COMMIT,
        "rule_revision": "2026-09-17 integration of the authorized analogous replacements: twelve remaining retention recipes grant fresh regular parts; SH034 follows ordinary expiry. Existing conditions, costs and cooldowns preserved.",
        "source_commit_scope": "Pins all 606 printed recipe rows after the analogous Shield batch: 13 rows changed in this pass, 593 unchanged, including the earlier SH026 replacement.",
        "source_path": SOURCE_REL,
        "source_sha256_normalized_utf8": hashlib.sha256(text.encode()).hexdigest(),
        "method": "Assistant semantic reading of all 606 rows; manually curated findings; mechanical coverage and source-preservation checks.",
        "checked_dimensions": CHECKED_DIMENSIONS, "rules": RULES,
        "counts": dict(totals), "findings": FINDINGS, "recipes": rows,
        "limitations": "Not runtime verification, a balance simulation, an exhaustive search of recipe combinations, or owner acceptance of replacement designs.",
    }
    (DESIGN / "analysis/recipe_rule_audit.json").write_text(json.dumps(ledger, indent=2) + "\n", encoding="utf-8")

    lines = ["# Recipe rule audit — review together", "",
             "**17 September 2026 · all 606 recipes reviewed · 13 analogous Shield corrections this pass; other 593 rows unchanged**", "",
             f"Reviewed all recipe rows from `{SOURCE_COMMIT[:7]}` against the settled owner rules, updated for the authorized analogous Shield replacements. "
             f"Found **{totals['conflict']} recipes with a definite conflict or obsolete dependency**, "
             f"**{totals['clarification']} additional recipes needing wording/dependency clarification**, and "
             f"**{totals['no_direct_conflict_identified']} with no direct conflict identified**. "
             "A recipe can have findings in several groups; group counts therefore overlap. Clarifications on a conflicting recipe are also retained.", "",
             "**Owner resolutions:** C04/C05 use ordinary Shield parts and explicit future deliveries. Following Folding Brace, the owner approved Spare Metal Brace's 8-now/conditional-5-next-round schedule and authorized analogous replacements. All twelve remaining C01 retention recipes now deliver fresh parts under their stated conditions; SH034's old expiry is resolved in C10. End Turn activates parts left loaded, and active Shield resets at enemy-turn end. Parts remain removable/saveable; a delivery does not copy the source's delivery or secondary effects.", "",
             "This is an audit for joint review. This pass changed the twelve C01 rows plus SH034, preserving their costs/cooldowns and the other 593 rows. The earlier owner-selected SH026 replacement remains unchanged. "
             "The four new inherent abilities remain proposals; their values are not treated as owner rules. "
             "No direct conflict identified means the row passed this written-rule review, not that it is implementation-ready, balanced or proven in every combination.", "",
             "**Resolved starter:** MA004 Quick Vent automatically loads its 5-Shield part; the player may use it at this End Turn or remove and save it. "
             "SH005 Split Outlet still needs Q10 spread-Modifier lifecycle clarification. "
             "**Next joint review:** NO060 Field Pocket in C07 converts Shield into Charge at the obsolete reset point and needs a distinct timing correction.", "",
             "## Coverage", "", "| Pool | Reviewed | Conflict | Clarification only | No direct conflict identified |",
             "| --- | ---: | ---: | ---: | ---: |"]
    for pool in ("SH", "MA", "IV", "AD", "NO"):
        selected = [r for r in rows if r["id"].startswith(pool)]
        c = Counter(r["status"] for r in selected)
        lines.append(f"| {pool} | {len(selected)} | {c['conflict']} | {c['clarification']} | {c['no_direct_conflict_identified']} |")
    lines += ["", "All rows were read for: " + "; ".join(CHECKED_DIMENSIONS) + ".", "",
              "The [complete per-recipe ledger](analysis/recipe_rule_audit.json) contains every ID, name, original row, source line, disposition and finding references. "
              "The [audit renderer](analysis/recipe_rule_audit.py) verifies that all printed recipe rows still match the reviewed revision before regenerating this document. Rule prose may change with recorded owner clarifications; the ledger hashes the current full catalogue. "
              "It validates coverage and evidence preservation; it does not discover or prove semantic conflicts automatically.", "",
              "## Findings index", "", "| Group | Classification | Recipes | Topic |", "| --- | --- | ---: | --- |"]
    for g in FINDINGS:
        lines.append(f"| [{g['id']}](#{g['id'].lower()}) | {g['status']} | {len(g['recipes'])} | {g['title']} |")
    lines += ["", "## Rule references", "",
              "These summaries refer to the selected-rule sections in [RECIPE-CATALOGUE.md](RECIPE-CATALOGUE.md) "
              "and the owner chronology in [DECISIONS.md](../DECISIONS.md). Draft conventions are used to explain dependencies, not promoted into owner decisions.", ""]
    for key, rule in RULES.items():
        lines.append(f"- **{key}:** {rule}")
    for g in FINDINGS:
        lines += ["", f"## {g['id']}", "", f"**{g['title']} — {g['status']} · {g['rule']}**", "",
                  g["reason"], "", "**For our review:** " + g["review"], "",
                  "Exact excerpts below; full effects and costs remain in the catalogue and ledger.", "",
                  "| Recipe | Existing wording |", "| --- | --- |"]
        for recipe_id in g["recipes"]:
            r = by_id[recipe_id]
            quote = excerpt(r, g).replace("|", "\\|")
            lines.append(f"| **{r['id']} — {r['name']}** | {quote} |")
    lines += ["", "## Cases deliberately not called rule conflicts", "",
              "- Percentages, half-values and multipliers remain legal: the selected integer arithmetic floors their results. They are not automatically fractional-damage violations.",
              "- The seven player-HP-cost recipes already state survival at 1 HP. Bolt HP costs and disablement are separate from player death. No additional player-HP-cost conflict was found.",
              "- A numeric cooldown can be cleared and reused in the same turn. Self-cooling is allowed; identical effects or strong resource loops are balance concerns, not automatically violations. No new targeted-cooling or no-cooldown-refresh violation was found.",
              "- Per-recipe damage/effect caps, minimum part requirements and explicit non-stacking clauses do not impose a global shot or part-count cap.",
              "- A Utility may mark or sacrifice a real Ammo/Shield/Helper part already in reserve. That is different from requiring a nonexistent Utility part. MA030, MA066 and IV102 illustrate legal dependencies on real parts.",
              "- Explicit on-kill transfers or newly triggered support attacks differ from automatically redirecting an already assigned hit whose target died. No new forced-retargeting contradiction was identified.",
              "- Start-of-next-turn resource deliveries and temporary collection enhancements follow the existing fight-end expiry rule. They do not create an extra ordinary haul or another Precision attempt.",
              "- SH060's explicit loss of remaining Shield is not a recipe retention exception; it can be a downside when an upgrade would otherwise preserve Shield. A weaker or redundant effect is not itself a rule conflict.",
              "- The existing same-effect cooling pairs and relative recipe prices remain balance work. Renaming Burn/Corrosion/Mark/Weaken to the proposed robot vocabulary is a separate editorial migration.", "",
              "## Shared specification gaps, not 606 separate questions", "",
              "Modifier/Helper lifecycle, Shield-part payment ordering, final secondary-effect timing and precise stat sampling are still partly draft. "
              "Q03/Q04/Q10 identify concrete rows that expose these gaps. The owner resolved C04 through ordinary Shield-part grants; further clarification must preserve End Turn and the active-Shield reset. "
              "Main-shot singular wording is generally readable through the existing next-shot/default-duration rules; it is not automatically a one-shot-per-turn restriction. "
              "This review does not label every ordinary row as conflicting merely because the eventual engine still needs an effect-resolution order.", "",
              "**Next action:** review remaining groups with Klaus, starting with C07/NO060's Shield-to-Charge conversion. Record chosen replacements before changing affected rows and re-auditing them. "
              "The analogous Shield-part replacements are applied; unrelated replacements and gameplay implementation are not implied.", ""]
    (DESIGN / "RECIPE-RULE-AUDIT.md").write_text("\n".join(lines), encoding="utf-8")
    print(json.dumps({"reviewed": len(rows), "counts": dict(totals),
                      "groups": {g['id']: len(g['recipes']) for g in FINDINGS}}, indent=2))


if __name__ == "__main__":
    render()
