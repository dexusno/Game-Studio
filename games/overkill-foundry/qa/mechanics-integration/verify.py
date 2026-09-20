"""Read-only verification of the frozen combined native mechanics build; never runs it."""
from __future__ import annotations
import datetime as dt
import difflib
import hashlib
import json
from pathlib import Path
import re
import subprocess
import xml.etree.ElementTree as ET

HERE = Path(__file__).resolve().parent
GAME = HERE.parents[1]
REPO = GAME.parents[1]
ARCHIVE = GAME / 'core/build/mechanics-integration-20260920-221228'
FAILED = GAME / 'core/build/mechanics-integration-20260920-220954'
BASE = '0e4c019e1435665d73ce4dcf534618b8faf015a2'
GRAPH = '8ce7ff421aa9bc2a6ed4a1be4d77ad5c4a882f08cc6b48b92a49ca4fa3798637'
LIBRARY = 'afe752450de55d4837b2b5ebe3e1517eb5a21fb77912fdfd0bb03a0649a05228'
MSBUILD = {'m': 'http://schemas.microsoft.com/developer/msbuild/2003'}
checks = []


def digest(value: bytes) -> str:
    return hashlib.sha256(value).hexdigest()


def sha(path: Path) -> str:
    return digest(path.read_bytes())


def text(path: Path) -> str:
    data = path.read_bytes()
    return data.decode('utf-16' if data[:2] in (b'\xff\xfe', b'\xfe\xff') else 'utf-8-sig').replace('\r\n', '\n')


def read(path: Path):
    return json.loads(path.read_text(encoding='utf-8'))


def check(label: str, condition: bool):
    checks.append({'check': label, 'passed': bool(condition)})


def inputs(archive: Path, expected_count: int, live: bool):
    record = read(archive / 'identity.json')
    rows = record['inputs']
    check(f'{archive.name}: input count', len(rows) == expected_count)
    check(f'{archive.name}: unique sorted inputs', [r['path'] for r in rows] == sorted({r['path'] for r in rows}, key=lambda x: x.lower()))
    copied, current = [], []
    for row in rows:
        data = (archive / 'source' / row['path']).read_bytes()
        copied.append(len(data) == row['bytes'] and digest(data) == row['sha256'] and digest(data.replace(b'\r\n', b'\n')) == row['sha256_lf'])
        if live:
            current.append(sha(GAME / row['path']) == row['sha256'])
    check(f'{archive.name}: every copied raw/LF digest and length', all(copied))
    if live:
        check(f'{archive.name}: every live raw input matches', all(current))
    graph = digest(''.join(row['path'] + ' ' + row['sha256_lf'] + '\n' for row in rows).encode())
    check(f'{archive.name}: independently recomputed graph', graph == record['source_graph_lf_sha256'])
    return record


def parse_full_output(path: Path):
    # Independent line-state parser; no execution and no invocation of capture.py.
    records = []
    current = None
    collecting = False
    separator = False
    for line in text(path).splitlines():
        match = re.fullmatch(r'(\d+)/(\d+) Testing: (.+)', line)
        if match:
            current = {'index': int(match[1]), 'total': int(match[2]), 'name': match[3], 'lines': [], 'passed': False}
            records.append(current)
        elif current is not None:
            if line.startswith('Command: '):
                current['command'] = line[len('Command: '):].strip('"')
            elif line == 'Output:':
                separator = True
            elif separator:
                check(current['name'] + ': output separator', bool(line) and set(line) == {'-'})
                separator = False
                collecting = True
            elif collecting:
                if line == '<end of output>':
                    collecting = False
                else:
                    current['lines'].append(line)
            elif line == 'Test Passed.':
                current['passed'] = True
    for row in records:
        row['output'] = '\n'.join(row.pop('lines'))
    return records


