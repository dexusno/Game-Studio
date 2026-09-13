# Magnet Sweep — proposed complete redesign plan

Prepared for Klaus, 13 September 2026. **Current assignment: mechanics and scenes Q&A.** Review the mechanics and player-facing scene sequence one topic at a time, recording Klaus's answers here. Five premise options, reference-game studies, the art atlas, storyboard and first motion study are delivered reference material. Fiction and detailed combat rules remain open; redesign gameplay implementation has not started. The four-panel storyboard is concept art and graphics direction, not actual game screens or a finished UI specification.

Current art direction: after reviewing [the visual atlas](ART-DIRECTION.md), Klaus prefers 16C's graphical treatment and proposes [preparation in 2.5D with firing/impact and enemy response in 3D](COMBAT-CAMERA-FLOW.md). Refine that foundation and test repeatable assets and camera continuity. Damage, targeting and whether Fire ends the turn remain open; the remaining implementation steps follow owner-set goals.

Combat direction: choose a character with its own basic weapon and core recipes. Games contain fights, and fights contain turns. Once per turn, use the hanging magnet to gather source materials; assign those materials to available recipes in the furnace to craft the parts you choose. Build attacks and defence, or save unused raw resources and finished parts for later turns. After each fight, search the attackers' rubble and choose one of three recipe rewards or skip. The detailed owner-defined rules are recorded below; exact costs, yields, turn ending and persistence across fights/games remain to be specified.

The aim is a game whose purpose, objects, actions and consequences make sense while you play, with an expressive swinging magnet and a visual style you want to spend time in. This is a redesign of the whole experience. Existing features earn their place by serving the new design.

Use the [completed strategy study](BUILD-STRATEGY-STUDY.md) for Slay the Spire 2's strategic functions, combinations and balancing principles while building original mechanics, code, UI and graphics. Recipes determine what parts can be produced; physical parts build the current shot/defence and can be saved between turns. This distinction supersedes the earlier shorthand that the magnet directly retrieves finished ammunition components. Old example costs, multiple-haul proposals and progression numbers are not the owner's current rules.

## How we will work

### Mechanics and scenes Q&A — opening screen first

Known foundation: characters with individual weapons/abilities and core recipes; turn-based fights; one magnet gathering action per turn; player-assigned resource crafting in the furnace; physical ammunition/defence/modifier parts; unused raw-resource and part retention between turns; and one-of-three-or-skip recipe rewards after each fight. Preparation uses the 16B side view and combat presentation the 16C perspective. Exact costs, yields, effects, turn ending and persistence across fights/games remain open.

Klaus wants to begin at the opening screen and work forward through the player journey. Title Screen leads through Start New Game to Choose Character. He then defined the recipe/part combat and reward loop below. Ask one main question at a time and preserve these answers. The assistant's separate End Turn/multiple-shots recommendation has not been accepted or rejected.

Scene inventory: the title-screen baseline is recorded below; other entries remain discussion topics rather than a final scene list.

| Player-facing scene or function | Current standing |
| --- | --- |
| Title screen | Owner-defined first version below; Start New Game leads to Choose Character |
| Choose Character | Confirmed next scene; each character has its own weapon, strengths/abilities and starting core recipes; roster and layout open |
| Preparation and assembly | Selected side view; gather source materials, assign them to available recipes in the furnace, build shot/defence or save raw resources/parts; no separate furnace scene has been specified |
| Combat action | Selected perspective; firing, impacts and enemy response are moments within it, not automatically separate scenes |
| Rubble / recipe reward | After each fight, choose one of three recipes or skip; standalone scene versus overlay and physical presentation remain open |
| Other rig upgrades | Broader upgrade functions remain to define beyond the selected recipe reward system |
| Progress between fights | Progression required; map, hub or direct advancement not selected |
| Victory and defeat | Meaningful outcomes required; presentation and reset rules open |

### Scene 1 — title screen, first version

