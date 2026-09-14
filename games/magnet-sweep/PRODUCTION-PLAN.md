# Magnet Sweep — rebuild production plan

**Parked project, 14 September 2026.** The complete redesign is now the independent [Scrap Combat game](../overkill-foundry/STATUS.md), public title pending. Continue redesign work there. This directory preserves the old Magnet Sweep implementation and historical records; it is not the new game's codebase.

> 2026-09-13 superseded-plan notice: the owner rejects the current game's overall coherence and requests a complete 2.5D side-view redesign. The current draft for owner review is [the complete redesign plan](design/REDESIGN-PLAN.md). Klaus will set each implementation goal individually. Everything below is historical career/rebuild context; neither this old plan nor the expedition recommendation in design/BUILD-SYSTEM.md authorizes new implementation. Preserve old saves and builds.

Updated 2026-09-12. The owner rejected the first playable demo and authorized implementation of meaningful goals, challenge, risk, satisfying attraction/melting, improved graphics and sound, music, upgrades, leveling and loot. [BRIEF.md](BRIEF.md) is the current contract. The prior unlimited, risk-free brief and plan at Git revision `8af011f` are historical rejected alternatives.

**Development timeline is not a consideration.** Complete and assess the intended experience through dependency-led increments. Do not replace gameplay judgment with deadlines, asset counts, successful compilation or additional feature lists. Root integrates shared records and the package; bounded specialists preserve one another's assigned files.

## Current increment: make the coil change achievable hauls

Klaus rejected radius-only progression because it changes no possible accomplishment. Extraction E1 implements aimed F extraction of a selected linked piece, cancelling broad attraction and leaving unwanted weight behind. Tiers allow 1/2/3 extractions between real smelts; spent uses and severed links persist. Optional mixed assemblies provide concrete selection opportunities while all baseline quota routes remain. Existing saves and purchased tiers are preserved. The game-designer/model specialist, gameplay critic and independent QA exchanged mechanic and failure cases; root owns integration.

Acceptance is the observable 20 kg cargo + 8 kg mixed bundle case: normal capture becomes unsafe at 28 kg; F takes the valuable 4 kg piece alone and creates a safe 24 kg haul. Higher tiers must provide useful further selections within the same finite fuel budget. Ordinary pickups cannot satisfy the coil-use lesson, and the lesson must not force a wasteful tiny pour. Both the exact extraction and F→E→RMB cancellation are exercised through runtime callbacks; actual physical input and presentation remain pending.

The current Clarity game remains under possible owner control after Escape. Extraction is packaged separately with its own persistent preview career; no native control, save reset or migration occurred. Do not replace that live session to obtain a QA result.

## Current implementation and acceptance

Extraction 0.5 editor/Windows package and 21 engine automation cases pass (eleven Rework, ten Tutorial). Independent static critique found and prompted fixes for queued-smelt cancellation, misleading bundle-versus-selected mass, lost guard-off explanation and wasteful immediate-smelt teaching. Exact tests and pending live routes: [QA-EXTRACTION.md](QA-EXTRACTION.md). No actual Extraction playtest is claimed. Historical Clarity/Tutorial/Rework evidence remains in its original QA files and did not predict the owner's negative novice experience. Current artifact and reproduction: [BUILD.md](BUILD.md).

| Work package | Current implementation | Acceptance needed in the actual package |
| --- | --- | --- |
| Physical collection and readable choice | Continuous attraction; combined-mass linked bundles; hold/toggle field and precision controls; aimed F extraction with purchased uses; stable attachment; 150% ordinary capture ceiling | Scrap visibly accelerates and impacts the magnet; group behavior matches visible links; ordinary play and narrow approaches remain readable; toggles, UI boundaries and focus do not produce accidental capture |
| Risk and finite contracts | 24 kg starting capacity; four fuel charges; six quotas/gold targets; persistent three-second fuse plus stabilizer; one-charge and highest-value-piece loss; recoverable whole-haul drop; stable-only furnace | The player sees the exact impending loss and can deliberately prevent it; tiny smelts visibly consume a batch; failed/unreachable and already-completed attempts resolve correctly; dropped groups remain separately recoverable at tray corners without tedious overlap |
| Melt reward and lasting career | Whole-load credit and ingot payoff; one-time completion/gold bonuses; spendable credits and lifetime XP; four ranks; three branches with three tiers; six banked core identities | First ordinary melt feels consequential; first clear affords a real choice; purchase visibly changes its advertised capability; rare find is at risk before banking and permanent afterward; finish/continue/abandon clearly preserve or forfeit the stated material |
| Presentation and audio | Rebuilt scene/materials, attraction and impact effects, visible rig/cargo/furnace; 19 original cues and 96-second original music; separate volume settings | Review motion, silhouette, HUD, depth and lighting at gameplay scale; listen to ordinary and dense pickups, danger and melt together; music remains soothing and important cues audible; generated/concept imagery is not substituted for rendered evidence |
| Career continuity and repeat play | Deterministic six-job arrangement families; same-seed retry; changing run seed; saved career/attempt/fuse; atomic save and valid backup | Real save/quit/reopen retains unstable cargo and its remaining fuse, heat/bonus ledgers, mods, collection and audio settings; retries do not duplicate rewards or roll back purchases; upgraded revisits and changed approaches motivate voluntary play |
| Packaged verification | Unreal editor build, engine tests and Windows package produced | Independent native pass on the exact candidate, defect repairs with focused rechecks, fresh owner entry state and accurate remaining limitations |

