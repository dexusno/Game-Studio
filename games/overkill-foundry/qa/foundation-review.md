# Foundation increment — independent review

20 September 2026. Reviewer: independent QA agent, not the core/content implementation author. Scope: P00–P04's staged Mara/Cinderwall content contract, 19 recipes and first Mite/Ram fight. The Unreal adapter was being edited and was explicitly excluded. Production files were not edited by this reviewer.

**Final assessment:** ready to continue technical integration. The engineer fixed all four independently reproduced rule/validation findings; the final recheck passes 15 independent runtime cases and 443 authored assertions. The controlled fight and content structure reproduce their stated results, with no known open defect in this bounded review. This is not complete-city, graphical, balance, packaged-build or human visual acceptance.

## Build and environment

- Checkout contains the required baseline `dcb72a6662559da466e6cb4029b648e210f52c76`; HEAD was that revision with uncommitted implementation files. The revision alone does not identify the reviewed implementation, so file/artifact hashes are recorded below.
- Initial rules/content: `of-core-0.1` / `cinderwall-staged-0.1`; snapshot schema 1.
- Windows 11 Pro 64-bit, `10.0.26200`; MSVC `19.44.35228.0`, Visual Studio 2022; QA build selected Windows SDK `10.0.26100.0`; Python `3.11.5`.
- Executed native x64 Release console binaries and separately compiled QA probes with C++17, `/W4 /WX /permissive-`. Tools: PowerShell, CMake/MSBuild, Python, source inspection and actual native process execution.
- Inputs: runner `--fixture` submits the authored action sequence; independent probes submit `Rules::apply` actions for Collect, Craft, Install, Remove, Load, Fire, Activate and End Turn. These are simulated commands to the real core, not mouse/keyboard gameplay or observations of human enjoyment.

Initial SHA-256 identities:

| File/artifact, relative to game | SHA-256 |
| --- | --- |
| `core/include/overkill/core.hpp` | `d4e3a200d97e6ddc1bceb601d964fe1ce050d6e7b05a3f2c898e24bf06f1d768` |
| `core/src/core.cpp` | `0a31c07316255c0234000dc0a7de555d3efcc51b18e5790dac542efe3144ec63` |
| `core/src/serialization.cpp` | `b97b1f5d5bdeadfcb1cfe26b36a159f4cbd61ab8046c12dced34223240760b8b` |
| `core/build/Release/overkill_core_tests.exe` | `19f3eca6796331a947c9d2caf18944b6dacfc5d4ce56ca9024e41b54444b4a51` |
| `core/build/Release/overkill_runner.exe` | `cab0bb422a8de142adb54978c92288edccc8496ea3ba8b83bcd59037a934987c` |
| `tools/compile_content.py` | `5e47ddeede1e723d830653d629c0571d237e9e9b6bef81fbd5f4e159309a230f` |
| `content/cinderwall.manifest.json` | `b95f20dc3e4706046e105d291db4f83ef62291c770f355bbd4acf11d3165c6db` |

## Executed checks

| Check | Initial result and coverage |
| --- | --- |
| `core/build/Release/overkill_core_tests.exe` | **Passed:** 400 author-suite behavioral assertions. This run is independent execution of authored tests; it does not make those tests independently designed. |
| `core/build/Release/overkill_runner.exe --fixture` | **Passed:** `mite-ram-v1.1`, HP 80, round 3, four shots, final state hash `7b7445f97952ad3c`. Trace includes 20-damage shot / 17 actual Ram damage, and 18 enemy damage absorbed by the assembled 20 Shield. |
| `python tools/compile_content.py --check --self-test` | **Passed:** exact regenerated artifacts; 606 source recipes / 246 eligible / 12 starters; 127 ordinary upgrades, 25 Mayor gifts, 10 robots, 10 formations, 3 Mysteries, 12 statuses, 207 physical part types. All 9 negative mutations rejected. Author validator enumerates 560 schedules and 6,160 offer states. |
| `python tools/compile_content.py --check --release` | **Passed refusal:** exit 1, `Unsupported release operations: 665`. No executable support was falsely certified. |
| `python qa/content_probes.py` | **Passed:** 51 independent checks against source rows, beta acquisition/rarity rules, exact Cinderwall pool/gates, single-Mite formation/summon rules, 35+5 chassis/Mite cores, Mystery proportions, unlock boundary and representative nested output chains. |
| `qa/build/Release/foundation_probes.exe` | **Failed:** after correcting the duplicate-Modifier expectation, 9 cases passed and 5 failed, representing 4 findings below. Fresh compilation used the initial core hash above, checked before and after the build. |

The independent runtime passes cover lethal main recoil preceding a heal/victory; lethal spread recoil preventing later spread; fully depleted saved Shield surviving a snapshot and next-round reinstall without refilling/repaying; explicit cooling in the blocked round with the new use-round exemption; multihit and nonattack Weaken decay; zero-after-Armor hits preserving Tiles and Burn bypassing Armor/Tiles but spending enemy Shield; 40 rounds of legal stockpiling, 40 Shield parts, a 34-part shot and seven resource-supported shots in one round; deterministic repeated preview, RNG consumption, snapshot continuation/event order and corruption rejection. Duplicate SH050 correctly refreshes its named bonus.

