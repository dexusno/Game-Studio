# Magnet Sweep — production plan

Revised 2026-09-11. This plan follows the owner's gameplay correction and replaces the previous production plan. [BRIEF.md](BRIEF.md) governs the concept; [the gameplay critique](design/CRITIQUE.md) and current revisions in the [interaction](design/01-interaction.md), [progression](design/02-progression.md) and [replay](design/03-replay.md) notes explain the reasoning. No implementation or human playtest exists.

## Production objective

Produce a generous, tactile collection game: sweep useful scrap, aim a satisfying tug, release the linked group you expected, pour the haul into a responsive furnace and retain a visibly more capable magnet. Fresh deliveries must remain inviting after the final improvement.

Development timeline is not a consideration. Work proceeds by dependencies and observed quality, with no labor estimate, schedule cap or arbitrary content quota. Simplicity comes from understandable actions that reinforce one another. It does not justify removing the interesting pull, meaningful improvement or replay test.

The representative playable slice must contain ordinary sweeping, secure unrestricted cargo, a direct tug, an earned Breakaway improvement, a linked burst, a full deposit payoff and an arrangement that makes the new capability useful. It also needs a full-rig comparison across meaningfully different arrangements. A collection counter, a polished animation demonstration or an upgrade menu alone cannot answer whether this game is enjoyable.

## Dependency-led work packages

Root remains the integration owner for BRIEF.md, STATUS.md, DECISIONS.md, game.json and asset-manifest acceptance. Workers preserve one another's changes. Before implementation, assign exact files within these proposed boundaries; dependent work on shared modules is sequential. These packages describe future authorized implementation, not completed work.

| Package | Responsibility and owned paths | Dependency | Acceptance evidence |
| --- | --- | --- | --- |
| A. Isolated runnable project | Engineer: unreal/, scripts/ and BUILD.md | Current brief | A new Magnet Sweep project compiles, packages, renders and accepts input in an actual Windows launch. Record commands and build identity. |
| B. Collection and input | Engineer: gameplay/input modules under unreal/Source/MagnetSweep/ | A | Direct pointer manipulation; visible hover intentions; fixed press-context priority; sweep release retains cargo; deliberate dump banks once. |
| C. Expressive pull | Engineer: pull selection/state modules; designer: authored comparison data under design/arrangements/ | B and shared action contract | Direct tug works; finite corridor preview exactly predicts loose scrap and eligible directly linked tangles; no recursive release. Play demonstrates understandable authorship. |
| D. Representative presentation | Art specialist: assets/models/, assets/materials/, assets/textures/; audio specialist: assets/audio/; engineer alone imports and integrates under unreal/Content/MagnetSweep/ | B–C contracts; asset preparation may run alongside them | Sweeps, tether, burst, cargo and furnace read clearly in motion and sound; improvement is visible and usable. |
| E. Delivery and retained progression | Engineer: delivery/progression/save/UI modules | B–D representative loop | Furnace goal below available salvage; complete-action credit and celebration; retained Breakaway/reach; safe interruption and continuation; honest post-upgrade reward. |
| F. Fresh arrangements | Designer: arrangement source data and validity examples; engineer: importer/remix implementation | C and E; promising pull observed | Different geometry produces different appealing starting rings or directions with the full rig. New content does not merely relocate the same gesture. |
| G. Independent playable verification | QA: QA.md and reproducible test evidence; source author fixes owned implementation | Identified playable build, repeated after relevant changes | Actual controls, persistence, preview fidelity, progression, display/audio and package behavior checked; human enjoyment observations kept distinct. |
| H. Product and release preparation | Release producer: RELEASE.md and marketing/; engineer: versioned package | Promising complete experience and independent verification | Truthful captures/copy, rights and credits, verified package, unresolved commercial questions stated. External submission remains a separate action. |

The critical dependency chain is **runnable project → understandable sweep/tug/dump → representative feedback and improvement → full-rig replay → complete product verification → reviewable release materials**. Content follows promising interaction. Art should make the core legible during its test, not arrive only after a mechanic has been accepted from code review.

[BUILD.md](BUILD.md) records installed Unreal 5.8.2 and existing editor/build/package executables. A studio doctor schema mismatch is known; it does not establish a missing engine. Resolve actual project failures from their output. Reuse the established build approach without importing the parked FPS or assuming its successful builds prove this game works.

## What the first playable must demonstrate

