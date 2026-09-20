# Baseline-5: tile progress and Precision sensitivity

**20 September 2026.** The preregistered 400-run experiment produced **392 city completions and eight explicit held-rule stops**. There were no defeats, timeouts, unsupported choices or policy errors. **All 400 traces replay exactly**, including the rejected transactions: **150,218 commands**, ordered events and final campaign bytes. [Compact full results](baseline-5-results.json) preserve every policy/seed/model combination, trace hash, outcome, resource ledger, last loadout and actual recipe/effect counts.

The collection models are synthetic sensitivity assumptions, not measured human ability. These bots do not establish human difficulty, game feel, graphical parity, or all-content balance. The eight held cases remain unresolved outcomes, separate from gameplay losses.

## Policy repair and frozen identity

Baseline-4's [complete 200-run report](BASELINE-4.md), including aggressive seed 57's timeout, remains unchanged. Baseline-5 assigns progress value to removing visible finite tiles. Mark or shot bonuses trapped behind a tile have no usable next-hit damage value, and stored shot bonuses share the remaining damage opportunity with already-valued statuses. This prevents a retained bonus from making a useful attack look worse than indefinite storage. These are policy scores; no game defense, bonus, payment, damage, resource or action limit changed.

The strict Release build and runner CTest pass **30,090 author assertions**, including real Ballistic Starter plus three-tile depletion and combined Mark/bonus kill regressions. Four actual old timeout saves recover in **16 / 6 / 9 / 19 legal commands**: baseline-2 aggressive 7/9, defensive 13, and baseline-4 aggressive 57. Eleven fresh regression cities complete and replay; aggressive 57 now completes at 73 HP in 363 commands. These inspected seeds are development data, separate from the experiment below.

Independent QA passed **32 groups**, 5,162 exact command/event/final-byte replays across 16 trace artifacts, 12 integrity checks plus old-version rejection, 80 stopped-combat and 20 concealed-route information checks, all category distributions, and the natural held-rule control. See [the independent report](../../qa/city-runner-review.md). All **30 reviewed source hashes still match after the 400-run batch**. The executable and rules remained unchanged throughout.

| Identity | Value |
|---|---|
| Prior committed checkpoint | `3e63131e5127cc7f054ef24d62a1451adfa22ef6` |
| Policy version | `baseline-5` |
| Rules / content | `of-core-0.4` / `cinderwall-upgrades-0.3` |
| Manifest SHA-256 | `d56bf01290f16567cd7009a64f4d49f825454a7149fbf6a922a695a4b71deab9` |
| Evaluated executable SHA-256 | `462c641183d6fbada187fe339e451ff14de6b90ecc4e152e69afdb0d6b9c9079` |
| `policy.cpp` SHA-256 | `ab1365fad02c8cf90cafbd56ecaa6fcfa557411d7a99293e12c625328f61a5fe` |
| Preregistered protocol SHA-256 | `d2bb2522d69edab861692873220c718c78863cbf6b47a258c4fe1b17a577297e` |
| Complete replay index SHA-256 | `bfaf8cbdf38b769d2bef17e687c0530511ee4c9db48ba0abcaa3fbf15fd7bb85` |

## Fixed experiment

The protocol was written at **15:33:47 UTC before the first run**: fresh seeds **121–170**, both policies, all four collection modes, **50 runs per cell / 400 total**, 1,200 previews per decision, default 30-second runner watchdog. Runs were sequential within each batch. No result was replaced, seed omitted, or numerical composition supplied for an unresolved rule.

| Collection assumption | Miss / Good / Perfect | Expected Precision bonus |
|---|---|---:|
| Auto | No attempt; recorded input `-1` | 0 |
| Learning | 55% / 35% / 10% | 0.55 |
| Practised | 20% / 45% / 35% | 1.15 |
| Expert | 5% / 25% / 70% | 1.65 |

The full distributions come from `design/data/beta-balance-v1.json` (SHA-256 `39ca65256ee02de8288b1610bb0cffb053d64920fcd64a9e1ac748909894887b`). Auto preserves the manual opportunity; it does not execute a failed Precision attempt. The protocol also retains the source's zero-bonus Auto distribution for provenance. Optional source-defined retries remain explicit commands. The compact results' collection-input counts cover committed Collect commands and exclude typed retry answers.

