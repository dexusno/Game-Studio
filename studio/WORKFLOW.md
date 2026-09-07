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

Use a short, repeatable loop and one learning objective. Examples: does a tether feel controllable, is a chain reaction readable, or does a reward choice change the next run? For a first test, one arena and a few variations can be enough. Do not build a progression economy to hide an unsatisfying central action.

Record the build identifier, machine/input method, observations, and next tuning change. Automated simulation can test scoring and physics invariants; it cannot prove delight. A worker who has not run the game must label their contribution a code or design review. If direct play is unavailable, finish the runnable package and clear controls so the owner can provide the missing observation.

## Integration and independent review

Assign nonoverlapping files and explicit interfaces before parallel work. The producer or integration owner reconciles the changes and runs the actual build. Give the reviewer an acceptance target and a build or commit, not just the implementer's summary. Fix substantive defects and rerun affected checks; do not keep generating process artifacts after the work is verified.

Check representative input, capture/focus and pause behavior, accessibility settings appropriate to the game, save/load and recovery, difficulty/progression, display modes, startup/exit, and frame pacing. Verify the packaged artifact from a clean launch. State untested hardware/platforms plainly.

## Engine and asset choice

Choose an engine for the game's rendering, interaction, deployment target, existing tools, and maintainability. Unreal remains an option, especially for strong 3D presentation. Godot, Unity, web technology, or custom code may fit other games. No studio rule forces browser play or a particular engine. Check current official licensing and platform requirements when making the choice.

Before downloading a pack, check the license and whether committing its source publicly is permitted. Record generated, procedural, original, and third-party assets with their origin. Capture representative in-game views and inspect them; do not report visual quality from source code alone. Keep raw high-resolution media, engines, and packaged builds out of Git. Decide storage/LFS for substantial assets before introducing them.

## Resume and ship

At a handoff, keep `STATUS.md` short: current stage, last verified build, completed change, remaining issue, next exact action, owner input if genuinely needed. Update `game.json` when the stage changes. The CLI derives the portfolio from those manifests.

Prepare store work early enough for current onboarding and review lead times. Refresh official policies for the intended store, platform, monetization, multiplayer, AI disclosures, achievements, and content where applicable. Record which requirements actually apply. Produce the build and store materials before any final approval that is still needed. Public release, legal agreements, and outbound messages are separate actions whose authorization must come from the user.

After release, record actual results over a stated time window: paid units, realized revenue, refunds, reviews, traffic or wishlists when available, owner hours, and support burden. Do not invent missing analytics or add telemetry automatically. Feed the practical lessons back into research and the relevant skill only when a recurring lesson warrants it.
