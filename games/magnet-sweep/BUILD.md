# Magnet Sweep — build and run

## Expedition Visuals 0.7.0

The current preview is the owner-requested major visual overhaul. It integrates ImageGen/TRELLIS/Blender hero art, 13 original Blender kit meshes, PBR materials, animated furnace heat, Barlow typography and the rebuilt industrial interface. Gameplay rules, prices and save schema are preserved.

[Play Expedition Visuals.cmd](Play%20Expedition%20Visuals.cmd) targets separate `BuildOutput/ExpeditionVisuals/Windows`, profile `expedition_visuals_preview`. The packaged child EXE is **332,956,160 bytes**, SHA-256 **9a57f4bf8ab3b891a3628919d063be317a44bf2205a06db6738e03d0d3575f03**. Build/cook/stage/archive passed in 29.90 seconds. Final regression report **2026.09.13-03.26.02** has 82 success/0failed/warning/unrun. Five packaged 1600×900 screenshots were inspected and the packaged fonts/licenses verified. Exact evidence and limitations: [visual-overhaul-verification.json](evidence/visual-overhaul-verification.json).

```powershell
pwsh -NoProfile -File games/magnet-sweep/scripts/Build.ps1 -Stage Editor
pwsh -NoProfile -File games/magnet-sweep/scripts/Build.ps1 -Stage Visuals
pwsh -NoProfile -File games/magnet-sweep/scripts/Build.ps1 -Stage Tests
pwsh -NoProfile -File games/magnet-sweep/scripts/Build.ps1 -Stage Package -ArchiveName ExpeditionVisuals
pwsh -NoProfile -File games/magnet-sweep/scripts/CaptureVisualAudit.ps1 -Extended -Executable games/magnet-sweep/BuildOutput/ExpeditionVisuals/Windows/MagnetSweep/Binaries/Win64/MagnetSweep.exe
```

Run each step only after the previous step succeeds. On a checkout without original runtime content, run the existing Content stage before Visuals (All also imports both in order). Visuals imports the checked-in original files; local TRELLIS inference is not required to rebuild the game. To regenerate the procedural kit, run Blender with `--background --factory-startup --python games/magnet-sweep/scripts/GenerateVisualKit.py` before Visuals. Keep tool installations and full raw TRELLIS output outside Git.

The capture command launches only its own offscreen game process, uses a unique save-disabled profile, verifies PNG completion and exits. The first 3 screens use real fresh-run callbacks. Extended adds 2 clearly labelled unearned late-site rendering fixtures with unchanged starter gear; it is not an earned gameplay run. These checks do not establish native input, sound, sustained performance, other resolutions or enjoyment. Loaded Rail cargo was not captured. All earlier packages and owner profiles remain separate.

## Preserved Expedition Choices 0.6.3


The current build adds affordable same-shop tool/support combinations, concrete purchase/refit forecasts and actual new free-starter notifications. It preserves the existing four-card stock size, earned frame routes, exact saved inventories and prior packages. A conditional support requires its actual tool to be fitted before purchase; no gear is automatically removed. Native rendering, input, sound, performance and enjoyment remain unverified.

Editor compilation and 82 engine cases pass (60 Expedition behavior, one shop diagnostic, 21 legacy), report 2026.09.13-02.01.30. Build/cook/stage/archive succeeded in 39.75 seconds. The earned Closed Circuit and Reaction full runs pass; actual Relay+Flow acquisition and a legal two-tool Conductive Tether refit pass. The 128-seed diagnostic is budget/availability evidence with declared reward assumptions, not physical gameplay or proof of fun.

[Play Expedition Choices.cmd](Play%20Expedition%20Choices.cmd) targets separate `BuildOutput/ExpeditionChoices/Windows` with profile `expedition_choices_preview`. The child EXE is 332,920,320 bytes, SHA-256 **3d5195fc81a5016a4a4cc2470540d774399264a698a8a2f4146ae9929b806ad6**. The package has not been launched. Exact source/artifact hashes, preserved old child hashes and verification history: [expedition-choices-verification.json](evidence/expedition-choices-verification.json).

```powershell
pwsh -NoProfile -File games/magnet-sweep/scripts/Build.ps1 -Stage Editor
pwsh -NoProfile -File games/magnet-sweep/scripts/Build.ps1 -Stage Tests
pwsh -NoProfile -File games/magnet-sweep/scripts/Build.ps1 -Stage Package -ArchiveName ExpeditionChoices
```

