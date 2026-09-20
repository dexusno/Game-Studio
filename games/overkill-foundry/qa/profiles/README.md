# Independent profile probes

These five groups supplement the author's storage tests. They use real Windows
SaveStore/CampaignSession and the preserved 2016 sources, without Unreal or a
player profile. The C1 repair is a separately identified single-source override.

From the repository root, with the installed MSVC CMake on PATH:

```powershell
$frozenGame = (Resolve-Path 'games/overkill-foundry/unreal/Saved/Validation/profiles-native-archive-20260920-2034/source/games/overkill-foundry').Path
cmake -S games/overkill-foundry/qa/profiles -B games/overkill-foundry/core/build/qa-profiles-fixed -A x64 "-DFROZEN_GAME=$frozenGame" "-DPROFILE_SOURCE_OVERRIDE=$((Resolve-Path 'games/overkill-foundry/core/build/qa-profiles/inputs/FoundryProfiles-c1.cpp').Path)"
cmake --build games/overkill-foundry/core/build/qa-profiles-fixed --config Release --parallel 4
```

The override must hash to
`871b3c6016c5cc7f7d0e9fa8be45475c3249e72d10e7f053f80106e02d9aaf58`.
Omit `PROFILE_SOURCE_OVERRIDE` and use a separate build directory to reproduce
the original frozen failure. Do not substitute current core sources.

Prepare a new isolated directory for every execution; the probe deliberately
keeps its files for inspection. Create `alias-target` and `Profiles` beneath it,
then a real NTFS directory junction named
`Profiles/99999999999999999999999999999999` targeting that `alias-target`.
Pass three positional arguments to `Release/independent_profiles.exe`:

1. That new case directory.
2. The original archive's `native` directory.
3. The preserved original `defeat-ready-prepared.ofsave`, SHA-256
   `565ee873ec03903cd8175eb68fc71cf96fbbce033b0dcd8ca7262027469a1b51`.

The last file is supplemental evidence copied to
`core/build/qa-profiles/inputs/` from the original fixture folder; the final
native archive's `defeat-input.ofsave` has already become the replacement run.
Never use it as the pre-defeat input. Every path above is relative to the game
unless the repository prefix is shown.

`audit_evidence.py` independently verifies the source graphs, original binaries,
all referenced native/recheck artifacts and supplemental fixture. It only reads
the author archives. Optional `--output` writes a compact audit result at the
specified QA output path. A matching hash establishes identity, not coverage.

`evidence/initial.txt` retains the initial defect; `evidence/recheck.txt` is the
actual repaired-source execution. Binaries, copied sources and sandbox campaigns
remain in ignored `core/build/qa-profiles*`. The report and exact identities are
in `../profile-review.md` and `../profile-evidence.json`.
