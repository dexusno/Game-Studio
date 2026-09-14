# Magnet Sweep — proposed complete redesign plan

Prepared for Klaus, 13 September 2026; updated 14 September. **Current decision: encounter selection, claw collection, crafting/shop, Load, targeting, Fire or End Turn / Defend, enemy response and defeat flow are specified.** The four-mercenary recipe catalogue is delivered as a draft. Klaus authorized proposed specialties and recipe designs at a comparable scale to the Slay the Spire 2 card database, using original names, prose and material-based rules. The owner has chosen a mercenary campaign through fortified cities under robot attack, with recipe crafting, branching encounter choices, shops and permanent weapon-system upgrades. Detailed combat tuning, final character identities and campaign ending remain open; redesign gameplay implementation has not started. The old five premises are archived alternatives. The four-panel storyboard is concept art and graphics direction, not actual game screens or a finished UI specification.

The [four-mercenary recipe catalogue](RECIPE-CATALOGUE.md) contains 606 draft recipes: 126 shared and 120 exclusive per mercenary, with shared/exclusive Base, Common, Uncommon, Rare and Legendary pools. It proposes Mara's Heat, Ivo's Corrosion/Mark and saved parts, Ada's helper Bolt, and Noor's Charge. Each character can discover 246 recipes but carries only what fits recipe memory. The proposed 12-recipe starter kit never overrides the still-open memory capacity. Numbers, names and catalogue timing conventions are proposals; the owner-confirmed rules below govern. This pass adds no graphics or gameplay functionality. Full permanent upgrades, buffs and other modifier assets are later work.

Current art anchor, confirmed 14 September 2026: [art anchor.png](art%20anchor.png) is the game's **STYLE reference, not gameplay screenshots**. Its bottom **16C / 3D Perspective** image anchors the attack camera's look and composition. The concept's prop placement is not a gameplay layout requirement: planning/loading now places the claw and scrap gathering behind the player/gun, with enemies and intent indicators ahead. Every city must have a unique backdrop and surroundings while retaining this shared style. [Asset provenance](ART-DIRECTION.md#owner-supplied-style-anchor--14-september-2026) records the preserved PNG.

Current art direction: after reviewing [the visual atlas](ART-DIRECTION.md), Klaus prefers 16C's graphical treatment and proposes [preparation in 2.5D with firing/impact and enemy response in 3D](COMBAT-CAMERA-FLOW.md). Refine that foundation and test repeatable assets and camera continuity. Load prepares the bullet/shield, targets are selected afterward, and Fire leads through impact to surviving enemies' actions. Exact damage/effect arithmetic and loading reversibility remain open; implementation follows owner-set goals.

Combat direction: choose a mercenary with its own basic weapon and core recipes. Each city has a wave of 10–20 fights, with turns inside each fight. Each turn choose from baseline ingredient options and use Collect for automatic claw handling, or spend the saved Precision attempt, then assign gathered materials to furnace recipes. Once per fight, the player may choose a turn to use a manual timing marker for a bonus; this does not create a second ordinary haul. The owner now specifies a claw for collection. Recipe memory has a hard capacity; choose any stored recipe whose costs and availability rules allow crafting. A cooldown recipe becomes usable again as soon as all its counters are removed, including within the same turn, if resources suffice. Recipes without a cooldown ability allow one craft per turn unless a permanent upgrade explicitly grants more uses. Both unused raw resources and crafted parts clear at fight end; retained recipes and permanent system upgrades persist through the game. Upgrades can increase memory capacity or grant fresh resources/ready parts at fight start or on specified turns. Victories supply cores and recipes; Officers also award a permanent upgrade. City-opening upgrades, encounter choices, shops, mysteries and stronger later cities develop the build. Exact costs, yields, memory size, cooldown interactions, loading reversibility and other detailed rules remain open.

The aim is a game whose purpose, objects, actions and consequences make sense while you play, with gathering that supports combat choices and a visual style you want to spend time in. This is a redesign of the whole experience. Existing features earn their place by serving the new design.

Use the [completed strategy study](BUILD-STRATEGY-STUDY.md) for Slay the Spire 2's strategic functions, combinations and balancing principles while building original mechanics, code, UI and graphics. Recipes determine what parts can be produced; physical parts build the current shot/defence and can be saved between turns. This distinction supersedes the earlier shorthand that the magnet directly retrieves finished ammunition components. Old example costs, multiple-haul proposals and progression numbers are not the owner's current rules.

## Round resets, cooling and accepted loot — 14 September 2026

**Latest owner rules:** build a new bullet and shield each round. After the complete enemy turn, clear the current builds and remaining round Shield; do not clear Shield between individual enemies' attacks. Upgrades may grant fresh Shield at the beginning of a round. Such a grant creates new protection rather than carrying last round's Shield forward. Unused raw resources and unused parts still remain in their pools within the fight; the assembly reset does not delete that reserve. Fired/used parts remain consumed. Exact handling of a loaded-but-unfired build still belongs to Load/undo rules.

**Cooling:** using a recipe means crafting it and applying its cooldown counters then. At the end of the enemy turn, automatically remove one counter from every recipe with active cooling, stopping at zero. Upgrades and special cooling effects can remove more. A cooling Utility/Helper part removes its stated **X from every currently cooling recipe**, not X divided among selected recipes. No recipe target selector is needed. This changes cooling effects, not all Utility/Helper parts into cooling items. Under the all-recipes rule the originating recipe is also affected if cooling when the effect resolves. A numeric-cooldown recipe at zero is ready immediately if resources suffice; a recipe with no cooldown ability still allows only its ordinary once-per-turn craft unless a permanent upgrade says otherwise.

**First cooldown tick confirmed:** counters added by crafting also tick at the end of that same round's enemy turn. Cooldown 2 crafted in round 4 goes 2 -> 1 at the end of round 4, then 1 -> 0 at the end of round 5, and is ready in round 6. This replaces the earlier round-7 example. Cooldown 1 normally becomes ready for the very next round. Counter-removal effects can still make a recipe ready earlier during preparation; the exact printed cooldown values will need balance review under this shorter schedule.

**Permanent claw enhancements:** these are upgrades, not craftable parts. Buy them in the shop or find them after Officer fights. Buying one or clicking to accept its loot activates it immediately for the remainder of the current game. This explicitly adds Officer rewards to the earlier shop-only acquisition route. They do not occupy the parts pool or recipe memory, and they do not automatically grant extra Precision attempts. An effect with a specified future trigger still waits for that trigger; immediate installation does not itself repeat an already-completed collection. Exact upgrade effects, stock and purchase/stack limits remain to design.

**Loot screen:** after a fight, display a list of found loot. Click each desired item to accept it; **Skip** finishes the selection and leaves the unaccepted loot. Previously accepted items are kept when Skip is pressed. An accepted permanent upgrade activates immediately. Cores remain loot sold for credits under the established economy, rather than becoming spendable credits merely by appearing in rubble. The existing recipe-memory capacity still applies. **Recipe rewards remain a separate choice of zero or one from three.** Accept one into memory, exchanging an existing recipe if capacity is full, or skip all three. Skipping does not remove or replace a held recipe and does not prevent accepting other loot. Never exceed memory capacity. This was explicitly reconfirmed by the owner.

**Catalogue clashes raised for decision:**

