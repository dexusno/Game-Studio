# Cinderwall runtime presentation

The host compiles the shared rules sources directly. `FoundrySession` collects
committed events once; `FoundryStage` sends them to `FoundryRobot` cosmetics.
Previews never enqueue animation. Core HP, death, loot and targetability never
wait for an animation, and no animation notify changes combat state.

From the repository root, with the ignored local Unreal/Blender configuration:

```powershell
# Generate original FBX, textures and asset-report.json first; see art-source/README.md.
& ./games/overkill-foundry/tools/unreal.ps1 Build
& ./games/overkill-foundry/tools/unreal.ps1 Content
& ./games/overkill-foundry/tools/unreal.ps1 Art
& ./games/overkill-foundry/tools/unreal.ps1 BuildGame
& ./games/overkill-foundry/tools/unreal.ps1 ArtProbe
& ./games/overkill-foundry/tools/unreal.ps1 Fixture
& ./games/overkill-foundry/tools/unreal.ps1 Run
```

`Content` creates the base host material/map if absent. `Art` reimports the
original exports, reconstructs generated stage actors in that map, and writes
`Saved/ArtImport/cinderwall-import.json` with source SHA-256 identities, imported
bounds, clip lengths, compressed pose motion, event sockets and placements.
Generated products in `Content/Cinderwall` and `Content/Technical` are local and
must remain ignored. There are no downloaded or purchased assets.

Import uses Unreal's legacy FBX factory explicitly. Metres become centimetres;
the conversion is `(x,-y,z)*100`. Authored weighted normals are preserved and
Unreal computes tangents. OpenGL normal maps use linear normal compression with
green flipped; normal strength is 0.35. Base color is sRGB, ORM is linear with
roughness in G and metallic in B. The source has no baked AO. All robot slots,
including emissive parts, derive from one masked material exposing `Dissolve`.
Thirty deck placements reference one mesh. The 30fps FBX clips quantize some
nominal durations by less than one frame. UE retains the Blender armature object
as one additional root (13 Ram bones, 21 Mite bones).

For static meshes, preserving FBX object pivots drops the node's unit scale in
the legacy importer. The script bakes a 100x mesh build scale and checks every
deck tile is 196cm wide. `Art` enables commandlet rendering because Unreal skips
the static mesh render-data rebuild under NullRHI; it still runs hidden and exits
when import completes. Skeletal imports use the FBX unit conversion directly.

`ArtProbe` is a rendered scripted encounter using legal core actions. It covers
deflection, all three Ram reaction bands, Mite reaction/attack, Ram charge and
held pose, blast, death/collapse/dissolve, two-second actor cleanup and restart.
It checks preview/apply equality while playing and captures actual frames in
`Saved/Screenshots/art-*.png`. Its command-path automation does not establish
physical input handling, owner visual approval, game feel or shipping performance.

The live player controls are the debug HUD plus C collect, Tab steering,
P recorded precision outcome, L load, U unload, Space fire, Enter end turn,
R restart, 1/2 camera, H panels, F9 screenshot, Escape quit. Recipe, inventory,
target and spread choices are clickable. The gun remains a primitive host prop
because this art package supplies robots and the stage. Sound and production HUD
art are separate work. Escapes are supported by the adapter but are absent from
the two-enemy teaching route. Mite light/medium clips are imported and pose-tested;
the teaching route exercises its heavy and death clips.

Review `Saved/BuildLogs` for exact engine/compiler identity and `ART_*` markers.
`Fixture` retains the independent headless parity transcript in
`Saved/Parity/core-fixture.jsonl`. A compiled game target is not a packaged build;
`Package` is a separate BuildCookRun command and must be verified separately.
