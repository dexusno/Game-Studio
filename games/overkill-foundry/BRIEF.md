# Overkill Foundry — independent game brief

Owner-selected title: **Overkill Foundry**, 14 September 2026. Catalogue ID: `overkill-foundry`. Owner-separated from Magnet Sweep on 14 September 2026; fresh gameplay implementation, currently concept stage. Planned platform/tool direction: Windows and Unreal. Development timeline is unconstrained; implementation scope and numerical budgets are not approved.

## Player promise

Build a devastating shot and enough defence from the scrap you have, read the robots' intentions, and turn victories into a stronger mercenary weapon system. Travel between fortified cities in a dystopian world taken over by a hostile AI. Each city has distinct surroundings and a wave of 10–20 fights, ending in a boss.

## Selected loop

Choose a save profile and mercenary. Each mercenary starts with a basic weapon and core recipes. At city arrival choose from three weapon upgrades. Between fights choose among three semi-random encounters with at least one Regular; Officer and Mystery each appear about one quarter of the time among offers. Their exact joint distribution remains open.

Inspect enemies' intent, strength and buffs, then OK. Choose baseline ingredients and Collect with the rear claw, or spend the one optional Precision attempt available this fight. A very short collection/furnace animation delivers resources. In the loadout, open full-screen Recipe, focus a recipe and Use to craft with available materials; greyed recipes remain inspectable. Shop is also accessible during preparation. Parts and raw materials are separate pools.

Select parts for bullet and shield. Load requires a nonempty bullet, then select permitted targets and Fire. Brief firing/impact feedback communicates the build's power; surviving enemies follow their intents. End Turn / Defend allows a no-shot round. Shield absorbs ordinary damage before HP. Clear round Shield/builds and reduce all active cooldowns by one at enemy-turn end, including newly added counters. Repeat until enemies are killed/fled or the player dies. Death shows Game Over with a gloating evil AI.

## Build and persistence

Recipe memory has a hard capacity, amount still open; all stored recipes are visible without a deck/draw system. Numeric cooldowns start at crafting; global cooling removes X from every cooling recipe. At zero counters, an affordable recipe can be crafted again immediately. No-cooldown recipes are once per turn unless a permanent upgrade says otherwise. Unused materials and parts carry between rounds but both clear at fight end. Recipes/upgrades persist for the current game. Shield resets after the complete enemy turn by default, unless a permanent upgrade explicitly says otherwise. Permanent upgrades may grant beginning Shield each turn, at fight start or on a specified turn; fresh grants do not automatically preserve old Shield. Recipe drafts do not supply reset exceptions. Upgrades may also grant fresh supplies on their defined triggers.

Accept desired loot items individually and Skip the remainder. Recipe rewards allow zero or one of three; at full memory, exchange a held recipe or skip without changing it. Cores sell for credits. Permanent claw upgrades come from shops or Officer loot and activate on purchase/acceptance. The 23 temporary next-round gathering recipes remain alongside them. Neither implicitly grants another Precision attempt.

## Content and presentation

The catalogue has 606 drafts: 126 shared and 120 exclusive to each of four proposed mercenaries, for 246 possible recipes per character. Character identities, starter sizes, recipe numbers and detailed status rules remain proposals where the owner has not selected them.

Use the owner-supplied [style anchor](design/art%20anchor.png). Prepare in 16B's 2.5D side view with claw behind the gun and enemy intent ahead; use 16C's 3D perspective for action. Each bullet and shield interface must support at least 20 applied parts. The hard cap and physical-parts versus combined-effects presentation remain undecided. Illustrations and the old camera motion study are references only.

## Next proof and limits

Resolve Load/undo, secondary-effect activation, supply amounts, enemy moves, shop stock/prices, fleeing rewards and profile/retry consequences. Reconcile conflicting Shield recipes with the settled upgrade-only exceptions; do not reopen the base rule. Global cooling creates balance overlaps that need review. A proposed first demo is one mercenary, one city environment, about 30 recipes and a three-fight sample; this has not been selected and does not replace the full city wave.

The first playable should test whether different enemy intentions and supplies cause meaningful changes in crafting, saving, attack and defence, and whether an earned reward changes a later choice. Game feel and commercial appeal are untested. Naming research will screen current market relevance and public conflicts without treating an absence of search hits as legal clearance.

Details: [design plan](design/REDESIGN-PLAN.md), [recipe catalogue](design/RECIPE-CATALOGUE.md), [camera flow](design/COMBAT-CAMERA-FLOW.md), [decisions](DECISIONS.md).
