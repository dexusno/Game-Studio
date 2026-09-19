# Lockdown and the last command

**20 September numerical adoption:** [BETA-TUNING.md](BETA-TUNING.md) adopts this ten-tier numerical ladder as later test inputs, with every mercenary starting from 80 HP (tier 10: 64). This does not select the outstanding unlock/finale proposals, replace the first-MVP scope answer or claim any tier is beatable.

18 September 2026. **Post-campaign design proposal.** The owner requires escalating difficulty, Steam achievements and an ultimate challenge earned through all four mercenaries. The names, ten-tier ladder, numerical penalties, Tier 5 finale threshold and boss below are proposed implementations of that direction.

Read the [achievement roster](ACHIEVEMENTS.md), [Steam implementation plan](STEAM-ACHIEVEMENTS.md) and [dated research](research/2026-09-18-endgame-and-achievements.md). Structured values live in [lockdown-tiers.json](data/lockdown-tiers.json). Nothing here is a playable implementation or proven difficulty curve.

## Progress after the twelve cities

**Working entry-gate assumption:** first clear each mercenary's full three-city campaign in the same profile. This completes all twelve distinct cities and opens **Lockdown 1** for all four mercenaries. The owner has been asked whether entry should instead unlock immediately for each character's first campaign clear; until answered, this draft follows the all-four wording. The entry gate is one field, so that choice does not require redesigning the ladder.

After entry, progression is independent. Mara clearing Lockdown 2 opens Mara's Lockdown 3; it does not advance Ivo, Ada or Noor. Clear the entire three-city route at the highest available level to unlock the next. Repeating a lower level does not skip a level. Tier 10 is the maximum. Previously unlocked levels and the ordinary Tier 0 campaign remain selectable, and defeat never removes a clearance.

The original character access rule stays separate: a city clear with Mara unlocks Ivo, with Ivo unlocks Ada, and with Ada unlocks Noor. It still does not require a full campaign. Tiers belong to the chosen profile, not to Steam achievements earned across different profiles. A new profile starts with the ordinary progression even if the account already owns every achievement.

Each mercenary keeps their own three cities, route graph, enemy-pool gates and inherent abilities. A new run rerolls within those existing gates. Higher Lockdown does not put late-city robots into opening pools, delete the Regular alternative or secretly counter the selected build. The Mayor still offers three upgrades at each city entry. Recipes remain eligible from the first campaign under the existing character/source/stage rules; achievements do not unlock stronger recipes or passive account-wide damage.

## Ten cumulative tiers

Each row states the **total active settings**, not an additional percentage multiplied by earlier rows. A row introduces one further constraint. These are beta hypotheses that must be evaluated together with recipes, recovery and the upgraded economy.

| Tier | Name | Ordinary haul with the 10-unit fixture | Mercenary base maximum/start HP | Robot base HP | Added damage per attack hit |
| --- | --- | ---: | ---: | ---: | ---: |
| 0 | Open Contract | 10 | 100% | 100% | 0 |
| 1 | Supply Watch | 9 | 100% | 100% | 0 |
| 2 | Thin Margins | 9 | 95% | 100% | 0 |
| 3 | Hardened Frames | 9 | 95% | 105% | 0 |
| 4 | Blockade | 8 | 95% | 105% | 0 |
| 5 | Critical Reserve | 8 | 90% | 105% | 0 |
| 6 | Live Ammunition | 8 | 90% | 105% | 1 |
| 7 | Siege Rations | 7 | 90% | 105% | 1 |
| 8 | Last Reserve | 7 | 85% | 105% | 1 |
| 9 | Fortress Protocol | 7 | 85% | 110% | 1 |
| 10 | Absolute Lockdown | 7 | 80% | 110% | 1 |

**HP:** at new-campaign creation, use `max(1, floor(base_HP * tier_percent / 100))` as both starting and maximum HP. The proposed Mara fixture is 80, 76, 72, 68 or 64 HP across the relevant tiers. Other mercenaries use their eventual individual base values; 80 is not silently assigned to all four. Apply explicit acquired maximum-HP increases afterward, in full, including their stated healing. Do not reapply the reduction at each city or fight, reduce Bolt's HP, lower healing amounts or modify own-effect nonlethal rules.

