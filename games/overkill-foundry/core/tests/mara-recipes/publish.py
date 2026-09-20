"""Publish or verify portable, proposed-only Mara contract evidence from a frozen run."""
from __future__ import annotations
import argparse
import difflib
import hashlib
import json
from pathlib import Path
import re

HERE = Path(__file__).resolve().parent
GAME = HERE.parents[2]


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path: Path):
    return json.loads(path.read_text(encoding='utf-8'))


def write(path: Path, value):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(value, indent=2, ensure_ascii=False) + '\n', encoding='utf-8')


def verified_archive(path: Path):
    path = path.resolve()
    if not path.is_relative_to(GAME / 'core/build'):
        raise RuntimeError('Archive must be under ignored core/build')
    report = read(path / 'identity.json')
    for name, digest in report['sources'].items():
        if sha(path / 'source' / name) != digest:
            raise RuntimeError('Captured input changed: ' + name)
    if sha(path / 'build/Release/mara_recipe_contract_tests.exe') != report['executable_sha256']:
        raise RuntimeError('Captured contract executable changed')
    if sha(path / 'contracts.log') != report['log_sha256']:
        raise RuntimeError('Captured contract log changed')
    graph = hashlib.sha256(''.join(f'{key}\n{value}\n' for key, value in sorted(report['sources'].items())).encode()).hexdigest()
    if graph != report['source_graph_sha256']:
        raise RuntimeError('Captured graph identity differs')
    return report


def history():
    explanations = {
        'mara-semantic-01': {
            'MA012': 'Author event oracle incorrectly required a hit_lost event; printed rule only requires the late hit and Heat to be absent.',
            'MA041': 'Author oracle incorrectly gave an extra target ordinary Mark damage; selected spread rules retain and ignore that Mark.',
            'MA082': 'Author fixture used Heat12 without capacity upgrade; replaced by legal acquired-MAU-02 plus paid Fuel Brick Heat14 in run02.',
            'MA098': 'Author oracle treated untouched installed/removed plain Shield as used; canonical rules retain its unused eligibility.',
            'MA111': 'Author fixture used illegal ordinary Heat20; replaced by ordinary10 and acquired-MAU-02 capacity14 with paid Fuel Bricks.',
            'MA120': 'Author controlled Recover target did not execute the assigned GainShield intent; replaced with explicit changed-current-Shield fixture.'
        },
        'mara-semantic-02': {
            'MA082': 'MR01 production defect: legal Heat14 applies Burn14 instead of the printed cap10.',
            'MA098': 'Author support-cost oracle expected MA037 to drain6, but its source cost is5; final used-Shield witness has remaining1.'
        },
        'mara-semantic-03': {
            'MA082': 'MR01 production defect preserved: legal Heat14 applies Burn14.',
            'MA098': 'MR02 production defect: a paid/depleted Shield removed to reserve adds3 despite not being unused.'
        },
        'mara-semantic-04': {
            'MA091': 'Author ordering oracle assumed spread precedes Ammo payloads. Correct witness uses an earlier Ammo support hit to kill the recipient.',
            'MA107': 'Author Campaign fixture replaced the canonical robot definition, preventing core-value lookup at real reward creation.',
            'MA119': 'Same invalid Campaign robot-definition fixture as MA107; canonical IDs are now retained while only intent/HP are controlled.'
        }
    }
    captures = []
    for name, defects in explanations.items():
        archive = GAME / 'core/build' / name
        report = verified_archive(archive)
        failed = [row for row in report['tests'] if row['result'] == 'failed']
        if {row['recipe'] for row in failed} != set(defects):
            raise RuntimeError('Unexpected historical failure set: ' + name)
        captures.append({
            'archive': archive.relative_to(GAME).as_posix(),
            'source_graph_sha256': report['source_graph_sha256'],
            'executable_sha256': report['executable_sha256'],
            'contracts_log_sha256': report['log_sha256'],
            'recipe_effects_sha256': report['core_sources']['core/src/recipe_effects.inl'],
            'passed_groups': report['passed'], 'failed_groups': report['failed'],
            'failures': [{**row, 'triage': defects[row['recipe']]} for row in failed]
        })
    write(HERE / 'history/failures.json', {
        'scope': 'Preserved original failed executables, sources and logs; no failure has been relabeled as a pass.',
        'captures': captures
    })


