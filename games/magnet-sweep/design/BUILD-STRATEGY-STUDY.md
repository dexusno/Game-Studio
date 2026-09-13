# Magnet Sweep — build strategies and stacked ammunition

Prepared for Klaus, 13 September 2026. Research and design proposal. No new game systems have been implemented or playtested. Timeline is not a design constraint.

Follow-up requested by Klaus: [developer documentation on balance and a proposed test method](BALANCE-RESEARCH.md), covering energy, available scrap, enemy pressure, rewards and the limits of metrics.

## Direction from Klaus

Use the hanging magnet to gather physical components and stack them into ammunition. Each component adds a property or interacts with another function. Firing spends the assembled pieces. Klaus proposes retaining unused material between turns within a fight, then refreshing it for the next fight. Shots may hit one enemy or several, pierce armour or apply poison and other effects. Scrap also builds protection. Energy and material availability must force meaningful attack/defence choices. Animate components loading and contributing to the shot, building anticipation before firing and showing the consequences clearly.

The desired reference is the strategy and progression of Slay the Spire 2: different semi-random early rewards suggest different builds, which grow more capable through subsequent choices. Cards are not requested for this game. Exact assembly rules, numbers, material supply, reset rules and reward distribution remain to be selected.

## What the reference study actually covered

Read all 595 unique base-card descriptions in the owner's [Cards List](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Cards_List), accessed through its rendered browser page on 2026-09-13. The page footer recorded its latest edit as 2026-08-24 01:48. Separate collection reads avoided relying on a truncated whole-page result.

| Collection | Base entries read | Strategic themes inspected |
| --- | ---: | --- |
| Ironclad | 90 | Armour as offence, sacrifice, self-harm, vulnerability, repeated hits |
| Silent | 91 | Poison, generated small attacks, productive discard, retention, burst setup |
| Regent | 91 | Building one weapon, separate resources, generated pieces, threshold payoffs |
| Necrobinder | 91 | A defensive/offensive companion, delayed execution, generated resources, sacrifice |
| Defect | 91 | Persistent machinery, stored effects versus discharge, waste conversion, repeat actions |
| Shared and special | 141 | Bridges between builds, delayed rewards, selection, costs and drawbacks |
| **Total** | **595** | **All base descriptions in the inspected database** |

Coverage cross-check: 595 unique names and URLs, no empty descriptions; 197 attacks, 247 skills, 113 powers, 16 statuses, 18 curses and four quests. The count includes 37 multiplayer entries and generated/special cards; it is not 595 normal reward choices. Multiplayer is not a proposed feature. Re-read Regent's star-bearing entries with image-based resource symbols preserved so their costs/effects were not silently omitted.

Before using the database, native library inspection read 44 Ironclad cards and encountered one hidden entry. Installed v0.107.1 (2026.06.18) showed 87 Ironclad entries with multiplayer enabled. Numerical differences include installed Setup Strike giving two temporary Strength versus three in the database, and installed Colossus giving five Block versus four. Several linked mechanic pages carry beta warnings or differ from the main list. Treat this community wiki as dated descriptive evidence, not a numerical specification for our game.

The earlier native sample covered two opening fights over eight turns, two rewards and an event; see [the play study](SCRAP-COMBAT-DIRECTION.md). This card review does not add combat playtest coverage. Upgraded versions were not exhaustively reviewed; bosses, all combinations, win rates and full-run balance were not tested. The database and keyword pages explain functions, but cannot establish how enjoyable our translation will be.

## The most useful design lesson

A strong build makes several ordinary actions work together. It needs an accessible starting benefit, a way to improve that benefit, a meaningful cost and opportunities to combine it with another strategy. A rare part should change a decision the player makes, not just colour the same attack differently.

