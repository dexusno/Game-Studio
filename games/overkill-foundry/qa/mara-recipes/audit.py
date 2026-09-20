"""Independent frozen Mara inventory, source-clause, fixture and artifact audit."""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import re

GAME = Path(__file__).resolve().parents[2]
RECORDS = GAME / 'core/build/qa-mara/records'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path):
    return json.loads(path.read_text(encoding='utf-8'))


def need(value, reason):
    if not value:
        raise AssertionError(reason)


def graph(sources):
    return hashlib.sha256(''.join(f'{key}\n{value}\n' for key, value in sorted(sources.items())).encode()).hexdigest()


def verify_proposal(record, proposal, contracts, fixtures):
    tests = {row['recipe']: row for row in record['tests']}
    need(len(tests) == len(record['tests']) == 107, 'Actual named inventory')
    need(set(proposal['bindings']) == set(contracts), 'Proposed inventory differs')
    need(proposal['candidate_source_graph_sha256'] == record['source_graph_sha256'], 'Proposal graph differs')
    for key, row in proposal['bindings'].items():
        recipe = key.split(':')[1]
        need(row['status'] == 'proposed_only', 'Proposal activated or silently upgraded')
        test = tests[recipe]
        need(test['result'] == 'passed' and row['test'] == test['id'], 'Missing named PASS')
        need(row['asserted_behavior'] == test['observation'] and row['assertions'] == test['assertions'], 'Named assertion evidence mismatch')
        need(row['source_clause'] == contracts[key]['source'], 'Exact source clause differs')
        titles = [fixture['title'] for fixture in fixtures[recipe]]
        need('; '.join(titles) == test['observation'], 'Claim differs from actual fixture registration')


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    record = read(RECORDS / 'evidence/results.json')
    proposal = read(RECORDS / 'evidence/proposed-bindings.json')
    archive = GAME / record['archive']
    source = archive / 'source'
    need(len(record['sources']) == 37 and graph(record['sources']) == record['source_graph_sha256'], 'Candidate graph differs')
    for path, digest in record['sources'].items():
        need(sha(source / path) == digest, 'Captured source changed: ' + path)
    for path, digest in {**record['archive_artifacts'], **record['executables']}.items():
        need(sha(archive / path) == digest, 'Captured artifact changed: ' + path)
    need(sha(RECORDS / 'residual-branches.json') == record['residuals_sha256'], 'Residual declaration changed')
    clauses = read(source / 'core/tests/mara-recipes/source-contracts.json')
    coverage = read(source / 'content/runtime-evidence/coverage.json')['decisions']
    expected = {key for key, value in coverage.items() if key.startswith('recipe:MA') and value['status'] == 'unbound'}
    need(set(clauses) == expected and len(clauses) == 107, 'Historical Mara inventory differs')
    catalogue = read(source / 'content/catalogue.snapshot.json')['recipes']
    markdown = (source / 'design/RECIPE-CATALOGUE.md').read_text(encoding='utf-8').splitlines()
    rarity = None
    parsed = {}
    for number, line in enumerate(markdown, 1):
        heading = re.match(r'^### (Base|Common|Uncommon|Rare|Legendary)(?:\s|$)', line)
        if heading:
            rarity = heading.group(1)
        if re.match(r'^\| MA\d{3} \|', line):
            fields = [cell.strip() for cell in line.split('|')[1:-1]]
            parsed['recipe:' + fields[0]] = (fields, number, line, rarity)
    for key, row in clauses.items():
        fields, number, text, rarity = parsed[key]
        need(row == catalogue[row['id']], 'Catalogue/source index differs')
        need(fields == [row['id'], row['name'], row['output_text'], row['kind'], row['cost_text'], row['cooldown_text'], row['effect']], 'Printed fields differ')
        need(row['source']['line'] == number and row['source']['exact_text'] == text and row['rarity'] == rarity, 'Source location/heading differs')
        need(row['materials'] == {material: int(value) for value, material in re.findall(r'(\d+) (Iron|Copper|Carbon|Glass|Circuit)', fields[4])}, 'Printed ingredients differ')
        need(row['cooldown'] == (None if fields[5] == 'None' else int(fields[5])), 'Printed cooldown differs')

    fixtures = {}
    extensions = []
    for file in (source / 'core/tests/mara-recipes').glob('*.inl'):
        for number, line in enumerate(file.read_text(encoding='utf-8').splitlines(), 1):
            found = re.match(r'\s*(add|extend)\("(MA\d{3})","([^"]+)"', line)
            if not found:
                continue
            kind, recipe, title = found.groups()
            item = {'path': file.relative_to(source).as_posix(), 'line': number, 'title': title, 'sha256': sha(file), 'kind': kind}
            if kind == 'add':
                need(recipe not in fixtures, 'Duplicate primary fixture')
                fixtures[recipe] = [item]
            else:
                extensions.append((recipe, item))
    for recipe, item in extensions:
        fixtures[recipe].append(item)
    need(len(fixtures) == 107 and len(extensions) == 33, 'Fixture/extension inventory differs')
    log = (archive / 'contracts.log').read_text(encoding='utf-8')
    executed = []
    for line in log.splitlines():
        match = re.match(r'PASS recipe:(MA\d+) assertions=(\d+) (.*)', line)
        if match:
            recipe, count, observation = match.groups()
            executed.append({'id': 'mara-recipe:' + recipe, 'recipe': recipe, 'result': 'passed', 'assertions': int(count), 'observation': observation})
    need(executed == record['tests'] and 'SUMMARY 107 passed, 0 failed, 20526 assertions.' in log, 'Actual log/record differs')
    verify_proposal(record, proposal, clauses, fixtures)

    history_counts = []
    for item in read(RECORDS / 'history/failures.json')['captures']:
        old = GAME / item['archive']
        identity = read(old / 'identity.json')
        need(graph(identity['sources']) == item['source_graph_sha256'], 'Failed historical graph differs')
        for path, digest in identity['sources'].items():
            need(sha(old / 'source' / path) == digest, 'Failed source changed')
        need(sha(old / 'build/Release/mara_recipe_contract_tests.exe') == item['executable_sha256'], 'Failed binary changed')
        need(sha(old / 'contracts.log') == item['contracts_log_sha256'], 'Failed log changed')
        need([row for row in identity['tests'] if row['result'] == 'failed'] == [{key: value for key, value in failure.items() if key != 'triage'} for failure in item['failures']], 'Failure relabelled or lost')
        history_counts.append({'archive': item['archive'], 'verified_inputs': len(identity['sources']), 'failed_groups': len(item['failures'])})
    old_recipe = (GAME / 'core/build/mara-semantic-03/source/core/src/recipe_effects.inl').read_text()
    corrected = old_recipe.replace('case 1082:status(main,0,heatAtFire);break;', 'case 1082:status(main,0,std::min(10,heatAtFire));break;').replace('if(p.place==Place::Reserve)++n;base=add(base,std::min(18,3*n));', 'if(p.place==Place::Reserve&&isUnusedPart(p))++n;base=add(base,std::min(18,3*n));')
    need(corrected == (source / 'core/src/recipe_effects.inl').read_text(), 'More than the two declared recipe repairs')
    old_identity = read(GAME / 'core/build/mara-semantic-03/identity.json')
    for path, digest in record['sources'].items():
        if path.startswith(('core/src/', 'core/include/')) and path != 'core/src/recipe_effects.inl':
            need(digest == old_identity['sources'][path], 'Unexpected other production change')

    controls = []
    for name, alter in [
        ('Missing PASS', lambda r, p: r['tests'].pop()),
        ('Inflated assertion claim', lambda r, p: p['bindings']['recipe:MA082'].update(assertions=99999)),
        ('Altered source clause', lambda r, p: p['bindings']['recipe:MA098']['source_clause'].update(exact_text='Changed unused definition')),
        ('Activated link', lambda r, p: p['bindings']['recipe:MA002'].update(status='active')),
    ]:
        r, p = copy.deepcopy(record), copy.deepcopy(proposal)
        alter(r, p)
        try:
            verify_proposal(r, p, clauses, fixtures)
        except AssertionError:
            controls.append(name)
        else:
            raise AssertionError('Negative control accepted: ' + name)
    result = {'graph_sha256': record['source_graph_sha256'], 'candidate_inputs_verified': 37,
              'candidate_artifacts_verified': len(record['archive_artifacts']) + len(record['executables']),
              'printed_rows_verified': 107, 'primary_fixtures': 107, 'extensions': 33,
              'proposed_status': 'bounded proposed_only; no exhaustive/full or active acceptance',
              'retained_failed_captures': history_counts, 'two_line_production_delta_verified': True,
              'negative_controls_rejected': controls, 'fixture_locations': fixtures,
              'scope': 'Identity, inventory and assertion linkage; semantic review and additional executions are separate.'}
    if args.output:
        args.output.write_text(json.dumps(result, indent=2, ensure_ascii=False) + '\n', encoding='utf-8')
    print(json.dumps({key: value for key, value in result.items() if key != 'fixture_locations'}, indent=2))


if __name__ == '__main__':
    main()
