# Terminal notification regression

MA058's immediate damage calls the terminal check, then the surrounding recipe
resolution calls it again. The former implementation emitted another `victory`
for an already victorious state. A lethal recoil could similarly emit `defeat`
in damage handling and again in the terminal check. The correction emits a
terminal notification only when the phase changes, checking zero HP first.

The diagnostic reproduced two victory events and two defeat events before the
fix. Real `CampaignRules(cinderwallUpgradeHooks)` still dispatched one victory
upgrade event: UGS036/065/129 healed 30 to 43 with max HP +1, UGS072 paid 10
Credits, UGS033 earned one entitlement, UGS065 spent one charge, and the receipt
and transaction sequence advanced once. Replaying the receipt changed nothing.
These exact effects must remain unchanged after the fix.

The regression also compares preview with committed state, verifies that killing
the last enemy while taking lethal recoil ends in defeat, checks repeated
Victory/Escaped completion boundaries, and ensures a provisional Victory can
still become Defeat. The saved-continuation cases are explicit boundary fixtures,
not additional player actions.

Build and execute from the repository root in PowerShell:

```powershell
& 'C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe' -S games/overkill-foundry/core/tests/terminal -B games/overkill-foundry/core/build/terminal-events -G 'Visual Studio 17 2022' -A x64
& 'C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe' --build games/overkill-foundry/core/build/terminal-events --config Release --parallel 4
& 'C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/ctest.exe' --test-dir games/overkill-foundry/core/build/terminal-events -C Release --output-on-failure
```

The target and shared library compile with `/W4 /WX /permissive-`. The original
failure is preserved under ignored `core/build/part-lifecycle/terminal-before.*`;
its executable SHA256 is
`857fe3dedf7910bab2ddd81b56eb86c5f2aab5a87491ab2208001258e2f7974a`.
That executed `core.cpp` SHA256 was
`c7d46a6475a78da59a78b70086e616e9aeed7e267418a8c07ed2b6b5c85d1adb`.
The historical runtime-linkage records and runner experiments are unchanged.

The correction changes subsequent event IDs and serialized `nextEvent`, so it
has deterministic replay compatibility implications even though the observed
combat rewards are unchanged. Version/save migration is an integration decision;
this fixture does not silently migrate old snapshots or relabel their evidence.
