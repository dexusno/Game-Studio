# Reverie independent QA — final package, bounded coverage

2026-09-10. Reviewer owns this report only; production fixes belong to the integration/scene authors. Acceptance is the owner's complete visible graphics replacement, functional and naturally structured procedural spaces, improved sound effects, and retained controls/progression/saves. Suno use is pending sign-in; the current audio source is a disclosed CC0 foley replacement.

Latest result: the final Windows Shipping package's integration-run geometry suite passed **2 groups / 11 observations / 0 failures**, independently checked against its retained report. Its final packaged first view preserves the corrected stone materials, wall/soil contact, tree, sky and planting. Independent native startup and menu entry worked. Further input checks stopped when Sky twice detected user activity in the game window; a W tap was inconclusive, and guard, pause, sound controls, persistence and exit remain unverified. The isolated demo remains open at integration's request. Fountain crown/trunk are still dark; no sound-quality, complete-playthrough or anchor-B acceptance claim.

## Artifact and coverage

- Final source: `10e9738586944c561a79b73792d04a9c61d2c878`, Windows Shipping `0.4.0-reverie`. Earlier findings below were made against uncommitted source over base `042c8aab0f1d19b5cc5877e0040915519f6bdf5d`; their subsequent fixes/rechecks are recorded.
- Executable: `BuildOutput/Reverie/Windows/Dreambound/Binaries/Win64/Dreambound-Win64-Shipping.exe`, SHA-256 `5C5DCD3B0D0A27E384AE8C9E4FAFB8BD974B780984A5FE7383F4B07B20741F07`.
- Environment: Windows, Unreal `5.8.2-56702186`, local RTX 4090 machine. Tools used: filesystem/source inspection, image inspection, and installed computer-use `@oai/sky` for the native package. Native launch used1440×900 windowed; retained packaged art capture is1600×1000.
- Retained controls: WASD/mouse, LMB strike/hold-select/release, RMB guard, Q recall, F heavy, Shift dash, Space jump, E ward, 1–3 rewards, R core, Tab instructions, Escape pause. Pause adds sound level and mute.
- Actual inspection: source and import records; source Blender images versus anchor B; several actual integration-owned Editor captures; the final integration-owned packaged art capture and geometry report; then independent native package launch/startup/menu entry and bounded attempted inputs. No audio listening or human playtest assessment is claimed.
- Initial render identity: Unreal5.8.2-56702186, game module SHA-256 `E8B329B0A5385F7E1265587553E0B811FD82AC96C535251891FD00855E75BF2E`; first-view PNG SHA-256 `87809559F4B4FF9ABC9C75AB903781457DCB37EA37907884E3B0D92D25691ED2`. Log confirms `-DBCapture -DBFirstView -DBSeed=833283 -DBSaveSlot=DreamboundQA_ReverieFirst -RenderOffscreen -windowed -ResX=1600 -ResY=1000 -ForceRes -unattended -nosound -log -game`,29 scene mesh families, three rooms and no existing QA save. Content may be revised after this image.

## Findings

### RV-QA-01 — High: closed mid-bridge gate has traversable side gaps

Initial finding: prepackage uncommitted Reverie source described above. Status: source correction independently rechecked; final packaged runtime report confirms the closed/open gate correction.

Steps: before earning the first ward reward, enter its outgoing arch; approach the gate at the bridge midpoint; move to either side of the visible gate at approximately 270 cm from the route centerline; continue along the bridge.

Expected: the earned progression barrier blocks all playable lanes until the reward opens it. Actual source geometry: the gate box half-width is 220 cm, while the bridge floor/curb interior spans ±316 cm. A radius-32 cm player capsule centered at offset270 occupies 238–302 cm, clearing both barrier and curb. The arch only narrows the entrance and exit, leaving room to sidestep after entering. This is a concrete source-supported clearance defect, not yet an executed player traversal.

