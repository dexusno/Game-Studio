# Campaign interface — independent rendered review

**20 September 2026, refreshed recheck.** The settled parts, action and victory layout failures are visibly repaired. Recipe reward cards now expose kind, printed cost and cooldown; Fuel Brick describes its immediate effect; current materials and the plain Carbon1 quote are visible. The claimed/inactive reward presentation and Skip/Continue distinction are also visible in the combined scripted/native capture evidence. A comparison gap remains in the full-memory exchange source. Discounted/HP-cost quote input and transition behavior remain untested by this reviewer.

## Refreshed evidence and finding status

This short recheck inspected six relevant **1600×900** PNGs from `CampaignProbe-20260920-160609.log`, plus two existing native-window JPGs supplied by the author. The log SHA256 is `f41248f1850fe58a8d15dd7d22c8373734a0f91e9b1224c8ca81509f8b21e313`; it ends at 14:07:09 UTC with `ok=1 position=2 hp=68 receipts=37 revision=39 hash=82a77fe009a411b5 screenshots=12 physical_input=0`. It used UE 5.8.2-56702186, Windows 11 25H2 build10.0.26200.9457, DX12/RTX4090, `-game -windowed -ResX=1600 -ResY=900 -nosound`. These remain editor-game captures, not a verified standalone package. The reviewer did not launch Unreal or send input.

The source inspected after those captures has further local repairs and must not be attributed to the older pixels. At checkout `842300e9dcdf6612768485250b0ac2320dbde27c`, the relevant source hashes were:

- `FoundryCampaignUI.cpp`: `0fd5b8ad09bb04e3dc0e52b7c5461d9f0544fe3b781d2e5c480e7b63e4a4052e` (14:24:14 UTC; includes the later Recipes heading repair).
- `FoundryCampaign.cpp`: `eac3b4ffc5753ecd2138aa53f224ca4ed4a8474fa6369f84584185d3b56d70a0`.
- `FoundryCampaignProbe.cpp`: `848ff22079803a67ecf80d6bfcb10052ee11e042760c8af716c5e736e5199dcc`.
- `FoundryHost.cpp`: `28827e90f1ffac64a8da871a1fe8867c396f0dbd85baba9bd0494485b0ba711c`.

| Finding | Independently observed repair | Remaining acceptance limit |
| --- | --- | --- |
| **UI-01** | **Settled-capture layout failure closed.** Parts has separate reserve/installed/loaded columns, readable Shield6 and three loaded parts. Action has a normal header/banner/footer. Victory has two readable claims and departure control. Source `Rebuild()` also performs `SlatePrepass`, and the log records deferred capture after layout. | No independent observation in motion. These frames cannot rule out a one-frame flash or input/focus issue. |
| **UI-02** | **Selected Utility detail teaching closed.** Fuel Brick now says Immediate effect, Cooldown: None / normally once per turn, Ready to use, Use recipe, and explicitly no saved Utility part. | The refreshed image still has the generic heading “Forge a part”; this residual was sent to the author and current source changes it to “Recipes”. That heading repair is not yet rendered here. Actual immediate Heat increase/resource debit/no-part result was not independently clicked. |
| **UI-03** | **Three-choice comparison repaired.** Pitched Roof shows Shield/Iron2/None; Soft Mounting shows Modifier/Carbon1+Glass1/CD1; Contact Wire shows Shield/Iron1+Copper1/None. All remain readable beside their effects. | **Medium residual source finding:** `Exchange()` still shows only the offered name and “Replace <copy name>” buttons. It provides no offered/existing effect, kind, cost or cooldown at the permanent replacement decision. Preserve inspectable details for both sides. Sent to the author; no new exchange runtime failure is alleged. |
| **UI-04** | **Current supply visibility and plain payable quote repaired.** Fuel Brick shows Iron4/Copper2/Carbon1/Glass3/Circuit1 and Pay now: Carbon1. `Payable()` reads successful preview events for material, HP, Heat and Shield costs; rejected previews retain printed cost and the core reason. | Discounted copies, HP/Heat/Shield payments, unavailable recipes, resulting deductions and unchanged rejection state were not independently exercised through input. Source wiring is not an input pass. |
| **UI-05** | **Claimed-state/departure presentation closed.** Refreshed pending rewards show Skip remaining rewards. The native claimed-rewards JPG shows two grey “Claimed” rows and Continue through Cinderwall. The following native JPG shows route position2. `Rewards()` retains disabled claimed entries and derives Skip/Continue from actual pending entries. | The clicks, no-op disabled row and single Continue transaction are author evidence below, not independent execution. Partial claims, upgrade hover/focus, cancellation/reopen and quit/Continue still require the relevant T19–T22 interaction coverage. |