Present inviting loose scrap, an obvious isolated ring and a furnace that visibly anticipates the carried contribution. Let the player sweep and pour a worthwhile haul. Then offer a tangle beside a loose seam: a straightforward tug succeeds, while a considered direction gathers a more expressive release. Forge Breakaway and immediately provide an understandable linked opportunity.

The same slice must let the player use the extended full rig on the replay designer's **linked fan** and **offset fork with seam**, without naming their intended tactics. These are contrasting test cases, not a fixed product content count. Keep their available salvage comparable when judging the effects of geometry. The fan rewards selecting a useful hub; the fork allows a seam-rich leaf-start pull to compete with a hub-start pull. Neither requires one correct answer.

Observe before suggesting tactics:

- Can the player predict what the next press and release will do, and collect without fearing lost cargo?
- Does ordinary sweeping feel pleasant, and does the player still choose it between tangles?
- Does a simple tug feel worthwhile? Does choosing another starting ring or direction produce a visibly better result that the player anticipated?
- Can they explain why a linked neighbor released and another stayed?
- Does the full pour feel like a payoff, and does the forging line motivate an enjoyable haul rather than an unwanted cleanup trip?
- After an improvement, what do they want to try, and do they actually try it?
- With no unlock pending, do they voluntarily ask for another delivery and see a different useful pull?

Record build, input method, observations, quotations and prompted versus spontaneous behavior. A voluntary repeat with a concrete new idea is stronger evidence than praise. Code assertions, agent agreement and a player's completion are not proof of fun.

If a central action is confusing or dull, revise that action and observe it again before expanding content. If players enjoy only the finite upgrade journey, report that limitation: the requested replayability remains unproven. Do not disguise it with more templates, record pressure or promises of endless upgrades.

## Engineering contracts

**Input and direct manipulation.** At primary-button down, choose exactly one action: highlighted furnace with cargo → dump; otherwise highlighted available ring → latch; otherwise usable tray → sweep. UI and outside-tray clicks must not leak into gameplay. Keep the chosen action until release. Crossing a ring or furnace during a sweep changes no mode. After a dump, release triggers nothing further. Hover preview and button-down resolution must use the same hit rules, including any overlapping targets. The magnet follows the pointer directly within the usable tray; screen edges and UI must not cause position jumps.

**Pull geometry.** Use a finite broad capsule from the selected ring to the reachable endpoint, clamped by current reach and usable tray. The selected tangle always releases, including a very short valid drag. Loose salvage qualifies by the same explicit positional test used for its highlight. With Breakaway, another available tangle qualifies only if directly linked to the selected one and its ring center lies inside the capsule, endpoints included. Newly released neighbors never propagate again. Preview and commit call one selection function with the same state and geometry. If anything relevant changes, refresh the preview before commitment.

**Material ownership.** Every recoverable unit has one stable identity and amount. Tangle membership and loose membership cannot overlap. Recovery transfers available material into owned cargo exactly once; banking transfers a snapshot of that cargo into the delivery's banked total exactly once. Reserve affected IDs atomically when a pull commits so later input cannot release or collect them again. Cosmetic streams may follow that transfer; they cannot award material. If a deposit occurs during a committed stream, its visuals must resolve consistently with the banked snapshot rather than duplicate or abandon the haul.

**Unlimited cargo, bounded presentation.** There is no gameplay capacity limit or load slowdown. Use representative pieces, instancing or an aggregated visible mass when individual cargo would obscure interaction or degrade performance. Preserve the true material amount independently of display count. Never silently discard salvage to satisfy a rendering limit. Released tangles join the secure haul rather than creating compulsory overflow cleanup.

**Delivery progression.** All material contributes consistently by amount. The visible goal is reachable through different appealing collections and sits comfortably below total available salvage. A deposit credits its entire amount, even beyond the goal. Complete the current action and its feedback before offering the forged improvement and continuation. Further collection remains available. Milestone ownership is monotonic and applied once. New deliveries reset local arrangement/fill state while retaining the rig. Full-rig deliveries forge a recovered-metal block and celebrate completed work; they must not imply another upgrade is imminent.

**Pause and persistence.** Pause/focus loss retains collected and earned state, cancels an uncommitted aim without releasing its tangle, and requires fresh input on resume. A committed recovery remains committed. Save the current delivery's actual arrangement, material states, cargo, banked amount, goal/completion state, rig, progression and settings. Restore a canonical playable state rather than a partially replayed scoring animation. Use versioned data, atomic replacement and a recoverable prior valid save. Test interrupted writes and incompatible/corrupt data without overwriting recoverable progress. An explicit exact-arrangement retry resets that attempt's scrap/fill, preserving the earned rig and earlier progress.

