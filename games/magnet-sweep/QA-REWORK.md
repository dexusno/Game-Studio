# Magnet Sweep rework — independent gameplay QA

2026-09-12. Controlled R1 and partial R2 native review finished. R2 input was handed to the owner after new live input was detected. The game remains open; current R2 progress is preserved. Owns this QA record only. Root verified the temporary F10 Input.ini is absent; guarded cleanup changed nothing.

## Scope and evidence boundary

Review the actual Windows rework after its author inspects the first scene. The reviewer did not implement gameplay/model/HUD code. The reviewer did create the audio assets, so audio integration/mixing is checked by root; no independent listening claim is made. This runtime explicitly rejects audio input to the model.

Read the current SalvageModel.h, WorkbenchRuntime.cpp, WorkbenchHUD.cpp and relevant persistence code. The initial R1 implementation had seven passing model tests; the final R2 report has nine passing tests, zero failures and zero warnings. This is automation evidence supplied by the implementer, separate from the native observations below.

Package: `BuildOutput/Rework/Windows/`. Root designated the **historical R1 `player` profile as disposable QA state** and it was reset through UI at R1 close. R2 began with fresh QA state, but later live input changed the handoff: **current R2 `player` state may now contain owner progress and must be preserved**. Root explicitly prohibited further native input/reset/quit. The earlier owner's separate ConceptDemo profile was untouched. No fabricated progression or save editing was used to claim native completion.

R1 package launched through Sky: `BuildOutput/Rework/Windows/MagnetSweep/Binaries/Win64/MagnetSweep.exe`, Development PCD3D_SM5, 1600 × 900 client on Windows. Child EXE SHA256 `7F9324A182A3ED6EE626053F209DA19325B48D9219CD96AF4A26AA8D25A21DDC`; PAK SHA256 `E122CB435B27605FB1E35584C829B6DEA6AA5513047B0A7EE9E0A9E41D160754`. R1 predates the final R2 risk correction and does **not** consume furnace fuel on an overload.

## Executed R1 observations

- Sustained Space field visibly moved two iron pieces before capturing 4 kg / 8 cr. Furnace click spent one heat, awarded 8 cr and 8 XP, arced the pieces into the bowl and produced an ingot. The visible movement is native evidence; the reviewer cannot listen to the audio channel.
- **Risk loophole reproduced:** with empty cargo and Q precision, capture right hot cell ID35 and leave its fuse to expire. Cell becomes Lost, while banked/wallet/XP remain 8 and heats used remains 1. This safely deletes the obstacle at no economic cost. Root has accepted a later one-heat emergency-quench correction; do not count that fix as tested in R1.
- Narrow field then isolated the Amber Dynamo from the remaining lower hot cell. Its 40-cr deposit brought banked/wallet/XP to 48; collection saved core ID0.
- A three-ring copper bundle displayed `3 pieces / 9 kg / 36 cr` before capture. Two such groups plus one loose ring made a safe 21 kg / 84 cr haul. Smelting completed the first 120-cr quota with banked132, wallet212, XP212/rank2 and one heat left. Exact arithmetic: 8 + 40 + 84 salvage + 80 completion reward. This run included the earlier free hazard-destruction probe, so it proves native accounting/progression rather than a clean no-failure first run.
- The post-quota tray remains playable and visibly offers Finish order or a 180-cr gold chase. No forced interruption cut off the melt.
- Finish order opened the workshop. One 150-cr Deep Basket purchase changed wallet212→62 and tier0→1/capacity24→32; the next tier correctly required rank3. Copper Knot (180-cr quota) was available, later jobs remained rank-gated. Its tray had a different material mix and a cell beside a linked bundle.
- **Actual save/quit/relaunch passed:** on Copper Knot, held alloy ID40 (4 kg / 24 cr), wallet62, XP212, capacity tier1, core collection[0], four heats. Changed music30→55%, effects80→100%, masterOFF; quit through Save & quit and verified the game window disappeared. Relaunched the exact same child EXE: menu settings and the same cargo/career were restored. Save JSON matched these values. Pausing stopped the latched field; resume required fresh attraction input.
- **Valuable overload executed:** a broad pull gathered the Copper Heart and two adjacent hot cells with an existing alloy haul. Expiry destroyed exactly the40-cr core and cells, while alloy and washer scattered back recoverably. Wallet62/XP212/equipment/core0 remained safe; R1 spent no heat. A purported fast RMB probe used an incorrect Sky argument initially and is excluded from vent evidence. Repeated observation/action round trips also exceeded the3-second fuse, so timed rescue is not yet covered.
- Retry confirmation stated the consequence clearly. Confirming restocked Copper Knot with four heats while preserving wallet62, XP212, equipment and recovered core. No save data was fabricated.
- F11 switched the actual scene to2560×1440 borderless and back to1600×900 windowed; HUD remained visible and aligned. Activating the already-open Calculator paused the game; returning foreground left it paused. No Calculator content was changed. Escape successfully resumed the game on a later focused test.

