# Combined native mechanics integration review

**Pass for the stated build and evidence scope, with the initial-failure log
limitation below.** This read-only review found no actionable integration defect.
It did not rebuild or rerun the passing tests. It is not independent semantic
approval of the Mara cases: this reviewer authored those cases; their separate
independent clause review remains a different record.

Reviewed frozen archive: `core/build/mechanics-integration-20260920-221228`.
Its LF-normalized 78-input graph is
`8ce7ff421aa9bc2a6ed4a1be4d77ad5c4a882f08cc6b48b92a49ca4fa3798637`.
The one `overkill_core.lib` is
`afe752450de55d4837b2b5ebe3e1517eb5a21fb77912fdfd0bb03a0649a05228`.
The detailed identities and 286 passing review checks are in
`qa/mechanics-integration-evidence.json`.

## Source, compilation and linkage

All 78 copied files match their recorded length, raw SHA256 and LF-normalized
SHA256. Recomputing the manifest graph matches the declared value. Every listed
live source also matched its captured raw hash at this review. This is a check
of the recorded snapshot, not a claim about later source changes.

The captured integration CMake file enables the original core tools and adds
eight contract executables to the original eight tests. All sixteen actual
generated Release x64 projects link the same captured `overkill_core.lib`.
The runner test additionally links `overkill_runner_support.lib`, whose PUBLIC
dependency is that same core library. Actual MSBuild link-read logs confirm
the shared library path for all sixteen test executables; none compiles a
private replacement core. The core project compiles the ten authoritative
production translation units from the copied tree.

All eighteen relevant generated projects—the sixteen tests plus core and
runner-support libraries—and their actual compiler command logs confirm
C++17, warning level 4, warnings as errors and conforming mode
(`/std:c++17 /W4 /WX /permissive-`). The test targets also request `/utf-8`;
the core/support library targets do not make that additional claim. The
observed compiler is MSVC 19.44.35228.0 with SDK 10.0.26100.0 and the captured
CMake 3.31.6-msvc6 build.

The MSBuild compiler-read logs refer to 63 distinct game source/dependency
files; all are captured inputs, with no reference to a live game source outside
the copied tree. The 78-row manifest also contains unused standalone CMake
files and two pre-existing generated CompilerId sources under older local
test-build directories. Those extra rows are verified capture entries, not
additional compiled gameplay code. Their inclusion is harmless for this
snapshot, although future capture filtering could omit them for clarity.

All 20 recorded executable/library artifacts match their recorded lengths and
hashes. This inventory includes the sixteen tested executables, runner tool,
two libraries and the CMake compiler-identification executable. The configured
project, compiler/link dependency logs, executable hashes and test commands
agree with the same source graph.

## Test execution and full-output provenance

The final archive records exactly three successful commands: configure, build
and one CTest execution. `ctest.log` reports 16/16 passed. The JUnit and full
`LastTest.log` have the same ordered sixteen unique suite IDs, statuses and
captured executable commands. A separate line-state parser compared every
full stdout body to the published result; all matched. This review invoked
neither CTest nor any test executable.

| Suite | Recorded result from the full output |
| --- | --- |
| Part lifecycle | 207 groups; 59,169 assertions |
| Upgrade contracts | 106 groups; 6,139 assertions; held IDs remain unproposed |
| Shared recipes | 113 groups; 27,881 assertions |
| Mara recipes | 107 groups; 20,526 assertions |
| Upgrade copy | 7 groups; 671 assertions |
| Encounters | 9 groups; 3,946 assertions |
| Campaign lifecycle | 6 groups; 652 assertions |
| Terminal transitions | Unique notifications and once-only campaign completion/receipt assertions passed |
| Original core | 443 assertions |
| Original route | 146,303 assertions; 1,000 controlled paths |
| Original robots | 2,462 assertions |
| Original recipes | 2,055 assertions |
| Original upgrades | 1,130 assertions |
| Original campaign | 784 assertions |
| Campaign upgrade adapter | 255 assertions |
| Runner | 30,090 codec/replay assertions; seed 1 completion, 440 actions |

