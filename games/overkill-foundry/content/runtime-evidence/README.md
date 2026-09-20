# Partial shared-core runtime linkage — 20 September 2026

**133 of 665 obligations are bound; 532 remain unbound. Release is refused.**
This is an evidence-linkage increment. It changes no gameplay rules, eligibility,
runner policy or release gate. An unbound entry means this audit has not proved
its complete contract; it does not, by itself, identify broken implementation.
The [finite independent audit](../../qa/runtime-linkage-review.md) verifies the
complete identity graph and samples twelve semantic obligations. It does not
establish independent semantic acceptance of every remaining binding.

| Obligation | Required | Bound | Unbound |
| --- | ---: | ---: | ---: |
| Recipe | 246 | 38 | 208 |
| Physical output | 207 | 0 | 207 |
| Upgrade / Mayor | 152 | 52 | 100 |
| Shared core operation | 23 | 17 | 6 |
| Status | 12 | 8 | 4 |
| Robot | 10 | 4 | 6 |
| Formation | 10 | 10 | 0 |
| Mystery | 3 | 2 | 1 |
| Route | 1 | 1 | 0 |
| Ability | 1 | 1 | 0 |

[coverage.json](coverage.json) records a decision for every required ID, including
the missing evidence for each unbound entry. The manually assessed
[binding cases](../../tools/runtime-binding-cases.json) link source handlers to
specific assertions. Prefix selectors must resolve to exactly one name printed
by an executed passing fixture; generated bindings store that complete name.
Registration, catalogue text, a suite exit code, an ID-count total and action-path
smoke are not semantic acceptance. The release manifest remains unchanged.

The five owner-held obligations are `core.collect-and-precision`, `UGS-011`,
`UGS-018`, `UGS-126` and `UGS-141`. Individual reductions exist, but the combined
base-haul composition beyond three removed units and the UGS-141 two-unit voucher
have no selected composition. The runtime's explicit refusals remain intact.
`SH066` is also unbound: existing fixtures establish the nine-Shield case, not
chosen payment and nonmultiple remainder behavior below nine. This audit makes
no new rule interpretation.

All 207 physical-output obligations remain unbound until exact types have a
named materialization/copy/immutable-resale/lifetime matrix. Representative
factories and source recipe tests do not establish that entire matrix. Other
gaps include unasserted conditional payloads, selector exclusions, trigger limits
and expiry windows. For example, MA066's next-round activation is tested, but its
Fire branch and early/late-use exclusions are not explicitly linked. UGS-094's
third-turn tie is tested, but unequal counters and later periodic opportunities
are not. Such partial observations do not become bindings.

## Independent linkage correction

The bounded independent review sampled twelve obligations and identified five
insufficient links: SH043's twelve-damage cap and second-shot exclusion; SH070
skipping an earlier one-Iron Use; MA102's eight-Heat snapshot cap; UGS-116's
round-two discount renewal; and UGS-142's zero-failure/escape exclusions. All five
were removed from the support map and receive explicit unbound reasons. These
are evidence gaps, not reproduced gameplay defects. The remaining 133 links
have not all been independently sampled.

The original 138-binding candidate is preserved byte-for-byte under ignored
`core/build/runtime-linkage/review-138/`, with `identities.json`. Its support SHA
is `f9cace6a9a9140bb3c480aaf279193ee94a08b2805fdd53acb6645a82f51ed85`.
This revision only changes cases, support, derived coverage, this report and gate
outputs. The 217 executed fixture results, native binaries, capture tool and all
37 captured inputs retain their original hashes; no new native pass is claimed.

## Executed evidence

The capture built one shared `overkill_core.lib` and eleven executables with
Visual Studio 2022 x64 Release / C++17. The shared core and four independent
probe executables use `/W4 /WX /permissive-`; the seven author suites retain their
existing compiler settings. All **217 named groups** passed, with all 37 captured
source/test/manifest files unchanged during compilation and execution.

| Suite | Named groups | Existing assertion total / result |
| --- | ---: | --- |
| Core author | 26 | 443 assertions |
| Recipe author | 19 | 2,055 assertions |
| Upgrade author | 31 | 1,130 assertions |
| Robot module + real Engine | 7 | 2,462 assertions |
| Route author | 2 | 146,303 assertions; 1,000 ordinary and 100 replacement paths |
| Campaign author | 11 | 784 assertions |
| Production upgrade adapters | 12 | 255 assertions |
| Foundation probes | 15 | 15 passed, 0 failed |
| Expanded recipe probes | 43 | 43 passed, 0 failed |
| P08 core probes | 36 | 36 passed, 0 failed |
| P08 campaign probes | 15 | 15 passed, 0 failed; eight actual process exits |