**Supplies:** reduce the ordinary foundation before adding the existing two steered units. For the proposed 10-unit haul, the foundation is Iron 3 / Copper 2 / Carbon 1 / Glass 1 / Circuit 1. Successive reductions remove Iron, Copper, then Iron, leaving at least one of each material and preserving the steering choice. A revised base economy must supply a new explicit deduction table; never choose a random missing ingredient or produce a negative quantity.

The reduced foundation applies to each ordinary haul and an upgrade explicitly granting another ordinary haul. Precision uses that reduced ordinary foundation plus its normal success bonus. Upgrade/recipe grants, extra steered bonuses and named replacement hauls retain their full printed effects. The tier is a supply default, not permission to invalidate Supply/Precision upgrades. No global shot cap, crafting-action budget or part-storage cap is added. Costs and recipe-copy cooldowns are unchanged.

**Robot pressure:** spawn maximum HP is `ceil(base_HP * tier_percent / 100)` using integer arithmetic. Relative phase thresholds use the resulting maximum; absolute thresholds in existing entries scale by the same factor once. At Tier 6+, add 1 to each positive direct attack-hit component before existing Drive Gain, Weaken and player defence. A non-damaging action remains non-damaging. Recoil, damage-over-time and HP-loss effects do not receive that attack bonus. Apply the settings once to initial robots and summons, never again during a phase change. A Tier 9 total of 110% replaces 105%; it is not 115% or 115.5%.

Increased durability/damage supplies two secondary pressure steps. Resources and HP remain the main restrictions. Rare recipes, Legendary upgrades, special character effects and earned powerful combinations keep their stated functions. A tier is not guaranteed harder in every individual seeded matchup; increased aggregate challenge is what must be demonstrated.

## The reveal at Lockdown 5

Proposed threshold: **clear Lockdown 5 with all four mercenaries in one profile**. This requires at least 24 successful three-city campaigns: four ordinary clears plus five tier clears for each mercenary. With the current 12-encounter city fixture, that is 864 completed encounters before losses. This is a substantial commitment, not a short tutorial objective. Track completion time and abandonment in beta; move the threshold lower if the reveal arrives too late. Tiers 6–10 remain optional mastery progression beyond that narrative threshold.

Each mercenary's Tier 5 victory captures one distinct command signature. These are permanent profile evidence, not inventory parts or a new currency. On the fourth signature, reveal that the gloating AI heard at Game Over is a single system coordinating the city invasions: **CROWN-0, the First Command**. Names and exact dialogue remain proposals. Show the four signatures and the newly opened **Command Core** challenge; give a visible long-term four-slot objective earlier without revealing the boss's identity.

The completing run can enter the Core immediately after its City 3 victory. Subsequent victories at Tier 5 or above can enter it with the current mercenary and build. The encounter is an **optional epilogue**, not a fourth city: no extra Mayor, additional route tree or fresh campaign start. The owner-selected three-city route still ends at City 3.

Commit City 3/route clearance and the next-tier unlock before offering `Finish campaign` or `Enter Command Core`. Entering retains current HP, Credits, recipes and upgrades; ordinary fight-end stock clearing, explicit cargo exceptions and the ordinary shop restock apply. Permit the usual shop preparation once before the Core. Do not heal automatically or hand out a fourth Mayor gift. Defeating the Core awards **The Last Order**, the special hidden Steam achievement, a unique profile crest and the final cinematic. These rewards grant no combat power.

Losing at the Core ends that campaign. The already-earned route/tier clearance and profile access stay earned; report `Route cleared / Command Core defeated you` so this is not mistaken for a lost unlock. Quitting an unfinished Core fight restarts that fight from its original entry state with the same seed. Declining the epilogue keeps access for a later qualifying run, but it does not bank or resurrect the discarded build. Simultaneous mercenary/Core death is defeat. The completion achievement requires a living mercenary and all hostile bodies defeated.

## CROWN-0 encounter sketch

This uses existing combat vocabulary and is a **costing fixture**, not final balance. At the Tier 5 threshold, its inherited difficulty modifiers are already active. Three bodies fit the existing formation scale.

