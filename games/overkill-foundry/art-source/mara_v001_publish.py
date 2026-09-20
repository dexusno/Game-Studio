"""Publish compact original-art identities, never raw assets/source renders."""
import argparse
import hashlib
import json
from pathlib import Path
import shutil

p = argparse.ArgumentParser()
p.add_argument("--output", required=True)
source = Path(p.parse_args().output).resolve()
target = Path(__file__).resolve().parents[1] / "assets/production/mara-v001"
verification = json.loads((source / "interchange-verification.json").read_text())
if verification["failures"]:
    raise RuntimeError("Cannot publish failed interchange checks")
target.mkdir(parents=True, exist_ok=True)
for name in ("asset-report.json", "interchange-verification.json"):
    shutil.copyfile(source / name, target / name)
files = []
for path in sorted(source.rglob("*")):
    if path.is_file() and path.suffix.lower() in {".fbx", ".blend", ".png"}:
        files.append({"path": path.relative_to(source).as_posix(), "bytes": path.stat().st_size,
                      "sha256": hashlib.sha256(path.read_bytes()).hexdigest(),
                      "purpose": "source-review-only; not gameplay" if "renders" in path.parts else "original reproducible source product"})
result = {"version": "mara-art-v001", "large_artifacts": "ignored .local/overkill-foundry/art/mara-v001", "files": files,
          "source_sha256": hashlib.sha256(Path(__file__).with_name("mara_v001_build.py").read_bytes()).hexdigest(),
          "surface_dependency": "Original Cinderwall-v001 palette/algorithms; registered separately.",
          "rights": "Original Game Studio / Codex Blender geometry, rig and animation. No external art; studio distribution licence unset.",
          "quality": "Not gameplay evidence or human visual approval."}
(target / "source-artifact-index.json").write_text(json.dumps(result, indent=2), encoding="utf-8")
print("MARA_COMPACT_PUBLISHED " + str(target))
