# A richer game direction after the Sixfold playtest

**Resume current work:** read [STATUS.md](STATUS.md), or run `python scripts/studio.py context opportunity:reactor-raider` from the repository root. This report preserves historical comparisons; the handoff routes new tasks to current decisions and relevant detail.

Research and local verification: **2026-09-07**. Sixfold Recoil remains parked. The initial three-direction comparison is preserved below. Klaus subsequently selected **first person and one modular weapon that forms a shield**, then proposed a cyborg whose dreams reveal real worlds with different technology, magic and procedural biomes. The current [integrated cyborg/worlds design](cyborg-worlds-design.md) combines the requested writer, art director and producer work with [attachment/progression design](weapon-shield-design.md). Exact lore and carryover rules remain proposals; no new game has been built. The former reactor-raider opportunity record retains its identifier while following this evolving concept.

Klaus found the delivered beta boring: one all-at-once attack, repeated movement/shooting/recall, little strategy, no powerups or meaningful progression, and presentation too close to Breakout. That is direct owner feedback. Session duration, exact mode, completion and retry count were not supplied. The working browser package and passing technical tests are preserved in the [game record](../../../games/scrapstorm/STATUS.md); technical correctness did not establish enjoyment.

The owner wants substantially greater challenge, learnable skill, interacting abilities and builds, exploration, progression and attractive Unreal 3D presentation. “Ten times” was explicitly illustrative. There is no literal complexity multiplier, new spending allocation or hard time cap. First person is now chosen; the camera comparisons below are historical. Native computer control is authorized when needed, including editor work and beta testing.

## What the inspirations contribute

These are reported features from developer/publisher sources accessed on 2026-09-07, not our own hands-on reviews. The design lessons are hypotheses for our game. The [detailed gameplay comparison](gameplay-directions.md) records the source dates, examples and limitations.

