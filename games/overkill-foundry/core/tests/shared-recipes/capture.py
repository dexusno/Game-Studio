"""Capture executed named Shared recipe contracts without changing release bindings."""
import argparse
import datetime
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess

HERE = Path(__file__).resolve().parent
GAME = HERE.parents[2]
BUILD = GAME / 'core/build/shared-recipes'
EVIDENCE = HERE / 'evidence'
DEFAULT_CMAKE = Path('C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe')
TARGETS = {'contracts':'shared_recipe_contract_tests','legacy-recipes':'shared_recipe_legacy_tests'}
SUPPLEMENTAL = {
    'SH035.boundaries':['SH035','SH102','SH118'],
    'SH043.boundaries':['SH043'],
    'SH064.boundaries':['SH064'],
    'SH072.boundaries':['SH072'],
    'SH092.boundaries':['SH092','SH113'],
    'SH093.boundaries':['SH093'],
    'SH103.boundaries':['SH103'],
    'SH104.boundaries':['SH104'],
    'SH110.boundaries':['SH110'],
    'SH113.boundaries':['SH113','SH066'],
    'SH119.boundaries':['SH119','SH123'],
    'SH122.boundaries':['SH122','SH123','SH126'],
}
RESIDUAL = {'recipe:SH064': 'Player Corrosion, Mark and Weaken prevention branches lack a demonstrated eligible application producer; Burn, once-only/expiry and Fouling/ShieldLeak exclusions are exercised. No full binding proposed.'}
IMPLEMENTATIONS = ['core/src/core.cpp','core/src/recipe_effects.inl','core/src/catalogue.cpp','core/src/catalogue_data.inc','core/src/campaign.cpp','core/src/serialization.cpp']

def sha(path): return hashlib.sha256(path.read_bytes()).hexdigest()
def encoded(value): return (json.dumps(value,indent=2,sort_keys=True,ensure_ascii=False)+'\n').encode('utf-8')
def write(path,value): path.write_bytes(encoded(value))
def inputs():
    names=['core/CMakeLists.txt','core/tests/shared_recipe_contract_tests.cpp','core/tests/recipe_tests.cpp',
           'content/cinderwall.manifest.json','content/catalogue.snapshot.json','content/runtime-support.json',
           'content/runtime-evidence/coverage.json','design/RECIPE-CATALOGUE.md','design/TIMING-AND-PERSISTENCE.md',
           'design/PART-RESALE.md','design/UPGRADE-SYSTEM.md','design/data/part-resale-bases.json',
           'design/data/beta-balance-v1.json','design/data/shared-upgrades.json','design/data/character-mayor-upgrades.json']
    paths={GAME/name for name in names}
    for folder in ['core/src','core/include']:
        paths.update(p for p in (GAME/folder).rglob('*') if p.suffix in {'.cpp','.hpp','.inl','.inc'})
    paths.update(p for p in HERE.iterdir() if p.is_file())
    paths.update((HERE/'history').glob('*.json'))
    return {p.relative_to(GAME).as_posix():sha(p) for p in sorted(paths)}
def run(command,log):
    result=subprocess.run([str(v) for v in command],capture_output=True,text=True)
    text=result.stdout+result.stderr
    log.write_text(text,encoding='utf-8',newline='\n')
    if result.returncode: raise RuntimeError(f'Exit {result.returncode}: {log}')
    return text
def groups(text): return [line[5:] for line in text.splitlines() if line.startswith('PASS ')]
def portable(command,cmake):
    return [str(v).replace(str(GAME),'<game>').replace(str(cmake),'cmake').replace(str(cmake.with_name('ctest.exe')),'ctest') for v in command]
def obligations():
    source=json.loads((HERE/'source-contracts.json').read_text(encoding='utf-8'))
    catalogue=json.loads((GAME/'content/catalogue.snapshot.json').read_text(encoding='utf-8'))['recipes']
    coverage=json.loads((GAME/'content/runtime-evidence/coverage.json').read_text(encoding='utf-8'))['decisions']
    expected={key:catalogue[key.split(':')[1]] for key,row in coverage.items() if key.startswith('recipe:SH') and row['status']=='unbound'}
    assert source==expected and len(source)==101
    return source
