# Magnet Sweep — build-system redesign; Extraction E1 remains the playable build

Updated 2026-09-13, Europe/Oslo. Stage: prototype. The current package is **0.5.0 / Extraction E1**. The active design recommendation is a salvage expedition with interacting tools and physical recovery objectives. This is not implemented in the package. Timeline is not a consideration. No native game control, launch, owner-save access, migration, reset, purchase or publication occurred during this design increment.

## Active goal and integrated recommendation

The owner requests a scalable upgrade system with many different kinds and price tiers, combinations, competing choices, very different builds, a clear goal and rewarding strategies. Research all eight named inspirations and rethink the game where needed. A radius-only advantage and the permanent nine-purchase ladder do not meet that requirement.

Start with [the integrated design](design/BUILD-SYSTEM.md), then read only the needed specialist sections: [mechanics/catalogue](design/BUILD-SYSTEM-MECHANICS.md), [economy/paths](design/BUILD-SYSTEM-ECONOMY.md) and [independent gameplay critique](design/BUILD-SYSTEM-CRITIQUE.md). Three experts researched primary sources and debated directly; root researched Nova Drift and integrated their proposals. All eight requested games are covered, with observation/inference and historical-source limits.

Recommendation: four-site salvage expeditions ending in a physical 20 kg core recovery; two active tools and four passive sockets; 30 distinct proposed modules across six action families and four price bands. A finite site battery replaces four furnace charges in this proposed mode. The final core uses physical preload, counterweight and brace conditions rather than three service bars. Every mandatory site needs a baseline route, with valuable optional outcomes enabled by tools. These are design requirements, not tested world content.

Critique changed arbitrary valuable-item loss to a recoverable spill plus 12 battery; damage is causal and the mission object remains recoverable. Pause freezes all danger clocks. Exact retry restores the whole site-entry transaction. Shops must offer two independently useful affordable module choices. Root's capability audit additionally caught a forced extraction pivot in an electrical shop; the repaired stock passes final verification. A stricter support check also repaired the early electrical, later winch and later vector shops without changing purchase-path balances. Welding and launching are separately paid stages so a welded body can be used as a real counterweight. Switching to Winch keeps the paid slug; RMB consistently drops the whole haul, while field-off retains it.

Fresh run equipment with permanent discoveries/records is the recommended working model, **not an owner-selected reset preference**. The optional preference question received no answer; no old career is converted. Existing save/profile protection remains below.

## New design evidence and limitations

The proposed catalogue has 30 different identities, including 6 active tools and 6 two-socket keystones. Six authored purchase/refit paths and 24 shop inventories cover six family directions, a non-keystone hybrid and pivots. Final verification passes all 30 catalogue entries, six routes and 24 shops, with remaining cash **1 / 16 / 2 / 0 / 3 / 2**. The verifier checks prices, actual purchase-based resale, slots, once-only rewards, depot order, precharge limits, installed capability prerequisites and an affordable current-tool support alongside another compatible option. Six deliberately invalid scenarios were rejected. See evidence/build-economy-verification.json and evidence/build-economy-invalid-scenarios.json. Its report is design evidence only: it cannot prove physical opportunities, reachable refining output, generated-shop quality, satisfying attraction or voluntary replay. The original weak 21-credit leftover shop is retained as rejected design evidence, not accepted stock.

No Unreal source or native build was changed in this research increment. The new mode, real support/charge/launch/field interactions and broad build diversity remain unimplemented. The old 21 passing engine tests concern Extraction E1 only. Independent review of the integrated design completed with conditional design endorsement and no remaining blocking paper inconsistency after repairs. It explicitly leaves physical opportunity, attraction feel, reward plateau and voluntary replay unproven. Scoped whitespace checks pass; studio validation still reports only the unrelated pre-existing Scrapstorm broken link. Do not treat a research catalogue or budget check as a fun/playability result.

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

## Next action and limits

Immediate next action: implement the isolated expedition mechanics experiment specified in design/BUILD-SYSTEM.md. The researched design/critique and logical economy milestone is complete; the expanded playable experience remains open. Preserve the full six-family/catalogue objective while first demonstrating the conductor and counterweight combinations, a real core route and continuous-versus-bounded attraction. Never reinterpret the old career. The old native Extraction acceptance below remains useful historical follow-up, not completion of the redesign.

When the foreground is safely handed back, independently play the exact Extraction E1 package using the routes in QA-EXTRACTION.md. Prove the 20 kg + 8 kg versus selected 24 kg case visibly, then judge whether choosing when to extract improves the bulk-salvage loop and makes the purchase worth earning. Do not claim three worthwhile tiers from numeric counts alone. Coordinate which existing career to continue before any future owner-profile migration; current preview is separate.

Broader construction purpose, the value of the other upgrade branches, economy balance, voluntary replay, perceptual listening and broad hardware/performance remain unverified. No new audio or external assets were added. Preserve unrelated Dreambound work. No release/publishing is authorized.