The new PASS labels only follow already-existing assertion blocks in
`robot_tests.cpp`, `route_tests.cpp`, `campaign_tests.cpp` and
`campaign_upgrade_tests.cpp`. Assertions, fixtures, execution order and production
behavior are unchanged. Independent probe sources were copied byte-for-byte into
the ignored build directory; their historical source and reports were untouched.
These are **new author-operated executions of independent probes**, not a rewrite
of historical independent review.

Controlled fixtures use prepared arenas, test-only upgrade callbacks or terminal
ammunition where their logs say so. They prove the named mechanism, not normal
city balance, graphical behavior, human enjoyment or shipping readiness. The
legacy pre-P08 campaign probe uses an obsolete API and was not rewritten or
reported as a current pass. Its profile/city-completion evidence and the frozen
runner experiments have not been substituted for missing named current fixtures.

[results.json](results.json) records exact fixture names, test-source hashes,
native executable hashes, commands, logs and the linked library. Capture started
at `2026-09-20T16:23:59.338716+00:00`, on repository HEAD
`5647174d6fde6d7c6bd54cdd1dfff023d1048bba`, with the reporting edits described above.
The core, recipe and upgrade effect source hashes still match the reviewed P08
rules. The baseline-5 runner and its experiments were neither rebuilt nor edited.

| Artifact | SHA-256 |
| --- | --- |
| Shared library / build identity suffix | `e071b631023020f3e880c2a691306c1c21de9ad65fde25c12a90d124c8657a33` |
| Manifest | `d56bf01290f16567cd7009a64f4d49f825454a7149fbf6a922a695a4b71deab9` |
| Runtime support | `c3eb16d905ff3f93e592439653929f0cfdacfa88483a68a91502bed8e67f817d` |
| Fixture results | `93d22967306e4974f435ec71253864dd95d2af3ee4e2ef107d147b047d045646` |
| Coverage decisions | `6a14488fc8c5313a040d28556e2fe8e7f39e63ddd38826edd194adf164808802` |
| Capture tool | `fcd818aab97b4809859a58b61369c89b294968a5f9714aa83f9cefed1818cd66` |
| Binding cases | `918535eec6e25676c1c2b30eefbb27dac1a54675376c9c6aaa91e6b5075c29c9` |

## Reproduce and review

From repository root, using the installed CMake path verified for this capture:

```powershell
$runtimeCmake = "C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe"
python -X utf8 games/overkill-foundry/tools/capture_runtime_support.py --run --self-test --cmake $runtimeCmake
python -X utf8 games/overkill-foundry/tools/capture_runtime_support.py --check --self-test
python -X utf8 games/overkill-foundry/tools/compile_content.py --check --self-test
python -X utf8 games/overkill-foundry/tools/compile_content.py --check --release
```

`--run` deliberately creates fresh build identities and replaces this capture's
generated results/logs; preserve a frozen copy first for independent comparison.
`--check` writes nothing and validates current source hashes, named log results,
bindings, and local binaries when present. Native artifacts and build logs live
under ignored `core/build/runtime-linkage/`; target commands in the results are
relative to the game directory. `<cmake>` there denotes the verified executable
used above. All binaries are individually runnable from
`core/build/runtime-linkage/native/bin/Release/runtime_*.exe`.

[gate-results.json](gate-results.json) preserves actual command outputs:

- Partial-linkage check and all seven false-evidence rejection controls: exit 0.
- Content check, 560 schedules, 6,160 offer states and nine negative fixtures: exit 0.
- Unmodified full release gate: **exit 1, 532 unsupported release operations**.

The compiler checks missing obligations before inspecting bindings, so the
separate partial-linkage validator is necessary to check these 133 entries now.
It is not a release waiver. Independent QA should compare each binding's full
source clauses with its cited assertions, verify hashes and printed PASS names,
review reporting-only diffs, and challenge any insufficient linkage by leaving
that operation unbound. No change to the game's rules is needed for that review.