| Inspiration | Useful lesson for this task |
|---|---|
| [Roboquest](https://store.steampowered.com/app/692890/Roboquest/) | Combine aimed combat and movement with distinct weapons, class upgrades and persistent Basecamp development. Reward practice and build choices together. |
| [Gunfire Reborn](https://store.steampowered.com/app/1217060/Gunfire_Reborn/) | Let weapon and ability interactions change how the player fights. Different drops should suggest different plans. |
| [Witchfire](https://store.steampowered.com/app/3156770/Witchfire/) | Give exploration, guns, spells, extraction and arsenal development consequences across expeditions. Continuing deeper can be a meaningful decision. |
| [Remnant II](https://www.remnantgame.com/en/news/article/11537003) | Use complementary skills, weapon powers and archetype progression to create recognizable loadout identities. This is an action RPG reference, not a proposed content inventory. |
| [Risk of Rain 2](https://store.steampowered.com/app/632360/Risk_of_Rain_2/) | Let items and abilities interact, and unlock further ways to play after an individual clear. Keep effects and scaling readable. |
| [Ravenswatch](https://www.nacongaming.com/en-US/ravenswatch) | Character differences, build choices and preparation under pressure can coexist with a top-down camera. Camera and mechanical depth are separate decisions. |

This is a mechanics and production research pass. It does not establish a rising market, likely sales or an optimal price. Strong incumbents also set a demanding quality bar.

## Initial three-direction comparison

| Rank and perspective | What the player does | Decisions and lasting progression | Main risk |
|---|---|---|---|
| **1. First-person reactor raider** | Explore a vertical storm foundry, use distinct guns, dash, deploy a control device and spend suit charge to empower an attack or defense. | Choose how charge is generated and spent; take routes offering different modules; extract weapon plans and open further regions. | Crowded FPS roguelite field. Charge can become another repetitive optimal rotation. |
| **2. Third-person monster hunter** | Hunt in a ruined mountain sanctuary with committed melee, parry/dodge, an aimed binding stake and harvested creature abilities. | Break a dangerous body part for immediate safety or preserve it for a desired graft; combine powers and develop equipment between expeditions. | Creature animation, hit detection and camera quality carry a substantial production burden. |
| **3. Third-person breach engineer** | Fight through a partially powered ruin using guns, field anchors, devices and machinery. | Spend scarce expedition power on a safer fight, a valuable route or extraction; restore workshop functions and access deeper districts. | Machinery can become switch chores. Interacting world states increase authoring and testing work. |

The [full proposals](gameplay-directions.md) include actual encounters, example builds, persistent progress, a representative first biome, exclusions and failure criteria. Candidate records: [reactor raider](../../opportunities/reactor-raider.json), [monster hunter](../../opportunities/graft-hunter.json), [breach engineer](../../opportunities/breach-engineer.json). All are drafts with low confidence and unverified fun; their provisional scores do not select a project.

For the recommended direction, imagine a shield carrier covering a rifle unit while a flanker climbs a side stair. You could gain height and shoot around the shield, hold the flanker with a control field, or spend your charge to stagger the carrier and push through. That expenditure changes what defense remains available. Later, a precision-based module pairing rewards weak-point shots and lining enemies up; an aggressive pairing rewards close entries and well-timed retreats. A route fork changes which upgrades you can obtain. These are proposed alternatives the encounter must actually support.

The recommendation favors precision combat, movement, builds and exploration together. The hunter becomes the better fit if visible equipment and deliberate melee are the more compelling fantasy. First person still needs excellent weapons/hands, recoil, enemy animation and camera comfort; it does not remove animation work.

**Independent challenge:** the game-design and art reviews were separate. Integration checked the lead concept against [Battle Shapers](https://www.battleshapers.com/), whose developer already describes boss powers, build enhancers and reward/trap manipulation in an FPS. A robot shooter with acquired powers is insufficient differentiation. Our proposed reserve allocation and route rewards must produce distinct player behavior. The hunter also faces overlap with [Arboria](https://store.steampowered.com/app/924070/Arboria/). Neither combination is assumed novel or commercially validated.

## Camera and game length

Top-down constrains what we can do with spatial immersion, vertical aiming, traversal and close-up character detail. It can support substantial strategy, as the Ravenswatch reference illustrates. The failed demo's small decision space was a separate problem. For this owner direction, a freely navigable first- or third-person world offers a better starting fit; changing camera alone will not make the combat interesting.

Separate the **demo**, an **expedition/run**, and **completion of the full game**. The proposed demo covers a representative first biome. A successful expedition would bank plans or other lasting progress and open further objectives. A later campaign would develop across expeditions toward an ending, with optional challenge content afterward. Death/reset rules would preserve clearly identified unlocks. This is a proposed structure; no replacement campaign has been implemented or timed, so the previous 90-minute discussion is not a completion estimate for these directions.

## Procedural levels and runs — owner refinement, 2026-09-07

Klaus wants procedurally varied levels and runs that remain playable and fun. This refines the replacement-game comparison; the recommended FPS remains a draft. Use **designed 3D encounter modules plus constrained procedural assembly**. Modules supply deliberate cover, movement space, elevation, entrances and landmarks. Generate the route network and compatible room configurations, then select encounters, objectives and rewards. Different biomes would have their own module families and gameplay identities. A finite library will contain recognizable elements; infinitely unique content is not promised.

| What changes across new runs | What remains dependable |
|---|---|
| Route connections, loops, optional branches and compatible room configurations | A traversable path to required objectives and exits, useful landmarks and coherent architecture |
| Enemy combinations, patrol approaches and tested hazards | Readable attacks, response windows, movement space and limits on simultaneous pressure |
| Upgrade offers, shops, optional objectives and reward locations | Useful early choices, sufficient required resources and completion with the standard movement/combat kit |
| Selected modifiers and environmental details | Clear modifier explanations, deliberate pacing, consistent art and lasting campaign progression |

**Example, not implemented:** one foundry run offers an elevated turbine route with a sniper encounter and precision upgrades, or a lower route with flankers and close-combat equipment. Another connects a cooling chamber to the workshop, with an optional rescue offering a control-device choice. Show enough information to weigh those routes. Geometry, threats and rewards should change the player's plan; cosmetic differences alone do not satisfy the goal.

Use authored pacing patterns with exploration, escalating combat, rewards, recovery and a finale. Reserve traversal and interaction space before adding decoration. Enemy combinations need compatibility rules as well as a difficulty budget. Keep boss attacks learnable and vary only arena/escort arrangements tested with them.

**Unreal evidence:** Epic's UE 5.8 documentation describes PCG graphs for procedural content, asset placement and editor/runtime generation, including modes for larger spaces. [PCG overview](https://dev.epicgames.com/documentation/en-us/unreal-engine/procedural-content-generation-overview), [generation modes](https://dev.epicgames.com/documentation/en-us/unreal-engine/using-pcg-generation-modes-in-unreal-engine), accessed 2026-09-07. **Proposed implementation:** game-specific layout and encounter rules determine the playable structure; PCG assists appropriate asset placement and dressing. Generate the next level before entry so validation and loading can finish. No procedural system has been implemented or tested here.

**Playability checks to implement:** validate route/objective and key/resource dependencies; instantiate layouts in Unreal and check collision, door alignment, player clearance, navigation and enemy access; check spawns, encounter limits and rewards. Exercise saved regression seeds, deliberately difficult combinations and fresh batches. Reject invalid layouts before entry, limit retries, and use a known-valid fallback if generation fails. These checks reduce defects without proving every possible seed fair or fun.

Save the run seed, generator/content version, chosen layout and changing run state. Separate layout, encounter, loot and decoration randomness so an art change does not inadvertently rearrange a run. A new expedition normally receives a fresh seed; resuming restores the same expedition. Replaying a seed within a compatible build should reproduce its starting setup for controlled practice and bug reports. Test save/version behavior explicitly.

**Fun test:** the owner beta should offer multiple generated versions of one representative biome, with the combat, upgrades, boss and persistent unlocks already required. Compare a designed reference encounter with generated encounters. Observe whether route/build choices change behavior, whether pressure and recovery feel appropriate, whether arrangements remain readable, and whether Klaus wants another expedition. If layouts all play alike or assembly harms combat, improve the modules and rules before expanding the library. Campaign milestones and an ending remain part of the proposed larger game.

## Practical graphics and asset routes


Use one coherent visual family, then give it distinctive original equipment, landmarks, material treatment, animation and effects. A suitable starting direction is tactile stylized 3D with detailed surfaces and strong lighting; a scanned realistic environment is also feasible if selected for the fantasy. Judge it while moving and fighting.

| Route | Useful contribution | Current evidence |
|---|---|---|
| **Blender + Unreal** | Original weapons, gadgets, modular structures, enemy shells and procedural variations; export geometry for Unreal materials, collision and animation. | Blender supports background/Python workflows; it was not found in the checked local installations and has not been executed here. First validate a small export with the installed version. [Official command-line manual](https://docs.blender.org/manual/id/5.1/advanced/command_line/arguments.html), [Blender licensing](https://www.blender.org/about/license/). |
| **Coherent asset libraries** | Quaternius for editable sci-fi modules, Synty for a consistent stylized cast/world, selected Fab/Megascans content for realistic environments. | [Quaternius Megakit](https://quaternius.com/packs/modularscifimegakit.html) is CC0; only part is free, while its complete Source offering includes the advertised Unreal integration. [Synty Dungeon Realms](https://syntystore.com/products/polygon-dungeon-realms) needs separate animation. Exact Fab item/entitlement determines availability and terms. |
| **Materials, lighting and motion** | Poly Haven/ambientCG foundations, suitable animation libraries, Epic's Game Animation Sample, retargeting and cleanup, Niagara effects. | [Poly Haven](https://polyhaven.com/license) and [ambientCG](https://docs.ambientcg.com/license/) provide CC0 assets. Epic published a [Game Animation Sample update for UE 5.8](https://www.unrealengine.com/tech-blog/download-the-latest-game-animation-sample-project-now-updated-for-ue-5-8) on 2026-08-12. None of these candidate assets has been imported in this research. |
| **Generated imagery and 3D services** | Original visual concepts, decals and texture inputs; candidate props from text/image-to-3D, followed by cleanup and inspection. | [Meshy's game-asset guide](https://docs.meshy.ai/en/webapp/guides/use-cases/game-assets) and [Tripo's feature guide](https://www.tripo3d.ai/help/getting-started/what-features-does-tripo-have) describe generation, mesh processing, texturing and rigging routes. These are vendor-reported possibilities; service access, output quality and integration were not tested. |

For Unreal, use lighting to establish atmosphere and readable depth, animation to communicate anticipation/contact/recovery, and effects to communicate hit timing and ability states. Procedural tools can help construct scenery while route and encounter design remain intentional. Engine feature availability is not evidence of a finished look. New experimental features, including UE 5.8's toon shader and MCP tooling, need evaluation before becoming dependencies. [Epic's UE 5.8 overview](https://www.unrealengine.com/news/unreal-engine-5-8-is-now-available).

The [art pipeline report](art-pipeline.md) compares eight concrete routes, camera/animation burdens and asset terms. Blender scripting is particularly useful for hard-surface and modular work; polished organic characters and animation require more iteration. Generated or purchased models still need geometry, UV, scale, collision and deformation checks. Store exact acquisition terms in the game's existing asset manifest when assets are obtained. Keep restricted source packs in private storage. No new assets were acquired during this research.

## Verified local Unreal and Breakout workflow

Observed locally: the installed engine is **Unreal 5.8.2**. An isolated background editor process successfully executed Python with **zero reported errors or warnings**. Import, material, actor, static-mesh and automation API symbols were present. This proves editor scripting execution and API presence; it did not import an asset, render a scene or build a replacement game. Private paths, scratch project and raw logs remain ignored. [Epic's editor Python documentation](https://dev.epicgames.com/documentation/en-us/unreal-engine/scripting-the-unreal-editor-using-python).

The existing Breakout project's README, build script, launcher and validation record document a reusable sequence: compile the editor target, generate content through editor Python, build/cook/package, run smoke checks with the ordinary renderer, and exercise native packaged-game controls. Its validation includes pause, cursor capture, focus loss and resolution checks. Those are historical Breakout results, not fresh verification of a new project. Its later beta also contains campaign stages, upgrades and pickups; its pipeline does not imply an arcade scope limit.

Klaus explicitly confirmed that computer control used by Breakout is acceptable for this work. Use it when needed and coordinate foreground input; background compilation, scripting and asset work can continue alongside the owner's computer use. Concurrent heavy builds or rendering can affect responsiveness, and native input shares the desktop.

The local installation includes Epic's experimental Unreal MCP plugins, but they are not configured or tested as an active connection. Editor Python is the automation route actually exercised here. Blender was not located through PATH, standard installation folders or filtered installed-program records; this was not an exhaustive disk search.

## Earlier test proposal — refined by the cyborg/worlds design

After choosing a direction, establish its concrete movement/combat design and art/animation sources. For the recommended FPS, first disprove the risky charge system internally using the same mixed vertical encounter with two genuinely different build pairings. Stop expanding if both builds reduce to the same behavior, charge mostly creates waiting, or basic shooting does not reward practice.

The **owner-facing beta** should then contain multiple generated versions of the representative biome: distinct weapons and active abilities, enemies requiring different responses, exploration and route choices, rewards that change subsequent decisions, a boss, and a real saved unlock usable on re-entry. Compare reproducible seeds and fresh runs, including save/resume of the same expedition. It needs coherent 3D presentation, readable motion, sound, failure/retry and tested native controls. This is the slice needed to assess the intended game before multiplying campaign content. No final campaign length, hardware performance, asset cost or implementation schedule is claimed established.

Record whether Klaus can explain a meaningful choice, whether practice improves outcomes, whether an upgrade changes the next fight, and whether the rewards make him want to continue. These observations determine expansion. The current research recommends a direction and preserves alternatives; it does not silently select or start the next game.

## Access and confidence limits

All external sources in this run were accessed on 2026-09-07. Current undated listings are identified as such in the specialist reports; historical developer updates retain their publication dates. No comparator was personally played for this research, and no sales or trend measurement was produced. Several English Blender documentation fetches failed; one official localized command-line reference was accessible. The previously suggested Ravenswatch domain returned unrelated content on recheck, so it was excluded and replaced with the Nacon publisher page. Some asset media/dynamic listings were inaccessible; those limits are retained in the art report.
