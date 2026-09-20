# Baseline-4: fresh paired Auto evaluation

**20 September 2026.** The frozen numerical policies completed 199 of 200 legitimate city runs on fresh seeds 21–120. Aggressive seed 57 stopped at the runner watchdog; it is a policy stall, not a gameplay defeat. Every result is retained. All 200 traces replayed exactly through the production campaign rules, covering **78,756 commands**, their ordered events, and final campaign bytes.

These are local bot observations. They do not establish human difficulty, fun, visual quality, or a balanced roster. The two policies take different routes and make shallow choices; their results are not a controlled comparison of identical encounter sequences. No rule, resource grant, seed, offer or enemy pattern was changed to obtain a completion.

## Candidate and review

Native rules/content remain `of-core-0.4` / `cinderwall-upgrades-0.3`; manifest SHA-256 is `d56bf01290f16567cd7009a64f4d49f825454a7149fbf6a922a695a4b71deab9`. The last committed runner baseline was `db09296`. The evaluated working candidate declares `baseline-4`.

| Artifact | SHA-256 |
|---|---|
| Author executable | `8e8d3eafd1240e66794af1ed5862df244b8eec96f6f3fd57d195936dff6de221` |
| `policy.cpp` | `246e58ce16d6221ef921709888361b0191c89e5cfe433186ff6628c47c5a1b36` |
| `city_runner.hpp` | `72006d52a8b4a21a2d0028164f8107eab028c2e31da7e60721509d1b7b45d0cc` |
| `runner_tests.cpp` | `37339b97692425273a0b5746eda332fb4208a3bcf6660bcce6907ad8db03a0ce` |

The strict Release build and runner CTest pass **30,052 author assertions**. Independent QA captured 28 matching inputs, built its own executable, and passed 23 probe groups, five fresh cities, 14 exact command replays (5,522 commands), 60 combat and 20 concealed-route information checks, 12 corruption/control cases, and previous-policy rejection. See [the independent review](../../qa/city-runner-review.md). The legacy fixture still completes at 80 HP, round 3, four shots, hash `965951ac6f2b9563`.

Changes since [baseline-2](BASELINE-2.md) are policy/runner only:

- Canonical Practised sampling is 20% Miss / 45% Good / 35% Perfect. Separate category assertions prevent the earlier mean-equivalent mistake.
- Immediate boss/death-spawn transitions count as progress. Shield scoring uses exposed current damage, avoiding a reward for keeping a protected attacker alive.
- Craft lookahead evaluates new outputs without repeatedly rescoring every old reserve part.
- Pending Mark/Burn/Corrosion value stays below the target's remaining HP; Weaken is already included in current threat. No game status is capped by this heuristic.

Baseline-3 passed all nine fresh development regressions, but independent recovery tests found that excessive Mark still stalled two retained baseline-2 saves. Baseline-4 resolves those three old fights in 16, 6 and 9 legal commands (aggressive 7, aggressive 9, defensive 13). All nine fresh regression cities complete and replay. Baseline-2 and baseline-3 binaries, source identities and traces are preserved under their separate ignored build folders. Seeds 1–20 remain development data.

## Fresh experiment

Both policies used Auto collection, 1,200 previews per decision and the default 30-second wall-clock watchdog. The reviewed executable stayed unchanged throughout. No action, turn, part or storage limit was added to the game. Runs were sequential within each policy batch; timings include previews, serialization and trace IO on the shared development machine.

| Metric | Aggressive, 100 seeds | Defensive, 100 seeds |
|---|---:|---:|
| City complete | 99 | 100 |
| Actual defeat | 0 | 0 |
| Runner timeout | 1 | 0 |
| Rejection / unsupported choice / blocked rule | 0 / 0 / 0 | 0 / 0 / 0 |
| Completed-city HP, mean (range) | 74.40 (48–98) | 80.54 (70–102) |
| Winning boss rounds, mean (range) | 3.51 (3–5) | 6.26 (3–11) |
| Winning boss shots, mean | 7.87 | 13.35 |
| Resolved-run time, mean seconds | 0.241 | 0.279 |
| All-run time, seconds | 53.914 | 27.908 |
| Commands, mean including timeout | 384.64 | 402.92 |
| Previews, mean including timeout | 11,584.71 | 7,643.19 |
| Gross credit income / purchase spending, mean | 255.22 / 292.90 | 171.00 / 215.00 |
| Healing / enemy HP loss / HP payments, mean | 6.22 / 11.52 / 0.24 | 10.84 / 3.93 / 6.37 |

There are 99 paired city completions and one aggressive timeout paired with a defensive completion. Average paid recipe material vectors (Iron, Copper, Carbon, Glass, Circuit) are `[115.99, 42.12, 25.73, 20.74, 7.26]` aggressive and `[111.14, 37.16, 26.34, 21.41, 9.83]` defensive. Those unconditional means include the long stall; they should not be treated as efficient-build costs.

Actual event use is narrower than available content. Aggressive/defensive produced 297/160 shots at Heat ≥6 across 66/32 runs; 397/452 total Burn was applied across 47/46 runs. Braced Shot's installed-Shield threshold was actually met 11/14 times across four runs per policy. Shield costs were actually paid in 9/10 runs, totaling 85/226 Shield. No hot White Casting shot occurred in this sample. Ownership alone is not counted as successful use of a synergy.

