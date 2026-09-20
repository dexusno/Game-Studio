# Physical-output lifecycle matrix

This author-operated native target selects all 207 physical-output
obligations. The selected inventory is a test plan until the exact named groups
pass. `evidence/results.json` records the executed graph, executable, library,
commands, logs and assertions. The proposed bindings are review input only;
`content/runtime-support.json` is unchanged.

| Executed family | Types |
| --- | ---: |
| Recipe-backed Ammo, including Warm Rivet | 64 |
| Recipe-backed Shield | 59 |
| Planning Modifiers | 28 |
| Physical Spread parts | 6 |
| Magnets | 21 |
| Generated fixed-value Ammo | 14 |
| Generated fixed-value Shield | 15 |
| **Selected total** | **207** |

`contracts.json` lists every exact `part:` identifier, authored origins and
resale basis, executed producer paths, test entry points, copy eligibility and
inapplicable lifecycle clauses. SH066 follows the explicit automatic formula
in `design/TIMING-AND-PERSISTENCE.md` section 3: spend
`3 * min(3, availableShield // 3)` after enemies and before retention/reset.
The historical 206-type report mistakenly required a player choice; that was
an evidence interpretation error, not a missing rule or production defect.
No owner-held haul composition is guessed by this matrix.

The graph is **provisional on corrected `of-core-0.4`**. The separate terminal
notification correction is present, while the rules-version/save migration
decision remains pending. This matrix changes no production rule. A later
version or source edit requires a new capture; it must not relabel this one.

## Run and capture

Run from the repository root in PowerShell:

```powershell
python -X utf8 games/overkill-foundry/core/tests/parts/generate_cases.py --check
& 'C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe' -S games/overkill-foundry/core/tests/parts -B games/overkill-foundry/core/build/part-lifecycle -G 'Visual Studio 17 2022' -A x64
& 'C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe' --build games/overkill-foundry/core/build/part-lifecycle --config Release --parallel 4
& 'C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/ctest.exe' --test-dir games/overkill-foundry/core/build/part-lifecycle -C Release --output-on-failure
python -X utf8 games/overkill-foundry/core/tests/parts/capture.py --capture
python -X utf8 games/overkill-foundry/core/tests/parts/capture.py --check
```

The isolated CMake target uses C++17 and `/W4 /WX /permissive-` against the real
shared core. It disables the ordinary core-tool targets only in this separate
build. Capture performs a clean native build, records actual per-type PASS
lines, runs CTest, and checks that all inputs stayed unchanged. `--check` only
validates the captured identities and exact selected group set.

## Physical contracts exercised

Every recipe-backed type receives real paid materialization with an authored
ingredient oracle, unique physical IDs, output count and source-copy payment
events. Current-price resale uses the immutable one-part reference rather than
the paid batch. A no-cost `grantPart` path is also exercised where its interface
can represent the part. Six typed Magnets require material choices that this
factory does not accept; only their actual paid Craft path is claimed.

The 123 original Ammo/Shield types exercise load/unload or install/remove,
depletion, used/unused distinctions, storage age, normal reset, source rarity,
real sale receipts and fight-end cleanup. SH071/SH102/SH118 make fresh next-turn
copies only where eligible. Copies remove an actual Fuse attachment or UGS140
production bonus while preserving printed effects and resale. UGS015 exercises
its different committed-properties copy on every paid Ammo recipe. SH072
returns eligible consumed Ammo once; unsupported rarities reject atomically.
Untouched pure Shield may be removed and stored; spent Shield cannot refill or
regain sale/copy eligibility by reinstalling.

Planning Modifiers are physical reserve objects consumed by Activate, not Fire
or Shield installation. Their selected payload assertions establish that
activation creates the intended once-only trigger/payment/attachment and that
the source cannot be reused. Unactivated parts retain their original age in
reserve. Cases include variable Heat and Shield payments, selected sacrifices,
the actual robot status screen, batch-output selection, SH043's 12-point cap
and second-shot rejection, and SH070's one-Iron nonqualifying Use before its
first qualifying discount. These are supplementary finite effect cases, not
new recipe bindings or exhaustive payload coverage.

SH066 now also exercises every available Shield total from 0 through 14, the
three-Iron cap, preservation of 1-2-point remainders through real MY3-03
retention, reserve exclusion, and actual enemy damage before payment. The
eligible Mara reader MA055 records 2 Iron before Sweep at 12 Shield, or 0 after
Sweep leaves 3; Sweep records its separate 3 Iron in either order. Reapplying
Sweep refreshes its binding without stacking or moving its priority. SH060's
earlier pre-reset loss leaves nothing to spend; reversing the order preserves
the Iron already recorded. Preview/reload, event order, next-turn delivery,
once-only expiry and armed fight-end cleanup are checked. These exercise the
T05/T06 ordering principles with eligible content; they do not claim the
out-of-pool Noor/MY3-21 example effects are implemented in Mara's pool.