def verify():
    source=obligations()
    record=json.loads((EVIDENCE/'results.json').read_text(encoding='utf-8'))
    assert record['inputs']==inputs(),'Live inputs changed; review archived inputs instead'
    archive=GAME/record['archive']
    for name,digest in record['inputs'].items(): assert sha(archive/'inputs'/name)==digest,name
    for name,digest in record['archive_artifacts'].items(): assert sha(archive/name)==digest,name
    actual=set()
    for entry in record['executions']:
        assert sha(GAME/entry['executable'])==entry['executable_sha256']
        assert sha(EVIDENCE/entry['log'])==entry['log_sha256']
        named=groups((EVIDENCE/entry['log']).read_text(encoding='utf-8'))
        assert len(named)==entry['named_groups']
        if entry['suite']=='contracts': actual={'contracts::'+name for name in named}
    assert actual=={row['id'] for row in record['tests']} and len(actual)==113
    assert all(row['result']=='passed' for row in record['tests'])
    proposal=json.loads((EVIDENCE/'proposed-bindings.json').read_text(encoding='utf-8'))
    assert proposal['evidence']['sha256']==sha(EVIDENCE/'results.json')
    assert set(proposal['bindings'])==set(source)-set(RESIDUAL) and len(proposal['bindings'])==100
    assert proposal['unbound']==RESIDUAL
    for key,row in proposal['bindings'].items():
        assert row['source_contract']==source[key]
        assert row['tests'] and set(row['tests'])<=actual
        assert any(test.split(' ',1)[0]=='contracts::'+key for test in row['tests'])
        for implementation in row['implementations']:
            assert record['inputs'][implementation['path']]==implementation['sha256']
    for entry in record['build_commands']: assert sha(GAME/entry['log'])==entry['log_sha256']
    assert sha(GAME/record['library']['path'])==record['library']['sha256']
    print('PASS graph and artifact identities; 113 executed groups, 100 proposed IDs, SH064 explicit residual.')
