"""Run the integrated native mechanics suites on a copied source graph."""
import argparse
import datetime
import hashlib
import json
from pathlib import Path
import re
import subprocess
import sys
import xml.etree.ElementTree as ET

HERE = Path(__file__).resolve().parent
CORE = HERE.parents[1]
GAME = CORE.parent


def digest(data):
    return hashlib.sha256(data).hexdigest()


def extract_results(archive, record):
    # JUnit's successful stdout is truncated by CTest's default size limit.
    # LastTest.log retains the actual full output from this same execution.
    log_path = archive / "build/Testing/Temporary/LastTest.log"
    raw = log_path.read_bytes()
    sections = re.split(r"(?m)^\d+/\d+ Testing: (.+)\n", raw.decode("utf-8").replace("\r\n", "\n"))
    outputs = {}
    for name, section in zip(sections[1::2], sections[2::2]):
        match = re.search(r"\nOutput:\n-+\n(.*?)\n<end of output>", section, re.S)
        if not match or name in outputs:
            raise RuntimeError("Missing or duplicate full test output: " + name)
        outputs[name] = match[1]
    cases = ET.parse(archive / "ctest.xml").getroot().findall("testcase")
    if len(cases) != 16 or any(case.find("failure") is not None or case.get("status") != "run" for case in cases):
        raise RuntimeError("Expected exactly 16 passing native suites")
    if set(outputs) != {case.get("name") for case in cases}:
        raise RuntimeError("Full stdout and JUnit suite identities differ")
    record["suites"] = [{"name": case.get("name"), "result": "passed", "output": outputs[case.get("name")]}
                       for case in cases]
    record["full_output"] = {"path": log_path.relative_to(archive).as_posix(), "bytes": len(raw), "sha256": digest(raw)}
    record["results_extraction_tool_sha256"] = digest(Path(__file__).read_bytes())
    return cases


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cmake", required=True)
    args = parser.parse_args()
    cmake = Path(args.cmake).resolve(strict=True)
    ctest = cmake.with_name("ctest.exe" if sys.platform == "win32" else "ctest")
    stamp = datetime.datetime.now().strftime("%Y%m%d-%H%M%S")
    archive = CORE / "build" / ("mechanics-integration-" + stamp)
    archive.mkdir(parents=True, exist_ok=False)
    files = [CORE / "CMakeLists.txt", GAME / "presentation/precision.hpp"]
    for folder in ("include", "src", "runner", "tests"):
        files.extend(p for p in (CORE / folder).rglob("*") if p.is_file() and
                     (p.suffix in {".cpp", ".hpp", ".h", ".inl", ".inc"} or p.name == "CMakeLists.txt"))
    rows = []
    for path in sorted(set(files)):
        data = path.read_bytes()
        relative = path.relative_to(GAME).as_posix()
        destination = archive / "source" / relative
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_bytes(data)
        if data != path.read_bytes():
            raise RuntimeError("Source changed during copy: " + relative)
        rows.append({"path": relative, "bytes": len(data), "sha256": digest(data),
                     "sha256_lf": digest(data.replace(b"\r\n", b"\n"))})
    graph = digest("".join(row["path"] + " " + row["sha256_lf"] + "\n" for row in rows).encode())
    record = {
        "recorded_at": datetime.datetime.now(datetime.timezone.utc).isoformat(),
        "scope": "Version-held mechanics candidate; one authoritative library, 16 native suites; no save compatibility, support activation or release approval.",
        "graph_algorithm": "SHA256 of sorted path + space + LF-normalized SHA256 + newline",
        "source_graph_lf_sha256": graph, "inputs": rows,
        "capture_tool_sha256": digest(Path(__file__).read_bytes()),
        "archive": archive.relative_to(GAME).as_posix(), "commands": [], "status": "copied",
    }
    (archive / "capture.py").write_bytes(Path(__file__).read_bytes())
    (CORE / "build/current-mechanics-integration.txt").write_text(str(archive), encoding="utf-8")
    identity = archive / "identity.json"

    def save_record():
        identity.write_text(json.dumps(record, indent=2) + "\n", encoding="utf-8")

    def run(command, log_name):
        log = archive / log_name
        with log.open("wb") as output:
            result = subprocess.run([str(arg) for arg in command], stdout=output, stderr=subprocess.STDOUT, check=False)
        record["commands"].append({"command": [str(arg) for arg in command], "exit_code": result.returncode,
                                   "log": log_name, "sha256": digest(log.read_bytes())})
        save_record()
        print(log_name + ": " + ("PASS" if result.returncode == 0 else "FAIL"), flush=True)
        if result.returncode:
            raise RuntimeError("Command failed; inspect " + str(log))

    save_record()
    build = archive / "build"
    try:
        run([cmake, "-S", archive / "source/core/tests/mechanics", "-B", build,
             "-G", "Visual Studio 17 2022", "-A", "x64"], "configure.log")
        run([cmake, "--build", build, "--config", "Release", "--parallel", "4"], "build.log")
        run([ctest, "--test-dir", build, "-C", "Release", "--output-on-failure",
             "--output-junit", archive / "ctest.xml"], "ctest.log")
        cases = extract_results(archive, record)
        record["binaries"] = [{"path": path.relative_to(archive).as_posix(), "bytes": path.stat().st_size,
                                "sha256": digest(path.read_bytes())}
                               for path in sorted(build.rglob("*")) if path.is_file() and path.suffix in {".exe", ".lib"}]
        for row in rows:
            if digest((GAME / row["path"]).read_bytes()) != row["sha256"] or digest((archive / "source" / row["path"]).read_bytes()) != row["sha256"]:
                raise RuntimeError("Source changed before verification: " + row["path"])
        record["status"] = "passed"
        save_record()
        print(json.dumps({"archive": record["archive"], "inputs": len(rows), "graph": graph, "suites": len(cases), "status": "passed"}), flush=True)
    except Exception as error:
        record["status"] = "failed"
        record["error"] = str(error)
        save_record()
        print(str(error), file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
