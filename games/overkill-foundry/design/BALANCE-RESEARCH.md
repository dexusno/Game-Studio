# Overkill Foundry — balance research and proposed test method

13 September 2026. Requested by Klaus during the stacked-ammunition discussion. This records developer documentation and a proposed method for our game. It does not establish tuned values or authorize implementation. Companion: [stacked ammunition and build strategies](BUILD-STRATEGY-STUDY.md).

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