| Reference examples, observed in the list | Useful principle | Possible physical translation — our proposal |
| --- | --- | --- |
| Body Slam, Barricade, Juggernaut | Building protection can become a route to damage | A fitted converter stores part of a defensive action's energy for a later shot |
| Feel No Pain, Dark Embrace, Fiend Fire | Consuming a resource can trigger several complementary benefits | Melting a component powers a system or produces useful residue, with finite recovery |
| Blade Dance, Accuracy, Finisher, Envenom | Many small hits can reward a different build from one large hit | Light projectiles repeatedly trigger a fitted applicator; a heavy stack instead breaks protection |
| Poison effects with Accelerant, Outbreak and Mirage | Delayed damage creates a reason to invest in survival or accelerate the payoff | Corrosive payloads plus scrap armour buy time, or a later part spends the buildup for a burst |
| Forge, Seeking Edge, Parry and Sword Sage | One core weapon gains genuinely different capabilities | A weapon can acquire spread, defensive support or repeated discharge through distinct modules |
| Osty, Unleash, Bone Shards and Sacrifice | A useful defensive resource can also be spent offensively | A protective assembly can be deliberately consumed for a powerful shot, leaving the rig exposed |
| Orbs and Evoke; Gunk Up, Compact and Flak Cannon | Installed systems provide ongoing value; stored effects and waste can be spent | Keep a charged component installed for ongoing protection, or discharge it; convert selected offcuts into usable feedstock |

The [Sovereign Blade](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Sovereign_Blade) is especially relevant as a reference for developing a weapon's capabilities. Its exact Forge mechanic need not become our ammunition rules. Likewise, [Doom](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Doom) is a delayed execution threshold, not poison; these should not be confused when borrowing strategic ideas. [Discard and Sly](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Discard#Sly), [Osty and Summon](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Osty#Summon), [orbs](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Orbs) and the [keyword definitions](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Keywords#Ethereal) were checked to distinguish resource movement, consumption, persistence and timing.

## Proposed ammunition and energy rules

The side-view assembly should be understandable as a physical object. A recognisable projectile body establishes the shot; added pieces supply a payload or modify its delivery. Example: a pointed penetrator, a corrosive container behind it and a pressure unit combine into an armour-piercing corrosive shot. Visible shapes and connections must explain why the combination works. A familiar instruction could be “Add the pointed tip to pierce armour.”

Keep the owner's linear stack. Give position a rule only where it creates an understandable consequence: a front tip penetrates first, then the payload is released. Repeated pieces may strengthen a property or reach a clearly shown threshold. Do not turn arbitrary ordering into hidden recipes. Ordinary combinations must remain useful without memorising an encyclopaedia.

**Separate three decisions:** what to retrieve from the pile, what to assemble from available pieces, and what to spend energy activating this turn. Provisional recommendation: charge energy for each committed powered haul and for activating the weapon or armour press; adding a component may visibly increase that assembly's activation cost. Moving the magnet or rearranging an uncommitted assembly should not repeatedly charge energy. The exact split needs a representative encounter test: if gathering consumes so much that firing and defending become impossible, the economy has failed.

Gathering cannot be an unlimited preparation phase. A finite powered-haul budget, visible load capacity and limited replenishment must make pile position and retention relevant. A further haul can expose buried useful parts but leave less power to fire or protect the rig. The result preview includes that opportunity cost before committing. The hanging magnet should still visibly pull, lift and swing physical loads; a menu that silently gives the requested material would lose the game's identity.

Armour and ammunition draw from the same stock and energy. A steel component might contribute to a projectile or be pressed into protection. Some specialised parts can support only one use, but the starter stock must contain a viable basic attack and defence. Protection should be visually mounted on the rig, where enemy hits break or consume it. A proposed simple rule is that ordinary temporary armour lasts through the enemy response and then clears; particular modules may retain some of it. Do not silently imply this is permanent equipment.

Energy refreshes each player turn in the initial proposal. Unused components remain in the physical reserve during the encounter, subject to a readable capacity. Allowing unfinished assemblies to remain is a further option to test. Spent components leave that reserve. Replenishment should be predictable enough to plan around; random stock should create adaptation rather than an unavoidable inability to defend. Clearing enemies must end the fight promptly, without encouraging infinite resource farming before the last kill.

