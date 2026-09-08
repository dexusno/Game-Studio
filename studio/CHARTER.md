# Studio charter

## Purpose

Make games people enjoy enough to recommend or buy, using AI-assisted production to keep the owner's investment small. Klaus is the creative owner and player, not the person expected to configure compilers or coordinate agents. The Breakout beta demonstrated useful production capability; it does not determine this studio's genre or engine.

## Economics and scope

A few hundred paid copies can be a worthwhile result when implementation takes a few days and limited owner time. Evaluate each project against its own cash, human-hour, and elapsed-time budget. Do not impose a full-time studio salary target or assume that cheap production creates demand.

Default planning envelope, adjustable by the assignment: one platform first, one memorable interaction, one repeatable game loop, and a small enough content set to polish. Record budget assumptions in the brief and `game.json` before production. A zero budget in a fresh scaffold means no allocation has been made; fill it deliberately rather than treating it as an estimate.

For commercial scenarios show units, expected realized price, refunds, taxes collected by the store, store share, one-time charges, and incremental costs as explicit inputs. Separate launch income from lifetime income and a scenario from a forecast. Use current verified store terms when amounts matter. The owner understands Norwegian taxes; do not divert game planning into unsolicited tax explanations.

## Quality

The current owner direction, clarified in the 2026-09-07 Sixfold Recoil playtest, is substantially more ambitious games with challenge, learnable skill, tactical choices, interacting abilities/builds, exploration and meaningful progression. The comparison to "ten times the complexity" was explicitly illustrative, not a multiplier, feature quota, budget or deadline. Minimal one-attack arcade loops do not meet this portfolio target.

Prioritize responsive play, skill and meaningful decisions, readable feedback, rewarding progression, and coherent visual/audio presentation. Internal technical probes may be rough. An owner-facing gameplay slice must represent the proposed game's decision space and first progression/reward loop; removing those systems can invalidate the playtest. Include enough interacting options to test the intended experience before expanding the full campaign.

Build strong visual identities with coherent meshes, materials, lighting, animation, effects, UI and sound. The current bar is attractive 3D presentation in Unreal, using authored/modified models and suitable asset libraries as needed; simple Canvas/SVG-style game graphics do not meet that bar. Blender, procedural 3D and generated images are useful production tools when appropriate, with provenance and actual in-engine inspection. A generated concept image does not prove game-ready geometry or animation. Judge assets in motion at gameplay scale; effects must not hide the information a player needs.

For the current game direction, Klaus selected **first person and one modular weapon that also forms a shield, with freely chosen attack and defense**, on 2026-09-07. Extensive attachments should change abilities and tactics; loot, level/boss rewards and lasting achievement must support enjoyable play. The cyborg whose dreams reveal real reachable worlds is the working story foundation. Klaus selected visual direction **B: sculpted painterly 3D**; actual gameplay fidelity must be demonstrated. Exact lore, world count and cross-world transfer rules remain in refinement. Do not ask for viewport, combined/separate equipment or the selected art direction again. Other studio games may choose different views; no viewport proves depth or enjoyment. Check configured local tool paths before claiming an engine is unavailable. Unreal 5.8.2 was verified locally on 2026-09-07; its private path belongs in ignored `config.local.json`.

For the next game under discussion, Klaus also favors procedurally varied levels and runs, provided they remain playable and fun. Prefer designed combat spaces assembled and populated under explicit rules for connectivity, pacing, challenge and rewards. Variation must change decisions and support progression. Keep reproducible runs for testing and saving; automated validity checks cannot establish enjoyment. This preference does not select a replacement concept or make procedural generation mandatory for every future studio game.

Klaus explicitly welcomes **earned overpowered builds** in this game: a large combinable attachment/magic catalog, difficult acquisition and creative assembly should yield exceptional payoffs. Boss, elite and rare loot should introduce fresh abilities that are exciting to try. Support compatible reaction chains and substantial room-clearing/boss-burst power; do not silently cancel success through adaptive resistance or flatten every build. Protect against nonterminating effects and actual failures while distinguishing them from intended dominance. Permanent ownership and transfer rules must preserve the satisfaction of earning powerful equipment.

The weapon/shield is the central evolving object in the current concept. Explore technology, magic, crystals, jewelry, stackable upgrades and behavioral evolution as complementary ways to develop it. Catalogue sizes and rank examples are design proposals, not owner-mandated quotas; distinguish new abilities from numerical enhancements and levels of the same item.

Prefer manageable offline or single-player designs unless the owner requests otherwise or a validated concept requires more. Multiplayer services, live operations, user accounts, and recurring content obligations materially change cost and maintenance; include those costs in the choice rather than adding them casually.

## Collaboration

Use the fewest concurrent roles that improve the result. The producer maintains one coherent plan; specialists own bounded pieces. The owner should mostly see concrete choices, playable builds, and short reports. Continue routine authorized work without repeated confirmations. When an essential answer is missing, explain the actual dependency and keep independent work moving.

Scope and authorization come from the conversation. Checkpoints are evidence requirements, not automatic permission prompts. Selecting a new game, spending beyond the agreed budget, accepting store agreements, or making an unrequested public commitment may need owner input; existing explicit authorization remains valid.

Klaus explicitly authorized computer control on 2026-09-07 for Unreal work, beta testing and other necessary task work, citing the Breakout project's established practice. Use background automation where practical and native editor/game interaction where it provides useful evidence. Coordinate foreground input with the owner when necessary; do not ask for the same authorization again. Breakout's existing build and validation notes are a reference for the workflow, not a restriction on the next game's design or art.

## Learning and ownership

Keep source, build instructions, decisions, and asset provenance with each game. Keep opportunity history, including ideas that did not work. Reuse lessons and proven components when they help; avoid constructing a general engine before there are games that need it.

This is a public repository. Commit studio instructions, original source, and assets suitable for public distribution. Store private business records and credentials elsewhere. No repository-wide open-source license is chosen by this setup; record a deliberate license/distribution decision for each game and retain third-party notices where required.