- **Claw conflict resolved:** retain all 23 temporary gathering recipes, including SH121–SH126, alongside permanent claw upgrades. The temporary recipes retain their next-round-only craft-and-fit behaviour. Purchased/accepted permanent upgrades are not parts; temporary recipe outputs remain a separate system. Neither grants another Precision attempt without an explicitly selected exception.
- **Cooling targeting resolved:** cooling applies across all currently cooling recipes, including its origin if cooling. The catalogue removes manual target counts, type restrictions and self-exclusions from cooling effects while preserving printed costs and amounts. Full-reset drafts clear all active cooldowns. Some previously distinct target-count variants now overlap or differ mainly by cost; these need balance/content review, not extra selectors.
- Several draft Shield-retention recipes preserve old protection across rounds. They are not automatically owner-approved exceptions to the new reset rule. Flag them for redesign or explicit exception selection. Fresh start-of-round Shield from an upgrade is already allowed.
- Shield duration is settled. The activation point of secondary Shield/Utility/Helper effects and Load reversibility still need exact controls and ordering; a reset rule alone does not resolve them.

**Next:** review the balance/identity of global cooling recipes and resolve the drafted Shield-retention exceptions, then Load reversibility and secondary-effect activation. The first cooldown tick, temporary/permanent claw coexistence and zero-or-one recipe rewards are settled. Continue design work; no gameplay implementation is authorized.

## Owner-defined encounter and turn sequence — 14 September 2026

This section records Klaus's selected flow and his follow-up choice of an **End Turn / Defend** button. It supersedes earlier uncertainty about the claw, in-fight shop access, recipe pop-up versus full-screen presentation, and whether Fire leads to the enemy turn. It is a design specification, not an implemented demo. One ordinary collection event per round and one optional Precision attempt per fight remain in force.

### Encounter choice and initial inspection

Enemies approach and the **Choose Your Battle** screen offers three semi-random encounters. At least one Regular option is always present, and several Regular options may appear. An Officer appears among the three choices about one quarter of the time; Mystery also has about a one-in-four chance of appearing. Record these as approximate appearance rates across offers, rather than silently applying a 25% roll independently to every slot. Whether Officer and Mystery occurrence is independent, how they share an offer, and whether duplicate special types are possible remain distribution details to specify. The mandatory city boss and noncombat Mystery branches still need their own handling.

After choosing a combat encounter, its enemies appear with their **intent, strength, buffs and relevant state** visible. The player reviews them and presses **OK** before opening collection. This review informs the current round's supply and build decisions. A new encounter is chosen between fights, not each round of the same fight. Existing merchant and tech-support Mystery outcomes remain available; they do not manufacture an enemy turn when no combat occurs.

### Collection, preparation and resolution

| Step / screen | Player action and visible result |
| --- | --- |
| Claw collection | Choose from the available baseline ingredient options. Press **Collect** for ordinary handling, or **Precision** to use this fight's remaining manual timing attempt. Precision affects the bonus of this collection, not a second haul. The claw performs a cool but **very short** pickup and furnace-dump animation, then adds the actual materials to the resource pool. Exact options, amounts, bonus bands and precision confirm/cancel behaviour remain open. |
| Main loadout | Show the gun, current bullet build, current shield build, available parts and available resources. Offer **Recipe**, **Shop**, **Load**, and the newly selected **End Turn / Defend** button. Clicking a build part assigns it to its appropriate bullet or shield location. Continue to support at least 20 parts per bullet and per shield with readable combined effects. |
| Full-screen Recipe view | Show all recipes in current memory and a small resource counter/meter. Clicking a recipe focuses it and exposes its cost, effects and **Use** action. **Use means craft:** pay the materials, apply the existing crafting availability/cooldown rule, and add its output to the parts pool. Update affordability immediately. Unavailable recipes are greyed out but remain clickable for inspection; crafting is blocked while resources, cooldown or permitted uses disallow it. Repeat as desired, then **Close** returns to the loadout. |
| Shop during preparation | **Shop** opens a shop containing parts, raw resources and upgrades; special items remain optional future content. Prices vary, with better and rarer goods generally expensive. Spend credits or simply inspect, then **Close** returns to the same loadout. Existing recipe sales and between-fight shop access remain; this adds shop access during the fight. Raw resources and ready parts bought here are available for this preparation. Exact stock refresh, prices, purchase limits and when installed upgrades begin affecting the fight remain open. |
| Assemble and Load | Click the desired available parts to build the bullet and shield. **Load is greyed out until the bullet contains a part.** When satisfied, press **Load** to load that assembled bullet into the gun and prepare the shield. Loading is a separate action from firing. Whether it can be undone, and whether crafting/shopping remains accessible afterward, are not yet selected. |
| Target and Fire | After loading, **Fire** changes from grey to a red pulsing/blinking ready state. Click the enemy to target it, or select the enemies allowed by a multi-enemy component, then press **Fire**. The component governs target eligibility/count; it does not allow unrestricted extra targets. A loaded state and selected legal targets must be distinguishable. Exact target-count and automatic-all-enemy selection details remain to specify. |
| Shot and impact | Show a cool, **short** firing animation in the selected 16C perspective. Stronger builds produce a larger boom, and short impact reactions communicate the power/effects of the hit. Defeated robots fall into rubble with their power cores glowing visibly. These visuals do not introduce another accuracy challenge or immediate core spending. |
| Enemy response | If enemies survive and the fight remains active, they take their turn immediately after the shot resolves. They follow the displayed intentions: attack, buff or use other special abilities. Ordinary attack damage depletes Shield first; excess reduces player HP and the lifebar. Exact status, bypass, multi-hit and simultaneous-trigger rules still need the selected combat rule set. |
| Next round | If the player and any enemies remain in the fight, advance the round, show the surviving enemies' new intentions and return to the claw/baseline-choice step, then loadout. Keep unused resources and parts. The manual Precision use remains spent if already used. Clear round builds/remaining Shield and tick active cooldowns at the end of the enemy turn, including newly crafted counters in that round's first tick. Fresh upgrade Shield can arrive next round; exact status/supply ordering remains open; a repeated OK prompt each round is not yet required. |
| End of fight | The fight ends when no enemies remain alive/in the fight, including enemies that fled where their abilities permit, or the player dies. Killed robots leave glowing cores in rubble. Existing victory salvage/recipe rewards follow qualifying wins; rewards for escaped robots or an all-fled ending remain open. Both unused consumable pools clear at fight end. |
| Game Over | Player death ends the game and shows an irritating, gloating evil-AI animation intended to encourage revenge. Exact defeat lines, animation length/skip control, restart destination and what profile progress survives remain to define. |

**Owner-selected alternate exit: End Turn / Defend.** The player can finish preparation without Ammo or deliberately choose a defence-only round while Load is unavailable. This action prepares the chosen shield and proceeds to enemy actions **without firing**. Unfired parts remain unused under the existing retention rule. It prevents the empty-bullet Load requirement from trapping the player. Exact availability after a bullet has already been loaded belongs to the unresolved loading/undo rules.

**Crafting, staging and activation are separate.** Recipe **Use** spends resources to create a part; clicking that part stages it for the build; **Load** prepares the selected bullet and shield; **Fire** fires and consumes the assembled shot. **End Turn / Defend** prepares defence without a shot. The recipe catalogue's older immediate Shield/Utility/Helper activation conventions are not silently confirmed by this flow: decide when shield secondary effects, cooling, helpers and parts affecting both builds actually activate, and how those part types are selected. Temporary next-round gathering recipes retain their explicit craft-and-fit timing alongside immediately installed permanent claw upgrades. A full-screen recipe view must still give the player a way to consult relevant enemy intentions; its exact presentation is UI work.

### Remaining turn and encounter gaps

