"""Build and run an immutable, isolated Mara semantic-contract source capture."""
from __future__ import annotations
import argparse
import datetime as dt
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess

HERE = Path(__file__).resolve().parent
GAME = HERE.parents[2]


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--output', type=Path, required=True)
    parser.add_argument('--cmake', type=Path, required=True)
    args = parser.parse_args()
    output = args.output.resolve()
    if output.exists() or not output.is_relative_to(GAME / 'core/build'):
        raise RuntimeError('Use a fresh isolated directory under ignored core/build')
    files = [p for folder in ('core/src', 'core/include') for p in (GAME / folder).rglob('*') if p.is_file()]
    files += [GAME / 'core/tests/mara_recipe_contract_tests.cpp', GAME / 'core/tests/recipe_tests.cpp']
    files += [p for p in HERE.rglob('*') if p.is_file() and p.suffix in ('.inl', '.inc', '.py')]
    files += [HERE / 'CMakeLists.txt', HERE / 'source-contracts.json', GAME / 'content/catalogue.snapshot.json',
              GAME / 'content/runtime-evidence/coverage.json', GAME / 'design/RECIPE-CATALOGUE.md',
              GAME / 'design/TIMING-AND-PERSISTENCE.md', GAME / 'design/UPGRADE-CATALOGUE.md']
    files = sorted(set(files))
    before = {p.relative_to(GAME).as_posix(): sha(p) for p in files}
    output.mkdir(parents=True)
    for path in files:
        relative = path.relative_to(GAME)
        destination = output / 'source' / relative
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(path, destination)
        if sha(path) != before[relative.as_posix()] or sha(destination) != before[relative.as_posix()]:
            raise RuntimeError(f'Source changed during capture: {relative}')
    commands = [
        [str(args.cmake.resolve()), '-S', str(output / 'source/core/tests/mara-recipes'), '-B', str(output / 'build'), '-A', 'x64', f'-DMARA_CORE_ROOT:PATH={output / "source/core"}'],
        [str(args.cmake.resolve()), '--build', str(output / 'build'), '--config', 'Release', '--parallel', '4'],
        [str(output / 'build/Release/mara_recipe_contract_tests.exe')],
        [str(output / 'build/Release/mara_legacy_recipe_tests.exe')],
    ]
    outcomes = []
    for label, command in zip(('configure', 'build', 'contracts', 'legacy-recipes'), commands):
        with (output / (label + '.log')).open('wb') as stream:
            result = subprocess.run(command, cwd=output, stdout=stream, stderr=subprocess.STDOUT)
        outcomes.append({'phase': label, 'exit_code': result.returncode})
        print(f'{label}: exit {result.returncode}', flush=True)
        if result.returncode and label in ('configure', 'build'):
            break
    copied = {key: sha(output / 'source' / key) for key in before}
    if copied != before:
        raise RuntimeError('Captured build inputs changed during execution')
    log = (output / 'contracts.log').read_text(encoding='utf-8') if (output / 'contracts.log').exists() else ''
    cases = []
    for line in log.splitlines():
        match = re.match(r'(PASS|FAIL) recipe:(MA\d+) assertions=(\d+)[: ]+(.*)', line)
        if match:
            status, recipe, assertions, text = match.groups()
            cases.append({'id': 'mara-recipe:' + recipe, 'recipe': recipe, 'result': 'passed' if status == 'PASS' else 'failed', 'assertions': int(assertions), 'observation': text})
    core_files = {key: value for key, value in before.items() if key.startswith(('core/src/', 'core/include/'))}
    graph = hashlib.sha256(''.join(f'{key}\n{value}\n' for key, value in sorted(before.items())).encode()).hexdigest()
    exe = output / 'build/Release/mara_recipe_contract_tests.exe'
    report = {
        'utc': dt.datetime.now(dt.timezone.utc).isoformat(), 'scope': 'Mara printed recipe semantic contracts; proposed links only',
        'archive': output.relative_to(GAME).as_posix(), 'sources': before, 'core_sources': core_files,
        'source_graph_sha256': graph, 'copied_source_unchanged': True,
        'live_sources_still_equal': all(sha(GAME / key) == value for key, value in before.items()),
        'commands': commands, 'commands_result': outcomes,
        'executable_sha256': sha(exe) if exe.exists() else None,
        'legacy_executable_sha256': sha(output / 'build/Release/mara_legacy_recipe_tests.exe') if (output / 'build/Release/mara_legacy_recipe_tests.exe').exists() else None,
        'tests': cases,
        'passed': sum(c['result'] == 'passed' for c in cases), 'failed': sum(c['result'] == 'failed' for c in cases),
        'log_sha256': sha(output / 'contracts.log') if log else None,
        'bindings_activated': False,
    }
    (output / 'identity.json').write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({k: report[k] for k in ('passed', 'failed', 'source_graph_sha256', 'executable_sha256')}), flush=True)
    raise SystemExit(0 if report['passed'] == 107 and report['failed'] == 0 and len(outcomes) == 4 and all(row['exit_code'] == 0 for row in outcomes) else 1)


if __name__ == '__main__':
    main()
