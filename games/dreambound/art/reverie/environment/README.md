# Reverie environment kit

Twenty-four wholly new original Blender meshes for the sculpted painterly
abbey/garden direction. No old models or textures were loaded. Root supplies
new generated PBR textures, sky and water shaders; the source material nodes
are honest untextured vertex-colored previews of the new geometry.

Use integration-manifest.json and asset-metadata.json for exact bounds,
material slots, collision hulls, placement datums and exported file hashes.
All fronts are +Y, architectural width is X and traversal through Arch is Y.
CraftUV uses 1unit/100cm projected along each actual face; LightmapUV is a
separate smart-packed unwrap. Vertex Color: R=painted value, G=bevel wear,
B=recess/weathering, A=opaque. No new UV-dependent old image textures exist.

Tile/TileB: base0/top20; place-20 for a groundZ0 route. Bridge decktop0.
Arch: keep the entire440wide x500high rectangular ground passage clear.
Rill: longitudinalX; bed-30, water-15. Basin: water35, inner radius230.
Tree has trunk and low-root hulls; foliage has no collision. TerrainPatch and
Cliff are visual landforms and require scene-owned playable floor boundaries.
No decorative gateways should be placed against solid wall faces.

The two PNGs are Blender source inspections of actual meshes, not Unreal
gameplay captures. Material import, actual game integration, performance and
play validation belong to the root integration owner.