Manifest inspection specifically followed `SH102 → recipe-MA016 → plain-shield-5` and `SH118 → recipe-SH096 → plain-shield-12`, plus Folding Brace's separate generated 4/6 Shield parts. These paths retain future outputs of copied generator parts. No concrete missing enabled ID or nested output was found. This is sampled semantic review plus structural checks, not proof of every source effect or future runtime behavior.

## Actionable findings on the initial build

All findings below apply to `of-core-0.1`, source SHA-256 `0a31c073…4ec63`, and are implemented as reproducible cases in `foundation_probes.cpp`. Severity concerns the foundation/integration risk; no delivered player save or packaged game was used.

### F01 — Medium: removed insulation still prevents Burn

- **Steps:** prepared state at 80 HP, Burn 3; craft SH006; Install; Remove; End Turn with no other protection and a nonattacking surviving robot.
- **Expected:** the removed source cannot intercept the tick; HP 77. The reactive installed-source condition is stated in `RECIPE-CATALOGUE.md:105`; SH006's first-round restriction remains.
- **Observed:** HP 80, Burn suppressed while the pad is in reserve.
- **Evidence:** `FAIL SH006 removed source does not ward Burn … observed 80`.
- **Affected source:** `core/src/core.cpp`, `Engine::effects(Op::BurnWard)` / `Engine::endTurn`; the global flag lost source presence. Corrected with installed-source/original-round checks and associated header/snapshot changes.

### F02 — Medium: duplicate Extra Lift boosts stack

- **Steps:** own two SH008 copies (within memory capacity); craft/Activate each during the same round; End Turn; ordinary Collect next round.
- **Expected:** one +2 bonus, haul 12. Identical named Magnet enhancements refresh; different named enhancements may combine (`RECIPE-CATALOGUE.md:206`, `:216`).
- **Observed:** haul 14. Each duplicate schedules and delivers another +2.
- **Evidence:** `FAIL identical SH008 Magnet copies refresh one next-haul bonus … observed 14`.
- **Affected source:** `core/src/core.cpp`, `Engine::schedule`, delayed-haul delivery. Source identity and due collection must survive saving.

### F03 — Medium: different named next-shot modifiers overwrite

- **Steps:** use SH050 (+60%), then a controlled distinct named Modifier with the supported `PercentDamage/Activate` operation (+20%); assemble SH001+SH003 for base 9; Fire at an undefended target.
- **Expected:** +80% combined, `9 + floor(9 × 80/100) = 16`. Different named Modifiers combine; same named copies refresh (`RECIPE-CATALOGUE.md:202`).
- **Observed:** 10 damage: only the later +20% remains. Reversing use order would leave only +60%.
- **Evidence:** `FAIL different named percentage modifiers combine … observed 10`.
- **Affected source:** `core/src/core.cpp`, `Engine::effects(Op::PercentDamage)`; `s.nextPercent = effect.amount` overwrites the earlier source.
- **Scope:** controlled operation-level fixture; only SH050 is in the currently staged percentage-Modifier catalogue. This is an integration defect, not a claim that another percentage recipe is already enabled.
- **Correction:** the first QA expectation that two SH050 copies should produce +120% was wrong. The explicit same-name exception was identified during author review; the independent test now correctly expects 14 damage for those duplicates, and passes on the original build. That withdrawn expectation is not a defect.

### F04 — High: unsupported definitions can consume resources as silent no-ops

- **Steps:** construct a Utility recipe costing 1 Iron with `Op(255), Timing::Use, amount 9`; create `Rules` with it, grant its recipe copy to a prepared state, and Craft. Repeat with known `FlatDamage` on the unsupported `Use` clock.
- **Expected:** explicit content-construction rejection, or action rejection before costs/state changes. P02 requires unsupported effects to be rejected; the manifest README explicitly prohibits silent no-ops.
- **Observed:** both definitions are accepted; Use succeeds and consumes the Iron while applying no payload. Constructor checks only nonnegative amounts/costs, while the executor falls through unsupported combinations.
- **Evidence:** `observed unknown effect accepted; Iron paid=1`, followed by both opcode and clock rejection-case failures.
- **Affected source:** `core/src/core.cpp`, `Rules::Rules` and `Engine::effects`. The separate full-manifest release gate correctly fails, but cannot make a directly admitted core definition functional.

## Reproduction

