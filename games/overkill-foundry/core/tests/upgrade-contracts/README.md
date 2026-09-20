# Upgrade contract evidence

This isolated CPU-only target reviews the 100 upgrade/Mayor obligations left unbound by the historical 133-binding runtime ledger. It executes 96 exact upgrade cases and supplemental clause tests through the real shared engine and production campaign hooks. It changes no active support binding.

The proposed evidence covers 94 IDs. UGS-011, UGS-018, UGS-126 and UGS-141 remain unproposed because their combined haul compositions await owner decisions. MAU-05 retains an unproved full-HP-at-post-reaction-trigger branch; its actual-healing cap, reaction order, death interruption, full-HP-before-Use and turn renewal are exercised. UGS-029 exercises all three enemy debuff types Cinderwall can produce; the fourth distinct type needed to demonstrate the explicit cap is unavailable in that roster, and no eligible self-inflicted debuff producer is demonstrated. Those two full bindings remain unproposed.

The UGS-031 normal-rarity interpretation follows the owner's 20 September clarification in `design/UPGRADE-SYSTEM.md`. No rarity weights or manifest were changed. The selected clarification is captured beside the unchanged manifest, whose identity is recorded in the evidence; canonical catalogue/manifest synchronization belongs to the integration owner.

Run from the repository root:

```powershell
python games/overkill-foundry/core/tests/upgrade-contracts/generate_contracts.py --check
cmake -S games/overkill-foundry/core/tests/upgrade-contracts -B games/overkill-foundry/core/build/upgrade-contracts -G "Visual Studio 17 2022" -A x64
cmake --build games/overkill-foundry/core/build/upgrade-contracts --config Release --parallel 4
ctest --test-dir games/overkill-foundry/core/build/upgrade-contracts -C Release --output-on-failure
python games/overkill-foundry/core/tests/upgrade-contracts/capture.py --check
```

`capture.py --capture` performs a clean strict MSVC build, runs all four executables and CTest, verifies that inputs stayed unchanged, then copies every input and executable/library/log to an ignored immutable capture directory. `--check` verifies identities and proposed linkage without claiming a new execution. Exact commands, named PASS records, assertion totals, source hashes and artifact hashes are in `evidence/results.json`. The proposed-only map and all residual reasons are in `evidence/proposed-bindings.json` and `contracts.json`.

The existing upgrade, production campaign-upgrade and campaign transaction suites are compiled unchanged in this separate build. Their executions are new regressions against this provisional candidate; historic independent logs are not relabelled or overwritten. No graphical behavior or human game feel is claimed.

## Controlled inputs and boundaries

Paid recipe Uses and positive-price acquisitions exercise actual payment, ordering and atomic rejection. Tests explicitly supply materials, known recipe copies, enemy states, stock or reward candidates to isolate a printed clause. Controlled base-valued cores check per-core integer rounding. A fixture-only 500-damage plain Ammo factory grant closes real campaign encounters so rewards run through `CampaignRules`; it is not naturally available equipment or a balance result. These controls must not be interpreted as natural loadout reachability.

UGS-062's additional restricted content pool isolates the Shared Modifier selector with SH051 as its only matching candidate. Actual production pools are unchanged. Controlled `CityStart` notifications verify campaign-scope counters across that lifecycle boundary; there is no second playable city in this test. UGS-045 additionally wins a controlled fourth Officer through real damage and the production completion hook to exercise its cap beyond the three Officer opportunities in Cinderwall.

Saved choices, canonical snapshots and transaction receipt replay are exercised. Remaining composition and save/version decisions are held. This candidate keeps the provisional corrected `of-core-0.4` and does not establish compatibility with archived binaries that report the same version.

## Two reproduced defects and bounded corrections

- UGS-095 omitted the six physical spreading recipes from the source Modifier category. The analogous UGS-062 additional-option filter also omitted spread recipes. Only those two category predicates now include `Kind::Spread`; loading, firing and planning placement retain their distinct kinds. The failed executable, library, log and 35 copied inputs are archived under the path in `history/ugs095-failure.json`.
- UGS-085 lacked its explicit acquisition purchase-price invariant. Controlled shop prices at or below 100 were accepted and then granted 100 Credits. `CampaignRules::Buy` now rejects that specific upgrade unless its original product price exceeds 100, before any payment. Discounted final prices below 100 and nonpurchase Officer reward acquisition remain valid. This was a controlled-stock invariant failure; no naturally offered exploit was demonstrated. The failed build and 37 copied inputs are preserved under `history/ugs085-failure.json`.

Root authorized these corrections after the native reproductions. They change outcomes and join the pending version/migration decision. The old 133-binding ledger and physical lifecycle captures remain historical and untouched. No full binding or release gate is activated here.

For independent review, start with each exact source effect, the named group in `contracts.json`, and its copied C++ assertion body. Review sufficiency rather than accepting the count or broad suite result. Rebuild from the capture's `inputs/core/tests/upgrade-contracts` directory when an independent executable identity is needed.
