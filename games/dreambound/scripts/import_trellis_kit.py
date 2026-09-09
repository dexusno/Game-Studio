"""Import the approved local TRELLIS environment kit into the playable project.

Run in the full Unreal editor with -ExecutePythonScript=<this script> and
-TrellisKitConfig=<local JSON>. The config contains an assets list with name,
fbx, base_color and metallic_roughness. Large source assets stay outside Git.
Only /Game/Art/Trellis is written; maps, input settings and gameplay are untouched.
The existing dual-UV/vertex-color PBR graph also preserves the waymarker repair.
"""
import importlib.util
import json
from pathlib import Path
import re
import traceback

import unreal


def main():
    command = unreal.SystemLibrary.get_command_line()
    match = re.search(r'-TrellisKitConfig=(?:"([^"]+)"|(\S+))', command, re.IGNORECASE)
    if not match:
        raise RuntimeError("Pass -TrellisKitConfig=<local JSON>")
    config = json.loads(Path(match.group(1) or match.group(2)).read_text(encoding="utf-8-sig"))
    source = Path(__file__).with_name("inspect_ai3d_unreal.py")
    spec = importlib.util.spec_from_file_location("trellis_import_helpers", source)
    helper = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(helper)
    report = {"complete": False, "assets": [], "warnings": helper.REPORT["warnings"]}
    output = Path(unreal.Paths.project_saved_dir()).resolve() / "TrellisArt"
    output.mkdir(parents=True, exist_ok=True)
    try:
        for item in config["assets"]:
            name = item["name"]
            if name not in ("BellTree", "Cloister", "RootRock", "Waymarker"):
                raise RuntimeError("Unexpected kit asset: " + name)
            for field in ("fbx", "base_color", "metallic_roughness"):
                if not Path(item[field]).is_file():
                    raise RuntimeError("Missing source: " + item[field])
            options = {**item, "asset_root": "/Game/Art/Trellis",
                       "mesh_name": "SM_Trellis_" + name,
                       "material_name": "M_Trellis_" + name}
            for suffix in ("/Meshes", "/Textures", "/Materials"):
                helper.LIB.make_directory(options["asset_root"] + suffix)
            base = helper.import_texture(options, "base_color", "T_" + name + "_BaseColor", True)
            packed = helper.import_texture(options, "metallic_roughness", "T_" + name + "_MetallicRoughness", False)
            material = helper.create_material(options, base, packed)
            mesh = helper.import_mesh(options, material)
            bounds = mesh.get_bounds()
            dimensions = [float(value) * 2 for value in (bounds.box_extent.x, bounds.box_extent.y, bounds.box_extent.z)]
            if min(dimensions) <= 0:
                raise RuntimeError("Empty mesh bounds: " + name)
            uv_count = helper.MESHES.get_num_uv_channels(mesh, 0)
            if uv_count < 2:
                raise RuntimeError("Dual-UV material requires UV0 and UV1: " + name)
            convex_hulls = helper.MESHES.get_convex_collision_count(mesh)
            if not convex_hulls:
                raise RuntimeError("Authored collision is required; a bounds box would obstruct the arch or tree canopy: " + name)
            if mesh.get_num_nanite_triangles() <= 0:
                raise RuntimeError("Nanite geometry was not built: " + name)
            report["assets"].append({
                "name": name, "mesh": mesh.get_path_name(), "material": material.get_path_name(),
                "dimensions_cm": dimensions, "uv_channels": uv_count,
                "nanite_enabled": bool(helper.MESHES.get_nanite_settings(mesh).get_editor_property("enabled")),
                "convex_hulls": convex_hulls, "nanite_triangles": mesh.get_num_nanite_triangles(),
            })
            unreal.log("TRELLIS_ART_IMPORTED " + name + " " + str(dimensions))
        report["complete"] = True
        unreal.log("TRELLIS_ART_IMPORT_COMPLETE")
    except Exception as error:
        report["error"] = str(error)
        report["traceback"] = traceback.format_exc()
        raise
    finally:
        (output / "import.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")


if __name__ == "__main__":
    main()
