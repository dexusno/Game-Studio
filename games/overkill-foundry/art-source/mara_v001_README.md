# Mara gun and rear claw v001

Original Game Studio / Codex geometry, rigid rigs and keyframes, authored
20 September 2026 for Overkill Foundry. The cast receiver, hollow bore, recoil
rails, loading door, gauge, handwheel and small furnace window replace the four
primitive host shapes. A separate rear claw picks up cosmetic scrap and drops
it into a rear hopper. No human Mara model is included.

The dedicated generator imports helper functions from this game's original
`build_cinderwall.py` without running or changing its main generator. Its PBR
algorithms/maps are reused. No external model, photo, purchased asset, image
generation or other game's project/pipeline is used. STYLE and 16B/16C guide
composition only; no reference pixels are embedded. Studio distribution licence
remains unset. No new third-party art credit requirement is introduced.

## Reproduce

From the repository root, with configured Blender **5.2.1 LTS**:

```powershell
$artBlender = (Get-Content config.local.json | ConvertFrom-Json).tools.blender
& $artBlender --background --python-exit-code 1 --python games/overkill-foundry/art-source/mara_v001_build.py -- --output .local/overkill-foundry/art/mara-v001
& $artBlender --background --python-exit-code 1 --python games/overkill-foundry/art-source/mara_v001_verify.py -- --output .local/overkill-foundry/art/mara-v001
python games/overkill-foundry/art-source/mara_v001_publish.py --output .local/overkill-foundry/art/mara-v001
# Original Cinderwall Content and Art products must already exist.
& ./games/overkill-foundry/tools/unreal.ps1 MaraArt
& ./games/overkill-foundry/tools/unreal.ps1 Build
& ./games/overkill-foundry/tools/unreal.ps1 BuildGame
& ./games/overkill-foundry/tools/unreal.ps1 ArtProbe
& ./games/overkill-foundry/tools/unreal.ps1 Fixture
& ./games/overkill-foundry/tools/unreal.ps1 Run
```

`--no-render` skips the two labelled source-review stills. Launch background
helpers hidden; the Unreal script does so. Editable `Mara_Source.blend`, FBX
meshes/clips, reports and generated copies of the original surface maps remain
in the ignored output directory. Unreal derivatives remain ignored under
`Content/Cinderwall/Mara`; compact reports and the contract are published under
`assets/production/mara-v001`. Manifest rows are supplied separately for root.

Gun: **50,744 triangles, 12 source bones, 3.27 × 1.665 × 2.212 m**. Claw:
**10,524 triangles, 9 bones, 1.249 × 1.218 × 3.575 m**; suspended rest geometry
occupies Z 2.53–6.105 m. All vertices have one full-weight bone. Unreal retains
one extra armature root. Attachment bones locate the muzzle, loading/ejection
points, future shield/operator mounts and claw grab/dump points. Sockets do not
implicitly provide character/shield meshes.

Metres become centimetres with Y reflected. Weighted normals are preserved;
Unreal generates tangents. BaseColor is sRGB; ORM is linear (R=1, G roughness,
B metallic); linear OpenGL normals flip green and use strength 0.35. The existing
Cinderwall PBR master supplies all surfaces. Static render geometry bakes 100×
build scale to preserve pivots. Centimetre payloads attached to FBX sockets use
absolute world scale 1 to avoid inheriting the armature's unit conversion twice.
The host also synchronizes static mesh culling bounds to actual render bounds:
UE5.8 otherwise caches pre-build-scale mesh-description bounds, causing scenery
to disappear intermittently despite correctly sized render geometry.

The validator actually reimports both FBX meshes and all seven clips, checking
sockets, rigid weights, UVs, dimensions, durations, motion, recoil, claw reach,
cable scale and eleven depth/payload modules. Unreal checks compressed motion
and centimetre bounds separately. Compression removes the gun idle's 8 mm valve
drift; action motion remains mandatory. Neither check establishes visual quality.

## Runtime and evidence boundary

`FoundryMara` consumes committed `collected`, `loaded`, `unload` and `fire`
events. Gun and claw playback are independent. Rapid legal actions replace short
gun cosmetics without a queue. Core resource, damage, load and turn state never
waits for animation, and animation never writes it. The 1.4 s claw animation
shows a cosmetic payload and a 0.18 s drop; it grants nothing. Restart clears it.

`ArtProbe` legally collects, crafts, loads and unloads, captures those motions,
then restarts the teaching state before the existing eight-shot encounter.
It checks cue delivery, payload scale/reset, preview/apply equality and existing
enemy death/cleanup. Captures are `Saved/Screenshots/mara-*.png` and `art-*.png`.
Logs and import identities are in `Saved/BuildLogs` and `Saved/ArtImport`.
Bindings and mechanics are unchanged; physical input remains a separate check.

The first hero proof uses an explicit **bebb22d review core snapshot** while P08
changes live core. `Build/BuildGame -CoreRoot <local core directory>` overrides
only that invocation; default builds use live core. The binary logs a source
digest and snapshot flag. A local dependency stamp invalidates UBT rules when
roots change. Current P08 core requires a later rebuild/parity check.

The verified 20 September review used these build invocations:

```powershell
& ./games/overkill-foundry/tools/unreal.ps1 Build -CoreRoot .local/overkill-foundry/core-snapshots/bebb22d/games/overkill-foundry/core
& ./games/overkill-foundry/tools/unreal.ps1 BuildGame -CoreRoot .local/overkill-foundry/core-snapshots/bebb22d/games/overkill-foundry/core
```

The snapshot was extracted unchanged with `git archive bebb22d` for the core
directory. Its compiled source digest is
`b09cccab6d7202ebafc36946497fabed5c5bf089d0b27c1fbe9aeac900b57d32`.
Both targets succeeded in Unreal 5.8.2 / MSVC 14.44.35228. Final logs are
`Build-20260920-151630.log`, `BuildGame-20260920-151707.log`,
`ArtProbe-20260920-151734.log` and `Fixture-20260920-151856.log` under
`unreal/Saved/BuildLogs`. The rendered probe passed 22 capture requests and
legal event checks; measured muzzle motion was 26.25 cm. The 85-line fixture
matched a separately compiled snapshot runner byte-for-byte (state hash
`37978146880e1302`, transcript SHA-256
`FEB692E05ED1BA47FDD9B0A3985F61DFD73B355F79D151A4825DA4CAA76451CD`).
Exact binary/import identities are in ignored
`unreal/Saved/ArtImport/mara-runtime-verification.json`.

This is an early rendered hero implementation, with a debug HUD, repeated
environment modules, simple FX, no sound/human model, no LOD/performance approval
and no packaged Windows proof. It is not F.I.S.T.-level acceptance or owner
sign-off. The production contract describes exact clip/event boundaries.
