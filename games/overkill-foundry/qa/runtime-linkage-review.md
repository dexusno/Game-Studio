# Partial runtime-linkage review — 20 September 2026

The corrected partial map passes this finite evidence audit: **133/665 obligations bound, 532 explicitly unbound**. Five sampled bindings overstated their cited assertions; the author removed all five and recorded the missing clauses. This closes the mapping findings without claiming that the underlying game behavior was repaired or fully tested. The unchanged release gate correctly refuses the remaining 532 obligations.

## Artifact and scope

Independent reviewer: `foundation_qa`; implementation/evidence author: `core_effects`. Windows x64, VS 2022 Release/C++17 native capture; review used PowerShell, Python, Git, and source inspection. No UE launch, native input, packaged-game playtest, or new execution of the eleven gameplay suites occurred in this review. `--run` was not used and the frozen native capture was not overwritten.

The author-operated capture began at `2026-09-20T16:23:59.338716+00:00`, from revision `5647174d6fde6d7c6bd54cdd1dfff023d1048bba` plus the recorded reporting changes. Its new executions of unchanged independent probe sources remain author-operated evidence. They do not replace the historical independent QA runs.

The semantic sample was **12 selected obligations**, not all 138 initial bindings: SH036, SH043, SH070, SH085, MA059, MA096, MA102, UGS-035, UGS-055, UGS-074, UGS-116, UGS-142. Selection emphasized thresholds, finite allowances, installed source identity, saved costs, and clock renewal. The complete evidence identities and mechanical linkage were checked separately.

| Reviewed identity | SHA-256 |
| --- | --- |
| Unchanged `content/runtime-evidence/results.json` | `93d22967306e4974f435ec71253864dd95d2af3ee4e2ef107d147b047d045646` |
| Shared `overkill_core.lib` | `e071b631023020f3e880c2a691306c1c21de9ad65fde25c12a90d124c8657a33` |
| Manifest | `d56bf01290f16567cd7009a64f4d49f825454a7149fbf6a922a695a4b71deab9` |
| Capture tool | `fcd818aab97b4809859a58b61369c89b294968a5f9714aa83f9cefed1818cd66` |
| Corrected `runtime-support.json` | `c3eb16d905ff3f93e592439653929f0cfdacfa88483a68a91502bed8e67f817d` |
| Corrected `coverage.json` | `6a14488fc8c5313a040d28556e2fe8e7f39e63ddd38826edd194adf164808802` |
| Corrected `runtime-binding-cases.json` | `918535eec6e25676c1c2b30eefbb27dac1a54675376c9c6aaa91e6b5075c29c9` |

The [compact independent identity check](runtime-linkage-evidence.json) records all eleven executable hashes, log hashes, and linkage checks. The initial 138-binding support hash was `f9cace6a9a9140bb3c480aaf279193ee94a08b2805fdd53acb6645a82f51ed85`; initial cases `bf488b8a2f0b4957362d551a774a2885f288c30e22315add122c71f8631db675`; initial coverage `f8f38b8903f4b9580c940fbbd5f447701326517a46ec5a61892cc4fb39d0cd4f`. The author preserved that candidate locally under `core/build/runtime-linkage/review-138`.

## Findings and closure

All five were **Medium: evidence sufficiency**, on the initial 138-binding candidate above. Reproduction consists of resolving each binding's fixture selectors and comparing the actual setup/assertions with the selected catalogue clause. These are concrete missing checks, not observed runtime failures. Each could admit an incorrect implementation while its cited assertions still pass.