Run Tests only after Editor succeeds. The former 0.6.2 reproduction commands below apply to its archived revision `0a6b2db3a134aea0c25385765cf64fc9c3e3be68`; do not overwrite that archive with current source. Native control remains stopped at the unanswered foreground handoff.

## Preserved Expedition Frames 0.6.2

Current source adds one optional 40 kg power frame to each new late-site revision 2. Ordinary pickup stops at 36 kg. The frame needs an actual supported move or powered hoist, then explicit E receipt at FRAME DOCK. Receipt records its current appraisal once and leaves an installed machine. Final-site installation also supplies the existing counterbalance. Existing revision 1 worlds, depots and source identities remain available.

Walking Gantry is now a 14-credit, two-socket specialist. The first controlled comparison recovered the same 320 appraisal for 20 battery with Gantry and 22 with Hook + Ratchet; that result did not justify the earlier 24-credit price. Historical paid-24 receipts retain their actual purchase and 12-credit resale basis. New paid-14 receipts use 7-credit resale. No owner save has been read or changed.

Editor compilation and **76 engine cases pass** (55 Expedition plus 21 legacy), report **2026.09.13-01.24.34**. Build/cook/stage/archive succeeded in **44.46 seconds**. The actual earned Gantry run recovers both late frames and reaches victory; electrical and Reaction alternatives use clearly labelled equipment fixtures with real physical inputs. Old/current receipts and frozen old layouts pass. The first report's retry failure was a test attempting input while intentionally paused; the corrected witness uses the normal Resume control.

The child EXE is **332,807,168 bytes**, SHA-256 **d5d2741ed194ab9fc7298400445624397238b53079946675063845e612a2a50f**. Earlier 0.6.1 and 0.6.0 child hashes remain unchanged. Actual native controls, rendering, sound, performance and enjoyment are still unverified.

[Play Expedition Frames.cmd](Play%20Expedition%20Frames.cmd) targets the separate `BuildOutput/ExpeditionFrames/Windows` archive and its own `expedition_frames_preview` profile. It has not been launched. Reproduce the current increment with:

```powershell
pwsh -NoProfile -File games/magnet-sweep/scripts/Build.ps1 -Stage Editor
pwsh -NoProfile -File games/magnet-sweep/scripts/Build.ps1 -Stage Tests
pwsh -NoProfile -File games/magnet-sweep/scripts/Build.ps1 -Stage Package -ArchiveName ExpeditionFrames
```

Current evidence: [expedition-frames-verification.json](evidence/expedition-frames-verification.json). Native control remains stopped at the unanswered foreground handoff.

## Preserved Expedition Sites 0.6.1

The following describes the package committed at `609e8dd8de0ae44576c5dc1f7c9d874669f15349`.

This archived source revision adds distinct later worksites and independently witnessed support effects. [Play Expedition Sites.cmd](Play%20Expedition%20Sites.cmd) targets the separate `BuildOutput/ExpeditionSites/Windows` archive and its own `expedition_sites_preview` profile. Build/cook/stage/archive succeeded in 39.86 seconds. All 69 engine cases pass (48 Expedition plus 21 legacy), report 2026.09.13-00.38.45. Both actual 480-output recovery routes, remote counterweight alternatives and electrical final dispatch pass. Starting equipment in the late capability cases is a labelled fixture; output, rewards and physical actions are actually earned. The previous 0.6.0 child hash is unchanged.

The first two sites preserve their teaching and earned recovery routes. The third is a **Balanced Recovery Rack**: place 10–14 kg of resting material on each platform, with no more than 2 kg difference, then secure the 16 kg core. Three valuable optional targets need support, physical fracture or heat transfer; loose ordinary pools total 208, below the first 240 refining milestone. The final **Counterweight Exchange** uses a 20 kg core and a 20 kg replacement weight. Basic controls can stage the core to make room; Winch or Relay can move the weight while the core stays secured. A preserved finite generator can power isolated receivers.

World schema 3 records the exact layout ID/revision. Packaged schema 2 restores frozen E1 geometry, including existing depot previews and site-entry retries. Restoring an old depot refreshes its capability information without rerolling its stock. New depots exclude Closed Circuit where no functioning return loop exists. This exposes an unresolved design issue: some expensive combinations lose useful opportunities before the finale; current encounter variety is not proof of a rewarding complete build.

