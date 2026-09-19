# Permanent upgrades — acquisition and implementation proposal

18 September 2026. Companion to the [upgrade catalogue](UPGRADE-CATALOGUE.md), [Mayor offers](MAYORS.md) and [mercenary abilities](MERCENARY-ABILITIES.md). This is a design contract for future implementation, not implemented gameplay or measured balance.

## Selected direction and proposed details

The owner defines upgrades as campaign-lasting items comparable to STS2 relics. Their scope includes high-tech equipment that changes game defaults, not only gun and Shield improvements. Some are shared and others belong to a mercenary. Mayors offer three Rare/Legendary options at city entry, with stronger rewards through the three city waves. The owner specifically identifies Ancient relics as the inspiration for Mayor upgrades.

The owner also requires rarity parity with the reference. Common, Uncommon and Rare source relics map directly to those upgrade rarities. Shop and Event describe exclusive source pools rather than an ordered power tier: preserve that source category and give the adapted item a stated proposed power rarity. Ancient sources map to Rare/Legendary Mayor gifts. Starter-inspired acquired items extend an innate feature rather than replacing or recounting the existing innate ability; their acquired rarity needs an explicit rationale.

All individual effects, quantities, acquisition distributions and implementation conventions below are proposals. A catalogue entry is an authored exception in its stated scope; it does not silently revoke a different character, recipe or upgrade effect. Existing Hot Barrel, Charged Barrel, Find the Seam and Bolt definitions remain intact. Quench Recovery and Residual Current remain unselected alternatives.

## Lifetime and ownership

- Install an accepted upgrade immediately. It occupies neither recipe memory nor part storage. Proposed default: no equipment-slot limit and no voluntary unequip action; all owned upgrades coexist.
- Each named ID is obtainable once per campaign unless it explicitly describes another rule. Different IDs combine. Legacy names/IDs are aliases of the same item, not another stack. Keep an ever-acquired ledger separately from active ownership: temporary equipment expiring does not make its ID eligible to acquire again. Catalogue wording "eligible unowned" includes this once-per-campaign filter.
- An on-acquisition effect occurs once. Reopening the shop, loading a save, moving a part or entering another city does not reacquire the item.
- An upgrade may remain owned after its limited charges are spent. Show the spent state; permanent ownership does not imply unlimited trigger uses.
- The campaign resets upgrades on New Game/death. Fight-local counters, scheduled deliveries and generated supplies expire at fight end unless an entry explicitly changes that boundary. Campaign counters persist.
- Continue restores the original fight-entry state and seed. Upgrades bought or triggered inside the abandoned fight must replay from that state; no duplicated purchases, payouts or consumed charges survive outside the restored snapshot.

## Events and ordering

Use typed events with a stable event ID, source ID, originating player action, phase/turn number, targets, actual costs paid, actual damage/Shield/HP change and part provenance. Stable ordering is essential for previews and same-seed repeats.

| Event | Meaning and boundary |
| --- | --- |
| Acquire | Commit ownership and payment, then apply this item's acquisition payload once. A shop discount acquired in this purchase does not retroactively discount itself. |
| Fight start | Initialize enemies, meters, counters and innate traits; resolve fight-start upgrades before the first player turn. Distinct first-turn triggers still occur at turn start. |
| Player turn start | Deliver scheduled next-turn values and fresh upgrades at their stated trigger after the prior phase's reset. Automatic cooldown accounting retains the rule that CD1 blocks the next whole round. |
| Collect | One committed normal haul, with its chosen steering and optional Precision outcome. Extra grants are not another Collect event. Only a named exception grants another haul or Precision attempt. |
| Recipe Use | Validate the actual copy, all costs and availability. Pay costs and mark the use/cooldown before effects. Utilities resolve immediately. Production and copying are distinct event kinds. |
| Part created | Record the canonical part type, source and resulting values once. Copies use the copy rule's eligibility; they are not automatically another resource-paid production event. |
| Fire | A valid nonempty bullet is committed. Apply stated character payments/bonuses and upgrade Fire modifiers once per main shot, preserving Charged Barrel's before-other-Charge-checks payment. Support hits do not become new Fire actions. |
| Hit / HP loss | Record each actual hit and its result. Flat reductions apply to their named event; a paid HP cost is not enemy damage. Lethal enemy recoil stops unresolved work under the existing death rule unless a named rescue exception intercepts death. |
| End Turn | The player's button ends the turn and returns an unfired bullet's parts. It does not activate the Shield. Explicit End Turn upgrade effects can run at this boundary. |
| Enemy attack | Installed Shield protects automatically. Protection is depleted across attacks, not refilled by recalculation or reinstallation. |
| Enemy phase end | Finish attacks/reactions, then read/pay remaining Shield and record deferred rewards before the normal reset. Apply explicit retention only to the remaining value. Weaken decays once per entire enemy phase. |
| Victory / encounter complete | Commit rewards once. A no-kill, all-escaped fight gives no kill/victory reward. Non-combat Mystery completion advances city progress but is not a combat victory. |