- **Load and undo:** can a loaded build be unloaded or edited, and can Recipe/Shop be reopened? What is reserved versus irreversibly spent at Load? Does End Turn / Defend remain available after Load?
- **Part activation:** when do Shield secondary effects, Utility, Helper and dual-purpose Modifiers activate? Cooling must be usable during preparation to preserve the owner-selected ability to clear counters and craft again in the same round. Do some parts need a Use action or target choice instead of automatic bullet/shield assignment? Preserve each part's effect once, without making crafting itself activate ordinary parts.
- **Round boundary:** Shield/build clearing and automatic cooldown reduction occur at enemy-turn end, including fresh counters; order remaining status effects, delayed supplies and new intent; whether later rounds repeat the initial OK acknowledgement.
- **Shop transactions:** stock persistence across closes and rounds, upgrade activation timing, recipe-memory replacement on purchase and next-fight staging for purchases made between fights. Glowing rubble cores remain post-fight loot until collection timing is selected otherwise.
- **Encounter/end conditions:** special-type offer distribution, boss/noncombat-Mystery routing, fleeing rewards, deaths outside the main shot and simultaneous defeat/victory. Player death and its Game Over presentation are settled; profile/retry consequences are not.

**Next:** define what Load commits and whether it can be reversed, then resolve the activation timing of non-Ammo parts. The separate resource-budget, enemy-roster and economy gaps still need demo values. No gameplay code or new artwork is requested by this flow definition.

## Gathering options — baseline supply and claw

**Owner discussion, 14 September 2026:** Klaus says the magnet is becoming less central and asks for ways gathering could provide a meaningful skill bonus without becoming a repeated chore. He proposes a guaranteed baseline resource pool each turn, possibly replacing the magnet with a mechanical claw, and a golf-style moving marker whose stopping accuracy improves the bonus haul. This began as an options discussion. **Subsequent owner decision: select C, the occasional grab, with one optional manual timing-marker use per fight at a moment the player chooses.** Normal collection requires choosing baseline ingredients and pressing Collect, with no timing input; Precision is the optional alternative. The later turn sequence selects the claw and baseline ingredient choice. Resource amounts and exact bonus calculation remain open. References below to the magnet's central physical challenge record the earlier direction; do not insist on that challenge while this decision is reopened. Rear gathering, the style anchor and the combat camera pairing remain the current scene foundation.

### Three candidates

| Candidate | Player action and result | Why it could work | What could make it a chore |
| --- | --- | --- | --- |
| A — Quick precision grab each turn | Select a resource priority, then stop one moving marker during the claw's grab. Receive the guaranteed baseline plus a small targeted bonus based on accuracy | Clear feedback and a brief physical action; the resource choice connects timing to the current build | The same centre target can become routine. A large manual advantage makes even an optional input feel compulsory |
| B — Choose a scrap pocket | Receive the baseline, then choose among a few clearly shown resource bundles; the claw handles the pickup automatically | Changes the available crafting choices without demanding reflexes; quick to operate | One bundle may become the obvious choice every turn, or the selection may duplicate decisions already made in crafting |
| **C — Occasional precision grab — selected** | Normal collection is automatic. Once per fight, the player may choose a turn to use the manual timing marker for a bonus | Lower repetition and a decision about when a targeted supply boost matters | A large all-or-nothing payout could make one miss feel worse than several small misses. Keep a useful targeted minimum |

**Selected frequency:** one optional manual grab per fight; A and B remain comparison alternatives, not the current input schedule. The player can spend the grab early for immediate supplies or save it for a later shortage or prepared recipe combination. Once used, the manual opportunity is spent for that fight regardless of timing quality; the next fight has its own single opportunity. An unused opportunity does not accumulate across fights. **Updated input choice:** the owner now specifies a claw screen with baseline ingredient options each round and Collect/Precision buttons. A persistent-priority-only flow remains an earlier alternative. The claw suits the mixed-scrap fiction; it does not require introducing aluminium as a sixth ingredient or renaming the project now. For the selected optional timing action, a presentation proposal is to show the target on the claw's short movement path or beside it, with one stop input and clear bonus quantities. Target choice is made without a timer; only the brief precision action is timed.

### A concrete reward example to test

Illustrative values, not chosen economy: a turn guarantees **8 mixed material units**. With Copper selected, automatic handling adds **2 Copper**. Manual handling instead adds **1, 2 or 3 Copper**, depending on timing. A miss never removes the baseline or materials already held. Display the bonus directly, rather than a large percentage that could imply the whole turn's supply depends on a perfect result. The standard automatic result should be calibrated against ordinary manual performance; it need not equal an expert's ceiling.

In this illustrative reward model, manual handling replaces the automatic bonus calculation for that one collection event. It is not a second haul and does not add another ordinary gathering activation. The owner has selected one optional manual use per fight; bonus bands and the automatic result remain open. This example illustrates a small edge, not proof that eight materials support our recipes or that the material types have equal value.

### Rare and legendary recipes for perfect timing

