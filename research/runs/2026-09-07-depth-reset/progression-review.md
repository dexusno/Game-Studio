# Independent progression review

2026-09-07. **Unimplemented design proposal**, based on Klaus's latest feedback; no fun validation or project selection. Sixfold remains parked. This review does not select a viewport, theme, combined weapon/shield or separate equipment. It supports an extensible attachment/elemental framework without fixing its combat implementation. No content quota, price or schedule is proposed.

## The consequential challenge

A large attachment catalog can still produce the same repetitive fight. If loot merely completes a predetermined combo, increases damage, or removes the need to defend, the player is collecting permission to repeat the same actions. The first biome must demonstrate an upgrade that changes a tactical decision, followed by a worthwhile reward that makes the player reconsider part of that plan.

## Starting kit and rewards

The starting equipment must already support deliberate offense, defense, movement and a punish/control opportunity. Drops can transform these actions but must not unlock basic responsiveness, useful defense or the ability to answer a mandatory enemy. A practiced player should be able to defeat the biome with its starting tools; rewards create attractive possibilities and power, not a hidden entry requirement.

Evaluate attachments by changed behavior: range, commitment, target choice, attack timing, positioning, resource use or an interaction with another ability. An elemental attachment should do something recognizable in play, such as redirecting a committed attack through a marked target; an orange damage number alone is weak evidence. Preserve opportunity costs through equipped slots, incompatible transformations or clearly stated consequences. Do not let stacking erase every tradeoff.

Offer both useful reinforcement and a comprehensible pivot. Let players inspect an effect and test it safely before abandoning a current attachment. Defer detailed trigger/resource rules to the combat designer and engineer; the reward system must know compatibility before generating offers.

## Exact acquisition and state example

Illustrative attachment: **Storm Conductor**. A successful timed defense stores a spark; the next deliberate attack spends it to arc through a marked target. Choosing its offensive routing occupies the place of a defensive recovery attachment. This is a proposed interaction, not an agreed combat rule.

1. The journal identifies Storm Conductor and its source: complete the Relay Trial. The player can pin that pursuit before departure. No random drop is required to reveal its known acquisition path.
2. Generation reserves a reachable optional Relay Trial in the biome and announces its reward at the route junction. Its challenge is solvable with the baseline kit. Choosing that branch gives up another opportunity; completion depends on play, not a rare spawn or repeat count.
3. Trial success atomically records `ownedPlans += storm_conductor` and grants an installable copy for the current expedition. The acquisition identifier prevents duplicate rewards after reloading. A nearby safe station allows immediate use, so the next encounter tests the reward.
4. Death later ends that expedition's active loadout and temporary amplifiers. The plan remains owned. On the next departure the player may select Storm Conductor as a starting attachment, subject to the same loadout capacity as alternatives. No crafting-material grind or second lucky drop is needed to use the earned option.
5. Repeating the trial does not produce another plan or an ever-growing mandatory currency advantage. Its replay version offers an announced temporary reward or a skill challenge; the player can take a different route.

Keep these states separate: **known source**, **owned plan**, **equipped this expedition**, and **temporary amplification**. Losing the latter must never look like losing ownership. Save expedition seed/content version, pursuit, chosen routes, claimed rewards and equipment. Resuming restores the same expedition; starting a new one varies it.

## Discovery, starts and campaign payoff

Targeted pursuit provides agency; unplanned finds provide surprise. Newly discovered plans become additional pursuits or immediate unlocks according to their stated source. Avoid dropping all future content into one expanding global pool. Generate rewards from compatible equipment, biome identity and current progression, with visible alternatives that can change direction.

Different starts combine chosen equipment identity with varied entry routes and starting circumstances. They should alter the opening problem, not merely relocate the same enemies. Early rewards should support the starting idea without guaranteeing the complete ideal combo. Bosses must remain defeatable without finding one exact mod.

A first level clear should visibly resolve its objective and open new content: a region entrance, a starting origin or a meaningful equipment choice. A first boss victory should have an immediate usable reward and a permanent campaign milestone, saved before any later hazard. A mastery achievement can add an optional alternate challenge or cosmetic distinction; it should not hide a required upgrade behind repetitive perfect clears. Campaign progress ultimately resolves a larger objective with an ending. Further expeditions are a choice after that achievement.

## Three failure risks and observable tests

| Risk | Test in the representative biome |
| --- | --- |
| **Catalog growth without combat depth** | Compare the baseline and contrasting builds against the same mixed encounter. Observe target order, positioning, timing and resource decisions. If only completion speed changes, revise the attachment behavior. Ask which reward changed the player's plan. |
| **Randomness frustrates intent or dictates a required build** | Run disclosed pursuits across varied seeds and deliberately awkward loadouts. Verify reachable acquisition, compatible offers and baseline-solvable mandatory fights. Observe whether Klaus understands how to obtain a wanted mod and ever prefers an unexpected alternative. |
| **Persistence creates grind or makes failure feel erased** | Complete a trial, die, reload and start differently. Verify the plan survives, the lost temporary state is explained, and rewards cannot duplicate. Observe whether the next goal feels worthwhile; if repeating the easiest trial becomes the rational progression route, change incentives before expanding content. |

This is a design review. The engineer must validate compatibility data, reward/save transactions and generator guarantees before the producer adopts these rules. The next owner slice needs the complete acquire–use–fail/succeed–restart sequence; a loot menu or simulated economy cannot establish its appeal.
