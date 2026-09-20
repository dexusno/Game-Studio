# Independent upgrade contract review — 20 September 2026

**The frozen candidate does not support all 94 proposed bindings. UGS-121 fails an explicit copy restriction.** The three requested predicate repairs pass. Of the 100 obligations examined, 93 have provisional finite evidence, one is rejected and six retain their existing residuals. No active binding, version or production file was changed by QA.

The exact reviewed graph is `49b5f5045b558856d2ceeea4f10639bb2c3c62f209817784a649dc5d96f5ce38`, containing 54 copied inputs. The author's executable is `3fae26e0acb5b90dd1de6cc120516dd166262662cf86a40c8393f90bd56f978b`. Source inspection and builds use the capture at `core/build/upgrade-contracts/captures/native-upgrade-contracts-3fae26e0acb5b90d-49b5f5045b55/inputs`, not a later working tree. The provisional corrected `of-core-0.4` includes outcome changes; save/version/migration approval remains unresolved.

## Executed checks

On Windows 10.0.26200 x64, MSVC 19.44.35228.0, Release, C++17, `/W4 /WX /permissive-`:

| Check | Observed result |
|---|---|
| Independently rebuild unchanged captured CMake project | Success; all four CTests pass: 6,139 contract, 1,130 legacy upgrade, 255 production adapter and 784 campaign assertions |
| New QA probes using that independently rebuilt library | Nine groups pass, Q09 fails; 544 assertions executed; exit 1 |
| UGS-095 paid Spread category | All six source spreading recipes grant exactly one Shield 7; grants and a genuinely zero-resource Use do not consume the paid allowance; preview/reload/event parity passes |
| UGS-062 fourth Shared Modifier option | Four Shared Spread rarities admitted through actual second-victory adapter requests; duplicate candidates and Mara exclusion checked with restricted controlled pools |
| UGS-085 purchase invariant | Original prices 99/100 reject atomically; 101/102 with 20% discount allow only fully affordable purchases, granting 100 once; stale receipt and duplicate acquisition checked |
| Archive and source linkage | 144 archived identities including both prior failed captures match; all 100 JSON source rows and 106 named results match; four malformed-link controls reject |

The independently rebuilt contract executable is `ac88c82ba4fe0246bd7057f7ad6f432accb95da490af51f9eb39b46e1a79660b`. The final frozen QA executable is `2562f156ab6b3b14ae4eef7332d9a589e0e6f166920481a4932430d1967d2273`. Full hashes, controls, source inventory and explicit ID sets are in [upgrade-contract-evidence.json](upgrade-contract-evidence.json). Concise execution records are in [upgrade-contracts](upgrade-contracts/README.md).

## UC-01 — S2 gameplay correctness: Jig bonus copied onto a second slug

**Status: reproduced on the frozen graph; root authorized a separate narrow repair, not yet independently rechecked.**

Source `design/data/shared-upgrades.json`, `/upgrades/120/effect`, limits UGS-121's +2 to one physical slug even when a copy effect creates more. The proposed fixture at `core/tests/upgrade-contracts/production.inl:14` tests separate Uses, grants and renewal, but never the explicit copy clause.

Reproduce with Q09 in [probes.cpp](upgrade-contracts/probes.cpp): acquire UGS-121, then UGS-015; start a fight, collect, and pay for SH001. Save/reload the resulting original and Lease copy, load both and Fire at an unarmored 100-HP target. Expected damage is **14** (6 + 2, then 6). Actual damage is **16** because both parts retain +2. With an earlier UGS-046 +1 tag, expected **16**, actual **18**. Reversing Jig/Lease acquisition order produces the correct 14/16.

`core/src/upgrade_effects.inl:207` adds Jig to the original; line 210 then copies its aggregate `upgradeDamage`. The repair must exclude Jig's contribution while retaining independently allowed committed bonuses. Q10 confirms later SH118 printed-effect copies already discard Jig and other attached bonuses correctly. Original failed binary, source and log remain in `core/build/qa-upgrade-contracts/uc01-failure`; the expanded four-order/control execution is archived separately in `frozen-49b5f504`. Neither is a passing capture.

## Clause sufficiency and proposed ID set

I compared the 100 listed source effects with the 106 cited fixture bodies, rather than accepting registration or PASS labels as effect coverage. The per-ID source, fixture and disposition are recorded in [clause-review.json](upgrade-contracts/clause-review.json).

- **89 IDs:** original cited assertions provide provisional coverage of the examined local clauses; the explicit list is in the evidence JSON.
- **Four additional IDs need the new QA evidence linked:** MY2-01's fresh Copper when none was paid (Q06); MY1-08 and MY1-11 rewards after actual MA058 recoil, including lethal interruption (Q07); UGS-016's tenth acceptance as well as its fifth, through persisted reward/exchange transactions (Q08). These pass, but those branches are absent from the original cited groups.
- **UGS-121:** reject the full binding until UC-01 is repaired and rechecked.
- **Six unchanged residuals:** UGS-011/018/126/141 have owner-held haul compositions; MAU-05 lacks the exact post-reaction full-HP opportunity branch; UGS-029 lacks the fourth distinct debuff cap and an eligible self-applied case.

This yields **93 provisional IDs**, conditional on the four supplemental links, rather than acceptance of the unchanged 94-ID proposal. It does not establish all combinations of those upgrades or acceptance of all 152 eligible upgrades. UGS-031 follows the captured owner clarification in `design/UPGRADE-SYSTEM.md:5`, rather than silently treating the older JSON wording as a new rarity decision.

## Controls, preserved failures and limits

The author suites use prepared materials, stock, copies, reward candidates, a 500-damage terminal fixture and controlled later-city notifications/fourth Officer. QA uses real paid recipe actions and production campaign hooks, with prepared materials/enemy HP, selected reward cards and restricted category pools. Those controls isolate rules; neither suite demonstrates natural loadout probability or city balance.

The first QA run also retained two test-assumption failures: campaign receipt replay returns its original events, and natural cooldown skips the Use round. Corrected checks assert unchanged state plus identical receipt events, and an actual one-counter reduction rather than unconditional readiness. These were harness corrections, not game fixes; the original source, binary and output remain in `core/build/qa-upgrade-contracts/initial-capture`.

All execution was CPU-only. No Unreal launch, desktop input, graphics/audio assessment, packaged-build test, human playtest, shipping acceptance or save-compatibility claim is made. Historical active support and earlier archives remain unchanged. A later source repair requires a separately identified recheck.
