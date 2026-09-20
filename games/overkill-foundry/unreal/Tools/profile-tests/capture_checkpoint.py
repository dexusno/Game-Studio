"""Archive a closed profile/UI validation run; identities do not infer passes."""
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
    parser.add_argument("--core-root", required=True, help="Exact core directory used by the validated build")
    parser.add_argument("--source-snapshot", help="Previously verified source/binary snapshot; never substitute later working files")
    parser.add_argument("--portable-record", default="checkpoint.json", help="Record filename beside this tool")
    parser.add_argument("--log", action="append", required=True)
    args = parser.parse_args()
    root = Path(__file__).resolve().parents[5]
    game = root / "games/overkill-foundry"
    unreal = game / "unreal"
    core = (root / args.core_root).resolve()
    output = root / args.archive
    output.mkdir(parents=True, exist_ok=False)
    inputs = {}
    snapshot = (root / args.source_snapshot).resolve() if args.source_snapshot else None
    frozen = json.loads((snapshot / "identity.json").read_text(encoding="utf-8-sig")) if snapshot else None
    expected = {}
    if frozen:
        for source in frozen["sources"]:
            inputs[source["path"]] = snapshot / "source" / source["path"]
            expected[source["path"]] = source["sha256"]
    else:
        for folder in (unreal / "Source", unreal / "Config", game / "platform/include", game / "platform/src", game / "presentation"):
            for path in folder.rglob("*"):
                if path.suffix in {".cpp", ".h", ".hpp", ".inl", ".inc", ".cs", ".ini"}:
                    inputs[path.relative_to(root).as_posix()] = path
        for folder in (core / "src", core / "include"):
            for path in folder.rglob("*"):
                if path.is_file():
                    inputs[(game / "core" / path.relative_to(core)).relative_to(root).as_posix()] = path
        for path in (unreal / "OverkillFoundry.uproject", game / "tools/unreal.ps1"):
            inputs[path.relative_to(root).as_posix()] = path
        for path in Path(__file__).parent.iterdir():
            if path.suffix in {".cpp", ".py"} or path.name == "CMakeLists.txt":
                inputs[path.relative_to(root).as_posix()] = path
    sources = []
    for name, path in sorted(inputs.items()):
        data = path.read_bytes()
        if name in expected and sha(data) != expected[name]:
            raise RuntimeError("Frozen source changed: " + name)
        destination = output / "source" / name
        destination.parent.mkdir(parents=True, exist_ok=True)
        destination.write_bytes(data)
        if destination.read_bytes() != data or path.read_bytes() != data:
            raise RuntimeError("Source changed during archive: " + name)
        sources.append({"path": name, "bytes": len(data), "sha256": sha(data), "sha256_lf": sha(data.replace(b"\r\n", b"\n"))})
    graph = sha("".join(s["path"] + " " + s["sha256_lf"] + "\n" for s in sources).encode())
    if frozen and graph != frozen["source_graph_lf_sha256"]:
        raise RuntimeError("Frozen source graph differs")
    artifacts = []

    def archive(path, name):
        destination = output / name
        destination.parent.mkdir(parents=True, exist_ok=True)
        shutil.copy2(path, destination)
        data = destination.read_bytes()
        if sha(data) != sha(path.read_bytes()):
            raise RuntimeError("Archive copy differs: " + str(path))
        artifacts.append({"source": path.relative_to(root).as_posix(), "archive": name, "bytes": len(data), "sha256": sha(data)})

    logs = ""
    for name in args.log:
        path = unreal / "Saved/BuildLogs" / name
        archive(path, "logs/" + path.name)
        companion = Path(str(path) + ".binary.json")
        if companion.exists():
            archive(companion, "logs/" + companion.name)
        logs += path.read_text(encoding="utf-8-sig", errors="replace") + "\n"
    native = root / args.native_dir
    for path in sorted(native.iterdir()):
        if path.is_file():
            archive(path, "native/" + path.name)
    for name in ("UnrealEditor-OverkillFoundry.dll", "OverkillFoundry.exe"):
        path = snapshot / "binaries" / name if snapshot else unreal / "Binaries/Win64" / name
        if frozen and sha(path.read_bytes()) != next(b["sha256"] for b in frozen["binaries"] if b["name"] == name):
            raise RuntimeError("Frozen binary changed: " + name)
        archive(path, "binaries/" + name)
    for mode in ("ProfileTests", "ProfileTests20"):
        archive(unreal / "Saved" / mode / "Testing/Temporary/LastTest.log", "tests/" + mode + ".log")
    for name in sorted(set(re.findall(r'Tracing Screenshot "(campaign-[^"]+)" taken', logs))):
        archive(unreal / "Saved/Screenshots" / (name + ".png"), "captures/" + name + ".png")
    content = []
    if not snapshot:
        for path in sorted((unreal / "Content").rglob("*.umap")):
            content.append({"path": path.relative_to(root).as_posix(), "bytes": path.stat().st_size, "sha256": sha(path.read_bytes())})

    def observed(pattern):
        match = re.search(pattern, logs)
        return match.group(1) if match else None

    result = {"schema": 1, "checkpoint": "Independent profiles, retained Collection and AI Game Over",
              "recorded_at": datetime.datetime.now().astimezone().isoformat(),
              "engine": observed(r"LogInit: Engine Version: ([^\r\n]+)"),
              "compiler": observed(r"Using Visual Studio ([^ ]+) toolchain"),
              "sdk": observed(r"and Windows ([^ ]+) SDK"),
              "core_digest": observed(r"FOUNDRY_CORE_BUILD digest=([a-f0-9]+)"),
              "core_source": core.relative_to(root).as_posix(),
              "source_snapshot": snapshot.relative_to(root).as_posix() if snapshot else None,
              "source_snapshot_identity_sha256": sha((snapshot / "identity.json").read_bytes()) if snapshot else None,
              "collector_sha256": sha(Path(__file__).read_bytes()),
              "source_graph_lf_sha256": graph,
              "graph_algorithm": "SHA256 of sorted path + space + LF-normalized SHA256 + newline records",
              "sources": sources, "artifacts": artifacts, "scene_identities": content,
              "scene_identity_note": "The earlier native scene was not separately archived before the next asset import; later working maps are intentionally excluded." if snapshot else "Captured current map identities at this boundary.",
              "scope": "Identity record only; see README and native trace for actual observations. Prepared fixtures are not earned progression or visual approval."}
    data = (json.dumps(result, indent=2) + "\n").encode()
    (output / "identity.json").write_bytes(data)
    Path(__file__).with_name(args.portable_record).write_bytes(data)
    print(json.dumps({"archive": output.relative_to(root).as_posix(), "sources": len(sources), "artifacts": len(artifacts), "graph": graph, "identity_sha256": sha(data)}))


if __name__ == "__main__":
    main()