Evidence: `DBRecoveryScene.cpp` creates the mid-bridge gate box with `FVector(16,220,250)`. `art/reverie/environment/asset-metadata.json`, `SM_RV_Bridge`, gives the continuous floor width632 and curb inner edges ±316. `DBShieldChecks.cpp::CheckRouteGeometry` checks a centerline closed-gate ray only; the three route capsule lanes deliberately ignore gates.

Affected files: scene gate setup and its focused geometry check; visible gate scale may also need to span the full bridge. Recheck closed capsule lanes at center and ±270, then verify those routes reopen with the earned gate.

Source recheck: author widened the visible lattice by1.6 and set the child collider to `350/1.6` half-width, giving350 cm after inherited scale. The engine fixture sweeps capsules at0 and±270 while closed, temporarily opens the gate, checks all three lanes clear, then restores it. Final packaged `reverie-art-checks.json` reports those lanes blocked when closed and clear when opened; reviewer inspected that actual integration-run result. This closes the defect for the sampled routes, without claiming ordinary player traversal.

### RV-QA-02 — Medium: documented audio voice limits are not imported

Build identity: current uncommitted importer described above. Status: corrected import independently inspected; playback impact unmeasured.

Steps: use an earned multi-target attack against several enemies, or overlap rapid enemy fire/hits/collapses. Expected: the new palette's stated shared voice caps and priorities protect important feedback and bound aggregate effects. Actual importer only marks looping sounds; it does not configure the documented concurrency or priority settings. Local trigger cooldowns do not enforce a global cap across distinct enemies.

Evidence: `scripts/import_reverie.py` audio loop versus `assets/audio-reverie-notes.md` integration table (shared cap2 for fire/hit/defeat and water; priority hierarchy). No clipping or perceptual failure is claimed from source alone. Apply the intended limits or record a deliberate revised mix design, then inspect one representative engine mix.

Recheck: importer now writes each recipe's per-SoundWave concurrency override across owners, stop-farthest-then-oldest resolution and priority. Actual `Saved/Reverie/import.json` reports complete=true and persisted settings for23 cues. Water deliberately uses3 virtualized voices to keep each distant fountain alive; sources are4200 cm apart with1300 cm audible ranges. This is a documented correction; final engine mix/listening remains unverified.

### RV-QA-03 — High visual: obvious repetitive striping on architectural materials

Artifact: actual Editor-game offscreen first view, seed833283,1600×1000, file timestamp18:48:34 local. Reviewer independently inspected and preserved `captures/raw/reverie-qa/editor-first-view-184834.png` (ignored raw capture). This is ordinary first-view framing under `-DBCapture -DBFirstView`, not native-input gameplay or packaged evidence.

Steps: load the new spring cloister first view; inspect the left masonry, distant rock faces and the foreground ward plinth's vertical side. Expected: tactile stone with stable, readable surface detail. Actual: dense regular horizontal/zigzag striping overlays these surfaces, especially the plinth and left walls. It looks like repeated weave/moire rather than stone, substantially degrading the clean source-mesh preview.

Affected file likely `scripts/import_reverie.py` stone material: unfiltered procedural normal uses `sin/cos(UV*110)` on all stone/mortar/moss and crystal-base materials. This is a source-informed hypothesis; remove/filter the fine procedural normal and re-render to establish the cause. Other possibility is excessive image texture repetition. Sent to integration for correction.

Integration reports the analytic normal node removed, a new soil material/texture, reduced central paving, and fresh TRELLIS tree/bank/cliff replacements in progress. Hold additional image runs until that import is complete; the first image remains the only inspected Unreal version so far.

Visual recheck — resolved in the integrated Editor scene: `editor-integrated-first-193500.png` no longer shows striped/weave bands on the left walls, cliff/stone surfaces or foreground plinth. Broad surface mottling is stable in this static view. This closes the observed artifact for that image; motion aliasing and the final cooked result remain untested.

