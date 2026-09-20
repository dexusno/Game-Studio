# Independent city-runner review

## Baseline-5 follow-up — complete

**2026-09-20 — finite independent recheck passed; unchanged graph released for the preregistered 400 sensitivity cases, seeds 121–170 × two policies × four Precision modes.** No evaluation seed from that batch was accessed by this review. The integration owner retains batch acceptance. Baseline-4's frozen evidence remains unchanged below; its subsequently discovered seed 57 policy stall prompted this separate increment.

The [30-file source capture](city-runner/evidence/baseline5-source-identity.json), taken at `2026-09-20T15:23:51.5273960Z` above revision `3e63131e5127cc7f054ef24d62a1451adfa22ef6`, matches both its frozen copy and live sources after execution. Only the runner header, policy, README and author tests differ from the reviewed baseline-4 graph; its two historical result records were added. **Rules, campaign and trace decoder are unchanged.** [Execution identity](city-runner/evidence/baseline5-execution-identity.json) records all harness/executable/evidence hashes.

- Author executable: `462c641183d6fbada187fe339e451ff14de6b90ecc4e152e69afdb0d6b9c9079`.
- Independent executable: `af6d7566ce1f3dbe2f670620b9b9a7ce7c46e177482ae025aac0d0433fc85b74`.
- Policy source: `ab1365fad02c8cf90cafbd56ecaa6fcfa557411d7a99293e12c625328f61a5fe`; header: `8077496b2aebbd12a79c583c12fd8089729652c95bbd20bfbda95d9baa42f723`.
- Windows 11 x64 build 26200, MSVC 19.44.35228.0, SDK 10.0.26100.0; strict Release `/W4 /WX /permissive-` build passed. Only headless CLI/typed actions were used.

### Changed scoring and recovery

The author found baseline-4's aggressive Auto seed 57 stopped at a Foil Warden while retaining Ballistic Starter's first-shot bonus. Independently loading that exact stopped trace confirms round 436, Warden 44 HP / 3 tiles / Mark 366, and 360 physical parts. Baseline-5 now completes that encounter in **19 legal commands / four shots**, without added resources or altered game rules. The three previous baseline-2 stopped states still complete in **16 / 6 / 9 commands**. These diagnostic resumptions end at rewards; they are not four additional full-city runs.

Source inspection confirms the repair values current finite tile depletion, gives blocked Mark/shot bonuses no next-hit credit through tiles, and limits stored shot bonuses to the remaining damage opportunity shared with statuses. It does not inspect future intent draws, preview End Turn, impose a game cap or change authoritative tile behavior. **Eighty valid hidden future-combat/RNG variants** across the four retained states and **twenty current concealed-Mystery/future-RNG variants** preserve the decisions and caller bytes. The earlier separate observation and atomic-invalid-action groups also pass. This is selected invariance coverage, not a redacted observation API or proof of all hidden-state combinations.

### Executed cases and classification

| New independent CLI case | Outcome / exit | Commands | HP | Final state hash |
| --- | --- | ---: | ---: | --- |
| Aggressive / Auto / seed 57 | city_complete /0 | 363 | 73 | `cff3620920a6964c` |
| Defensive / Auto / seed 57 | city_complete /0 | 347 | 79 | `c8a03ca699bddf1f` |
| Aggressive / Auto / seed 2, preview budget1 | defeat /0 | 34 | 0 | `ef1d139df91bc451` |
| Defensive / Auto / seed 2, 0.000001-second watchdog | policy_timeout /2 | 0 | 80 | `30038de7e7f89700` |
| Aggressive / Expert / historical seed 53 | blocked_rule /2 | 185 | 74 | `b7e1f4a8becdc82c` |

