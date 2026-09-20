"""Rebuild the preserved lifecycle candidate and record this independent sample."""
import argparse
import datetime
import hashlib
import json
from pathlib import Path
import platform
import re
import shutil
import subprocess
from audit import audit, digest, read, HERE, GAME, AUTHOR

BUILD = GAME / "core/build/qa-parts"
OUTPUT = GAME / "qa/part-lifecycle-evidence.json"


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--run", action="store_true")
    parser.add_argument("--check", action="store_true")
    parser.add_argument("--cmake", default="C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe")
    args = parser.parse_args()
    if args.run == args.check:
        parser.error("Choose --run or --check")
    candidate = read(AUTHOR / "evidence/results.json")
    archive = GAME / candidate["archive"]["root"]
    audit_result = audit()
    own_sources = {"qa/parts/" + name: digest(HERE / name) for name in ["CMakeLists.txt", "probes.cpp", "audit.py", "run.py"]}
    if args.check:
        record = read(OUTPUT)
        assert record["independent_sources"] == own_sources
        assert record["candidate"]["source_graph_sha256"] == audit_result["source_graph_sha256"]
        for artifact, sha in record["artifacts"].items():
            assert digest(GAME / artifact) == sha, artifact
        print("PASS frozen QA evidence/source/artifact linkage; no tests executed")
        return
    BUILD.mkdir(parents=True, exist_ok=True)
    (HERE / "evidence").mkdir(exist_ok=True)
    commands = []

    def run(command, log_name):
        result = subprocess.run([str(x) for x in command], capture_output=True, text=True)
        log = BUILD / log_name
        output = result.stdout + result.stderr
        log.write_text(output, encoding="utf-8", newline="\n")
        commands.append({"command": [str(x).replace(str(GAME), "<game>").replace(GAME.as_posix(), "<game>") for x in command], "exit_code": result.returncode, "raw_log": log.relative_to(GAME).as_posix(), "raw_log_sha256": digest(log)})
        if result.returncode:
            raise RuntimeError(f"{log_name} failed; exit {result.returncode}")
        return output

    run([args.cmake, "-S", HERE, "-B", BUILD, "-G", "Visual Studio 17 2022", "-A", "x64", "-DFROZEN_INPUTS=" + (archive / "inputs").as_posix()], "qa-configure.log")
    run([args.cmake, "--build", BUILD, "--config", "Release", "--parallel", "4"], "qa-build.log")
    exe = BUILD / "Release/independent_part_probes.exe"
    own = run([exe], "qa-probes.log")
    author_exe = BUILD / "Release/rebuilt_author_parts.exe"
    author = run([author_exe], "qa-author.log")
    ctest = run([Path(args.cmake).with_name("ctest.exe"), "--test-dir", BUILD, "-C", "Release", "--output-on-failure"], "qa-ctest.log")
    summary = re.search(r"SUMMARY (\d+) passed, 0 failed, (\d+) assertions", own)
    assert summary and "FAIL " not in own
    assert author == (archive / "parts.log").read_text(encoding="utf-8"), "Rebuilt author output differs from candidate"
    assert audit() == audit_result, "Captured inputs/metadata changed during review execution"
    assert own_sources == {name: digest(GAME / name) for name in own_sources}
    portable_ctest = ctest.replace(str(GAME), "<game>").replace(GAME.as_posix(), "<game>")
    (HERE / "evidence/probes.log").write_text(own, encoding="utf-8", newline="\n")
    (HERE / "evidence/ctest.log").write_text(portable_ctest, encoding="utf-8", newline="\n")
    # Keep newly built binaries and QA source bytes independently of future builds.
    frozen = BUILD / "captures" / ("qa-parts-" + digest(exe)[:16])
    frozen.mkdir(parents=True, exist_ok=True)
    artifacts = {}
    for source in [exe, author_exe, BUILD / "frozen-core/Release/overkill_core.lib"]:
        dest = frozen / source.name
        shutil.copyfile(source, dest)
        artifacts[dest.relative_to(GAME).as_posix()] = digest(dest)
    for source_name in own_sources:
        dest = frozen / "sources" / Path(source_name).name
        dest.parent.mkdir(exist_ok=True)
        shutil.copyfile(GAME / source_name, dest)
        artifacts[dest.relative_to(GAME).as_posix()] = digest(dest)
    for name in ["qa-probes.log", "qa-author.log", "qa-ctest.log", "qa-build.log", "qa-configure.log"]:
        dest = frozen / name
        shutil.copyfile(BUILD / name, dest)
        artifacts[dest.relative_to(GAME).as_posix()] = digest(dest)
    for name in ["probes.log", "ctest.log"]:
        artifacts["qa/parts/evidence/" + name] = digest(HERE / "evidence" / name)
    record = {
        "review": "Independent bounded physical-part lifecycle review",
        "captured_utc": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "environment": {"platform": platform.platform(), "machine": platform.machine(), "compiler": "MSVC 19.44.35228.0 / Windows SDK 10.0.26100.0", "configuration": "Release, C++17, /W4 /WX /permissive-", "input": "Native typed Rules/CampaignRules commands in controlled CPU fixtures; no UI/GPU"},
        "candidate": {key: candidate[key] for key in ["verified_build", "source_graph_sha256", "manifest_sha256", "rules_status", "inputs"]},
        "candidate_artifacts": candidate["archive"],
        "candidate_results_sha256": digest(AUTHOR / "evidence/results.json"),
        "candidate_contracts_sha256": digest(AUTHOR / "contracts.json"),
        "candidate_proposed_bindings_sha256": digest(AUTHOR / "evidence/proposed-bindings.json"),
        "independent_sources": own_sources,
        "independent_build": "qa-parts-" + digest(exe)[:16],
        "independent_executable_sha256": digest(exe),
        "rebuilt_author_executable_sha256": digest(author_exe),
        "rebuilt_library_sha256": digest(BUILD / "frozen-core/Release/overkill_core.lib"),
        "inventory_audit": audit_result,
        "runtime": {"passed_groups": int(summary.group(1)), "assertions": int(summary.group(2)), "failed_groups": 0, "names": [x[5:] for x in own.splitlines() if x.startswith("PASS ")], "author_rebuild": "207 groups / 59169 assertions; complete output identical to frozen original", "ctest": "2/2 passed"},
        "commands": commands,
        "artifacts": artifacts,
        "finding_status": "No reproducible production defect identified in this finite sample.",
        "limits": ["Mechanical coverage of 207 inventories and direct source fields does not establish full semantic coverage of every part/effect/producer.", "Independent runtime: 36 focused groups, including six Spread and 21 Magnet types; other types retain author-operated matrix evidence.", "Generated Ammo domain in author suite uses controlled SH076 inputs; natural availability of every value is not established.", "Provisional corrected of-core-0.4 includes terminal guard; save/version migration decision remains outside this review.", "No bindings activated, no production changed, no full recipe/upgrade gate, balance, visual, native-input, audio, packaging or human acceptance claimed."]
    }
    OUTPUT.write_text(json.dumps(record, indent=2, sort_keys=True) + "\n", encoding="utf-8", newline="\n")
    print(f"PASS independent {summary.group(1)} groups / {summary.group(2)} assertions; author207 / 59169; CTest2/2; {record['independent_build']}")


if __name__ == "__main__":
    main()
