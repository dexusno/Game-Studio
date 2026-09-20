# Independent upgrade probes

Read `../upgrade-contract-review.md` and `../upgrade-contract-evidence.json` for dispositions. The frozen 54-input candidate passes nine independent groups and fails Q09's UGS-121 copy rule. Exit 1 is the expected historical reproduction, not a clean candidate pass.

All paths below are relative to the game directory. The recorded Windows build used MSVC 19.44.35228.0 and the Visual Studio 2022 CMake installation. `cmake` and `ctest` must resolve to that toolchain, or use their installed absolute executable paths. No production source is compiled from the live tree.

```powershell
$upgradeCapture = 'core/build/upgrade-contracts/captures/native-upgrade-contracts-3fae26e0acb5b90d-49b5f5045b55/inputs'
cmake -S "$upgradeCapture/core/tests/upgrade-contracts" -B core/build/qa-upgrade-contract-rebuild -G 'Visual Studio 17 2022' -A x64
cmake --build core/build/qa-upgrade-contract-rebuild --config Release --parallel 4
ctest --test-dir core/build/qa-upgrade-contract-rebuild -C Release --output-on-failure
$upgradeInput = (Resolve-Path $upgradeCapture).Path.Replace('\','/')
$upgradeLibrary = (Resolve-Path 'core/build/qa-upgrade-contract-rebuild/core-runtime/Release/overkill_core.lib').Path.Replace('\','/')
cmake -S qa/upgrade-contracts -B core/build/qa-upgrade-contracts -G 'Visual Studio 17 2022' -A x64 "-DFROZEN_GAME=$upgradeInput" "-DFROZEN_LIBRARY=$upgradeLibrary"
cmake --build core/build/qa-upgrade-contracts --config Release --parallel 4
& core/build/qa-upgrade-contracts/Release/independent_upgrade_contracts.exe
python qa/upgrade-contracts/audit.py --records core/build/qa-upgrade-contracts/frozen-49b5f504
```

`audit.py` performs read-only artifact/source/label checks and rejects four in-memory corruptions. It does not import the author's generator or infer semantic completeness. Without `--records`, it checks the current author evidence directory; use the archived records to reproduce this historical review. Raw paths, compiler output and binaries remain in ignored build directories. Public output is limited to the concise text and JSON records beside this file.

Q01–Q05 independently exercise the three repaired predicates, discounts, physical outputs and atomic campaign behavior. Q06–Q08 cover missing fresh-grant, reaction-order and recurrence clauses. Q09 checks both Jig/Lease acquisition orders with and without an unrelated selected-copy damage bonus. Q10 checks delayed printed-only copies. Controlled material stocks, HP, memory entries, reward cards and narrow content pools are explicit test setup; production Rules/CampaignRules resolve all tested effects and costs.
