"""Archive a completed local recipe-control check without publishing raw assets.

This records identities; it never infers a pass from a log or approves a visual.
Run only after closing Unreal and freezing the reviewed source graph.
"""
import argparse
import datetime
import hashlib
import json
from pathlib import Path
import re
import shutil


def sha(data):
    return hashlib.sha256(data).hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--archive", required=True)
    parser.add_argument("--native-dir", required=True)
    parser.add_argument("--log", action="append", required=True)
    parser.add_argument("--core-root", help="Exact explicit core snapshot used for this build; default is the live shared core")
    parser.add_argument("--baseline-source", help="Earlier archived game source directory for a precise final-build delta")
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[5]
    game = root / "games/overkill-foundry"
    unreal = game / "unreal"
    core_root = (root / args.core_root).resolve() if args.core_root else game / "core"
    if not (core_root / "include/overkill/core.hpp").is_file():
        raise SystemExit("Core source identity is missing")
    baseline = (root / args.baseline_source).resolve() if args.baseline_source else None
    output = root / args.archive
    output.mkdir(parents=True, exist_ok=False)
    inputs = set()
    for folder in (unreal / "Source", unreal / "Config", game / "core/include", game / "core/src", game / "platform/include", game / "platform/src", game / "presentation"):
        inputs.update(p for p in folder.rglob("*") if p.suffix in {".cpp", ".h", ".hpp", ".inl", ".inc", ".cs", ".ini"})
    inputs.add(unreal / "OverkillFoundry.uproject")
    inputs.add(game / "tools/unreal.ps1")
    for directory in (unreal / "Tools/recipe-control-tests", unreal / "Tools/ui-fixtures"):
        inputs.update(p for p in directory.iterdir() if p.suffix in {".cpp", ".py"} or p.name == "CMakeLists.txt")
    sources = []
    delta = []
    for source in sorted(inputs, key=lambda p: p.relative_to(root).as_posix()):
        actual = core_root / source.relative_to(game / "core") if source.is_relative_to(game / "core") else source
        data = actual.read_bytes()
        relative = source.relative_to(root).as_posix()
        sources.append({"path": relative, "bytes": len(data), "sha256": sha(data), "sha256_lf": sha(data.replace(b"\r\n", b"\n"))})
        if baseline:
            old = baseline / source.relative_to(game)
            old_hash = sha(old.read_bytes().replace(b"\r\n", b"\n")) if old.is_file() else None
            if old_hash != sources[-1]["sha256_lf"]:
                delta.append({"path": relative, "before_sha256_lf": old_hash, "after_sha256_lf": sources[-1]["sha256_lf"]})
        destination = output / "source" / relative
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_bytes(data)
    graph = sha("".join(p["path"] + " " + p["sha256_lf"] + "\n" for p in sources).encode())
    artifacts = []

    def archive(source, category):
        destination = output / category / source.name
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(source, destination)
        artifacts.append({"source": source.relative_to(root).as_posix(), "archive": destination.relative_to(output).as_posix(), "bytes": destination.stat().st_size, "sha256": sha(destination.read_bytes())})

    capture_names = set()
    log_text = ""
    for name in args.log:
        path = unreal / "Saved/BuildLogs" / name
        archive(path, "logs")
        companion = Path(str(path) + ".binary.json")
        if companion.exists():
            archive(companion, "logs")
        current_log = path.read_text(encoding="utf-8-sig", errors="replace")
        log_text += current_log + "\n"
        capture_names.update(re.findall(r'Tracing Screenshot "(campaign-[^"]+)" taken', current_log))
    for name in sorted(capture_names):
        archive(unreal / "Saved/Screenshots" / (name + ".png"), "captures")
    native = root / args.native_dir
    for path in sorted(native.iterdir()):
        if path.suffix.lower() in {".jpg", ".png", ".json"}:
            archive(path, "native")
    for name in ("UnrealEditor-OverkillFoundry.dll", "OverkillFoundry.exe"):
        archive(unreal / "Binaries/Win64" / name, "binaries")
    archive(unreal / "Saved/Parity/core-fixture.jsonl", "parity")
    archive(unreal / "Saved/Parity/recipe-controls-native-fixture.jsonl", "parity")
    archive(root / ".local/overkill-foundry/recipe-control-tests/Testing/Temporary/LastTest.log", "tests")
    def observed(pattern):
        match = re.search(pattern, log_text)
        return match.group(1) if match else None
    result = {"schema": 1, "checkpoint": "Advanced recipe action controls", "recorded_at": datetime.datetime.now().astimezone().isoformat(), "engine": observed(r"LogInit: Engine Version: ([^\r\n]+)"), "compiler": observed(r"Using Visual Studio ([^ ]+) toolchain"), "sdk": observed(r"and Windows ([^ ]+) SDK"), "core_digest": observed(r"FOUNDRY_CORE_BUILD digest=([a-f0-9]+)"), "source_graph_lf_sha256": graph, "graph_algorithm": "SHA256 of sorted path + space + LF-normalized SHA256 + newline records", "sources": sources, "artifacts": artifacts, "scope": "Identity only. See README for tested behavior and attribution. Prepared saves are not earned progression, and source tests are not native input or visual acceptance."}
    result["core_source"] = {"explicit_snapshot": bool(args.core_root), "path": core_root.relative_to(root).as_posix()}
    if baseline:
        result["baseline_source"] = baseline.relative_to(root).as_posix()
        result["source_delta_from_baseline"] = delta
    text = json.dumps(result, indent=2) + "\n"
    (output / "identity.json").write_bytes(text.encode("utf-8"))
    portable = Path(__file__).with_name("checkpoint.json")
    portable.write_bytes(text.encode("utf-8"))
    print(json.dumps({"archive": output.relative_to(root).as_posix(), "portable": portable.relative_to(root).as_posix(), "sources": len(sources), "artifacts": len(artifacts), "graph_sha256": graph, "identity_sha256": sha(text.encode())}))


if __name__ == "__main__":
    main()
