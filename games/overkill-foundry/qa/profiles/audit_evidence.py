"""Independent read-only audit of the preserved profile checkpoints.

Run from any directory. Optional output is a compact JSON result, never an
overwrite of an author capture. The native artifacts remain ignored locally.
"""
import argparse
import hashlib
import json
from pathlib import Path


ROOT = Path(__file__).resolve().parents[4]
GAME = ROOT / "games/overkill-foundry"
NATIVE = GAME / "unreal/Saved/Validation/profiles-native-archive-20260920-2034"
checks = 0


def sha(data):
    return hashlib.sha256(data).hexdigest()


def require(condition, message):
    global checks
    checks += 1
    if not condition:
        raise AssertionError(message)


def read_json(path):
    return json.loads(path.read_text(encoding="utf-8-sig"))


def artifact(path, row):
    data = path.read_bytes()
    require(len(data) == row["bytes"], f"Size mismatch: {path.relative_to(ROOT)}")
    require(sha(data) == row["sha256"], f"Hash mismatch: {path.relative_to(ROOT)}")


def graph(folder, identity):
    records = []
    for row in identity["sources"]:
        path = folder / "source" / row["path"]
        artifact(path, row)
        lf = sha(path.read_bytes().replace(b"\r\n", b"\n"))
        require(lf == row["sha256_lf"], f"LF hash mismatch: {row['path']}")
        records.append((row["path"], lf))
    digest = sha("".join(f"{p} {h}\n" for p, h in sorted(records)).encode())
    require(digest == identity["source_graph_lf_sha256"], "Source graph mismatch")
    return len(records), digest


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    native_path = GAME / "unreal/Tools/profile-tests/native-checkpoint.json"
    recheck_path = GAME / "unreal/Tools/profile-tests/recheck-checkpoint.json"
    native, recheck = read_json(native_path), read_json(recheck_path)
    require(native == read_json(NATIVE / "identity.json"), "Portable/native identity differs")
    primary_count, primary_graph = graph(NATIVE, native)
    frozen = ROOT / native["source_snapshot"]
    require(sha((frozen / "identity.json").read_bytes()) == native["source_snapshot_identity_sha256"], "Original capture identity mismatch")
    original = read_json(frozen / "identity.json")
    graph(frozen, original)
    require(original["sources"] == native["sources"], "Sources differ between compile and native archive")
    for row in original["binaries"]:
        artifact(frozen / "binaries" / row["name"], row)
    for row in native["artifacts"]:
        artifact(NATIVE / row["archive"], row)

    parent = ROOT / recheck["parent_compile_archive"]
    require(sha((parent / "identity.json").read_bytes()) == recheck["parent_identity_sha256"], "Recheck parent identity mismatch")
    parent_identity = read_json(parent / "identity.json")
    parent_count, parent_graph = graph(parent, parent_identity)
    require(parent_graph == recheck["source_graph_lf_sha256"], "Recheck source graph differs")
    for row in parent_identity["artifacts"]:
        artifact(parent / row["archive"], row)
    for row in recheck["artifacts"]:
        artifact(ROOT / row["path"], row)
    for row in recheck["binaries"]:
        require(sha((parent / "binaries" / row["name"]).read_bytes()) == row["sha256"], "Recheck binary mismatch")
    ui = parent / "source/games/overkill-foundry/unreal/Source/OverkillFoundry/Private/FoundryCampaignUI.cpp"
    require(sha(ui.read_bytes()) == recheck["ui_source_sha256"], "Recheck UI source mismatch")

    # The first three native images belong to the earlier failed creation build.
    native_images = sorted(row["archive"] for row in native["artifacts"] if row["archive"].startswith("native/") and row["archive"].endswith(".jpg"))
    require(len(native_images) == 24, "Native frame inventory differs")
    for suffix in ["04-alpha-created-no-run.jpg", "17-filter-scroll-position-defect.jpg", "21-actual-game-over-hostile-ai.jpg", "24-new-run-retains-sixteen-discoveries.jpg"]:
        require("native/" + suffix in native_images, f"Missing checkpoint frame {suffix}")
    supplemental = GAME / "core/build/qa-profiles/inputs/defeat-ready-prepared.ofsave"
    require(sha(supplemental.read_bytes()) == "565ee873ec03903cd8175eb68fc71cf96fbbce033b0dcd8ca7262027469a1b51", "Supplemental pre-EndTurn fixture differs from recorded original")
    result = {
        "result": "pass", "checks": checks,
        "native_checkpoint_sha256": sha(native_path.read_bytes()),
        "recheck_checkpoint_sha256": sha(recheck_path.read_bytes()),
        "primary_sources": primary_count, "primary_graph_lf_sha256": primary_graph,
        "primary_artifacts": len(native["artifacts"]),
        "primary_binaries": original["binaries"],
        "recheck_parent_sources": parent_count, "recheck_graph_lf_sha256": parent_graph,
        "recheck_parent_artifacts": len(parent_identity["artifacts"]),
        "recheck_artifacts": len(recheck["artifacts"]), "recheck_binaries": recheck["binaries"],
        "supplemental_pre_endturn_sha256": sha(supplemental.read_bytes()),
        "limits": ["Byte identity is not behavioral coverage.", "Earlier map was not archived; no substituted map identity."]
    }
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
