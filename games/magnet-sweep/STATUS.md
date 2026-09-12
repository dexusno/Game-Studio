# Magnet Sweep — meaningful Extraction Coil preview

Updated 2026-09-12. Stage: prototype. Version **0.5.0 / Extraction E1**, separate Windows Development package. Klaus rejected radius-only coil progression: faster gathering does not let the player accomplish more. The replacement is implemented and packaged; editor, 21 automation cases and independent static critique pass after repairs. No native playtest occurred in this increment because the prior physical owner Escape stopped computer control. Timeline is not a consideration; no publication or purchases requested.

## Current owner decision and implemented change

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

When the foreground is safely handed back, independently play the exact Extraction E1 package using the routes in QA-EXTRACTION.md. Prove the 20 kg + 8 kg versus selected 24 kg case visibly, then judge whether choosing when to extract improves the bulk-salvage loop and makes the purchase worth earning. Do not claim three worthwhile tiers from numeric counts alone. Coordinate which existing career to continue before any future owner-profile migration; current preview is separate.

Broader construction purpose, the value of the other upgrade branches, economy balance, voluntary replay, perceptual listening and broad hardware/performance remain unverified. No new audio or external assets were added. Preserve unrelated Dreambound work. No release/publishing is authorized.
