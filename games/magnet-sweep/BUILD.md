# Magnet Sweep — build and run

Unreal Engine 5.8.2, Windows Development, version 0.5.0 / Extraction E1. Klaus rejected the radius-only coil; this increment gives it a selected-piece extraction capability. It remains a prototype for owner judgment.

## Play and preserve progress

[Play Extraction Preview.cmd](Play%20Extraction%20Preview.cmd) opens the new `BuildOutput/Extraction/Windows/MagnetSweep.exe` with its own persistent `extraction_preview` career. It does not overwrite or migrate the current Clarity career. No Extraction game has been launched during this increment. Finish using the current game before starting a different package.

[Play Magnet Sweep.cmd](Play%20Magnet%20Sweep.cmd) targets the corrected `BuildOutput/Clarity/Windows/MagnetSweep.exe`. The entire adjacent Windows folder is required. The earlier 0.3 package/save remain in BuildOutput/Tutorial, the 0.2 rebuild remains in BuildOutput/Rework and the rejected 0.1 demo remains in BuildOutput/ConceptDemo. [Continue Previous Demo.cmd](Continue%20Previous%20Demo.cmd) still opens the 0.2 Rework package.

The first objective is 150 credits for an actual magnet improvement. The side panel tracks that goal and then the rig's 9 installed improvements; orders and 6 rare cores are the longer goals. This version does not add construction of a larger building or machine. It makes the existing salvage-and-upgrade purpose explicit.

## Controls and rules

- After fitting an **Extraction Coil**, aim at a linked piece and press **F** to pull only that piece. Other weight remains on the tray and broad attraction stops until a new player action. Tiers allow 1/2/3 extractions between successful smelts; dropping, quenching and reloading do not recharge them. The tooltip compares the normal whole-bundle load with the selected F load.

- Hold left mouse or toggle Space to attract metal. Release on the tray keeps cargo.
- Click **SMELT HAUL**, click the furnace, press E, or release a tray-origin drag over the furnace/button to bank a stable haul. Any metal mix counts. Guidance never refuses a valid deposit because an unrelated lesson is incomplete.
- Right mouse drops the entire haul recoverably onto the tray; this is not a furnace deposit.
- Shift or Q narrows the field. Connected pieces have one combined pickup weight/value, shown in the tooltip.
- Each smelt uses 1 of 4 fuel charges. Pickup numbers mean carried value; melting pays credits and XP. The first order's 120 salvage target adds 80 bonus credits, enough for a 150-credit upgrade.
- The labelled tutorial guard blocks red-cell capture and loads above safe capacity until the first actual upgrade purchase. The first guided sweep pauses around 36 carried credits or at capacity, but smaller deposits remain allowed.
- Purchase explicitly switches off the guard. The first unpractised dangerous pickup pauses for a real rescue; subsequent excess weight or red cells run the normal fuse. Expiry spends 1 fuel and destroys the most valuable unbanked piece, if any; banked progress stays safe.
- Tab opens Workshop; Escape pauses; R requests retry confirmation; M mutes; F11 changes display mode; F9 saves an actual screenshot. Pause includes separate music/effects controls, saved practice, return-to-career and Save & quit.

## Reproduce

Machine paths come from ignored config.local.json. No engine is copied into source control.

```powershell
pwsh -NoProfile -File games/magnet-sweep/scripts/Build.ps1 -Stage Editor
pwsh -NoProfile -File games/magnet-sweep/scripts/Build.ps1 -Stage Tests
pwsh -NoProfile -File games/magnet-sweep/scripts/Build.ps1 -Stage Package -ArchiveName Extraction
```

For a clean content setup, also run `-Stage Content`. That imports existing original meshes/audio and saves the Concept map. This increment adds no external assets or new audio. `-ArchiveName` defaults to Extraction and accepts another simple directory name. All does not include Tests; run Tests explicitly. Tests validates a fresh report and fails on failed/unrun cases even when Unreal exits zero.

The runtime career is `MagnetSweep/Saved/Rework/player.json` with a valid previous backup; optional tutorial metadata keeps compatible numeric lesson IDs. Old Release/Bundle saves reconcile toward smelting, and actual banked value recognizes an interrupted payout. Pause's practice run uses `Saved/Tutorial/player.json` separately. Skipping/completing practice does not erase its earnings; an explicit confirmed restart replaces only practice. Retry keeps banked career progress.

`-DemoProfile=qa_name -DemoQA` selects an isolated QA profile; `-DemoFresh` resets only that chosen profile. Never use the owner's career for destructive QA. The original 0.3 owner save was backed up before this increment under ignored BuildOutput/OwnerBackups/before-clarity-v04. The original and its backup were also copied without overwriting an existing destination into the Clarity normal-career save location. The normal launcher continues that copied career; it has not been launched or verified natively in Clarity yet.

The live qa_clarity session must now be preserved as possible owner progress: physical Escape stopped computer control during QA. Do not reset it or replace the foreground. The separately prepared qa_clarity_legacy copy has not been launched. A later native check requires a safe foreground handoff.

## Verification

Extraction E1 editor compilation and 21 automation cases pass in fresh report 2026.09.12-21.46.20, with zero failures/warnings/unrun tests. Eleven Rework and ten Tutorial cases cover selected extraction versus unsafe bulk capture, all three use limits, real callback field cancellation, F→E→RMB cancellation, delayed/once-only payout, real coil teaching, saved severed links/spent uses, legacy loading and ordinary quota feasibility. Optional assembly center clearances were 42.46 units or greater in the tested six default layouts, with at least 25 units of wall inset. This is model geometry evidence, not visual inspection.

Build/cook/stage/archive succeeded in 39.47 seconds. Child EXE SHA256: 771df590f395bd1a4dc76bf79142f334133a70d6f161d37c027d425ccc7ab8dd. Artifact identities and exact cases: [extraction-verification.json](evidence/extraction-verification.json). Independent static review and untested native routes: [QA-EXTRACTION.md](QA-EXTRACTION.md). Physical F input, tooltip/target readability, pull animation, owner-save continuation and enjoyment remain unverified; prior owner Escape keeps native control stopped.

The following Clarity results are historical, from the preceding 0.4 package:

Editor compilation and 17 engine automation cases pass in report 2026.09.12-19.41.26 with zero failures/warnings/unrun tests. Eight Tutorial cases include real immediate iron/mixed payouts from early/legacy states, training guard capacity/cells/atomic groups, actual earned purchase, natural release over furnace or button 40 versus other UI, no duplicate payout, legacy saves and transactional metadata. Nine Rework cases retain the existing economy/risk/save coverage.

Build/cook/stage/archive succeeded in 40.01 seconds. Child EXE SHA256: 49c9e253f2446c761d6b38f4ad75b558ba6efdcb81d5f8bc47acfc1bc3e036de. Exact artifact identities and compact tests are in [clarity-verification.json](evidence/clarity-verification.json). Independent reported-behavior checks and their limits belong in [QA-CLARITY.md](QA-CLARITY.md); historical evidence remains in QA-TUTORIAL.md and QA-REWORK.md.

Native play confirmed immediate off-route iron payout, a safe 24 kg training cap and immediate whole-haul drop without spending fuel or banked money. Physical owner Escape stopped the pass before the mixed-haul, spare-capacity cell, first purchase, continuous drag-release and copied-save checks. Automated callbacks cover logic for these paths but do not establish rendered input or visual comprehension. A successful prescribed route did not predict the owner's experience. Human enjoyment, long-term purpose/replay, perceptual listening and broader hardware/performance coverage remain unverified.
