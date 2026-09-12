# Tutorial native QA — 12 September 2026

Independent native gameplay QA of the isolated Windows Tutorial archive, version 0.3.0. T1 completed the actual guided loop without a blocking input, layout, or progression failure. T2 verified restart persistence and the corrected smelting, warning and quota-marker presentation. Testing used only the disposable `qa_tutorial` career and its separate practice save; the owner's earlier Rework package and career were untouched. Both passes were saved and closed through the menu; fresh Sky inventory confirmed the final Tutorial window absent. Foreground was returned to root. Compilation, screenshots and this scripted playthrough do not establish that owners or customers find the game fun.

## Artifact and method

- Package: `BuildOutput/Tutorial/Windows/MagnetSweep.exe`; child under `MagnetSweep/Binaries/Win64/`.
- T1 child SHA256: `E5106A6F094F8E6C41D854E8EF360F85AFB10D45A647626D19FD77CD9488FE32`.
- T1 PAK SHA256: `7D4686C0AEABE938D0C24ABF62D226DC8AFF3F35FDFD3C76729064DC5E1CE4A6`.
- Final T2 child SHA256: `1090787DD46EAFC7AA14A1B84E4D41E04696825BBF6D2F40E4B6FF57ACA0DCF5`; PAK unchanged. Root reported final BuildCookRun success in 37.32 seconds. Changes since T1 were confined to tutorial text/marker presentation, with gameplay model/runtime unchanged.
- Root reported BuildCookRun success, 45.35 seconds, and 15 passing automated tests before this pass. Those are separate from native observations below.
- Root launched with `-DemoProfile=qa_tutorial -DemoQA -DemoFresh`. All native control used documented Sky actions, with actual player-facing Space/Q toggles for sustained attraction and precision. No save editing, simulated earned progress, console commands, slow motion, or owner-window interaction.
- Observed client was 1600×900. F9 produced game-only PNGs; seven were copied unchanged into `evidence/tutorial-*.png` across both passes.

## T1 observed results

| Check | Actual result |
|---|---|
| Welcome and start | Launch focus left the game paused; Escape exposed welcome. Start learning worked. Welcome and lesson cards wrapped legibly without clipping at this resolution. |
| Attract then release | Continuous field visibly pulled iron into cargo. Release lesson remained active until Space-off. The 8 kg / 16 cr haul remained attached, and the bundle lesson appeared. |
| Linked pickup | Visible chain, group tooltip and lesson agreed on 3 pieces / 9 kg / 36 cr. Capturing the group, with one incidental iron, produced a safe 19 kg / 56 cr haul and advanced to first payout. |
| First meaningful smelt | Furnace click paid 56 credits/XP, emptied cargo and consumed exactly one of four fuel charges. Visible metal travelled toward the furnace and an ingot remained afterward. Lesson advanced after the animation. |
| First risk teaching hold | At the highlighted cell pocket, one 4 kg hot cell was captured. Warning explicitly said it was paused and described the normal 3-second fuse and one-fuel cost. It remained held across more than 3 seconds of observation; saved `fuse_elapsed` was 0. No loss occurred while reading. |
| Actual rescue | Correct RMB input dropped the cell recoverably, switched off attraction, cleared the warning and advanced to Precision. Three fuel charges and the 56 banked credits remained. This was a lone-cell lesson; a valuable mixed haul was not tested in this hold. |
| Precision event gate | Q alone did not complete the lesson. A real narrow-field capture secured the optional 4 kg / 40 cr Amber Dynamo without either nearby red cell and advanced to Quota. |
| Goal and reward | Added 4 kg / 24 cr alloy, then smelted the 64 cr load. Total salvage reached exactly 120; the 80 cr bonus produced 200 wallet / 200 XP, rank 2, with two fuel charges remaining. |
| Collection and purchase | Workshop showed Amber Dynamo recovered, 1/6. Buying the Power Coil for 150 reduced wallet to 50 while retaining 200 XP. Coil became tier 1 with radius 145. Instruction advanced to trying it. |
| Completion requires action | Returning to the tray and actually collecting one 2 kg / 4 cr iron completed guidance. Saved state was step 10, disabled, risk learned, purchased mod 1. A completion toast appeared; the teaching panel closed. |
| Separate practice and skip | Pause → Practice tutorial opened a fresh, separately saved run and explicit return text. Play without guidance removed teaching and exposed Return to saved career. |
| Return preserves career | Return restored 50 wallet, 200 XP, 120 banked, coil tier 1, core ID 0, 2 kg / 4 cr carried scrap, two used fuel charges and completed tutorial. Native display and read-only save inspection agreed. |
| End of pass | Save & quit worked; a subsequent Sky window inventory was empty for the Tutorial package. No external-user-input stop was reported during this pass. |