def publish(archive: Path, report):
    if report['passed'] != 107 or report['failed'] or any(c['exit_code'] for c in report['commands_result']):
        raise RuntimeError('Only a passing complete bounded candidate can be proposed')
    legacy = archive / 'build/Release/mara_legacy_recipe_tests.exe'
    if sha(legacy) != report['legacy_executable_sha256']:
        raise RuntimeError('Legacy executable changed')
    clauses = read(archive / 'source/core/tests/mara-recipes/source-contracts.json')
    if set(clauses) != {'recipe:' + row['recipe'] for row in report['tests']}:
        raise RuntimeError('Proposed links differ from the assigned source gap')
    residuals = read(HERE / 'residual-branches.json')
    evidence = HERE / 'evidence'
    evidence.mkdir(exist_ok=True)
    for name in ('contracts.log', 'legacy-recipes.log'):
        (evidence / name).write_bytes((archive / name).read_bytes())
    log = (archive / 'contracts.log').read_text(encoding='utf-8')
    total = int(re.search(r'SUMMARY 107 passed, 0 failed, (\d+) assertions', log).group(1))
    legacy_log = (archive / 'legacy-recipes.log').read_text(encoding='utf-8')
    legacy_total = int(re.search(r'All (\d+) recipe assertions passed', legacy_log).group(1))
    config = (archive / 'configure.log').read_text(encoding='utf-8')
    compiler = re.search(r'The CXX compiler identification is (.+)', config).group(1).strip()
    sdk = re.search(r'Selecting Windows SDK version ([0-9.]+)', config).group(1)
    artifacts = {name: sha(archive / name) for name in ('identity.json', 'configure.log', 'build.log', 'contracts.log', 'legacy-recipes.log')}
    results = {
        'scope': '107 bounded Mara recipe semantic groups; proposed review evidence only',
        'utc': report['utc'], 'archive': archive.relative_to(GAME).as_posix(),
        'source_graph_sha256': report['source_graph_sha256'], 'sources': report['sources'],
        'executables': {
            'build/Release/mara_recipe_contract_tests.exe': report['executable_sha256'],
            'build/Release/mara_legacy_recipe_tests.exe': report['legacy_executable_sha256']
        },
        'archive_artifacts': artifacts,
        'toolchain': {'compiler': compiler, 'windows_sdk': sdk, 'cmake': '3.31.6-msvc6', 'language': 'C++17', 'configuration': 'Release x64', 'flags': ['/W4', '/WX', '/permissive-', '/utf-8']},
        'commands_result': report['commands_result'],
        'command_record': 'Exact machine-local command argv is retained in the ignored archive identity.json; portable reproduction is in README.md.',
        'passed_groups': report['passed'], 'failed_groups': report['failed'],
        'contract_assertions': total, 'existing_recipe_assertions': legacy_total,
        'copied_source_unchanged': report['copied_source_unchanged'],
        'live_sources_still_equal_at_run': report['live_sources_still_equal'],
        'residuals_sha256': sha(HERE / 'residual-branches.json'),
        'tests': report['tests'], 'bindings_activated': False
    }
    write(evidence / 'results.json', results)
    bindings = {}
    for row in report['tests']:
        key = 'recipe:' + row['recipe']
        clause = clauses[key]
        bindings[key] = {
            'status': 'proposed_only', 'test': row['id'],
            'asserted_behavior': row['observation'], 'assertions': row['assertions'],
            'source_clause': clause['source'],
            'test_source': 'core/tests/mara_recipe_contract_tests.cpp + core/tests/mara-recipes/*.inl',
            'residuals': residuals['by_kind'][clause['kind']] + residuals['by_recipe'].get(row['recipe'], [])
        }
    write(evidence / 'proposed-bindings.json', {
        'scope': 'Proposed bounded semantic links; no global coverage binding or eligibility is activated.',
        'claim_boundary': 'The passing named assertions and selected source precedence are the claim. Recipe count alone is not acceptance or exhaustive clause/interactions proof.',
        'candidate_source_graph_sha256': report['source_graph_sha256'],
        'results': 'core/tests/mara-recipes/evidence/results.json',
        'shared_limits': residuals['shared_limits'], 'bindings': bindings
    })
    before_path = GAME / 'core/build/mara-semantic-03/source/core/src/recipe_effects.inl'
    after_path = archive / 'source/core/src/recipe_effects.inl'
    before, after = before_path.read_text(), after_path.read_text()
    expected = before.replace('case 1082:status(main,0,heatAtFire);break;', 'case 1082:status(main,0,std::min(10,heatAtFire));break;')
    expected = expected.replace('if(p.place==Place::Reserve)++n;base=add(base,std::min(18,3*n));', 'if(p.place==Place::Reserve&&isUnusedPart(p))++n;base=add(base,std::min(18,3*n));')
    if expected != after:
        raise RuntimeError('Production delta is larger than the two authorized Mara changes')
    history()
    patch = ''.join(difflib.unified_diff(before.splitlines(keepends=True), after.splitlines(keepends=True), fromfile='mara-semantic-03/core/src/recipe_effects.inl', tofile='mara-semantic-05/core/src/recipe_effects.inl'))
    (HERE / 'history/MR01-MR02.patch').write_text(patch, encoding='utf-8')
    write(HERE / 'history/repairs.json', {
        'authorized_scope': 'Root explicitly authorized only these two source-defined corrections; no schema/rules/content version changes.',
        'base_inl_sha256': sha(before_path), 'fixed_inl_sha256': sha(after_path),
        'minimal_diff': 'core/tests/mara-recipes/history/MR01-MR02.patch',
        'MR01': {
            'source': 'design/RECIPE-CATALOGUE.md:543, MA082 White Casting',
            'reproduction': 'Acquire MAU-02 through Rules; pay five MA001 Uses to fill legal Heat14; pay MA082 craft, load and Fire at a living target.',
            'before': 'Flat12 main damage, Burn14; violates the printed Burn10 cap.',
            'after': 'Flat12 main damage, Burn10 event/stack and unchanged Heat14; lower0/3 and exact10 controls remain unchanged.'
        },
        'MR02': {
            'source': 'design/RECIPE-CATALOGUE.md:559, MA098 Stored Weight',
            'reproduction': 'Pay SH002 craft/install (Shield6), then MA037 Use drains5, remove the remaining1-Shield part; craft/activate MA098 and craft/load/Fire SH001.',
            'before': 'Main9 despite no unused reserve parts; all Reserve parts were counted.',
            'after': 'Main6 with the used part excluded by isUnusedPart; untouched removed plain Shield still adds3, installed Shield does not, unused0/1/6/7 count/cap controls pass.',
            'origin_limit': 'No extra kind or origin filter was introduced.'
        }
    })


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--archive', type=Path, required=True)
    parser.add_argument('--check', action='store_true')
    args = parser.parse_args()
    archive = args.archive.resolve()
    report = verified_archive(archive)
    if not args.check:
        publish(archive, report)
    results = read(HERE / 'evidence/results.json')
    if results['source_graph_sha256'] != report['source_graph_sha256']:
        raise RuntimeError('Published evidence refers to a different capture')
    for name, digest in results['archive_artifacts'].items():
        if sha(archive / name) != digest:
            raise RuntimeError('Archived artifact changed: ' + name)
    for name, digest in results['executables'].items():
        if sha(archive / name) != digest:
            raise RuntimeError('Executable changed: ' + name)
    for name in ('contracts.log', 'legacy-recipes.log'):
        if sha(HERE / 'evidence' / name) != sha(archive / name):
            raise RuntimeError('Published log differs: ' + name)
    proposals = read(HERE / 'evidence/proposed-bindings.json')
    if len(proposals['bindings']) != 107 or any(row['status'] != 'proposed_only' for row in proposals['bindings'].values()):
        raise RuntimeError('Proposal scope was changed')
    if sha(HERE / 'residual-branches.json') != results['residuals_sha256']:
        raise RuntimeError('Residual declaration changed after publication')
    print(json.dumps({
        'verified': True, 'source_graph_sha256': report['source_graph_sha256'],
        'copied_source_files': len(report['sources']), 'proposed_links': len(proposals['bindings']),
        'contract_assertions': results['contract_assertions'],
        'existing_recipe_assertions': results['existing_recipe_assertions'],
        'live_source_matches_capture': all(sha(GAME / name) == digest for name, digest in report['sources'].items())
    }))


if __name__ == '__main__':
    main()