The integration owner also identified the action capture's misleading “Resolving the shot and committed enemy actions…” banner. Current `FoundryHost.cpp::SetActionView` supplies **Firing…** for Fire and **Enemy turn** for End Turn through `GetActionCaption()`. That source repair is visible in code, not yet in this capture set. No enemy-action timing defect is inferred from the old label.

The author supplied `Run-20260920-161518.log` (SHA256 `db1f30855331aac2efeb9372eb0300c4ccfe2f951e77db1dad4a12756a7ff510`), using a copied previous checkpoint from the legal 160609 probe. The author reports clicking a disabled Claimed row produced no transaction and Continue produced one; the inspected log contains one committed transaction, revision39. The reviewer inspected the resulting two images but did not perform or independently observe those clicks. Other author-reported native sessions cover ordinary New Game/Mayor/collection/craft/load/Fire/End Turn (`155123`), a prepared full-memory exchange/cancel (`160905`), and a prepared 24-Ammo/24-Shield scroll/Unload case (`161235`). They are attributed coverage only in this report.

Only the following refreshed images were visually rechecked; other files from the twelve-capture run were not re-reviewed in this finite pass. Filenames may be overwritten; these hashes identify the inspected pixels. All are under ignored `unreal/Saved/Screenshots/`.

| Capture | SHA256 |
| --- | --- |
| campaign-parts.png | `fb3c0d363d3d1f068e07d4455407fe383b4086abbead4ba8ca5914ded06737fb` |
| campaign-action.png | `632c0d7165aa14d2236327f78ce3dc1b6a7f3222fabec03403c94dbe7c9ad31e` |
| campaign-victory.png | `391dac48100b459406ee1221a4e89229f231fded3e733de0f6a692f97f3d3e18` |
| campaign-recipes.png | `a5d36c34806f673ef5af404f6d7a569ea59460f85c9d0391d787d4d39b1aed45` |
| campaign-recipe-reward.png | `0d028c56b2f954794c6493e8fb7f1f1e4cc5d74054b83a02b071eb697a4f09c6` |
| campaign-skip-confirm.png | `01d5e5d61ab4189e60128d6243c46a58fce0bf9c06fd94d58fe0daa001d22ec9` |
| native-campaign-claimed-rewards.jpg | `9361f55d498e97a91cd0165b50e8821d12efe020d2012bef3fa239d22ff9b2c5` |
| native-campaign-continue-after-claims.jpg | `eeb58508a58426754ff249de8806e7cf5864f485d38a874b38a59ed143d7fae1` |

The author accepted the remaining exchange comparison gap and is adding offered/existing recipe details while preserving exact-copy identity and cancellation. That further change was not included in the inspected source identity and is not closed here. A later 162255 probe has since overwritten the shared campaign PNG filenames; it was not visually reviewed in this bounded pass, so the table above identifies the earlier inspected artifacts rather than the files' newest contents.

The next bounded checks are the exchange comparison repair and targeted quote/reward input coverage. New Precision UI is outside this assignment. No audio, packaged-build, human comprehension, F.I.S.T. or whole-P14 acceptance is claimed. The original findings and evidence below are retained as the history of the first capture review.

## Original 154452 review — retained history

