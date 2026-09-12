# Magnet Sweep — Expedition Lab 0.6.0

Updated 2026-09-13, Europe/Oslo. Stage: prototype. The new **Expedition Lab 0.6.0** is compiled and packaged separately at `BuildOutput/Expedition/Windows`. **52 engine cases pass: 31 Expedition plus 21 legacy, zero failed/warning/unrun cases.** No Expedition native launch, owner-save access, migration, reset, purchase or publication occurred. Timeline is not a consideration.

## Current goal and milestone

The owner requests a scalable upgrade system with many different abilities and price tiers, combinations, competing choices, very different builds, a clear goal and rewarding strategies. A wider radius and a permanent nine-purchase ladder do not meet that requirement. The previous goal turn was **progress**, integrating researched design at db860a446e7e7cf1faabd1b053585fea4b2de45b. This turn is also **progress**: the isolated playable implementation and its first engine verification/package milestone. **The full gameplay goal remains active; it is not achieved by this package or test count.**

The working expedition has four sites, two active tools, four passive sockets and thirty implemented catalogue identities across six families: extraction, launching/welding, conduction, rigging, vector manipulation and remote fields. Some individual support effects still lack independent witnesses; the catalogue is not a claim that every upgrade is accepted. Fresh run equipment/cash with permanent discoveries and archived rigs remains a **working recommendation, not an owner-selected reset preference**. Old careers stay separate.

## Implemented behavior and verified evidence

- The `-Expedition` branch runs before legacy Start/Load/Save. [Play Expedition Lab.cmd](Play%20Expedition%20Lab.cmd) uses its own `expedition_preview` profile under `Saved/Expedition`. World schema 2 stores current appraisal, fragments, guides, charge, direction, machinery constraints and paid motion. The composite rig/world/site-entry codec validates before publishing; retry restores the whole site transaction.
- LMB buys a real capture batch, Shift narrows it, field-off retains secured cargo and RMB drops everything recoverably. Q/F hold to aim and matching release commits. Aiming and pause retain paid physical motion; stale input and unpaid multi-stage marks are cancelled. Rail has explicit single-ammunition selection and selected-iron welding, never an automatic weld/shot. Ground, sensor/receiver, heat sink, saved anchor, split and guide operations have actual targeting controls. Normal Push/Transfer remain available with advanced modules fitted.
- Recovery has physical collar, ballast and arm-brace conditions, a protected core and a separate receiver. Furnace E banks mixed settled scrap; receiver E delivers the core. Actual first-site recovery funds real purchases/refits, and a complete four-site runtime-input route archives a winning rig. Secured final delivery remains possible at zero battery.
- Unsafe/hot cargo has a three-second fuse. Timely drop preserves the haul; expiry spills recoverably and spends 12 battery. Actual collisions/press contact can damage struck salvage. Pause freezes physical clocks and paid motion. Welds conserve raw source mass/value and current appraisal; healthy material does not inherit an unexplained payout loss from damaged input.
- Tested combinations include a placed conductor, welded counterweight, selected heat transfer, physical sensor impact, split transfers, guide plus separately paid projectile, reaction partner, orbit fragments, functional generator and moving supports. Shear now previews and requires crossing the same finite plane; its fixture and real advertised-rule defect were repaired. Intact Recovery handles loss of the generator's last mount whether the generator or its cage is cut.
- A gameplay critique caught easy salvage making new capabilities economically unnecessary. Site index 1 now preserves 828 total raw value but limits ordinary haulable pools to 168, below the 180 bonus. **Actual earned runtime routes** recover the supported machine plus six loose alloys for 362, or isolate the live component plus six alloys for 366; both earn the real +4 refining credits, resist duplicate bank rewards and restore correctly. Ordinary core completion still pays 12. These are physical/economic witnesses, not proof the routes are equally appealing.

Final report: **2026.09.12-23.45.53**. Build/cook/stage/archive succeeded in 42.59 seconds. Packaged child EXE SHA-256: **6485ed0efb9c512ba8b33044b87bc330b515d98d5a410cabd7320f7c8b69aa6f**. [Verification evidence](evidence/expedition-verification.json) includes failed intermediate cases and final artifact identities; [independent QA](QA-EXPEDITION.md) records exact coverage and limits. [BUILD.md](BUILD.md) describes controls and reproduction. Existing legacy compiler warnings remain documented; zero test warnings does not mean a globally warning-free compiler log. Studio validation reports only the existing unrelated Scrapstorm QA link to `build/Sixfold-Recoil-Beta.html`; it remains untouched.