For otherwise simultaneous effects, use acquisition order and then stable ID order. Resolve each effect fully before the next, with death interruption taking priority. Explicit before/after text overrides this fallback. Required choices pause resolution and show their consequence; choices do not reroll the source event.

Claim an item's once-per-turn/fight allowance **before** resolving its payload. A fresh resource grant is not a refund; a refund cannot exceed the named eligible material actually paid unless the entry explicitly grants a bonus. A proc that requires a paid recipe cannot trigger from a free copy or passive grant. Cross-item synergies remain possible: this is event attribution, not a blanket ban on upgrades triggering each other. Flag an actual self-sustaining trigger cycle for redesign; do not impose a global shot/action cap to hide it.

## Ordinary parts and Shield

Generic plain N-damage grants use SH001's Plain Slug canonical part type with this upgrade setting its damage to N. Generic N-Shield grants use SH002's Basic Shield Plate type with its Shield value set to N. A named different output must cite its own canonical recipe. No generic grant inherits another recipe's secondary effects or scheduled rewards.

Shield grants auto-install ordinary removable parts, unless an entry explicitly grants direct active/retained Shield rather than a fresh part or explicitly places parts in reserve. Opening Relay preserves its earlier direct active-Shield exception; that balance cannot be unloaded as an inventory part. The player may unload a fresh granted part to save it under the ordinary rules. Moving a partially depleted installed part preserves its remaining contribution; it cannot recreate the spent Shield. The installed round build resets after the enemy phase. A new grant next round is a new part, not retention of the earlier part.

**Resale mapping updated to the owner-selected 20 September policy:** a generic pure N-damage/N-Shield grant uses the corresponding generated-only family in [Part resale](PART-RESALE.md), fixed from its original grant value when created. The SH001/SH002 convention above supplies ordinary part behavior, not a universal price for every grant size. An explicitly named recipe output, a copy of it, or an existing part modified by an upgrade keeps its own original main-recipe reference. Later bonuses, Shield spending and copying never recalculate that reference. Use half its normal one-part ingredient value at current resource prices, rounded down per part. No additional crafting cost or automatic zero-sale-price rule is introduced; ordinary sale eligibility remains. Finite supply triggers and the actual shop economy must be tested together.

Shield retention entries preserve only an available remaining amount. They do not duplicate it into both an installed part and a separate balance. Multiple independent retention allowances add, bounded by the actual remaining Shield, unless an entry specifies replacement. A retained amount loses its old one-use secondary payloads and cannot repeat a spent recipe effect. Entries that spend pre-reset Shield subtract it before retention.

## Counts, costs and numerical conventions

- Every damage, Shield, material, HP and meter amount is an integer. Use existing damage rounding and resolution order; percentages add at their named stage and floor once. Round a fractional shop price down once, with a proposed minimum price of 1 Credit for a priced product unless an item explicitly gives it free.
- A resource discount reduces a real printed ingredient quantity; it cannot create a missing ingredient or waive HP, Heat, Charge, part sacrifices, cooldown or use allowance. An entry must explicitly specify any such exception.
- A no-cooldown recipe's additional use allowance does not manufacture resources. Clearing cooldown counters does not itself refresh a no-cooldown recipe. Duplicate recipe copies keep independent state.
- Own HP costs still require cost + 1 HP. Ordinary damage reduction does not reduce an HP payment. Enemy recoil can kill. A named death-prevention upgrade is an explicit exception, with its consumption occurring before subsequent queued effects.
- Healing clamps at maximum HP. An explicit maximum-HP increase also heals for that increase, matching the selected rule. Reaching a city, visiting a shop or finishing a fight supplies no general healing; a named owned upgrade may explicitly add a healing trigger.
- A saved part is an unused ordinary part created in an earlier player round, where age is required. A fresh grant is not saved merely because it entered reserve. Loading/unloading does not change age.
- Recipe upgrades attach to a particular owned recipe copy unless their text specifies all copies or a part type. Selling a part does not sell its recipe or passive upgrade.