def capture(cmake):
    source=obligations();before=inputs()
    BUILD.mkdir(parents=True,exist_ok=True);EVIDENCE.mkdir(exist_ok=True)
    commands=[[cmake,'-S',HERE,'-B',BUILD,'-G','Visual Studio 17 2022','-A','x64'],
              [cmake,'--build',BUILD,'--config','Release','--clean-first','--parallel','4']]
    builds=[]
    for i,command in enumerate(commands):
        log=BUILD/f'capture-build-{i}.log';run(command,log)
        builds.append({'command':portable(command,cmake),'exit_code':0,'log':log.relative_to(GAME).as_posix(),'log_sha256':sha(log)})
    executions=[];tests=[];names={}
    for suite,target in TARGETS.items():
        executable=BUILD/'Release'/(target+'.exe');log=EVIDENCE/(suite+'.log')
        text=run([executable],log);named=groups(text)
        entry={'suite':suite,'command':[executable.relative_to(GAME).as_posix()],'exit_code':0,
               'executable':executable.relative_to(GAME).as_posix(),'executable_sha256':sha(executable),
               'log':log.name,'log_sha256':sha(log),'named_groups':len(named)}
        if suite=='contracts':
            match=re.search(r'SUMMARY 113 passed, 0 failed, (\d+) assertions',text);assert match and len(named)==len(set(named))==113
            names={line.split(' ',1)[0].split(':',1)[1]:'contracts::'+line for line in named}
            assert set(names)=={key.split(':')[1] for key in source}|set(SUPPLEMENTAL)
            entry['assertions']=int(match[1]);tests=[{'id':'contracts::'+line,'result':'passed'} for line in named]
        else:
            match=re.search(r'All (\d+) recipe assertions passed',text);assert match
            entry['assertions']=int(match[1])
        executions.append(entry)
    ctest=[cmake.with_name('ctest.exe'),'--test-dir',BUILD,'-C','Release','--output-on-failure'];log=BUILD/'capture-ctest.log';run(ctest,log)
    builds.append({'command':portable(ctest,cmake),'exit_code':0,'log':log.relative_to(GAME).as_posix(),'log_sha256':sha(log)})
    assert before==inputs(),'Source changed during native execution'
    graph=hashlib.sha256(encoded(before)).hexdigest();identity='native-shared-recipes-'+executions[0]['executable_sha256'][:16]
    archive=BUILD/'captures'/(identity+'-'+graph[:12]);archive.mkdir(parents=True,exist_ok=False)
    for name in before:
        target=archive/'inputs'/name;target.parent.mkdir(parents=True,exist_ok=True);shutil.copy2(GAME/name,target)
    for entry in executions:
        shutil.copy2(GAME/entry['executable'],archive/Path(entry['executable']).name);shutil.copy2(EVIDENCE/entry['log'],archive/entry['log'])
    for entry in builds: shutil.copy2(GAME/entry['log'],archive/Path(entry['log']).name)
    library=BUILD/'core-runtime/Release/overkill_core.lib';shutil.copy2(library,archive/library.name)
    assert before==inputs()=={name:sha(archive/'inputs'/name) for name in before}
    compiler=''
    for path in (BUILD/'CMakeFiles').glob('*/CMakeCXXCompiler.cmake'):
        match=re.search(r'set\(CMAKE_CXX_COMPILER_VERSION "([^"]+)"\)',path.read_text())
        if match: compiler='MSVC '+match[1]
    assert compiler
    record={'schema_version':1,'verified_build':identity,'captured_utc':datetime.datetime.now(datetime.timezone.utc).isoformat(),
            'status':'Author-operated provisional corrected of-core-0.4. Save/version decision pending; no support ledger activated.',
            'manifest_sha256':sha(GAME/'content/cinderwall.manifest.json'),'compiler':compiler,'flags':'/W4 /WX /permissive- C++17 Release x64',
            'inputs':before,'graph_sha256':graph,'executions':executions,'tests':tests,'build_commands':builds,
            'library':{'path':library.relative_to(GAME).as_posix(),'sha256':sha(library)},'archive':archive.relative_to(GAME).as_posix(),
            'archive_artifacts':{p.name:sha(p) for p in sorted(archive.iterdir()) if p.is_file()},
            'coverage':{'primary_recipe_ids':101,'supplemental_groups':12,'proposed_full_bindings':100,'residual_ids':1},
            'limitations':['Controlled numerical fixtures are not naturally earned builds or balance results.','SH064 three unavailable status prevention branches unclaimed.','Finite source-clause author coverage; independent review pending.','No graphical or human validation.']}
    write(EVIDENCE/'results.json',record)
    proposal={'schema_version':1,'status':'PROPOSED ONLY; does not activate runtime support','verified_build':identity,
              'evidence':{'path':(EVIDENCE/'results.json').relative_to(GAME).as_posix(),'sha256':sha(EVIDENCE/'results.json')},
              'bindings':{},'unbound':RESIDUAL}
    for key,row in source.items():
        if key in RESIDUAL: continue
        id=key.split(':')[1]
        linked=[names[id]]+[names[name] for name,ids in SUPPLEMENTAL.items() if id in ids]
        proposal['bindings'][key]={'handler':'Engine paid recipe Use, source-specific catalogue effect, public placement/Fire/round/collection actions',
                                  'source_contract':row,'selected_precedence':'design/TIMING-AND-PERSISTENCE.md; RECIPE-CATALOGUE first-installation and generated-part clarifications',
                                  'implementations':[{'path':path,'sha256':before[path]} for path in IMPLEMENTATIONS],
                                  'tests':linked,'claim_boundary':'Printed source contract plus named boundary assertions; physical-type proofs remain separate; not exhaustive interaction coverage.'}
    write(EVIDENCE/'proposed-bindings.json',proposal);verify()
    print(json.dumps({'verified_build':identity,'graph_sha256':graph,'input_count':len(before),'assertions':executions[0]['assertions'],'archive':record['archive']},indent=2))
def main():
    parser=argparse.ArgumentParser(description=__doc__);mode=parser.add_mutually_exclusive_group(required=True)
    mode.add_argument('--capture',action='store_true');mode.add_argument('--check',action='store_true');parser.add_argument('--cmake',type=Path,default=DEFAULT_CMAKE)
    args=parser.parse_args();capture(args.cmake) if args.capture else verify()
if __name__=='__main__': main()