Owner-defined baseline, 13 September 2026. This is a design decision, not an implemented screen, and Klaus explicitly leaves room for later iteration.

| Menu entry | Intended function and availability |
| --- | --- |
| Start New Game | Open Choose Character. Interaction with an existing saved game/progression remains to define |
| Continue Game | Resume an existing game when the active profile has a game to continue |
| Choose Profile | Select a save profile, allowing more than one independent progression lane |
| Settings | Access graphical, sound and game settings; individual options are not yet specified |
| Collection | View the currently discovered items in the active profile once a game has been played in that profile |
| Quit to Desktop | Exit the game to the desktop |

Profiles scope their progression, resumable game and discovered-item collection. Exact profile creation/management, number of profiles, save behaviour and settings scope remain open. The visual treatment of unavailable Continue Game/Collection entries (hidden, disabled or explanatory empty state) is not yet selected. The game's final title, title-screen composition and menu styling remain open; the concept storyboard does not specify them.

### Scene 2 — Choose Character

Confirmed by Klaus, 13 September 2026: the first scene after Start New Game is **Choose Character**. Offer different characters, each with its own weapon and individual strengths and abilities. A new game starts with the chosen character's basic gun and core recipes. Character count, identities, exact abilities, starting-recipe lists and character unlock/access rules are not selected yet. What happens after confirming the character (intro, map or first fight) also remains open.

### Owner-defined recipe and part system

Games contain **fights**, and each fight contains **turns**. The reference is Slay the Spire 2's basic strategic functions, synergy and balancing approach, not its visual/card design. Build the game's own framework and graphics. The earlier reference study remains evidence of observed functions, not proof that its exact tuning will transfer.

| Term | Role in this game |
| --- | --- |
| Source materials / scrap | Ingredients physically gathered with the magnet; compatible with many recipes, allocated by the player for furnace crafting, with unused resources retained between turns |
| Recipe | Owned knowledge that allows production of a part; recipes are the equivalents of cards in build/reward selection, not physical ammunition or a card UI |
| Ammo part | Contributes damage, delivery or an attack ability to the assembled shot; translates offensive card functions |
| Defence part | Contributes to the current round's defence, including shields; translates defensive skill functions |
| Modifier part | Adds or changes functions on offence and/or defence; translates other skill functions |
| Buff / debuff | Temporary beneficial effects for this fight / temporary negative effects applied to enemies; detailed effect and expiry rules still need specification |

**Turn supply and assembly, confirmed direction:**

1. Use the magnet **once per turn** to collect an as-yet unspecified amount of varied source materials. They contain the ingredients for different owned recipes.
2. The gathered materials are compatible with **many recipes**. The player **assigns resources to recipes available at that moment**, crafting the parts needed most now or preparing a stock for later. The furnace does not automatically decide the crafted batch. Recipe costs, ingredient ratios, part yields and the detailed crafting UI remain open.
3. Use parts to build the shot and the defence for the current round. Combine parts to increase damage, broaden targets or add interacting functions. Firing spends the parts assembled into the shot; exact activation/consumption timing for other parts remains to define.
4. **Keep both unused raw resources and unused finished parts for later turns within the fight.** The player may save materials because they cannot or do not want to spend them now, and may craft ahead without using the output immediately. These are separate holdings, not an automatic conversion of all scrap at turn end. The default once-per-turn gathering rule supersedes earlier proposals about paying for several hauls in one turn.

Owner's example: retain a good damage-multiplier part from the previous turn; obtain a multi-enemy-shot part this turn; combine them so one shot deals increased damage to several enemies. This establishes intended cross-turn planning and compositional synergy, not specific damage values or multiplication order.

Requested effect directions include ordinary damage enhancers, fire, explosive tips, shield demolition, shields, fight-duration buffs and enemy debuffs. Klaus expects many combinations and further iteration; the final catalogue, exact interaction rules and numbers are open. These examples do not imply that all parts are equally available to every character.