| Policy / mode | Complete | Held rule | Completed HP mean | Boss rounds mean | Boss shots mean |
|---|---:|---:|---:|---:|---:|
| Aggressive / Auto | 50 | 0 | 75.34 | 3.44 | 7.64 |
| Aggressive / Learning | 48 | 2 | 75.88 | 3.42 | 7.71 |
| Aggressive / Practised | 47 | 3 | 76.28 | 3.34 | 7.60 |
| Aggressive / Expert | 47 | 3 | 76.79 | 3.17 | 7.38 |
| Defensive / Auto | 50 | 0 | 79.86 | 6.58 | 14.02 |
| Defensive / Learning | 50 | 0 | 79.98 | 6.36 | 13.88 |
| Defensive / Practised | 50 | 0 | 80.00 | 6.24 | 13.74 |
| Defensive / Expert | 50 | 0 | 80.18 | 6.10 | 13.72 |

Every cell has zero actual defeats and zero runner failures. HP and boss means above include completed cities only. Per-cell income, purchases, healing, HP costs, material flows, loadouts and actual effect counts are in the full results. Mean gross flows should not be compared as equal-length experiments when a held rule ended some runs early.

Within pairs where both the tested model and Auto complete, Learning / Practised / Expert change aggressive final HP by **+0.35 / +0.49 / +1.00** on average (48 / 47 / 47 pairs), and defensive HP by **+0.12 / +0.14 / +0.32** (50 pairs each). Mean boss rounds change by **−0.02 / −0.11 / −0.28** aggressive and **−0.22 / −0.34 / −0.48** defensive. These are conditional observations from these policies. Extra supplies can change subsequent actions, acquisitions and encounters; the pairs do not isolate a universal value per Precision point.

Recorded run time totals 135.183 seconds; the slowest resolved run was defensive/Learning seed 155 at 0.835 seconds. Generation plus all exact replays took 279.473 seconds on the shared development machine. Timing includes policy previews, serialization and file IO, rather than in-game time.

## Held-rule cases

All eight stops came from UGS-141's additional base-haul composition, with the exact reason: `The authored combined base-haul composition awaits the owner decision.` Each rejected Collect left the previous campaign hash unchanged and returned zero events. Player HP remained positive. No pending owner answer was guessed or avoided.

| Aggressive seed | Learning stop | Practised stop | Expert stop |
|---|---|---|---|
| 150 | Position 8, HP 79 | Position 6, HP 79 | Position 6, HP 79 |
| 154 | City complete | Position 6, HP 73 | Position 6, HP 73 |
| 160 | Position 8, HP 80 | Position 6, HP 80 | Position 6, HP 80 |

Auto and every defensive cell completed these seeds. More successful synthetic Precision attempts can expose the unresolved voucher earlier, so lower completion counts here are **not** evidence that greater skill is harmful. No combined −4/−6 deduction case occurred in this experiment; those authored composition gaps remain separately owner-held.

Before launch, historical aggressive/Expert seed 53 independently verified the reporting boundary: 185 commands, HP 74, `blocked_rule` / process exit 2, unchanged rejected state hash `b7e1f4a8becdc82c`, zero events, and exact replay / exit 0. Its trace SHA-256 is `ef64ebe24650912a94bb2873127d588dc3ee46af485048a23ed27f95d5e8b9b3`. This control is stored separately and is not one of the 400 fresh cases.

## Reproduction, limits and next work

Bulk artifacts remain under ignored `core/build/runner-baseline-5/`: preserved executable/source identity, author regression and recovery logs, the historical held-rule control, and `sensitivity-121-170/` containing the preregistered protocol, eight complete reports, all 400 traces, replay index, summary and post-batch source verification. [Compact full results](baseline-5-results.json) are committed-size evidence without the bulk traces. The eleven development runs remain in `core/build/runner-baseline-5-pilot/`.

Example exact replay, including an honestly blocked result:

```powershell
& games/overkill-foundry/core/build/runner-baseline-5/overkill_runner-baseline-5.exe --replay games/overkill-foundry/core/build/runner-baseline-5/sensitivity-121-170/aggressive-learning-150.oftrace
```

Use [the build instructions](README.md) to compile. Repeat a cell with `--batch 121,50,aggressive,learning` and a new output directory; substitute the declared policy/mode for the other seven cells. Do not overwrite the retained experiment.

The report found no further policy failure in this finite sample. Shallow planning, incomplete exploration of recipe combinations, future-rules uncertainty and uncalibrated human Precision remain limitations. The two actual earned combinations in the [baseline-4 report](BASELINE-4.md) still demonstrate viable observed Heat/Burn and Shield/Braced Shot sequences through the unchanged game rules; they do not establish superiority or comprehensive build diversity. Owner-held composition decisions are the dependency for evaluating the eight blocked paths fully. Human play, rendered claw calibration, visual approval and packaged-game checks remain separate acceptance work. No gameplay balance, assets, dependencies or paid services were changed.
