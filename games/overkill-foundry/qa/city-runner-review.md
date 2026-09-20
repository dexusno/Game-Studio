# Independent city-runner review

**2026-09-20 — bounded execution recheck passed; later source finding CR-03 remains open.** Both initially reproduced defects below are repaired on the captured candidate. The runner is suitable for inspecting the planned unfiltered Auto pilot batch and its failures before expanding the experiment. The frozen Practised case used an alternative distribution, as recorded in the post-review finding below. This review does not complete P12's hundreds-of-runs requirement or establish balance, human difficulty, graphical parity or fun.

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

**CR-03 — Medium, Practised model differs from selected parameters; open for baseline-3.** The integration owner identified this after the frozen recheck; the reviewer then confirmed the source comparison. [`beta-balance-v1.json`, `precision_models_for_future_headless_tests_percent.Practised`](../design/data/beta-balance-v1.json) specifies **20% Miss / 45% Good / 35% Perfect**. Captured `policy.cpp::precisionResult` (line25, SHA256 `df662c3187a43281a48b87cd574d36c36a62718bba150ad997b15c3a2e6066b3`) implements **15% / 55% / 30%**, matching that candidate's README but not the selected JSON. Compare those two definitions to reproduce the discrepancy. Both average 1.15 bonus materials, but their miss/perfect frequencies and conditional effects differ.

The recorded `skill-practised` seed4 result therefore verifies replay and bookkeeping for the documented **15/55/30 alternative actually executed**; it does not validate the canonical Practised distribution. No prior result or frozen artifact has been rewritten. Auto runs are unaffected. The author will correct the parameters and policy version for baseline-3; canonical-model recheck is pending, and no additional execution was performed for this finding.

## Reproduction

From the repository root, capture a fresh source name, build it, run the finite matrix and apply the corruption checks. Existing capture names deliberately refuse overwrite. Bulk snapshots, binaries and traces remain under ignored `build/`; public evidence contains relative paths.

```powershell
& games/overkill-foundry/qa/city-runner/capture.ps1 -Name review-repeat
& games/overkill-foundry/qa/city-runner/build.ps1 -SourceRoot games/overkill-foundry/qa/city-runner/build/review-repeat
python games/overkill-foundry/qa/city-runner/run_cli_review.py --runner games/overkill-foundry/qa/city-runner/build/native/Release/review_city_runner.exe --probe games/overkill-foundry/qa/city-runner/build/native/Release/city_runner_probes.exe --output games/overkill-foundry/qa/city-runner/build/selected --evidence games/overkill-foundry/qa/city-runner/evidence
python games/overkill-foundry/qa/city-runner/adversarial_replay.py --runner games/overkill-foundry/qa/city-runner/build/native/Release/review_city_runner.exe --trace games/overkill-foundry/qa/city-runner/build/selected/defensive-s2/defensive-auto-2.oftrace --output games/overkill-foundry/qa/city-runner/build/corruptions
```
