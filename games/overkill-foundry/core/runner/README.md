# Cinderwall numerical runner

This executable plays the implemented Mara city through production `CampaignRules` and `cinderwallUpgradeHooks`. Combat candidates use `Rules::preview`; chosen commands use the same `apply` path as a player. It starts with canonical New Game resources, pays real costs, sells real cores, chooses fixed rewards, exchanges physical recipe copies, visits shops and Mysteries, and can defeat the boss. It adds no gameplay rules or resource grants.

This is an early local baseline for finding defects and measuring conditional numerical outcomes. It does not establish human difficulty, game feel, graphical parity, or visual approval. Other mercenaries, later cities and Lockdown are outside the implemented campaign.

## Build and run

From the repository root, with the verified Visual Studio 2022 CMake installation:

```powershell
$runnerCmake = 'C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe'
$runnerCtest = 'C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/ctest.exe'
& $runnerCmake -S games/overkill-foundry/core -B games/overkill-foundry/core/build
& $runnerCmake --build games/overkill-foundry/core/build --config Release --target overkill_runner overkill_runner_tests
& $runnerCtest --test-dir games/overkill-foundry/core/build -C Release -R runner_contracts --output-on-failure

& games/overkill-foundry/core/build/Release/overkill_runner.exe --fixture
& games/overkill-foundry/core/build/Release/overkill_runner.exe --city --seed 2 --policy defensive --precision auto --output games/overkill-foundry/core/build/city-runner
& games/overkill-foundry/core/build/Release/overkill_runner.exe --batch 21,100,aggressive,auto --output games/overkill-foundry/core/build/runner-baseline-4
& games/overkill-foundry/core/build/Release/overkill_runner.exe --batch 21,100,defensive,auto --output games/overkill-foundry/core/build/runner-baseline-4
& games/overkill-foundry/core/build/Release/overkill_runner.exe --replay games/overkill-foundry/core/build/city-runner/defensive-auto-2.oftrace
```

`--fixture` retains the legacy Mite/Ram encounter. `--city` and `--batch` emit one JSON line per run and write an `.oftrace` file per seed. `--replay` applies the recorded commands without invoking a policy. Outputs belong under ignored `core/build` or `.local`; do not commit bulk traces.

Default `--search-budget 1200` limits preview calls per policy decision. Default `--timeout 30` is a wall-clock watchdog checked between commands. Neither changes legal actions, inventory, parts, shots or turns. A timeout is a stopped runner, not a game loss; its exact stopping point can vary with machine load. A single core call is not interrupted mid-transaction.

Exit `0` means the run reached a city completion or an actual defeat, or a replay verified. Exit `2` means invalid input, replay mismatch, a blocked rule, unsupported choice, policy rejection or runner timeout. Batch JSON preserves each distinct outcome even when the process returns `2`.

## Policies and information boundary

`baseline-4` contains two deterministic local numerical policies:

| Policy | Main preferences |
|---|---|
| `aggressive` | Damage, Heat and Ammo; visible Officer routes; higher willingness to spend resources. |
| `defensive` | HP protection, recovery, Shields and conserving materials; ordinary/Mystery routes. |

Both evaluate current legal action previews, including physical-part creation followed by installation or assembly and Fire. Candidate bullets include all available Ammo, all Ammo plus Spreads, singles and prefixes. Every available part can enter a candidate; the preview budget bounds exploration. Typed targets, sacrifices, variable payments, physical-copy choices and nested acquisition decisions are passed to the real validator. Failed previews do not mutate live state.

These are shallow policies, not exhaustive solvers for the 246-recipe pool. They can miss profitable target allocations, combinations, saved-part plans and long-term synergies. They currently buy recipes/upgrades and sell cores; they do not optimize material purchases, part sales, route replacement, reward rerolls or every optional haul exchange. They may acquire a recipe that their candidate generator uses poorly. An illegal submitted command is reported as `policy_error`, never silently replaced with free effects or counted as defeat.

Policies read committed enemy intents, visible defenses/statuses, owned recipes/upgrades, currently displayed stock and offers. They do not preview End Turn, inspect future robot pattern packets or core RNG, or plan from future seeded draws. A preview can reveal the immediate consequence of the action being considered. Visible boss Shield and death-spawn transitions are credited as progress. Current unprotected threat is penalized, so eliminating an attacker does not look like losing valuable Shield. Pending damage statuses are valued below the target's remaining HP: surplus Mark, Burn or Corrosion cannot make retaining a nearly dead target more valuable than killing it. These are scoring choices; the game statuses are unchanged. Newly produced reserve outputs receive one bounded lookahead, without repeatedly revaluing the entire old reserve for each candidate.

