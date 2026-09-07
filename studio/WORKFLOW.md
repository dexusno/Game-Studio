# Workflow

This is a set of useful completion checks, not a requirement to run every stage for every request. A bug fix starts in the affected game; an explicit idea can start with its brief. Use short documents and continue within existing authorization.

| Stage | Work and evidence | Durable record | Decision |
|---|---|---|---|
| Discover | Find rising-interest topics in trend sources; derive original playable hooks; then compare games and assess audience, scope, timing, and uncertainty | Dated research run plus opportunity JSONs | Recommend one experiment; preserve alternatives |
| Define | Describe the player fantasy, core loop, difference, cut list, target platform, budget, and what the prototype must teach us | Game brief and manifest | Choose the smallest useful playable test |
| Prototype | Implement controls, feedback, success/failure, and restart; expose tuning values; test the riskiest mechanic first | Runnable build, BUILD.md, STATUS.md | Playtest, revise, or park |
| Prove the feel | Observe a real play session; note input latency, comprehension, frustration, and voluntary replay | QA.md with build and observations | Call fun promising only with evidence; record missing human feedback |
| Polish a slice | One representative segment with final-direction art, sound, UI, rewards, transitions, and performance | Brief's design/experience sections plus playable build | Establish the quality bar before multiplying content |
| Produce | Add only content/features justified by the loop; integrate saves, settings, progression, and platform behavior as needed | Small tasks in STATUS.md; meaningful decisions in DECISIONS.md | Finish a coherent game within budget |
| Verify and prepare release | Independent QA, packaged build, rights/credits, current store requirements, truthful screenshots/trailer, recovery plan | QA.md, RELEASE.md, marketing/, asset manifest | Resolve release blockers; prepare a reviewable submission |
| Release and learn | Perform authorized store actions; collect supplied or accessible aggregate results; support real defects | Release record and concise retrospective | Improve this game, reuse a lesson, or start another |

## Research and selection

Use [the research method](../research/README.md). For next-game discovery, scan trends across all subjects before deriving varied game concepts, then compare paid games and check scope. A trend can inspire mechanics, setting, visuals or other elements; literal simulation is optional. A searchable name is optional too. The core game must remain enjoyable after the trend fades; potential organic discovery is an upside, not a sales forecast. The owner's USD 9.99 ceiling is conditional on genre/price evidence and a worthwhile product achievable within the work budget; it is not a target price. Keep commercial potential, confidence, and fun evidence separate. A near-peak opportunity can work if its attention window outlasts development, store lead time, and promotion. Reuse a theme only in an original expression with suitable rights.

Do not silently select a project when asked only to research. If the owner authorizes autonomous selection or prototyping within a budget, record the choice and proceed. A candidate status and a game stage answer different questions; link the selected candidate with `source_opportunity` in the game manifest.

## A playable test

Use a bounded, repeatable segment and a clear learning objective. Match the test to the intended game: the 2026-09-07 owner feedback requires more substantial skill, strategy, ability/build interactions and progression than a single-attack arena. The next owner-facing slice should include varied combat actions, contrasting threats, useful reward choices and visible progression in representative 3D presentation. A technical control probe alone must be labelled as such. Evaluate whether rewards change decisions while also checking that the central actions are satisfying.

Record the build identifier, machine/input method, observations, and next tuning change. Automated simulation can test scoring and physics invariants; it cannot prove delight. A worker who has not run the game must label their contribution a code or design review. If direct play is unavailable, finish the runnable package and clear controls so the owner can provide the missing observation.

## Integration and independent review

Assign nonoverlapping files and explicit interfaces before parallel work. The producer or integration owner reconciles the changes and runs the actual build. Give the reviewer an acceptance target and a build or commit, not just the implementer's summary. Fix substantive defects and rerun affected checks; do not keep generating process artifacts after the work is verified.

Check representative input, capture/focus and pause behavior, accessibility settings appropriate to the game, save/load and recovery, difficulty/progression, display modes, startup/exit, and frame pacing. Verify the packaged artifact from a clean launch. State untested hardware/platforms plainly.

For Unreal, reuse the established Breakout workflow where appropriate: editor compilation, scripted content generation, packaging, ordinary rendered smoke checks and native input testing of the packaged game. A headless or NullRHI check cannot establish visual quality. The owner has authorized computer control when needed; coordinate foreground use while continuing background work where practical. Read existing validation notes as historical evidence, then record the exact new build and interactions actually tested.

## Engine and asset choice

Choose an engine for the game's rendering, interaction, deployment target, existing tools and maintainability, respecting the owner's current direction. The current 2026-09-07 cyborg/worlds direction uses Unreal, first person and one modular weapon/shield; those owner choices are settled. Verify ignored `config.local.json`, Epic installation manifests and existing studio tool configuration before claiming an engine is unavailable. Other engines and viewpoints can be reconsidered for a later explicit direction. Check current official licensing and platform requirements when making the choice.

Before downloading a pack, check the license and whether committing its source publicly is permitted. Record generated, procedural, original, and third-party assets with their origin. Capture representative in-game views and inspect them; do not report visual quality from source code alone. Keep raw high-resolution media, engines, and packaged builds out of Git. Decide storage/LFS for substantial assets before introducing them.

## Resume and ship

At a handoff, keep `STATUS.md` short: current stage, last verified build, completed change, remaining issue, next exact action, owner input if genuinely needed. Update `game.json` when the stage changes. The CLI derives the portfolio from those manifests.

For design/research before a game directory exists, the existing candidate JSON can reference a short handoff through `context.entrypoint`. Use `python scripts/studio.py context` to discover both kinds of work; `status` remains the game-stage report. Read the [continuity workflow](CONTINUITY.md) before crossing task or worktree boundaries. Keep proposals separate from owner decisions, update the authoritative record during work, review and refresh its content checkpoint at a useful handoff, and integrate that record into the branch the receiving task will use. Chat compaction and optional memories do not replace this handoff.

Prepare store work early enough for current onboarding and review lead times. Refresh official policies for the intended store, platform, monetization, multiplayer, AI disclosures, achievements, and content where applicable. Record which requirements actually apply. Produce the build and store materials before any final approval that is still needed. Public release, legal agreements, and outbound messages are separate actions whose authorization must come from the user.

After release, record actual results over a stated time window: paid units, realized revenue, refunds, reviews, traffic or wishlists when available, owner hours, and support burden. Do not invent missing analytics or add telemetry automatically. Feed the practical lessons back into research and the relevant skill only when a recurring lesson warrants it.
