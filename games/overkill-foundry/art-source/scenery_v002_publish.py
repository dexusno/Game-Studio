"""Publish verified compact scenery metadata and only the five new PBR sets."""
import argparse
import csv
import hashlib
import json
from pathlib import Path
import shutil

parser = argparse.ArgumentParser()
parser.add_argument("--output", required=True)
source = Path(parser.parse_args().output).resolve()
game = Path(__file__).resolve().parents[1]
target = game / "assets/production/scenery-v002"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


verification = json.loads((source / "interchange-verification.json").read_text(encoding="utf-8"))
if verification["failures"] or verification["fbx_sha256"] != sha(source / "meshes/CinderwallSceneryV002.fbx") or verification["source_report_sha256"] != sha(source / "asset-report.json"):
    raise RuntimeError("Scenery verification failed or no longer matches exported data")
target.mkdir(parents=True, exist_ok=True)
for name in ("asset-report.json", "interchange-verification.json"):
    shutil.copyfile(source / name, target / name)
textures = sorted((source / "textures").glob("CW_v2_*.png"))
if len(textures) != 15:
    raise RuntimeError("Expected five complete new PBR map families")
(target / "textures").mkdir(exist_ok=True)
for texture in textures:
    shutil.copyfile(texture, target / "textures" / texture.name)
files = [{"path": p.relative_to(source).as_posix(), "bytes": p.stat().st_size, "sha256": sha(p),
          "purpose": "source render only; not gameplay" if "renders" in p.parts else "original reproducible source product"}
         for p in sorted(source.rglob("*")) if p.is_file() and p.suffix.lower() in {".blend", ".fbx", ".png"}]
index = {"version": "cinderwall-scenery-v002", "large_artifacts": "ignored .local/overkill-foundry/art/scenery-v002", "files": files,
         "source_sha256": sha(Path(__file__).with_name("scenery_v002_build.py")),
         "helper_sha256": sha(Path(__file__).with_name("build_cinderwall.py")),
         "rights": "Original Game Studio / Codex Blender geometry and materials. Existing original Cinderwall/Mara heroes appear only in source reviews. No external art; studio distribution licence unset.",
         "limits": "No Unreal quality or performance claim. Klaus's human visual approval remains required."}
(target / "source-artifact-index.json").write_text(json.dumps(index, indent=2), encoding="utf-8")
rows = []
for asset_id, path in [("scenery_v002_source", "art-source/scenery_v002_build.py")] + [("scenery_v002_" + p.stem.lower(), (target / "textures" / p.name).relative_to(game).as_posix()) for p in textures]:
    rows.append([asset_id, path, "original: Blender 5.2.1 LTS procedural source; no external assets", "Game Studio / Codex", "Original project content; studio distribution licence unset", "assets/production/scenery-v002/source-artifact-index.json", "Original layered foundry scenery and matte PBR surfaces using existing project helpers", "source-and-FBX-verified; Unreal and human visual acceptance pending"])
with Path(__file__).with_name("scenery_v002_manifest-rows.csv").open("w", encoding="utf-8", newline="") as f:
    csv.writer(f, lineterminator="\n").writerows(rows)
print("SCENERY_COMPACT_PUBLISHED " + str(target))
