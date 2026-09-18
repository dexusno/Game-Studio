# Overkill Foundry — balance research and proposed test method

13 September 2026. Requested by Klaus during the stacked-ammunition discussion. This records developer documentation and a proposed method for our game. It does not establish tuned values or authorize implementation. Companion: [stacked ammunition and build strategies](BUILD-STRATEGY-STUDY.md).

## Graphics-free balancing version — owner direction, 18 September 2026

Build a graphics-free version of the game for Codex to play and run hundreds of games in fast succession. Use it for encounter, economy and full-campaign balance, including all four mercenaries, three-city routes, permanent upgrades, Lockdown tiers and the final challenge as those systems become implemented. This is now a planned development deliverable. No simulator or automated campaign results exist yet; the existing Python reports inspect printed costs and design data only.

The owner explicitly also requires **human tests for graphics verification and approval**. Automated balance evidence does not approve graphics or replace human playability tests. Klaus must review the rendered game and approve its visual result, including the selected style, camera flow, animations, effects and readable combat feedback.

### One rules implementation, two ways to play

Implement a shared, presentation-independent rules core used by both the graphical game and the graphics-free runner. Share content definitions, legal-action validation, whole-number calculations, event order, enemy AI, rewards and save/seed handling. Preserve all scoped character/recipe/upgrade exceptions. Avoid maintaining a simplified second combat engine whose values or timing can drift from the playable version.

The graphical game translates player input into commands and displays resulting events. The runner accepts the same commands through a text/structured interface: inspect observable state and legal actions, choose an encounter or offer, steer/collect, Use recipes, install/unload parts, target, Fire, buy/sell and End Turn. It resolves turns without rendering, audio, camera transitions, animation waits or real-time presentation delays. Use simulation steps rather than merely speeding up the frame clock. Rendering must not be responsible for applying damage or advancing cooldowns.

Codex can directly play selected runs through this interface, inspect why a choice succeeded or failed, and orchestrate repeatable batches. Fast automated player policies handle bulk runs; an LLM call for every action is not required. Compare aggressive, defensive, resource-saving, synergy-focused and deliberately imperfect policies, with bounded lookahead where useful. Agents receive the information a player is entitled to see, not future random draws, concealed offers or the seed as a planning oracle. Record policy identity, version and search budget. Random actions are useful for bug finding, not evidence of human difficulty.

Collection skill needs an explicit boundary: full combat/economy rules stay shared, while fast batches may supply recorded haul outcomes or a labelled Precision/physical-claw success model. Compare low, medium and high execution skill rather than assuming perfect collection. Calibrate those inputs from the rendered interaction later. Such batches are conditional on the stated collection model; they do not establish that claw physics, timing or controls feel good. Where collection physics itself affects balance, include separate headless fixed-step integration checks and rendered human tests.

### Batch experiments and reproducible reports

Start with known encounter fixtures, then short routes, then complete three-city campaigns with persistent HP and actual reward/shop decisions. Advance to hundreds of runs per experiment once legality, event ordering and deterministic replay agree with the playable rules. Begin the wider matrix with each mercenary at Tier 0, 1, 5 and 10, then cover intermediate tiers. Sample normal eligible builds; label forced rare combinations and guaranteed healing as targeted stress fixtures rather than ordinary play.

For each run preserve the build/content/rules versions, seed, collection model, player policy, starting state, ordered action/event trace and final outcome. Use separately seeded random streams for encounters, rewards and supplies where appropriate, so an unrelated extra draw does not accidentally scramble every later comparison. Replaying the same version, initial state and command trace must reproduce the same state. Retain failing/extreme traces for a readable turn-by-turn review and eventual visual replay. Keep bulk logs in ignored local output directories and commit compact evidence summaries.

Compare proposed changes on the same seed sets and player policies, then check fresh held-out seeds and other policies. Split reports by mercenary, tier, encounter type and strategy; hundreds pooled across many categories are not hundreds of observations for each category. Include sample sizes and uncertainty with win rates. Investigate why a policy loses before weakening an enemy, and why an item is selected before declaring it too strong. Do not tune exclusively against the bot used to discover a problem.

Record city/campaign completion, loss causes, HP attrition and healing access, turns per fight, material shortages and leftovers, recipe/upgrade offer-pick-use rates, damage/Shield effectiveness, shop economy and growth loops. Distinguish an earned powerful build from an illegal action or a nonterminating loop. A runner watchdog may halt a suspected loop or excessive search, but record that result as unresolved/timeout rather than a loss; it must not add an in-game shot or turn cap. Report simulated turns separately from machine throughput: wall-clock simulation speed is not human campaign duration.