These are attributed suite outputs, not new independent semantic findings.
Their own scope limits remain in the full stdout and existing suite reviews.
The counts are not combined into a blanket mechanics-completeness claim.

Nine successful JUnit stdout bodies are truncated at the stated 1024-byte
threshold. Each retained prefix matches its corresponding full log. The
remaining seven JUnit outputs match in full. `LastTest.log` is 74,274 bytes,
SHA256 `045a6bdf796d76283c3d77cc7770b72defd15a1a64e01f606a562971137342f5`.
The later extractor reads that log from the same execution; it does not obtain
the missing text from a second test run.

The original `capture.py` and later `results-extractor.py` are both preserved
and have distinct identities. The later file matches the current helper and
the record's `results_extraction_tool_sha256`; the original still matches
`capture_tool_sha256`. Inspection of their delta shows the new full-output
extraction helper and its call replacing the JUnit-only stdout extraction.
No compiled input changed with that extraction update. The public results
match the archive identity apart from intentionally omitted local command argv
and added explicit scope limits; command exit codes and log identities remain
identical. Exact original argv remains in the ignored archive.

## Initial failed capture

The earlier archive `core/build/mechanics-integration-20260920-220954` remains
marked failed. Its 77 copied inputs match the failed manifest; every one is
unchanged in the successful retry. The only added input is
`presentation/precision.hpp`, required by the captured
`upgrade_contract_tests.cpp`. There is no CTest XML for that failed capture.
Its graph is `eddeb2d6ab7ecd837459231358338443e7c850da6f854b03e70cf789bd144adc`.

The first build was executed directly, and **its original compiler console
output was not retained as an archive log**. Root attributes exit 1/C1083 at
`upgrade_contract_tests.cpp(4,10)` to tool session 87257, completion chunk e04b3a,
for missing `../../presentation/precision.hpp`. This review independently
verifies the missing dependency, unchanged 77 inputs and failure-marked record;
it does not claim to have read a preserved original compiler diagnostic.
No reconstructed text is presented as that original log. The final successful
capture has the actual configure/build/test logs, which were hash-verified.

## Production changes and remaining limits

Compared with committed baseline `0e4c019e1435665d73ce4dcf534618b8faf015a2`,
the captured production graph differs in exactly five files and ten diff hunks.
The complete normalized comparison is `qa/mechanics-integration/production.delta.patch`.
Manual inspection matches the declared narrow changes:

- `core.cpp`: emit terminal notifications only on the actual phase transition.
- `campaign.cpp`: require UGS-085's undiscounted shop price to exceed 100.
- `campaign_upgrades.cpp`: include runtime Spread in the source Modifier offer category.
- `recipe_effects.inl`: put SH092/SH113 unconditional delivery schedules at
  first installation; remove only the UGS121-attributed bonus on copied
  materialization; cap MA082 Burn at 10; require MA098 reserve parts to satisfy
  the canonical unused predicate.
- `upgrade_effects.inl`: mark UGS121's physical bonus using the existing
  Attachment; include runtime Spread in UGS095's Modifier-use condition.

No production header or serialization file differs from that baseline, and
this review found no additional production delta. That observation does not
approve compatibility with historical saves: behavior/receipts changed while
the candidate retains its held version. Save migration/version decisions,
active support bindings and content eligibility remain outside this review.

The passing combined build establishes coexistence and recorded regression
execution on one library. It does not retroactively replace older isolated
review identities, erase declared semantic residuals, prove exhaustive
upgrade/recipe interactions, or establish UI, graphics, packaging, balance or
human play quality. No core, version, binding, STATUS or Unreal file was edited
by this review.

Recheck the same read-only evidence from the repository root with:

```text
python -X utf8 games/overkill-foundry/qa/mechanics-integration/verify.py
```

The verifier writes only its owned QA JSON and normalized diff. It reads the
frozen archive, generated build metadata, Git baseline and current sources;
it never configures, builds, launches or reruns tests.
