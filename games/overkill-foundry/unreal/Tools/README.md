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
# Generate/verify Mara first; see art-source/mara_v001_README.md.
& ./games/overkill-foundry/tools/unreal.ps1 MaraArt
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
UE5.8's cached mesh-description bounds omit that build scale. The host therefore
synchronizes each loaded static mesh's bounds to its actual render data before
the first view, then checks every deck component's culling extent is 98 cm.
Checking render bounds alone misses intermittent self-occlusion. Editor launches
also finish pending asset compilation before exposing the encounter.

`ArtProbe` is a rendered scripted encounter using legal core actions. It covers
deflection, all three Ram reaction bands, Mite reaction/attack, Ram charge and
held pose, blast, death/collapse/dissolve, two-second actor cleanup and restart.
It checks preview/apply equality while playing and captures actual frames in
`Saved/Screenshots/art-*.png`. Its command-path automation does not establish
physical input handling, owner visual approval, game feel or shipping performance.

The separately callable `Teaching` scene uses the debug HUD plus C collect, Tab steering,
P recorded precision outcome, L load, U unload, Space fire, Enter end turn,
R restart, 1/2 camera, H panels, F9 screenshot, Escape quit. Recipe, inventory,
target and spread choices are clickable. The dedicated `mara_v001` package adds
the original forge gun and articulated rear claw; see its source README and
presentation contract. The campaign uses the player interface described below.
Escapes are supported by the adapter but are absent from
the two-enemy teaching route. Mite light/medium clips are imported and pose-tested;
the teaching route exercises its heavy and death clips.

Review `Saved/BuildLogs` for exact engine/compiler identity and `ART_*` markers.
`Fixture` retains the independent headless parity transcript in
`Saved/Parity/core-fixture.jsonl`. A compiled game target is not a packaged build;
`Package` is a separate BuildCookRun command and must be verified separately.

## Campaign interface

Normal `Run` opens the campaign title. New Game creates Mara's Cinderwall run;
only replacement of an active saved campaign asks for confirmation. Continue
restores a between-fight choice exactly and restarts an unfinished fight from
its original entry. Escape closes the current drawer or opens the menu; after
opening a save from disk, Escape cannot bypass Continue. F3 toggles diagnostics.
The default save is local `Saved/Campaign/profile.ofsave`, with a durable prior
generation managed by the shared Windows SaveStore. `-SavePath` selects an
explicit independent QA profile.

The adapter compiles CampaignRules, cinderwallUpgradeHooks, CampaignSession and
SaveStore directly. Every durable action carries the saved run ID and next
transaction sequence. Screen selection, animation and previews never change
committed state. The interface includes the Mayor, twelve-position route,
finite shop, preparation/shot cameras, full-screen recipe and part drawers,
reward choices, memory exchanges and pending upgrade choices. Mystery route
cards hide the stored outcome; only production reveal queries expose forecasts.

Use the mouse for choices and target cards. During preparation, C collects,
L loads the selected bullet, U unloads, Space fires and Enter ends the turn.
Recipe and part pages suppress encounter shortcuts. Recipe details show source
effects, printed cost, current supplies and the successful shared-core preview's
actual payment; blocked recipes retain the source cost and exact rejection.
Utilities act immediately. A zero cooldown is labelled None with the ordinary
once-per-turn rule. Lists scroll; no twenty-part display cap is imposed.

```powershell
& ./games/overkill-foundry/tools/unreal.ps1 Audio
& ./games/overkill-foundry/tools/unreal.ps1 CampaignProbe
& ./games/overkill-foundry/tools/unreal.ps1 AudioProbe
& ./games/overkill-foundry/tools/unreal.ps1 Run -SavePath .local/my-foundry-test.ofsave
```

CampaignProbe traverses a real seeded run from New Game through Mayor, finite
purchase, collection, crafting, installed Shield, loading, firing, rewards and
the next route position. It checks Continue entry restoration and disk reload.
It is scripted command-path evidence, not physical-input or balance evidence.
Screenshots are requested after a completed layout. UI also prepasses newly
rebuilt Slate pages so the player does not see an unarranged transition frame.

The fixture generator under `ui-fixtures` creates **prepared UI stress data**
with twenty memories and twenty-four parts in each pool, plus pending pack and
nested offer choices. These injected datasets do not prove legitimate acquisition
or balance. `Run -SavePath <fixture> -InspectSave` opens that exact prepared
state without Continue; this explicit inspection option is absent in Shipping.

Audio is original procedural PCM, imported into local `/Game/FoundryAudio`.
The audio adapter consumes committed events once, uses bounded voice groups,
and stops its ambience when the scene closes. `AudioProbe` plays the palette and
exports the actual engine master mix under `Saved/AudioCapture`; it omits
`-nosound`. Mixer capture, native listening and owner sound approval are separate.

