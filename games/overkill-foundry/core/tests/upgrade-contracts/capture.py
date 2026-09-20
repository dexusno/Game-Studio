"""Capture the executed upgrade contracts without activating runtime support."""
import argparse
import datetime
import hashlib
import json
from pathlib import Path
import re
import shutil
import subprocess
import sys

from generate_contracts import encoded, make_contracts

HERE = Path(__file__).resolve().parent
GAME = HERE.parents[2]
BUILD = GAME / "core/build/upgrade-contracts"
EVIDENCE = HERE / "evidence"
DEFAULT_CMAKE = Path("C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe")
TARGETS = {
    "contracts": "upgrade_contract_tests",
    "legacy-upgrades": "upgrade_contract_legacy_upgrades",
    "campaign-upgrades": "upgrade_contract_campaign_upgrades",
    "campaign": "upgrade_contract_campaign",
}


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def write(path, value):
    path.write_bytes(encoded(value))


def inputs():
    names = [
        "core/CMakeLists.txt", "core/tests/upgrade_contract_tests.cpp", "core/tests/upgrade_tests.cpp",
        "core/tests/campaign_upgrade_tests.cpp", "core/tests/campaign_tests.cpp", "presentation/precision.hpp",
        "content/cinderwall.manifest.json", "content/catalogue.snapshot.json", "content/runtime-support.json",
        "content/runtime-evidence/coverage.json", "design/RECIPE-CATALOGUE.md", "design/TIMING-AND-PERSISTENCE.md",
        "design/UPGRADE-SYSTEM.md", "design/PART-RESALE.md", "design/data/part-resale-bases.json",
        "design/data/shared-upgrades.json", "design/data/character-mayor-upgrades.json", "design/data/beta-balance-v1.json",
    ]
    paths = {GAME / name for name in names}
    for folder in (GAME / "core/src", GAME / "core/include"):
        paths.update(p for p in folder.rglob("*") if p.suffix in {".cpp", ".hpp", ".inl", ".inc"})
    paths.update(p for p in HERE.iterdir() if p.is_file())
    paths.update((HERE / "history").glob("*.json"))
    return {p.relative_to(GAME).as_posix(): sha(p) for p in sorted(paths)}


def run(command, path):
    result = subprocess.run([str(x) for x in command], capture_output=True, text=True)
    text = result.stdout + result.stderr
    path.write_text(text, encoding="utf-8", newline="\n")
    if result.returncode:
        raise RuntimeError(f"Exit {result.returncode}: {path}")
    return text


def portable(command, cmake):
    return [str(value).replace(str(GAME), "<game>").replace(str(cmake), "cmake").replace(str(cmake.with_name("ctest.exe")), "ctest") for value in command]


def verify():
    record = json.loads((EVIDENCE / "results.json").read_text(encoding="utf-8"))
    assert record["inputs"] == inputs(), "Current graph differs from executed graph"
    assert (HERE / "contracts.json").read_bytes() == encoded(make_contracts())
    archive = GAME / record["archive"]
    for name, digest in record["inputs"].items():
        assert sha(archive / "inputs" / name) == digest, name
    for name, digest in record["archive_artifacts"].items():
        assert sha(archive / name) == digest, name
    fixtures = set()
    for execution in record["executions"]:
        assert sha(GAME / execution["executable"]) == execution["executable_sha256"]
        assert sha(EVIDENCE / execution["log"]) == execution["log_sha256"]
        if execution["suite"] == "contracts":
            text = (EVIDENCE / execution["log"]).read_text(encoding="utf-8")
            fixtures = {"contracts::" + line[5:] for line in text.splitlines() if line.startswith("PASS ")}
            assert len(fixtures) == execution["named_groups"]
    assert fixtures == {test["id"] for test in record["tests"]}
    assert all(test["result"] == "passed" for test in record["tests"])
    proposal = json.loads((EVIDENCE / "proposed-bindings.json").read_text(encoding="utf-8"))
    assert proposal["evidence"]["sha256"] == sha(EVIDENCE / "results.json")
    assert proposal["verified_build"] == record["verified_build"]
    contracts = make_contracts()
    expected = {key for key, value in contracts["obligations"].items() if value["propose_full_binding"]}
    assert set(proposal["bindings"]) == expected and len(expected) == 94
    assert len(proposal["unbound"]) == 6
    for operation, binding in proposal["bindings"].items():
        assert set(binding["tests"]) <= fixtures and binding["tests"]
        for implementation in binding["implementations"]:
            assert record["inputs"][implementation["path"]] == implementation["sha256"]
        assert binding["tests"] == [fixture["id"] for fixture in contracts["obligations"][operation]["fixtures"]]
    for build in record["build_commands"]:
        assert sha(GAME / build["log"]) == build["log_sha256"]
    assert sha(GAME / record["library"]["path"]) == record["library"]["sha256"]
    print(f"PASS proposed linkage: {len(fixtures)} actual named results, 94 proposed IDs, 6 residual IDs; source/artifact graph verified.")