All HP ledgers reconcile. Both new seed 57 full traces byte-match the author's frozen regression traces. The seed 53 control naturally acquires UGS-141 and reaches its held base-haul composition, returning the explicit owner-decision reason. Its rejected command emits zero events and retains the prior state hash; the complete trace also byte-matches the author's separate control (`ef64ebe24650912a94bb2873127d588dc3ee46af485048a23ed27f95d5e8b9b3`). This verifies a real held result separately from defeat and timeout, without inventing a composition rule. A controlled two-drawback fixture additionally verifies whole-state rollback. Its initial setup omitted the campaign ever-acquired ledger after core-only acquisition; [the initial diagnostic](city-runner/evidence/baseline5-initial-results.txt) is retained. Correcting that QA fixture resolves its failure; no production defect or repair is claimed.

The five fresh traces plus eleven author regression traces (aggressive1/2/3/7/9/57; defensive1/2/3/13/57) pass both replay implementations. The separate decoder independently reapplies **5,162 commands**, ordered events and full final serialized bytes across **16 artifacts**, including duplicate selected cases. **19 replay/legacy groups, six unchanged policy groups and seven new policy groups pass.** [Complete execution](city-runner/evidence/baseline5-results.txt) and [compact run evidence](city-runner/evidence/baseline5-runs.json) preserve results. Twelve original/corrupted-trace integrity checks pass, and falsely relabeling a baseline-5 trace as baseline-4 is rejected.

All three synthetic Precision distributions were separately checked against the selected beta JSON through 10,000 public policy samples each:

| Model | Canonical Miss / Good / Perfect percentages | Observed counts |
| --- | --- | --- |
| Learning | 55 /35 /10 | 5,466 /3,547 /987 |
| Practised | 20 /45 /35 | 2,013 /4,500 /3,487 |
| Expert | 5 /25 /70 | 522 /2,451 /7,027 |

Each category is within 1.5 percentage points of its declared rate; same-seed decisions are deterministic and an already-spent attempt is not resampled. Auto submits ordinary collection with no timing measurement and leaves the manual opportunity unspent. These are synthetic assumptions, not observed human timing distributions.

No remaining defect was found in this finite scope. Naturally occurring `unsupported_choice` and `policy_error` outcomes were not forced; held-rule classification above does not claim coverage of every unsupported decision. Shallow search, omitted economic optimizations, owner-held rules and human/graphical limits remain. The author's 30,090 assertions are separate evidence. The 400-case batch was released after final source identity verification; its results are outside this report.

Reproduction uses the preserved ignored snapshot and a separate build directory:

```powershell
& games/overkill-foundry/qa/city-runner/build.ps1 -SourceRoot games/overkill-foundry/qa/city-runner/build/baseline-5 -BuildName native-baseline-5
python -X utf8 games/overkill-foundry/qa/city-runner/baseline5_review.py --runner games/overkill-foundry/qa/city-runner/build/native-baseline-5/Release/review_city_runner.exe --probe games/overkill-foundry/qa/city-runner/build/native-baseline-5/Release/city_runner_probes.exe --legacy-policy-probe games/overkill-foundry/qa/city-runner/build/native-baseline-5/Release/city_runner_baseline3_probes.exe --policy-probe games/overkill-foundry/qa/city-runner/build/native-baseline-5/Release/city_runner_baseline5_probes.exe --game games/overkill-foundry --output games/overkill-foundry/qa/city-runner/build/selected-baseline-5 --evidence games/overkill-foundry/qa/city-runner/evidence
```

## Baseline-4 follow-up — complete

**2026-09-20 — finite independent recheck passed. CR-03 and CR-04 are closed on this candidate.** The reviewed freeze is ready for the already-authorized fresh paired Auto seeds 21–120; the integration owner retains batch acceptance. The earlier baseline-2 and baseline-3 results below remain historical evidence, including the reproduced stall. This is not acceptance of balance, all policy choices or hundreds of completed runs.

The reviewer captured and verified **28 source files** before copying and after execution against git `db092963b07cb4f69c459b955a41b213ee268ee8`, at `2026-09-20T14:59:02.6178402Z`. [Source identity](city-runner/evidence/baseline4-source-identity.json) and [execution identity](city-runner/evidence/baseline4-execution-identity.json) record the full graph and harness hashes. Baseline-4 changes only the runner header, policy, README and author tests from baseline-3; authoritative rules, campaign, main and trace decoder are unchanged.

