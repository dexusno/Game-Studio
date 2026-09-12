# Magnet Sweep — gameplay rebuild ready for owner play

Updated 2026-09-12. Stage: prototype. Version 0.2.0 / final R2 Windows Development package. The owner-authorized rebuild is implemented. Timeline is not a consideration. No publication or purchases were requested.

## Governing direction and current game

Klaus rejected the original concept demo as “utterly boring”: no felt goal, progress, melting reward, challenge, risk or magnetic impact. That negative human playtest supersedes earlier agent design confidence and functional QA. The former unlimited/risk-free rules at revision `8af011f` are historical; that artifact and owner's save remain separately under BuildOutput/ConceptDemo. BRIEF.md governs this rebuild.

Four fuel charges, salvage quotas and optional gold targets give each haul a purpose. Safe load starts at 24 kg with a 36 kg hard attachment ceiling. Material value differs from weight; connected bundles move together under actual continuous attraction. Hot cells or excessive mass trigger a persistent three-second fuse. Expiry spends one fuel charge and destroys the forecast highest-value unbanked salvage piece; cells are removed and remaining scrap returns to the tray. Banked money, XP, mods and collection remain safe. Running out of fuel below quota fails the attempt.

RMB Drop haul releases ALL cargo recoverably and turns the field off, replacing automatic hazard/cheap-piece sorting. Distinct spilled groups stay separated, with extra room around cells; large hauls may spread farther across the tray to find clear space. Smelting pays credits and XP, completion/gold bonuses, four ranks, six contracts, three upgrade branches with three tiers and six permanent collectible cores. Same-seed retry preserves career progress; completed runs remix arrangements. Physical motion, material-specific objects, force/impact/furnace effects, 19 original cues and a 96-second original music loop are integrated. Music and effects have separate controls.

## Build and actual evidence

- Final editor compilation passed; nine Unreal automation tests passed with zero failures/warnings in report 2026.09.12-10.19.45. Coverage includes atomic groups/capacity, persistent fuse and exact loss, charge consumption/final-charge end, whole-haul corner/full-rig recovery, rewards/purchases/collection, transactional saves and actual JSON/backup recovery. Legal-load model routes meet advanced 300/380/460 quotas at an affordable 40 kg capacity. This establishes feasibility, not native difficulty or enjoyment.
- Final Windows build/cook/stage/archive succeeded in 40.62 seconds. [Play Magnet Sweep.cmd](Play%20Magnet%20Sweep.cmd) targets BuildOutput/Rework/Windows/MagnetSweep.exe. Exact EXE/PAK/IoStore hashes and compact tests are in [evidence/rework-verification.json](evidence/rework-verification.json) and BUILD.md.
- Independent R1 native play visibly attracted and captured moving salvage, smelted it into an ingot, reached 132 salvage value / 212 credits and rank 2, banked the first rare core, bought a 150-credit capacity mod and opened the next contract. Real save/quit/relaunch preserved cargo, career and audio settings. Retry, focus pause and F11 restoration worked. R1 also exposed free hot-cell destruction; final R2 source/model tests close it with the one-fuel cost.
- Final R2 native play verified clean default entry, no missing ambient-cubemap warning, actual save/relaunch of a 7 kg mixed haul and whole-haul drop at the upper-right corner: both pieces recoverable, zero cargo, no loss and all four fuel charges retained. Root inspected the actual R2 render in [evidence/rework-board.png](evidence/rework-board.png). All 19 sound assets loaded; music and field audio components were observed active. Signal files pass reproducibility/peak/seam checks. No perceptual listening was possible.

## Current foreground and progress — preserve

During R2 testing, Sky detected user input and the tray state changed independently of the QA agent. ALL native automation stopped. The running game and current R2/player career may now contain owner play; **do not reset, quit or overwrite them without a coordinated handoff**. The original ConceptDemo save was never changed. Initial R2 began fresh after the earlier disposable R1 QA save was backed up, but fresh career is no longer guaranteed after external input. The current session was left under the user's control.

The one temporary QA-only Input.ini console-key attempt was unsuccessful; no slomo command ran. Root subsequently checked Saved/Config/Windows and found the temporary Input.ini absent. Preserve GameUserSettings and all career data.

## Limits and next concrete action

The nine final logic tests pass, but the native interruption left short-fuse rescue, unsafe furnace refusal, exact 24/36 kg boundaries, four tiny pours and final-charge receipt insufficiently exercised in R2. The R2 corner test did not yet recover a hazardous mixed spill with precision. QA-REWORK.md distinguishes these gaps from earlier R1 evidence and never counts independently changed state as an agent test. Formal hardware/performance coverage, hold-specific LMB/Shift combinations, listening, human enjoyment and voluntary replay remain unverified. Hard shadows, dense-haul recovery and late-career interest remain gameplay/presentation questions, not certified polish.

Next: Klaus plays the running rebuild and reports whether approaching a rich cluster, rescuing a load and melting/purchasing now feel worthwhile. Use that evidence for the smallest corrective increment. When foreground is explicitly available again, finish the listed R2 native failure/recovery checks; avoid adding content to conceal a weak interaction. Do not publish or infer commercial success. Shared studio validation reports only the existing unrelated Scrapstorm QA broken link. Preserve unrelated Dreambound owner changes.