## Two naturally acquired combinations

These examples were selected from the unfiltered completed batch after execution. They show different functioning combinations in complete cities, not their causal advantage over an alternative build or deliberate long-term specialization by the policy.

**Defensive seed 46: Shield into Heat, then hotter Burn shots.** The run acquired MA014 Breathing Plate, MA037 Shield Boiler, SH013 Hot Filling and the Shield supply recipes SH057/SH060. It owned MY1-M1. Across the city it used MA014 eight times, MA037 four, SH013 eight, SH057 four and SH060 seven. Command 233 paid five installed Shield through MA037 and gained four Heat; commands 267, 289 and 333 repeated actual payments. Command 245 fired SH013 at Heat 10 for nine shot damage and Burn 2. During the boss, command 344 fired SH013 at Heat 7 for seven damage plus Burn 2, and command 358 did so at Heat 8 for eight damage plus Burn 2. These shots include Mara's normal Heat damage bonus. The city ended at **79 HP**, with **five boss rounds and ten boss shots**. Total Burn applied was 30, and 20 Shield was paid for the four conversions.

- Trace: `defensive-auto-46.oftrace`, SHA-256 `7fa2cd6a7b6fa851bfabcb29d3214b1653f3b7a6d1c83edf74943d193b2250b8`.
- Final campaign hash: `33cc329ee01648b6`.

**Defensive seed 80: cold Shield into Braced Shot's percentage bonus.** It acquired MA062 Cold Riveting and MA068 Braced Shot, with SH034/SH060 adding protection later. Its owned upgrades were MY1-M2 and UGS-065. MA062 was used ten times and MA068 fourteen. At command 153, MA062 supplied 16 installed Shield at zero Heat and MA068 fired for 12 damage: its printed 10 plus the integer-truncated 25% bonus. Commands 169 and 193 repeated that combination. The Shield threshold also held at commands 263, 329 and 367, including two boss shots. At command 367 the installed MA062/SH034 parts held 24 Shield at zero Heat. The city ended at **80 HP**, with **six boss rounds and fifteen boss shots**. Six Braced Shot percentage bonuses actually occurred; there were no Heat ≥6 shots or Burn applications. MY1-M2 modified two outputs earlier, rather than defining the later cold-Shield sequence.

- Trace: `defensive-auto-80.oftrace`, SHA-256 `a10db9a0560cf83b898388033447a73c2df71a621c44309ac11ecc91c7ad1a92`.
- Final campaign hash: `b4eeb783d5faf7eb`.

## Retained timeout and limits

**Aggressive seed 57** times out at position 10 against Foil Warden (C1-O02), after 2,784 city commands / 488,980 previews. Its final fight is at round 436 with zero shots; player HP is 80 and the Warden still has 44 HP and all three Ablative Tiles. The bot accumulated 360 reserve parts, Mark 366 and Weaken 300 on the enemy. The game accepted the commands normally.

The scoring omission is reproducible from the first actual preparation state, after command 265. UGS-001 supplies the unused +6 first-shot bonus. An ordinary paid SH001 → Load → Fire is legal and strips one finite tile while dealing one HP damage, but the policy values spending that bonus at three points and assigns no value to removing the tile: the Fire scores −2. Later accumulated Mark makes the refusal stronger. At the final save the normal decision uses only 412 of 1,200 previews; raising the budget does not change the choice. This is a valuation defect, not current search-budget starvation or a gameplay loss. No repair was mixed into this batch.

- Trace: `aggressive-auto-57.oftrace`, SHA-256 `dfff08ebb97d87ee0fd4728fa7f9af1ee7f2405610831c05ca6e509024ef427d`.
- Final campaign hash: `d8141e9ab1a7a740`; exact replay passes.
- Diagnostics: `timeout-57-inspection.txt` and `warden-entry-scores.txt` in the local artifact folder. The separate `.diagnostic` snapshot is a score-inspection input, not a valid completed trace.

The next useful policy increment should value depletion of visible finite defenses and then recheck this retained state without changing gameplay. Preserve this evaluation as baseline-4. Precision sensitivity remains separate work; UGS-141/additional base-haul composition decisions were still owner-held during this experiment, and no affected transaction occurred under Auto. The high completion rate warrants investigation with stronger encounter/build analysis and people, not a difficulty conclusion from these bots.

## Reproduction and artifacts

All bulk artifacts are ignored local files under `core/build/runner-baseline-4/`: the preserved executable and source hashes, both complete 100-row JSONL summaries, 200 traces, `replay-all.jsonl`, `summary.json`, actual-event `mechanics.jsonl`, selected `build-witnesses.jsonl`, and the evaluation/replay scripts. The nine development traces are separate under `core/build/runner-baseline-4-pilot/`.

From the repository root, replay an evaluated city with the preserved version:

```powershell
& games/overkill-foundry/core/build/runner-baseline-4/overkill_runner-baseline-4.exe --replay games/overkill-foundry/core/build/runner-baseline-4/defensive-auto-46.oftrace
```

Use the [runner build instructions](README.md) to rebuild. To repeat the experiment without overwriting evidence, run `--batch 21,100,aggressive,auto` and `--batch 21,100,defensive,auto` with a new `--output` directory. Timeout stopping points are wall-clock dependent; all completed command streams replay independently of policy timing. No assets, dependencies, services or paid tools were added.