Visual observations apart from that artifact: the first view clearly contains a new full art vocabulary, a real arch/bridge opening, new planted tree, paving, ward crystal and distant fountain; the old sparse arrangement is visibly replaced. The broad bright flat floor, geometric lawn edge, faceted cliffs and angular canopy still read substantially below anchor B's sculpted organic forms and layered, enveloping composition. This is reviewer visual judgment, not owner acceptance. Waterfall/cup alignment and enemy/expanded-shield presentation require closer or staged views.

### RV-QA-04 — Medium: location-based enemy cues lack attenuation/spatial settings

Build identity: new Reverie SoundWave import and current enemy source. Status: sent to integration; no listening test.

Steps: have a caster fire from either side or at different distances in a court. Expected: hostile fire, warnings and contacts convey a useful source direction and distance. Actual source configuration: `DBEnemy.cpp` calls `PlaySoundAtLocation` without an attenuation argument; the new raw SoundWaves have no attenuation configured by `import_reverie.py`. A location argument alone does not enable that processing.

Supporting primary source: installed Unreal5.8 `Engine/Source/Runtime/Engine/Private/SoundBase.cpp::GetAttenuationSettingsToApply` returns null when no asset is assigned. `AudioDevice.cpp::PlaySoundAtLocation` selects override or SoundBase settings and sets `bHasAttenuationSettings` only when one exists. The current importer assigns looping/concurrency/priority but no attenuation asset. This supports the configuration defect; it is not an observed stereo-listening result.

Affected files: new sound import/settings, with explicit per-call overrides if shared local/world cues need distinct behavior. Keep local charge/weapon mechanism cues local; add spatial attenuation to hostile/location-dependent effects and inspect an engine mix when possible.

Recheck: independently inspected actual `Saved/Reverie/surfaces-import.json`, SHA-256 `B66D565E0BC50820AB83B14736728C23332CC3EA1FF224E46C719C042EEAA0A8`. It reports complete=true, scope=materials_and_audio,27 materials,23 sounds,0 warnings. `A_RV_Threats` is persisted on all four `S_Enemy*` cues plus `S_BossTell`, `S_Impact` and `S_HeavyImpact`; source creates spatialization and distance attenuation with350 cm inner extent plus1850 cm falloff. This verifies the new material/audio import, not a full mesh import or audible engine behavior.

One residual case remains: `DBThrownShield.cpp:348` plays `S_Guard` at an Anchor projectile-interception contact. That SoundWave still has null attenuation in the inspected report and is excluded from the importer's spatial list. Add it to that list or supply an explicit per-call attenuation override. Player-local `PlaySound2D` guard/hinge calls remain local; assigning asset attenuation does not convert those2D calls into positional playback. Sent to integration before the next import.

Complete-import recheck — corrected: the later `Saved/Reverie/import.json` now records `S_Guard` with `/Game/Audio/Reverie/A_RV_Threats.A_RV_Threats`, cap2 and priority100. This resolves the remaining configuration omission; auditory spatial behavior is still not claimed.

## Bounded normal-runtime asset/audio source audit

Follow-up audit while TRELLIS uses the GPU: read-only inspection of current uncommitted `0.4.0-reverie` source over base042c8aa. No game, editor, render or GPU process launched by the reviewer. Existing import reports and WAV files were read only.

**Passed static routing checks:** no unconditional legacy game 3D mesh/material load, and no legacy sound path, was found in the normal path. This statement is about the source paths and inspected import records, not every component in a final cooked world.

