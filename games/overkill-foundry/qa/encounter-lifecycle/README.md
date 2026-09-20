# Independent encounter/lifecycle review

This directory reviews the two unchanged root-authored source archives. It does
not build the live rules or combine the archives. Cinderwall encounter/status
graph `ce72d5dce85a` precedes the Shared delayed-Shield correction; campaign graph
`94daf9b8e9a3` includes that correction. Both precede UGS-121 copy attribution.

From the repository root, with the installed MSVC/CMake toolchain:

```powershell
python games/overkill-foundry/qa/encounter-lifecycle/audit.py
python games/overkill-foundry/qa/encounter-lifecycle/audit.py --build all
python games/overkill-foundry/qa/encounter-lifecycle/audit.py --probes all
```

The first command checks the two author graphs, their nine original artifacts,
and exact named group/link references. The second configures the unchanged
archived CMake projects. The third builds this review's boundary probes against
each matching archived core. All native outputs remain under ignored `build/`.
Each native command/log/binary identity is recorded in the portable JSON records;
`${GAME}` denotes the game directory, not an omitted command argument.

The author captures used different aggregate algorithms: recorded input order,
`path + TAB + sha256_lf + newline` for encounters, and `path + NUL + sha256_lf +
newline` for campaign. Both are UTF-8 SHA-256. Raw bytes, sizes and LF hashes are
also independently checked. `audit.json` verifies both formulas; the earlier
`rebuild-all.json` retains the initially pending campaign aggregate check, which
was resolved without changing any source or rebuilding its executable.

`clause-review.json` examines all 15 proposed operation links. Fourteen have
support within the stated MVP core scope. `core.profile-and-unlocks` proves the
ProfileFacts/Mara-to-Ivo layer here; whole-profile support additionally needs the
separate profile-manager/confirmation/storage evidence.

Prepared HP, statuses, route prefixes, helper combinations, materials, prices and
terminal ammunition are labelled in the probes. They are not evidence of earned
builds, natural acquisition, balance, UI, disk durability or later campaigns.
Supplemental sources for Patrol and eligibility are copied/hash-recorded
separately; they do not alter either historical runtime graph.

Two initial QA fixture errors (Glass instead of Copper for SH002; an incorrect
death-event name) were corrected after source inspection. Their initial sources
and results remain in ignored `build/fixture-diagnostics`; neither was a game
defect. The final checks use the corrected fixtures only.
