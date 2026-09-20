"""Preserve an actually failed native run and the complete source graph before further work."""
import hashlib
import json
from pathlib import Path
import shutil
import subprocess
import sys
HERE = Path(__file__).resolve().parent
GAME = HERE.parents[2]
BUILD = GAME / 'core/build/shared-recipes'
def sha(p): return hashlib.sha256(p.read_bytes()).hexdigest()
def main():
    label = sys.argv[1]
    exe = BUILD / 'Release/shared_recipe_contract_tests.exe'
    sources = {GAME / 'core/CMakeLists.txt', GAME / 'core/tests/shared_recipe_contract_tests.cpp'}
    for folder in ['core/include','core/src']:
        sources.update(p for p in (GAME / folder).rglob('*') if p.suffix in {'.hpp','.cpp','.inc','.inl'})
    sources.update(p for p in HERE.iterdir() if p.is_file())
    sources.update(GAME / p for p in ['content/catalogue.snapshot.json','content/cinderwall.manifest.json','design/RECIPE-CATALOGUE.md','design/TIMING-AND-PERSISTENCE.md'])
    before = {p.relative_to(GAME).as_posix():sha(p) for p in sorted(sources)}
    result = subprocess.run([str(exe)], capture_output=True, text=True)
    assert result.returncode == 1
    archive = BUILD / 'history' / (label + '-' + sha(exe)[:16])
    archive.mkdir(parents=True,exist_ok=False)
    (archive/'failed.log').write_text(result.stdout+result.stderr,encoding='utf-8')
    shutil.copy2(exe, archive / exe.name)
    for name in before:
        target=archive/'inputs'/name
        target.parent.mkdir(parents=True,exist_ok=True)
        shutil.copy2(GAME/name,target)
    assert before == {name:sha(GAME/name) for name in before} == {name:sha(archive/'inputs'/name) for name in before}
    record={'label':label,'archive':archive.relative_to(GAME).as_posix(),'exit_code':1,'inputs':before,
            'executable_sha256':sha(exe),'log_sha256':sha(archive/'failed.log'),
            'failures':[line for line in result.stderr.splitlines() if line.startswith('FAIL ')]}
    (HERE/'history').mkdir(exist_ok=True)
    (HERE/'history'/(label+'.json')).write_text(json.dumps(record,indent=2)+'\n',encoding='utf-8')
    print(json.dumps({key:value for key,value in record.items() if key!='inputs'},indent=2))
if __name__=='__main__':main()
