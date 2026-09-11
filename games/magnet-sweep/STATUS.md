# Magnet Sweep — status and resume

Updated 2026-09-11. Stage: concept. Owner: Klaus. Working title; no playable build.

## Governing owner direction

Klaus selected the magnet game for the first experiment and requested three game designers to pitch and debate directly, a professional production planner, and a critique agent. He explicitly clarified that **the critic assesses gameplay, fun and bad design; development timelines are not a consideration.** This supersedes earlier day/hour ceilings and studio defaults for this assignment. Keep the game simple, coherent, engaging, rewarding and replayable. Do not sacrifice these goals for a calendar.

The commercial objective remains at least NOK 20,000 cash profit per game, with further games conditional on real worthwhile results. No play, build, sales, purchase or publication evidence exists.

## Current recommendation and evidence

The three designers exchanged pitches and challenged each other. Following the gameplay critic's objections, they substantially revised their original timed score design. The current [BRIEF.md](BRIEF.md) governs:

- Untimed sweeping with desirable scrap, retained cargo, no hard capacity or load slowdown.
- Deliberate ring-drag/release tugs through a broad finite preview corridor.
- Earned Breakaway releases selected tangle plus directly linked neighbor rings inside that corridor; no recursive propagation.
- A forgiving deliberate furnace dump advances a visible goal below the available salvage supply. Complete the whole satisfying action before offering transition; leftover scraps remain optional.
- Retain visible Breakaway and reach improvements. Fresh deliveries with the complete rig change useful starting tangles, links, seams and directions.
- Positive feedback for the current action, without standing record pressure that makes ordinary sweeping feel wasteful. After final upgrades, forge recovered metal and complete deliveries without promising nonexistent further improvements.

The [gameplay critique](design/CRITIQUE.md) records genuine disagreements and changed judgments. Designer revisions in design/01-interaction.md, design/02-progression.md and design/03-replay.md supersede their preserved first-round proposals. The replay designer supplied a linked fan and offset fork/seam example respecting the direct-link rules. These are document judgments and designed test cases, not observed fun.

The professional producer completed the revised [PRODUCTION-PLAN.md](PRODUCTION-PLAN.md): implementation dependencies, owned outputs and quality evidence, with no development timetable or time-based stop rules. Time fields in game.json are intentionally unset (schema uses zero), not a zero-hour estimate.

## Unresolved gameplay questions

- Does the magnet feel satisfying and cooperative before reward explanations?
- Can players understand sweep/latch/dump without surprise state changes?
- Are pull corridors expressive and forgiving, or do players tediously search previews?
- Do the predicted linked bursts feel authored by the player and remain readable?
- Is a large uncapped haul satisfying to dump, rather than a perfunctory final click?
- Do forge goals give purpose without chores or neutralizing earned power?
- After all improvements, do new arrangements invite genuinely different pulls and voluntary replay?

These questions must be answered in a playable build. More templates or upgrades cannot establish missing enjoyment.

## Next action and ownership

Root integrates BRIEF, STATUS, DECISIONS, game.json and source opportunity. The three designers own their notes; the gameplay critic owns CRITIQUE; the professional producer owns the revised production plan. Design and plan integration are complete. Root accepted current-delivery save/resume and a clear “Pour & next delivery” action for surplus cargo, preserving the complete deposit payoff. The planning records passed JSON, whitespace and 43 local-link checks across 14 Markdown files. Full studio validation reports only the pre-existing parked Scrapstorm link to missing build/Sixfold-Recoil-Beta.html; no new-game issue is reported. Content checkpoints and the local commit preserve this reviewed handoff. No gameplay evidence is implied.

The next concrete action is an implementation assignment for an isolated Windows project with the representative sweep/tug/linked-burst/deposit/earned-improvement loop. Do not withhold the defining action behind an untested cleaning sequence. Actual save/input/presentation/package evidence must replace assumptions as work proceeds. The requested design, professional production plan and gameplay critique are complete. No implementation has begun; no unresolved owner preference blocks the proposed next step.

[BUILD.md](BUILD.md) records read-only Unreal 5.8.2/tool presence and the separate doctor configuration-schema error. [QA.md](QA.md) records the untested behaviors. Preserve unrelated parked Dreambound input/audio changes; do not stage or reset them.

## Verification boundary

Only planning artifacts were checked. Unreal was not launched for this game, no package was built and no human fun/replay test occurred. The separate studio doctor issue remains the private tool-configuration schema mismatch recorded in BUILD.md; no private configuration was changed. Any receiving task must use the committed revision or saved main checkout containing these records and preserve unrelated owner edits.