**Advancing with surplus.** Root's integrated presentation uses “Next delivery” when cargo is empty and “Pour & next delivery” when carrying surplus. The latter explicitly banks the remaining cargo through the existing deposit payoff, then transitions after the whole action resolves. Do not silently discard collected material or require another precision trip. Uncollected leftovers may be left behind. Test that this feels like a satisfying finish rather than an extra chore.

## Content, assets and reward construction

Author enough content to teach, express and revisit the actual enjoyable actions. The proposed opening sequence—discover the tug, earn Breakaway, gain reach, enjoy the completed rig—is a progression structure, not a prescribed number of levels. Determine delivery goals and density from substantial enjoyable actions and optional leftovers, not desired duration or an escalating quota formula. Earned power should visibly improve earlier situations and remain useful in later ones.

Arrangement data should describe loose seams, ring positions, direct links and open approach space together. Validate unique IDs, disjoint material membership, reachable rings, visible nonmisleading links, valid furnace placement, adequate salvage and useful corridors with the current rig. Include examples that catch accidental transitive propagation and universal hub solutions. Author successful spatial relationships before building any remix system. A generator may rearrange those relationships only when validity and play show that the result remains readable and invites alternatives.

The art kit follows the loop: workbench/tray, furnace with fill/forge presentation, base magnet and visible coil/arm evolutions, readable loose-scrap families, distinct tangle assemblies, rings/links, tether/corridor cues, growing cargo and the post-progression metal block. Reuse coherent materials and forms where appropriate; do not turn every arrangement into a separate environment. Inspect silhouette, motion, light and contrast at gameplay size and under the largest intended haul.

Audio needs recognizable sweep/attachment, latch, tug, linked release, deposit and forging feedback. Layer dense events with controlled concurrency and levels so a large burst feels rich rather than louder noise. Important cues must survive it. Music is optional and cannot replace weak interaction sound. Asset creators return provenance and commercial-rights evidence with their files; root integrates assets/manifest.csv. Optional generation tools are useful only when they improve the actual result.

## Verification and product decisions

The author demonstrates the intended behavior in a rendered build. Independent QA then checks that exact artifact, reports reproducible failures, and rechecks the author's corrections. Separate static review, automated state checks, actual engine/package behavior and human gameplay observations in QA.md.

Required coverage includes input-priority overlaps; sweep crossing rings/furnace; release outside the tray; UI capture; pause and focus loss during aim, transfer and deposit; fresh-input resume; repeated rapid press/release; reach clamping; capsule boundary and zero-length cases; direct versus transitive links; preview/commit equality; already-recovered neighbors; duplicate recovery/banking; full credit across a forge threshold; optional cleanup; retry; next-delivery surplus; retained improvements; interruption save/resume; corrupted or interrupted saves; and post-full-rig completion.

Check pointer mapping and readable UI at different aspect ratios, window sizes and fullscreen modes. Test audio levels, controls and dense event overlap. Evaluate hold alternatives and accessibility bindings in actual contextual play rather than claiming a toggle is automatically equivalent. Profile ordinary and largest supported arrangements, cargo and simultaneous effects on identified hardware. Record frame pacing and memory behavior across repeated deliveries; choose supported limits from observed results without imposing a cargo penalty.

Verify a fresh Windows package launch, complete delivery, save/relaunch, settings and clean exit outside the editor. Record exact commands, engine/compiler versions, source revision, package identity and tested machine. A headless test cannot verify appearance or sound, and one machine does not establish minimum hardware requirements.

Once the experience is coherent, prepare a reviewable product using real gameplay captures, accurate description of progression/replay, controls, credits and relevant asset/AI disclosures. Refresh official storefront requirements before submission. Do not advertise content hours or replay depth that testing has not established. The earlier price range is a research hypothesis, not inherited proof of value for this revised game.

Seek relevant players' reactions to the complete experience and its proposed price, with authorized recruitment. Distinguish expressed interest from actual purchases. Commercial ambition remains NOK 20,000 cash profit per game, with later games conditional on real results; no forecast follows from this plan. No implementation, spending, outreach or publication is performed by this planning assignment.

The next executable implementation assignment is the isolated project followed by the representative sweep–tug–dump–improve slice, preserving the action contracts and full-rig replay comparison above. Progress decisions follow observed experience and unresolved defects, not a calendar.
