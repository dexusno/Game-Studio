# Encounter and status mechanics contracts

The isolated strict C++17 target passes nine named groups and 3,946 assertions
against the 28-input frozen graph recorded in [evidence.json](evidence.json).
CTest passes 1/1. The evidence lists six robot and four status candidate links;
they remain pending independent clause review, with no active binding changed.

The tests exercise each body's authored HP, defenses, actions and core value,
Binder packet variation, Cask pressure, the Nest's finite helper supply and locked
intent, Press brace/Leak timing, Pursuer per-hit scaling/completed-turn motor,
and Chassis death release. Player/enemy Burn, Corrosion and Weaken boundaries
include defenses, timing, decay and lethal interruption. The actual ordered
events and complete resulting state must match replay from a serialized input.

Fixtures deliberately prepare HP, statuses, deliveries and ordinary part grants.
They establish those isolated rules, not natural acquisition, encounter balance,
human feel or all combinations. The candidate uses the provisional corrected
`of-core-0.4`; it establishes no compatibility with older same-version binaries.

```powershell
cmake -S games/overkill-foundry/core/tests/encounters -B games/overkill-foundry/core/build/encounters -G "Visual Studio 17 2022" -A x64
cmake --build games/overkill-foundry/core/build/encounters --config Release --parallel 4
ctest --test-dir games/overkill-foundry/core/build/encounters -C Release --output-on-failure
```

The recorded run builds the immutable copy at
`core/build/encounter-contracts-final/source/core/tests/encounters` instead.
Its copied inputs, executable, library and full CTest log remain in that ignored
archive. Source hashes were checked after execution. The historical runtime
ledger remains at 133/665; these results must not silently replace its identities.