### Current gameplay judgment

Precision pickup can actually protect a selected haul: it isolated the rare core and each copper bundle, whereas the broad field pulled the hazards beside the core. R1's hazard-first/cheapest-value-per-kg vent nevertheless performs the valuable selection automatically. Replacing it with a fully recoverable emergency drop is recommended to give deliberate extraction a purpose, provided spilled cargo remains nearby/readable and the field switches off. This recommendation combines observed extraction behavior and rule analysis; **successful R1 auto-vent dominance was not executed**. Reaction tests must not confuse tool latency with a player's difficulty.

## Native input capability researched

Read the computer-use skill plus its complete guidance/API/confirmation documents. Initialized `@oai/sky` without selecting, activating or sending input to any application.

The documented Sky API supports click, keyboard chord and atomic drag. `DragInput` exposes only window/screenshot and from/to coordinates: **no duration, separate button-down/up, hold API or held modifier state**. The key API is likewise an atomic chord. Continuous held-field and simultaneous Shift/mouse coverage may therefore be limited. Measure actual drag behavior against runtime telemetry rather than assuming it gives a sustained field. Do not invent unsupported calls or report an atomic action as a human-length hold.

R1 included actual player-facing **Space field toggle and Q precision toggle**, retaining LMB/Shift holds. Both toggles were used successfully for sustained native physics. Hold-specific behavior remains uncovered. Root authorized visible Development-console `slomo 0.1` for R2 if available; this would be explicitly limited to slow-motion functional rescue coverage, not normal reaction or human-feel evidence.

## Native coverage at R1 handoff

| Check | Result |
| --- | --- |
| Start/readability and actual attraction | Observed; distinct iron/copper/alloy/cell/core silhouettes and readable linked-group forecast. |
| Capacity24 / hard36 boundary | Pending R2 boundary probe; safe21 kg and purchased32 kg observed. |
| Hot-cell overload | Executed R1 exact highest-value loss and recoverable remainder. R2 one-fuel penalty pending. |
| Field-off fuse, timed rescue, furnace refusal | Pending R2; three-second reaction budget exceeded native tool round trips. |
| Four tiny pours / final-charge failure | Pending R2. |
| Quota, cash, XP, upgrade, gallery, next job | Passed native sequence documented above. |
| Retry, save/quit/relaunch, settings | Passed with real earned career and live unbanked alloy. Loose-piece save geometry visually matched; no exhaustive per-coordinate comparison. |
| Focus / F11 / modal input | Focus paused; returning stayed paused. F11 round trip passed. Tested tray targets stayed interactive; no exhaustive HUD overlap sweep. |
| Audio | No listening capability. Root owns actual mixing/trigger review. |

R1 ended with **New career confirmed through game UI**, followed by Save & quit and no remaining game window. Designated Rework QA save: wallet0, XP0, cargo0, heats0, tiers[0,0,0], cores[]. Master sound restoredON; persistence-test music55%/effects100% remain and were reported to root for default restoration. Owner's separate ConceptDemo save was not touched.