- Author executable SHA256: `8e8d3eafd1240e66794af1ed5862df244b8eec96f6f3fd57d195936dff6de221`.
- Independent executable SHA256: `6c549424fd1e6f6707944ccaf08f98da305a3fba8867aca4d0cf40f8e1215773`; policy source: `246e58ce16d6221ef921709888361b0191c89e5cfe433186ff6628c47c5a1b36`.
- Independent build: Windows 11 x64 build 26200, MSVC 19.44.35228, Windows SDK 10.0.26100.0, Release with `/W4 /WX /permissive-`. Controls are the headless CLI and typed production actions; no Unreal window or native input was used.

### Final execution

Five new cities use ordinary production NewGame, a 1,200-preview budget and a 10-second watchdog. Every selected run completes, exits 0 and reconciles its HP ledger. [Run summaries](city-runner/evidence/baseline4-runs.json) preserve trace hashes and identities.

| Policy / model / seed | Commands | Final HP | Elapsed seconds | Final state hash |
| --- | ---: | ---: | ---: | --- |
| Aggressive / Auto / 7 | 326 | 78 | 0.243 | `50ec0d0e21ddd7b9` |
| Aggressive / Auto / 9 | 434 | 75 | 0.385 | `2dafecfe3b0ff376` |
| Defensive / Auto / 13 | 435 | 83 | 0.352 | `b4f0fb8b56ad0f63` |
| Defensive / Auto / 2 | 370 | 78 | 0.284 | `98cbdcccb0b32362` |
| Defensive / Practised / 4 | 436 | 98 | 0.371 | `755a2f5482d42a79` |

The separate parser replays those five traces plus the nine author regression traces (aggressive 1/2/3/7/9, defensive 1/2/3/13): **5,522 commands**, checking typed results, ordered events and complete final serialized bytes. The four new Auto traces are also byte-identical to the author's corresponding traces. These are 14 replayed artifacts, including duplicate cases, not 14 independent new city observations. Seventeen replay/legacy groups and six new policy groups pass. [Execution log](city-runner/evidence/baseline4-results.txt).

- **CR-03:** the independent 10,000 public-policy samples again return 2,013 Miss / 4,500 Good / 3,487 Perfect, separately consistent with canonical 20/45/35. An already-spent Precision opportunity returns no further measurement. This verifies the declared synthetic model, not human timing ability.
- **CR-04:** the actual baseline-2 stopped snapshots now reach rewards in **16 commands** (aggressive 7), **6 commands** (aggressive 9) and **9 commands** (defensive 13). The 1/2 HP Chassis bodies retain their original 108/24 Mark at entry. `simpleValue` now bounds combined Mark/Burn/Corrosion score below remaining HP and removes redundant standalone Weaken value, already reflected in committed threat. The repair bounds policy valuation, not game statuses, shots, turns or parts. Victory cleanup resets fight counters, so diagnostic final `kills=0` is not a claim that no enemy was killed.
- Sixty valid future-combat/RNG perturbations and twenty valid concealed-current-Mystery/future-RNG perturbations preserve the tested decisions and the caller's complete bytes. Source review confirms the changed scoring uses committed threat and immediate legal previews, without End Turn preview, sampled future intent or new hidden-state reads. Newborn enemies are excluded from current-turn threat; previewed death-spawn/boss transitions receive progress credit; craft search values newly generated outputs rather than repeatedly previewing all old reserve parts.
- [Twelve replay-integrity cases pass](city-runner/evidence/baseline4-replay.txt): one valid original and eleven corrupt metadata/command/event/result/final artifacts. A separate false baseline-3 metadata label on the baseline-4 trace is rejected with exit 2. The final defensive 2 trace SHA256 is `d4a470479efc813aee612bb0a5855f61e80aa4cd179e2ffd85e3d770603fc743`.

No remaining defect was found in this finite recheck. Search remains shallow; the full-state API is protected by selected invariance checks and source discipline rather than a redacted observation type. This does not prove every hidden-state combination, recipe, upgrade, failure outcome or owned-part strategy. Existing source-defined held rules, natural `unsupported_choice`/`blocked_rule` outcomes, graphical parity, persistence crash recovery, audio and human play were not rerun. The author's 30,052 assertions are separate evidence, not included above. The fresh paired batch was released to the runner author only after these results; its outcomes are outside this report.

