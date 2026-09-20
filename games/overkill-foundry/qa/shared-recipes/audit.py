"""Independent frozen-link/source inventory audit, not a semantic test oracle."""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import re

GAME = Path(__file__).resolve().parents[2]
RECORDS = GAME / 'core/build/qa-shared-recipes/frozen-9b4b7a2b/records'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path):
    return json.loads(path.read_text(encoding='utf-8'))


def need(condition, message):
    if not condition:
        raise AssertionError(message)


def encoded(value):
    return (json.dumps(value, indent=2, sort_keys=True, ensure_ascii=False) + '\n').encode('utf-8')


def links(record, proposal, rows, archive):
    actual = {'contracts::' + line[5:] for line in (archive / 'contracts.log').read_text(encoding='utf-8').splitlines() if line.startswith('PASS ')}
    recorded = {row['id'] for row in record['tests'] if row['result'] == 'passed'}
    need(len(actual) == 113 and actual == recorded and len(record['tests']) == 113, 'Named PASS linkage')
    need(proposal['verified_build'] == record['verified_build'], 'Build linkage')
    need(set(proposal['bindings']) | set(proposal['unbound']) == set(rows), 'Obligation partition')
    need(not (set(proposal['bindings']) & set(proposal['unbound'])), 'Overlapping residual')
    need(set(proposal['unbound']) == {'recipe:SH064'}, 'Original residual changed')
    for key, binding in proposal['bindings'].items():
        need(binding['source_contract'] == rows[key], 'Cited source contract changed')
        need(binding['tests'] and set(binding['tests']) <= actual, 'Missing named PASS')
        need(any(test.split(' ', 1)[0] == 'contracts::' + key for test in binding['tests']), 'Missing primary ID group')
        for item in binding['implementations']:
            need(record['inputs'][item['path']] == item['sha256'], 'Implementation identity mismatch')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    record, proposal = read(RECORDS / 'results.json'), read(RECORDS / 'proposed-bindings.json')
    archive = GAME / record['archive']
    source = archive / 'inputs'
    need(len(record['inputs']) == 48, 'Captured input count')
    for path, digest in record['inputs'].items():
        need(sha(source / path) == digest, 'Frozen input changed: ' + path)
    need(hashlib.sha256(encoded(record['inputs'])).hexdigest() == record['graph_sha256'], 'Source graph digest')
    for path, digest in record['archive_artifacts'].items():
        need(sha(archive / path) == digest, 'Frozen artifact changed: ' + path)
    need(len(record['archive_artifacts']) == 8, 'Artifact count')
    need(proposal['evidence']['sha256'] == sha(RECORDS / 'results.json'), 'Proposal evidence digest')
    for execution in record['executions']:
        need(sha(RECORDS / execution['log']) == execution['log_sha256'] == sha(archive / execution['log']), 'Log evidence digest')
        need(sha(archive / Path(execution['executable']).name) == execution['executable_sha256'], 'Binary evidence digest')
        need(execution['exit_code'] == 0, 'Execution failure')

    rows = read(source / 'core/tests/shared-recipes/source-contracts.json')
    catalogue = read(source / 'content/catalogue.snapshot.json')['recipes']
    coverage = read(source / 'content/runtime-evidence/coverage.json')['decisions']
    expected = {key for key, row in coverage.items() if key.startswith('recipe:SH') and row['status'] == 'unbound'}
    need(set(rows) == expected and len(rows) == 101, 'Historical Shared obligation inventory')
    markdown = (source / 'design/RECIPE-CATALOGUE.md').read_text(encoding='utf-8').splitlines()
    rarity = None
    parsed = {}
    for number, line in enumerate(markdown, 1):
        heading = re.match(r'^### (Base|Common|Uncommon|Rare|Legendary)(?:\s|$)', line)
        if heading:
            rarity = heading.group(1)
        if not re.match(r'^\| SH\d{3} \|', line):
            continue
        fields = [cell.strip() for cell in line.split('|')[1:-1]]
        parsed['recipe:' + fields[0]] = (fields, number, line, rarity)
    for key, row in rows.items():
        fields, number, line, rarity = parsed[key]
        need(row == catalogue[row['id']], 'Catalogue row differs')
        need([row['id'], row['name'], row['output_text'], row['kind'], row['cost_text'], row['cooldown_text'], row['effect']] == fields, 'Printed source fields differ')
        need(row['source']['line'] == number and row['source']['exact_text'] == line, 'Exact source location differs')
        need(row['rarity'] == rarity, 'Rarity heading mismatch')
        materials = {material: int(amount) for amount, material in re.findall(r'(\d+) (Iron|Copper|Carbon|Glass|Circuit)', fields[4])}
        need(row['materials'] == materials, 'Parsed source ingredients differ')
        need(row['cooldown'] == (None if fields[5] == 'None' else int(fields[5])), 'Printed cooldown differs')
    links(record, proposal, rows, archive)
    log = (archive / 'contracts.log').read_text(encoding='utf-8')
    need('SUMMARY 113 passed, 0 failed, 27881 assertions' in log, 'Contract summary differs')

    history = read(source / 'core/tests/shared-recipes/history/delayed-shield-schedules.json')
    old = GAME / history['archive']
    for path, digest in history['inputs'].items():
        need(sha(old / 'inputs' / path) == digest, 'Failed history input changed')
    need(sha(old / 'shared_recipe_contract_tests.exe') == history['executable_sha256'], 'Failed binary changed')
    need(sha(old / 'failed.log') == history['log_sha256'], 'Failed log changed')
    need(history['exit_code'] == 1 and all(line in (old / 'failed.log').read_text(encoding='utf-8') for line in history['failures']), 'Failed history not preserved')

    controls = []
    for name, alter in [
        ('Missing named result', lambda r, p, c: r['tests'].pop()),
        ('Wrong implementation hash', lambda r, p, c: p['bindings']['recipe:SH084']['implementations'][0].update(sha256='0' * 64)),
        ('Altered printed effect', lambda r, p, c: c['recipe:SH084'].update(effect='Add 99 damage.')),
        ('Fabricated PASS', lambda r, p, c: p['bindings']['recipe:SH084']['tests'].append('contracts::recipe:SH084 invented')),
    ]:
        r, p, c = copy.deepcopy(record), copy.deepcopy(proposal), copy.deepcopy(rows)
        alter(r, p, c)
        try:
            links(r, p, c, archive)
        except AssertionError:
            controls.append(name)
        else:
            raise AssertionError('Negative control accepted: ' + name)
    result = {
        'archive': record['archive'], 'graph_sha256': record['graph_sha256'],
        'verified_inputs': 48, 'verified_artifacts': 8, 'exact_source_rows': 101,
        'named_groups': 113, 'original_proposed_ids': 100,
        'failed_history_verified_inputs': len(history['inputs']),
        'negative_controls_rejected': controls,
        'scope': 'Identity, source inventory and cited PASS linkage only; semantic dispositions are in clause-review.json.',
    }
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_bytes(encoded(result))
    print(json.dumps(result, indent=2))


if __name__ == '__main__':
    main()