**After each fight:** pick through the attackers' rubble, discover recipe rewards, and **choose one of three recipes or skip all three**. The choice should fit or redirect the current build. A selected recipe is added to the player's recipe collection and expands the recipes usable in the current game; it is not merely a one-use reward part. Each new game still starts with the chosen character's core recipes and basic gun. Detailed profile-Collection records, whether unchosen offers count as discoveries, and whether discovery changes future-game availability remain to clarify.

**Still open:** ingredient/output counts and costs; recipe duplicate/upgrade rules and character-specific reward pools; energy and activation rules; shots per turn and whether Fire ends the turn; shield expiry and precise effects; inventory capacity; parts/materials carried between fights; profile discovery versus current-game recipe access; and the scene after character confirmation. No discard/reshuffle system, fixed starter-kit size or numerical balance is selected by the reference analogy.

**Full redesign; implementation not started.** Klaus reconfirmed that little of the old game's code may be usable. Design the new game on its own requirements, and assess old code for reuse only where it fits. Existing prototype code and its recorded stage describe the old design. Storyboards, the review page and standalone Blender camera tests are design/art tools; they are not a partially implemented redesign or a commitment to its code architecture.

You review and revise this plan as the direction evolves, and set a goal for the work you want to pursue. We complete that goal, give you something concrete to judge, and revise it until you are satisfied. We begin another implementation step only when you set its goal. A step can be split into smaller goals if useful. The current art goal is already authorized; no development timetable is imposed.

## Current instructions and preferences

| Area | What carries forward |
| --- | --- |
| Game concept | Games contain fights and turns; characters have individual weapons/abilities and core recipes. Magnet haul → furnace → physical parts → shot/defence. Slay the Spire informs strategic functions and balance; build original framework/graphics |
| Preparation view | A separate 2.5D side-view screen showing enemies, available scrap and a component bar used to build the shot. The middle row, 16B, is the corresponding viewpoint example |
| Action view and look | 16C is the owner's preferred graphical treatment and 3D perspective for the shot and impact. Explore enemy responses in 3D too. Keep the same visual identity across views |
| Reference asset | The owner reattached and reconfirmed [board 16](../assets/concepts/art-direction-2026-09-13/16-depth-camera.webp) for graphics and viewpoints; its saved pixels match the attachment exactly. Its props and HUD are references, not a complete mechanic specification |
| Physical magnet | Hang it from a chain or similar suspension; use once per turn to gather source materials for the furnace. Make attraction, collisions, weight and stacked scrap visible and satisfying |
| Shot assembly | Build ammunition from physical components in a linear stack/loading bar; components add properties or combine functions. Firing spends its component material. The pictured rack of completed bullets does not replace this system |
| Strategic choices | A shared energy/resource economy must create meaningful choices between offence, defence and preparation. Support focused and multiple-target attacks and scrap-built protection; exact rules remain to be designed |
| Build variety | After each fight, search attackers' rubble and choose one of three recipe rewards or skip. Combine ammo/defence/modifier parts and retain unused parts across turns to form useful synergies |
| Progress and stakes | Meaningful encounter rewards, intermediate objectives, clear stage completion, substantial obstacles and a final confrontation. Winning must matter, losing must be possible, and another run should offer different strategies |
| Upgrade quality | Enable something useful or an exciting new tactic. Merely gathering the same permitted load faster was rejected as sufficient justification for an upgrade |
| Clarity | Recognisable objects, visibly connected mechanisms, familiar words, clear goals and consequences, restrained UI and an interactive tutorial. The old jargon-filled, overcrowded board is not the target |
| Feedback and audio | Component loading should build anticipation into discharge and readable effects. Strong magnetic/impact sounds and suitable soothing background music are requested; tracks and exact sound direction remain open |
| Scope and review | Keep the game understandable and manageable for a solo owner; do not add huge lore or an excessive catalogue of systems. Development timelines are explicitly not the design constraint. Gameplay critique concerns fun, fairness and comprehension, not schedules |
| Delivery process | Klaus sets implementation goals; complete and revise each until he is satisfied. A playable concept demo must be judged before full development. Functional tests alone do not establish fun |

