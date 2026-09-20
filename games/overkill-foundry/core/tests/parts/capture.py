"""Build and capture named physical lifecycle evidence without changing runtime support."""
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
BUILD = GAME / "core/build/part-lifecycle"
EVIDENCE = HERE / "evidence"
DEFAULT_CMAKE = Path("C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe")


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def inputs():
    paths = {GAME / "core/CMakeLists.txt", GAME / "core/tests/part_lifecycle_tests.cpp",
             GAME / "content/cinderwall.manifest.json", GAME / "content/catalogue.snapshot.json"}
    paths.update(GAME / name for name in (
        "design/RECIPE-CATALOGUE.md", "design/PART-RESALE.md", "design/TIMING-AND-PERSISTENCE.md",
        "design/data/part-resale-bases.json", "design/data/shared-upgrades.json",
        "design/data/character-mayor-upgrades.json", "design/data/beta-balance-v1.json"))
    for folder in (GAME / "core/src", GAME / "core/include"):
        paths.update(p for p in folder.rglob("*") if p.suffix in {".cpp", ".hpp", ".inl", ".inc"})
    paths.update(p for p in HERE.iterdir() if p.is_file())
    return {p.relative_to(GAME).as_posix(): digest(p) for p in sorted(paths)}


def encoded(value):
    return (json.dumps(value, indent=2, sort_keys=True) + "\n").encode()


def write_json(path, value):
    path.write_bytes(encoded(value))