## Readability observations and corrections

1. During the T1 first smelt animation the card briefly changed to "Gather copper for your first useful load" and "Your haul is worth 0 credits" although a successful payout was underway. During the second animation it briefly said "Bank 0 more credits." Corrected and observed in T2: the card now says "SMELTING YOUR HAUL", reports the actual payout and explains that the next lesson starts when the pour finishes.
2. The T1 lone-cell first warning taught fuel loss but did not teach that ordinary overload also destroys the most valuable unbanked salvage. Corrected and observed in T2: both consequences appear even with no valuable cargo, along with banked-reward protection and the first-warning hold.
3. The T1 Quota body recommended useful copper/alloy loads, but its generic marker pointed to the first available iron until the carried value could meet quota. Corrected and observed in T2: the generic pickup arrow is absent during Quota, leaving selection to the player. The conditional furnace highlight was already demonstrated in T1; it was not repeated after this localized marker change.
4. The linked-bundle marker and incoming aggregate tooltip provided concrete information absent from the earlier unguided build. This is an observed readability improvement, not proof of lasting engagement.

## T2 focused results

- Root reopened `qa_tutorial` without `DemoFresh`. The actual native tray resumed completed guidance with 50 wallet, 200 XP, 120 banked, tier-1 coil, one banked core, 2 kg / 4 cr carried iron and two remaining fuel charges. Default music 30%, effects 80%, master on were also retained.
- Entering practice resumed the earlier saved Skipped state instead of silently starting again. Pause offered **Restart practice**, whose confirmation explicitly limited replacement to practice progress. Confirming opened the guided welcome.
- Replayed attraction, release, linked pickup and 56-credit smelt. The corrected smelting card was directly inspected in Sky's immediate post-click screenshot and wrapped cleanly. F9 was too late to preserve that brief phase, so the subsequent PNG is not claimed as smelting evidence; no timing workaround was used.
- The new warning explanation fit the card and stated both one fuel and the most valuable unbanked piece. The first warning still held safely, and a real RMB drop advanced to Precision.
- After an actual precision core pickup, Quota had no generic iron arrow. Its own instruction remained visible and readable.
- Returned from the restarted practice to the main QA career; the exact prior career and carried iron were unchanged. Save & quit closed the process. Final read-only save inspection agreed with the native display and retained tutorial step 10, disabled, risk learned, purchased mod 1.
- Final practice was saved independently at Quota with 56 banked and the unbanked 40-credit core. Neither QA save is owner progress.

## Evidence and limits

- `evidence/tutorial-welcome.png`: actual initial welcome.
- `evidence/tutorial-attract.png`: clear first instruction and highlighted iron.
- `evidence/tutorial-bundle.png`: retained haul and linked-bundle lesson.
- `evidence/tutorial-paused-warning.png`: explicit protected fuse and cost explanation, T1 wording.
- `evidence/tutorial-workshop.png`: real earned 200 cr, recovered core and affordable upgrade.
- `evidence/tutorial-t2-warning.png`: final explicit dual-penalty teaching text and protected warning.
- `evidence/tutorial-t2-quota.png`: final independent quota lesson with no misleading iron arrow.
- T1 session log: `music_playing_observed=1`, `field_audio_observed=1`, `warning_audio_observed=1`, `smelt_audio=2`, `reward_audio=3`, `overload_audio=0`, `sounds_loaded=19`, recorded active frame average 13.34 ms. These establish activation and an instrumented average only. No auditory listening or perceptual mix judgment was possible.
- T2 session log: all 19 sounds loaded; music/field/warning playing observed, one smelt cue, one reward cue, zero overload cues, recorded active frame average 13.33 ms. No auditory listening was possible.
- Actual completed-career restart and persisted skipped practice were verified in T2. Restart from an unfinished active lesson or protected warning remains untested natively.
- Not covered natively here: alternate resolutions, early off-path danger, mixed-haul protected rescue, skip during held danger, deliberately exhausted/failed tutorial orders, all-mods-maxed replay, arbitrary interrupted saves, later live-fuse reaction timing, every upgrade type, or novice comprehension without prior source knowledge. Relevant automated tests cover several rules; that does not substitute for these native scenarios.

No blocking failure remains in the tested path. The next useful observation is an owner walkthrough without source knowledge: can the player explain why to choose a useful load, what field-off preserves, and exactly what an expired warning costs? Enjoyment, audio quality and whether guidance leaves enough room for discovery still require that player feedback.
