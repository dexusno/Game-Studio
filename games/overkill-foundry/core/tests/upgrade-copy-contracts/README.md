# UC01: one physical Standard Ammunition Jig bonus

Independent QA reproduced UGS-121's +2 on both a paid SH001 and its UGS-015
lease copy when Jig was acquired first. The source explicitly limits the Jig
bonus to one physical Slug even when a copy effect creates more. The original
106-group upgrade graph `49b5f504…`, its executed report, and QA's failing Q09
remain historical evidence; they are not overwritten by this increment.

The narrow correction preserves `Part.upgradeDamage` on the original. It adds a
zero-valued existing `Attachment` with source `UGS-121` as attribution. When a
`PartOrigin::Copied` part is materialized, the Engine checks that at least 2
aggregate bonus damage exists, removes exactly Jig2 and that marker, and preserves
every other committed field/attachment. The marker adds no damage, Heat, clock or
resale value. Existing printed-only copying clears all later modifiers as before.
There are no schema, Action or RulesVersion changes. The one old regression oracle
that expected Mayor2+Jig2 on both originals and copies now expects original4/copy2.

The separate focused suite covers both acquisition orders, permanent recipe
bonuses, Mayor bonuses, the committed SH103 attachment, stored/reloaded originals,
SH118 copies and their descendants, SH072 printed return, failed/paid/granted
production, per-turn renewal versus lease limits, unchanged sale/unused eligibility,
and atomic rejection of inconsistent attribution. MY1-06 does not copy physical
parts: its source grants memory capacity and one Shared Uncommon recipe, so it is
inapplicable to Slug duplication and is not represented as a tested copy path.

**Migration limitation:** old saved Slugs lack this marker. The patch does not guess
which historical aggregate included Jig. The owner-held save migration/fresh-start
and version decision must address those states before claiming compatibility.

The current full capture also contains the separately frozen SH092/SH113 repair.
`uc01.patch` isolates only this Jig correction and the single oracle change against
the original frozen upgrade graph; QA can apply it to its own copy without importing
the unrelated schedule change. Earlier Shared and upgrade captured inputs are
unchanged. No support ledger is activated by this overlay.

From the game directory:

```text
cmake -S core/tests/upgrade-copy-contracts -B core/build/upgrade-copy-contracts -G "Visual Studio 17 2022" -A x64
cmake --build core/build/upgrade-copy-contracts --config Release --parallel 4
ctest --test-dir core/build/upgrade-copy-contracts -C Release --output-on-failure
python core/tests/upgrade-copy-contracts/capture.py --capture
python core/tests/upgrade-copy-contracts/capture.py --check
```

The evidence records all five actual suite results, exact compiled/copied inputs,
executable/library hashes and portable commands. It is a new author execution, not
an amendment to past independent QA. Copies of binaries stay in ignored `core/build`.