def project(path: Path, core_library: Path, manifest):
    root = ET.parse(path).getroot()
    groups = [g for g in root.findall('m:ItemDefinitionGroup', MSBUILD) if "'Release|x64'" in g.get('Condition', '')]
    check(path.stem + ': single Release x64 settings', len(groups) == 1)
    group = groups[0]
    compile_node = group.find('m:ClCompile', MSBUILD)
    flags = {name: compile_node.findtext('m:' + name, namespaces=MSBUILD) for name in ('WarningLevel', 'TreatWarningAsError', 'ConformanceMode', 'LanguageStandard', 'AdditionalOptions')}
    check(path.stem + ': strict actual generated compiler options', all(flags[k] == v for k, v in {'WarningLevel': 'Level4', 'TreatWarningAsError': 'true', 'ConformanceMode': 'true', 'LanguageStandard': 'stdcpp17'}.items()))
    source_paths = [Path(n.attrib['Include']).resolve() for n in root.findall('m:ItemGroup/m:ClCompile', MSBUILD)]
    check(path.stem + ': compiled units are copied inputs', all(p.is_relative_to(ARCHIVE / 'source') and p.relative_to(ARCHIVE / 'source').as_posix().lower() in manifest for p in source_paths))
    link = group.find('m:Link', MSBUILD)
    dependencies = []
    if link is not None:
        dependencies = link.findtext('m:AdditionalDependencies', '', MSBUILD).split(';')
        supplied = [(path.parent / dep).resolve() for dep in dependencies if dep.endswith('overkill_core.lib')]
        check(path.stem + ': exact shared core in linker dependencies', supplied == [core_library])
    tlogs = list(path.parent.glob(path.stem + '.dir/Release/*/CL.command.1.tlog'))
    check(path.stem + ': one actual compile command log', len(tlogs) == 1)
    command = text(tlogs[0])
    check(path.stem + ': actual compile command strict C++17', all(flag in command for flag in ('/W4', '/WX', '/permissive-', '/std:c++17')))
    read_logs = list(path.parent.glob(path.stem + '.dir/Release/*/CL.read.1.tlog'))
    user_dependencies = set()
    unexpected = set()
    game_prefix = str(GAME.resolve()).replace('/', '\\').upper() + '\\'
    archive_prefix = str((ARCHIVE / 'source').resolve()).replace('/', '\\').upper() + '\\'
    for log in read_logs:
        for line in text(log).splitlines():
            for item in line.lstrip('^').split('|'):
                value = item.strip().upper()
                if value.startswith(archive_prefix):
                    relative = value[len(archive_prefix):].replace('\\', '/').lower()
                    if relative not in manifest:
                        unexpected.add(relative)
                    user_dependencies.add(relative)
                elif value.startswith(game_prefix):
                    unexpected.add(value[len(game_prefix):].replace('\\', '/').lower())
    check(path.stem + ': all compiler-read game dependencies captured', not unexpected)
    artifacts = [path, *tlogs, *read_logs]
    if link is not None:
        link_logs = list(path.parent.glob(path.stem + '.dir/Release/*/link.read.1.tlog'))
        check(path.stem + ': actual link read records exact core library', len(link_logs) == 1 and str(core_library).replace('/', '\\').upper() in text(link_logs[0]).upper())
        artifacts.extend(link_logs)
    return {
        'target': path.stem, 'project': path.relative_to(ARCHIVE).as_posix(), 'flags': flags,
        'source_units': [p.relative_to(ARCHIVE / 'source').as_posix() for p in source_paths],
        'captured_user_dependencies': sorted(user_dependencies), 'unexpected_user_dependencies': sorted(unexpected),
        'artifact_sha256': {p.relative_to(ARCHIVE).as_posix(): sha(p) for p in artifacts}
    }