def capture(cmake):
    contract = make_contracts()
    assert (HERE / "contracts.json").read_bytes() == encoded(contract), "Regenerate authored review index first"
    before = inputs()
    BUILD.mkdir(parents=True, exist_ok=True)
    EVIDENCE.mkdir(exist_ok=True)
    build_commands = []
    commands = [
        [cmake, "-S", HERE, "-B", BUILD, "-G", "Visual Studio 17 2022", "-A", "x64"],
        [cmake, "--build", BUILD, "--config", "Release", "--clean-first", "--parallel", "4"],
    ]
    for index, command in enumerate(commands):
        log = BUILD / f"capture-build-{index}.log"
        run(command, log)
        build_commands.append({"command": portable(command, cmake), "exit_code": 0,
                               "log": log.relative_to(GAME).as_posix(), "log_sha256": sha(log)})
    executions, tests = [], []
    for suite, target in TARGETS.items():
        executable = BUILD / "Release" / (target + ".exe")
        log = EVIDENCE / (suite + ".log")
        text = run([executable], log)
        named = [line[5:] for line in text.splitlines() if line.startswith("PASS ")]
        entry = {"suite": suite, "command": [executable.relative_to(GAME).as_posix()], "exit_code": 0,
                 "executable": executable.relative_to(GAME).as_posix(), "executable_sha256": sha(executable),
                 "log": log.name, "log_sha256": sha(log), "named_groups": len(named)}
        if suite == "contracts":
            expected_groups = contract["named_upgrade_cases"] + contract["supplemental_cases"]
            summary = re.search(rf"SUMMARY {expected_groups} passed, 0 failed, (\d+) assertions", text)
            assert summary and len(named) == len(set(named)) == expected_groups
            entry["assertions"] = int(summary[1])
            tests = [{"id": "contracts::" + group, "result": "passed"} for group in named]
        executions.append(entry)
    ctest = [cmake.with_name("ctest.exe"), "--test-dir", BUILD, "-C", "Release", "--output-on-failure"]
    ctest_log = BUILD / "capture-ctest.log"
    run(ctest, ctest_log)
    build_commands.append({"command": portable(ctest, cmake), "exit_code": 0,
                           "log": ctest_log.relative_to(GAME).as_posix(), "log_sha256": sha(ctest_log)})
    assert before == inputs(), "Inputs changed during build/execution"
    graph = hashlib.sha256(encoded(before)).hexdigest()
    identity = "native-upgrade-contracts-" + executions[0]["executable_sha256"][:16]
    archive = BUILD / "captures" / (identity + "-" + graph[:12])
    if archive.exists():
        raise RuntimeError("Refuse to overwrite existing capture")
    archive.mkdir(parents=True)
    for name in before:
        output = archive / "inputs" / name
        output.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(GAME / name, output)
    for execution in executions:
        shutil.copy2(GAME / execution["executable"], archive / Path(execution["executable"]).name)
        shutil.copy2(EVIDENCE / execution["log"], archive / execution["log"])
    for build in build_commands:
        shutil.copy2(GAME / build["log"], archive / Path(build["log"]).name)
    library = BUILD / "core-runtime/Release/overkill_core.lib"
    shutil.copy2(library, archive / library.name)
    assert before == inputs() == {name: sha(archive / "inputs" / name) for name in before}
    compiler = ""
    for file in (BUILD / "CMakeFiles").glob("*/CMakeCXXCompiler.cmake"):
        match = re.search(r'set\(CMAKE_CXX_COMPILER_VERSION "([^"]+)"\)', file.read_text())
        if match:
            compiler = "MSVC " + match[1]
    assert compiler
    record = {
        "schema_version": 1, "verified_build": identity, "captured_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "status": "Provisional corrected of-core-0.4; exact hash identifies these outcomes; save/version decision pending.",
        "manifest_sha256": contract["manifest_sha256"], "compiler": compiler, "flags": "/W4 /WX /permissive- C++17 Release x64",
        "graph_sha256": graph, "inputs": before, "executions": executions, "tests": tests,
        "build_commands": build_commands, "archive": archive.relative_to(GAME).as_posix(),
        "archive_artifacts": {p.name: sha(p) for p in sorted(archive.iterdir()) if p.is_file()},
        "library": {"path": library.relative_to(GAME).as_posix(), "sha256": sha(library)},
        "coverage": {"new_upgrade_ids_executed": 96, "proposed_full_bindings": 94, "explicit_residual_ids": 6},
        "limitations": contract["scope"],
    }
    write(EVIDENCE / "results.json", record)
    proposal = {
        "schema_version": 1, "status": "PROPOSED ONLY; no runtime-support or release binding changed",
        "verified_build": identity, "manifest_sha256": contract["manifest_sha256"],
        "evidence": {"path": (EVIDENCE / "results.json").relative_to(GAME).as_posix(), "sha256": sha(EVIDENCE / "results.json")},
        "bindings": {}, "unbound": {},
    }
    for operation, row in contract["obligations"].items():
        if not row["propose_full_binding"]:
            proposal["unbound"][operation] = row["residual"]
            continue
        proposal["bindings"][operation] = {
            "handler": "Rules/Engine source-specific upgrade dispatch with production CampaignRules mediation where applicable",
            "implementations": [{"path": path, "sha256": before[path]} for path in row["implementations"]],
            "tests": [fixture["id"] for fixture in row["fixtures"]],
        }
    write(EVIDENCE / "proposed-bindings.json", proposal)
    verify()
    print(json.dumps({"verified_build": identity, "graph_sha256": graph, "inputs": len(before),
                      "named_groups": executions[0]["named_groups"], "assertions": executions[0]["assertions"],
                      "archive": record["archive"]}))


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--capture", action="store_true")
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--cmake", type=Path, default=DEFAULT_CMAKE)
    args = parser.parse_args()
    if args.capture == args.check:
        parser.error("Choose --capture or --check")
    if args.check:
        verify()
    else:
        capture(args.cmake)


if __name__ == "__main__":
    try:
        main()
    except (AssertionError, RuntimeError) as error:
        print(str(error), file=sys.stderr)
        raise SystemExit(1)
