# Living-world update — independent QA

2026-09-10. Owner scope: grounded Shift sprint with stamina, a separate forceful Left Alt dash, organic enemies and richer scenery. This report initially covers the movement increment; the previous Reverie reports remain historical evidence.

## Movement harness and coverage

The initial source review used base `85b73d919992ba947ea74e1bddcd90656b908368` plus the then-uncommitted movement implementation and QA additions. QA owns only the movement extension in `unreal/Source/Dreambound/DBShieldChecks.cpp/.h` and this report. The integration owner owns compilation, imports and packaged execution. Actual packaged evidence and the first-run harness defects are recorded below.

The new `-DBMovementCheck` branch uses the existing isolated save backup/restore and finish path. It bypasses the legacy combat and route cases. An ordinary ticking character settles onto a real collision floor before public `PressSprint`, `ReleaseSprint`, `Dash`, `ReleaseDash` and `AddMovementInput` calls. The full stamina run has an uninterrupted runway; actual actor displacement and swept wall contact supply the movement evidence. The fixture does not tick the world manually, set test velocities, or teleport during a measured phase.

The harness covers ten behavior observations plus the four existing isolation observations:

- Stationary Shift retains stamina; actual walking settles near 590 cm/s.
- A full stamina sprint physically covers roughly 44 metres, reaches roughly 885 cm/s and exhausts in about five seconds.
- Recovery starts after roughly 0.8 seconds, reaches roughly 25 stamina one second later and does not automatically restart a held exhausted sprint.
- A fresh press above 20 stamina produces sprint movement; a fresh press below 20 remains walking.
- Holding sprint/movement against the solid wall stops draining stamina after contact.
- Open-floor dash covers approximately 4.44 metres with no vertical hop or stamina cost.
- Cooldown rejects an early press; continued held calls do not retrigger after cooldown; release and a new press rearm dash.
- Dash stops the capsule at the physical wall without passing through or hopping.
- Pause cancels an active moving dash and freezes position/resources; resume has no stale movement input.
- Run reset during a moving dash clears its motion, restores resources and clears held input state.

Expected duration is about 16–20 seconds of ordinary world ticks, with a 24-second tick budget and 45-second wall-time abort. The result is `Saved/QA/movement-checks.json` with suite `grounded-movement-v1`. Required flags: `-NullRHI -DBVerify -DBMovementCheck -DBSaveSlot=DreamboundQA_LivingMovement -DBSeed=552389 -unattended -nosound`. Use the real Shipping binary rather than its returning bootstrap. Require complete=true, aborted=false and failed=0, then inspect each measurement and the journal-restore observation.

## First packaged run and harness defects

Root executed Windows Shipping **0.5.0-living1**, source `29743b142c8666ddd365ba5154fedbcd606dd372`, UE 5.8.2-56702186. This reviewer independently read `.local/living1-movement/Saved/QA/movement-checks.json`: complete=true, aborted=false, 2 passed / 2 failed groups, **11 passed / 3 failed observations**, 15.533 tick seconds and 3.926 wall seconds. These durations are an unattended NullRHI fixture, not a performance result. Original JSON SHA-256: `07745E0D2CB3F52736D12822E905B4F0BB327F137E7238C0C5BB4E3016FE5E06`.

**LW-QA-01 — medium, harness measurement defect; corrected and rechecked in the packaged run below.** Reproduce with the movement flags above on this package. The sprint actually covered 4414.2cm in 5.000s and the dash 444.19cm in 0.240s with zero vertical offset, but peak measurements reported only 442.5 and 925.0cm/s. `DBShieldChecks.cpp::TickMovement` divided each displacement by `Max(DeltaSeconds, 0.001)`, which understates speed for ticks below 1ms. The exact half-sized results are consistent with 0.5ms ticks; the first report did not retain per-tick deltas. The fresh-sprint observation also depends on this corrupted peak measurement. Expected: measured speed uses the actual positive tick interval. The correction removes the artificial denominator floor and adds minimum/maximum tick and sample-count diagnostics. All physical speed, distance and time acceptance bounds remain unchanged.

**LW-QA-02 — medium, premature harness phase transition; corrected and rechecked in the packaged run below.** In the same run the depletion phase ended with 0.007 stamina still present, because it advanced at <=0.01 while separately requiring `IsSprinting()` to have ended. Expected: sample the true zero-stamina exhaustion transition. The runner now waits for zero stamina and the sprint ending; it does not label a nearly empty, still-sprinting bar as exhausted. No production movement code was changed for either correction.

The first result independently supports 590cm/s walking, 0.809s first observed recovery, 25 stamina at 1.8s, no automatic held-sprint restart, no wall-contact drain, cooldown/release behavior, and zero-drift pause/reset cleanup. The wall dash stopped at capsule X292.900 against the expected X293.000 limit with no hop. The three first-run failures remain historical evidence; their corrected observations are recorded separately below.

## Corrected packaged movement result

Root executed the corrected Windows Shipping binary from source **`ecab7e21654d55ac30afa50a2556b6acb418aab8`** with executable SHA-256 **`FC70194B45B79DDC0E96F99DF7DF5B920CDE89675DADD674B300F38158A6FFA5`**. `.local/living1-movement-final-run.json` records that identity, exit code 0 and the movement flags above plus `-UserDir=D:/Game-Studio/.local/living1-movement-final`.

This reviewer independently read the actual result and copied it unchanged to [living-movement-checks.json](living-movement-checks.json), verifying the copy's SHA-256: **`97009CEBCD444676FE819957B498B432FAB3648252B67389B94C105E24F0C7AA`**. Result: complete=true, aborted=false, **4 passed groups / 14 passed observations / 0 failures**, including exact isolated-journal restoration. Execution belongs to root; this reviewer did not rerun the game.

