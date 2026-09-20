#!/usr/bin/env python3
"""Capture actual named native fixture results and generate a partial release map.

This does not infer support from registration, source prose, ID mentions, or a
suite exit code. The explicit cases file is the reviewed coverage decision;
fixture selectors must resolve to exactly one name printed by a passing test.
"""
from __future__ import annotations

import argparse
import copy
import hashlib
import json
import re
import subprocess
import sys
from collections import Counter
from datetime import datetime, timezone
from pathlib import Path

GAME = Path(__file__).resolve().parents[1]
OUT = GAME / "content/runtime-evidence"
BUILD = GAME / "core/build/runtime-linkage"
CASES = GAME / "tools/runtime-binding-cases.json"
SUPPORT = GAME / "content/runtime-support.json"
SUITES = {
    "core": "core/tests/core_tests.cpp",
    "recipes": "core/tests/recipe_tests.cpp",
    "upgrades": "core/tests/upgrade_tests.cpp",
    "robots": "core/tests/robot_tests.cpp",
    "route": "core/tests/route_tests.cpp",
    "campaign": "core/tests/campaign_tests.cpp",
    "adapters": "core/tests/campaign_upgrade_tests.cpp",
    "foundation": "qa/foundation_probes.cpp",
    "expanded": "qa/expanded/probes.cpp",
    "p08": "qa/upgrades/probes.cpp",
    "p08-campaign": "qa/upgrades/campaign/probes.cpp",
}


def need(value, message):
    if not value:
        raise ValueError(message)


def encoded(value):
    return (json.dumps(value, ensure_ascii=False, indent=2, sort_keys=True) + "\n").encode("utf-8")


def digest(value):
    return hashlib.sha256(value).hexdigest()


def relative(path):
    return path.relative_to(GAME).as_posix()


def read_json(path):
    return json.loads(path.read_text(encoding="utf-8"))


def hashes():
    paths = [GAME / "core/CMakeLists.txt", GAME / "content/cinderwall.manifest.json"]
    paths += sorted((GAME / "core/include").rglob("*.hpp"))
    paths += sorted((GAME / "core/src").glob("*.cpp"))
    paths += sorted((GAME / "core/src").glob("*.inl"))
    paths += sorted((GAME / "core/src").glob("*.inc"))
    paths += sorted((GAME / "platform/include").rglob("*.hpp"))
    paths += [GAME / "platform/src/campaign_session.cpp", GAME / "platform/src/windows_save_store.cpp"]
    paths += [GAME / name for name in SUITES.values()]
    return {relative(path): digest(path.read_bytes()) for path in sorted(set(paths))}


def run(command, log=None):
    result = subprocess.run([str(v) for v in command], cwd=GAME, stdout=subprocess.PIPE,
                            stderr=subprocess.STDOUT, encoding="utf-8", errors="replace")
    if log:
        log.write_text(result.stdout, encoding="utf-8", newline="\n")
    need(result.returncode == 0, f"Command failed ({result.returncode}): {command}\n{result.stdout[-8000:]}")
    return result.stdout


def fixture_names(output):
    # Totals and nested process-exit diagnostics are not semantic fixture names.
    names = [line[5:] for line in output.splitlines() if line.startswith("PASS ")
             and not re.match(r"PASS (?:[0-9]|process exit )", line)]
    need(len(names) == len(set(names)), "Duplicate named fixture result")
    need(names, "No actual named PASS results were emitted")
    need(not re.search(r"^FAIL\b", output, re.M), "A fixture reported failure")
    return names


