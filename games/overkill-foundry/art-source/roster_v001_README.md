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

## Unreal import and controlled rendered verification

`unreal/Tools/import_roster_v001.py` imports to existing `/Game/Cinderwall/Robots`
and `/Game/Cinderwall/Animations`, reusing existing material instances. It checks
source proof identities, centimetre bounds, compressed pose movement, complete
bone inventories and converted muzzle sockets. It does not alter map placement,
combat rules, core state or materials. Run `tools/unreal.ps1 -Action RosterArt`
after the original Cinderwall material import. RosterArt `20260920-182853`
and a separate fresh-process `RosterVerify` run `182940` complete with exit 0
on Unreal 5.8.2: all eight meshes and 72 clips pass these checks, including every
saved material interface. [Compact import evidence](../assets/production/roster-v001/unreal-import.json)
records exact products and source hashes. Sampled compressed bone motion spans
1.57–398.67 cm. FBX reports absent smoothing groups; the importer deliberately
preserves the exported vertex normals. The existing Mara class-default socket
warning also remains. Neither warning prevented the import checks. The initial
`180658` import passed geometry/animation checks but did not persist materials:
the first rendered run `182636` caught an invalid slot on Cable Binder. Unreal
Array iteration yielded edited struct copies that were discarded. The importer
now retains those copies in a Python list, assigns the complete array, checks
readback, and checks the saved assets again in a new process. The failed run is
retained; its missing material assignment is not described as a runtime pass.

`tools/generate_roster_visuals.py --check` verifies that the Unreal cosmetic table
matches both original asset reports: all ten definitions and 87 clips. The table
contains asset names and visual cue times only. Runtime action/hit/summon/cleanup
acceptance is separate. Final `RosterProbe-20260920-184928` passes all 80
controlled cases, 1,144 checks and 255 captures on Editor build `184357`.
[The portable checkpoint](../unreal/Tools/roster-v001-runtime-checkpoint.json)
records all 72 frozen inputs, exact Editor/Game binaries, logs and capture hashes.
The 26 action cases, 29 reachable integer hit bands, ten deaths, ten escapes and
five plate/regression cases cover the distinct original meshes and all 87 clips.
They check preview/apply parity and no core-state mutation during cosmetics.
This is prepared-state coverage, not an earned full-city playthrough.

The previous 79-case run `183007` remains a recorded failure: 1,131 checks passed
but one helper-hidden assertion inspected the dying parent after automatic target
selection changed. The corrected test uses the stable formation ID. Independent
source review also found and prompted two real fixes: a final committed attack
must play before later Burn/counter death, and cancelling a camera presentation
must reconcile a hidden helper before resuming. Both now have passing cases.

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
cleanup and distinct clips now have the controlled runtime evidence above;
natural campaign use and final visual quality remain separate acceptance work.

Render the saved blend without changing the verified FBX files:

```powershell
& $Blender --background --python games/overkill-foundry/art-source/roster_v001_render.py -- --output .local/overkill-foundry/art/roster-v001
```

Human visual approval, package/performance and the final material, lighting,
operator and scene-depth finish remain incomplete. The frozen checkpoint also
retains a separately reproduced shared-core duplicate terminal-notification
defect; its subsequent correction is outside this historical roster identity.
