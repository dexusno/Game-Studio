"""Publish compact identities only after all source FBX checks match."""
import argparse
import csv
import hashlib
import json
from pathlib import Path
import shutil

p = argparse.ArgumentParser()
p.add_argument("--output", required=True)
source = Path(p.parse_args().output).resolve()
game = Path(__file__).resolve().parents[1]
target = game / "assets/production/roster-v001"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


proof = json.loads((source / "interchange-verification.json").read_text(encoding="utf-8"))
report = json.loads((source / "asset-report.json").read_text(encoding="utf-8"))
if proof["failures"] or proof["source_report_sha256"] != sha(source / "asset-report.json"):
    raise RuntimeError("Failed or stale roster proof")
if report["source_sha256"] != sha(Path(__file__).with_name("roster_v001_build.py")):
    raise RuntimeError("Generator changed after export")
for relative, digest in proof["files"].items():
    if sha(source / relative) != digest:
        raise RuntimeError("Export changed after verification: " + relative)
target.mkdir(parents=True, exist_ok=True)
for name in ("asset-report.json", "interchange-verification.json"):
    shutil.copyfile(source / name, target / name)
index = {"version": report["version"], "large_artifacts": "ignored .local/overkill-foundry/art/roster-v001",
         "source_sha256": report["source_sha256"], "helper_sha256": report["helper_sha256"],
         "files": [{"path": f.relative_to(source).as_posix(), "bytes": f.stat().st_size, "sha256": sha(f),
                    "purpose": "source review only" if "renders" in f.parts else "original reproducible source product"}
                   for f in sorted(source.rglob("*")) if f.is_file() and f.suffix.lower() in {".fbx", ".blend", ".png"}],
         "rights": "Original Game Studio / Codex geometry, rigging and keyframes; original Cinderwall surfaces reused. No external art. Studio distribution licence unset.",
         "limits": "No Unreal, performance or human visual acceptance established by source export checks."}
(target / "source-artifact-index.json").write_text(json.dumps(index, indent=2), encoding="utf-8")
with Path(__file__).with_name("roster_v001_manifest-rows.csv").open("w", encoding="utf-8", newline="") as file:
    csv.writer(file, lineterminator="\n").writerow([
        "cinderwall_roster_v001_source", "art-source/roster_v001_build.py",
        "original: Blender 5.2.1 LTS procedural source; no external art", "Game Studio / Codex",
        "Original project content; studio distribution licence unset", "assets/production/roster-v001/source-artifact-index.json",
        "Eight distinct articulated robots and 72 clips; unchanged original Cinderwall PBR surfaces",
        "source-and-FBX-verified; Unreal and human visual acceptance pending"])
print("ROSTER_COMPACT_PUBLISHED " + str(target))