| Flow | Checked source and result |
| --- | --- |
| Startup, new run, resume/retry | `DBGameMode.cpp:72` selects the current slice unless explicit `DBLegacyBeta`; `BuildWorld` at157 immediately calls `BuildRecoveryCourtyard`. New at526 and resume at540 use that same function. Native character is the default pawn at67. Saves restore gameplay/seed data, not legacy mesh paths. |
| Generated environment, wards and gates | `DBRecoveryScene.cpp:45` and52 build only `/Game/Art/Reverie` mesh/material paths. Scene instances and individual moving ward/gate actors use new names. Its floor/barrier boxes are hidden physics components. Missing required scene assets log errors and skip those visuals; they do not substitute old game meshes. |
| Folded/guarding shield, sleeve, upgrades and contact effects | `DBCharacter.cpp:159` onward loads all mesh/material families from Reverie; old WeaponBody is explicitly cleared at182. Shield states at1760 onward reuse the new plate/glow meshes. DrawBeam/DrawSpark use the new Beam/Spark and new element materials. `DBThrownShield.cpp:33` onward resolves only new thrown plate/glow meshes and materials. |
| Enemies, practice and hostile bolts | `DBEnemy.cpp:243` onward uses new body/head/limbs/core and new effect materials. `DBRewardPractice.cpp:40` and46 spawn the same native enemy class. `DBProjectile.cpp:46` onward uses new Beam/Crystal and CombatGlow. `DBCombatEffect.cpp:72` onward uses the new Beam/Crystal and element materials. |
| Audio | Player cues at `DBCharacter.cpp:221`, enemy cues at `DBEnemy.cpp:243`, thrown contacts at `DBThrownShield.cpp:35`, encounter/clear at `DBGameMode.cpp:288`/333 and water in the new scene all load `/Game/Audio/Reverie`. No `/Game/Audio/` path outside that namespace remains in the source scan. Charge component explicitly receives the new loop. All23 new WAVs were compared to any same-named old WAV: none had an identical SHA-256. This is a byte comparison, not a perceptual assessment. |
| Assigned material slots | The earlier complete43-Blender-mesh import report contains no material slot name outside `M_RV_*`. The newer27-material/23-sound report is a separate surfaces/audio update; pending generated landform imports are not covered by those old mesh records. |

**Coverage limits and conditional fallbacks:** `DBEnemy.cpp:276–277` and `DBProjectile.cpp:60–61` can use the engine's BasicShapeMaterial only if the new CombatGlow material fails to load. This is not an unconditional old-game asset reference or an observed fallback. The older generic Mesh helper's engine cube fallback and `SM_StoneTile`/`SM_Wall` etc. callers are in the explicitly excluded historical world construction/verification path. The map asset itself is binary and was not opened during this source-only task; `CreateContent.py` preserves `/Game/Maps/Bellroot` rather than enumerating its actors. Therefore an eventual runtime component/asset inventory is stronger evidence against unexpected placed map actors or missing cooked resources. Packaging still includes legacy content through bCookAll/always-cook directories; inclusion is not proof it renders. The normal code-drawn HUD uses no old bitmap asset; its visual styling is outside this3D routing audit and is being revised by integration.

Later integration supersedes two of those limits: root removed the two engine material fallbacks, and actual `Saved/Reverie/map.json` records creation of `/Game/Maps/Reverie` with `preplaced_graphics: []` and complete=true. The final cooked world is still pending.

## Integrated Editor scene recheck

Independently opened the actual integration-owned Editor-game first view (ordinary first spawn, seed833283,1600×1000) and staged fountain detail. No reviewer game/GPU execution or native input. Raw reviewed images are preserved under ignored `captures/raw/reverie-qa/`.

| Artifact | Identity |
| --- | --- |
| `editor-integrated-first-193500.png` | SHA-256 `DCD3BD6DB11EBBB9780F0A89C1083E5FC3EEB0257EACE12D10B75B3395D4163A`; source image timestamp19:35:00 local |
| `editor-fountain-1936.png` | SHA-256 `AD0AA8E92197B885FFA055CD65E3F7870509FB75C797AF66003E8BBA932857F2`; staged detail supplied as17:36UTC |
| `Saved/Reverie/import.json` | SHA-256 `FC6B9B455EA773EB367C9D01FA91073D65D8C598791476EED96D5E6ED91E5FB9`; complete_kit,43 Blender meshes plus3 Nanite landmarks,27 palette materials plus3 landmark atlas materials,23 audio,0 warnings |
| `Saved/Reverie/map.json` | SHA-256 `71AA673D467EB5442EFC323CD0E3C73B5673B2DAD07ED8DF761487DED9627917`; complete=true, new Reverie map, no preplaced graphics |