To reproduce the finite follow-up with the retained ignored snapshot:

```powershell
& games/overkill-foundry/qa/city-runner/build.ps1 -SourceRoot games/overkill-foundry/qa/city-runner/build/baseline-4 -BuildName native-baseline-4
python -X utf8 games/overkill-foundry/qa/city-runner/baseline3_review.py --runner games/overkill-foundry/qa/city-runner/build/native-baseline-4/Release/review_city_runner.exe --probe games/overkill-foundry/qa/city-runner/build/native-baseline-4/Release/city_runner_probes.exe --policy-probe games/overkill-foundry/qa/city-runner/build/native-baseline-4/Release/city_runner_baseline3_probes.exe --game games/overkill-foundry --output games/overkill-foundry/qa/city-runner/build/selected-baseline-4 --evidence games/overkill-foundry/qa/city-runner/evidence --policy-version baseline-4 --pilot games/overkill-foundry/core/build/runner-baseline-4-pilot --evidence-stem baseline4
```

## Baseline-3 initial candidate — preserved history

The next frozen candidate was independently captured as [28 source identities](city-runner/evidence/baseline3-initial-source-identity.json). Rules, campaign code and trace decoding are unchanged; the runner header/policy/tests/README changed and `BASELINE-2.md` was added. Initial author executable: `b775afe8d745a4b8b38031ae28de3258077c2f9a9a55fbda216bfeb222ad8c85`; initial independent executable: `7b78d3916110cf4812d28e0b9a1ae3c9efddbbcaf3f11acdf556f52c4f140467`. [Initial execution identity](city-runner/evidence/baseline3-initial-execution-identity.json) retains the targeted diagnostic before any repair.

**CR-03 is independently closed on that candidate.** Selected beta JSON SHA256 `39ca65256ee02de8288b1610bb0cffb053d64920fcd64a9e1ac748909894887b` specifies Practised20/45/35. Independent public-policy samples over seeds100001–110000 produced **2,013 Miss / 4,500 Good / 3,487 Perfect**, with separate category checks, not merely a mean check. A real defensive/Practised seed4 city completes and replays. Baseline-2's alternative-distribution result remains unchanged below.

Five fresh targeted cities complete: aggressive Auto7/9, defensive Auto13/2, and defensive Practised4. The four Auto traces byte-match the corresponding frozen author runs. All five plus nine authored regression traces pass the separate command parser and full-state/event replay; strict corruption checks also pass. This does not rerun the complete prior baseline-2 matrix.

**CR-04 — Medium, residual policy valuation stall; initially reproduced, closed in baseline-4 above.** Loading the actual baseline-2 final snapshots into baseline-3's policy still produces64 legal commands without a kill for aggressive7 (round189→197,134 starting parts) and aggressive9 (round213→222,91 starting parts). The remaining Split Chassis has1HP/2HP respectively. An ordinary paid SH001→Load→Fire preview kills that body in both states, increasing kills0→1; the policy instead repeats crafting, installation and End Turn. Its first decisions use only134–189 of1,200 previews, so current search-budget starvation is not the explanation. These are diagnostic resumptions of real stopped states, not fresh CLI failures, game-rule defects or proof of an infinite loop. The equivalent defensive13 stopped state reaches rewards in9 commands. [Exact targeted diagnostic](city-runner/evidence/baseline3-policy-initial.txt).

The author traced the remaining valuation problem to accumulated Mark:108/24 on those1/2HP Chassis bodies. `simpleValue` rewards all Mark at0.4 per point, so removing the marked target can score worse than keeping it alive. At this checkpoint the author prepared bounded useful-status scoring under a new baseline-4 identity, preserving baseline-3. No core rule change was required. Candidate release was held until the repair and successful recheck recorded above. Sixty valid combat future-packet/RNG perturbations and twenty concealed-current-Mystery/future-RNG perturbations pass; two earlier observation fixtures were corrected because their mutations violated route serialization before reaching the policy. Those setup errors are not production findings.