## Next action and limits

A separate native playtest is the next integration check: verify actual Q/F and staged targeting, tutorial comprehension, field feel, visual readability, sound, pause/focus, rendered saving and one earned upgrade route. Compare deliberate and continuous attraction. **Native control is still stopped:** the earlier Clarity playtest recorded the owner's Escape. Preserve the possible owner `qa_clarity` process and all old saves. A foreground-handoff question is pending; no answer is inferred from time. Do not launch or control a game until that handoff is resolved.

There is useful work beyond native acceptance: independently witness the support gaps in QA, make the four sites materially different, and challenge Coil/Jaw dominance versus slower rigging. All four sites still share machinery geometry; one changed reward distribution is not four different encounters. Controls, visual quality, listening, performance, enjoyable risk/reward, voluntary replay and commercial appeal remain unproven. Current original meshes/audio are reused, with no new external assets. Do not narrow the full goal to thirty flags, passing callbacks or a package. No release is authorized. Preserve unrelated Dreambound work.

## Research and design reference

[BUILD-SYSTEM.md](design/BUILD-SYSTEM.md) integrates three experts' direct debate and research of all eight owner-named games plus Nova Drift. [Mechanics/catalogue](design/BUILD-SYSTEM-MECHANICS.md), [economy/paths](design/BUILD-SYSTEM-ECONOMY.md) and [gameplay critique](design/BUILD-SYSTEM-CRITIQUE.md) distinguish primary-source observations from proposals and uncertainty. The committed logical economy check covers 30 modules, six authored paths and 24 stocks, ending with cash 1/16/2/0/3/2 and rejecting six invalid scenarios. That design evidence does not prove generated shops, physical opportunities or fun. Price bands, charge budgets and support prerequisites remain tunable.

## Prior implemented increment — Extraction E1

Radius/force alone is rejected as the coil's selling point. The **Extraction Coil** now lets the player aim at a linked available piece and press **F** to sever its reciprocal links and pull only that selected piece. Broad and latched attraction, current drag-to-smelt intent and other loose-piece motion stop after success. A new player action is required for ordinary collection to resume. Only the selected mass needs to fit safe capacity; existing cargo must be stable. Choosing a linked hot cell deliberately retains its hazard, with the existing first-risk teaching rules.

Concrete verified comparison: holding 20/24 kg, an alloy tied to two irons weighs 8 kg. Ordinary capture produces an unsafe 28 kg haul which the furnace refuses. F extracts the 4 kg alloy alone and makes a safe 24 kg haul while leaving 4 kg of iron available. Extraction earns no credits/XP/core album reward until a real smelt. The selected pull is configured for 0.55 seconds, and immediate smelting waits for that transfer.

Coil tiers 1/2/3 supply 1/2/3 extractions between successful smelts. Drops, failed/empty smelts, quenching, menus and reload do not recharge the count or restore severed links. Successful smelting and a new/retried order recharge it. Existing purchased Coil tiers map directly and retain prior radius/force extras; no purchase/refund migration is needed. These are more uses of one capability, not three different abilities.

Fresh layouts append three separate optional alloy-and-two-iron assemblies. Original pieces, IDs and positions remain, so ordinary quota routes and saved trays are preserved. Each valuable piece requires its own selected cut; there is no shared cheap hub which releases all three. Existing saves load their exact tray and receive new assemblies only on an ordinary new/retried order. Selective-plucking dominance and higher-tier value remain gameplay hypotheses.

## Teaching and defects repaired

The shop now sells extraction capability and count, rather than field radius. The target tooltip distinguishes normal whole-bundle mass from the selected F mass, result and cost; the same target selection is used by the action. Persistent F count, target highlight, link-break particles, extraction transfer and weight-left-behind feedback support the action. All use original assets.