All six Spread types instead load physically, unload reversibly, survive an
unfired End Turn, and are consumed with the bullet. Integer extra-hit values and
target choices are checked against explicit source numbers. Main Ammo Burn
does not spread; a Spread part's own Burn/Weaken does. Additional Heat is paid
at Fire, with unaffordable actions rejected unchanged. Two physical MA041 or
MA111 parts each pay their cost while respecting the printed non-stacking
extra-hit rule. SH110 reserves the exact Shield sacrifice, blocks other use or
sale, releases it on Unload, and consumes it on Fire.

All 21 Magnets exercise Auto, Miss, Good and Perfect collections, fitting the
source once, same-name refresh without stacking, selected material validation,
normal-mix versus typed additions, finite stock, Heavy Lift discards, exact
preview/apply parity, next-collection expiry and no repeat on a later haul.
Unfitted expired parts cannot be activated during Collection and disappear at
that boundary. SH126's perfect-only outputs and SH108's installed Shield grant
are checked separately from their material additions.

The 29 plain types use actual authored producers, not direct enumeration of
`grantPlainPart` outputs. The 14 Ammo values use a paid SH076 conversion with
controlled fixed printed Shield inputs; value 14 uses input 24 to cross the
cap. **Natural availability of every input value is not established.** Other
tested origins include real immediate Shield grants, shot reactions, installed
part deliveries, SH126's Perfect grant, MA119's scheduled output, Mayor pack
choices, an actual MY1-M1 Heat-payment reward, and UGS053's second-turn grant.
`producer_paths` lists exactly which origins were executed for each type;
additional manifest origins are outside that type fixture's producer claim.

Those generated objects retain their original fixed-value reference regardless
of producer ingredients, grant/copy provenance or current price vector. Their
behavior is ordinary damage or Shield, without the producing recipe's own
secondary hooks. Applicable printed copies and consumed-source returns retain
that reference and discard an attached bonus. Reserve storage, use, depletion,
real campaign sale receipts, victory/defeat cleanup and save round-trips are
executed for the accessible generated types.

**Generated Shield 1 has a different, explicit scope.** UGS028 creates it
installed inside atomic End Turn. The fixture shows it absorbing one point of
real enemy damage before normal reset deletes it. A terminal escape preserves
the installed object long enough to inspect its reference and snapshot, but
normal actions reject in that finished state and campaign cleanup removes it.
There is no intervening player removal, reserve, sale or copying window. Those
positive lifecycle actions are inapplicable, not invented to complete a list.

## Oracles and limits

`generate_cases.py` reads manifest/snapshot kind, rarity, ingredients and resale
expectations. The generated-origin cost checks also use that authored table,
not the runtime's recipe cost. `reference_values.json` preserves the finite
source-derived turn-one scalar table previously used by recipe author tests;
it is not independent QA for each recipe's conditional effects.

Controlled fixtures supply three 1000-HP enemies, abundant materials, selected
status/Heat values and protection. The original reference arena has main
Burn4/Corrosion2/Weaken3, other Burn2, three 10-damage intents, 80 installed
Shield, 8 Heat and 60/80 HP; additional cases quiet or replace those intents.
Campaign cleanup uses explicitly labelled 500-damage test Ammo. These inputs
isolate contracts and do not represent earned resources or balance runs.

The proposed coverage concerns physical types: creation, identity, applicable
copying, immutable price, placement and lifetime. It does not certify every
producer trigger, all effect branches, cross-upgrade combinations, natural
reachability, balance, presentation, usability or the full 665-operation gate.
No held composition choice is exercised. Independent review of the new graph
is still required before any support-ledger integration.

The original 123-type slice is preserved byte-for-byte under
`history/recipe-backed-123/`, with its source copies, executable and library in
ignored `core/build/part-lifecycle/history/recipe-backed-123/`. Its archive
index retains the original hashes. It is historical evidence, not overwritten
by this extension. The 206-type candidate is likewise preserved under
`history/physical-types-206/`, with all 41 source copies, executable, library
and logs in its recorded ignored capture directory. Its mistaken SH066
exclusion is retained as historical text, with the correction in the archive
index. The earlier 133-binding runtime audit is also unchanged.
