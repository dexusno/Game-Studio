"""Preserve the initial executed UGS-095 failure before its authorized repair."""
import hashlib
import json
from pathlib import Path
import shutil
import subprocess

HERE = Path(__file__).resolve().parent
GAME = HERE.parents[2]
BUILD = GAME / "core/build/upgrade-contracts"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    sources = {GAME / "core/CMakeLists.txt", GAME / "core/tests/upgrade_contract_tests.cpp",
               GAME / "content/cinderwall.manifest.json", GAME / "content/runtime-evidence/coverage.json",
               GAME / "design/RECIPE-CATALOGUE.md", GAME / "design/TIMING-AND-PERSISTENCE.md",
               GAME / "design/data/shared-upgrades.json", GAME / "design/data/character-mayor-upgrades.json",
               GAME / "presentation/precision.hpp"}
    for folder in (GAME / "core/src", GAME / "core/include"):
        sources.update(p for p in folder.rglob("*") if p.suffix in {".cpp", ".hpp", ".inl", ".inc"})
    sources.update(p for p in HERE.iterdir() if p.is_file())
    before = {p.relative_to(GAME).as_posix(): sha(p) for p in sorted(sources)}
    executable = BUILD / "Release/upgrade_contract_tests.exe"
    identity = sha(executable)
    archive = BUILD / "history" / ("ugs095-failure-" + identity[:16])
    if archive.exists():
        raise RuntimeError("Refuse to overwrite archived failure")
    archive.mkdir(parents=True)
    run = subprocess.run([executable], text=True, capture_output=True)
    log = run.stdout + run.stderr
    assert run.returncode == 1 and "FAIL upgrade:UGS-095:" in log
    assert log.count("FAIL upgrade:") == 1, log
    (archive / "run.log").write_text(log, encoding="utf-8", newline="\n")
    shutil.copy2(executable, archive / executable.name)
    shutil.copy2(BUILD / "core-runtime/Release/overkill_core.lib", archive / "overkill_core.lib")
    for name in before:
        output = archive / "inputs" / name
        output.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(GAME / name, output)
    assert before == {name: sha(GAME / name) for name in before}
    assert before == {name: sha(archive / "inputs" / name) for name in before}
    record = {"status": "historical failing reproduction, not proposed support", "exit_code": run.returncode,
              "command": ["core/build/upgrade-contracts/Release/upgrade_contract_tests.exe"],
              "archive": archive.relative_to(GAME).as_posix(), "inputs": before,
              "artifacts": {name: sha(archive / name) for name in (executable.name, "overkill_core.lib", "run.log")}}
    evidence = HERE / "history"
    evidence.mkdir(exist_ok=True)
    (evidence / "ugs095-failure.json").write_text(json.dumps(record, indent=2, sort_keys=True) + "\n", encoding="utf-8")
    print(json.dumps({"archive": record["archive"], "executable_sha256": identity, "copied_inputs": len(before)}))


if __name__ == "__main__":
    main()