**Owner request, 14 September 2026:** add Rare and Legendary perfect-timing enhancements to the recipe collection. [SH121–SH126 in the catalogue](RECIPE-CATALOGUE.md#rare--manual-grab-enhancements) now supply six shared recipes, three of each rarity, with named parts, material costs and cooldowns. This extends the catalogue to 606 recipes; each character can discover 246. The owner selected this content direction; exact names, costs, strengths and cooldowns remain balance proposals.

| Recipe | Rarity | Next-round direction |
| --- | --- | --- |
| Calibrated Jaws | Rare | One selected bulk material; a Perfect grab adds more of it |
| Clean Separation | Rare | A planned pair of different bulk materials |
| Chip Finder | Rare | Extra Circuit on Perfect, with an unconditional Glass addition |
| Perfect Salvage | Legendary | One extra copy of the unmodified perfect-bonus bundle; excludes baseline and other bonuses |
| Full Spectrum | Legendary | Extra supplies across all five materials on Perfect |
| Salvage Foundry | Legendary | A ready-made Ammo part and Shield part on Perfect |

The catalogue owns the current numbers and complete rules. Each recipe includes an unconditional next-round material addition that also works with automatic or missed handling. Earlier examples of two possible recipes are replaced by these complete entries. The claw is selected; the final supply/bonus quantities remain open.

These are crafted enhancements for **the next round only**, following the existing gathering-recipe timing. Baseline supplies remain unaffected by a miss. Choosing one of these recipes is an explicit investment in timing; ordinary gathering upgrades and their guaranteed supplies must continue to work without it. Proposed stacking rule: identical precision parts refresh rather than multiply, and the Legendary effect reads the unmodified precision bonus so amplifiers cannot multiply one another.

**Interaction with the selected once-per-fight rule:** a precision enhancement improves the remaining manual grab on the next round; it does not grant or recharge a manual use. This supersedes the earlier suggestion that crafting one should schedule a fresh precision opportunity. For example, craft the enhancement on turn 2, then spend the saved manual grab on turn 3. After the grab is spent, any unconditional next-round supply still applies, but a perfect-timing condition cannot be met again that fight. Recipe cooldown removal does not restore the separate manual-grab allowance. Automatic/assisted treatment of the conditional bonus, exact costs/cooldowns and interactions between different named precision parts remain proposals. No extra-use exception has been selected.

### Guardrails for usefulness and repetition

- Baseline **composition** matters as well as quantity. Starting kits need a useful attack or defence route without precision success; this does not promise maximum damage and complete protection at once. Rare materials must have a route that does not require perfect timing.
- Automatic handling should retain material targeting and gathering-upgrade benefits. Calling a lower-paying mode optional does not by itself remove pressure to perform the minigame.
- One brief input, roughly one or two seconds as an initial presentation target; no multi-click power/accuracy chain, long score screen, retries to fish for a perfect haul, or accumulating perfect streak needed for ordinary combat. Show and hear the claw deliver the actual extra scrap, then continue planning.
- Vary the **useful resource decision**, not just marker speed or target position. A Copper shortage before a shield recipe makes precision matter more than an arbitrary score. Avoid resource offers that have one strictly better choice regardless of the build.
- Permanent gathering upgrades from shops or Officer loot should improve dependable yield, resource targeting or available choices. Existing unconditional next-round gathering recipes must apply on that next round with either automatic or manual handling, even when no precision opportunity occurs. Their promised supplies must not depend on perfect timing. The newly proposed precision recipes can explicitly add a conditional perfect-timing bonus; keep that condition separate from guaranteed supplies. All ordinary raw/part fight-end clearing remains unchanged unless the owner chooses otherwise.
- Include an automatic/no-timing option and readable visual feedback with sound as reinforcement. Decide later whether assists match an average result or another chosen reward level; do not accidentally make access to a material depend on input dexterity.

**Reference check:** Nintendo's [Super Mario RPG battle guide](https://www.nintendo.com/us/whatsnew/heres-all-you-need-to-know-about-battling-in-super-mario-rpg/), published 27 November 2023 and read 14 September 2026, describes timed button presses attached to existing battle actions and rewards for successful timing. The transferable idea is a short input inside an action already happening. Its combat rewards and timing chains do not demonstrate that repeated salvage will be fun here; avoid copying those chains simply to give gathering more systems. This was a text-source inspection, not a playtest or capture review.

**First playtest, proposed only:** test the selected one optional manual grab per fight, using automatic targeted collection as a comparison control. Keep expected rewards comparable and observe a 30–50-turn session, including ordinary and pressured rounds. Does precision change a crafting plan? Is it still chosen when it pays about the same on average? Does the player start skipping it, resent its delay, or feel forced to maximise it? Keep it only if the action itself remains enjoyable. Six precision recipes have been added as design content; no playable minigame, graphics or input implementation has been made.

**Next:** define the baseline material mix, automatic result, manual bonus bands and gathering-upgrade/next-round-boost mapping. The claw, baseline ingredient choice, Collect/Precision controls and once-per-fight manual frequency are settled; the precision marker's exact confirm/cancel behaviour remains open. The other prototype-readiness gaps below remain open.

## Prototype readiness review — 14 September 2026

**Assessment:** the core loop, scene sequence and visual direction are established. A working demo still needs a consistent round, a supply model, actual enemies and clear boundaries between fights. The 606 recipes provide candidate content; their introductory combat rules are explicitly proposals. Review those existing defaults rather than treating them as either approved rules or missing work. This assessment authorizes no gameplay implementation and selects none of the recommendations below.

### Decisions that affect the first playable loop

| Gap | What needs defining for the demo | Why it matters |
| --- | --- | --- |
| Gathering and supply | Baseline ingredient option contents/quantities; ordinary result; precision bonus bands and marker confirm/cancel rules; upgrade mapping | Claw, baseline ingredient choice, Collect/Precision, one ordinary haul per turn, one manual use per fight and intent before collection are selected; actual supplies remain to tune |
| Turn commitment | What Load commits and whether it can be undone; access to Recipe/Shop and End Turn / Defend after loading; non-Ammo activation; round-start ordering | Load then target then Fire is selected; Fire leads to enemy actions. End Turn / Defend handles no-shot rounds. Empty-bullet Load is blocked. These remaining edges must agree with the recipe effects |
| Preparation budget | Resource-only costs versus an additional global energy/action budget; starting resources; raw/finished-stock limits; hard shot/defence part caps or resource-only constraint | The catalogue assumes no extra energy, while earlier planning discusses it. Both views must not reach implementation as conflicting requirements. At least 20 parts per bullet and shield is required support, not an agreed cap |
| Combat resolution | Adopt or revise the catalogue's damage, spread, status, Shield-expiry and stacking rules; choose targets; define the point that ends a fight, including a final enemy dying during preparation or simultaneous deaths | Most arithmetic is already drafted. The missing step is choosing the demo's rule set and handling its end conditions consistently |
| Enemies and pressure | A small enemy roster with HP, Shield, moves, intent information, move selection, action order and target rules; basic player HP and a clear loss trigger | Defence, saved parts and cooldowns have no testable value without real threats. Enemy numbers can be tuned after behaviour is specified |
| Recipe memory and offers | Demo capacity and starter kit together; duplicate/stronger-version handling; shared or separate use limits/cooldowns; eligible reward recipes and memory replacement | The proposed 12-recipe starter kit is not an approved memory size. Duplicates must not accidentally bypass the intended craft limit |
| Fight boundaries and defeat | Player HP retention/recovery, cooldown and character-meter resets, loss/retry consequences, and minimal save/continue behaviour | Both unused consumable pools already clear at fight end. HP and campaign stakes remain open; a restart must not silently undo the intended cost of losing |
| Rewards, shop and permanent upgrades | A few concrete mayor/Officer/shop upgrades with installation and stacking rules; core sale values; prices; when stock refreshes; how purchased resources are staged for the next fight | The next fight must demonstrate an earned improvement. Bought stock needs an explicit handoff after old fight supplies clear; reopening a shop must not create an unintended free stock reroll |

**Selected since this review:** Load prepares the shot/shield, targets are selected, and Fire proceeds through impact to enemy actions. End Turn / Defend ends preparation without firing. **Other starting defaults remain proposals:** Start the first balance experiment with the catalogue's resource-only costs and explicit effect rules. Use the listed Shield expiry and fresh-fight cooldowns as provisional test rules if accepted. Choose a memory number and a matching kit before implementing rewards. These recommendations are not owner decisions or proof of balance.

### Presentation and technical preparation

Specify one playable preparation layout: distinguish raw stock, finished reserve, loaded Ammo, activated defence, recipe availability and the combined outcome. Decide which edits are reversible: unloading before Fire is different from refunding a completed craft or undoing an already activated helper command. Keep enemy intentions consultable while the full-screen recipe selector is open; target selection follows Load. The outcome preview and action sequence must describe the same result, including multi-enemy hits and delayed effects.

Use the selected rear claw and 16B/16C cameras, with a representative forge, robot and surroundings for one city. Test the required 20-part shot and 20-part shield readability with a prepared stress case. Choose a practical grouped or physical assembly presentation for this demo, and include magnetic, furnace, firing and impact feedback. The existing generated boards and Blender clip are useful references; actual interactive assets, input, camera behaviour and runtime quality still need proving.

At implementation start, assess the existing Windows/Unreal project and reusable pieces against this design, then record the actual demo build/run path. Preserve old demo saves. A playable demo needs start, pause, defeat, restart and a clear ending; choose whether its Continue resumes at fight boundaries or within a fight. Full settings/profile-management/Collection polish can follow, but any exposed menu entry must have honest working behaviour. This review neither changes the engine nor claims old gameplay is reusable.

### Proposed demo boundary and evidence

For the first playable experiment, propose **one complete mercenary, one city environment, about 30 selected recipes and a three-fight sample**. Begin with a city offer of three real upgrades, then a teaching fight, recipe reward and core-sale/shop visit. The next encounter choice should include an optional Officer; a follow-up fight must let the player use its earned upgrade/recipe. Include defeat and restart. Other mercenaries and the full 606-recipe pool remain part of the planned game. This is a labelled demo sample, not a replacement for the owner's 10–20-fight city wave or evidence of full-campaign balance. The content boundary remains a proposal; no development time limit is imposed.

The full upgrade/modifier catalogues, every mystery service, additional city environments, final campaign ending and profile unlock economy can wait for a later increment. The demo needs only the upgrades/effects it actually uses. Standalone recipe removal can be left out of this sample while its value remains unresolved. If the demo instead includes city completion or mystery events, settle boss-offer handling and whether noncombat mysteries advance the wave before building those scenes.

**Play question:** with comparable available resources but different enemy intentions, does the player change what they craft, save or load, and explain what they chose to give up? Check this in two contrasting encounters. If the same safe sequence wins both, or the player has no meaningful answer, revise the supply/threat relationship before adding more content. Also require an earned reward to change a later decision, a loss the player can explain, and a desire to try another combination.

**Main design risk:** unlimited within-fight stockpiling plus always-accessible recipes can favour safe stalling or the same efficient crafting sequence every round. This is a hypothesis to challenge with enemy pressure, comparable starting supplies and actual play, not a reason to impose an arbitrary turn timer or secretly change the selected resource rules. Technical checks should cover resource/part accounting, cooldown clearing, next-round boosts, memory replacement, purchased stock, fight resets and any enabled save path. They cannot establish fun.

**Next discussion:** settle supply quantities, Load/undo and non-Ammo activation timing, then write one complete example fight with starting stock, enemy intentions, attack/defence choices, damage, victory and defeat. Numeric tuning can change during prototype play; these structural rules need explicit temporary answers first.

## How we will work

### Mechanics and scenes Q&A — opening screen first

Known foundation: individual mercenaries/weapons and core recipes; 10–20-fight city waves; one claw collection per turn with Collect or the saved Precision attempt; player-assigned furnace crafting from limited recipe memory; cooldown recipes usable again when all counters are removed; recipes without a cooldown ability limited to once per turn unless a permanent upgrade says otherwise; resource/part retention between turns and clearing at fight end; game-long retained recipes/upgrades including memory expansion, fight-start and turn-timed supply grants; core/recipe rewards; Officers; between-fight shops; and mysteries. Preparation uses 16B and action uses 16C. Exact crafting/energy numbers, memory size, cooldown interactions, Load/undo, non-Ammo activation and new-game/profile persistence remain open.

Klaus wants to begin at the opening screen and work forward through the player journey. Title Screen leads through Start New Game to Choose Character. He then defined the recipe/part combat and reward loop below. Ask one main question at a time and preserve these answers. The latest prepare-screen requirement describes a single upcoming shot assembled from many parts. The later owner-defined sequence selects a separate End Turn / Defend action, Load before target selection, and Fire followed by surviving enemies' actions; it does not select multiple main shots.

Scene/function inventory: the owner has selected the following flow and functions. Exact screen composition, transitions and which functions share a screen are still design work.

| Player-facing scene or function | Current standing |
| --- | --- |
| Title screen | Owner-defined first version below; Start New Game leads to Choose Character |
| Choose Character | Confirmed next scene; each character has its own weapon, strengths/abilities and starting core recipes; roster and layout open |
| City arrival / mayor's offer | Before the city's wave, choose among three special weapon upgrades; an opening offer can include a legendary upgrade |
| Encounter choice | Three semi-random options, at least one Regular; Officer and Mystery each appear about one quarter of the time among the three; exact joint distribution and boss exceptions open |
| Preparation and assembly | Selected side view; claw/scrap gathering behind the player/gun, enemy intent visible; support at least 20 applied parts each in the bullet and shield; show accumulated effects, with physical parts optional; Recipe opens full screen with focus/Use/Close for resource-based crafting; claw and massive scrap pile; hard part caps remain undecided |
| Combat action | Selected perspective; firing, impacts and enemy response are moments within it, not automatically separate scenes |
| Salvage / analysis rewards | Take robot energy cores and analyze dropped items for recipes; normally choose one of three recipes or skip; Officer rewards also include a permanent system upgrade |
| Local shop | Accessible between fights and from the in-fight loadout; sells parts, resources, recipes and upgrades for credits; special items optional; Close returns to loadout |
| Mystery encounter | May resolve as combat, a special merchant or a tech-support encounter; these need their own interaction states |
| City boss / city cleared | End-of-wave boss is the last enemy in the city; clearing the wave leads to a more advanced city and a fresh opening upgrade offer |
| Victory and defeat | Fight ends when enemies are killed/fled or player dies; death shows Game Over with a gloating evil AI; fleeing rewards, restart/profile consequences and final campaign victory remain open |

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

Confirmed by Klaus, 13 September 2026: the first scene after Start New Game is **Choose Character**. Offer different mercenaries, each with its own weapon and individual strengths and abilities. A new game starts with the chosen character's basic gun and core recipes. The 14 September recipe assignment now specifies four mercenaries; their identities, exact abilities and starting lists are proposed in the catalogue, while final acceptance and character unlock/access rules remain open. The campaign begins in a fortified city, with an upgrade offer before its wave; whether an introductory sequence precedes that city interaction remains open.

### Main prepare screen — recorded requirements

**Owner-defined requirement, not a layout or implementation assignment:** each round's build can be different, layering abilities, strengths and enhancements into the **single upcoming shot** and its accompanying defence. The selected preparation view remains 16B. The interface must support **at least 20 applied parts per bullet and at least 20 applied parts per shield**, independently. This is a minimum interface requirement, not a selected 20-part cap or 20 shared slots across offence and defence.

**Part limit remains undecided:** Klaus may impose an explicit limit on shot/defence parts or let limited resources provide the constraint. Neither option is selected. Do not use the three illustrative components in the storyboard as a capacity limit.

**Build visibility and quality:** the player must be able to understand the accumulated strengths, abilities and enhancements of both shot and defence at these part counts. Klaus leaves two presentation approaches open: show their strengths/abilities through a clear effects presentation, or physically show added parts together with an effects summary. Both must look good in the selected graphics direction. Exact composition, grouping, interaction, geometry and summary design remain to choose; this requirement does not mandate 20 individually visible physical attachments.

**Recipe button:** open a full-screen view of recipe **cards** in memory with a small resource counter/meter. Select a recipe to focus it and show Use; Use crafts it by paying resources and adding the output to the parts pool. Greyed recipes remain inspectable. Close returns to loadout. This supersedes the earlier pop-up-only presentation. The player chooses recipes to craft based on available resources, following the established cooldown and once-per-turn rules. Card presentation is now explicitly requested for this selector; it does not add a random draw, deck or discard mechanic, or prescribe another game's visual design. Exact card layout remains open; full-screen focus, Use and Close behaviour are selected.

**Gathering scene — placement confirmed 14 September 2026:** in the side-on planning/loading screen, place the **claw and massive scrap pile behind the player/gun**, gathering from the rear. The gathering system must not occupy the space between the gun and enemies. This owner requirement governs over the different arrangement pictured in the style anchor and earlier storyboard. The magnet's yield and resource mix remain upgradeable as specified below. Exact framing and loading connections remain to design.

**Enemy intent during planning:** enemies must display indicators of their upcoming intentions during the planning phase, giving the player information that affects attack and defence choices for the current round. Slay the Spire 2 is the functional reference for this advance information; the visual design remains original and consistent with the style anchor. Exact intent types, symbols, numerical detail and update behaviour remain to specify. These are confirmed screen requirements, not a request for a new mock-up or gameplay implementation.

### Magnet upgrades and next-round enhancements

The current physical tool is a **claw**. This heading and older catalogue labels retain the legacy magnet terminology; their gathering-yield and next-round roles carry forward. The latest owner rule makes claw enhancements permanent upgrades activated on purchase or loot acceptance, and adds Officer acquisition. The owner reconfirmed temporary next-round gathering recipes alongside these permanent upgrades; their distinct routes are below.

The magnet is upgradeable alongside the weapon and defence systems. Its upgrades may **increase resources gathered per magnet run/haul** and **steer the catch toward one or more specific resource types** to support the player's build strategy. The existing baseline remains one magnet use per turn; improved yield does not imply additional activations. Exact resource bias, quantities and guarantees are not chosen.

**Updated acquisition:** permanent claw upgrades can now be bought or accepted from Officer loot. This supersedes the earlier shop-only/Officer-exclusion restriction. Mayor inclusion is not selected. Temporary next-round recipes remain a separate selected route; the current routes are:

| Route | Effect and duration |
| --- | --- |
| Shop-bought or Officer-loot claw upgrade | Buying or accepting activates its gathering upgrade immediately for the remainder of the current game; it is not a crafted part |
| Temporary gathering recipe | Craft and fit its temporary enhancement for next round only; retained alongside permanent upgrades, including the six precision recipes |

Temporary effects last for that one next round. Prices, recipe costs, effect strengths, stacking, exact application timing within the next round and what happens if the fight ends before that round remain to define. Permanent claw upgrades are bought or accepted from Officer loot, not crafted. Their exact effect definitions are later design work.

### Chosen world and city campaign

**Owner-defined premise:** you are a mercenary travelling between fortified cities in a dystopian world. An AI judged humans a threat and used robots to take over the world. Scattered fortified cities survive robot attacks with help from mercenaries wielding special weapons. Many mercenaries work for a city and several attacks occur at the same time, explaining the player's choice of assignments. This background does not itself specify multiplayer, companions or simulated allied battles.

**City visual identity, confirmed 14 September 2026:** every city has a **unique backdrop and surroundings**, while sharing the game's selected graphics style. The anchor's pictured scrapyard is not a requirement to reuse one environment across all cities. City names, environmental themes and particular landmarks remain open.

**Campaign scale:** a game progresses through cities; a city is cleared by defeating its wave of **10–20 fights**; each fight contains many turns. More advanced cities have often-better shops, more advanced recipes and harder/more advanced enemies. Progression must balance the player's growing build against those threats. City count, named locations, actual enemy rosters, total-game ending and difficulty curves remain open. The 10–20 range is owner-selected, not an implementation-time budget.

**Opening reward:** before each city's wave, the city/mayor offers a choice among **three special upgrades to the weapon**. These are the first rewards before the wave's fights and can sometimes include a legendary upgrade. System upgrades gained through the campaign are permanent for the current game, not merely this fight. Exact opening reward pools, probability, skip rules and equip/stack limits are not specified.

**Upgrade supply effects, explicitly allowed by Klaus:** permanent upgrades may provide starting raw resources or ready-made parts at the beginning of each fight, or extra raw resources/ready parts on a specified turn during a fight. These are fresh grants from retained systems, not leftovers carried across the fight boundary. They supplement the default magnet/furnace supply route; no extra magnet activation is implied. Exact trigger turns, quantities, part eligibility, repeat/stack rules and initial grant order remain to define per upgrade. A turn-timed effect is not automatically an every-turn effect.

**Before the next fight:** choose among **three semi-random offered encounters** with at least one Regular. Multiple Regulars may appear; Officer and Mystery each appear among the three approximately one quarter of the time. They are not a fixed one-of-each set. Joint probabilities and duplicate-special handling remain open. The city ends with an end-of-wave boss as its last enemy. The exact treatment of the final boss within the three-choice flow, and whether noncombat mystery encounters count toward the 10–20 fights, remain to clarify.

| Encounter | Challenge and rewards |
| --- | --- |
| Regular | Troops grow stronger with progression but remain weaker than comparable Officers. Recipe drops can be common, uncommon and sometimes rare; **never legendary** |
| Officer | Stronger enemy, often assisted by minions/troops. Victory gives a **permanent system-upgrade part in addition to a recipe reward**. Recipes are often uncommon/rare and sometimes legendary |
| Mystery | Can become a regular fight, special merchant or tech-support encounter; outcome probabilities and advance information are open |
| End-of-wave boss | Last enemy in the city. Can drop legendary rewards; the owner has not specified a guaranteed legendary, full reward table or boss selection UI |

After clearing the city/wave, move to a more advanced city and repeat its opening offer and wave. The scene sequence is thus character choice → city arrival/offer → encounter choice → fight or mystery → rewards → between-fight services/next choice → city boss/clear → next city. This describes functions; it is not a locked map layout or a requirement to visit the shop after every fight.

### Cores, credits and the between-fight shop

Defeated robots provide **energy cores**. Stronger robots' cores have greater sale value. Collect cores after victory and **sell them to the shop for credits**; they are not automatically the same thing as spendable credits or the fight's expendable crafting pool. Prices and core types remain to balance.

The local shop is accessible **between fights and from the main loadout during a fight**. The latter is now explicitly selected. It offers ready-made parts as well as the goods below; special items remain optional. Close returns to the loadout, and in-fight raw-resource/part purchases are available in that preparation. Stock varies, but reopening it is not yet specified as a restock trigger. It sells raw resources for recipes, whole recipes at higher prices, and permanent system upgrades of various strengths, including magnet upgrades acquired separately from weapon rewards. Legendary stock can sometimes appear. Offers are semi-random and balanced for the chosen character and current level/city; later cities often offer better goods alongside harder threats. Exact restock trigger, stock persistence during revisits, prices, sale restrictions and eligibility rules are not selected. Between-fight purchased resources must remain usable for the upcoming fight; the purchase/staging/reset order still needs definition rather than silently deleting those purchases.

### Recipe strength, rarity and services

Each recipe has a **name and strength**. The same functional recipe can exist at different strength levels and material costs. Owner's illustrative example: a strength-3 shield costs 1 iron + 2 copper; a strength-5 version might cost 2 iron + 2 copper. These are examples of improving efficiency, **not approved tuning**. Strength, rarity, material cost and the shop purchase price are distinct properties.

Rarity tiers are **common → uncommon → rare → legendary**. Common recipes are the weakest and most expensive relative to delivered strength. Progressively rarer recipes should become better/stronger, offer more abilities and give a better cost-to-strength ratio. This does not mean a legendary must always cost fewer absolute materials or fewer shop credits. Exact cost curves, effect budgets, offer probabilities, duplicate handling and how two strength versions coexist remain open.

Legendary sources named by Klaus are occasional Officer rewards, occasional shop stock, the mayor's opening wave reward and the end-of-wave boss. Regular troops cannot drop legendary recipes. Do not turn possible drops into guaranteed drops or silently remove Officer eligibility because other legendary sources were also listed.

Mystery merchants can sell better items. Mystery tech-support characters can offer to **transform recipes, improve/enhance recipes or add special functions** such as doubled output or additional defence/attack capability. Standalone recipe removal was proposed earlier and is now under review because it has no current advantage; see the memory rules below. They can also offer permanent upgrades in exchange for **health, another upgrade, or one or more recipes**. Availability, prices, exact sacrifices and whether health means current or maximum health remain open. These are optional offers the player may accept, not automatic losses.

### Recipe memory and cooldowns

**Owner-defined capacity:** the player can hold only a limited number of recipes in **recipe memory** at one time. The exact starting capacity is to be determined later. This is a hard limit: accepting or exchanging a recipe must never leave memory above capacity. When a new recipe is offered at full memory, the player may exchange an existing recipe for it or skip the offer. Permanent upgrades obtained as rewards or bought in shops can increase memory capacity; exact increases and stacking remain to balance.

**Availability each round — latest owner clarification:** all recipes in memory are accessible for selection without a random draw. Crafting requires sufficient resources and follows two rules:

- **Recipes with a cooldown ability:** active counters block crafting. When all cooldown counters have been removed, the recipe is usable again if the player can pay its resource cost, including again in the same turn. The earlier blanket once-per-round restriction does not block this reuse. Each craft activates the recipe's cooldown again; storing the crafted part does not delay activation until firing.
- **Recipes without a cooldown ability:** craft only once per turn, unless a **permanent upgrade** explicitly allows otherwise. The absence of counters does not permit repeated crafting of this recipe type.

A cooldown recipe temporarily at zero counters is still a recipe with a cooldown ability. Increased output from one craft and additional permitted crafting uses are separate effects. This clarification supersedes the earlier blanket limit and its general enhancement/special-ability exception; for recipes without cooldown, the owner now specifies a permanent upgrade as the exception.

**Cooldown example, updated by the owner:** crafting a recipe with 2 counters in round 4 gives 2 immediately; the end of round 4 reduces it to 1, and the end of round 5 reduces it to 0, making it ready in round 6. Fresh counters receive the same-round enemy-end tick. This supersedes the earlier round-7 example. Two rounds is an illustrative cooldown, not a universal duration for strong recipes. **An active cooldown blocks crafting while counters remain.** Removing all counters makes the recipe usable again immediately if its resource cost can be paid. The cooldown restricts crafting that recipe; the existing rule permitting storage of crafted parts still applies.

**Cooldown-counter effects:** other recipes may provide an ability that removes a cooldown counter. Partial removal shortens cooldown but leaves the recipe blocked while any counters remain. Removing the last counter restores crafting availability immediately, including within the same turn, provided the player has the resources. For example, craft a cooldown recipe, remove all of its counters through an effect, then pay to craft it again in that turn; the second craft activates its cooldown again. Exact effect targets, how the counter-removal effect is activated and removal amounts remain to define. The round-4-to-round-6 example above describes normal countdown without extra cooling effects. Every cooling effect removes its X from all currently cooling recipes; no target selection or distribution of X is needed.

Recipe memory is the current game's active crafting set. The profile's Collection records discoveries; discovery does not itself place every discovered recipe in memory or grant future-game access. Retained recipes persist across fights/cities, subject to exchanges and accepted recipe-sacrifice trades; standalone removal remains under review. Whether a removed/replaced recipe can be recovered without finding another offer remains open.

**Owner correction — standalone removal has no current advantage:** the player can ignore unwanted recipes and replace one when accepting a new offer at full memory. Empty memory has no selected gameplay benefit. Klaus leaves two possibilities open: design a worthwhile benefit for free memory, or drop the standalone remove-recipe service from shops/tech support. Neither outcome is selected yet. Do not invent a memory bonus, draw/discard system or penalty for unused recipes. Sacrificing a recipe in exchange for a permanent upgrade remains a distinct offer with an actual reward.

**Still open:** the numerical capacity and upgrade increments; duplicate/strength-version memory, use-limit and cooldown behaviour; exact cooldown-counter effects; cooldown handling at fight boundaries; and whether to retain standalone removal.

### Persistence boundaries

| Boundary | Confirmed behaviour |
| --- | --- |
| Next turn in the same fight | Unused raw resources stay in the resource pool; unused crafted parts also remain available |
| End of fight | **Clear both unused raw crafting resources and unused crafted parts.** Neither stockpile carries into the next fight |
| Start of the next fight / a specified turn | Retained upgrades can grant fresh starting resources/parts or extra supplies on specified turns; exact amounts and triggers are upgrade-specific and still to define |
| Later fights and cities in this game | Retained recipes and permanent weapon-system upgrades, including memory-capacity upgrades, carry through the **whole game**. Recipe exchanges and accepted sacrifices still apply; standalone removal and cooldown handling at fight boundaries are open. Cores/credits support between-fight trading; exact economy reset at a new game is not specified |
| New game / active profile | Start with the chosen character's basic gun and core recipes as previously selected. Collection discovery, unlocks, loss consequences and other cross-game persistence still need definition |

The phrase permanent upgrade refers to the current game/campaign; it is not yet a permanent statistical upgrade to all future games in the profile.

### Owner-defined recipe and part system

Games contain **fights**, and each fight contains **turns**. The reference is Slay the Spire 2's basic strategic functions, synergy and balancing approach, not its visual/card design. Build the game's own framework and graphics. The earlier reference study remains evidence of observed functions, not proof that its exact tuning will transfer.

| Term | Role in this game |
| --- | --- |
| Source materials / scrap | Ingredients physically gathered with the claw; compatible with many recipes, allocated by the player for furnace crafting, with unused resources retained between turns |
| Recipe | Knowledge stored in limited recipe memory that allows production of a part when affordable, off cooldown and within its permitted uses this round; recipes take the build/reward role of cards and appear in the prepare screen's full-screen recipe view; crafted parts are the physical ammunition/defence ingredients |
| Recipe memory | The limited active recipe set; choose any available stored recipe each round, exchange recipes within the hard limit, and expand capacity through permanent upgrades |
| Ammo part | Contributes damage, delivery or an attack ability to the assembled shot; translates offensive card functions |
| Defence part | Contributes to the current round's defence, including shields; translates defensive skill functions |
| Modifier part | Adds or changes functions on offence and/or defence; translates other skill functions |
| Buff / debuff | Temporary beneficial effects for this fight / temporary negative effects applied to enemies; detailed effect and expiry rules still need specification |

**Turn supply and assembly, confirmed direction:**

1. Choose baseline ingredient options, then use Collect or the saved Precision attempt with the claw **once per turn** to collect an as-yet unspecified amount of source materials. They contain the ingredients for different owned recipes.
2. The gathered materials are compatible with **many recipes**. The player **assigns resources to an affordable, available recipe in memory**, crafting the parts needed most now or preparing a stock for later. Cooldown recipes are available at zero counters and may be reused in the same turn after all counters are removed. Recipes without a cooldown ability allow one craft per turn unless a permanent upgrade says otherwise. Crafting activates the recipe's cooldown if it has one. The furnace does not automatically decide the crafted batch. Recipe costs, ingredient ratios, part yields, specific enhancement tuning remain open; the full-screen Recipe/focus/Use/Close flow is selected.
3. Click parts to stage the shot and defence, press Load, select permitted targets, then Fire; or use End Turn / Defend to prepare defence without firing. Combine parts to increase damage, broaden targets or add interacting functions. Firing spends the parts assembled into the shot; exact activation/consumption timing for other parts remains to define.
4. **Keep both unused raw resources and unused finished parts for later turns within the fight.** The player may save materials because they cannot or do not want to spend them now, and may craft ahead without using the output immediately. These are separate holdings, not an automatic conversion of all scrap at turn end. The default once-per-turn gathering rule supersedes earlier proposals about paying for several hauls in one turn.

Owner's example: retain a good damage-multiplier part from the previous turn; obtain a multi-enemy-shot part this turn; combine them so one shot deals increased damage to several enemies. This establishes intended cross-turn planning and compositional synergy, not specific damage values or multiplication order.

Requested effect directions include ordinary damage enhancers, fire, explosive tips, shield demolition, shields, fight-duration buffs and enemy debuffs. Klaus expects many combinations and further iteration; the final catalogue, exact interaction rules and numbers are open. These examples do not imply that all parts are equally available to every character.

**After each fight:** collect robot energy cores for sale and analyze dropped items for recipe rewards, normally **choosing one of three recipes or skipping all three**. The choice should fit or redirect the current build. Accept a recipe into memory if there is room, or exchange an existing recipe when full; never exceed capacity. Retained recipes remain usable in later fights/cities of the current game, subject to their crafting costs and the availability rules for recipes with and without cooldown abilities. Officers also give a permanent system upgrade. Encounter rarity rules and shops are specified above. Each new game still starts with the chosen character's core recipes and basic gun. Detailed profile-Collection records, whether unchosen offers count as discoveries, and whether discovery changes future-game availability remain open.

**Still open:** ingredient/output counts and costs; recipe strength/rarity tuning, duplicates and reward pools; memory size/upgrade increments, cooldown-counter effect details and cooldown handling at fight boundaries; energy/activation rules; Load/undo and non-Ammo activation timing; shield expiry/effects; raw-resource/part storage limits; upgrade supply triggers/amounts; retaining or dropping standalone removal and any possible benefit for free memory; shop purchase staging/restock rules; mystery counting and final-boss choice; and profile progression/campaign ending. No discard/reshuffle system, fixed starter-kit size or numerical balance is selected by the reference analogy.

**Full redesign; implementation not started.** Klaus reconfirmed that little of the old game's code may be usable. Design the new game on its own requirements, and assess old code for reuse only where it fits. Existing prototype code and its recorded stage describe the old design. Storyboards, the review page and standalone Blender camera tests are design/art tools; they are not a partially implemented redesign or a commitment to its code architecture.

You review and revise this plan as the direction evolves, and set a goal for the work you want to pursue. We complete that goal, give you something concrete to judge, and revise it until you are satisfied. We begin another implementation step only when you set its goal. A step can be split into smaller goals if useful. The current art goal is already authorized; no development timetable is imposed.

## Current instructions and preferences

| Area | What carries forward |
| --- | --- |
| Game concept | Games contain fights and turns; characters have individual weapons/abilities and core recipes. Magnet haul → furnace → physical parts → shot/defence. Slay the Spire informs strategic functions and balance; build original framework/graphics |
| Recipe memory | Hard recipe-count limit, exact size later; cooldown recipes may be crafted again at zero counters if affordable; recipes without cooldown allow one craft per turn unless a permanent upgrade says otherwise; crafting starts applicable cooldowns and other recipes may remove counters; exchange recipes at capacity; permanent upgrades can expand memory |
| Preparation view | 16B side view with rear scrap gathering and visible enemy intent guiding current-round attack/defence; minimum support for 20 parts per bullet and 20 per shield, accumulated effects visible, physical-part display optional; Recipe Memory button opens recipe cards; hard part limits and exact layout remain open |
| Action view and look | 16C is the owner's preferred graphical treatment and 3D perspective for the shot and impact. Explore enemy responses in 3D too. Keep the same visual identity across views |
| Reference asset | [art anchor.png](art%20anchor.png) is the owner-selected STYLE anchor, not gameplay images; bottom 16C anchors attack perspective. It matches board 16 pixels. Rear gathering placement, intent indicators and unique city surroundings are separate owner requirements |
| Gathering claw | Claw and massive scrap pile behind the player/gun, outside the space between gun and enemies; baseline ingredient choice and Collect/Precision, once per turn gathering for the furnace; shop/Officer claw upgrades activate on acceptance and improve yield and/or resource bias; special recipes enhance only the next round |
| Shot assembly | Build ammunition from physical components in a linear stack/loading bar; components add properties or combine functions. Firing spends its component material. The pictured rack of completed bullets does not replace this system |
| Strategic choices | Shared resources must create meaningful choices between offence, defence and preparation. An additional global energy/action budget remains undecided; the recipe draft assumes none. Support focused and multiple-target attacks and scrap-built protection |
| Build variety | After each fight, search attackers' rubble and choose one of three recipe rewards or skip. Combine ammo/defence/modifier parts and retain unused parts across turns to form useful synergies |
| Progress and stakes | Meaningful encounter rewards, intermediate objectives, clear stage completion, substantial obstacles and a final confrontation. Winning must matter, losing must be possible, and another run should offer different strategies |
| Upgrade quality | Enable something useful or an exciting new tactic. Merely gathering the same permitted load faster was rejected as sufficient justification for an upgrade |
| Clarity | Recognisable objects, visibly connected mechanisms, familiar words, clear goals and consequences, restrained UI and an interactive tutorial. The old jargon-filled, overcrowded board is not the target |
| Feedback and audio | Component loading should build anticipation into discharge and readable effects. Strong magnetic/impact sounds and suitable soothing background music are requested; tracks and exact sound direction remain open |
| Scope and review | Keep the game understandable and manageable for a solo owner; do not add huge lore or an excessive catalogue of systems. Development timelines are explicitly not the design constraint. Gameplay critique concerns fun, fairness and comprehension, not schedules |
| Delivery process | Klaus sets implementation goals; complete and revise each until he is satisfied. A playable concept demo must be judged before full development. Functional tests alone do not establish fun |

## Proposals and decisions still open

- **Persistence:** both raw resources and crafted parts remain between turns and clear at fight end. Retained recipes and permanent system upgrades persist across fights/cities, and upgrades may expand recipe memory or grant fresh fight-start or turn-timed supplies. New-game starter kit remains character-specific; profile discovery/unlocks, cooldown reset, memory size and raw-resource/part storage limits are open.
- **Combat rules:** exact energy and haul costs, carrying limits and risky-pull consequences; the relationship between Fire and End Turn; number of actions/shots; targeting and multi-target distribution; armour duration; effect order; and damage values. Poison, piercing, shock, explosions and other effects are possibilities, not an approved full catalogue.
- **World and progression:** the mercenary/AI-robot takeover and fortified-city campaign are now chosen. Character/city identities, total cities, exact bosses, mystery counting, three-choice handling at the boss, final campaign victory, defeat/retry consequences and profile unlocks remain open. The five older premises are archived alternatives, not a pending prerequisite.
- **Visual production:** refine the 16B/16C treatment into consistent objects, UI and motion. Exact character, materials, physical-part versus effects presentation and camera transitions remain open. Preparation must support at least 20 applied parts each in the bullet and shield; a hard part cap is not selected. The first shared-geometry motion test is delivered; matching the reference's finish in actual assets remains unproved.
- **Product details:** exact controls, content scope, release configuration and pricing remain open. The commercial aim is a worthwhile solo project, with a stated target of at least NOK 20,000 profit per game and further games if profitability is demonstrated.

The old four-site recovery structure, safe unlimited hauling, equipment catalogue and earlier implementation hypotheses are historical material. They are not automatically retained in this combat redesign. [DECISIONS.md](../DECISIONS.md) preserves the chronology and superseded choices.

## 1. Decide who we are and why the work matters

**Owner premise now selected:** a mercenary travels between fortified cities in an AI/robot-dominated dystopian world, helping defeat waves of robot attacks and developing a weapon/recipe build. See the chosen city campaign above. The [five earlier premise options](PREMISE-OPTIONS.md) are archived alternatives; do not ask Klaus to choose one before continuing.

Refine the chosen premise's character/city identities and final campaign objective. City defence provides the intermediate goal: clear a wave and travel to a more advanced city. Keep the backstory compact enough to communicate through the opening, environment and progress rather than a lore manual.

**What remains to define:** how the chosen world is introduced and what completing the whole campaign changes, beyond clearing individual cities.

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

**Next action: resolve the surfaced activation conflicts and Load reversibility before further turn-flow specification.** Preserve the explicit style-versus-gameplay distinction, 16C attack anchor, rear gathering position, enemy-intent indicators and unique city backdrops/surroundings. Preserve the minimum 20 parts per bullet and per shield, unresolved hard-cap versus resource-only balance, open effects-only versus physical-part-plus-summary presentation, full-screen recipe focus/Use/Close and separate gathering progression. No layout production or gameplay implementation is requested by this increment. Later questions include fight-boundary cooldown reset, counter-removal effect activation/targets, magnet-effect stacking and fight-end timing, mystery counting and boss-choice handling. Standalone removal remains under review; exact costs, profile progression and campaign ending remain open.