Design references inspected on 20 September 2026: the text and UI examples in
the [Into the Breach developer postmortem](https://media.gdcvault.com/gdc2019/presentations/Into%20the%20Breach%20Postmortem%20Final.pdf)
(telegraphed intent and clear consequence), and [Xbox guideline 112](https://learn.microsoft.com/en-us/xbox/accessibility/xbox-accessibility-guidelines/112)
(consistent navigation and focus). They inform hierarchy and cancellation;
this is an original engine-native interface and no accessibility certification.

This is a connected campaign UI proof, not final visual approval. Eight enemy
definitions still use explicitly labelled temporary models; only Mite and Ram
have original finished rigs here. Precision uses the live timing widget below;
manual success cannot be selected. Advanced optional recipe payments, sacrifice/choice
fields and discount-allocation controls remain to be exposed. Such recipes can
be inspected but may reject a default Use request. Controller navigation,
localization, reduced-motion settings and packaged-game QA remain unverified.

### Verified campaign checkpoint, 20 September 2026, 16:37 local

Editor `Build-20260920-163257.log` and game target
`BuildGame-20260920-163320.log` succeeded with Unreal 5.8.2 CL56702186,
MSVC 14.44.35228 and Windows SDK 10.0.22621.0. The editor module SHA-256 was
`604c752c98d1b10d277b4505e57484f61fb81910129c14f2b4681e62d2908705`;
the game executable was
`7be774d815fccf8d311619616461ffa0ac645aae5e9749f1475a853d83cb47b3`.
These identify a development checkpoint before Precision integration.

`CampaignProbe-20260920-163350.log` completed the legal entry-to-reward loop,
position 2, HP 68, revision 39, 37 receipts, hash `4947d322bc82be4e`, and thirteen
rendered captures. `Fixture-20260920-163506.log` matched all 85 ordered native
transcript lines, final hash `965951ac6f2b9563`. Explicit bebb22d review mode
also compiled (`Build-20260920-161814.log`) and matched all 85 native snapshot
lines (`Fixture-20260920-161833.log`, hash `37978146880e1302`).

Native mouse/keyboard checks used separate saves: `Run-155123` covered New Game
cancel, saved pack selection, collection, crafting, loading, Fire and End Turn;
`Run-160905` covered prepared twenty-copy exchange/cancel/scroll;
`Run-161235` covered prepared twenty-four loaded Ammo and twenty-four installed
Shield at 1280×720, suppressed Enter behind the part drawer and Unload;
`Run-161518` verified disabled claimed rewards and Continue to position 2 from
a real prior campaign checkpoint. All names use the `20260920` date prefix.
The final `Run-20260920-163602.log` checked the repaired side-by-side exchange
facts at 1280×720, scrolled to exact copy 20, replaced it and reached saved
Choice 2 of 3. These prepared stress saves do not demonstrate earned inventory.

Exact source, binary, import-report, log and archived capture identities are in
local `Saved/Validation/campaign-ui-20260920-1637/identity.json`. The archive
includes the final `Recipes`, separate `Firing…`/`Enemy turn`, recipe reward
and native comparison frames. Independent QA reviewed earlier named captures;
the final comparison repair was checked by its implementing agent. The Fire
capture occurs during the camera blend; a settled 16C view is still needed.

The earlier `CampaignProbe-20260920-162255.log` showed multi-second transaction
delays. A subsequent isolated run measured 38 successful transactions at
8.533–12.310 ms and pure rules on a copy at no more than 1.235 ms. The cause of
the earlier incident is unknown. `-FoundryTransactionTiming` records existing
SavePoint intervals and an extra nonmutating rules evaluation; CampaignProbe
enables it. Save flushing, replacement and verification were not weakened.

Audio import and the corrected master mix probe were verified separately in
`Audio-20260920-160601.log` and `AudioProbe-20260920-162046.log`; the latter's
`.binary.json` preserves its different module identity. Root's independent PCM
analysis is in `assets/audio/mechanical-v001/unreal-mix-evidence.json`.

### Precision integration checkpoint, 20 September 2026, 16:58 local

During Collection, P or Precision opens the real timing meter. Steering is
captured on opening; all three displayed material outcomes come from core
previews. The widget receives `upgradePrecisionWidthPercent`. Before Start,
Back/Escape changes no campaign state. Once started, the modal keeps its single
widget instance, blocks other controls and cannot be cancelled. Only the measured
0/1/2 category crosses into Collect. The saved MY1-17 retry instead submits
ResolveUpgradeChoice for that exact saved ID with `values={result}` and retains
its original steering. A failed save freezes the result; returning to title uses
ordinary Continue semantics rather than reopening the measured attempt.

Editor `Build-20260920-164602.log`, game `BuildGame-20260920-164704.log` and
`CampaignProbe-20260920-164745.log` pass. The latter checks opening, command
blocking and cancellation against an unchanged campaign hash, then completes
the legal city loop with fourteen captures. Editor module SHA-256:
`89519c6fbb0b61b2b652fffc25345f0a64b488c6439da99c9115beb3bcc697b8`;
game SHA-256:
`93feb5a3fd65750ed87ee66fa05cb445b260ae1b3dc220b400e243ca5e7a9526`.

Native 1280×720 checks in `Run-20260920-164911.log` verified keyboard P,
suppressed C behind the modal, pre-start Escape, timeout Miss at 1.800000 s and
an actual timed Space stop producing Perfect at 0.893910 s. The core committed
10 and 12 collected units respectively. These were separate attempts separated
by the ordinary title/Continue encounter restart. A fixed timing script sends
real Windows key input; it does not assign the result directly.

`Run-20260920-165450.log` inspected a prepared saved retry: pre-start cancellation
kept the choice, Escape during the running sweep did not cancel it, focus loss
paused the marker with no transaction, and a resumed native stop produced Good
at 0.722187 s. Exactly one saved retry answer added one Glass. In round 2 the
Precision button stayed disabled and P returned the core's already-used reason.
The ready, Perfect, retry, paused, Good and spent captures are archived with
source/log identities under `Saved/Validation/precision-ui-20260920-1658`.
The parent-owned controller also has 46 isolated assertions. Wider bands are
covered there, not by this 100%-width graphical pass. Save-failure recovery UI,
held-key hardware repeat and owner feel assessment remain untested.
