"""Capture the narrow UC01 overlay and affected native suites, preserving old evidence."""
import argparse
import datetime
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
HERE=Path(__file__).resolve().parent
GAME=HERE.parents[2]
BUILD=GAME/'core/build/upgrade-copy-contracts'
EVIDENCE=HERE/'evidence'
CMAKE=Path('C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe')
TARGETS={'copy':'upgrade_copy_contract_tests','legacy-upgrades':'upgrade_copy_legacy_tests','author-upgrades':'upgrade_copy_author_tests','shared':'upgrade_copy_shared_tests','parts':'upgrade_copy_part_tests'}
def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()
def data(obj): return (json.dumps(obj,indent=2,sort_keys=True)+'\n').encode()
def write(path,obj): path.write_bytes(data(obj))
def inputs():
    paths={GAME/name for name in ['core/CMakeLists.txt','core/tests/upgrade_copy_contract_tests.cpp','core/tests/upgrade_tests.cpp','core/tests/upgrade_contract_tests.cpp','core/tests/shared_recipe_contract_tests.cpp','core/tests/part_lifecycle_tests.cpp','presentation/precision.hpp','content/cinderwall.manifest.json','content/catalogue.snapshot.json','design/RECIPE-CATALOGUE.md','design/TIMING-AND-PERSISTENCE.md','design/UPGRADE-SYSTEM.md','design/PART-RESALE.md','design/data/shared-upgrades.json','design/data/character-mayor-upgrades.json','design/data/part-resale-bases.json','design/data/beta-balance-v1.json']}
    for name in ['core/src','core/include','core/tests/parts','core/tests/upgrade-contracts','core/tests/shared-recipes']:
        paths.update(p for p in (GAME/name).rglob('*') if p.suffix in {'.cpp','.hpp','.inl','.inc'})
    paths.update(p for p in HERE.iterdir() if p.is_file())
    return {p.relative_to(GAME).as_posix():sha(p) for p in sorted(paths)}
def run(command,log):
    result=subprocess.run([str(v) for v in command],capture_output=True,text=True)
    text=result.stdout+result.stderr;log.write_text(text,encoding='utf-8',newline='\n')
    if result.returncode: raise RuntimeError(f'Exit {result.returncode}: {log}')
    return text
def verify():
    record=json.loads((EVIDENCE/'results.json').read_text())
    assert inputs()==record['inputs']
    archive=GAME/record['archive']
    for name,digest in record['inputs'].items(): assert sha(archive/'inputs'/name)==digest
    for name,digest in record['archive_artifacts'].items(): assert sha(archive/name)==digest
    for entry in record['executions']:
        assert sha(GAME/entry['executable'])==entry['executable_sha256']
        assert sha(EVIDENCE/entry['log'])==entry['log_sha256']
        named=[line[5:] for line in (EVIDENCE/entry['log']).read_text().splitlines() if line.startswith('PASS ')]
        assert named==entry['named_results']
    assert len(record['executions'][0]['named_results'])==7
    for entry in record['build_commands']: assert sha(GAME/entry['log'])==entry['log_sha256']
    assert sha(GAME/record['library']['path'])==record['library']['sha256']
    print('PASS UC01 input/artifact graph and five executed suite identities.')
def capture(cmake):
    before=inputs();BUILD.mkdir(parents=True,exist_ok=True);EVIDENCE.mkdir(exist_ok=True)
    commands=[[cmake,'-S',HERE,'-B',BUILD,'-G','Visual Studio 17 2022','-A','x64'],[cmake,'--build',BUILD,'--config','Release','--clean-first','--parallel','4']]
    build=[]
    def record(command,log):
        run(command,log);build.append({'command':[str(x).replace(str(GAME),'<game>').replace(str(cmake),'cmake').replace(str(cmake.with_name('ctest.exe')),'ctest') for x in command],'exit_code':0,'log':log.relative_to(GAME).as_posix(),'log_sha256':sha(log)})
    for index,command in enumerate(commands): record(command,BUILD/f'capture-build-{index}.log')
    executions=[]
    for suite,target in TARGETS.items():
        executable=BUILD/'Release'/(target+'.exe');log=EVIDENCE/(suite+'.log');text=run([executable],log)
        named=[line[5:] for line in text.splitlines() if line.startswith('PASS ')]
        assert not any(line.startswith('FAIL ') for line in text.splitlines())
        summary=[line for line in text.splitlines() if 'assertions' in line][-1]
        if suite=='copy': assert re.search(r'SUMMARY 7 passed, 0 failed, \d+ assertions',summary)
        executions.append({'suite':suite,'executable':executable.relative_to(GAME).as_posix(),'executable_sha256':sha(executable),'log':log.name,'log_sha256':sha(log),'exit_code':0,'summary':summary,'named_results':named})
    record([cmake.with_name('ctest.exe'),'--test-dir',BUILD,'-C','Release','--output-on-failure'],BUILD/'capture-ctest.log')
    assert before==inputs()
    graph=hashlib.sha256(data(before)).hexdigest();identity='native-uc01-'+executions[0]['executable_sha256'][:16]
    archive=BUILD/'captures'/(identity+'-'+graph[:12]);archive.mkdir(parents=True,exist_ok=False)
    for name in before:
        target=archive/'inputs'/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(GAME/name,target)
    for entry in executions:
        shutil.copy2(GAME/entry['executable'],archive/Path(entry['executable']).name);shutil.copy2(EVIDENCE/entry['log'],archive/entry['log'])
    for entry in build: shutil.copy2(GAME/entry['log'],archive/Path(entry['log']).name)
    library=BUILD/'core-runtime/Release/overkill_core.lib';shutil.copy2(library,archive/library.name)
    assert before==inputs()=={name:sha(archive/'inputs'/name) for name in before}
    output={'schema_version':1,'verified_build':identity,'captured_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),'inputs':before,'graph_sha256':graph,'executions':executions,'build_commands':build,'archive':archive.relative_to(GAME).as_posix(),'archive_artifacts':{p.name:sha(p) for p in sorted(archive.iterdir()) if p.is_file()},'library':{'path':library.relative_to(GAME).as_posix(),'sha256':sha(library)},'compiler':'MSVC19.44.35228.0 /W4 /WX /permissive- C++17 Release x64','limitations':['No schema/Action/version change; old unmarked saved Slugs cannot be retrospectively attributed.','Old upgrade106 and Shared113 reports remain separate immutable historical captures.','Full current graph includes separate authorized SH092/SH113 fix; uc01.patch isolates only Jig changes for original49b5f5 recheck.','No global support ledger change.']}
    write(EVIDENCE/'results.json',output);verify();print(json.dumps({'build':identity,'graph':graph,'inputs':len(before),'archive':output['archive']},indent=2))
def main():
    parser=argparse.ArgumentParser();mode=parser.add_mutually_exclusive_group(required=True);mode.add_argument('--capture',action='store_true');mode.add_argument('--check',action='store_true');parser.add_argument('--cmake',type=Path,default=CMAKE);args=parser.parse_args();capture(args.cmake) if args.capture else verify()
if __name__=='__main__': main()
