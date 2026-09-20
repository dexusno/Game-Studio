# Original Cinderwall roster source

Eight remaining enabled robot definitions have separate articulated source meshes,
72 clips and explicit mappings for all 24 authored actions. Rivet Mite and Breach
Ram remain the original Cinderwall-v001 assets. The new meshes total **161,934
triangles**. All surfaces reuse that project's original PBR algorithms and palette;
no external model, texture, generated image or licence assumption is involved.

| ID | Asset / prefix | Silhouette and articulation |
| --- | --- | --- |
| C1-R04 | CableBinder / CB_ | Upright cable drum walker; separate reel, feed jaws and sensor |
| C1-R05 | PressureCask / PC_ | Banded pressure vessel with walking feet, valve, lid and flywheel |
| C1-R07 | CoilNest / CN_ | Four-foot brood vault, paired shutters, induction crown and launch ramp |
| C1-R08 | KnucklePress / KP_ | Walking C-frame press with separate vertical die and two piston fists |
| C1-O01 | RedlinePursuer / RP_ | Low three-wheel motor, paired guns and tall exhaust bank |
| C1-O02 | FoilWarden / FW_ | Tall sensor/lance walker; three individually articulated ceramic tiles |
| C1-O03 | SplitChassis / SC_ | Split tracked carrier, independent hull halves, paired guns and brood opening |
| C1-B01 | GatebreakerPrime / GP_ | Large furnace torso, gate hammers, recovery joint and rear reactor |

Build with the already configured Blender executable (`config.local.json`):

```powershell
& $Blender --background --python games/overkill-foundry/art-source/roster_v001_build.py -- --output .local/overkill-foundry/art/roster-v001
& $Blender --background --python games/overkill-foundry/art-source/roster_v001_verify.py -- --output .local/overkill-foundry/art/roster-v001
python games/overkill-foundry/art-source/roster_v001_publish.py --output .local/overkill-foundry/art/roster-v001
```

Add `--no-render` to build exports without source review frames. Large FBX, blend,
repeated palette textures and renders remain ignored. Compact reports, provenance
and file identities are under `assets/production/roster-v001`.

The actual FBX reimport passes **365 checks**: manifest/generator/helper identities,
exact skeleton hierarchy, every vertex's rigid unit weight, original UV/material
slots, metre bounds, triangle counts, forward sockets, all action mappings,
sampled articulation throughout all clips, durations and complete death cleanup
contracts. The initial verifier incorrectly assumed imported animations began at
frame 1; Blender imports them at frame 2. Duration now measures last minus first
key, preserving the authored interval and 30-fps rounding tolerance. No asset
timing was altered to hide that verifier error.

## Unreal contract — prepared, not yet executed

`unreal/Tools/import_roster_v001.py` imports to existing `/Game/Cinderwall/Robots`
and `/Game/Cinderwall/Animations`, reusing existing material instances. It checks
source proof identities, centimetre bounds, compressed pose movement, complete
bone inventories and converted muzzle sockets. It does not alter map placement,
combat rules, core state or materials. The Unreal wrapper/runtime owner must add
the import action and bind these assets after the current camera checkpoint.

Every rig has root/hull/core/muzzle/intent; attack sockets inherit the actual
weapon bone. `asset-report.json` owns each definition's exact `action_clips` map.
Generic clips are prefix + idle/hit_light/hit_medium/hit_heavy/death/escape.
Split Chassis's committed death-release uses its own opening sequence. Each
animated part stays inside one skinned actor so the host can dissolve **all**
material slots from 1.1 s and destroy the actor and FX at 2.0 s. Presentation cues
only display previously committed events; they never trigger damage, summoning
or progression. Foil Warden's tile_0/1/2 bones permit visible finite-tile depletion.

Source export checks do not establish correct Unreal playback, all-action event
coverage, LOD/performance, final material/lighting quality or Klaus's approval.
Root inspected all eight source views. Cable drum, pressure vessel, brood vault,
press, motor, ceramic shield, split carrier and furnace boss have distinct
silhouettes. That review led to actual recessed bores on Cask/Split and an open
Nest chamber; the revised source was re-exported, passed all 365 checks again,
and those three new views were inspected. Finish/detail/lighting remain below
the owner benchmark and must be assessed in the real scene. The complete actor
cleanup and distinct clips are source contracts, not verified runtime behavior.

Render the saved blend without changing the verified FBX files:

```powershell
& $Blender --background --python games/overkill-foundry/art-source/roster_v001_render.py -- --output .local/overkill-foundry/art/roster-v001
```

In-engine roster/action/reaction/death checks and human visual acceptance remain
to be completed.