Propose focused balance changes, rerun affected comparisons and preserve the before/after report. Running a batch does not authorize rewriting settled game rules. Measure actual completed campaigns per minute and resource use before making a speed claim. The requirement is rapid batches of hundreds; no seconds-per-campaign performance is claimed before measurement.

### Implementation sequence and acceptance

1. Extract the shared rules core as part of the first combat implementation, with a text action interface and reproducible single-encounter traces. Keep presentation adapters thin; choose the concrete build target during implementation.
2. Verify graphical/headless agreement after each command for representative Shield reset/retention, cooldown clearing, character bonuses, recoil death, sales, upgrades and same-seed restart. Reuse recorded collection outcomes when isolating combat parity.
3. Add automated player policies and full route/reward/shop decisions. Explicitly list unsupported mechanics; do not run incomplete full-game content with silently ignored effects.
4. Produce a first several-hundred-run report with declared coverage, policy/seed versions, invalid/timeout counts, throughput and selected replayable cases. Test the modified settings on fresh seeds before treating a result as general.
5. Have humans test the rendered build for decisions, pacing, understandable losses, controls and enjoyment. Klaus verifies graphics and approves the visual result in motion: style fidelity, city identity, both camera views, animation/effect readability and larger resource-permitted builds. Numeric reports, screenshot checks and successful headless tests cannot substitute for that approval.

Unreal's current documentation lists `nullrhi` for running without a rendering interface, and its [Automation Test Framework](https://dev.epicgames.com/documentation/en-us/unreal-engine/automation-test-framework-in-unreal-engine) supports automated checks. These are available implementation tools, not an already-built balance runner or a guarantee of speed/determinism. Source checked 18 September 2026: [Epic command-line reference](https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-command-line-arguments-reference). The shared-core architecture and experimental method above are our design recommendations.

## HP and enemy benchmarks from STS2 — 16 September 2026

**Owner direction:** use Slay the Spire 2's HP/enemy balance as experience for our initial values. Klaus points out that comparable robot balancing would let us draw on its player-HP balance. This selects a reference approach, not a wholesale numerical copy or a change to our recovery rules. The current [robot library](ENEMY-ROSTER.md) adapts tactical behaviours; its printed HP, damage and effects are our provisional values, not already matched STS2 balance.

**Proposed first Mara test fixture: 80 starting HP and 80 maximum HP before Mayor or other upgrades.** Start the controlled test at 80/80; an explicit maximum-HP upgrade still adds its stated amount and heals that amount. This is a usable provisional input for demo studies, not owner-selected final HP or a claim of character parity. Ivo, Ada and Noor's final HP remain to tune. Keep the existing proposed ten-material haul as a separately labelled test input; neither proposal becomes a new owner decision merely by being used in a paper test.