def capture(cmake):
    need(cmake and Path(cmake).is_file(), "--run requires --cmake with an existing CMake executable")
    OUT.mkdir(parents=True, exist_ok=True)
    project = BUILD / "project"
    project.mkdir(parents=True, exist_ok=True)
    before = hashes()
    lines = ["cmake_minimum_required(VERSION 3.20)", "project(RuntimeLinkage LANGUAGES CXX)",
             "set(CMAKE_CXX_STANDARD 17)", "set(CMAKE_CXX_STANDARD_REQUIRED ON)",
             "set(OVERKILL_BUILD_CORE_TOOLS OFF CACHE BOOL \"No runner or unrelated targets\" FORCE)",
             f'add_subdirectory("{(GAME / "core").as_posix()}" core-runtime EXCLUDE_FROM_ALL)',
             'set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")']
    for suite, source in SUITES.items():
        target = "runtime_" + suite.replace("-", "_")
        # An unchanged copy keeps QA's __FILE__-relative crash artifacts in this
        # ignored build, rather than overwriting any historic QA evidence.
        copied = project / (target + ".cpp")
        copied.write_bytes((GAME / source).read_bytes())
        need(digest(copied.read_bytes()) == before[source], "Probe copy changed bytes")
        lines += [f'add_executable({target} "{copied.as_posix()}")',
                  f"target_link_libraries({target} PRIVATE overkill_core)"]
        if suite == "robots":
            lines.append(f"target_compile_definitions({target} PRIVATE OVERKILL_ROBOT_INTEGRATION=1)")
        if suite in {"foundation", "expanded", "p08", "p08-campaign"}:
            lines.append(f"target_compile_options({target} PRIVATE /W4 /WX /permissive-)")
        if suite == "p08-campaign":
            for source_file in ("campaign_session.cpp", "windows_save_store.cpp"):
                lines.append(f'target_sources({target} PRIVATE "{(GAME / "platform/src" / source_file).as_posix()}")')
            lines += [f'target_include_directories({target} PRIVATE "{(GAME / "platform/include").as_posix()}")',
                      f"target_compile_definitions({target} PRIVATE NOMINMAX WIN32_LEAN_AND_MEAN)",
                      f"target_link_libraries({target} PRIVATE bcrypt)"]
    cmake_text = "\n".join(lines) + "\n"
    (project / "CMakeLists.txt").write_text(cmake_text, encoding="utf-8", newline="\n")
    cmake_version = run([cmake, "--version"]).splitlines()[0]
    configure = [cmake, "-S", relative(project), "-B", relative(BUILD / "native"),
                 "-G", "Visual Studio 17 2022", "-A", "x64"]
    compile_command = [cmake, "--build", relative(BUILD / "native"), "--config", "Release", "--parallel", "4"]
    print("Configuring and building one current shared-core library and 11 fixture executables...", flush=True)
    run(configure, BUILD / "configure.log")
    run(compile_command, BUILD / "build.log")
    lib = BUILD / "native/core-runtime/Release/overkill_core.lib"
    need(lib.is_file(), "Actual built core library missing")
    library_hash = digest(lib.read_bytes())
    build_id = "MSVC-x64-Release/overkill_core.lib/" + library_hash
    result = {"schema_version": 1, "verified_build": build_id,
              "manifest_sha256": before["content/cinderwall.manifest.json"],
              "captured_utc": datetime.now(timezone.utc).isoformat(),
              "git_head": run(["git", "rev-parse", "HEAD"]).strip(),
              "execution_kind": "New execution of unchanged independent probes and reporting-only author suites against current rules; historic QA reviews are not rewritten.",
              "cmake_version": cmake_version, "configuration": "Visual Studio 17 2022 x64 Release",
              "strict_targets": "Shared core and four independent probe executables use /W4 /WX /permissive-; author fixtures retain their existing compiler settings.",
              "capture_tool_sha256": digest(Path(__file__).read_bytes()),
              "source_sha256": before, "source_unchanged_during_run": False,
              "library": {"path": relative(lib), "sha256": library_hash},
              "commands": [["<cmake>"] + configure[1:], ["<cmake>"] + compile_command[1:]],
              "targets": [], "tests": []}
    for suite, source in SUITES.items():
        executable = BUILD / "native/bin/Release" / ("runtime_" + suite.replace("-", "_") + ".exe")
        log = OUT / (suite + ".txt")
        # Windows resolves an executable containing a relative directory before
        # applying subprocess cwd; execute the absolute path, record it portably.
        output = run([executable], log)
        names = fixture_names(output)
        result["commands"].append([relative(executable)])
        result["targets"].append({"suite": suite, "test_source": source,
                                  "executable": relative(executable), "sha256": digest(executable.read_bytes()),
                                  "exit_code": 0, "named_fixtures": len(names),
                                  "log": relative(log), "log_sha256": digest(log.read_bytes())})
        for name in names:
            result["tests"].append({"id": suite + "::" + name, "name": name, "suite": suite, "result": "passed"})
        print(f"{suite}: {len(names)} actual named fixtures passed", flush=True)
    need(before == hashes(), "A source or test changed during compile/execution; no evidence published")
    result["source_unchanged_during_run"] = True
    (OUT / "results.json").write_bytes(encoded(result))


