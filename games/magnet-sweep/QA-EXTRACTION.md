# Extraction review — 12 September 2026

Independent static acceptance review for the Extraction 0.5.0 candidate, plus inspection of root's completed automation report. This is **not native gameplay verification**. Following the owner's physical Escape during the earlier Clarity pass, all native control remained stopped: no window inspection, input or game launch occurred in this assignment. The current Clarity game and its saves remain untouched; no owner profile was read or edited. Root owns packaging and the final artifact record.

## Evidence inspected

Reviewed runtime, HUD, teaching, tutorial state, persistence and `TutorialTests.cpp` changes against the five-case extraction contract. The model specialist owns implementation and deeper model verification; this review did not change source or run builds/tests.

Read `unreal/Saved/Automation/index.json`, report `2026.09.12-21.46.20`: **21 Success, 0 failed, 0 succeeded with warnings, 0 not run and 0 in progress**. Every listed test had zero warnings/errors. Report SHA256 at inspection: `8BB1B5FA3F4EB98A80F63C7C3F0FB6890B96023C14CAB20634A0BE7421D1DBA9`. The report is an ignored working artifact and may be replaced by a later run.

The successful suite includes:

- `MagnetSweep.Rework.BreakawaySelectionAndAtomicRefusals`
- `MagnetSweep.Rework.BreakawayTiersRefillAndSavedState`
- `MagnetSweep.Tutorial.ExtractionInputAndEarnedUse`
- `MagnetSweep.Tutorial.ExtractionSaveContinuity`

The remaining successful tests cover existing capacity, risk, payout, career, persistence and tutorial rules. Runtime tests call handlers directly; their names do not establish actual hardware key dispatch or visual clarity.

## Independent findings and source corrections

| Finding | Corrected behavior inspected |
|---|---|
| A radius increase alone did not establish a new accomplishment. | The coil now severs and captures one explicitly selected available linked piece. The remainder stays on the tray. Selection uses the same target finder in the F handler and tooltip. |
| Broad attraction could immediately collect the unwanted remainder. | Successful extraction clears the field latch, current drag and sweep action, and stops residual piece velocities. Old mouse release cannot restart the field. |
| An immediate smelt could hide the extraction transfer. | The selected piece has a 0.55-second visual transfer and `RecoveryTimer` holds pending smelting through it. This is source/test evidence, not an observed animation. |
| **F → E → RMB left a deferred-smelt loop.** Empty cargo made the queued request fail every frame; later collection could be auto-smelted. | Whole-haul drop clears both pending deposit/finish flags. Invalid and held-risk deposits also clear them. A regression covers cancellation, retained fuel and no subsequent automatic deposit. |
| Any ordinary pickup could falsely complete the coil-use lesson. | Coil `TryUpgrade` requires a successful useful recovery marked `UsedBreakaway`. Other purchased-mod paths retain their ordinary useful-capture behavior. No extraction or capture itself awards wallet/XP. |
| Coil purchase text replaced the danger explanation. | Coil purchase and teaching now preserve the first-warning pause or real-fuse explanation when the training guard ends. |
| The whole bundle's red incoming mass could be mistaken for the F result. | Tooltip separates **Normal pull** from the selected piece's mass/value and resulting haul. It also reports waiting during a transfer or pending smelt. Actual readability remains untested. |
| Precision's narrow field could be confused with extraction range; empty-haul fallback suggested an unnecessary pour. | Wording now says extraction reach. With no linked pieces, quota met and no cargo, guidance says to finish the order and choose the next. Ordinary earning remains available. |
| Reload or drop could refill charges or restore links. | JSON persists the spent-use counter and link state. Missing legacy counters initialize without resetting the career; malformed counters reject transactionally. The successful continuity test covers a spent extraction after a drop. |

No unresolved source-level blocker was identified in the reviewed paths after these corrections. That conclusion does not establish fun or successful packaged interaction.

## Focused native acceptance route — pending

Only perform this after a new safe foreground handoff, in an explicitly isolated test career. Do not reset or repurpose the current owner-controlled Clarity state.

1. **Before/after accomplishment:** with the training guard off and an earned extraction coil, compare normal attraction against F from a 20 kg haul and 24 kg safe limit. Find the 8 kg mixed bundle containing a 4 kg alloy and two 2 kg irons. Ordinary attraction captures the whole bundle into an unsafe 28 kg haul; it is not an impossible pickup. Rebuild the same 20 kg situation through normal play, point at the alloy and press F: only that piece should move into cargo, leaving a safe 24 kg haul and both irons available. Verify the preview matches what F actually selects.
2. **Input and transfer:** test Q precision, target range, hovering UI/empty space, held attraction and old mouse release. Invalid F must spend nothing. Observe the 0.55-second motion, visible severing and cessation of broad attraction; press E during transfer and confirm the pour waits.
3. **Cancellation:** F, immediately E, then RMB before the transfer finishes. The haul should return recoverably, pending smelt should cancel, fuel/payment should remain unchanged, and later collection must not auto-smelt or leave F blocked.
4. **Charges and saves:** at earned coil tiers 1/2/3, verify exactly that many successful extractions between real smelts. Drop and save/quit/resume must preserve spent charges and severed links. Empty/unsafe smelt must not recharge; a valid smelt recharges once and consumes normal fuel.
5. **Real reward and teaching:** compare carried value against wallet/XP before extraction, after extraction and after smelting. Only smelting pays or banks a collectible. Verify the actual first purchase's guard transition, real extraction requirement and no-linked-piece fallback without blocking normal payout.
6. **Existing-career continuation:** use an authorized isolated copy of an older earned-coil save, without Fresh/reset. Check preserved money, upgrades, cargo and tutorial state, discoverability of F, then actual extraction and smelt. No owner-save continuation was verified here.

Actual F dispatch, target readability, extraction feel, guard-transition presentation, save/reopen behavior and owner-save continuation remain native gaps. No listening judgment was made. The useful player question is: **Can the player explain and demonstrate what this upgrade lets their current haul accomplish, without being told the exact pickup route?**