The unresolved gameplay question is whether extracting, correcting and smelting remains interesting after the first upgrade. Agent taste, compilation, screenshots and source rules cannot establish voluntary replay or auditory comfort; the report does identify concrete working interactions and rule weaknesses without treating those as proof of fun.

## R2 focused execution — 2026-09-12

R2 child EXE SHA256 `0848A376445EA7DAD0BA6DE1A4F0ED2254A9895CF68ADCEAC8E450F9A183BE80`; PAK SHA256 `E411209A92E5A3E8AC868C94A6E8F96A0E732EF5BF589C02B9B268177749F027`. Same native launch path as R1. Root reports nine headless model tests passing (report10:19:45) and successful build/cook/stage; separate from this native coverage.

- Clean career/default music30%, effects80%, masterON observed. F9 captured the clean board at `BuildOutput/Rework/Windows/MagnetSweep/Saved/Screenshots/MagnetSweepRework00001.png` (12:22:34 local,1,794,553bytes). Actual R2 log has no earlier missing AmbientCubemap/TextureCube warning.
- A controlled broad pull gathered alloy4kg/24cr plus copper3kg/12cr. Saved/relaunched with that real7kg/36cr haul. Moved it to the upper-right tray corner and RMB dropped **both** pieces; cargo became0, all four fuel charges remained, no pieces became Lost. Both scrap silhouettes settled visibly inside the corner. Field was off. This proves safe mixed-haul drop/conservation; it does not yet prove a timed hazardous rescue or subsequent precision recovery.
- First R2 stop logged `music_playing_observed=1 field_audio_observed=1 warning_audio_observed=0 smelt_audio=0 reward_audio=0 overload_audio=0 sounds_loaded=19`, frame-average13.34ms. This is instrumented activation/loading evidence, not listening or a performance guarantee.
- Console `grave`/literal`~` did not show a visible console; `asciitilde` was unsupported by Sky. Root authorized one temporary ignored `Saved/Config/Windows/Input.ini` containing `[/Script/Engine.InputSettings]` and `+ConsoleKeys=F10`. Added only after game quit; relaunched. F10 also did not show a visible console. **No slomo command was entered.** Root subsequently verified that exact file was already absent on12September; guarded cleanup changed nothing. The override was not written elsewhere.
- **Native input ended for owner handoff:** Sky rejected Resume with `user input was detected in this window; call get_window_state before continuing`. A read-only refresh showed an independently changed tray,35/24kg warning and3fuel; the last controlled QA state had been emptycargo/fourfuel after the corner drop. Stopped all native input and notified root. The changed state is not attributed to this reviewer's executed test sequence. Root explicitly directed preservation of the open game and potential owner progress; no reset, quit or further native action was sent.

### Final coverage boundary and next check

Controlled native evidence supports continuous attraction, readable group previews, real quota/cash/XP/upgrade/loot progression, save/quit/resume, retry, focus/display behavior (R1), and whole-haul corner-drop conservation plus cooked ambient/audio loading (R2). No controlled functional blocker was found in those paths.

**Not independently completed in R2:** hazardous mixed-cargo drop before the short fuse, precision recovery from that spill, field-off fuse persistence, refusal of an unstable furnace click, exact one-fuel quench including a lone cell, final-charge failure receipt, four tiny pours, and explicit24/36kg boundary behavior. Nine passing model tests are implementer-supplied evidence for rules; they do not replace these native/UI checks. The temporary console attempt did not create slow-motion coverage.

The smallest useful follow-up is a coordinated native risk pass on a separate disposable profile after the owner's play session, preserving the current career. Root verified the QA Input.ini is absent; the live game/save remains untouched. Owner observations should determine whether rescue/recollection feels fair and whether another contract is inviting; no auditory listening, fun or replay endorsement is inferred from this review.
