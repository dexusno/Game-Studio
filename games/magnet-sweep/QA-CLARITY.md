# Clarity native QA — 12 September 2026

Partial independent native check of Windows Clarity v0.4.0. The reported iron-only payout obstruction is fixed in the tested path. Testing stopped immediately when Sky reported physical Escape from the user. The live `qa_clarity` session may now contain owner play: preserve its game, save and current progress. It was initially launched for isolated QA, but must not now be treated as disposable. No native input, window inspection, closing, resetting or process launch followed the stop.

The owner's rejection of the previous tutorial is the reason for this check. The earlier coached walkthrough did not establish novice comprehension or enjoyment. This pass deliberately ignored the suggested pickup route and tested the owner's reported behavior; it remains incomplete.

## Artifact and method

- Package: `BuildOutput/Clarity/Windows/MagnetSweep.exe`, version 0.4.0; a separate package from the previous Tutorial/Rework archives.
- Child executable SHA256: `49C9E253F2446C761D6B38F4AD75B558BA6EFDCB81D5F8BC47ACFC1BC3E036DE`.
- PAK SHA256: `C045D1E4E165D5EEFFB3BE8F64D7C182922B672BBCAE4B80C9607A8F944BA5D0`.
- Root reported successful build/cook in 40.01 seconds and 17 passing automated tests with zero failures/warnings, report 19:41:26 UTC. These are separate from the native observations below.
- Root launched with `-DemoProfile=qa_clarity -DemoQA -DemoFresh`. Native actions used documented Sky clicks, drags and player-facing Space attraction toggle. No save edits, manufactured earnings, console commands or slow motion were used.
- Observed client resolution: 1600×900. F9 produced the three game-only PNGs listed below. No auditory listening was performed.

## Observed before the stop

| Scenario | Actual observation and limit |
|---|---|
| Welcome and goal | Welcome text wrapped without visible clipping. It explained the first 150-credit upgrade, collecting and smelting, accepted mixed metals, whole-haul RMB drop, the temporary guard, and the larger rig/collection objective. The tray distinguished carried value from banked credits and displayed the 150-credit target. This establishes visible presentation, not player understanding. |
| Iron-only first haul | Ignoring the suggested copper route, one isolated iron produced a 2 kg / 4 cr haul. Banked credits remained 0 and the visible **SMELT HAUL +4** button enabled. Clicking it immediately paid 4 credits and 4 XP, emptied cargo, recorded 4/120 toward the order and consumed one fuel charge. No bundle or lesson-order prerequisite prevented payment. |
| Careless collection | Sustained attraction and broad drags through crowded iron grew the haul through 14, 18 and 22 kg to exactly 24 kg / 48 cr. Attraction automatically stopped at capacity. The next piece displayed a training-guard instruction to smelt the current load. No overload was observed. |
| Full haul over red cells | The full 24 kg haul was dragged into the red-cell pocket and attraction switched on again. After more than three seconds it remained at 24/24 kg and 48 cr, safe; the red cell remained on the tray. Its tooltip explicitly said the training guard leaves it there. Because the haul was already full, this does **not** independently prove cell exclusion when spare capacity exists. |
| Whole-haul RMB drop | RMB produced the visible toast that fuel and salvage were saved and everything returned to the tray. Carried value cleared, the smelt button disabled, and the twelve iron pieces appeared in the spill animation. Wallet remained 4 and three fuel charges remained. This is the immediate post-input frame only: settled placement, complete recoverability and final field-off readback were not checked. |
| External interruption | On the next read-only window-state request, Sky returned: “Computer Use was stopped by the user with the physical Escape key.” Testing stopped immediately. No later game or save state is attributed to QA. Root was notified and foreground returned without closing the game. |

## Evidence

- [Welcome](evidence/clarity-welcome.png): initial goals, controls and training-guard explanation.
- [Iron-only smelt offer](evidence/clarity-iron-smelt-offer.png): 2 kg / 4 cr carried, separate empty wallet and enabled smelt button. The subsequent payout was observed natively but is not what this PNG depicts.
- [Guarded full haul](evidence/clarity-guarded-full-haul.png): full 24 kg haul at the cell pocket, safe state and explicit red-cell guard tooltip.

## Pending acceptance checks

- With spare capacity, attract across a red cell and adjacent valuable metal: useful salvage must collect while the cell remains excluded under the labelled guard.
- Smelt a stable mixed haul directly, without following the suggested route. Mixed materials must not cause a refusal.
- Start a tray drag, trigger the automatic training stop during that same held gesture, and release over the furnace or enabled smelt button. It must pay once. The supported atomic Sky drag has no configurable hold duration; separate drags after automatic stopping do not prove this exact gesture.
- Recollect the dropped load and confirm settled, recoverable placement, attraction off and no loss. The immediate animation alone is insufficient.
- Earn and buy the first upgrade; verify the guard-off warning appears at the actual purchase and the next objective is clear. The displayed 150-credit goal does not prove this transition.
- Load the prepared `qa_clarity_legacy` copy of the owner's earlier Bundle save without Fresh or reset, then earn a payout. This was not launched or tested natively. Root recorded the original backup SHA256 as `E8A42A0389AF5ABE363FC3A02AE6A0A6BFF195E9CD87D7B20AAE747E152A3775`; no post-stop source-backup verification is claimed here.
- Observe an unfamiliar player explain carried value versus wallet, why a haul costs one fuel to smelt, what RMB does, and when protection ends. Layout inspection and source-informed QA cannot answer that comprehension question.

No functional failure was observed in the completed checks. The limited fuel consequence of repeated tiny accepted hauls, later risk teaching, save/resume, alternate resolutions, upgrade progression and remaining novice routes were not validated in this pass. The smallest useful follow-up is the pending mixed-haul/legacy payout and actual purchase checks, only after a new safe foreground handoff; current owner control and progress must remain untouched.