def select_fixture(selector, results):
    matches = [entry["id"] for entry in results["tests"]
               if entry["result"] == "passed" and entry["id"].startswith(selector)]
    need(len(matches) == 1, f"Fixture selector must match exactly one executed name: {selector} -> {matches}")
    return matches[0]


def generate(results, cases):
    manifest = read_json(GAME / "content/cinderwall.manifest.json")
    required = set(manifest["required_runtime_operations"])
    support = {"schema_version": 1, "manifest_sha256": results["manifest_sha256"],
               "verified_build": results["verified_build"], "bindings": {},
               "notice": "Partial evidence map. Registration and action-path smoke are not full semantic coverage. See runtime-evidence/coverage.json for every residual obligation."}
    decisions = {}
    for case in cases["bindings"]:
        implementation = case["implementation"]
        path = GAME / implementation["path"]
        need(path.resolve().is_relative_to(GAME) and path.is_file(), "Implementation path escapes game or is missing")
        text = path.read_text(encoding="utf-8")
        for symbol in implementation["symbols"]:
            need(re.search(r"\b" + re.escape(symbol) + r"\s*\(", text), f"Actual handler missing: {path.name}:{symbol}")
        fixtures = sorted({select_fixture(selector, results) for selector in case["fixtures"]})
        need(fixtures, "A binding needs actual named fixture results")
        for operation in case["operations"]:
            need(operation in required and operation not in decisions, "Unknown or duplicate operation: " + operation)
            support["bindings"][operation] = {
                "handler": ", ".join(implementation["symbols"]),
                "implementation": {"path": implementation["path"], "sha256": digest(path.read_bytes())},
                "tests": fixtures,
                "evidence": {"path": "content/runtime-evidence/results.json", "sha256": digest(encoded(results))}}
            decisions[operation] = {"status": "bound", "coverage": case["coverage"]}
    for operation in sorted(required - decisions.keys()):
        category = operation.split(":", 1)[0] if ":" in operation else "core"
        reason = cases["unbound_overrides"].get(operation, cases["default_gaps"][category])
        decisions[operation] = {"status": "unbound", **reason}
    need(not (set(cases["unbound_overrides"]) - required), "Gap override names an unknown obligation")
    need(not (set(cases["unbound_overrides"]) & set(support["bindings"])), "An explicitly unproved/held operation cannot be bound")
    count = Counter(operation.split(":", 1)[0] if ":" in operation else "core" for operation in required)
    covered = Counter(operation.split(":", 1)[0] if ":" in operation else "core" for operation in support["bindings"])
    audit = {"schema_version": 1, "verified_build": results["verified_build"],
             "manifest_sha256": results["manifest_sha256"], "required": len(required),
             "bound": len(support["bindings"]), "unbound": len(required) - len(support["bindings"]),
             "by_category": {k: {"required": count[k], "bound": covered[k], "unbound": count[k] - covered[k]} for k in sorted(count)},
             "method": cases["method"], "decisions": dict(sorted(decisions.items())),
             "cases_sha256": digest(CASES.read_bytes())}
    return support, audit