## Baseline-2 completed review — preserved history

**2026-09-20 — bounded execution recheck passed; subsequent CR-03 was open at this checkpoint (closed above).** Both initially reproduced defects below are repaired on the captured candidate. The runner is suitable for inspecting the planned unfiltered Auto pilot batch and its failures before expanding the experiment. The frozen Practised case used an alternative distribution, as recorded in the post-review finding below. This review does not complete P12's hundreds-of-runs requirement or establish balance, human difficulty, graphical parity or fun.

The reviewer authored only this report and `qa/city-runner/`, independently of the runner author. Acceptance sources: [P12 and simulation-quality criteria](../design/MVP-IMPLEMENTATION-PLAN.md), and [BALANCE-RESEARCH, sections 1–3](../design/BALANCE-RESEARCH.md), particularly canonical shared rules, observable information, explicit collection assumptions, exact replay and separate unresolved outcomes.

## Artifact and method

- Core baseline: `842300e9dcdf6612768485250b0ac2320dbde27c`. The runner changes are local work above that revision. All 27 source files were hashed before copying, after copying and after execution; see [captured source identity](city-runner/evidence/final-source-identity.json). Capture time: `2026-09-20T14:12:07.4793412Z`.
- Independent executable SHA256: `e6edf05c35096733ab6601a9bc9f536446db04d0e0bfe1b9cd724758a5fc1131`. Independent probe SHA256: `072465d5cd48ec3d29720f9bc09e18491adc08fb85999fc75271582df49f2772`. The author's separately built executable is `f92ba5b79576b4e3d2bf8a2f998b92e02a2adf42df4b16c32b2dff6a3057dd1d`; the execution evidence here uses the independent build.
- Windows 11 x64, OS build 26200; Visual Studio 2022, MSVC 19.44.35228, Windows SDK 10.0.26100.0, Release C++17 with `/W4 /WX /permissive-`. CMake built the frozen production core, runner and separate review probes successfully. [Execution identity](city-runner/evidence/final-execution-identity.json) records source and harness hashes.
- Controls were the headless CLI: `--city`, seed, policy, Precision model, preview budget, watchdog and `--replay`. No Unreal window, physical input, audio or packaged graphical artifact was exercised.

The independent parser does not call the runner's `decodeAction` or `replayTrace` when checking complete runs. It decodes each typed action, applies production `CampaignRules` with `cinderwallUpgradeHooks`, checks every result/event/hash, and compares the complete final serialized campaign bytes. It shares authoritative core rules and event serialization; it is not a second independent combat arithmetic engine.

## Findings and repair checks

| Finding | Severity | Reproduction and initial observed behavior | Expected behavior and final recheck |
| --- | --- | --- | --- |
| **CR-01: replay accepts false metadata/outcome** | High, artifact integrity; **closed** | Baseline EXE `b35fd1a8c3be509b621ca27dc04c2b813c14ffd543c49eedaff3bc5cb740a89c`: run defensive/Auto seed 2; then apply [controlled trace mutations](city-runner/adversarial_replay.py). The fresh trace has 332 commands and final hash `103a401e19424e4e`. Empty/wrong META, trailing META/R/FINAL fields, and changing FINAL `city_complete` to `defeat` all return exit 0. The false defeat is printed as validated. Initial `trace.cpp:52–54` checks only META's prefix, trusts the outcome and leaves extra fields unread. [Before: 5/12 checks pass, 7 fail.](city-runner/evidence/replay-before.txt) | Reject inconsistent/malformed metadata, extra fields and terminal labels contradicting canonical state. The repair in [`trace.cpp::replayTrace`](../core/runner/trace.cpp) validates metadata, canonical initial state, complete fields and final phase/result agreement. [Final: all 12 checks pass](city-runner/evidence/replay-final.txt), comprising one unchanged valid trace and 11 rejected corruptions. Existing changed-event, truncated-command and altered-final-byte detection remains effective. |
| **CR-02: Technician HP payment omitted** | Medium, attrition reporting; **closed** | Same baseline run: command 149 accepts UGS-030 from the Technician; HP falls from 78 to 70. The summary reports `hp_paid=0`, healing14, damage6 and final HP80 from start80. `campaign.cpp::AcceptCalibration` applies the actual 8-HP debit; the returned upgrade events omit that outer service cost. Initial `trace.cpp::recordEvents` counts only event costs. This is a reporting defect, not a free gameplay upgrade. | Include the service's actual residual debit without counting separately recorded acquisition costs twice. Final seed2 uses 342 commands because real OpenShop discovery actions were also added; Technician command155 still pays 78→70. Summary now reports `hp_paid=8`, and **80 + 14 healing − 6 damage − 8 payment = 80**. The separate parser counts the typed Technician payment plus emitted HP costs; all 12 selected runs match reported totals. [Execution evidence](city-runner/evidence/cli-results.txt). |