def run(command, log):
    result = subprocess.run([str(x) for x in command], capture_output=True, text=True)
    text = result.stdout + result.stderr
    log.write_text(text, encoding="utf-8", newline="\n")
    if result.returncode:
        raise RuntimeError(f"Command failed ({result.returncode}); see {log}")
    return text


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--capture", action="store_true", help="Build and replace this new matrix's evidence only")
    parser.add_argument("--check", action="store_true", help="Validate captured identities; execute no tests")
    parser.add_argument("--cmake", type=Path, default=DEFAULT_CMAKE)
    args = parser.parse_args()
    assert args.capture != args.check, "Choose --capture or --check"
    contracts = json.loads((HERE / "contracts.json").read_text(encoding="utf-8"))
    selected = {key for key, row in contracts["obligations"].items() if row["selected"]}
    residual = {key: row["residual"] for key, row in contracts["obligations"].items() if not row["selected"]}
    expected = len(selected)
    assert expected == contracts["selected_for_execution"] == 207 and len(residual) == 0
    if args.check:
        record = json.loads((EVIDENCE / "results.json").read_text(encoding="utf-8"))
        assert inputs() == record["inputs"], "Captured input graph changed"
        actual_tests = set()
        for suite in record["executions"]:
            assert digest(GAME / suite["executable"]) == suite["executable_sha256"]
            assert digest(EVIDENCE / suite["log"]) == suite["log_sha256"]
            groups = [line[5:] for line in (EVIDENCE / suite["log"]).read_text(encoding="utf-8").splitlines() if line.startswith("PASS ")]
            assert {group.split()[0] for group in groups} == selected and len(groups) == expected
            actual_tests.update(suite["suite"] + "::" + group for group in groups)
        for build in record["build_commands"]:
            assert digest(GAME / build["log"]) == build["log_sha256"]
        assert digest(GAME / record["library"]["path"]) == record["library"]["sha256"]
        assert len(record["tests"]) == expected and all(t["result"] == "passed" for t in record["tests"])
        assert actual_tests == {test["id"] for test in record["tests"]}
        proposal = json.loads((EVIDENCE / "proposed-bindings.json").read_text(encoding="utf-8"))
        assert proposal["evidence"]["sha256"] == digest(EVIDENCE / "results.json")
        assert {binding["operation"] for binding in proposal["bindings"]} == selected
        assert {binding["fixture"] for binding in proposal["bindings"]} == actual_tests
        assert all(binding["operation"] == binding["fixture"].split("::")[1].split()[0] for binding in proposal["bindings"])
        assert proposal["unbound"] == residual
        archive = GAME / record["archive"]["root"]
        for name, expected_hash in record["inputs"].items():
            assert digest(archive / "inputs" / name) == expected_hash
        for name, expected_hash in record["archive"]["artifacts"].items():
            assert digest(archive / name) == expected_hash
        print(f"PASS lifecycle linkage: {expected} physical groups; {len(residual)} residual output type.")
        return
    EVIDENCE.mkdir(exist_ok=True)
    BUILD.mkdir(parents=True, exist_ok=True)
    before = inputs()
    commands = [
        [args.cmake, "-S", HERE, "-B", BUILD, "-G", "Visual Studio 17 2022", "-A", "x64"],
        [args.cmake, "--build", BUILD, "--config", "Release", "--clean-first", "--parallel", "4"],
    ]
    build_logs = []
    for index, command in enumerate(commands):
        log = BUILD / f"capture-build-{index}.log"
        run(command, log)
        build_logs.append({"command": [str(x).replace(str(GAME), "<game>") for x in command],
                           "exit_code": 0, "log": log.relative_to(GAME).as_posix(), "log_sha256": digest(log)})
    tests, executions = [], []
    for label, name in (("parts", "part_lifecycle_tests"),):
        executable = BUILD / "Release" / f"{name}.exe"
        log = EVIDENCE / f"{label}.log"
        output = run([executable], log)
        groups = [line[5:] for line in output.splitlines() if line.startswith("PASS ")]
        assert len(groups) == expected, "Unexpected named PASS set"
        assert len(groups) == len(set(groups))
        if label == "parts":
            assert {g.split()[0] for g in groups} == selected
            summary = re.search(rf"SUMMARY {expected} passed, 0 failed, (\d+) assertions; {len(residual)} explicitly untested", output)
            assert summary, "Expected successful bounded summary"
        tests.extend({"id": f"{label}::{group}", "result": "passed"} for group in groups)
        executions.append({"suite": label, "command": [executable.relative_to(GAME).as_posix()], "exit_code": 0,
                           "executable": executable.relative_to(GAME).as_posix(), "executable_sha256": digest(executable),
                           "log": log.name, "log_sha256": digest(log), "named_groups": len(groups), "assertions": int(summary.group(1))})
    ctest_command = [args.cmake.with_name("ctest.exe"), "--test-dir", BUILD, "-C", "Release", "--output-on-failure"]
    ctest_log = BUILD / "capture-ctest.log"
    run(ctest_command, ctest_log)
    build_logs.append({"command": [str(x).replace(str(GAME), "<game>") for x in ctest_command],
                       "exit_code": 0, "log": ctest_log.relative_to(GAME).as_posix(), "log_sha256": digest(ctest_log)})
    assert inputs() == before, "Sources changed during compilation/execution"
    library = BUILD / "core-runtime/Release/overkill_core.lib"
    graph = hashlib.sha256(encoded(before)).hexdigest()
    build = "native-part-lifecycle-" + executions[0]["executable_sha256"][:16]
    archive = BUILD / "captures" / (build + "-" + graph[:12])
    archive.mkdir(parents=True, exist_ok=True)
    for name, expected_hash in before.items():
        target = archive / "inputs" / name
        target.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(GAME / name, target)
        assert digest(target) == expected_hash, "Source changed while preserving capture"
    artifacts = {}
    for source in [BUILD / "Release/part_lifecycle_tests.exe", library, EVIDENCE / "parts.log"] + [GAME / log["log"] for log in build_logs]:
        shutil.copyfile(source, archive / source.name)
        artifacts[source.name] = digest(source)
    record = {"verified_build": build, "manifest_sha256": digest(GAME / "content/cinderwall.manifest.json"),
              "captured_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
              "source_graph_sha256": graph, "inputs": before, "build_commands": build_logs,
              "supported_type_candidates": expected, "explicit_residual": residual,
              "rules_status": "Provisional corrected of-core-0.4; version/save compatibility decision pending. A later source/version change requires a new capture.",
              "archive": {"root": archive.relative_to(GAME).as_posix(), "artifacts": artifacts},
              "executions": executions, "tests": tests,
              "library": {"path": library.relative_to(GAME).as_posix(), "sha256": digest(library)},
              "scope": "Author-operated physical lifecycle matrix, pending independent review; no full recipe payload, balance, release or human acceptance claim. Runtime support is unchanged."}
    write_json(EVIDENCE / "results.json", record)
    proposal = {"notice": "Proposed only. Each pair names an actual executed group; the current runtime support ledger is unchanged.",
                "verified_build": build, "manifest_sha256": record["manifest_sha256"],
                "handler": "Engine::recipePart/plain/receive/printedCopy/copyLater/completeFire/install/catalogueActivate/collectionEffects; upgradePartSaleValue; CampaignRules::apply/clearFight",
                "implementations": {path: digest(GAME / path) for path in (
                    "core/src/core.cpp", "core/src/recipe_effects.inl", "core/src/upgrade_effects.inl", "core/src/upgrades.cpp", "core/src/campaign.cpp")},
                "evidence": {"path": (EVIDENCE / "results.json").relative_to(GAME).as_posix(), "sha256": digest(EVIDENCE / "results.json")},
                "contracts": {"path": (HERE / "contracts.json").relative_to(GAME).as_posix(), "sha256": digest(HERE / "contracts.json")},
                "bindings": [{"operation": test["id"].split("::")[1].split()[0], "fixture": test["id"]} for test in tests if test["id"].startswith("parts::")],
                "unbound": residual}
    write_json(EVIDENCE / "proposed-bindings.json", proposal)
    print(f"PASS captured {build}: {expected} physical groups, {len(residual)} unbound output type; source graph {graph}")


if __name__ == "__main__":
    main()