## An illustrative turn, not tuned numbers

Suppose two enemies show attacks of eight and four. The player has five energy after the normal refresh and these parts retained from an earlier turn: two steel pieces, a splitter and a corrosive canister. A new haul would cost one energy. These numbers only demonstrate competing choices; they are not selected balance values.

| Choice from that same starting state | Spend | Visible result and sacrifice |
| --- | --- | --- |
| Build one large shot using all four pieces | Five energy, all four pieces | Its preview kills the eight-damage attacker, but the second enemy deals four damage and no material remains |
| Press one steel piece into armour; fire the other with the splitter | Two energy for eight armour, three for the small spread shot | Neither enemy dies in this example; armour absorbs eight of their combined twelve damage. Keep the corrosive canister for a later shot |
| Press one steel piece into armour; use a powered haul | Two energy for eight armour, one for the haul | Accept four damage to gain useful pieces, retaining the remaining energy for a cheap action if one is available |

These routes trade enemy removal, health and future stock. Different enemy intentions or an acquired module should change which route is strongest. They must not collapse into the same calculation on every turn. Actual damage, stacking rules, replenishment and costs require a complete example encounter before implementation. Importantly, spending on an attack that prevents an enemy action is itself a defensive decision.

## Six possible build directions

These are a menu of original design possibilities, not six systems to implement at once. Start a concept test with enough overlap to discover combinations, then use owner playtests to choose what deserves expansion.

| Direction | Early reward that changes play | Later complementary reward | Real weakness or competing use |
| --- | --- | --- | --- |
| Heavy penetrator | A barrel accepts and fires a heavier stack that the starter weapon cannot | A tip breaches armour; a later payload deals extra damage after penetration | Large energy/material commitment and possible overkill; several enemies pressure it |
| Corrosive spread | A delivery head distributes a corrosive payload across enemies | A module rewards corrosion with protection, or an activator trades buildup for immediate damage | Takes time; direct damage may be needed to stop an imminent attack |
| Rapid discharge | A feeder fires several small assembled rounds in one activation | A fitted applicator adds an effect to each hit; a recovery module reduces selected waste | Must pay for each round's components; individual hits struggle against suitable protection |
| Charged shot | A chamber safely retains charge between turns | Split the discharge among enemies or concentrate it into a powerful single-target shot | Keeping the charge delays its payoff and occupies capacity needed now |
| Armour counterattack | A converter stores a limited benefit when scrap is used for protection | Spend that stored benefit on a shot, or retain part of the protection | Armour and offensive parts compete; no infinite gain from rebuilding the same plate |
| Scrap recovery | A separator recovers one useful offcut from certain spent assemblies | Compatible feedstock powers a shield or makes a low-cost finishing shot | Recovery is capped and lossy; throughput and reserve space still matter |

Bridges matter more than rigid classes: repeated hits can deliver corrosion; armour can buy charging time; a splitter can distribute a charged or corrosive payload. Upgrades should open these choices visibly. A wider magnet field earns its place only if it enables a meaningful new retrieval tactic under the actual load and energy rules; faster gathering alone does not satisfy the owner's earlier feedback.

## Progression and what persists

Root recommendation: each fight refreshes consumable stock, while the rig built during the run persists. Otherwise every fight would erase the capabilities that made the earlier rewards exciting.

| Boundary | Proposed persistence |
| --- | --- |
| Next turn, same fight | Unused components and fitted machinery remain; ordinary energy refreshes. Temporary armour follows its clearly displayed duration |
| Next fight, same run | Refresh encounter stock and clear combat statuses/temporary charge. Keep acquired rig modules, learned component capabilities and route progress. Damage to the rig persists unless explicitly repaired |
| New run after victory or defeat | Rebuild the run's equipment strategy through new rewards. Optional unlocks expand starting alternatives rather than automatically increasing all statistics |