def validate(results, support):
    need(results["source_unchanged_during_run"], "Source changed during capture")
    need(results["capture_tool_sha256"] == digest(Path(__file__).read_bytes()), "Capture tool changed after execution")
    need(results["source_sha256"] == hashes(), "Evidence source/test/manifest hashes are stale")
    need(results["manifest_sha256"] == digest((GAME / "content/cinderwall.manifest.json").read_bytes()), "Manifest hash mismatch")
    recorded = {}
    for target in results["targets"]:
        need(target["exit_code"] == 0, "A target failed")
        log = GAME / target["log"]
        need(digest(log.read_bytes()) == target["log_sha256"], "Fixture log hash mismatch")
        recorded[target["suite"]] = fixture_names(log.read_text(encoding="utf-8"))
        need(len(recorded[target["suite"]]) == target["named_fixtures"], "Fixture count mismatch")
        exe = GAME / target["executable"]
        if exe.exists():
            need(digest(exe.read_bytes()) == target["sha256"], "Recorded executable was replaced")
    expected = {suite + "::" + name for suite, names in recorded.items() for name in names}
    passed = {entry["id"] for entry in results["tests"] if entry["result"] == "passed"}
    need(expected == passed and len(passed) == len(results["tests"]), "Recorded fixture results do not match actual logs")
    required = set(read_json(GAME / "content/cinderwall.manifest.json")["required_runtime_operations"])
    need(support["manifest_sha256"] == results["manifest_sha256"], "Binding manifest mismatch")
    need(support["verified_build"] == results["verified_build"], "Binding build mismatch")
    need(results["verified_build"] == "MSVC-x64-Release/overkill_core.lib/" + results["library"]["sha256"],
         "Build identity is not the captured library hash")
    library = GAME / results["library"]["path"]
    if library.exists():
        need(digest(library.read_bytes()) == results["library"]["sha256"], "Recorded shared library was replaced")
    for operation, binding in support["bindings"].items():
        need(operation in required, "Unknown binding operation")
        need(binding["handler"] and binding["tests"] and set(binding["tests"]) <= passed, "Unproved fixture binding")
        for field in ("implementation", "evidence"):
            entry = binding[field]
            path = (GAME / entry["path"]).resolve()
            need(path.is_relative_to(GAME) and path.is_file(), "Missing/escaping binding path")
            need(digest(path.read_bytes()) == entry["sha256"], "Stale " + field + " hash")


def self_test(results, support):
    first = next(iter(support["bindings"]))
    mutations = [
        ("invented fixture", lambda s: s["bindings"][first].update(tests=["invented/never-executed"])),
        ("stale implementation", lambda s: s["bindings"][first]["implementation"].update(sha256="0" * 64)),
        ("stale evidence", lambda s: s["bindings"][first]["evidence"].update(sha256="0" * 64)),
        ("wrong manifest", lambda s: s.update(manifest_sha256="0" * 64)),
        ("wrong build", lambda s: s.update(verified_build="not-the-built-library")),
        ("empty handler", lambda s: s["bindings"][first].update(handler="")),
        ("unknown operation", lambda s: s["bindings"].update({"recipe:NOT-ELIGIBLE": copy.deepcopy(s["bindings"][first])})),
    ]
    for name, change in mutations:
        bad = copy.deepcopy(support)
        change(bad)
        try:
            validate(results, bad)
        except ValueError:
            print("PASS linkage validator rejects " + name)
        else:
            raise ValueError("Validator accepted " + name)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--run", action="store_true", help="Build and execute current named fixtures before generating")
    parser.add_argument("--cmake", help="Verified installed CMake executable, required with --run")
    parser.add_argument("--check", action="store_true", help="Verify exact existing outputs and evidence, writing nothing")
    parser.add_argument("--self-test", action="store_true", help="Exercise seven false-evidence rejection controls")
    args = parser.parse_args()
    need(not (args.run and args.check), "--run and --check are mutually exclusive")
    if args.run:
        capture(args.cmake)
    results = read_json(OUT / "results.json")
    support, audit = generate(results, read_json(CASES))
    validate(results, support)
    if args.check:
        need(SUPPORT.read_bytes() == encoded(support), "runtime-support.json differs from evidence decisions")
        need((OUT / "coverage.json").read_bytes() == encoded(audit), "coverage.json differs from evidence decisions")
    else:
        SUPPORT.write_bytes(encoded(support))
        (OUT / "coverage.json").write_bytes(encoded(audit))
    if args.self_test:
        self_test(results, support)
    print(f"Runtime linkage: {audit['bound']}/{audit['required']} bound; {audit['unbound']} explicitly unbound. Release remains gated.")


if __name__ == "__main__":
    try:
        main()
    except (OSError, ValueError, KeyError) as error:
        print("RUNTIME EVIDENCE FAILED: " + str(error), file=sys.stderr)
        sys.exit(1)