The imported landmarks are FountainHero961,305 triangles, SculptedBank985,900 and CrownTree954,410, each with Nanite enabled, two UV channels,3 convex hulls and separate4096×4096 base/metal-roughness maps. These are inspected import facts, not performance evidence.

Passed visual recheck: the high-frequency stone striping is absent; the new tree has a substantially more natural root/trunk/canopy silhouette and the sky has visible cloud layering. The gateway still clearly frames a bridge/gate route. The fountain detail shows two visible water ribbons meeting bowl silhouettes and ending inside the basin in projection; no separate clear placement failure was found from this single angle. This cannot establish depth alignment from every angle or animated flow quality. Its dark upper trunk and cups obscure detail, matching integration's pending local-fill correction.

### RV-QA-05 — Medium visual: wall bottoms float above exposed soil

Artifact: integrated Editor first view identified above. Steps: inspect the left perimeter/soil edge behind the foreground tree. Expected: perimeter masonry meets or sinks into the ground. Actual: a dark open gap, including a conspicuous cyan strip, appears beneath the wall along the exposed soil. Current placement supplies soil topZ-22 while masonry startsZ0, supporting the visible22 cm clearance as the cause.

Affected file: `DBRecoveryScene.cpp`, TerrainPatch at127 and perimeter wall placements at204/211/256 in that source snapshot. Requested correction was to extend the masonry footing down to the soil while retaining wall tops/coping and doorway floors. Sent to the scene author through root; subsequent corrected-image rechecks follow.

Visual recheck — resolved: the19:47:21 corrected Editor first view, then the final packaged first view, show perimeter masonry meeting the soil. The open dark gap/cyan strip behind the tree and along the left soil edge is absent. Final packaged route/floor queries also passed as described below.

Known concurrent corrections owned by integration: soil still appears as broad flat sand rather than forest loam, and some planted patches are isolated at its edge; root is changing texture world scale/darkness and the scene author is joining edge growth. Do not run extra interim renders before those changes and the fountain fill are complete. No additional distinct fountain alignment defect is reported.

## Corrected Editor views — ready for packaged verification

Independently inspected the two correction PNGs, without launching the game/GPU or operating native UI:

| Image | SHA-256 |
| --- | --- |
| `captures/raw/reverie-qa/editor-corrected-first-194721.png` | `9853D0B3FF531CAE6B096496058E62498B431BCE3C676819312D7FA705E4B9C8` |
| `captures/raw/reverie-qa/editor-corrected-fountain-194910.png` | `1EE431D2C5BD66ECDC0B501F60F5389A0F992736156DD6A04FD30C2347DF21D3` |

Passed visual corrections: wall/soil contact (RV-QA-05); soil now has litter/loam texture instead of broad featureless sand; edge plants connect into drifts; stone striping remains absent (RV-QA-03). No additional clear water/cup placement defect appears in the corrected detail. The fountain's upper crown/trunk still lose dark carved detail, while bowl edges and streams remain readable. Record that as remaining lighting polish, not a blocker to packaging and focused runtime verification.

The first view now visibly demonstrates the new environment set and more natural landmarks. This is a static presentation assessment of one seed, not a claim of matching anchor B, responsive gameplay, natural structure at every seed, or owner acceptance.

## Final Shipping evidence and bounded native check

The reviewer independently opened `evidence/reverie-courtyard.png`, the actual final packaged ordinary-spawn capture supplied at18:05:53UTC, seed833283. SHA-256 `D88B95D7D52B9760FD6F48C84C2E57126E6E6E1AEDB025AA5521BBCBB9DE9E6D`. The new art and earlier stone/footing corrections remain visible; no recurrence of the reported striping or cyan wall gap. This static camera view is separate from native menu entry and does not establish gameplay or performance.