**20 September 2026, initial pass.** This finite review inspected all twelve supplied campaign PNGs at their native **1600×900** resolution, the corresponding scripted probe log, and relevant presentation source. Nine captures are readable; three have collapsed, overlapping UI and cannot establish readiness of the parts, action or main reward screens. Three additional decision/flow defects were open below. The misleading Utility wording in the older capture had an author-reported repair, not an independent rendered recheck at that point.

This is an agent visual/source review, not an input playtest, packaged-build test, human comprehension test or F.I.S.T. approval. The reviewer did not launch a game, send mouse/keyboard input, modify production, or interfere with the Unreal worker's active window. Only this report is owned by the reviewer. The `game-ui-ux` skill was applied.

## Exact evidence and limits

The capture run is `unreal/Saved/BuildLogs/CampaignProbe-20260920-154452.log`, SHA-256 `0ab7bd236e796707e62a1b3c7bb33c51d16c6968d0c6dcf0677931e1fb2f6c9c`. It ran **UnrealEditor.exe -game**, UE **5.8.2-56702186**, Windows 11 25H2 10.0.26200.9457, DX12 / RTX 4090, windowed 1600×900, with **-nosound**. The log concludes at 13:45:51 UTC with `ok=1 position=2 hp=68 receipts=37 revision=39 hash=073fd4087a27adc2 screenshots=12 physical_input=0`.

Those are author-scripted command results, not independent physical-input results. The log contains 16 screenshot events but only 12 unique filenames: parts and action files were overwritten on successive shots. This review inspected the last saved image of each. Raw captures/logs remain ignored; the hashes below identify the inspected evidence if those filenames are replaced by a later run.

The checkout HEAD at review start was `7dd085f5017a5490705ae6475769bf0d66b4e834`, with local UI work. The module had already been rebuilt after these images; its original binary hash was not captured by this reviewer. Do not assign the new DLL's identity to the older screenshots. The separately examined UI source was `FoundryCampaignUI.cpp`, SHA-256 `e602b8f512c5ff0bbfb4e3c777ea9d9c9800ed243478defca1a0d3748653eebb`, last written 13:50:00 UTC. It already differs from the capture in Utility wording. Supporting source hashes: `FoundryCampaign.cpp` = `85bd4938536352b0864e1c5927ce36df08a5cd7dec991b743d795d0085a1c060`; `FoundryCampaignProbe.cpp` = `50994018da2c7a8a517d82dd6df26c3ce2458ea37094039d6d18deb0169dcbc1`. Source locations below refer to those inspected versions, before the author's subsequent repairs.

