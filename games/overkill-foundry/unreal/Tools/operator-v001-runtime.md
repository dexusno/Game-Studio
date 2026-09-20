# Operator and shooting scene integration

**Owner review:** Klaus rejects the current graphics and perspective as far below
the concept16C/F.I.S.T. target. The evidence below records technical integration
only. Further art, pose and UI refinement is deferred while mechanics are completed.
The game is turn based: core rules assign damage and these graphics display it.

The 20 September 2026 integration places Mara beside the gun and adjusts her
authored arm poses to its current aim. Clicking **Lock and load** enters the
interactive shooting view before Fire; shot playback returns to preparation in
the same player turn. Only End Turn advances the enemy phase.

The shooting scene retains the original nearby work platform, hopper, claw and
rail. More distant factory geometry, the original city matte, fog and three
adjacent sections of the original deck provide depth and continuous ground.
The Mite is forward of its earlier position; larger robots retain stable slots
when another robot dies. These transforms do not change core targeting or order.

## Frozen execution

| Operation | Recorded result |
| --- | --- |
| BackdropArt193926 | Imported original texture/support; corrected scalar setter. |
| OperatorArt203155 | Imported mesh, one material and four clips; read back bounds, bones and compressed motion. |
| Editor203232 / Game203314 | Both Development targets built with Unreal 5.8.2. |
| Smoke203338 | Preparation/action captures and scene bounds passed. |
| ArtProbe203530 | Legal Ram/Mite actions, relative hit bands, death/cleanup/reset passed; 22 captures. |

The Editor module is
`9d2d671e961a206f0c4ca2144e500d220fd026178b9881d8de9453ee217d5a9d`.
The built Game executable is
`9fb422f304443ac17bdebebc73d0fe1f0e5740a6759cd6cb79805993efe9b61f`.
The 89-input LF-normalized source graph is
`1282abcaabbce655d2e10518c8f4d1b3e072dab8ccf4e67b91bc35580965d166`.
It explicitly uses the reviewed pre-terminal-correction core in
`Saved/Validation/roster-v001-runtime-20260920-1843/source/core`.
Later live core corrections are excluded from this evidence.

[The portable checkpoint](operator-v001-checkpoint.json) records all copied
inputs, both binaries, 34 execution artifacts and 296 content files. The ignored
archive is `Saved/Validation/operator-runtime-20260920-2032`. The content was
copied before the next FX import; read/copy/read checks preserve the actual
operator, scene map and materials, rather than only listing source asset names.

## Deformation and motion

The imported mesh measures 42.825 by 97.320 by 179.287 cm. Its 24 bones include
the 23 authored bones and the FBX root. The four compressed clips have the
expected durations and actual root motion of zero. Measured foot motion ranges
from 0.17 to 0.51 cm after compression; the more precise source measurements
must not be presented as engine compression results.

The runtime transforms the current authored wrists by the gun's aim delta,
preserving load/fire gestures. A shared lean, capped at ten degrees, makes room
before the two-bone arm solve. Root, pelvis and legs keep their evaluated poses.
The operator ticks after the gun. Its culling bounds include the current bone
origins and a measured envelope, rather than relying on a narrow rest pose.

[Independent source pose review](../../assets/production/operator-v001/runtime-review/REVIEW.md)
checks all 109 clip frames at both the Mite and far Gatebreaker targets. Its
218 solves have maximum wrist error below 0.000004 cm and maximum lean of
9.5 degrees. All 17,983,365 linear-skinned vertex samples fit within the bounds;
the narrowest measured clearance is 8.617 cm. CPU and Blender linear skinning
agree within 0.000036 cm on the two rendered poses. These are source checks.

The eight actual ArtProbe shots log zero reach residual and lower-body shift
at the recorded 0.001 cm precision. Root inspects the action, load and heavy-hit
captures and two source pose renders. The hands and planted body are plausible
in those views; small glove, face and hair detail remains provisional. Native
target transitions, imported extreme-pose culling and full-roster motion still
need their declared follow-up checks.

## Reproduction and limits

From the repository root, with the existing tools configured:

```powershell
$operatorCore = 'games/overkill-foundry/unreal/Saved/Validation/roster-v001-runtime-20260920-1843/source/core'
& games/overkill-foundry/tools/unreal.ps1 -Action OperatorArt
& games/overkill-foundry/tools/unreal.ps1 -Action Build -CoreRoot $operatorCore
& games/overkill-foundry/tools/unreal.ps1 -Action BuildGame -CoreRoot $operatorCore
& games/overkill-foundry/tools/unreal.ps1 -Action Smoke
& games/overkill-foundry/tools/unreal.ps1 -Action ArtProbe
```

Do not overwrite a retained archive when reproducing against changed sources.
The reported execution used Editor `-game`; a built Game target is not a
verified standalone package. ArtProbe uses controlled legal actions, not an
earned full-city run. This evidence is neither human gameplay/listening nor
Klaus's visual approval, and does not establish the selected final quality bar.