From the repository root, with CMake available (the verified installation is Visual Studio's bundled CMake):

```powershell
cmake -S games/overkill-foundry/qa -B games/overkill-foundry/qa/build -G 'Visual Studio 17 2022' -A x64
cmake --build games/overkill-foundry/qa/build --config Release --parallel 4
& games/overkill-foundry/qa/build/Release/foundation_probes.exe
python games/overkill-foundry/qa/content_probes.py
```

The QA CMake target compiles the current shared core sources; it does not patch them. Build products are ignored under `qa/build`. Controlled test-only recipes are declared inside the QA executable and never added to the game's staged or final content pools.

## Recheck and untested scope

First recheck, `of-core-0.2` / snapshot schema 2: the original 14 independent cases pass, and the rebuilt author suite passes 441 assertions. The fixture remains 80 HP / round 3 / four shots; schema/state changes produce hash `5de73f967dc79677`. Core source at that recheck: `a51ec946020ddbc5455a0dec4f9af4321331effde06bf7f0aa8c65a2cba5825d`.

At the first recheck F01–F03 and the original F04 opcode/clock reproductions were fixed. A relevant follow-up found **F04 still partially open**: `Kind::Utility` with valid `FlatDamage/Assembly` passed the new opcode/clock validator, successfully spent 1 Iron on Use, and could never reach Assembly because a Utility produces no part. The fifteenth independent probe failed on that exact state. The engineer subsequently added reachable kind/clock validation, including automatic-output restrictions, and rejected empty definitions. No new issue was inferred for ordinary staged recipe definitions.

**Final recheck:** `of-core-0.2` / `cinderwall-staged-0.1`, snapshot schema 2. All **15 independent cases pass**, and the native author suite passes **443 assertions**. This recheck additionally exercises named bonus serialization/preview, consumption after Fire, and duplicate Magnet refresh after serializing its pending delivery. The fixture finishes at HP 80, round 3, four shots; state hash remains `5de73f967dc79677`. Content/compiler hashes are unchanged from the independently checked initial manifest. The release gate continues to require all 665 operations; it was not populated or bypassed.

| Finding | Recheck outcome |
| --- | --- |
| F01 | **Closed:** removed pad permits Burn 3 to reduce 80 HP to 77; authored regression also covers the old source not rearming in another round. |
| F02 | **Closed:** duplicate named SH008 gives one +2 next-haul bonus, including after saving its first pending delivery; haul 12. |
| F03 | **Closed:** different sources combine to +80% / damage 16, while duplicate SH050 refreshes +60% / damage 14. Named bonuses survive snapshot/preview and are consumed by the shot. |
| F04 | **Closed:** unknown opcode, unsupported opcode/clock and unreachable kind/clock are rejected. Author regressions also reject automatic-output content with a physical-part clock. |

Final SHA-256 identities:

| File/artifact, relative to game | SHA-256 |
| --- | --- |
| `core/include/overkill/core.hpp` | `1e5741b9a45c1a5922976371e1111eca4cfa7325e1de083c9e0d9f81461684b6` |
| `core/src/core.cpp` | `7bb7813fe562c2e81d6d2892664330fb28374abe4a549bb6af57b9304b0cddca` |
| `core/src/serialization.cpp` | `c0ffcb4633b70317f10fb3cb564b122896f419df3b662a7963f51a0d9bd96206` |
| `core/build/Release/overkill_core_tests.exe` | `56773886909afa1e814ac083c29872c3ff8c2daf2765b7d25d413ed7a4dbbf04` |
| `core/build/Release/overkill_runner.exe` | `a81c753f2f647aba58246ee0dff3dd1b32c6231ed796d253812085e1ef374322` |
| `qa/build/Release/foundation_probes.exe` | `2565e6ef4ceb7bebdabbb2e1c2afa6c0da143009e8f710cdd909f24045d20d3b` |
| `qa/foundation_probes.cpp` | `8358148bb42e9f1b60e20d7cbb0aea419cfc3640d3b0e30de24d686645349b0e` |

Runner startup/no-argument help exits 0 and unknown-option help exits 2 on this version. Schema 1 migration was not added or required for this pre-delivery technical increment; earlier development snapshots are incompatible with schema 2.

**Not run in this assignment:** Unreal adapter/graphical parity; rendered or packaged launch; native input/capture/focus; audio; display modes, resolution and performance; human play or visual approval. No meaningful audio or exit-to-desktop gameplay is available in the console fixture. Native process startup/normal completion is covered by the executions above; that is not a packaged Windows-game smoke test.

**Known staged limits, not newly discovered bugs:** complete city, all 246 recipe effects/upgrades/robots, Mayor, Mystery runtime outcomes, shop/reward transactions, profile/Collection/campaign Continue and durable saves are not present in this foundation. The snapshot probes establish deterministic in-memory serialization only, not T11–T14/T18–T22 transaction/crash durability or original-entry Continue. T05/T06's specific Noor/later-city content is outside the selected MVP; P05+ effect infrastructure and applicable later equivalents need later review. T01–T04/T07–T10 have controlled tests here or in the executed suite, not full content-acquisition integration.

No human playtest observation or fun/visual-quality conclusion is supplied by this review. Subsequent campaign/host changes are outside these hashes and require their own bounded evidence.