| Finding | Source contract and actual cited evidence | Expected evidence / correction |
| --- | --- | --- |
| RL-01 — SH043 bypass cap and consumption | `design/RECIPE-CATALOGUE.md:306` specifies up to 12 damage from the next main shot. The only linked case, `qa/expanded/probes.cpp:55`, fires SH001 6 against Armor 3/Shield 20 and observes 3 HP damage with Shield unchanged. It never crosses 12 or fires a second shot. | A shot above 12 must demonstrate the bypass cap and ordinary treatment of the remainder; another shot must demonstrate allowance consumption. **Removed from bindings; explicit unbound reason verified.** |
| RL-02 — SH070 minimum-cost eligibility | Catalogue line 333 requires the first next-turn Use costing at least 2 Iron. `qa/expanded/probes.cpp:84` uses SH104 costing 2 Iron before the scheduled turn and twice during it. The current-turn exclusion and one discount pass, but no 1-Iron Use precedes the eligible Use. | A 1-Iron Use must pay normally and preserve the discount for the later qualifying Use. **Removed; explicit unbound reason verified.** |
| RL-03 — MA102 remembered Heat cap | Catalogue line 563 caps the remembered Heat at 8. `core/tests/recipe_tests.cpp:96` captures 8 and 3; `qa/expanded/probes.cpp:73` removes the 8 source and proves that the installed 3 source wins. Neither starts above 8, although Mara can hold 14. | Capture above 8 and verify that post-phase restoration is capped at 8. **Removed; explicit unbound reason verified.** |
| RL-04 — UGS-116 turn renewal | `design/UPGRADE-CATALOGUE.md:1881` grants the discount each turn. Both U35 (`qa/upgrades/probes.cpp:144`) and the opening-production fixture (`core/tests/upgrade_tests.cpp:62`) exercise only the first and second matching Uses in round 1. A once-per-fight allowance would pass these assertions. | Advance the turn and demonstrate a renewed discount, with the same selected material. **Removed; explicit unbound reason verified.** |
| RL-05 — UGS-142 qualifying failure/victory gate | Upgrade catalogue line 2297 requires a failed Precision attempt in the previous victorious fight. The finite-lease fixture (`core/tests/upgrade_tests.cpp:109`) directly sets two failures, sends Victory, then verifies one Iron and consumption. It does not test zero failures or failed-but-Escaped completion. | Both ineligible cases must produce no voucher, alongside the existing positive/consumption case. **Removed; explicit unbound reason verified.** |

The seven other sampled entries supplied direct useful assertions: SH036 preserves the original stored +4 while its copy loses the attachment; SH085 checks optional 0/4 payments, rejected amounts, ordered Shield drain and damage; MA059 checks removed-source exclusion and independent physical sources; MA096 checks selected payment, rejection and saved reinstall history; UGS-035 checks five fights, a sixth without the bonus, two shots per fight and escape consumption; UGS-055 checks actual-payment qualification and the first-two cap; UGS-074 checks the exact half-HP boundary versus above half. These observations are not a certification of every clause or interaction for those entries. The other **126 still-bound obligations were not semantically audited in this finite sample**.

## Executed and inspected checks

- **Passed:** independently matched all 37 captured input hashes, all eleven existing executable hashes, all eleven log hashes, the shared library, and all 217 unique named PASS results against `results.json`. No missing local binary was silently skipped. All eleven copied fixture sources match their originals byte for byte, and their generated projects link the shared library.
- **Passed:** inspected all four author test diffs (`robot_tests`, `route_tests`, `campaign_tests`, `campaign_upgrade_tests`). Removing only the added literal PASS output statements leaves the same code after whitespace normalization; assertion bodies and call order are unchanged. The robot capture enables actual Engine integration. This does not make controlled campaign callbacks or terminal ammunition ordinary campaign play.
- **Passed:** `python -X utf8 games/overkill-foundry/tools/capture_runtime_support.py --check --self-test`, before and after correction. All seven controls reject invented fixture, stale implementation, stale evidence, wrong manifest, wrong build, empty handler and unknown operation. The final exit is 0 at 133/665.
- **Expected refusal:** `python -X utf8 games/overkill-foundry/tools/compile_content.py --check --release` returns 1: 527 unsupported obligations initially, 532 after correction. This is a working release restriction, not a test failure to waive.
- **Passed recheck:** the five findings are absent from final bindings and present as explicit unbound decisions. The 37 inputs and `results.json` are unchanged; the metadata correction asserts no new native test pass.

The final map is suitable to retain as a **partial evidence ledger**. It is not independent semantic approval of all 133 bindings, a completed 665-obligation gate, a whole-city runtime acceptance, or evidence of balance, visual quality, audio, input usability or fun. All 207 physical-output obligations and the explicit owner-held composition rules remain unbound. Further binding additions require their own clause-to-assertion review.
