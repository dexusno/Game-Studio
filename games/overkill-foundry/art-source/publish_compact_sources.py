"""Copy only compact source maps and evidence into owned production paths.

The large editable scene, meshes, clips and source renders stay ignored.
The shared assets/manifest.csv is deliberately not written by this worker.
"""
import argparse
import csv
import hashlib
import json
import shutil
from pathlib import Path

parser = argparse.ArgumentParser()
parser.add_argument("--output", required=True)
args = parser.parse_args()
output = Path(args.output).resolve()
game = Path(__file__).resolve().parents[1]
repo = game.parents[1]
production = game / "assets/production/cinderwall-v001"
textures = production / "textures"
textures.mkdir(parents=True, exist_ok=True)
verification = json.loads((output / "interchange-verification.json").read_text(encoding="utf-8"))
if verification["failures"]:
    raise SystemExit("Refusing publication of failed interchange checks")
files = sorted((output / "textures").glob("*.png"))
if len(files) != 27:
    raise SystemExit("Expected all 27 original maps")
for source in files:
    shutil.copyfile(source, textures / source.name)
for filename in ("asset-report.json", "interchange-verification.json"):
    shutil.copyfile(output / filename, production / filename)

def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()

index = {
    "build": "cinderwall-art-v001",
    "source": "art-source/build_cinderwall.py",
    "source_sha256": digest(game / "art-source/build_cinderwall.py"),
    "output_root_from_repo": output.relative_to(repo).as_posix(),
    "source_renders_are_gameplay": False,
    "files": [{"path": p.relative_to(output).as_posix(), "bytes": p.stat().st_size, "sha256": digest(p)}
              for p in sorted(output.rglob("*")) if p.is_file()
              and p.suffix in (".fbx", ".glb", ".png") and ".fbm" not in p.as_posix()],
}
(production / "source-artifact-index.json").write_text(json.dumps(index, indent=2) + "\n", encoding="utf-8")

header = ["asset_id", "path", "source_url", "creator", "license", "proof_path", "modifications", "approval_status"]
creator = "Game Studio / Codex / Blender 5.2.1 LTS"
license_text = "Original project content; studio distribution licence unset"
proof = "art-source/README.md"
status = "source-export-verified; Unreal integration and human visual approval pending"
rows = [["cinderwall_art_v001_source", "art-source/build_cinderwall.py", "original: procedural Blender geometry materials rigs and keyframes", creator,
         license_text, proof, "Fresh Breach Ram and Rivet Mite meshes; 15 action clips; modular Cinderwall stage; no external art", status],
        ["cinderwall_art_v001_contract", "assets/production/cinderwall-v001/presentation-contract.json", "original: ART-DIRECTION presentation requirements and authored source clips", creator,
         license_text, proof, "Original rig socket bindings and material/reaction/death integration contract; no gameplay authority", status]]
for path in sorted(textures.glob("*.png")):
    rows.append(["cinderwall_v001_" + path.stem.lower(), path.relative_to(game).as_posix(),
                 "original: art-source/build_cinderwall.py materials()", creator, license_text, proof,
                 "Original deterministic procedural PBR map; 256x256 RGBA PNG; fully opaque; no external texture pixels", status])
with (game / "art-source/manifest-rows.csv").open("w", encoding="utf-8", newline="") as stream:
    writer = csv.writer(stream)
    writer.writerow(header)
    writer.writerows(rows)
print(json.dumps({"published_maps": len(files), "manifest_rows": len(rows),
                  "verification_checks": len(verification["checks"]),
                  "compact_bytes": sum(p.stat().st_size for p in production.rglob("*") if p.is_file())}))
