"""Freeze and execute the bounded CPU city replay. Never starts Unreal."""
from __future__ import annotations
import argparse
import datetime as dt
import hashlib
import json
import pathlib
import shutil
import subprocess
import sys

HERE = pathlib.Path(__file__).resolve().parent
GAME = HERE.parents[2]
WORKSPACE = GAME.parents[1]
CORE_DIGEST = "b915b36ef05991e865abed40e9ebcd6fe82db70e8bf20556ad47633a5e32f2cf"
TRACE_SHA256 = "36c77c625df0e2b40fba6391c012552a1a92669db90a304cdfc4b2227eb46fdd"


def sha(path: pathlib.Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def digest(entries: dict[str, str]) -> str:
    return hashlib.sha256("".join(f"{key}\n{value}\n" for key, value in sorted(entries.items())).encode()).hexdigest()


def run(command: list[str], cwd: pathlib.Path, log: pathlib.Path) -> None:
    # Synchronous CPU processes only; no background window or engine process.
    with log.open("wb") as stream:
        result = subprocess.run(command, cwd=cwd, stdout=stream, stderr=subprocess.STDOUT, check=False)
    print(f"exit={result.returncode} {log.name}", flush=True)
    if result.returncode:
        raise RuntimeError(f"Command failed; inspect {log}")


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--cmake", type=pathlib.Path, required=True)
    parser.add_argument("--core-root", type=pathlib.Path, required=True)
    parser.add_argument("--trace", type=pathlib.Path, required=True)
    parser.add_argument("--output", type=pathlib.Path, required=True)
    args = parser.parse_args()
    core, trace, output = args.core_root.resolve(), args.trace.resolve(), args.output.resolve()
    if output.exists():
        raise RuntimeError("Use a new evidence directory; prior results are never overwritten")
    if not output.is_relative_to(GAME / "unreal/Saved"):
        raise RuntimeError("Keep generated output beneath this game's ignored unreal/Saved")
    if sha(trace) != TRACE_SHA256:
        raise RuntimeError("This preparation is pinned to the reviewed defensive/Auto seed148 witness")
    core_files = sorted([*core.joinpath("src").rglob("*"), *core.joinpath("include").rglob("*")])
    core_files = [path for path in core_files if path.is_file()]
    core_identity = hashlib.sha256()
    for path in sorted(core_files, key=lambda item: item.relative_to(core).as_posix()):
        core_identity.update((path.relative_to(core).as_posix() + "\n").encode())
        core_identity.update(path.read_bytes())
    if core_identity.hexdigest() != CORE_DIGEST:
        raise RuntimeError("Core source differs from the explicitly reviewed 1843 snapshot")
    historical = json.loads((GAME / "qa/city-runner/evidence/baseline5-source-identity.json").read_text())
    sources: dict[str, pathlib.Path] = {}
    for path in core_files:
        sources["core/" + path.relative_to(core).as_posix()] = path
    for name in ("city_runner.hpp", "trace.cpp", "policy.cpp"):
        relative = "core/runner/" + name
        path = GAME / relative
        if sha(path) != historical["sources"][relative]:
            raise RuntimeError(f"Historical runner source changed: {relative}")
        sources[relative] = path
    for folder in (GAME / "platform/include", GAME / "platform/src"):
        for path in folder.rglob("*"):
            if path.is_file() and path.suffix in (".cpp", ".hpp"):
                sources[path.relative_to(GAME).as_posix()] = path
    for name in ("CMakeLists.txt", "replay.hpp", "replay.cpp", "main.cpp", "prepare.py"):
        sources["unreal/Tools/city-probe/" + name] = HERE / name
    before = {name: sha(path) for name, path in sources.items()}
    output.mkdir(parents=True)
    for name, path in sources.items():
        destination = output / "source" / name
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copyfile(path, destination)
        if sha(destination) != before[name] or sha(path) != before[name]:
            raise RuntimeError(f"Source changed during copy: {name}")
    shutil.copyfile(trace, output / "input.oftrace")
    source = output / "source/unreal/Tools/city-probe"
    commands = []
    summaries = {}
    for standard in (17, 20):
        build = output / f"build{standard}"
        configure = [str(args.cmake.resolve()), "-S", str(source), "-B", str(build), "-A", "x64",
                     f"-DFOUNDRY_CITY_CORE_ROOT:PATH={output / 'source/core'}", f"-DFOUNDRY_CITY_CXX_STANDARD={standard}"]
        compile_ = [str(args.cmake.resolve()), "--build", str(build), "--config", "Release", "--parallel", "4"]
        execute = [str(build / "Release/foundry_city_prepare.exe"), str(output / "input.oftrace"), str(output / f"cpu{standard}")]
        for phase, command in (("configure", configure), ("build", compile_), ("replay", execute)):
            commands.append(command)
            run(command, output, output / f"{phase}{standard}.log")
        summaries[str(standard)] = json.loads((output / f"cpu{standard}/summary.json").read_text())
    if (output / "cpu17/transactions.jsonl").read_bytes() != (output / "cpu20/transactions.jsonl").read_bytes():
        raise RuntimeError("C++17 and C++20 exact transaction transcripts differ")
    for name, path in sources.items():
        if sha(path) != before[name] or sha(output / "source" / name) != before[name]:
            raise RuntimeError(f"Source changed during CPU execution: {name}")
    artifacts = {path.relative_to(output).as_posix(): sha(path) for path in output.rglob("*")
                 if path.is_file() and not path.is_relative_to(output / "source")
                 and (not path.is_relative_to(output / "build17") and not path.is_relative_to(output / "build20")
                      or path.name == "foundry_city_prepare.exe")}
    identity = {
        "utc": dt.datetime.now(dt.timezone.utc).isoformat(),
        "scope": "CPU production CampaignSession replay and independent C++17/C++20 compiler parity; UE runtime deferred",
        "trace_sha256": TRACE_SHA256, "core_digest": CORE_DIGEST, "sources": before,
        "source_graph_sha256": digest(before), "source_before_after_equal": True,
        "commands": commands, "artifacts": artifacts, "results": summaries,
    }
    (output / "identity.json").write_text(json.dumps(identity, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"ok": True, "source_graph": identity["source_graph_sha256"], "commands_per_standard": summaries["17"]["commands"],
                      "checks_per_standard": summaries["17"]["checks"], "final_hash": summaries["17"]["final_hash"]}), flush=True)


if __name__ == "__main__":
    try:
        main()
    except Exception as error:
        print(f"CITY_PREPARATION_FAILED {error}", file=sys.stderr)
        raise SystemExit(1)