The recovery panel, markers and hints follow the saved worksite definition. Valuable anchored, hot and functional objects show their actual appraisal. Authored support/power connections identify the payload being manipulated. Actual rendering, sound, native controls and enjoyment remain unverified while the earlier foreground handoff is unanswered.

```powershell
pwsh -NoProfile -File games/magnet-sweep/scripts/Build.ps1 -Stage Editor
pwsh -NoProfile -File games/magnet-sweep/scripts/Build.ps1 -Stage Tests
pwsh -NoProfile -File games/magnet-sweep/scripts/Build.ps1 -Stage Package -ArchiveName ExpeditionSites
```

Current verification history: [expedition-sites-verification.json](evidence/expedition-sites-verification.json). Exact encounter contracts and open gameplay issues: [EXPEDITION-SITES.md](design/EXPEDITION-SITES.md).

## Preserved Expedition Lab 0.6.0

This package corresponds to source commit `2045723cdeb8d77cb36fbb24bfe47a995801ef84`. Its commands below describe that revision; use the separate archive above for current source.

The new `-Expedition` mode runs a separate four-site salvage expedition with a fresh rig, earned depot choices and a physical final core. [Play Expedition Lab.cmd](Play%20Expedition%20Lab.cmd) targets `BuildOutput/Expedition/Windows/MagnetSweep.exe` with its own `-ExpeditionProfile=expedition_preview`. Build/cook/stage/archive succeeded; all 52 engine cases pass (31 Expedition plus 21 legacy) in report 2026.09.12-23.45.53. The package has not been launched natively. Exact artifact hashes and test history are in [expedition-verification.json](evidence/expedition-verification.json); native and effect-coverage limits are in QA-EXPEDITION.md.

The mode is selected before any old career loading occurs. World schema 2 is isolated from every old career. Its composite rig/world/checkpoint lives in `Saved/Expedition/<profile>.json`; `DemoProfile` and `DemoFresh` are ignored. The launcher does not reset a run. A damaged current save can restore its valid backup; if neither validates, the originals remain until an explicit new run. No owner files have been used to test this path.

- LMB buys one ordinary capture batch for 6 battery. Shift narrows its field; Space toggles capture. Release turns the field off and retains secured cargo. The planning menu contains the deliberate/continuous comparison.
- Q/F: hold to aim while the magnet stays put, release the same key to use its fitted tool. Invalid actions cost nothing. Preview shows cost, load and risk. Uncommitted marks are cancelled by drop, pause, banking or another tool; paid physical movement survives aiming and pause.
- Rail preparation lets you select a single held body to launch (C cycles), or explicitly check at least two iron bodies to weld. Welding costs 4 and does not fire; the next separate launch costs 8. Other held material is preserved.
- Fitted supports expose relevant operation buttons beneath Q/F: ground an endpoint, arm a sensor/receiver, transfer heat into a chosen iron sink, switch a saved anchor, or lay a guide. Multi-stage selections explain the next target; only the final valid operation commits its displayed cost.
- RMB drops the whole haul onto the worksite. To sell scrap, carry it to the furnace, wait for it to settle and press E. Metal types can be mixed. The protected mission core is delivered at the separate receiver with E.
- Follow the recovery panel: slide the collar into its stop, place ballast in the catch, and place an iron brace in the arm stopper. Recovering the first three cores pays 10/12/14 credits. Refining can earn two additional 2-credit milestones per site. The fourth core completes the expedition and archives the rig. On the second site, the supported machine and live assembly hold most of the upgrade value; two tested capability routes reach 362/366 output, while ordinary haulable pools total 168.
- Safe mass is 24 kg, hard limit 36 kg. An unsafe/hot haul starts a three-second fuse. Timely drop preserves the haul; expiry spills it recoverably and spends 12 battery. Actual hazard contact can damage struck scrap. Secured core delivery remains possible at zero battery.
- Escape pauses all physical clocks. Retry restores the complete site entry, including cash, spent battery, world, rig and refining awards. M toggles sound. Save & quit retains the expedition.

Build this package separately:

```powershell
pwsh -NoProfile -File games/magnet-sweep/scripts/Build.ps1 -Stage Editor
pwsh -NoProfile -File games/magnet-sweep/scripts/Build.ps1 -Stage Tests
pwsh -NoProfile -File games/magnet-sweep/scripts/Build.ps1 -Stage Package -ArchiveName Expedition
```

The current possible owner Clarity session remains protected. No native control resumes without a safe foreground handoff. The older launchers and packages below remain available.

## Historical Extraction E1


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
