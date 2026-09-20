"""Independently verify and rebuild the two unchanged author archives.

Only this QA directory receives output. The author archives and portable
evidence remain read-only. Run --build to execute the two strict native suites.
"""
import argparse
import hashlib
import json
import re
import subprocess
from datetime import datetime, timezone
from pathlib import Path

HERE = Path(__file__).resolve().parent
GAME = HERE.parents[1]
CMAKE = Path("C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe")
CTEST = CMAKE.with_name("ctest.exe")
SPECS = {
    "encounters": ("encounters", "encounter_contract_tests", "ENCOUNTER_CONTRACTS_OK"),
    "campaign": ("campaign-lifecycle", "campaign_lifecycle_contract_tests", "CAMPAIGN_LIFECYCLE_OK"),
}


def sha(data):
    return hashlib.sha256(data).hexdigest()


def file_info(path):
    data = path.read_bytes()
    return {"path": path.relative_to(GAME).as_posix(), "bytes": len(data), "sha256": sha(data)}


def audit(name):
    directory, _, _ = SPECS[name]
    path = GAME / "core/tests" / directory / "evidence.json"
    evidence = json.loads(path.read_text(encoding="utf-8-sig"))
    source = GAME / evidence["archive"]
    records = []
    checks = 0
    for row in evidence["inputs"]:
        data = (source / row["path"]).read_bytes()
        assert len(data) == row["bytes"], row["path"]
        assert sha(data) == row["sha256"], row["path"]
        lf = sha(data.replace(b"\r\n", b"\n"))
        assert lf == row["sha256_lf"], row["path"]
        checks += 3
        records.append((row["path"], lf))
    separator = "\t" if name == "encounters" else "\0"
    graph = sha("".join(f"{p}{separator}{h}\n" for p, h in records).encode())
    assert graph == evidence["graph_lf_sha256"], f"{name} graph mismatch"
    checks += 1
    for row in evidence["artifacts"]:
        data = (GAME / row["path"]).read_bytes()
        assert len(data) == row["bytes"], row["path"]
        assert sha(data) == row["sha256"], row["path"]
        checks += 2
    assert len(evidence["result"]["named_groups"]) == evidence["result"]["groups"]
    names = [name.removeprefix("PASS ") for name in evidence["result"]["named_groups"]]
    assert all(group in names
               for groups in evidence["candidate_links_pending_review"].values()
               for group in ([groups] if isinstance(groups, str) else groups))
    checks += 2
    return evidence, {"evidence": file_info(path), "declared_graph_lf_sha256": evidence["graph_lf_sha256"],
                      "independent_graph_lf_sha256": graph,
                      "aggregate_formula": f"recorded input order; path + {repr(separator)} + LF hash + newline; UTF-8 SHA256",
                      "aggregate_formula_confirmed": True,
                      "input_count": len(records), "artifact_count": len(evidence["artifacts"]),
                      "identity_checks": checks}


def run(command, log):
    result = subprocess.run([str(x) for x in command], stdout=subprocess.PIPE, stderr=subprocess.STDOUT)
    log.parent.mkdir(parents=True, exist_ok=True)
    log.write_bytes(result.stdout)
    if result.returncode:
        raise RuntimeError(f"Command failed ({result.returncode}): {command}\n{result.stdout.decode(errors='replace')}")
    portable = [str(x).replace(str(GAME), "${GAME}") for x in command]
    return {"command": portable, "exit_code": result.returncode, "log": file_info(log)}


def build(name, evidence):
    directory, target, marker = SPECS[name]
    source = GAME / evidence["archive"] / "core/tests" / directory
    build_dir = HERE / "build" / name
    commands = [run([CMAKE, "-S", source, "-B", build_dir, "-G", "Visual Studio 17 2022", "-A", "x64"], build_dir / "configure.log")]
    commands.append(run([CMAKE, "--build", build_dir, "--config", "Release", "--parallel", "4"], build_dir / "build.log"))
    commands.append(run([CTEST, "--test-dir", build_dir, "-C", "Release", "--output-on-failure"], build_dir / "ctest.log"))
    exe = build_dir / "Release" / (target + ".exe")
    result_path = HERE / f"{name}-suite-results.txt"
    commands.append(run([exe], result_path))
    output = result_path.read_text(encoding="utf-8-sig")
    groups = re.findall(r"^PASS (.*)$", output, re.M)
    assert groups == [name.removeprefix("PASS ") for name in evidence["result"]["named_groups"]]
    match = re.search(marker + r" groups=(\d+) assertions=(\d+)", output)
    assert match and int(match[1]) == evidence["result"]["groups"]
    assert int(match[2]) == evidence["result"]["assertions"]
    return {"commands": commands, "groups": groups, "assertions": int(match[2]),
            "executable": file_info(exe), "library": file_info(build_dir / "core-runtime/Release/overkill_core.lib")}


def probes(name, evidence):
    source = GAME / evidence["archive"]
    build_dir = HERE / "build" / (name + "-probes")
    commands = [run([CMAKE, "-S", HERE, "-B", build_dir, "-G", "Visual Studio 17 2022", "-A", "x64",
                     "-DFROZEN_GAME=" + str(source), "-DQA_CAMPAIGN=" + ("ON" if name == "campaign" else "OFF")], build_dir / "configure.log")]
    commands.append(run([CMAKE, "--build", build_dir, "--config", "Release", "--parallel", "4"], build_dir / "build.log"))
    exe = build_dir / "Release/independent_probes.exe"
    commands.append(run([exe], HERE / (name + "-probe-results.txt")))
    commands.append(run([CTEST, "--test-dir", build_dir, "-C", "Release", "--output-on-failure"], build_dir / "ctest.log"))
    return {"commands": commands, "executable": file_info(exe),
            "library": file_info(build_dir / "core-runtime/Release/overkill_core.lib")}


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--build", choices=["encounters", "campaign", "all"])
    parser.add_argument("--probes", choices=["encounters", "campaign", "all"])
    parser.add_argument("--check-review", action="store_true", help="Verify the closed review without rewriting its evidence")
    args = parser.parse_args()
    records = {}
    for name in SPECS:
        evidence, result = audit(name)
        if args.build in (name, "all"):
            result["native"] = build(name, evidence)
            _, after = audit(name)
            assert {k: v for k, v in result.items() if k != "native"} == after
            result["archive_unchanged_after_execution"] = True
        if args.probes in (name, "all"):
            result["independent_probes"] = probes(name, evidence)
            _, after = audit(name)
            assert {k: v for k, v in result.items() if k not in ("native", "independent_probes", "archive_unchanged_after_execution")} == after
            result["archive_unchanged_after_execution"] = True
        records[name] = result
    output = {"date_utc": datetime.now(timezone.utc).isoformat(), "archives": records}
    if args.check_review:
        review = json.loads((HERE.parent / "encounter-lifecycle-evidence.json").read_text(encoding="utf-8"))
        files = review["review_files"] + review["execution_artifacts"] + review["supporting_copies"]
        for row in files:
            assert file_info(GAME / row["path"]) == row, row["path"]
        print(f"PASS {sum(r['identity_checks'] for r in records.values())} author identity/link checks and {len(files)} closed-review file identities")
        return
    label = f"probes-{args.probes}" if args.probes else "audit" if not args.build else f"rebuild-{args.build}"
    path = HERE / (label + ".json") if args.build or args.probes else HERE / "build/last-audit.json"
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(json.dumps(output, indent=2) + "\n", encoding="utf-8")
    print(f"PASS {sum(r['identity_checks'] for r in records.values())} identity/link checks; {path.relative_to(GAME)}")


if __name__ == "__main__":
    main()