| Body | Base HP before tier | Effect / action schedule |
| --- | ---: | --- |
| CROWN-0 | 220 | Armor 2. Command mode repeats Attack 14, Charge without attacking, Attack 24. At or below half of its scaled maximum HP while alive, enter Emergency mode once. Replace its currently unperformed action with a visible Recover action; afterward repeat Attack 5 × 4, Charge, Attack 28. |
| Bulwark Relay | 30 | Reinforcement Link 2 protects CROWN-0 while this relay lives. Alternate Attack 6 and a non-attacking Brace action that gives the relay Armor 2 until its next action. |
| Overclock Relay | 30 | Alternate Attack 4 × 2 and grant CROWN-0 1 Drive Gain. No grant if CROWN-0 has died. |

At Tier 5 these bodies begin at 231 / 32 / 32 HP. If the threshold is crossed after CROWN-0 has acted, Recover replaces its next action. Multiple hits can cross and pass the threshold normally. Killing it never forces recovery, a second life, a damage cap or an invulnerable phase. Surviving relays must still be defeated and continue their defined actions. Relay death immediately removes its live link, but does not undo Drive Gain already granted.

Evaluate the half-HP test as `2 * current_HP <= maximum_HP`, without storing fractional HP. The three printed robot values are pre-tier data; spawn scaling produces the actual values once.

This creates an intended choice: destroy protection, stop future attack growth, or burst the commander through its recovery threshold. Ivo's damage-over-time, Ada's Helper damage, Mara's burst and Noor's stored Charge all retain useful routes. All enemy intents, HP thresholds, Armor sources and accumulated Drive Gain are visible; no hidden response invalidates an earned combination. Exact solvability with depleted post-city HP/supplies remains to test.

## Why play again

The short horizon is the next city and a different set of gated robot formations. The medium horizon is a higher clearance for a chosen mercenary, different recipes/Mayor engines, and bounded combat challenges. The long horizon is four signatures, the Command Core and optional Tier 10 mastery. Collection offers record discovery even when skipped, keeping exploration useful without requiring every item to be purchased. No achievement requires online multiplayer, a daily appointment or a speedrun.

Proposed profile screen: four mercenary columns, each showing three city badges, highest unlocked/cleared Lockdown, signature state and Core victory. The new-game screen explains every active tier modifier before starting. Difficulty locks for the campaign and persists with its rules version and seed; changing it requires a new campaign under the existing replacement confirmation.

## Implementation and balance sequence

1. Integrate profile city/route flags, per-mercenary tier records and versioned tier data with the ordinary save flow. Test lower-tier replays, failure, same-seed restart and independent profiles.
2. Apply the modifiers to the actual haul, HP and enemy creation paths. Preview final values and list upgrade exceptions. Validate all four starter kits at 10, 9, 8 and 7 materials; unaffordable attack-and-Shield combinations are a balance finding, not evidence that the whole game is impossible.
3. Add the platform-neutral achievement evaluator and Steam adapter described in the companion plan. The static JSON conditions are a specification, not that evaluator.
4. Test Tier 0, 1, 5 and 10 representative routes before filling intermediate playtests. Record deaths by cause, zero-use turns, HP loss, resource use, healing access, playtime and upgrade choices. Do not claim success from static recipe affordability alone.
5. Build the Core transition and encounter after late-city builds are costed. Verify phase crossing, commander-first and relays-first victory, damage-over-time kills, reaction death and retry persistence. The reward should feel earned without requiring one specific rare upgrade.

No new game source, Steam backend configuration, achievement artwork or endgame build is delivered by this design increment. Those are concrete planned implementation stages; the baseline combat demo remains the next gameplay dependency.

## Static evidence from this pass

The [reference calculation](analysis/endgame_progression.py) validates 40 distinct achievement names/API IDs, the 39-item meta set without self-reference, all 606 recipe and 288 upgrade collection IDs, cumulative tier settings, independent advancement, lower-tier replay, no tier skipping, the Tier 10 cap and the all-four finale gate. The [saved report](analysis/endgame_progression.json) also probes all four 12-recipe starter kits with each of the five steering choices at 10, 9, 8 and 7 materials.

Every seven-material fixture still has at least one cost-affordable subset containing both Ammo and Shield recipes. The number of distinct qualifying subsets ranges from 5 to 59 across characters/steering at seven materials, compared with 106 to 526 at ten. This is a combinatorial sensitivity result, not a win rate or a measure of useful damage/protection. It ignores effect grants, extra effect costs, reuse, upgrades, targeting, enemy pressure and healing. The narrowing makes Tier 7+ a priority for actual playtests; it does not establish that all four kits remain equally enjoyable or viable.
