# Mara recipe semantic contracts

This isolated author suite exercises the 107 Mara recipes marked `unbound` in
the captured `content/runtime-evidence/coverage.json`: 29 Ammo, 24 Shield,
16 Modifier and 38 Utility recipes. Its proposed links are **not active runtime
bindings**. Passing the recipe count is not full mechanics acceptance.

The frozen candidate at `core/build/mara-semantic-05` passes all 107 bounded
groups, **20,526 assertions**, and the unchanged existing recipe regression
suite's **2,055 assertions**. The strict native build used MSVC 19.44.35228.0,
Windows SDK 10.0.26100.0 and CMake 3.31.6-msvc6, Release x64, C++17, with
`/W4 /WX /permissive- /utf-8`. No Unreal, graphics or desktop work was performed.

- Full captured input graph: `254e4f72c531dcac6f70c1df74cf7a09592aaa15803a191767e54db936556ef3`.
- Contract executable: `f93f37ccb67b3f778ccb5846590f25c0f0f836d52393e8b025f8ccc3f2d861ef`.
- Corrected `recipe_effects.inl`: `4adce1cd3c24c9e16a771e1ada3a2aac7141668a1ab4258f79be6a9bc6f82ebf`.

`evidence/results.json` records all 37 captured inputs, executable and log
identities, command exit codes, named passing assertions and the native toolchain.
The ignored archive contains the full copied sources, binaries, library and
unabridged build/execute logs. `evidence/proposed-bindings.json` links every
bounded passing group to its exact source row and explicit residual coverage.
`residual-branches.json` also states shared and recipe-specific limits. These
are review proposals; no source version, save schema or global coverage record
was changed by this suite.

## What is tested

The expected costs and recipe clauses come from the catalogue snapshot and
`design/RECIPE-CATALOGUE.md`, with the selected timing/part rules in
`design/TIMING-AND-PERSISTENCE.md`. `source_index.py` copies the 107 exact source
rows and emits the printed material cost oracle without reading C++ effects.
The Heat-cap control also uses the source-defined MAU-02 upgrade.

Tests call production `Rules` for paid recipe Use, installation, removal,
activation, Load, Fire, Collect and End Turn. Every successful action compares
its preview to the actual committed state and ordered events. Paid Uses check
the source recipe/copy/material receipt and each insufficient-ingredient
rejection. Other rejected choices must preserve the entire serialized state,
IDs, RNG and empty event result. Snapshot checks exercise canonical in-memory
serialization, without claiming file durability.

The named assertions cover numerical effects, selected thresholds/caps,
target exclusions, first-install values, payments, delayed events, once-only
triggers, ordered reads and expiry. `boundaries.inl` extends those groups with
additional explicit clauses. For example, MA082 uses real acquired MAU-02 and
paid Fuel Bricks to reach legal Heat14; MA098 distinguishes intact removed,
depleted removed and still-installed Shield. MA107/MA119 use real Campaign
actions through Mayor choice, an actual regular route offer, recipe payments,
delivery, finishing shots or fatal attack, and cleanup at Rewards/Defeated.

The fixtures deliberately provide selected memory copies, plentiful materials,
controlled HP, enemy statuses/intents and protection. Some remaining Shield
balances use the public spending helper as fixture setup. The MA120 current
Shield test changes protection after Fire explicitly; it does not pretend a
generic target performed a canonical Shield-gain action. Campaign cleanup
fixtures retain real robot definitions for reward lookup while controlling
their HP/intent and supplies. None of these is an earned loadout, natural route,
economic balance or human-play witness. Actual campaigns and disk transactions
have separate evidence.

The assertion total includes repeated payment, preview, event and fixture
invariants. It is not a count of independent source clauses. A group fails
at its first failed assertion; later assertions in that failed historical group
are not claimed as executed. Recipe-specific limits remain explicit, including
selected removed-source cases, fatal reaction permutations and unenumerated
upgrade/listener combinations. Physical output/lifecycle coverage alone was
not substituted for recipe effects.

## Preserved failures and authorized repairs

`history/failures.json` distinguishes every failed group in four original
immutable captures. Early failures included author mistakes: illegal ordinary
Heat fixtures, extra-target Mark arithmetic, an unpromised event name, a wrong
support Utility cost, a generic robot intent that did not execute, incorrect
payload/spread ordering, and an invalid Campaign robot identity. Those originals
remain failed artifacts; they are not retroactively described as engine defects.

Two source-defined defects remained after legal reproduction. Root explicitly
authorized only these two changes, against the candidate already containing
the separately owned SH092/SH113 and UGS121 repairs:

- **MR01, MA082 White Casting:** at legal Heat14, the original payload applied
  Burn14 despite its printed maximum10. Clamp only that payload to10. Applied
  stack/event and unchanged Heat controls cover 0, 3, 10 and14; a same-Fire
  separate Heat payment still uses the recorded Fire snapshot.
- **MR02, MA098 Stored Weight:** a crafted Shield paid from6 down to1, then
  removed, incorrectly added3 damage as unused reserve. Require the existing
  `isUnusedPart` predicate alongside Reserve. Untouched removed plain Shield
  still qualifies; no new kind/origin restriction was added. Unused0/1/6/7
  controls retain the18 cap.

`history/repairs.json` gives the minimal paid-action reproductions and before/
after identities. `history/MR01-MR02.patch` is exactly the two-line production
delta from capture03 to capture05, excluding concurrent owners' earlier work.
No further production edit was made by this task. The broader save compatibility
decision remains outside this task.

## Reproduce and independently review

From `games/overkill-foundry`, with a CMake executable selected for the machine:

```powershell
python -X utf8 core/tests/mara-recipes/source_index.py
python -X utf8 core/tests/mara-recipes/capture.py --cmake "C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe" --output core/build/mara-semantic-fresh
```

That is the actual capture invocation used here, with `mara-semantic-05` as the
original output name. Use a new directory: the helper refuses to overwrite
an existing archive. It copies and hash-checks every source before configuring
the copied CMake project, builds the strict target and runs both native suites.
Exact command argv is also preserved in the archive's local `identity.json`.

For a direct local build without publishing evidence:

```text
cmake -S core/tests/mara-recipes -B core/build/mara-direct -G "Visual Studio 17 2022" -A x64
cmake --build core/build/mara-direct --config Release --parallel 4
ctest --test-dir core/build/mara-direct -C Release --output-on-failure
core/build/mara-direct/Release/mara_recipe_contract_tests.exe --case MA082
core/build/mara-direct/Release/mara_recipe_contract_tests.exe --case MA098
```

The optional `--case` selects one complete bounded group. The recorded capture
ran the complete executables directly; the CTest command is an equivalent
reproduction path, not an additional claimed capture run.

Verify the frozen evidence without executing tests again:

```text
python -X utf8 core/tests/mara-recipes/publish.py --archive core/build/mara-semantic-05 --check
```

Without `--check`, the publisher regenerates the portable proposals and failure
history from that local candidate. It verifies copied source, binary and log
identities and requires precisely the two authorized production changes. It
reports current-source agreement separately; a later live edit does not alter
the historical capture. New source candidates need a fresh isolated capture
and explicit report update, not relabeling of this one.

Independent QA should start with MR01/MR02 against the failed03 and corrected05
copies, inspect the exact source clauses and fixture limits, and challenge
several delayed, conditional and exclusion branches. Review proposals against
their actual assertions before activating any global binding. Root owns the
combined candidate build and integration; this author run does not claim
independent approval.