The existing early `runner-v2` pilots use OFCITY1, which the OFCITY2 reader explicitly does not support. Fresh OFCITY2 traces were generated before testing corruption; this format mismatch was not recorded as a game defect.

## Passed checks

The finite CLI matrix produced **12 cases: 10 city completions including one repeated case, one real defeat, and one watchdog stop**. These are QA selections, not a win-rate estimate. [Detailed summaries and choices](city-runner/evidence/runs.json) retain every selected result.

| Selected input | Observed outcome | Commands | Final HP |
| --- | --- | ---: | ---: |
| Aggressive, Auto, seeds 1 / 2 / 3 | Three city completions | 445 / 406 / 336 | 57 / 77 / 69 |
| Defensive, Auto, seeds 1 / 2 / 3 | Three city completions | 603 / 342 / 459 | 50 / 80 / 82 |
| Defensive, seed4, learning / practised / expert | Three city completions | 490 / 488 / 498 | 98 / 98 / 98 |
| Aggressive, Auto, seed2, preview budget1 | Defeat in first encounter, exit0 | 34 | 0 |
| Defensive, Auto, seed2, watchdog0.000001 seconds | `policy_timeout`, exit2 | 0 | 80 |
| Repeat defensive/Auto/seed2 | City completion, byte-identical full trace | 342 | 80 |

- **All 4,443 commands were independently reapplied**, including ordered events and complete final snapshots. All 12 also passed the authored replay CLI. The separate executable passed 15 probe groups: twelve full traces, two policy-observation groups and one atomic-invalid-action/typed-codec group.
- Every initial snapshot equals ordinary production `newGame(seed, runId)` byte for byte: 80/80 HP, 100 Credits, twelve starting recipe copies and no physical parts. Source inspection of `trace.cpp::runCity` confirms every selected command uses `rules.apply`; it contains no material/HP grants, terminal fixture injection or substituted effects. Paid recipe uses and collection actions occur throughout the ordinary traces.
- Twenty altered hidden route/future RNG states and twenty altered future enemy pattern/RNG states leave the tested current policy decisions unchanged. Calling the policy preserves the caller's serialized state. These controlled observation fixtures are not counted as normal campaign runs.
- Source inspection of `policy.cpp` shows ranking based on committed intent and owned/current offers; no End Turn preview or future robot pattern inspection. Before entering Mystery, route ranking uses `revealedMysteryCategory`; raw selected Mystery identity is read only after entry. The policy performs an actual recorded OpenShop action before considering that stock. The seed initializes content and the declared separate Precision model, rather than ranking future draws.
- The repeated defensive seed2 trace SHA256 is `7d8a2e121f0c76d0bdd06d288852db55463c787f72ab9cb13b878e768ceecaac`, also matching the author's published pilot. Final state hash is `dae7aea3a8a7e0f5`.
- The deliberately weak policy reaches `CityPhase::Defeated` with zero HP; a genuine loss exits0 as a completed simulation. The watchdog stops before any command with unchanged canonical state, exits2 and replays as `policy_timeout`. It does not inject an in-game turn/shot cap.

## Limits and next acceptance evidence