Before entering Mystery, only `revealedMysteryCategory` can reveal its category. Hidden Mystery identifiers and formations are ignored. OpenShop is an actual recorded action, so shop discoveries use production behavior. The seed initializes canonical content and the separate declared Precision model; it is not used to rank hidden future outcomes.

## Collection assumptions

`auto` uses automatic collection and does not spend the optional Precision attempt. Other models sample one recorded Precision result on the first collection of each fight. A source-defined retry choice can supply another sample; ordinary later rounds cannot.

| Uncalibrated model | Miss / Good / Perfect | Expected Precision bonus |
|---|---|---:|
| `learning` | 55% / 35% / 10% | 0.55 |
| `practised` | 20% / 45% / 35% | 1.15 |
| `expert` | 5% / 25% / 70% | 1.65 |

These full distributions come from `design/data/beta-balance-v1.json`; baseline-2's mean-equivalent Practised mismatch is recorded in [the pilot report](BASELINE-2.md). The model has its own deterministic random state. All haul arithmetic, stock availability, upgrade modifiers and Precision eligibility remain authoritative core rules. These distributions have not been calibrated against rendered claw input or people.

## Evidence format and metrics

`OFCITY2` stores rules/content versions, manifest hash, policy version/name, collection assumption and preview budget, followed by the complete initial campaign bytes. Each command includes every `CampaignAction` and nested `Action` field. Its success/reason, resulting state hash and exact ordered event JSON follow. The final record includes the complete resulting campaign bytes and outcome.

Replay checks the supported metadata and its agreement with the initial run identity, canonical Mara New Game, every command/result/event, each state hash, byte equality of the final snapshot, and the final outcome against the campaign phase. Truncated records, extra fields, trailing data, changed event payloads and false victory/defeat labels reject. The format is a reproducibility artifact, not an authentication mechanism. Early development `OFCITY1` artifacts are not supported by this reader.

JSON includes city outcome/reason, final HP/position, commands, previews and elapsed seconds. Each fight includes HP, rounds, shots, kills, recipe/upgrade IDs at entry, and compact collection/paid-use/fire choices. Nested-choice entries reference transaction sequence numbers in the exact trace. Entry lists preserve duplicate recipe copies; exact copy IDs, tags and all targets/payments are in the serialized trace.

Metric definitions:

- HP at fight end includes effects resolved by the atomic victory transaction. Entry HP is the value when the campaign enters Fight; any later pending start choices are recorded commands.
- Income is positive credit inflow; spending includes the real discounted purchase price even when buying also awards credits.
- Healing and combat HP loss use actual event amounts. HP paid includes recipe/acquisition events plus the Technician service's actual residual debit after its nested upgrade events, without counting that acquisition cost twice.
- `materials_spent` sums the actual paid vector of `recipe_used` events. `materials_gained` is the sum of positive net inventory changes per command, not gross production before simultaneous conversions/spending. Vector order is Iron, Copper, Carbon, Glass, Circuit.
- City `choices` lists selected definition IDs; fight `choices` is compact context. The trace is the complete ordered choice record.
- Wall-clock time includes policy previews, serialization and trace IO. It is throughput on this machine, not in-game time.

At the native `842300e` baseline, UGS-141 and combined base-haul deductions beyond the specified order reject pending owner decisions. Such a command produces `blocked_rule`, preserves the rejected transaction/state, and is excluded from gameplay losses. No alternate haul is substituted.

## Validation and next experiment

The runner contract target checks full command encoding, strict trace replay/corruption rejection, two real city runs, Technician HP accounting, concealed-Mystery and future-pattern/RNG decision invariance, Chassis death-spawn and protected-attacker scoring regressions, and 10,000 samples per Precision model with separate category/mean assertions plus once-per-fight eligibility. The sample counts verify the declared synthetic distributions only.

The [baseline-2 development pilot](BASELINE-2.md) retains all 40 outcomes, including three policy stalls and four defeats. Baseline-3 corrected the fresh-run stalls and Practised distribution, but independent recovery tests found that excess Mark still stalled two retained saves. Baseline-4 addresses that valuation defect. Seeds 1–20 remain development data. After regression and independent review, the next comparison uses fresh paired seeds 21–120 and Auto collection. Every loss/error/blocked run stays in the report. No winning seeds or favorable rare builds are forced, and these numerical runs do not establish human difficulty.