Early semi-random rewards should offer useful possibilities and enough choice to form an intention. For example, choosing a charge chamber suggests saving and releasing energy; choosing a corrosive delivery head suggests delayed damage; choosing a protective converter suggests defence that feeds later attacks. Subsequent rewards can deepen that direction, offer a compatible bridge or support a pivot. Weighted offers can avoid long runs of unusable rewards without guaranteeing the same recipe every time. All runs still need basic offence and defence before a special reward appears.

Progress happens at several scales: a satisfying assembly and enemy reaction within the turn; a meaningful choice after the fight; a stage guardian and substantial capability/recovery reward at a route milestone; and a final confrontation that tests the developed rig. Completing the world objective can end a successful run. Replay then comes from different rewards, routes, enemies and combinations, with optional harder challenges after victory. Neither endless play nor account-wide power inflation is required. Stage count, final enemy and fiction remain open.

## Loading, anticipation and readable impact

During assembly, show the final predicted target(s), damage, status effects, protection and energy cost. Each added component updates the preview immediately. If an interaction is uncertain, name the uncertainty; do not present a guaranteed number and resolve it differently. Let the player rearrange before commitment without watching a full firing sequence every time.

On confirmation, lock the paid action and run a compact physical sequence: the body seats with a heavy clunk; the payload locks in and activates its distinctive visual/sound cue; the pressure or charge rises; a brief held moment leads into recoil and discharge. The shot visibly travels to its target or splits. Armour breaks, corrosion takes hold or protection mounts on the rig where the consequence occurs. Count up through the already-previewed contributions rather than revealing previously unknowable rules during the animation.

The sound should build from mechanical clicks to weight, charge and release, leaving room for the impact. Energy, enemy intent and survival information remain readable. Distinguish effects using shape, motion and sound as well as colour. Frequent actions must remain brisk, with an optional faster presentation once familiar; a large combo can receive stronger emphasis without making every turn a long unskippable performance. Failed and resisted effects must be just as legible as successful ones.

## Gameplay critique to carry into the next design goal

- If the largest affordable stack is always best, assembly offers no strategy. Compare several small shots, one heavy shot and a mixed attack/defence turn under the same resources.
- If holding material is always free and safe, the best move becomes waiting. Enemy pressure, reserve capacity and useful immediate alternatives must create a cost without imposing an arbitrary timer.
- If every new component is mandatory to make the previous reward work, semi-randomness creates dead runs. Reward parts need baseline utility, flexible combinations or a clear conversion route.
- If a recovery chain produces more parts and energy than it spends indefinitely, it can remove the attack/defence dilemma. Explicit finite triggers, losses or capacity limits must preserve it.
- If shot results depend on hidden stack order or random outcomes concealed by the preview, the game repeats its previous comprehension failure. Show cause and effect in the assembly itself.
- If every enemy demands the same amount of armour every round, defence becomes a tax. Mix attack patterns, setup turns, target priorities and interruptible threats with readable intentions.
- If physics mistakes decide everything, build strategy becomes irrelevant; if retrieval has no cost or constraints, the magnet becomes decoration. Test both sides together.
- If the loading spectacle outlasts the decision on ordinary turns, anticipation becomes waiting. Playtest repeated use, not only a showcase shot.

Recommended next owner-set goal: specify one representative mid-game fight, including the available pile, retained rig, possible stacks, energy, enemy intentions, replenishment, reward choice and actual loss/reset consequences. Walk through at least a heavy-shot route and a defensive combination route, then identify which questions need a playable concept test. Continue the owner's goal-by-goal process; this report does not authorize production or select the final story or art.

Research confidence: high for the described coverage and observed text, moderate for interpreting strategic roles without full-run play, untested for our proposed combinations and tuning. Source URLs above support the reference observations; every translation and numerical illustration is our own proposal. No reference art, code or full card catalogue was copied into the project.
