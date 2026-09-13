# Scrap combat — lessons from playing Slay the Spire 2

13 September 2026. Native play by root; separate primary-source research by the game-design agent. This is a reference study and a proposal for Klaus's review. No new combat system, story, stage count or reset model has been approved or implemented.

## Recommendation

Use the strategic structure: read enemy intentions, spend a scarce shared budget, combine available tools, accept consequences, then improve the build through choices. Make the physical scrap pile and hanging magnet the distinctive way of making those decisions. Implement our own systems around that interaction; changing names would not establish a balanced adaptation.

The next design goal should settle one complete mid-game round alongside the route's rewards, stage victories and loss rules. These foundations belong together, before art production or a large recipe catalogue. Keep the existing owner-reviewed [redesign plan](REDESIGN-PLAN.md), with this proposed adjustment to its early design work.

## What was actually played

Installed Slay the Spire 2, UI version **v0.107.1, dated 2026.06.18**, Ironclad tutorial started by Klaus. Root played through native Windows UI and inspected screenshots after actions. This is evidence for that installed build, not a claim about the latest release.

Completed **two fights over eight player turns**, selected two card rewards, and completed the Byrdonis Nest event. Used Save and Quit; the main menu then showed Continue. Run left on floor 3 at **87/87 health, 128 gold, 12 cards**, including Setup Strike and Breakthrough. The existing Magnet Sweep prototype was not launched or modified.

| Fight / turn | Choice made | Observed consequence |
| --- | --- | --- |
| Nibbit / 1 | Two Defends and one Strike against an announced 12-damage attack | 10 block, 6 damage dealt, 2 health lost. Three energy forced an attack/defence split. |
| Nibbit / 2 | Bash, then Strike | Bash dealt 8 and applied Vulnerable; Strike dealt 9. Spending all energy on offence left the incoming 6 damage unblocked. |
| Nibbit / 3 | Two Strikes while the enemy prepared a buff | Removed its block and reduced it to 6 health. Ended with one unused energy because remaining defence would serve no purpose that turn. |
| Nibbit / 4 | Strike before its announced 14-damage attack | Killed it before it could attack. Burning Blood healed 6 after victory, leaving 78/80 health. |
| Three slimes / 1 | Two Strikes killed the 8-health attacker; Defend covered the smaller remaining attack | Prevented one attack entirely and took no health damage. The large slime added unwanted cards. |
| Three slimes / 2 | Setup Strike, then Strike, killed the 15-health slime; final energy paid for Defend | 7 + 8 damage killed it before its announced card addition. Block reduced the large slime's 8 damage to 3. |
| Three slimes / 3 | Bash and Strike against the large slime while it added cards | Dealt 17, leaving 18 health. A Slimed card occupied a hand slot. |
| Three slimes / 4 | Setup Strike then Strike against the vulnerable remaining slime | Dealt 10 + 12, killing it before its 8-damage attack. Post-combat healing restored health to 80/80. |

First reward: 17 gold and **Setup Strike**, chosen over Tremble and Blood Wall. Second reward: 12 gold and **Breakthrough**, chosen over Inflame and Anger because the group fight exposed a need for attacks against multiple targets. Breakthrough was acquired but not played. Reward screens also offered Skip.

At Byrdonis Nest, chose Eat the Egg over taking an egg card. The displayed reward was 7 maximum health; actual health became 87/87. The unchosen egg's detailed behavior was not inspected.

Tutorials explicitly introduced energy, block expiry, enemy intentions, discard reshuffling and combat-long Power effects. Unwanted Slimed cards were drawn in both final slime turns; their complete rules were not inspected. No audio assessment was performed. Native input initially required correcting the automation's card-selection method; that is not evidence of a game usability defect.

**Limits:** no boss, elite, death, full run, act transition, shop, rest stop or potion use was tested. The opening fights were forgiving: post-combat healing erased the remaining damage by the second victory. This sample explains several decisions; it does not establish the complete game's balance or prove our concept fun.

## Why the rewards and combat decisions worked

The clearest reward payoff came immediately. Setup Strike dealt 7 and strengthened the next Strike to 8. Together they killed a 15-health enemy with two energy, leaving one for defence. Two ordinary Strikes would deal 12; reversing the chosen sequence would deal 13. A third ordinary Strike could kill, but would use the energy needed for defence. The reward changed an achievable outcome, and order mattered.

Enemy intentions made prevention understandable. Killing an attacker removed an announced attack. Killing the small slime on turn two prevented its announced addition of an unwanted card. Defending while the remaining enemy only buffed or added cards would waste this turn's limited resources.

Limited availability also mattered: the hand changed each turn, and unwanted cards reduced useful options. Health connected the fights even though this character's opening recovery was generous. Rewards and the event introduced decisions between fights rather than simply increasing a score.

## Proposed translation to the magnet game

| Strategic role | Proposed scrap-game equivalent | What must stay clear |
| --- | --- | --- |
| Available hand | Reachable scrap in a persistent physical pile, plus a small staging tray if needed | What is reachable now and what a pull will disturb. Do not reset the entire pile every turn and erase the player's physical work. |
| Shared energy | A small number of powered loads per round | An offensive load competes with a defensive load. Unlimited free collecting cannot bypass the budget. |
| Card effect | Forge output determined by captured materials and selected attack or shield mode | Preview damage, protection, target and material consumption before committing. |
| Build identity | Rig modules, a compact recipe set and changes to future scrap supply | A new capability must have usable supporting materials; avoid rewards that cannot function. |
| Enemy intent | Visible weapon preparation plus a short, concrete next-action indicator | Show which enemy will do what and the expected damage after current protection. |
| Run attrition | Rig damage persists between encounters; repair opportunities compete with growth | State exactly what zero health, defeat and restarting mean. |