No additional defect was found during the bounded execution recheck; CR-03 below was identified afterward by the integration owner. The recheck does not prove every legal action, all 246 recipes or all 152 upgrade IDs are handled well by the policy. The policy receives the full campaign object; the information boundary is maintained by its implementation and selected invariance probes, not by a redacted observation type. Current hidden-Mystery behavior was source-inspected; this review's independent perturbation cases cover future route slots and future combat patterns, not every concealed-offer combination.

`blocked_rule`, `unsupported_choice` and `policy_error` are distinct in source and JSON. No selected ordinary run reached those outcomes, so their natural-run end-to-end classification is **not independently executed here**. The held UGS-141 and combined base-haul removal rules remain explicit core limitations; they must stay separate from gameplay losses in larger reports. The Risky Calibration double-cost edge has an author regression, but was not separately forced in this finite matrix.

Candidate search is shallow and budgeted. Material purchases, part sales, route replacement, reroll optimization and many saved-part strategies are not exercised by these policies. `materials_gained` intentionally measures positive net changes per command; it is not gross material production. The three Precision distributions are declared synthetic assumptions, not measurements of human claw control. Watchdog stopping points can vary with machine load.

Startup and exit were exercised as headless processes, and full-state trace replay was verified. Windows save-session crash recovery, Continue, graphical/headless command parity, packaged startup, focus/input, audio and human playtests were not rerun for this runner-only increment. The author's 30,032 runner assertions are separate evidence, not included in the independent counts above.

Next: inspect the author's unfiltered paired seeds1–20 pilot, retain every failure/held result and weak-policy behavior, then select a larger experiment. Do not infer tuned pacing or human difficulty from these selected completions.

## Post-review source finding

**CR-03 — Medium, Practised model differs from selected parameters; originally open, independently closed in baseline-3/4 above.** The integration owner identified this after the frozen recheck; the reviewer then confirmed the source comparison. [`beta-balance-v1.json`, `precision_models_for_future_headless_tests_percent.Practised`](../design/data/beta-balance-v1.json) specifies **20% Miss / 45% Good / 35% Perfect**. Captured `policy.cpp::precisionResult` (line25, SHA256 `df662c3187a43281a48b87cd574d36c36a62718bba150ad997b15c3a2e6066b3`) implements **15% / 55% / 30%**, matching that candidate's README but not the selected JSON. Compare those two definitions to reproduce the discrepancy. Both average 1.15 bonus materials, but their miss/perfect frequencies and conditional effects differ.

The recorded `skill-practised` seed4 result therefore verifies replay and bookkeeping for the documented **15/55/30 alternative actually executed**; it does not validate the canonical Practised distribution. No prior result or frozen artifact has been rewritten. Auto runs are unaffected. The author subsequently reported corrected parameters and a new baseline-3 policy version; the independent canonical-model recheck was pending when this finding was recorded. Its later execution is recorded above; the original results remain unchanged.

## Reproduction

From the repository root, capture a fresh source name, build it, run the finite matrix and apply the corruption checks. Existing capture names deliberately refuse overwrite. Bulk snapshots, binaries and traces remain under ignored `build/`; public evidence contains relative paths.

```powershell
& games/overkill-foundry/qa/city-runner/capture.ps1 -Name review-repeat
& games/overkill-foundry/qa/city-runner/build.ps1 -SourceRoot games/overkill-foundry/qa/city-runner/build/review-repeat
python games/overkill-foundry/qa/city-runner/run_cli_review.py --runner games/overkill-foundry/qa/city-runner/build/native/Release/review_city_runner.exe --probe games/overkill-foundry/qa/city-runner/build/native/Release/city_runner_probes.exe --output games/overkill-foundry/qa/city-runner/build/selected --evidence games/overkill-foundry/qa/city-runner/evidence
python games/overkill-foundry/qa/city-runner/adversarial_replay.py --runner games/overkill-foundry/qa/city-runner/build/native/Release/review_city_runner.exe --trace games/overkill-foundry/qa/city-runner/build/selected/defensive-s2/defensive-auto-2.oftrace --output games/overkill-foundry/qa/city-runner/build/corruptions
```