def main():
    record = inputs(ARCHIVE, 78, True)
    published = read(GAME / 'core/tests/mechanics/results.json')
    portable = dict(record)
    portable['commands'] = [{k: v for k, v in command.items() if k != 'command'} for command in record['commands']]
    check('published record differs only by omitted local argv and explicit limits', {k: v for k, v in published.items() if k != 'limits'} == portable and bool(published.get('limits')))
    check('declared final graph', record['source_graph_lf_sha256'] == GRAPH)
    check('status passed', record['status'] == 'passed')
    for cmd in record['commands']:
        check(cmd['log'] + ': successful command/log identity', cmd['exit_code'] == 0 and sha(ARCHIVE / cmd['log']) == cmd['sha256'])
    check('exact configure/build/single CTest command sequence', [x['log'] for x in record['commands']] == ['configure.log', 'build.log', 'ctest.log'])
    for row in record['binaries']:
        p = ARCHIVE / row['path']
        check(row['path'] + ': binary bytes/hash', p.stat().st_size == row['bytes'] and sha(p) == row['sha256'])
    core_library = (ARCHIVE / 'build/core-runtime/Release/overkill_core.lib').resolve()
    check('one core library exact identity', sha(core_library) == LIBRARY and len(list((ARCHIVE / 'build').rglob('overkill_core.lib'))) == 1)
    check('original capture tool identity retained', sha(ARCHIVE / 'capture.py') == record['capture_tool_sha256'])
    check('later extractor distinct and correctly attributed', sha(ARCHIVE / 'results-extractor.py') == record['results_extraction_tool_sha256'] != record['capture_tool_sha256'])
    check('live extraction source equals copied later extractor', sha(GAME / 'core/tests/mechanics/capture.py') == record['results_extraction_tool_sha256'])
    full_log = ARCHIVE / record['full_output']['path']
    check('full LastTest.log identity', sha(full_log) == record['full_output']['sha256'] and full_log.stat().st_size == record['full_output']['bytes'])
    full = parse_full_output(full_log)
    junit = ET.parse(ARCHIVE / 'ctest.xml').getroot()
    check('JUnit sixteen run/no failure/no skip', junit.get('tests') == '16' and junit.get('failures') == '0' and junit.get('skipped') == '0')
    cases = junit.findall('testcase')
    check('one complete LastTest execution sequence', len(full) == 16 and [x['index'] for x in full] == list(range(1,17)) and all(x['total'] == 16 and x['passed'] for x in full))
    check('JUnit/LastTest/result suite order and IDs', [x['name'] for x in full] == [x.attrib['name'] for x in cases] == [x['name'] for x in record['suites']])
    suites = []
    manifest = {x['path'].lower(): x for x in record['inputs']}
    projects = []
    for actual, case, supplied in zip(full, cases, record['suites']):
        name = actual['name']
        check(name + ': full extracted stdout equals same-run log', actual['output'] == supplied['output'] and supplied['result'] == 'passed')
        check(name + ': JUnit passed run', case.get('status') == 'run' and case.find('failure') is None and case.find('skipped') is None)
        xml_output = case.findtext('system-out', '')
        truncated = '[This part of the test output was removed' in xml_output
        if truncated:
            prefix = xml_output.split('...\n[This part of the test output was removed')[0]
            check(name + ': truncated JUnit prefix agrees with full log', actual['output'].startswith(prefix) and '1024 bytes' in xml_output)
        else:
            check(name + ': complete JUnit output agrees with full log', actual['output'].rstrip('\n') == xml_output.rstrip('\n'))
        executable = Path(actual['command']).resolve()
        check(name + ': actual command points to captured executable', executable.is_relative_to(ARCHIVE) and any(x['path'] == executable.relative_to(ARCHIVE).as_posix() for x in record['binaries']))
        project_paths = list((ARCHIVE / 'build').rglob(executable.stem + '.vcxproj'))
        check(name + ': unique compiled project', len(project_paths) == 1)
        projects.append(project(project_paths[0], core_library, manifest))
        suites.append({'name': name, 'target': executable.stem, 'executable_sha256': sha(executable), 'stdout_sha256': digest(actual['output'].encode()), 'stdout_bytes': len(actual['output'].encode()), 'junit_truncated_at_1024': truncated, 'summary': actual['output'].splitlines()[-1]})
    for name in ('overkill_core', 'overkill_runner_support'):
        paths = list((ARCHIVE / 'build').rglob(name + '.vcxproj'))
        check(name + ': one library project', len(paths) == 1)
        projects.append(project(paths[0], core_library, manifest))
    old = inputs(FAILED, 77, False)
    check('initial compile failure remains marked failed', old['status'] == 'failed' and not (FAILED / 'ctest.xml').exists())
    old_paths = {r['path']: r for r in old['inputs']}
    added = sorted(set(x['path'] for x in record['inputs']) - set(old_paths))
    check('only precision header added to retry input manifest', added == ['presentation/precision.hpp'] and all(old_paths[x['path']] == x for x in record['inputs'] if x['path'] in old_paths))
    check('initial source proves required precision header was omitted', '../../presentation/precision.hpp' in text(FAILED / 'source/core/tests/upgrade_contract_tests.cpp') and not (FAILED / 'source/presentation/precision.hpp').exists())
    production = []
    patch = []
    for row in record['inputs']:
        if not row['path'].startswith(('core/src/', 'core/include/')):
            continue
        git_path = 'games/overkill-foundry/' + row['path']
        old_data = subprocess.run(['git', 'show', BASE + ':' + git_path], cwd=REPO, stdout=subprocess.PIPE, stderr=subprocess.PIPE, check=True).stdout
        new_data = (ARCHIVE / 'source' / row['path']).read_bytes()
        old_text = old_data.decode().replace('\r\n', '\n')
        new_text = new_data.decode().replace('\r\n', '\n')
        if old_text != new_text:
            difference = ''.join(difflib.unified_diff(old_text.splitlines(keepends=True), new_text.splitlines(keepends=True), fromfile=BASE[:7] + '/' + row['path'], tofile=ARCHIVE.name + '/' + row['path']))
            patch.append(difference)
            production.append({'path': row['path'], 'base_lf_sha256': digest(old_text.encode()), 'candidate_sha256': row['sha256'], 'diff_sha256': digest(difference.encode()), 'hunks': difference.count('\n@@ ')})
    check('only five known production files differ from baseline', {x['path'] for x in production} == {'core/src/core.cpp', 'core/src/campaign.cpp', 'core/src/campaign_upgrades.cpp', 'core/src/recipe_effects.inl', 'core/src/upgrade_effects.inl'})
    (HERE / 'production.delta.patch').write_text(''.join(patch), encoding='utf-8')
    evidence = {
        'reviewed_at_utc': dt.datetime.now(dt.timezone.utc).isoformat(),
        'scope': 'Read-only integrated build/evidence verification, not semantic approval of reviewer-authored Mara cases; no tests rerun.',
        'archive': ARCHIVE.relative_to(GAME).as_posix(), 'source_graph_lf_sha256': GRAPH,
        'shared_core_library_sha256': LIBRARY,
        'checks': checks, 'passed_checks': sum(x['passed'] for x in checks), 'failed_checks': sum(not x['passed'] for x in checks),
        'inputs': record['inputs'], 'suites': suites, 'generated_projects_and_actual_logs': projects,
        'archive_artifacts': {p.relative_to(ARCHIVE).as_posix(): sha(p) for p in (ARCHIVE / 'identity.json', ARCHIVE / 'ctest.xml', full_log, ARCHIVE / 'capture.py', ARCHIVE / 'results-extractor.py', ARCHIVE / 'configure.log', ARCHIVE / 'build.log', ARCHIVE / 'ctest.log')},
        'published_results_sha256': sha(GAME / 'core/tests/mechanics/results.json'),
        'initial_failed_capture': {'archive': FAILED.relative_to(GAME).as_posix(), 'source_graph_lf_sha256': old['source_graph_lf_sha256'], 'input_count': len(old['inputs']), 'identity_sha256': sha(FAILED / 'identity.json'), 'original_compiler_log_retained': False, 'only_added_input_on_retry': added, 'attributed_root_observation': 'Direct exec session 87257 completion e04b3a reportedly returned exit 1 and C1083 at upgrade_contract_tests.cpp(4,10), missing ../../presentation/precision.hpp. The review verifies the omission and failed record, not that unretained console output.'},
        'baseline_commit': BASE, 'production_differences': production,
        'verifier_sha256': sha(Path(__file__)), 'test_execution_performed_by_review': False,
        'manual_production_delta_review': 'Five changed production files / ten hunks match terminal notification guards, Modifier/Spread category recognition, UGS-085 base-price gate, SH092/SH113 first-install schedules, UGS121 copy attribution, and MR01/02. No additional production/header/serialization changes were found against the stated baseline.',
        'note': 'Some copied manifest entries are unused standalone CMake files and two existing generated CompilerId sources. Actual compiler-read game dependency closure is checked separately.'
    }
    out = GAME / 'qa/mechanics-integration-evidence.json'
    out.write_text(json.dumps(evidence, indent=2) + '\n', encoding='utf-8')
    print(json.dumps({'passed_checks': evidence['passed_checks'], 'failed_checks': evidence['failed_checks'], 'failures': [x['check'] for x in checks if not x['passed']], 'suite_count': len(suites), 'source_graph_lf_sha256': GRAPH}, indent=2))
    return 1 if evidence['failed_checks'] else 0


if __name__ == '__main__':
    raise SystemExit(main())