## Proposals and decisions still open

- **Persistence:** keeping both unused raw resources and unused finished parts between turns in one fight is confirmed. Each new game begins with the chosen character's basic gun and core recipes; chosen fight rewards expand that game's recipe access. Fresh material each fight remains an earlier proposal; cross-fight part/material carryover, storage limits, profile discovery and future-game unlock rules remain to settle.
- **Combat rules:** exact energy and haul costs, carrying limits and risky-pull consequences; the relationship between Fire and End Turn; number of actions/shots; targeting and multi-target distribution; armour duration; effect order; and damage values. Poison, piercing, shock, explosions and other effects are possibilities, not an approved full catalogue.
- **World and progression:** character roster/identities, final fiction, specific mid-goals/bosses, route structure, rewards beyond the confirmed recipe choice, defeat/retry consequences and profile unlocks. The suggested village, harbour, survivor or factory premises are not selected canon.
- **Visual production:** refine the 16B/16C treatment into consistent objects, UI and motion. Exact character, materials, loading-bar capacity and camera transitions are not all approved simply because the reference is liked. The first shared-geometry motion test is delivered; matching the reference's finish in actual assets remains unproved.
- **Product details:** exact controls, content scope, release configuration and pricing remain open. The commercial aim is a worthwhile solo project, with a stated target of at least NOK 20,000 profit per game and further games if profitability is demonstrated.

The old four-site recovery structure, safe unlimited hauling, equipment catalogue and earlier implementation hypotheses are historical material. They are not automatically retained in this combat redesign. [DECISIONS.md](../DECISIONS.md) preserves the chronology and superseded choices.

## 1. Decide who we are and why the work matters

Current deliverable: [five premise options](PREMISE-OPTIONS.md), awaiting Klaus's choice or revisions. Preparing the choices does not settle the premise.

Establish the game's simple premise: the player's role, their situation, why using this magnet and weapon matters, and the larger result they want to achieve. Connect battles and stage victories to a visible end goal. Keep the backstory compact enough to communicate through the opening, environment and progress rather than a lore manual.

**What you receive:** a few short premise options, each explaining who you are, what a typical job accomplishes and what changes when the game is completed. We develop your preferred direction.

**What you judge:** “Do I understand why I am doing this, and do I care about reaching the end?”

## 2. Decide how a level works, including how we actually lose

For the proposed combat direction, use a complete encounter and one detailed round: enemy intentions, available scrap, powered loads, attack/shield choices, forge preview, material consumption and replenishment, then enemy response. Connect encounter rewards to a stage guardian and the eventual final mission. Settle the strategic role of the physical magnet before multiplying materials and status effects. The study's examples and three-stage outline are proposals, not selected rules.

Describe one typical mid-game level from arrival to outcome. Give it a concrete objective, a reason to take an optional risk, an understandable success condition and a real failure condition. Decide what failure costs, what is retained, and what restarting a level or expedition means. The actual losing rules must survive the retry/save flow; unlimited undo must not quietly erase the agreed stakes.

Explain how careful play differs from a risky move and how a player can recognise danger early enough to make a choice. Avoid surprise punishment caused by unexplained rules or unstable physics. Consider what happens if the objective becomes unreachable so the player gets a clear resolution rather than a dead end. Exact penalties and resources are choices for this goal, not decisions made by this plan.

**What you receive:** a short illustrated level walkthrough, a safe/risky decision example, and a plain explanation of win, loss, retreat and retry where applicable.

**What you judge:** “Would beating this feel earned? Could I lose, understand why, and want another attempt?”

## 3. Refine the chosen visual foundation and two-view presentation

**Breadth exploration delivered:** the original minimum of five families with five variants was expanded to **15 families with five variants each**, plus a three-panel camera comparison. All 16 boards are saved in [the atlas](ART-DIRECTION.md), with exact prompts, provenance and static observations. Do not repeat that exploration by default.