**Inherent-ability update, later 16 September:** the [Quench Recovery proposal](MERCENARY-ABILITIES.md#mara--quench-recovery) lets Mara recover up to 6 actual HP per fight through qualifying Heat payments, at most 2 per player turn. Quench Recovery is an unselected alternative to the retained Hot Barrel after the 18 September correction. If testing that alternative in the 80-HP fixture, include its actual use; do not count the full allowance when the route never spends Heat or heals at full HP. It supplies no automatic victory/city healing. The other mercenaries' proposed traits affect targeting, Bolt repair or Charge rather than directly healing the player.

### Verified reference and its limits

Accessed 16 September 2026 through indexed STS2 wiki extracts. These are current retrieved descriptions, not an inspected executable or a verified complete balance model. Use the ordinary single-player, non-Ascension reference when comparing values; do not mix higher-difficulty stats into the same table.

| Source observation | Implication for our initial balance |
| --- | --- |
| [Ironclad](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Ironclad) starts with 80 maximum HP at ordinary difficulty; its starting relic heals 6 HP at combat end. | 80 is a concrete reference-scale candidate, not an isolated proof that an 80-HP mercenary can survive the same enemy sequence. Mara has no selected equivalent automatic heal. |
| [Rest Sites](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Rest_Sites) offer healing of 30% maximum HP, rounded down, and a rest site precedes the boss. | At 80 maximum HP one chosen Rest can restore up to 24 HP, limited by missing HP. Choosing an upgrade instead grants no Rest healing. Our routes do not include this recovery by default. |
| [Map Locations](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Map_Locations) describes Ancient entry healing of all missing HP at ordinary difficulty. | Cross-act recovery changes the campaign's available HP substantially. Our Mayors grant their selected upgrade; they do not automatically restore missing HP. |

Illustration only: losing 10 HP during a fight and then receiving an effective 6-HP heal is a net loss of 4. The same 10 damage without that heal costs 10. Neither amount is an observed average STS2 fight loss. Actual healing is capped by missing HP, so nominal healing cannot simply be added to a campaign budget without checking timing.

### How to transfer the useful balance

For each selected demo formation, pair it with a comparable STS2 role from the existing source research and record the source difficulty/date, HP, attack sequence, hit counts, defence and support effects. Enemy figures can seed a comparison fixture, but a new effect such as our per-hit Armor must be evaluated as its actual mechanic rather than assumed equivalent to a source buff.

Compare four outcomes using our real starter recipes and material costs:

1. **Turns to remove each threat:** effective affordable part damage against its defences, with targeting, saved parts and preparation turns. Total enemy HP alone is insufficient.
2. **Defensive burden:** damage and status pressure remaining after plausible Shield preparation and early kills. All attacks in a formation must be evaluated together.
3. **Loss from a mistake:** damage when a player misses a charge warning or keeps the wrong target alive, expressed against the provisional 80 HP. Do not assume the demonstrated zero-loss worked route is typical play.
4. **Whole-city attrition:** actual HP carried across fights, including only healing recipes/upgrades acquired and paid for in that route. A random healing offer is not guaranteed recovery.

Our full recipe memory is available subject to costs/use limits, supplies and unactivated parts can be saved within fights, and firing has no separate energy budget. These change achievable damage and defence compared with source deck/energy constraints. Matching source enemy stats is therefore a starting experiment; matching the resulting player choices and survival pressure requires costed fights and playtests.

The next concrete comparison is Mara's selected demo encounters at the proposed 80 HP and ten-material haul, including a simpler Regular path and an Officer-taking path. Record healing access explicitly, then adjust enemy HP, attack cadence/damage, supply or the HP fixture according to the observed cause. Preserve the owner's no-automatic-healing rule, End-Turn-only prepared Shield, whole-number damage and uncapped resource-permitted shots. This pass supplies a benchmark and test input; it does not change robot/recipe rows, import a healing system or produce a combat-balance result.

## Current beta haul recommendation — 15 September 2026

**Recommendation for the first beta supply profile: 10 materials per normal haul in total — an 8-material foundation plus 2 materials directed by the existing steering choice.** The earlier 8+2 illustration was not a selected balance value; it now has a recipe-cost basis. These are proposed beta numbers, not an owner-approved economy or a tested result. Do not reopen whether gathering has steering.

Klaus requested this cost analysis on 15 September. The recorded rules already select baseline ingredient choices, one Collect/Precision collection per round, one optional Precision attempt per fight, material/part carryover within fights, and clearing both at fight end. There is no separate action-point/energy budget and no reserve or bullet/shield part-count cap. Recipe costs, use limits and cooldowns still apply; all cooldowns start clear in each fight. Memory begins at 20 recipes, with 12 starters: 8 shared and 4 character-specific. These rules determine what supply must support.

### What was measured

The [reproducible Python audit](analysis/resource_haul.py) parses all 606 recipe rows in the [catalogue](RECIPE-CATALOGUE.md), validates the five-material cost syntax and compares each selected starter kit. Run `python games/overkill-foundry/design/analysis/resource_haul.py` from the repository root for JSON results. It changes no files. Recipe-row SHA-256 at this analysis: `87f652df4b372d5a48b54ee170f6ed9dfc5fb4895213e24811c202677bcba4ee`.

This is **printed material cost analysis**, including the retained costs of recipes flagged for revision. It does not treat those recipes as implementation-ready. Equal weight per recipe is a descriptive inventory measure, not reward frequency, player usage or equivalent material value. Extra Heat/Charge/HP costs, sacrifices, effect legality, resource refunds, ongoing effects, damage and enemies are outside the calculation. No gameplay or win-rate simulation was run. The later 15 September owner decision permits selling unused crafted parts for half their main recipe's normal one-part ingredient value at current shop resource prices, rounded down. Rebates or discounts on the actual producing recipe do not reduce sale value; each output uses its own part's standard reference instead of a share of the actual batch cost. This cost-only audit does not model credits, part sales or their effect on supplies over a campaign; include those transactions in later economy tests. Its material-cost statistics remain unchanged. The owner subsequently confirms that copied parts retain the original's sale value. Later economy tests must therefore include discounted/rebated production, extra outputs, copying parts and selling them; the original cost-only starter probe does not measure credits generated by those chains.

| Recipe tier | Rows | Average materials per use | Range |
| --- | ---: | ---: | ---: |
| Base | 24 | 1.67 | 1–3 |
| Common | 176 | 2.36 | 1–3 |
| Uncommon | 200 | 3.17 | 2–4 |
| Rare | 140 | 4.12 | 3–5 |
| Legendary | 66 | 5.24 | 2–6 |

Overall, the mean is **3.32** and the median is **3**. The full distribution is 12 recipes costing 1, 143 costing 2, 206 costing 3, 157 costing 4, 59 costing 5 and 29 costing 6. Ammo and Shield both average **3.35**, while Utilities average **3.32**: Utilities need a real share of the haul. These figures describe separate printed uses, not the cost of a complete shot or turn.

Each selected starter kit averages **1.75 materials per recipe**, except Ada's **1.83**. Crafting every starter once would cost 21 materials, or 22 for Ada, before any effect grants. A 10-material haul therefore supports a selection of the kit, with early common/uncommon discoveries asking roughly 2–3 materials per use. Do not divide ten by the whole catalogue's average and claim a guaranteed number of useful actions.

### Mix matters as much as total supply

Across all catalogue costs, Iron accounts for 36.4% of units and Copper 26.6%; Carbon, Glass and Circuit account for 10.2%, 13.8% and 13.0%. **236 of 606 recipes require Circuit**, so making it inaccessible until late would strand many offers. Those are draft cost frequencies, not spawn probabilities.

The following includes the 126 shared recipes plus each character's 120 exclusive recipes, counted once each:

| Eligible character pool | Iron | Copper | Carbon | Glass | Circuit |
| --- | ---: | ---: | ---: | ---: | ---: |
| Mara | 39.1% | 20.3% | 14.4% | 13.8% | 12.5% |
| Ivo | 32.4% | 21.3% | 11.6% | 19.6% | 15.1% |
| Ada | 40.5% | 23.6% | 8.4% | 13.9% | 13.5% |
| Noor | 31.2% | 32.7% | 5.2% | 16.4% | 14.5% |

Mara's pool asks for more Carbon than Noor's; Ivo's asks for more Glass; Ada's leans heavily on Iron; Noor's on Copper. This supports the **already-selected steering choice**. A single average mix should not be expected to serve every build equally. Actual demand depends on the player's current 12–20 recipes and choices, not all 246 eligible recipes at once.

For a reproducible beta test, use this proposed foundation:

| Portion of one normal haul | Proposed materials | Total |
| --- | --- | ---: |
| Baseline | 3 Iron, 2 Copper, 1 Carbon, 1 Glass, 1 Circuit | 8 |
| Steering allocation | 2 more of the material selected through the existing steering choice | 2 |
| Normal haul before upgrades | Baseline plus steering | **10** |

This is a concrete test mapping of steering, not a claim that its exact +2 formula or offer contents were already approved. The fixed basket is a controlled starting fixture; any eventual variety in the available ingredient choices must be measured against the same affordability checks. The ten units are **one haul**, not ten plus another automatic bonus. Precision and permanent/temporary gathering modifiers remain separate; their exact bonus bands are not selected here. Establish the ordinary supply first so a precision success is not required to make the starter kit function. No additional fight-entry stash is assumed by this opening-haul probe.

### What those resources buy

These examples use distinct no-cooldown shared recipes once each; all costs are paid. Damage below is **supplied by parts**, before target defences and other modifiers. The later owner decision on 15 September confirms zero base gun damage, so these part-only totals remain unchanged. Prepared Shield activates only at End Turn.

| Package | Exact recipes | Material cost | Draft effect before other modifiers |
| --- | --- | --- | --- |
| Basic attack and defence | SH001 Solid Casting + SH003 Powder Packing + SH002 Flat Plate | 2 Iron + 1 Copper + 1 Carbon = **4** | 9 added shot damage and 6 Shield at End Turn |
| Stronger shared attack and defence | Basic package + SH004 Simple Sighting + SH006 Basic Insulation | 3 Iron + 2 Copper + 1 Carbon + 2 Glass = **8** | 13 added damage, or 17 against an attacking main target; 10 Shield at End Turn, plus insulation's conditional benefit |
| Attack, defence and future supply | Basic package + SH004 + SH008 Extra Lift | 3 Iron + 2 Copper + 1 Carbon + 1 Glass + 1 Circuit = **8** | Same added shot damage as the stronger package, 6 Shield, and the draft next-round haul boost if its pending fitting/timing requirements are met |

The proposed 10-material haul can afford the stronger shared package when steering toward Glass. Its two leftover units are Glass and Circuit; that is **not** two freely interchangeable materials. They cannot immediately pay for another Flat Plate or repeat an already-used no-cooldown recipe. Choosing the haul investment trades away immediate protection; its exact value needs the content issue below resolved.

This is the useful budget tension: ordinary attack plus some defence remains possible, while a stronger attack, more defence, cooling, character setup and saving for later compete for the remaining compatible materials. Cost alone cannot establish whether 6 or 10 Shield is sufficient against the eventual enemy intents.

### Sensitivity check across all four starter kits

For each candidate below, enumerate all 4,096 subsets of each 12-recipe starter kit and each of five possible steering materials. A subset includes each recipe at most once. The entries report the **maximum number of distinct printed recipe costs affordable**, with the range taken over steering choices. They are not recommended action counts or proof that the effects can all activate meaningfully.

Each candidate adds **2 of one chosen material** to the listed foundation; the first column includes those two units.

| Normal haul total | Fixed foundation before steering | Mara | Ivo | Ada | Noor |
| --- | --- | ---: | ---: | ---: | ---: |
| 6 | 1 Iron, 1 Copper, 1 Carbon, 1 Glass | 3–4 | 4 | 3–4 | 3–5 |
| 8 | 2 Iron, 2 Copper, 1 Carbon, 1 Glass | 5–6 | 5–6 | 4–5 | 5–6 |
| 10 | 3 Iron, 2 Copper, 1 Carbon, 1 Glass, 1 Circuit | 5–7 | 5–7 | 5–6 | 5–7 |
| 12 | 4 Iron, 3 Copper, 1 Carbon, 1 Glass, 1 Circuit | 6–8 | 6–7 | 6–7 | 6–8 |

All candidate recipes begin ready and opening reserves are empty. The probe excludes cooling-enabled repeat uses and does not resolve Heat, Charge, Bolt, extra payments, Utility targets or effect grants. For example, affording Quick Vent does not prove Mara has its required Heat. The subset counts are an affordability envelope, not a combat simulator. The later owner decision allows duplicate recipe copies in memory, each taking one slot; each copy independently tracks its cooldown and once-per-turn use. Acquired duplicate copies can therefore increase available crafts and should be included in later supply/cooling playtests. This audit uses the unchanged starter lists, which contain 12 distinct recipes, and does not model acquired duplicate copies. Automatic cooldown ticks still skip the use round in the actual rules; nothing here changes their timing.

**Interpretation:** 6 is a lean stress case; 8 is worth comparing as the tighter economy; **10 is the recommended first beta profile**, giving Ada more material room while retaining a choice among the 12 starters and providing some access to higher-cost discoveries. Twelve is a generous stress case, with more room for early stockpiling and cooling. The baskets differ in composition as well as total, so these results do not isolate the causal effect of two extra units. In particular, 10 introduces a guaranteed Circuit and another Iron relative to the 8-material fixture. Test quantity and mix separately before attributing a problem to total supply.

### Cost problems that a bigger haul will not solve

- **Extra Lift (SH008):** costs 1 Copper + 1 Circuit now and grants 2 mixed units next round. It has zero net material-count gain before other synergies and transforms known materials into a delayed mix. That may be a conversion choice, but it is weak as the introductory quantity upgrade. Compare a proposed **+3 next-round yield** (+1 net unit) against the current +2 during beta; no recipe row has been changed or that buff selected. Evaluate actual useful types and missed immediate defence, not just the count.
- **Copper/Glass competition:** cooling, defence and utility setup frequently compete for the same materials. When a player has plenty of total scrap but cannot use it, adjust mix/steering or a specific recipe before raising everyone's haul. A permanent +1 material per turn is 10% of a ten-unit haul, but its tactical benefit may jump when it completes a recipe cost.
- **Multiple cheap shots — base damage resolved, 15 September:** the owner specifies zero base gun damage; all bullet damage and effects come from its parts. SH001 and SH003 therefore supply 9 damage together or across two shots before other effects. The earlier 13-versus-17 comparison is superseded. Continue comparing part synergies, targeting and status timing when testing large versus split shots. No firing cost or shot cap is introduced, and the printed-cost supply analysis is unaffected.
- **Cooling and stockpiling:** positive printed costs alone do not prove a full refund/cooling chain cannot pay for itself. New counters can be cleared by explicit cooling, and surplus stock carries across rounds without a cap. Probe repeated cheap shots, resource refunds, global cooling and waiting for large builds in complete fights. Repeated no-cooldown uses remain blocked absent explicit permanent upgrades.
- **Pending catalogue timing:** shield-spending effects that need active Shield during preparation cannot assume an End-Turn-only prepared shield is already active. Utility dependencies and the character-specific draft rules also require reconciliation. Increasing supply cannot make an illegal timing chain valid.

### Slay the Spire inspiration for the beta

Mega Crit's [GDC 2019 balance slides](https://media.gdcvault.com/gdc2019/presentations/Giovannetti_Anthony_SlayTheSpire.pdf), rechecked on 15 September 2026, support iterative changes informed by both player feedback and metrics, preserving uses for different cards and comparing player skill groups. They do not supply a universal resource formula. The 22-page slide text was reviewed; no full video or current live-game balance audit is claimed.

**Our application:** seek several worthwhile ways to spend a constrained haul. Start with the costed ten-unit profile, observe why players choose or skip recipes, and change a specific cause. Preserve our visible recipe memory and carried materials; do not import a draw/discard system, energy budget or Slay the Spire win-rate target.

For the first playable beta comparison, record the supply profile, seed, character, starting recipes/upgrades, encounter and player familiarity. Log each haul by material and steering choice; recipe use and extra payments; resources returned, spent and banked; cooldown state; shots per turn; effective damage, overkill and Shield; HP lost and turns to victory/death; and unaffordable desired recipes. Ask what tradeoff the player saw and whether the steering helped. Pair 8/10-material comparisons on matching encounter seeds, then use fresh seeds; keep newcomer and experienced results distinct.

Begin with basic attacking, multi-enemy and setup-pressure encounters once their rules exist. Check all four starter kits before claiming character parity. Persistent HP and no automatic between-fight healing require a later encounter-sequence test; a comfortable isolated fight can still produce an impossible campaign. Reward pick/use rates need denominators, encounter context, acquisition timing and sample counts; a popular recipe is not automatically overpowered. Small early samples expose problems, not reliable target win rates.

Raise supply if useful ordinary choices remain blocked after reasonable steering; adjust mix when total stock is adequate but wrong; adjust a specific recipe or effect when it dominates across different supplies. If almost every ready useful recipe fits repeatedly and reserves grow without sacrificing protection, compare a leaner profile before expanding content. Leave strong earned combinations possible. The next step is owner review of this **10-total / 8+2 proposal**, followed by remaining recipe/enemy rules before any authorized playable test. No gameplay implementation or beta balance claim is made by this analysis.

## Historical research — 13 September 2026

The original research below is preserved as history. Its separate energy budget, paid additional hauls, physical retrieval constraints and older rig terminology are superseded by the owner decisions above where they conflict. Use the current recipe-cost analysis for the beta supply recommendation; the earlier research remains methodological background.

## Useful primary documentation

All sources accessed 2026-09-13. Historical examples explain design reasoning; they are not claims about the latest balance patch.

1. **Anthony Giovannetti, Mega Crit — GDC 2019, Slay the Spire: Metrics Driven Design and Balance.** [Official session](https://www.gdcvault.com/play/1025731/-Slay-the-Spire-Metrics%EF%BB%BF), [official slides](https://media.gdcvault.com/gdc2019/presentations/Giovannetti_Anthony_SlayTheSpire.pdf). Reviewed the 22-page deck's extracted text, not the full talk/video or its image-only charts. The stated aim is a purpose for every card while avoiding effects that dominate the design. The deck combines iteration, playtester feedback and metrics, explicitly cautions against treating data as a conclusion, and separates player skill through Ascension levels. It supports a method, not a universal damage formula.

2. **Mega Crit — Slay the Spire 2 beta v0.101.0, 27 March 2026.** [Official developer explanation](https://steamcommunity.com/games/2868840/announcements/detail/507357499795439630), verified in the rendered Steam page. Giovannetti reverses a Prepared change to preserve Silent's identity, adjusts early elite placement and map consistency, and rejects difficulty that reduces interesting choices. He describes common relics as broadly useful rather than necessarily weaker. An enemy's conflicting mechanics are simplified before considering further numerical tuning. This is unusually useful documentation of why changes were made, rather than only before/after values.

3. **Mega Crit — May 2026 Neowsletter, 22 May 2026.** [Developer report](https://www.megacrit.com/news/2026-5-22-neowsletter-issue-22/). It reports 25% overall wins at that point, 16% at A0 and about 17% at A10. These describe different player populations and are not a recommended target for our game. The update also explains replacing Doormaker because it was more complex than desired despite offering interesting small decisions. This is evidence that mechanical complexity itself can be a design problem.

4. **Ian Schreiber — Game Balance Concepts, 21 July and 11 August 2010.** [Costs and benefits](https://gamebalanceconcepts.wordpress.com/2010/07/21/level-3-transitive-mechanics-and-cost-curves/) and [situational balance](https://gamebalanceconcepts.wordpress.com/2010/08/11/level-6-situational-balance/). These designer-authored lessons explain comparing effects against their costs, including limitations and opportunity costs. They also explain why effects such as area damage have different value in different encounters. A comparison formula supplies a starting estimate; context and playtesting remain necessary. This is general design teaching, not Mega Crit's internal formula.

5. **Ian Schreiber — Game Balance Concepts, 18 and 25 August 2010.** [Progression and pacing](https://gamebalanceconcepts.wordpress.com/2010/08/18/level-7-advancement-progression-and-pacing/) and [metrics and statistics](https://gamebalanceconcepts.wordpress.com/2010/08/25/level-8-metrics-and-statistics/). The relevant sections connect visible rewards with progression and explain small samples, unrepresentative testers and correlation versus causation. These help frame our tests; the course's psychological generalisations are not treated here as independently verified research.

No public source inspected provides the complete current Slay the Spire 2 balancing model, internal tools, reward weights or a formula that can be transplanted into this game. The card library reveals possible interactions; the documents reveal parts of the developers' method. Our design still needs its own playable evidence.

## Our central balancing problem

The player has four connected resources: **energy this turn, physical parts available, rig health across the run and the capabilities fitted to the rig.** Each choice changes what those resources can accomplish later. All recommendations below are our application to Klaus's concept, not claims that Slay the Spire uses these exact rules.

More energy can buy an additional haul, shot or defensive action. More components only help if the player can retrieve, fit and activate them. Health lets the player accept a hit to prepare a better combination. A new module changes which tradeoffs are attractive. Balancing any one of these without the others can create a misleadingly strong or unusable upgrade.

The target is a range of appealing decisions, with moments of earned power. We should be able to name a situation where a component is useful and a situation where another choice is better. A powerful combination may make a particular fight easy; the concern is one accessible combination making most future decisions irrelevant.

## What to price and compare

| System | What must be compared in our game | Failure to look for |
| --- | --- | --- |
| Energy | Complete legal turns: gathering, firing, armour and preparation | A firing cost leaves no viable defence; or an extra energy point makes every choice affordable |
| Parts | Actual reachable stock, consumption, reserve capacity and replenishment | A useful-looking reward rarely has compatible material within reach |
| Large stacks | The extra benefit of the next component versus a separate shot or armour | Always filling the weapon is best, or the dramatic large shot is never worth preparing |
| Multiple targets | Damage applied to each target, overkill and enemy actions prevented | A splitter duplicates every payload at negligible cost and replaces focused shots |
| Poison/corrosion | Damage that actually occurs before death or combat end, and the cost of surviving until then | Advertised total damage is impressive but rarely has time to happen |
| Piercing | Actual protection bypassed in the current target, plus baseline utility | A specialist is useless in most fights or makes every other projectile obsolete |
| Armour | Incoming damage prevented, duration and parts unavailable for ammunition | Automatic full defence every turn; or defence is so costly that attacking is always better |
| Repeat and recovery effects | Total components, energy and activations produced by the full chain | An endlessly repeatable cycle pays for itself and removes enemy participation |
| Rewards | When a module becomes usable and what later decisions it opens | The run is decided by its first drop, with no practical bridge or pivot |

A simple numerical illustration shows why damage totals are insufficient. Against three enemies with 12 health each, a shot dealing 12 to one target immediately removes one enemy's next attack. A shot dealing four to each also totals 12, but leaves all three attacking. Area damage can become preferable with other health totals or a useful status effect. We must compare the resulting enemy turn, not just the number displayed above the weapon.

Another illustration: increasing a three-energy budget to four is a 33% increase in raw energy. Its actual benefit may be much larger if a two-energy shot and a two-energy defensive action now fit together. This arithmetic is not a recommendation to use three or four energy. Every energy upgrade needs a check of the new combinations it permits.

## Resource availability and semi-random builds

### Flexible scrap and choosing after seeing the threat

Schreiber's [situational-balance lesson](https://gamebalanceconcepts.wordpress.com/2010/08/11/level-6-situational-balance/) includes an unusually close example: metal that can become either a weapon or armour. He explains that choosing between uses can be worth more than either fixed use, without being worth their combined benefits. He also distinguishes selecting an effect before knowing the situation from choosing it only when it will work. Rechecked these sections on 2026-09-13.

Our application: a steel piece that can become ammunition or protection after enemy intentions are visible is more reliable than a piece restricted to one purpose. That does not mean we should penalise the starter metal until it feels weak. Keep it a dependable foundation, then give specialised components a clear advantage in their intended situation. Compare complete turns with the same starting stock, rather than assuming two pieces are equivalent because their printed numbers match.

The timing of commitment is therefore a balance rule. Holding a canister until a group appears, then using a splitter, avoids spending that combination on an unsuitable lone target. Judge the combined effect in the favourable situation the player can deliberately create, including the real costs of obtaining, holding and firing its pieces. Do not discount a powerful combination merely because it would be poor when used carelessly.

For a future experiment, compare identical encounters with (a) flexible steel, (b) separate attack and armour pieces, and (c) flexible steel plus one specialist. Observe whether a specialist creates an appealing decision, whether mixed stock forces unavoidable damage, and whether flexible steel makes every specialist redundant. These are alternative test conditions, not three new systems to implement.

Separate randomness before the decision from randomness after commitment. Unexpected scrap or reward options can create an interesting problem that the player sees and solves. A hidden last-second failure on a carefully assembled shot can undermine that planning. The initial recommendation is predictable activation outcomes with variety in visible supplies, encounters and rewards.

Test material access physically, not only by counting a level's inventory. A pile can contain enough armour material while every useful piece is buried beyond the player's affordable hauls. Record that distinction. An opening-stock rule, basic conversion option or alternative defence may prevent unavoidable starvation; choose the simplest solution that preserves retrieval decisions. Do not guarantee effortless survival or an exact desired combo every turn.

For early rewards, compare several starting directions on the same encounter sequence. Then vary the sequence and later rewards. Record whether the player obtains a functional build, finds complementary choices and can change direction after a poor offer. Showing a module three times is not evidence of three meaningful choices if the other options never work.

Encounter rewards and stage rewards must be balanced together with repair opportunities. Persistent damage creates stakes, but an early mistake should not leave a long sequence of visibly hopeless fights. Optional harder routes can offer stronger rewards with clear danger; a safer route can support recovery or a less developed rig. No exact route or reward distribution is selected here.

## Proposed order of balance work under later owner-set goals

1. **Fix the rules before tuning the catalogue.** Specify when energy refreshes, what consumes it, how the pile refills, what survives a turn/fight, armour duration, effect order, and how defeat works. A changed timing rule can invalidate every numerical comparison.
2. **Establish basic choices in a controlled encounter.** Give known reachable stock and compare a focused shot, a spread shot, protection and preparation. Include enemy attacking and preparing turns. Identify the decision each turn is intended to support.
3. **Add contrasting encounters.** A group of weak attackers, a protected target and a threat that grows stronger should challenge different habits. Avoid hard immunity that simply invalidates a semi-random build without a visible fallback.
4. **Add a few interacting rewards and an encounter sequence.** Check build development, health attrition, material resets, repairs and the stage guardian. Do not test only pre-equipped perfect builds; also test earning the pieces in ordinary reward order.
5. **Try to break the system.** Compare repeated cheapest shots, maximum stacks, full defence, hoarding, stun chains and scrap/energy recovery loops. Automated scenario searches can expose illegal states or dominant loops; human play must judge clarity, anticipation and enjoyment.
6. **Change a specific cause, then compare again.** Record the expected effect of a change. Keep the same seeds for comparisons, then use fresh seeds to check whether the apparent improvement generalises. Do not retune every system simultaneously and lose the explanation for the outcome.

## Evidence worth recording in the concept test

Use a compact local record per fight and a short observation from the player. No analytics service is needed to begin.

- Build version, scenario/seed, player familiarity, starting health, stock and modules; ending health, win/loss and turns.
- Energy spent on gathering, offence and defence; unused energy; components retrieved, spent and retained; attempted unaffordable actions.
- Actual damage, overkill, effective protection, delayed damage that resolved, enemy actions prevented and free-chain activations.
- Rewards offered, chosen or skipped, and when each chosen capability first became useful.
- What the player expected, what surprised them, which decision felt interesting and why a loss happened. Separate thinking time, magnet-operation time and animation time.

Interpret these together. Frequent selection can mean a part is strong, easy to understand, required to survive or simply over-offered. A rare item found late will naturally occur mostly in runs that already survived a long time. Compare skill groups, encounter stage, acquisition timing and sample counts before drawing conclusions. Tiny samples identify problems to investigate, not stable win-rate estimates. Mega Crit's large published aggregate should not become our difficulty target.

Good early evidence would show the owner understanding the choices, a reward changing their strategy, more than one successful approach across suitable situations, a loss they can explain, and enthusiasm to try another combination. A fair win rate alone cannot establish any of those. Loading animation and sound should receive the same repeated-play scrutiny as the resource maths.

**Next proposed goal:** define and review the representative combat scenario, including both assembly routes and the resource/reset rules. Use these documents as references when that goal is set. A playable balancing experiment follows the owner's authorization; no implementation or formal numerical balance model was created in this research increment.