**Working preference:** make one powered load a complete meaningful action, from committing a pull to delivering its forged result. Positioning can be considered freely; an active pull must consume a limited opportunity, and material capacity must constrain each load. Prototype the budget with only a few loads per round before deciding its number. Charging separately for every minor transport motion risks turning the magnet into a tax on combat.

Keep captured composition visible. Let the player choose the intended output; the smart weapon can explain the mixture without automatically choosing the best tactic. Incorrect loading should have readable consequences and a defined recovery cost. Physics should produce satisfying movement and understandable tradeoffs, without unpredictable swings arbitrarily destroying a careful turn.

A small material vocabulary is enough to test the idea: a basic attack/armour material plus one special component that enables a clearly different tactic. Shock, piercing, explosives and damage over time remain expansion candidates. Do not simultaneously model range, speed, weight, size and every status unless each creates a decision in this fixed side-view battle. Magnetic scrap must also look collectable; nonmagnetic components need a visible steel casing or another understandable collection rule.

## Mid-goals, completion and replay — proposed structure

Use three broad stages as a discussion model, not a locked level count. Each stage needs a visible local objective and a guardian encounter that resolves it: reach a defended route, break through the main blockade, then win the final mission confrontation. The actual locations and stakes follow the selected premise.

| Scale | Player goal | Reward / progress |
| --- | --- | --- |
| Within a fight | Remove a dangerous enemy or disable a recognisable threat | Immediate reduction in danger; possibly accessible salvage if the future rules support it. |
| After a fight | Win with enough health to continue | Choose a useful recipe, module or supply improvement, with Skip available. Show exactly what becomes possible. |
| Between fights | Prepare for the next known threat | Route choices and workshop decisions, including repair versus improving the rig. |
| Stage complete | Defeat the guardian and accomplish the local objective | Visible world change and a substantial new capability for the next stage. Avoid rewarding only larger numbers. |
| Final confrontation | Accomplish the world objective while keeping the rig alive | A proper ending, winning-build record and optional new starting approaches. |
| Another run | Solve different encounters with a different build and route | Changed starting kit, materials, modules and encounter combinations. Permanent power grinding is not required. |

For example, an upgrade that lets one shot hit several enemies addresses a new kind of threat. A module that converts surplus protection into a later attack creates a different turn sequence. These are illustrative proposals; their costs and interactions need testing. A wider magnet field only earns its place if it enables a useful collection pattern or previously unavailable load.

Proposed real loss: zero rig health ends the run, including its temporary equipment and route progress. Saving resumes the current state; routine restart should not erase consequences. What discoveries persist and whether the campaign has another structure are owner choices. The story can end decisively while the combat system remains replayable through different builds and routes.

## Gameplay critique and next useful test

The main risk is a card battler with slow material handling inserted into every turn. Every pull must contribute a tactical choice: which mixture to pursue, what to expose for later, or how much defence to sacrifice for a kill. If the best move is always to sweep up the maximum mass, the design has failed its central test.

Other rejection conditions: the weapon's result is hard to predict; a useful-looking reward lacks supporting scrap; unavoidable material starvation decides fights; bosses invalidate an entire build without warning; enemy intentions require a paragraph to interpret; upgrades only shorten chores. These are gameplay concerns, independent of development time.

**Proposed next owner goal:** define one complete mid-game round with exact enemy intentions, visible pile contents, controls, load budget/capacity, forge outputs, consumption/replenishment, two worthwhile choices, and the resulting enemy response. Also specify what ends a stage, how a run is lost and what survives a restart. Use concrete numbers to expose contradictions, clearly labelled as tuning candidates.

Then, when authorized, the concept demo should connect an introductory fight, a meaningful reward, a contrasting fight where that reward can matter, and a small boss. Include persistent damage and an actual defeat/restart path. Owner playtests must establish that gathering, forging and choosing defence are enjoyable before content expansion. This study does not authorize that implementation.

## Primary-source context

Sources accessed 13 September 2026. These supplement the native observations above; announced or later-game features were not personally played.

- Mega Crit describes Ancients at act entry, including substantial benefits and tradeoffs and their relationship to the first game's boss-relic choices. This supports planning major build decisions at stage transitions without assuming the two games have identical reward placement. [Neowsletter 16, 13 November 2025](https://www.megacrit.com/news/2025-11-13-neowsletter-issue-16/).
- Mega Crit's alternate-act announcement describes different environments, enemies, events and bosses. It supports variation as a replay design reference; an announcement alone does not verify availability in the installed build. [Neowsletter 14, 11 September 2025](https://www.megacrit.com/news/2025-9-11-neowsletter-issue-14/).
- The official store description presents deck building, relics and potions and describes Early Access plans, including a true ending. Do not treat unplayed or planned endgame content as observed balance evidence. [Official Steam page](https://store.steampowered.com/app/2868840/Slay_the_Spire_2/).
- Anthony Giovannetti's GDC presentation on the first game discusses giving cards a place and using data alongside interpretation and iteration. Our inference: transfer testable principles, then balance the physical game's different costs through play. [GDC 2019 presentation](https://media.gdcvault.com/gdc2019/presentations/Giovannetti_Anthony_SlayTheSpire.pdf).