**Preferred foundation received:** use 16C's visual treatment, the 16B-style side view for preparation, and a 16C-style perspective for firing and impact. Explore the enemy response with an angle that clearly shows the rig being hit. The owner explicitly reconfirmed the existing board as the graphics/viewpoint reference.

The [prepare/fire/impact/enemy-response storyboard and first motion study](COMBAT-CAMERA-FLOW.md#delivered-storyboard-and-first-motion-test) are delivered. The generated board includes loading and discharge versions. A separate eight-second Blender render uses shared geometry in both cameras, with readable target/plate framing and unchanged remaining stock on return. Its simple models do not yet match the reference's finish. Continue refining readable scrap, the forge/loading channel, materials and lighting in those cameras; do not decide damage or turn rules merely to fill a frame.

**What you receive next:** refinement toward the chosen quality using the delivered storyboard and motion evidence, followed by practical rules for shapes, materials, lighting, interface hierarchy and camera use. The first simple motion test does not prove that the complete reference-quality runtime appearance is already achieved.

**What you judge:** “Does this preserve the look I chose, make both phases clear and make the shot satisfying to watch?”

## 4. Make the hanging magnet feel good in a small playable scene

Build an isolated side-view physics experiment using the chosen visual direction on a representative magnet and a small set of scrap. Decide how you move its suspension point, raise/lower it and control the field. The chain or suspension should visibly support the magnet; acceleration, stopping and carried weight should produce understandable movement.

Test pieces pulling toward the magnet, colliding, attaching, hanging, falling, sliding and stacking. Check whether pulling one piece disturbs a pile in useful, readable ways. Keep interaction on a clear, reachable side-view plane while using depth for visual appeal. Tune control and damping so skillful handling is possible and ordinary movement does not become a struggle against wobble. Add essential pull, contact and release sounds now.

Use just enough temporary UI to operate and judge this experiment. It is not yet the full game or tutorial. Reuse existing source/assets only where they support the new behavior; preserve the old prototype and saves separately.

**What you receive:** a playable magnet-and-scrap scene with representative art, direct controls and observable physical responses.

**What you judge:** “Is moving, pulling, lifting and dropping enjoyable before we add a whole level around it?”

## 5. Build one combat encounter whose actions explain themselves

Turn the encounter from step 2 into the agreed preparation/action flow using the physical interaction from step 4. Give scrap, ammunition, protection, the magnet and enemies distinct recognisable forms and visible jobs. Show what the enemy is about to do, what the player can afford, how components change the assembled shot and what happens when it is fired.

The component bar must connect clearly to ammunition and the weapon. Protection must visibly receive attacks where it is mounted. Any added mechanism must visibly connect to what it affects; do not bring back arbitrary pads or hidden completion triggers from the rejected recovery board.

Arrange the scene so stacking, height, reach and swinging matter to the decisions. Introduce only the mechanisms needed for this representative level. Give the current objective and immediate danger clear visual priority; the UI/UX specialist works with the level designer and artist here, rather than arriving after the layout is finished. Include the agreed winning and losing outcomes in the playable level.

**What you receive:** one playable encounter with recognisable pieces, understandable attack/defence choices, clear camera transitions and a real risk/reward decision.

**What you judge:** “Can I work out what is happening and why my action helps, without someone explaining the board to me?”

## 6. Refine the interface and turn the first play into a tutorial

Develop the small interface already used in steps 4–5 into a consistent player experience. Keep the scene visually dominant. Show the immediate objective, relevant resources and available action; bring forward detail when the player selects an object, faces danger or makes a purchase. Remove repeated instructions and panels that compete with the action.

Use familiar words and highlight the actual object or destination being discussed. Teach one action at a time through doing it, seeing its effect and confirming success. Let players revisit help. A tutorial should not require someone to understand the game's invented vocabulary before they can begin. If a practice section protects the player from loss, make that boundary explicit and ensure the normal level's stakes are real.

Carry the same language and visual identity through start, pause, inspection, success, failure and retry. Make essential guidance available with the supported controls, not only by hovering. Check text size, contrast and screen scaling in the actual game. Support feedback with restrained sound and music suited to the chosen world.

**What you receive:** an interactive introduction and a consistent, uncluttered interface across the representative level and its outcomes.

**What you judge:** “Can I start without a manual, understand a mistake and know what to do next?”

## 7. Make rewards and upgrades change what we can accomplish

Connect encounter and stage rewards to the larger purpose from step 1. Show what the player earned, what it contributes toward and why an upgrade matters. Develop a small set of contrasting rewards that enable different actions and builds, influenced by semi-random early offers and later synergies. Try them in the representative encounter and show their effect on the rig or its behavior wherever possible.

Do not use the rejected radius-only improvement as the model for meaningful progression. Establish the basic equipment, an intermediate setup and the intended end-state payoff before expanding the catalogue. Decide what lasts between attempts, how loss interacts with ownership and what makes another playthrough worthwhile. Avoid adding currencies, tiers or loot solely to fill a screen.

**What you receive:** a playable earn → choose → upgrade → use loop, with visible progress toward the end goal and understandable consequences for losses.

**What you judge:** “Do I want this reward, and can I use it to accomplish something I could not do before?”

## 8. Review a complete concept demo before full development

Combine the accepted pieces into a short, representative demo: an approachable start, meaningful combat, an optional risk, actual success and failure, a reward choice and a chance to use the resulting upgrade. Include representative mid-game difficulty; do not make the whole demo an easy lesson with no stakes. Communicate the larger goal and how winning the encounter contributes to it.

Use the selected graphics, sounds and interface together. This demo is where we check whether the whole experience makes sense and feels rewarding. Fix issues across mechanics, art and guidance rather than assuming a confusing result only needs another tutorial paragraph. The exact demo length and number of scenes are set in its goal.

**What you receive:** a packaged concept demo you can play without me narrating the solution. Independent QA checks technical behavior; you judge clarity, feel, challenge and appeal.

**What you judge:** “Is this the game I want us to finish? Does the whole experience work, and do I want to play again?”

We revise this demo until you are satisfied before expanding into the full game. Passing automated tests alone does not satisfy this step.

## 9. Build out the approved game and deliver its ending

After the concept demo is accepted and you set further goals, create the remaining levels and progression around its proven visual and interaction language. Add variation that changes decisions while keeping objects and rules recognisable. Let the final challenge use what the player has learned and deliver the visible outcome promised by the premise.

Polish pacing, sound, music, controls and presentation as the game grows. Check ordinary play, risky play, failure/retry, upgrades, save/resume, input focus, display sizes and performance in the actual packaged build. Test for unwinnable states, ambiguous instructions and rewards that can be duplicated by restarting. Preserve existing owner saves; any redesigned save behavior needs an explicit migration/preservation plan.

**What you receive:** the complete redesigned playable game, built through your subsequent goals and revisions, with honest verification and any remaining limitations stated.

**What you judge:** “Does the game stay understandable and rewarding from the first pull to the ending?”

## Specialist responsibilities

The game designer connects purpose, choices, stakes and progression. The UI/UX designer works from the start on comprehension, interaction flow and visual hierarchy. The art director creates and carries the selected visual language into usable assets. The engineer implements and verifies the physics and game behavior; the audio designer connects sound to action and tone. Gameplay critique challenges clarity, fairness, reward and replay appeal. The producer coordinates the current goal, and QA independently checks the resulting build. Use these roles only where the authorized goal benefits from them.

**Next action: continue the player journey by asking which scene follows confirmation in Choose Character.** Furnace control is answered: the player assigns resources to available recipes, and both unused resources and crafted parts can carry into later turns. Preserve once-per-turn gathering and one-of-three-or-skip recipe rewards. Use the selected art as a reference rather than a finished screen specification; full gameplay implementation follows an owner-set goal. Store preparation, publication, marketing and purchases are outside this design discussion.