The coil lesson requires actual successful extraction; an unrelated iron pickup cannot pass it. It invites filling saved space before smelting, instead of forcing a wasteful tiny pour. No-links-left and no-fuel paths preserve normal banking and point to finishing/the next order without claiming use. Guard-off and first-risk explanations remain. The prior clarity fix still accepts any safe ordinary metal mix without lesson-order gates.

Independent critique found **F → E during the transfer → RMB** left a queued deposit alive: later F could be blocked and a new pickup could smelt automatically. Drop and invalid/held deposit now cancel both pending smelt and finish intent, with an actual runtime callback regression. See [QA-EXTRACTION.md](QA-EXTRACTION.md).

## Evidence and build identity

- Editor compilation succeeded. Fresh automation report **2026.09.12-21.46.20** has **21 success, 0 failed, 0 warnings, 0 not-run**: eleven Rework and ten Tutorial cases. Actual model/runtime callbacks and JSON exercise selected capture, three-tier limits, normal-pull comparison, F input-handler refusals, cancelled queued smelt, delayed/once-only payment, actual coil lesson and old/new save continuity. They do not exercise physical keyboard dispatch or prove presentation/fun.
- All six quota routes pass using original pieces only, without extraction; the existing affordable-rig model assumptions still apply to advanced jobs. Added assembly centers are at least 25 units inside walls; tested default-layout center clearance ranges from 42.46 to 46.00 units. This is geometry evidence, not a visual review.
- Win64 Development / DX11 SM5 build, cook, stage and archive succeeded in **39.47 seconds** at BuildOutput/Extraction/Windows. Child EXE SHA256: **771df590f395bd1a4dc76bf79142f334133a70d6f161d37c027d425ccc7ab8dd**. Exact artifact identities and cases: [extraction-verification.json](evidence/extraction-verification.json). Reproduction: [BUILD.md](BUILD.md).
- The final package includes the follow-up tooltip waiting hint. No Extraction game was launched. Physical F input, target/tooltip readability, 0.55-second visual feel, actual saved-owner continuation and enjoyment remain unverified.
- Final scoped whitespace checks pass. Studio-wide validation reports only the pre-existing unrelated broken link in games/scrapstorm/QA.md to build/Sixfold-Recoil-Beta.html; it was left unchanged.
- Historical Clarity 0.4 native evidence verified immediate iron payout and safe training capacity, then stopped on owner Escape. It does not establish the new extraction behavior. Earlier owner rejection supersedes scripted-agent confidence; history remains in QA-CLARITY.md, QA-TUTORIAL.md and QA-REWORK.md.

## Saved progress and launchers

The current **qa_clarity** session may contain owner play: preserve its process, save and current progress. Do not resume native control or treat that profile as disposable. No owner save was read, copied, migrated or reset in this Extraction increment.

[Play Extraction Preview.cmd](Play%20Extraction%20Preview.cmd) targets the separate 0.5 package with its own persistent **extraction_preview** career. It does not grant test money/equipment or overwrite an existing career. [Play Magnet Sweep.cmd](Play%20Magnet%20Sweep.cmd) still targets Clarity and its copied original career; it is intentionally not repointed while current owner progress may be in use. Existing packages remain separate.

Historical original 0.3 player.json and backup are preserved under BuildOutput/OwnerBackups/before-clarity-v04; source SHA256 at the prior copy was e8a42a0389af5abe363fc3a02ae6a0a6bff195e9cd87d7b20aae747e152a3775. The copied Clarity normal profile and current live qa_clarity are distinct and must both remain intact. Save compatibility tests do not substitute for a native continuation check.

## Historical Extraction follow-up

When the foreground is safely handed back, independently play the exact Extraction E1 package using the routes in QA-EXTRACTION.md. Prove the 20 kg + 8 kg versus selected 24 kg case visibly, then judge whether choosing when to extract improves the bulk-salvage loop and makes the purchase worth earning. Do not claim three worthwhile tiers from numeric counts alone. Coordinate which existing career to continue before any future owner-profile migration; current preview is separate.

Broader construction purpose, the value of the other upgrade branches, economy balance, voluntary replay, perceptual listening and broad hardware/performance remain unverified. No new audio or external assets were added. Preserve unrelated Dreambound work. No release/publishing is authorized.
