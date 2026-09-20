"""Verify captured UC01 identity, isolated overlay scope and retained failed evidence."""
import argparse
import hashlib
import json
from pathlib import Path

GAME = Path(__file__).resolve().parents[2]
ROOT = GAME / 'core/build/qa-upgrade-copy'


def read(path):
    return json.loads(path.read_text(encoding='utf-8'))


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def encoded(value):
    return (json.dumps(value, indent=2, sort_keys=True, ensure_ascii=False) + '\n').encode('utf-8')


def need(value, reason):
    if not value:
        raise AssertionError(reason)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path)
    args = parser.parse_args()
    record = read(ROOT / 'records/results.json')
    archive = GAME / record['archive']
    for path, digest in record['inputs'].items():
        need(sha(archive / 'inputs' / path) == digest, 'Captured source changed: ' + path)
    need(len(record['inputs']) == 58, 'Captured graph count')
    need(hashlib.sha256(encoded(record['inputs'])).hexdigest() == record['graph_sha256'], 'Captured graph digest')
    for path, digest in record['archive_artifacts'].items():
        need(sha(archive / path) == digest, 'Captured artifact changed: ' + path)
    need(len(record['archive_artifacts']) == 14, 'Captured artifact count')
    for execution in record['executions']:
        need(sha(archive / Path(execution['executable']).name) == execution['executable_sha256'], 'Named binary mismatch')
        need(sha(archive / execution['log']) == execution['log_sha256'], 'Execution log mismatch')
        text = (archive / execution['log']).read_text(encoding='utf-8')
        names = [line[5:] for line in text.splitlines() if line.startswith('PASS ')]
        need(names == execution['named_results'] and execution['summary'] in text and execution['exit_code'] == 0, 'Executed group linkage mismatch')

    old_record = read(GAME / 'core/build/qa-upgrade-contracts/frozen-49b5f504/results.json')
    old = GAME / old_record['archive'] / 'inputs'
    overlay = GAME / 'core/build/qa-upgrade-copy-overlay/inputs'
    overlay_inputs = {}
    changed = []
    for path, digest in old_record['inputs'].items():
        need(sha(old / path) == digest, 'Original upgrade source changed: ' + path)
        actual = sha(overlay / path)
        overlay_inputs[path] = actual
        if actual != digest:
            changed.append(path)
    need(set(changed) == {'core/src/recipe_effects.inl', 'core/src/upgrade_effects.inl', 'core/tests/upgrade_tests.cpp'}, 'Overlay changed unrelated source')
    patch_path = archive / 'inputs/core/tests/upgrade-copy-contracts/uc01.patch'
    need(sha(patch_path) == '66ea4e65a19ce61af7bd036e2a979056ccc0a413cf869d14ec95cef4a4dff74d', 'Isolated patch changed')
    full = archive / 'inputs'
    for path in ('core/src/upgrade_effects.inl', 'core/tests/upgrade_tests.cpp'):
        need((overlay / path).read_text() == (full / path).read_text(), 'Full capture diverges from attribution patch')
    # The two selected schedule corrections are the only full/overlay production difference.
    expected = (overlay / 'core/src/recipe_effects.inl').read_text()
    expected = expected.replace('case 96:schedule(DeliveryKind::ShieldPart,12,p.recipe);break;case 1014:heat(2);break;case 1016:schedule(DeliveryKind::ShieldPart,5,p.recipe);break;',
        'case 92:schedule(DeliveryKind::ShieldPart,15,p.recipe);break;\n        case 96:schedule(DeliveryKind::ShieldPart,12,p.recipe);break;\n        case 113:schedule(DeliveryKind::ShieldPart,20,p.recipe);break;\n        case 1014:heat(2);break;case 1016:schedule(DeliveryKind::ShieldPart,5,p.recipe);break;')
    expected = expected.replace('            case 92:schedule(DeliveryKind::ShieldPart,15,p.recipe);break;\n            case 113:schedule(DeliveryKind::ShieldPart,20,p.recipe);break;\n', '')
    need(expected == (full / 'core/src/recipe_effects.inl').read_text(), 'Unexplained full recipe difference')
    for path in old_record['inputs']:
        if path.startswith(('core/src/', 'core/include/')) and path not in changed:
            need(sha(full / path) == sha(old / path), 'Other production changed')

    failure = GAME / 'core/build/qa-upgrade-contracts/uc01-failure'
    identity = read(failure / 'identity.json')
    for path, digest in identity['artifacts'].items():
        need(sha(failure / path) == digest, 'Original failed evidence changed')
    need(identity['exit_code'] == 1, 'Original failed status changed')
    replay = (ROOT / 'original-failure-reproduced.log').read_text(encoding='utf-8')
    need('FAIL Q09' in replay and 'observed damage=16' in replay and 'SUMMARY 8 passed, 1 failed, 492 assertions' in replay, 'Old executable no longer reproduces original failure')
    need((ROOT / 'full-probes.log').read_bytes() == (ROOT / 'overlay-probes.log').read_bytes(), 'New independent probe outcomes differ')
    need((ROOT / 'full-original-review.log').read_bytes() == (ROOT / 'overlay-original-review.log').read_bytes(), 'Unchanged original review outcomes differ')
    need('SUMMARY 6 passed, 0 failed, 1333 assertions' in (ROOT / 'full-probes.log').read_text(), 'Independent final result mismatch')
    need('SUMMARY 9 passed, 0 failed, 503 assertions' in (ROOT / 'full-original-review.log').read_text(), 'Original-source final result mismatch')
    result = {
        'full_graph_sha256': record['graph_sha256'], 'full_inputs_verified': 58,
        'full_artifacts_verified': 14, 'named_suite_linkages': 5,
        'original_graph_sha256': old_record['graph_sha256'], 'original_inputs_verified': len(old_record['inputs']),
        'overlay_changed_files': changed, 'overlay_input_hashes': overlay_inputs,
        'overlay_graph_sha256': hashlib.sha256(encoded(overlay_inputs)).hexdigest(),
        'isolated_patch_sha256': sha(patch_path), 'old_failure_artifacts_verified': len(identity['artifacts']),
        'full_vs_overlay_production_difference': 'Only separately reviewed SH092/SH113 schedule moves',
        'independent_logs_identical_across_graphs': True,
        'scope': 'Artifact/link/scope integrity only; runtime and migration observations are described in the review.',
    }
    if args.output:
        args.output.write_bytes(encoded(result))
    print(json.dumps({key: value for key, value in result.items() if key != 'overlay_input_hashes'}, indent=2))


if __name__ == '__main__':
    main()
