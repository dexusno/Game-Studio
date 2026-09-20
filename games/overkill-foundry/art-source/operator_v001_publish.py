"""Publish small original textures and executed evidence; keep large source local.

Standard-library only. Never changes shared manifest, status, runtime or UE data.
"""
import argparse
import csv
import hashlib
import json
from pathlib import Path
import shutil

p = argparse.ArgumentParser()
p.add_argument("--output", type=Path, required=True)
a = p.parse_args()
source = a.output.resolve()
game = Path(__file__).resolve().parents[1]
destination = game / "assets/production/operator-v001"
report = json.loads((source / "asset-report.json").read_text())
verification = json.loads((source / "interchange-verification.json").read_text())
if verification["failures"]:
    raise RuntimeError("Only a verified source candidate can be published")
for relative, digest in report["file_sha256"].items():
    if hashlib.sha256((source / relative).read_bytes()).hexdigest() != digest:
        raise RuntimeError("Stale source product: " + relative)
for relative, digest in verification["files"].items():
    if hashlib.sha256((source / relative).read_bytes()).hexdigest() != digest:
        raise RuntimeError("Stale source verification: " + relative)
files = ["generation-report.json", "asset-report.json", "interchange-verification.json", "stance-review.json"]
files += ["textures/Mara_" + name + ".png" for name in ("BaseColor", "ORM", "Normal")]
files += ["renders/" + name + ".png" for name in (
    "surface_front", "surface_side", "surface_rear", "surface_face", "rig_ready", "rig_action_rear", "rig_load", "rig_fire",
    "operator_gun_preparation", "operator_gun_action")]
for relative in files:
    target = destination / relative
    target.parent.mkdir(parents=True, exist_ok=True)
    shutil.copy2(source / relative, target)
evidence = {"asset": report["asset"], "approval": "provisional; owner visual approval pending",
    "technical_checks": len(verification["checks"]), "engine_import": "not performed by this source workflow",
    "files": {relative: {"sha256": hashlib.sha256((destination / relative).read_bytes()).hexdigest(),
                         "bytes": (destination / relative).stat().st_size} for relative in files},
    "large_source": "Ignored .local/overkill-foundry/art/operator-v001; rebuild with the owned source scripts.",
    "limitations": report["limitations"]}
(destination / "published-evidence.json").write_text(json.dumps(evidence, indent=2))
contract = {
    "asset": report["asset"], "approval": "provisional; owner in-engine approval pending",
    "mesh_path": "/Game/Cinderwall/Operator/Meshes/SK_MaraOperator",
    "material_path": "/Game/Cinderwall/Operator/Materials/MI_MaraOperator",
    "clips": {name: {**info, "path": "/Game/Cinderwall/Operator/Animations/" + name}
              for name, info in report["clips"].items()},
    "placement_proposal_cm": report["placement_proposal_cm"],
    "rest_bounds_source_m": [report["bounds_min_m"], report["bounds_max_m"]],
    "source_to_unreal": report["unreal_conversion"],
    "ready_socket_source_m": report["sockets_ready_m"],
    "authority": "Cosmetic only. Play load after accepted load, fire after committed shot, recovery then idle. No input validation, ammunition or timing authority.",
    "aiming": "Fixed source stance. Runtime must inspect cradle rotation and add bounded cosmetic arm matching if needed.",
    "hand_contact_evidence": "stance-review.json", "runtime_verified": False,
}
(destination / "presentation-contract.json").write_text(json.dumps(contract, indent=2))

headers = ["asset_id", "path", "source_url", "creator", "license", "proof_path", "modifications", "approval_status"]
rows = []
for relative, suffix in [("Mara_SourceReference.png", "reference"),
                         ("textures/Mara_BaseColor.png", "basecolor"),
                         ("textures/Mara_ORM.png", "orm"),
                         ("textures/Mara_Normal.png", "normal")]:
    rows.append(["mara_operator_v001_" + suffix, "assets/production/operator-v001/" + relative,
        "generated: original built-in image_gen plus installed TRELLIS.2 direct geometry/material route; primary terms in proof",
        "Game Studio / Codex; OpenAI image_gen; Microsoft TRELLIS.2; original Blender cleanup and rig",
        "Applicable OpenAI output terms; installed build-tool terms reviewed in proof; no model/code redistribution; studio distribution licence unset",
        "art-source/operator_v001_README.md", "Unmodified original RGBA" if suffix == "reference" else
        "Coverage-normalized generated PBR; original source clothing/face projection; original CPU UV rasterization and palette cleanup",
        "provisional-source; owner in-engine visual approval pending"])
rows.append(["mara_operator_v001_source", "art-source/operator_v001_rig.py",
    "original: Game Studio source scripts; generated anatomy provenance in proof", "Game Studio / Codex",
    "Original authored script; generated source terms in proof; studio distribution licence unset",
    "art-source/operator_v001_README.md", "23-bone rig; four authored cosmetic clips; 110k-triangle hero surface; raw source and interchange ignored",
    "provisional-source; actual interchange verified; engine integration pending"])
rows.append(["mara_operator_v001_review_renders", "assets/production/operator-v001/renders/",
    "original: CPU Blender renders of the documented Mara operator and existing original Mara gun",
    "Game Studio / Codex", "Original authored output; source terms in proof; studio distribution licence unset",
    "art-source/operator_v001_README.md", "Source inspection only; no image postprocessing or external asset pixels",
    "provisional-source-review; not owner approval or Unreal screenshots"])
with (game / "art-source/operator_v001_manifest-rows.csv").open("w", newline="", encoding="utf-8") as f:
    writer = csv.writer(f)
    writer.writerow(headers)
    writer.writerows(rows)
print(json.dumps({"published_files": len(files), "bytes": sum(v["bytes"] for v in evidence["files"].values()),
                  "destination": str(destination), "manifest_rows": len(rows)}))