The dependency chain is **readable physical collection → meaningful batch/risk decision → convincing smelt and purchase payoff → progression and replay comparison → verified owner-facing package**. These are implemented systems awaiting integrated evidence, not permission to expand content or release publicly.

## Representative verification sequence

1. Fresh First Pour: identify the 150-credit upgrade goal, safe capacity and four fuel charges. Ignore the suggested route: capture iron and smelt immediately, then bank mixed metal. Check banked versus carried value, furnace/button/E/drag-release, whole-haul drop and the labelled guard at capacity and beside a red cell. Exercise left-mouse hold and Space field toggle.
2. Complete a conservative first contract and observe the full melt/bonus/level feedback. Buy one 150-credit improvement, confirm the guard-off warning and the improvement's physical change, then try an approach that uses it. For the coil, observe actual selected F extraction and demonstrate the 20+8 versus safe24kg comparison; ordinary capture is insufficient evidence. Fill the saved space before banking when useful, and test three selections at tier3 without a shared-hub bypass. The first dangerous pickup must pause for the actual rescue lesson. Compare finishing after quota with pursuing gold/core using remaining batches.
3. After the actual purchase and first rescue practice, provoke a controlled overload. Inspect the forecast one-charge cost and highest-value loss, release the field and verify the fuse continues; rescue one haul by dropping all cargo recoverably and let another expire. Confirm one fuel charge and the forecast item are lost, remaining salvage spills and banked earnings are safe. A lone hot cell must also spend fuel when it fails; a last-charge quench ends the attempt without retracting earned quota/gold. Unsafe furnace clicks must explain refusal. Compare ordinary attraction with Shift/Q precision beside hazards and linked bundles.
4. Fail with four inadequate batches; retry and inspect retained money/XP/mods. Bank a core, replay its family and confirm honest duplicate salvage value without duplicate collection. Inspect all six job definitions and relevant locks.
5. Run actual save/quit/relaunch, including an unsafe load and partial fuse. Check pause/focus, menus, music/SFX/mute, fullscreen/window restoration and pointer/HUD mapping. Use disposable profiles for corrupt-save or failure injection; preserve the owner's career.
6. Observe representative dense/full-rig play and an advanced job at the equipment state a normal player owns when it unlocks. Record frame pacing and the actual native/audio observations. The legal-load model planner passes advanced quotas with an affordable 40 kg rig, but this is feasibility evidence, not actual first-unlock difficulty.

Automated checks already cover atomic group/refusal rules, smelt heats and bonuses, forecast loss and persistent fuse, venting, purchase accounting/ranks/collection, deterministic job populations/legal model loads, transactional validation and real JSON/backup recovery. They do not establish visual comprehension, challenge, sound quality or enjoyment. Broaden tests when a change or observed failure justifies it; avoid repeatedly certifying unchanged code.

## Player evidence and smallest corrective increment

The next human observation must test the complete revised promise without coaching toward approval. Ask: **When nearly full, what did you choose to do next, what did you risk, and did the smelt make you want another haul?** Record spontaneous versus prompted replay and the exact build.

Prioritize the smallest change that addresses the observed failure: attraction/impact if the magnet feels weak; warnings or affordances if outcomes are unclear; batch composition and geometry if decisions collapse into a routine; payout/upgrade presentation if progress feels invisible; audio arrangement or mix if events lack weight. Keep readable challenge and earned power together. Do not multiply jobs or add another currency to conceal a failed core loop.

Specific unresolved hypotheses are whole-haul recovery becoming tedious, wider coils making hazard avoidance feel like a penalty, advanced-job balance before a fully funded rig, and replay interest after the finite upgrades/collection. Test these directly. Different seeds alone do not establish replayability.

## Completion boundary

Deliver a runnable, independently checked rebuild and honest current evidence for the owner's next playtest. Preserve actual source, assets, provenance and handoff together; root owns final STATUS/DECISIONS/game.json updates and reviewed commit. Resolve observed functional blockers and describe untested targets plainly.

Publication, store work, price validation, outreach and purchases are outside this rebuild assignment. The NOK 20,000 profit-per-game objective remains a commercial test for a later product; passing this development plan does not predict demand or revenue.