The 31,049 observed ticks span 15.535 tick seconds. Minimum tick 0.000500 seconds confirms the sub-millisecond sampling condition; maximum tick was 0.013431 seconds. Measured sprint reached 885.0cm/s and covered 4414.7cm in 5.000s, ending at actual zero stamina. Fresh sprint after recovery now passes, while the second press at 16.00 stamina stays walking. Dash reached 1850.0cm/s and covered 444.19cm in 0.240s with zero vertical offset and no stamina cost. The physical wall, cooldown/held-input, pause and reset observations also pass. Both harness defects are resolved without relaxing the movement acceptance bounds.

This passes the bounded flat-floor movement fixture for the recorded binary. It does not certify native key handling, varied terrain, combat immunity, player feel or later binaries. Any subsequent enemy/fixture-only package difference is tracked by the integration owner; this evidence is not silently relabeled as a new execution.

## Packaged posed visual review

Independently opened two root-captured 1600 × 1000 Shipping stills from the same `29743b1` package. Both show expedition 1 / seed 552389 in the Spring Cloister:

- `.local/living1-creatures/Saved/Screenshots/Windows/Living_Creatures.png`, file timestamp 2026-09-10 22:38:13 +02:00; SHA-256 `FC1D3ACD89EB037B6CF8D50A215E352EA082EFFABE0F50DE21A36DCE28654800`.
- `.local/living1-habitat/Saved/Screenshots/Windows/Living_Habitat.png`, file timestamp 2026-09-10 22:38:28 +02:00; SHA-256 `BA8F37D320A80A53E3C7BEF5D92D3F34E232F37B34EF8C0483BEEF4FD04BDDEA`.

The two creatures have continuous organic bodies, distinct broad/slender silhouettes and forward-facing heads. Their visible feet meet the paving; no detached-part or floating-foot defect is apparent in these poses. The preferred cream shield remains visible. The habitat view shows fern/reed/flower growth and a fungus-covered log emerging from the soil, with small pads inside the fountain basin; no concrete root/support gap is visible from this angle. Paving/basin bases still meet their visible supports.

Dark creature torsos/faces and parts of the fountain obscure some surface detail. This remains a visible readability limitation, already noted by root, rather than evidence that the new organic assets failed to load. No new blocking presentation defect was identified in these two stills. They are explicitly posed captures: gait, turning, attack readability, dynamic foot contact, scenery collision, ordinary spawn composition and owner visual acceptance are unverified here.

After the still review, root reported a floating **practice target** in the earlier motion capture `Frame_00092.png`. The grounded posed Room99 creature views above did not cover that case. The correction was subsequently checked against actual practice-target capture frames as recorded below.

## Corrected practice-target breathing contact

Root's final Windows Shipping capture uses source **`877be8e120fa89bdbd345f73f245b0cf031214de`**, executable SHA-256 **`FE8D1DB61568A541B7CBEA6A7813686838C54EC25B4FF00D6092DA94F9FE36B6`**. [living-motion-audio.json](living-motion-audio.json) records this identity, exit code 0, 302 captured 1280 × 800 frames and the actual scripted capture arguments. Root reports that `DBEnemy` now converts visual pose translation through the inverse full parent transform, accounting for the imported 100× unit scale; movement, collision and attack code were unchanged by that correction.

This reviewer independently opened these two frames from `.local/living1-audition-grounded/Saved/CombatFeelCapture`:

- `Frame_00022.png` at **2.28666s**, SHA-256 `0AE759A23820F2757DC669F9D8BCD1EA27CD4584A689BA793E67CDFABAD3593C`.
- `Frame_00094.png` at **10.9774s**, SHA-256 `BFC519CC05D853DBB55ED8099953D2ED7C75057FB562DDD4FCF5FF0DAA2DB04A`.

**The reported practice-contact correction is supported in both sampled breathing phases.** The central practice target's feet meet the paving in both frames, with no visible large floating gap or deep sinking. The visible feet of the right-hand target also meet the paving; its outer side is partly cropped. The expanded shield in the first frame partly overlaps the central target's left side, while the later folded/charging view exposes more of its silhouette. No gross body rise or sink is apparent between the two samples.

This resolves the pending contact note for these captured practice poses. It does not establish every intervening animation frame, walking/turning/attack foot contact, slopes or other enemy placements. The linked [scripted capture](living-world-scripted.mp4) is root's staged invulnerable-target demonstration; this reviewer inspected the two extracted frames and metadata, not the complete video or audio. No subjective audio-quality claim is made. The 14-observation movement result remains tied to `ecab7e2`; it was not rerun or relabeled for this later visual-pose correction.

## Static review and limits

No confirmed production defect was found in the reviewed movement paths. Sprint drain uses measured displacement, exhaustion requires a new Shift press, the root-motion dash is horizontal, and menu/reset paths remove its source and clear movement input. The dash's 0.13-second evasion immunity is separately stored from the retained Ram `DashTime`; this was inspected in source and is not a tested immunity result. The current input file maps Sprint to LeftShift and Dash to LeftAlt; it was read only and left unchanged.

Harness whitespace validation (`git diff --check` on the two owned source files) passed, including the sampling/exhaustion corrections. The corrected root-executed package passes the focused movement fixture as recorded above. Native keys, input focus, ordinary player traversal, slopes/stairs/ledges, combat/immunity, audio quality, performance and fun are outside this harness. Visual coverage comprises the two posed stills and two corrected practice-target breathing frames above; broader animation contact remains unverified. No native input, owner-process action, game launch or GPU work was performed by this reviewer.