## Acquisition and Mayor progression

The catalogue owns each item's eligibility. Normal shared and mercenary items use Officer, Shop or Mystery routes as listed. Mayor-exclusive entries never leak into an earlier city's Officer/shop pool; this is how later tiers retain their strength. Innate character abilities are not random loot or included in the new item count.

Proposed normal offering: filter by mercenary, city, source and unowned IDs first; sample without replacement. Bias toward different strategic roles when choices exist. No guaranteed build synergy or hidden enemy retuning follows a selection. Rarity and shop price are separate attributes. Exact Credit price bands remain an economy-tuning task because the campaign income baseline is not yet calibrated; the roster must not pretend an arbitrary price is tested balance.

Mayor draws use the current city's exclusive tier and the current mercenary plus shared pool. Present one broadly useful choice, one build-oriented choice and one other distinct eligible choice where available. A drawback must be visible before acceptance. Offer eligibility may reject an item that literally cannot resolve its cost/choice, but should not hide an unusual item merely because the current build is not optimal for it. Keep enough unconditional options for all three slots; seed both the ordered candidate list and nested choices.

City 1 establishes a build and useful recurring efficiency. City 2 changes the action economy or strengthens a developed synergy. City 3 grants a substantial recurring engine, survival mechanism or rule exception for the hardest wave. Do not promise that every City 3 item beats every City 1 item in every build; stage strength is a balance target, not an automatic scalar. Earlier Mayor gifts stay active, so assess cumulative power as well as each new choice.

Additional upgrade rewards draw from the normal pool unless explicitly stated otherwise; they cannot recursively grant Mayor bundles. On normal-pool exhaustion, suppress the impossible offer and use an authored non-upgrade alternative rather than an infinite reroll. This exceptional exhaustion reward remains an economy detail to define before runtime. The dedicated Mayor pools cannot be exhausted by ordinary acquisition.

## Data and presentation contract

Each roster row has a stable ID, original name, rarity, source rarity and mapping rationale, mercenary eligibility, acquisition routes, city bounds, complete effect, strategic purpose, explicit exception, risk, inspiration references and implementation tags. Inspiration is provenance, not an instruction to import an STS2 mechanic missing from our game. Normal rarity correspondence is preserved; special acquisition categories have an explicit mapping.

Future runtime state needs owned and ever-acquired IDs; acquisition order; upgrade version; per-turn/fight/campaign counters; remaining charges; selected recipe-copy IDs; scheduled payloads and their recorded amounts; provenance for generated/modified parts; and seeded offer/choice streams. Save the initial state for the whole fight, including an in-fight shop's finite stock, so restarting the same seed restores the same economy.

Show a compact owned-upgrade strip with tooltips for trigger, remaining charge/counter, affected recipe and any exception to a normal rule. A Mayor screen shows the immediate effect, recurring benefit and downside separately. A damage/Shield preview includes the named upgrade contribution; a player should never need the source relic reference to understand our item.

## What to verify before implementation is considered complete

Structural checks can verify counts, IDs, eligibility, stage separation, source references and the existence of three Mayor choices. They cannot prove balance or fun. Runtime tests must later cover trigger ordering, same-seed restart, nonlethal own costs versus lethal recoil, shield depletion/unloading/reset/retention, recipe-copy state, source-aware refunds and acquisition/sale loops.

For the first playable balance pass, select representative supply, offence, protection, recovery, memory, cooling, route and economic items plus one upgrade for every mercenary and Mayor tier. Measure HP lost per fight, victory time, resources spent/left, extra actions enabled, Credits created, selection rates and combinations that bypass enemy pressure. Keep the full pool as the design target; the first test subset is not a scope reduction or a claim all items are implementation-ready.