The reviewer also inspected the integration owner's actual `evidence/reverie-art-checks.json`, SHA-256 `4B1B950BFC6BEC9802C52C77D3A6B79DEDBF7E67679B91A1FC8A6F01ECD2CC11`: Windows Shipping, Unreal5.8.2, NullRHI, isolated slotDreamboundQA_Reverie, seed833283, complete=true, aborted=false,2 groups/11 observations/0 failures. Coverage includes same-seed reproduction/nearby variation, three capsule lanes per connector,102 floor probes, corrected closed/open gate sweeps at0/±270 cm, enclosing walls,26 actual scene mesh families including two CrownTrees per room with no retired tree/rock/cliff instances, exactly four useful arches/two bridges,30 reserved-pad probes and16/16 successive20 cm stair treads. QA journals were restored. This is an integration-executed engine fixture independently reviewed here, not a second QA execution or native playthrough.

Independent native procedure used the exact Shipping executable above with `-DBSaveSlot=DreamboundQA_ReverieInput -DBSeed=833283 -windowed -ResX=1440 -ResY=900 -ForceRes`. No pre-existing files matched that isolated profile before launch; the regular owner profile was not used. Game PID21580 and returned Sky window6949116 identified this session. Ordinary Enter creates a fresh run: the actual gameplay HUD reported **expedition1, seed552389**, rather than the833283 title-world seed. Record the observed seed instead of claiming deterministic native play on833283.

| Check | Result and boundary |
| --- | --- |
| Packaged startup | **Passed, independent native observation.** Title displayed `SANCTUARY / REVERIE 0.4.0`, Enter the courtyard and Exit; new world rendered behind it, with weapon hidden. |
| Ordinary menu entry | **Passed, independent native input/observation.** Clicking Enter opened the spring cloister, health100, six held pieces, folded weapon and ward instructions. This was normal menu entry on the isolated profile. |
| Forward movement | **Inconclusive.** One Sky `press_key('w')` tap was issued. Its immediate image did not establish player displacement; no held movement or traversal pass. |
| Look | **Not verified under controlled input.** The camera changed substantially between observations while the tool reported outside/user activity. The reviewer cannot attribute that movement to its own test or call it a controlled pass. |
| RMB unfold/refold | **Blocked by input-control interruption.** First RMB action returned `user input was detected in this window; call get_window_state before continuing`. Reviewer refreshed the window, then a second RMB attempt returned the same message. No guard transition pass is claimed. |
| Pause, sound +/−, mute/enable | **Not run/verified.** Further injection stopped after the repeated user-input detection. Neither visible settings response nor actual audio effect was established. |
| Settings persistence, exit/relaunch | **Not run.** Integration explicitly instructed the reviewer not to close/relaunch or inject further input after the interruption and to leave the final isolated demo open for the owner. |
| Audio quality/mix | **Not verified.** No supported auditory observation was obtained. Sound assets/import settings and earlier signal measurements do not prove perceived strength, harshness or spatial playback. |
| Combat, success/failure, restart, full ordinary route, sustained guard, performance/other hardware | **Not run in this bounded final native task.** No full combat regression or repeated render suite was added. |

After the second input-control rejection, reviewer returned GPU/native UI ownership, performed no further UI calls or inputs, and left the isolated demo running as instructed. Detected activity is a coverage interruption, not evidence of a production input defect or proof of a human playtest. The owner has not supplied a new enjoyment/visual-acceptance assessment here.

Readiness: final package opens, enters play and passes the focused integration-run geometry checks; earlier actionable geometry/material/import defects are corrected within the recorded coverage. It is available for owner review, with guard/pause/volume/persistence/exit and ordinary gameplay unverified because native control was interrupted. Dark upper fountain detail remains minor visual polish. No fun, sound-quality, performance or anchor-B acceptance claim.
