# Encounter proposal — drones and a siege robot

**Encounter route approved as a worked design example, 16 September.** Klaus says the route looks good and renames the damage-reduction buff to **Armor** to give the game its own terminology. The example uses two simple attackers beside one armoured threat to connect multiple shots, damage supplied by parts and saving unused parts. The enemy roster is still open: Scrap drone and Siege robot are working labels for this example, not finalized roster entries. Numerical values remain beta placeholders, with no playable implementation or tested balance.

| Enemy | Illustrative HP | Proposed behaviour and visible information |
| --- | --- | --- |
| Scrap drone A | 6 | Attacks for 5 each enemy turn while alive. Show Attack 5. |
| Scrap drone B | 7 | Same simple attack pattern: Attack 5. Different example HP do not select a random-HP system. |
| Siege robot | 24 | Alternates Charge and Blast 18, starting with Charge. Charge is its whole action and deals no damage; show that it prepares Blast 18 for the next enemy turn. During the next planning phase show Attack 18. After blasting, return to Charge. |

**Siege-robot buff — Armor 3 (owner-selected name):** reduce each incoming damage hit by 3, to a minimum of zero. Show the buff and the expected damage after its reduction before Fire. This is an enemy-specific flat damage reduction, separate from Shield; it is not a new rule for all robots, a player Shield-retention exception or a stacking rule for multiple reduction buffs. The fixture has no other enemy protection, status, recoil or escape action. It does not redefine those systems. These fixed patterns are proposed for this encounter; they do not require every future robot to use fixed cycles.

## Resources and shared recipes used in the example

Use the existing **proposed** ten-material haul with Glass steering: 3 Iron, 2 Copper, 1 Carbon, 3 Glass and 1 Circuit, once per round. Begin with no stored parts/materials and use no Precision bonus, shopping, permanent-upgrade grant or character-specific effect. This controlled fixture does not approve the haul or replace the full starter set.

Use SH001 Solid Casting (6 damage), SH003 Powder Packing (3 damage), SH004 Simple Sighting (4 damage, or 8 against an attacking target), SH002 Flat Plate (6 Shield) and SH006 Basic Insulation (4 Shield). Each is available once per turn under the selected no-cooldown rule. Crafting all five once costs 3 Iron + 2 Copper + 1 Carbon + 2 Glass = 8 materials. Recipe rows and values remain unchanged.

## One complete possible route

1. **Round 1 — split the offence and prepare defence.** Both drones intend Attack 5; the siege robot intends Charge. Craft those five recipes once. Fire the Plain Slug alone for 6 to destroy drone A. Fire the Aim Fin alone for 8 against attacking drone B to destroy it. Keep the Powder Cap and both Shield parts in reserve, without staging the Shield parts. End Turn activates no prepared shield; the only surviving robot charges, so no enemy attack reaches the player. The three saved parts remain available next round. This uses two shots with damage supplied entirely by their parts.
2. **Round 2 — combine the shot and spend the saved defence.** The siege robot now intends Attack 18. Take the next haul and craft the same five recipes once. Load the new Plain Slug (6), new Aim Fin (8 against this attacking target), new Powder Cap (3) and saved Powder Cap (3): one 20-damage shot. Armor removes 3, so 17 damage reaches HP, leaving the siege robot at 7. Stage both saved Shield parts and both new ones. Only End Turn activates their 20 Shield; it absorbs the 18-damage blast. The remaining 2 Shield resets after the enemy turn. Using two Powder Caps is legal because one was crafted and saved last round, not because a no-cooldown recipe was used twice this turn.
3. **Round 3 — finish during its charge.** The robot intends Charge again. Take the normal haul. Craft a new Plain Slug and Aim Fin for 2 Iron + 1 Glass. The Aim Fin contributes only 4 against this non-attacking target. Their combined 10 loses 3 to Armor, dealing the remaining 7 HP and ending the fight before another enemy action. Apply the existing fight-end/loot rules; this proposal does not set reward quantities.

The route loses no HP under these stated conditions. That is an arithmetic outcome for a deliberately readable introductory example, not a difficulty target or evidence of gameplay quality. A continuing campaign's actual incoming HP remains governed by the selected carryover rule.

## Why these behaviours matter

- **Several small shots:** remove two separate attackers in round 1 instead of spending a combined single-target bullet on one drone. Other available multi-target recipes can provide additional answers; the encounter does not forbid them.
- **One large shot:** in round 2 the same parts fired separately for 6, 8, 3 and 3 would deal only 3 + 5 + 0 + 0 = 8 after Armor. Combining them deals 17. This rewards assembly without adding a firing cost or shot cap.
- **Defence and preparation:** bank unactivated Shield parts during the charge. Round 2's newly crafted 10 Shield alone would leave 8 blast damage reaching HP; adding the saved 10 prevents that loss. Active Shield still resets normally. The next attack must be communicated so saving parts is an informed choice.

**Paper verification:** checked printed costs against the current recipe rows, one use per selected recipe per round, affordable material spending of 8/8/3 with the stated hauls, both first-round kills, combined versus split damage, saved-part accounting and final HP/Shield arithmetic. No runtime, automated combat simulation or playtest was used; the rest of the 606-row catalogue is unchanged.

**Future play question:** after seeing the displayed intents and Armor explanation, can a player explain why they split shots, combine parts or hold Shield parts, without following a prescribed sequence? If they cannot predict these consequences, revise the feedback. If the same sequence dominates different enemy patterns and available builds, revise the encounter combinations and tuning. This question does not authorize a build.

**Next discussion:** establish the enemy roster through concrete roles, identities and behaviours for Regular enemies, Officers and city bosses. The approved example route and Armor name are recorded; the example does not establish a complete roster or lock numerical balance. No additional recipe-name replacements were specified in this feedback, so the existing recipe names remain unchanged.
