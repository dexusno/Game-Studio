# Sixfold Recoil — one-level beta experiment

Historical prototype brief, 2026-09-07. The owner subsequently rejected the delivered experiment and the game is parked; see [STATUS.md](STATUS.md). The original test boundary below is preserved alongside the [full v0.2 design](../../research/runs/2026-09-07-scrapstorm-design/README.md).

**Play question:** does aiming, throwing protection away, dodging and catching the six pieces feel good enough that Klaus wants another attempt and a larger game? Compare harmless and cutting recall without claiming extra damage proves better route play.

## Playable boundary

One 24 × 14 Open Tray arena, one robot, six conserved pieces, three hull, Biter and Spitter ordinary enemies and a short Foreman using learned shooting patterns. Three finite groups build pressure, then Foreman ends the level. No cards, metagame, other arenas, campaign or permanent power. Duration emerges from play. All pieces gather harmlessly between groups; hull persists. Immediate retry on win/loss.

Harmless global recall at speed 10 is the default. A menu comparison enables cutting recall: speed 7.5, one new-crossing return hit per launched piece/cycle, no overlap/toggle damage. Recall wins simultaneous input by default; a settings comparison permits throw to interrupt recall, requiring a fresh recall press afterward. No ammo selector or extra attack interface.

Use v0.2 movement/fan/range/block/hull rules. Readable shots, blocks, damage, loose versus returning pieces and enemy aim lock. All attacks must remain avoidable while empty.

## Demo content

- Opening: two Biters, followed by two Biters + one Spitter after a kill and at most one remaining.
- Crossfire: three Biters + two staggered Spitters, split between warned entries.
- Pressure: four Biters + two staggered Spitters, with changed approaches.
- Foreman: 24 hull, learned aimed/fork shots, at most four Biter escorts over the fight with two alive maximum. Immediate finish on boss defeat; no invulnerable timer or shield bypass.

Each spawn has a 0.9-second warning and a safe position at least three units from Pip. Normal live cap six; shooters stagger tells. Groups clear when all assigned enemies die, including queued entries. Brief safe transitions allow the next warning; no survival clock.

## Experience and controls

Original code-drawn toy robot on a tactile workbench: warm metal, cyan scrap, coral danger, chunky silhouettes. Smooth movement/aim, crisp clacks and tuned catches, restrained sparks, optional shake and separate music/effects volumes. UI scales and stays clear of the arena.

WASD/arrows move, mouse aims, left click throws, right mouse holds recall, Escape pauses. Standard gamepad mapping where the browser supports it; physical-device testing must be reported separately. Keyboard-operable menus. Focus/visibility loss pauses and clears actions. Results save locally; no network telemetry.

## Engine and budget

Canvas 2D / Web Audio, JavaScript without external dependencies. Installed Node 24 assembles/checks it; Python 3.11 serves an optional localhost preview. A self-contained HTML must also open offline by double-click. This prototype engine choice does not fix the eventual native release engine.

No hard development or owner-time cap. No purchases/new cash allocation. Measure actual execution/rework, included tool capacity, extra charges and owner attention separately. Manifest day/hour zeros mean no numeric allocation imposed, not zero effort or a zero-time deadline. One playable level bounds this experiment.

## Integration contract

`src/core.js` defines global `SixfoldCore` and a CommonJS export with `Game` and `CONFIG`. `new Game({returnDamage, interruptRecall, seed})`; `step(dt,{moveX,moveY,aimX,aimY,firePressed,recallHeld})`, aimX/Y being a direction vector. Wrapper runs fixed steps. Public `setPaused(bool)`, `clearInput()`, `drainEvents()`, `snapshot()`.

Readable fields: `state` playing/won/lost, `paused`, `time`, `stage` 0–3, `stageName`, `phase`, `player` x/y/r/hull/grace/aim, `pieces` (six objects, held/outbound/ejected/loose/returning), `enemies`, `shots`, `spawns`, `stats`, `heldCount`. Enemy fields: id/type/x/y/r/hp/maxHp/phase/aim/tell/tellDuration/locked/flash. Spawns: x/y/type/time/duration. Engineer must return actual schema before integration if it differs.

Events: launch, empty, rejected, land, collect, recallStart, block, hull, enemyHit, enemyDie, shoot, warning, stageClear, win, lose. Event objects include type and x/y where applicable; hits include amount and returnHit; collects include held count. Core is independent of DOM/audio.

`src/audio.js` defines `SixfoldAudio.Sound` with `unlock()`, `setVolumes({music,sfx})`, `event(event)`, `setScene({active,paused,recalling,danger})`, `dispose()`. Lazy user-gesture unlock; safe without Web Audio; bounded voices; no sound while paused/hidden.

## Validation and next decision

Check conservation, return overlap/toggle damage, guard/ejection, finite clear/loss/retry, input arbitration, frame behavior, pause/focus, audio, save-failure fallback, resizing and standalone packaging. Independent QA inspects the actual artifact. Automated checks/simulated interaction do not establish fun.

Klaus's playtest: what felt good/awkward, whether cutting recall changes movement or only ease, and whether another attempt is wanted. Separate defeat retries from post-clear replay. Hold campaign expansion for this feedback; broader progression remains a later design decision.