Acceptance sources are [P14](../design/MVP-IMPLEMENTATION-PLAN.md), [selected reward flow](../design/REDESIGN-PLAN.md#victory-rewards-screen--owner-definition-20-september-2026), the Recipe/Shop/Load sequence immediately below it, and [timing/reward traces T19–T22](../design/TIMING-AND-PERSISTENCE.md). The relevant owner rules require exact costs/current supplies, inspectable unavailable recipes, immediate Utility use, individual visible claims, reversible recipe-window return, and main Skip confirmation only when unclaimed rewards remain.

## Prioritized findings

**UI-01 — High evidence/readability failure; persistence in live play unverified.** `campaign-parts.png`, `campaign-action.png` and `campaign-victory.png` render the header, menu, body and footer text almost on top of one another around x825–1450/y467–530, without their normal panels. The parts capture cannot communicate selection/load state; the victory capture cannot communicate available claims. Expected is the same stable layout hierarchy visible in the other nine images. Reproduction in the supplied probe: its step 14 loads and changes the drawer, then requests a screenshot immediately; Fire/action and victory presentation transitions also request a screenshot in their transition tick (`FoundryCampaignProbe.cpp:75–93`). The latest images correspond to log timestamps 13:45:41.188, 13:45:42.189 and 13:45:44.453 UTC. A screenshot alone cannot establish how long this lasts or whether input remains usable. The author accepted the issue and is repairing the root Slate rebuild/prepass as well as deferring capture until layout completes. **Required recheck:** settled frames and a short transition observation after Load, Fire and victory; verify there is no visible one-frame collapse before claiming the live issue fixed.

**UI-02 — Medium, visible incorrect action/result teaching; repair reported, not rechecked.** In `campaign-recipes.png`, selected Fuel Brick reads “Utility · Produces 1 · cooldown 0”, “Ready to forge”, “Forge part”, followed by instructions that crafted parts enter reserve and should be installed/loaded. MA001 is an immediate Utility with **no part**, costs 1 Carbon, and has **None** cooldown/one ordinary use per turn ([recipe row](../design/RECIPE-CATALOGUE.md), line 447). This predicts a saved object and later activation where Heat should change immediately. The inspected newer `Memory()` source, lines 311–317, already uses “Immediate effect”, “Use recipe” and explicit no-part instructions. The author reports this in build 155045. **Required recheck:** select Fuel Brick in that actual build, use it, observe immediate Heat/resource change and no reserve part; render None/once-per-turn separately from numeric cooldown 0. The latter still used the generic numeric formatter in the inspected source.

**UI-03 — Medium, visible missing comparison facts.** `campaign-recipe-reward.png` offers Pitched Roof, Soft Mounting and Contact Wire with a name, effect and “Keep recipe” only. Neither material cost, recipe kind nor cooldown appears before the permanent choice. Their differences matter: SH028 is Shield / 2 Iron / None; SH024 is Modifier / 1 Glass + 1 Carbon / CD1; SH027 is Shield / 1 Iron + 1 Copper / None. An effect-only comparison hides both future supply demand and that Soft Mounting is not a Shield part. `ItemCard()` lines 103–109 renders only name/description/button and is used at reward lines 435–438. **Fix:** include kind, printed recipe cost and readable cooldown/use limit on each offered recipe, retaining the effect and reversible return. The author accepted this change. **Required recheck:** this exact three-choice screen, then a full-memory exchange with the same information available for the offered and replaced copies.

**UI-04 — Medium, visible missing current supplies plus source-inferred cost gap.** `campaign-recipes.png` shows printed Carbon1 but no current Iron/Copper/Carbon/Glass/Circuit totals anywhere. The top bar contains HP, Shield, Heat and Credits; the body contains memory capacity. The player must leave the screen to know how much crafting stock remains, contrary to the selected Recipe view's resource counter. In the inspected source, `Memory()` lines 292–329 shows `R->cost` only; it uses `Rules::preview` to enable Use but does not show the actual paid amounts. Thus the source also lacks an authoritative current quote when an upgrade discounts a recipe. The screenshot alone does not demonstrate a wrong charge. **Fix:** show live material totals and a payable preview with actual material/HP/Heat/Shield costs and their payment timing. For an invalid preview with no payable events, retain the printed cost and exact rejection reason instead of inventing a quote. The author accepted this approach. **Required recheck:** an affordable plain recipe, a discounted copy, an HP-cost recipe and an unavailable recipe; the displayed quote, resulting deduction and unchanged-on-rejection state must agree.

**UI-05 — Medium, source-confirmed reward presentation mismatch; main-screen rendering unverified.** `Rewards()` line 445 skips every non-pending entry, so claimed controls disappear rather than visibly becoming claimed/inactive. Line 451 always labels departure “Continue through Cinderwall”, including when rewards remain. The selected owner flow requires visible claimed/inactive confirmation, **Skip** with unclaimed loot, and **Continue** when exhausted. This distinction tells the player whether departure leaves something behind. The supplied main reward capture is malformed, so this is a source finding, not an independently observed click sequence. `RequestAdvance` still performs confirmation and the author probe demonstrates it; **no confirmation bypass or lost reward is alleged**. **Fix:** retain claimed cards in a disabled claimed state and derive the departure label from actual pending entries. The author accepted both. **Required recheck:** claim cores only, return from/reopen the recipe window, cancel main Skip, then accept all loot and see Continue without a discard prompt. Include one independently claimed upgrade and inspect its effect through hover/focus.

All findings and concrete fixes were sent directly to `unreal_host` and the integration owner. No repaired capture set was independently inspected during this bounded review; source changes and author reports are not recorded as passes.

## What the images do establish

- The title communicates Mara/Cinderwall, the build-and-reach-the-city-end objective, and distinct New Game/Continue/Quit controls. Disabled Continue is visibly different in the fresh profile capture.
- Mayor choices expose complete effects beside individual acceptance buttons. The three cards fit and remain legible in this 1600×900 capture.
- Route screens show three concrete encounter alternatives, enemy names/HP/Armor, current position and a completed-position mark after the first victory. The shop capture exposes per-item price, finite remaining quantity and a separate core-sale area.
- Collection shows committed Mite/Ram intentions and a visibly selected steering material. It does not establish timing/Precision input or the initial inspect/OK flow.
- The confirmation screen explicitly says unclaimed rewards will be left behind and distinguishes return from commitment. The recipe reward screen presents three readable effects and a separate “Back to rewards” control. Their visual clarity is not evidence that every navigation path preserves the right state.

## Acceptance questions for the next interaction pass

1. After the layout repair, can the player select/install/remove parts, read actual remaining Shield and target/shot consequences, Load, Fire and return to preparation without a malformed frame or a modal stealing input? This needs normal input and later a larger assembly; no usable parts screenshot exists in this set.
2. Can the player compare an offered recipe and an existing copy, cancel a full-memory exchange unchanged, return/reopen the same reward choices, partially claim loot, quit/Continue, and then explicitly abandon only the remaining loot? Cover T19–T22 through the UI, not direct commands.
3. Are unavailable recipes still inspectable with an actionable reason, and do all presented costs/current materials update immediately after a paid or discounted Use? Does immediate Utility feedback teach the correct behavior without referring to a nonexistent part?
4. Are profile/Collection, defeat versus unfinished-fight Continue, city completion, initial enemy inspection, optional Precision, nested upgrade choices, hover/focus descriptions and alternate resolution/focus behavior reachable and understandable? Those states are absent from this image set. Audio was disabled; no audio result is claimed.

The next useful UI evidence is a refreshed settled capture set after the accepted fixes, followed by the focused input/reward checks above. Whole P14, package readiness, the full-city loop and human visual approval remain open.

## Capture identity table

All paths below are under `unreal/Saved/Screenshots/`; files are ignored local artifacts, not copied into the public repository.

| Capture | SHA-256 |
| --- | --- |
| campaign-title.png | `c0d7b5c3ec5ccf6d76bae756c3fa42c1d003c9aae7f6471ae95f5724722aa273` |
| campaign-mayor.png | `0a5e7d4c26e20eafdaf95456121867a24ac1df8d9aa755ba56a6aeed01ce3d91` |
| campaign-route.png | `535670d89eda59af82275df26f283b9cead3f25c45e5ee986abccc96af1b7b96` |
| campaign-shop.png | `929489a0f664ed7fab473dd3b3e3cb118112dcd8158997be3ae8e73ba619ed30` |
| campaign-collection.png | `d22d3eb7e77bad845dd560a02b49c093aee1c96ac040f47f7382424b6cccdc4b` |
| campaign-recipes.png | `3307eb5503aae3de2a6e80706942a37003d8d30e213bda7abcd45314c8bacc26` |
| campaign-parts.png | `9d9a834a558f4c98cbb7b23dc477f18875ffa8bda1bf5ad4d7b4f31f50385e17` |
| campaign-action.png | `f52c4551d9a22ec181ed23f29cd13f63fbcd49efd543a6041edabc371462b6a6` |
| campaign-victory.png | `6ac8b2e606764bccb524f2692f1c99bedaecef1b84ba882204d00f3aa6a90f02` |
| campaign-skip-confirm.png | `6f50365d53e4616f09da43d3818f05a895e9b07eb21622c97832294297a8383c` |
| campaign-recipe-reward.png | `f0e8a64d0c73cc6d0605fad98b786c67cab0eb8a35f071cba882914833d652f8` |
| campaign-next-route.png | `6a159bfd26b75d35fa4aadc018bcf9ab847568213a581eff09022f15b723e876` |
