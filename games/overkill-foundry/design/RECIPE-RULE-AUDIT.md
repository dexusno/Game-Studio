# Recipe rule audit — review together

**16 September 2026 · all 606 recipes reviewed · recipe text unchanged**

Reviewed the catalogue at `5aff12e` against the settled owner rules. Found **108 recipes with a definite conflict or obsolete dependency**, **73 additional recipes needing wording/dependency clarification**, and **425 with no direct conflict identified**. A recipe can have findings in several groups; group counts therefore overlap. Clarifications on a conflicting recipe are also retained.

This is an audit for joint review, not a rewrite. No replacement effect, category, price, cost or number has been selected. The four new inherent abilities remain proposals; their values are not treated as owner rules. No direct conflict identified means the row passed this written-rule review, not that it is implementation-ready, balanced or proven in every combination.

**Suggested first review:** C04, starting with **MA004 Quick Vent** from Mara's starter set. It is an immediate Utility that grants Shield before End Turn. The other flagged starter is **SH005 Split Outlet**, whose spread-Modifier placement/accounting needs Q10 clarification. The other unique starter rows have no direct conflict identified in this audit.

## Coverage

| Pool | Reviewed | Conflict | Clarification only | No direct conflict identified |
| --- | ---: | ---: | ---: | ---: |
| SH | 126 | 21 | 19 | 86 |
| MA | 120 | 15 | 17 | 88 |
| IV | 120 | 19 | 13 | 88 |
| AD | 120 | 24 | 5 | 91 |
| NO | 120 | 29 | 19 | 72 |

All rows were read for: output kind and Utility lifecycle; Shield activation, reset and grants; per-copy cooldown, global cooling and no-cooldown uses; part-based damage, multiple shots and modifier duration; status recipients, hit order, explicit copy/stack limits; whole-number arithmetic and player HP costs/death; resource/part persistence, collection count and Precision protection; part consumption, saved-part age and canonical sale basis; removed character dependencies; proposed traits are not owner rules.

The [complete per-recipe ledger](analysis/recipe_rule_audit.json) contains every ID, name, original row, source line, disposition and finding references. The [audit renderer](analysis/recipe_rule_audit.py) verifies the exact reviewed catalogue before regenerating this document. It validates coverage and evidence preservation; it does not discover or prove semantic conflicts automatically.

## Findings index

| Group | Classification | Recipes | Topic |
| --- | --- | ---: | --- |
| [C01](#c01) | conflict | 13 | Recipe-granted Shield retention |
| [C02](#c02) | conflict | 9 | Utilities create or copy physical parts |
| [C03](#c03) | conflict | 7 | Saved, split or sacrificed Utility items |
| [C04](#c04) | conflict | 41 | Shield granted during preparation, collection or after Fire |
| [C05](#c05) | conflict | 19 | Fresh recipe Shield at a later turn's start |
| [C06](#c06) | conflict | 5 | Shield-part activation expected before a shot |
| [C07](#c07) | conflict | 1 | Shield conversion at the old reset point |
| [C08](#c08) | conflict | 13 | Removed Charged Barrel payment |
| [Q01](#q01) | clarification | 8 | Shield replenishment during enemy actions |
| [Q02](#q02) | clarification | 12 | Preparation-time active-Shield dependencies |
| [Q03](#q03) | clarification | 5 | Order within End Turn's Shield activation |
| [Q04](#q04) | clarification | 5 | Remaining Shield sampled at enemy-phase end |
| [Q05](#q05) | clarification | 4 | Utility item wording left after conversion |
| [Q06](#q06) | clarification | 6 | Recipe crafting versus immediate Utility use |
| [Q07](#q07) | clarification | 2 | Cooling rewards restricted to parts |
| [Q08](#q08) | clarification | 14 | Status recipient implied rather than stated |
| [Q09](#q09) | clarification | 14 | Canonical sale basis for batches and generated subparts |
| [Q10](#q10) | clarification | 10 | Spread Modifier lifecycle and placement order |

## Rule references

These summaries refer to the selected-rule sections in [RECIPE-CATALOGUE.md](RECIPE-CATALOGUE.md) and the owner chronology in [DECISIONS.md](../DECISIONS.md). Draft conventions are used to explain dependencies, not promoted into owner decisions.

- **R01:** Utilities activate on recipe Use and produce no part; physical parts and immediate effects are distinct. See the catalogue's How recipes work / Utility reconciliation sections.
- **R02:** Active Shield resets at enemy-turn end by default; only explicit permanent upgrades provide retention exceptions.
- **R03:** Only End Turn activates prepared Shield. Fire, Load, staging and collection do not activate it; explicit upgrade grants retain their own timing. Secondary-effect and non-Ammo accounting still contain draft details.
- **R04:** The gun contributes zero innate damage/effects. Old Hot Barrel/Charged Barrel independent damage grants are superseded. Recipe-authored effects and explicit Utility bonuses are distinct from gun base damage.
- **R05:** Each recipe specifies its own status recipients; there is no blanket main-target-only or all-hit-target default.
- **R06:** A part's sale value uses the main recipe's normal one-part ingredient requirement, current resource prices, a 50% factor and whole-credit floor. Discounts/copies do not change that basis.
- **R07:** Spread contributions are separate hits resolved in contributing-part placement order. Load can be undone before Fire; resolved effects/crafting are not automatically refunded.

## C01

**Recipe-granted Shield retention — conflict · R02**

The recipe keeps active Shield through the normal reset. Only permanent upgrades may grant that exception; the existing superseded marker does not repair the effect.

**For our review:** Review replacement content for these recipes under the already-settled reset rule.

Exact excerpts below; full effects and costs remain in the catalogue and ledger.

| Recipe | Existing wording |
| --- | --- |
| **SH026 — Folding Brace** | Keep up to 4 of your remaining Shield at the start of next turn; that retained Shield expires normally one turn later. |
| **SH057 — Spare Metal Brace** | If at least 3 Iron remain in your raw pool after use, keep up to 5 Shield at next turn's start. |
| **SH092 — Hinged Wall** | Keep up to 15 of your remaining Shield at the start of next turn. |
| **SH113 — Last Wall** | Keep up to 20 remaining Shield at the start of next turn. |
| **MA048 — Holdfast** | At the start of next turn, keep up to 12 remaining Shield if you paid, or up to 4 if you did not. |
| **MA099 — Settled Slag** | At the start of next turn, if Heat is 0 before start-of-turn Heat gains, keep up to 14 remaining Shield instead of clearing it. |
| **IV019 — Feint Plate** | Retain up to 3 remaining Shield at next turn's start for that turn only. |
| **IV057 — Stored Wall** | At the start of your next turn, retain up to 4 remaining Shield for that turn only. |
| **IV061 — Hold the Gap** | If any enemy currently has at least 6 Mark, also retain up to 5 remaining Shield at your next turn's start for that turn only. |
| **IV090 — Stockroom Wall** | If at least three saved parts are currently in reserve, retain up to 14 remaining Shield at your next turn's start for that turn only. |
| **AD074 — Hold the Panel** | At the start of your next turn, keep up to 8 of your remaining Shield if Bolt is active then; this replaces normal clearing for that amount only. |
| **AD092 — Shared Scaffold** | At the start of each of your next two turns, keep up to 12 remaining Shield if Bolt is active then. |
| **NO087 — Field Lock** | If Charge is at least 8 at Fire, keep up to 12 remaining Shield at the start of next turn instead of clearing it. |

## C02

**Utilities create or copy physical parts — conflict · R01**

A Utility is labelled as an immediate effect with no part, but its effect manufactures or copies inventory parts, including delayed production. This contradicts the recorded no-part boundary.

**For our review:** Review the intended recipe category and output; no conversion has been applied.

Exact excerpts below; full effects and costs remain in the catalogue and ledger.

| Recipe | Existing wording |
| --- | --- |
| **SH071 — Spare Casting** | At the start of next turn, receive one fresh copy with its printed effect; do not copy later bonuses attached to it. |
| **SH076 — Plate Recasting** | Make one Recast Plate Slug adding that amount as shot damage, up to 14; it has no other effect. |
| **SH102 — Reserve Pattern** | Receive two fresh copies next turn. |
| **SH118 — Exact Duplication** | Receive two fresh copies next turn with printed effects only. |
| **MA107 — Warm Repair Bench** | At the start of next turn, gain 2 Ready Plates. |
| **MA119 — Spare Foundry** | At the start of next turn, gain 2 Siege Slugs and 2 Siege Plates. |
| **IV118 — Spare Needle Bench** | For this fight, when a main shot consumes at least two saved Ammo parts, receive 1 Plain Needle at your next turn's start. |
| **NO107 — Shield Press** | At the start of next turn, gain 2 Stored Field Plates. |
| **NO120 — Pocket Generator** | At the start of next turn, gain 2 Pulse Tips and 2 Charged Plates. |

## C03

**Saved, split or sacrificed Utility items — conflict · R01**

The effect requires an inventory Utility item, its age, or separate stored activations. Utilities activate on recipe Use and do not create those items.

**For our review:** Determine the intended replacement dependency or output. AD062 is newly identified beyond the existing superseded markers.

Exact excerpts below; full effects and costs remain in the catalogue and ledger.

| Recipe | Existing wording |
| --- | --- |
| **IV071 — Pocket Screen** | If this part was saved, apply Weaken 3 instead. |
| **IV099 — Twin Coolant** | If this part was saved, also apply Corrosion 3 to one chosen enemy now. |
| **IV105 — Empty the Tools** | Choose and reserve up to two saved Utility parts until firing. |
| **AD040 — Service Pair** | Each Service Tab repairs 3 Bolt HP while he is active. |
| **AD062 — Packed Lunch** | Restore 10 instead if this part was crafted on an earlier turn. |
| **AD097 — Supply Courier** | Consume two unused Utility parts from reserve as an additional cost. |
| **NO055 — Split Battery** | Each Small Cell gives 2 Charge when activated. |

## C04

**Shield granted during preparation, collection or after Fire — conflict · R03**

The text grants active Shield before End Turn, either directly or through a trigger that can occur in preparation. Treating this as pending Shield would change the printed effect and is not silently assumed.

**For our review:** Review how each intended defence benefit should fit the selected End Turn activation. MA004 is a starter recipe.

Exact excerpts below; full effects and costs remain in the catalogue and ledger.

| Recipe | Existing wording |
| --- | --- |
| **SH033 — Clean Wash** | Gain 3 Shield. |
| **SH045 — Return Spring** | After the main hit, gain 2 Shield per Ammo part consumed by the shot, up to 12. |
| **SH064 — Clean Screen** | Gain 5 Shield. |
| **SH065 — Salvaged Cover** | Gain 10 Shield now and 1 Iron at the start of next turn. |
| **SH095 — Quiet Round** | Gain 5 Shield for each living enemy that does not, up to 15 Shield. |
| **SH104 — Careful Repairs** | If no enemies intend to attack this round, also gain 8 Shield. |
| **SH108 — Patient Sorting** | After the haul, gain 6 Shield. |
| **SH111 — Perfect Packing** | If it uses at least 10 Ammo parts, also gain 15 Shield after it fires. |
| **MA004 — Quick Vent** | Gain 5 Shield. |
| **MA047 — Covering Impact** | Gain 8 Shield after the main shot's impact, before enemy actions. |
| **MA078 — Dense Pack** | If the shot consumes at least five Ammo parts, lose 3 Heat and gain 6 Shield after impact. |
| **MA101 — Emergency Forge Rivets** | For the rest of this round, after each part activation that costs HP, gain 5 Shield, up to 15 Shield. |
| **MA113 — Winter Furnace** | Add 30 damage to this round's main shot and gain 20 Shield. |
| **MA116 — Last Reserve** | Gain 6 Heat and 24 Shield, and add 18 damage to your next main shot this round only. |
| **IV011 — Guarded Leak** | Gain 3 Shield after firing. |
| **IV036 — Read the Ranks** | If some living enemies currently show attack intent and some do not, also gain 4 Shield. |
| **IV038 — Safe Loading** | Gain 4 Shield now. |
| **IV066 — Strip and Save** | Gain 7 Shield now and 2 Carbon at your next turn's start. |
| **IV079 — Covering Needle** | After this turn's main shot, gain 2 Shield for each saved Ammo part it consumed, up to 10 Shield. |
| **IV098 — Sacrifice the Payload** | Apply Mark 10 to one enemy now and gain 6 Shield. |
| **AD024 — Trade Places** | Gain 8 Shield. |
| **AD042 — Overflow Weld** | Gain Shield equal to the amount of that repair which exceeded his missing HP. |
| **AD049 — Full Service** | If he is at full HP afterward, gain 6 Shield. |
| **AD054 — Welded Recoil** | Gain 7 Shield and add 10 damage to this shot. |
| **AD056 — Guard Rotation** | Gain 6 Shield. |
| **AD060 — Busy Workshop** | If you have used at least three Helper parts this turn, gain 8 Shield and add 8 damage to this shot. |
| **AD068 — Even Repairs** | Repair 6 Bolt HP if he is active and gain 6 Shield. |
| **AD078 — Borrowed Time** | Restore Bolt to 8 HP if he is disabled and gain 8 Shield. |
| **AD082 — Roadside Rebuild** | Gain 8 Shield if he was disabled before this Utility recipe was used. |
| **AD103 — Trip Alarm** | Until the start of your next turn, the first time Bolt becomes disabled, apply Weaken 9 to every enemy and gain 10 Shield. |
| **AD107 — Dark Shift** | For this fight, the first time Bolt becomes disabled after installing this reserve, gain 22 Shield. |
| **AD114 — Guard the Gate** | Gain 18 Shield. |
| **AD116 — Crew Contract** | For this fight, the first Helper part you activate each turn grants 8 damage to your next main shot that turn only and grants 5 Shield. This is one bonus per turn, not per hit or per target. Duplicate links do not stack. |
| **AD119 — Stand Back Up** | Each trigger grants 6 Shield. |
| **AD120 — Every Last Rivet** | If Bolt is active at Fire, gain 1 Shield per other Ammo part consumed, up to 20 Shield, after impact. |
| **NO036 — Shield Needle** | After impact, gain 4 Shield, or 8 Shield if the main target still has Shield after main damage. |
| **NO041 — Twin Bridge** | Gain 6 Shield once if at least two recipes become ready from this effect. |
| **NO054 — Emergency Cell** | Gain 5 Charge and 6 Shield. |
| **NO092 — Total Recall** | Gain 6 Shield once if this removes at least 2 counters in total. |
| **NO095 — Barrel Reserve** | If you paid exactly 6, also gain 6 Shield. |
| **NO111 — Full Restart** | Gain 6 Charge and 16 Shield now. |

## C05

**Fresh recipe Shield at a later turn's start — conflict · R03**

The recipe directly grants active Shield at collection/turn start instead of End Turn. A delayed grant is not retention, but it still needs an authorised Shield-timing source; permanent-upgrade exceptions do not automatically apply to recipes.

**For our review:** Review the intended delayed defence under the established activation boundary; do not reinterpret it as stored parts automatically.

Exact excerpts below; full effects and costs remain in the catalogue and ledger.

| Recipe | Existing wording |
| --- | --- |
| **SH034 — Early Cover** | At the start of next turn, gain 8 Shield. |
| **SH096 — Two-Stage Cover** | Gain 12 Shield now and another 12 at the start of next turn. |
| **SH115 — Shelter Reserve** | At the start of each of your next three turns, gain 18 Shield. |
| **MA016 — Spare Lid** | Gain 5 Shield now and 5 Shield at the start of next turn, after old Shield clears. |
| **MA040 — Long Cooling** | Gain 7 Shield now and 7 Shield at the start of next turn, after old Shield clears. |
| **MA092 — Pressure Debt** | At the start of next turn, gain 8 Shield after old Shield clears. |
| **MA118 — Hammer and Anvil** | At the start of next turn, gain 24 Shield after old Shield clears. |
| **IV059 — Late Cover** | If at least one saved part remains in reserve at the end of this enemy phase, gain 12 Shield at the start of next turn, after old Shield clears. |
| **IV097 — Quiet Workshop** | For this fight, at your turn's start, gain 2 Shield for each saved part in reserve, up to 6 Shield. |
| **IV112 — Tomorrow's Opening** | At your next turn's start, remove all cooldown counters from every recipe currently cooling in memory and gain 12 Shield. |
| **IV120 — Patient Hunter** | For this fight, at your turn's start, if at least four saved parts remain in reserve, gain 5 Shield and apply Corrosion 2 and Mark 4 to one enemy chosen at that turn's start. |
| **AD063 — Shift Change** | Gain 6 Shield now and 8 Shield at the start of your next turn, after normal Shield clearing. |
| **AD066 — Scheduled Cover** | At the start of your next turn, gain 12 Shield after normal Shield clearing. |
| **AD098 — Two Shifts** | At the start of each of your next two turns, gain 8 Shield after clearing and repair 5 Bolt HP if he is active. |
| **NO018 — Field Anchor** | If you end this enemy phase with at least 2 Charge, gain 6 Shield at the start of next turn, after old Shield clears. |
| **NO034 — Late Field** | At the start of next turn, gain 6 Shield and 2 Charge, after old Shield clears. |
| **NO056 — Standing Charge** | For this fight, if you end an enemy phase with at least 6 Charge, gain 5 Shield at the start of the following turn, after old Shield clears. |
| **NO098 — Field Reservoir** | At the start of next turn, gain 2 Shield per Charge paid after old Shield clears, and recover half the Charge paid, rounded down. |
| **NO113 — City Grid** | For this fight, at the start of each turn after the current turn, gain 2 Shield per Charge held before that turn's Charge gains, up to 16 Shield, after old Shield clears. |

## C06

**Shield-part activation expected before a shot — conflict · R03**

The effect expects Shield parts to have activated in preparation, or arms a later shot trigger only when the Shield part activates. Under End-Turn-only activation there is no later ordinary Fire in that player turn.

**For our review:** Review which event these bonuses should measure. Staging a part and activating it are already different events.

Exact excerpts below; full effects and costs remain in the catalogue and ledger.

| Recipe | Existing wording |
| --- | --- |
| **SH044 — Guarded Loading** | Add 8 damage, plus 5 more if you used at least three Shield parts this round. |
| **SH062 — Impact Catch** | After your next main shot this round deals HP damage, gain another 7 Shield once. |
| **MA034 — Backplate** | After this round's main shot, gain 4 more Shield if it consumed at most one Ammo part. |
| **IV107 — Needle Thread** | After this turn's main damage, deal a separate 5 damage to the main target for each saved Shield part you activated during this planning phase, up to 15 separate damage in one support hit. |
| **AD076 — Rivet Collectors** | Each of the next four Shield parts you use this planning phase repairs 2 Bolt HP if he is active. |

## C07

**Shield conversion at the old reset point — conflict · R02**

NO060 schedules a next-turn action 'before clearing or retention', but the selected default reset has already occurred at enemy-turn end. Its stated reset ordering is obsolete even when an upgrade retained some Shield.

**For our review:** Review the conversion's intended trigger against the settled reset point.

Exact excerpts below; full effects and costs remain in the catalogue and ledger.

| Recipe | Existing wording |
| --- | --- |
| **NO060 — Field Pocket** | At the start of next turn, before clearing or retention, spend up to 9 remaining Shield and gain 1 Charge per full 3 Shield spent. |

## C08

**Removed Charged Barrel payment — conflict · R04**

The row refers to a separate barrel Charge payment at Fire, which belonged to the superseded independent gun-damage trait. Positive-payment branches lose their trigger, no-payment branches become automatic, and 'after payment' or exclusion clauses refer to a removed operation.

**For our review:** Review the affected condition or refund without restoring an independent gun-damage payment. This does not depend on accepting Residual Current's proposed numbers.

Exact excerpts below; full effects and costs remain in the catalogue and ledger.

| Recipe | Existing wording |
| --- | --- |
| **NO005 — Live Wire Tip** | Add 5 more if Charge is at least 6 at Fire, after the barrel's optional Charge payment. |
| **NO006 — Grounded Slug** | Add 6 more if Charge is 0 at Fire, after the barrel's optional Charge payment. |
| **NO017 — Hot Contact** | The barrel's Fire payment does not count. |
| **NO025 — Full Cell Tip** | Add 12 more if Charge is 12 at Fire, after the barrel's optional Charge payment. |
| **NO028 — Contact Mark** | After impact, apply Mark 3 to the main target, or Mark 7 if the barrel paid at least 2 Charge at Fire. |
| **NO035 — Even Current** | Add 6 more if you have an even positive amount of Charge at Fire, after the barrel's optional Charge payment. |
| **NO039 — Loose Contact** | If you pay no Charge through the barrel at Fire, add 4 more damage. |
| **NO048 — Discharge Record** | The barrel's Fire payment does not count. |
| **NO058 — Discharge Gate** | The barrel's Fire payment does not count. |
| **NO075 — Shot Ground** | Gain 2 Charge after impact if the barrel paid at least 2 Charge at Fire. |
| **NO081 — Patient Coil** | For this fight, gain 3 Charge after any main-shot impact for which you paid no Charge through the barrel at Fire. |
| **NO106 — Shot Receipt** | At the start of next turn, recover the Charge paid by Charged Barrel for this shot, up to 3 Charge, and gain 1 Copper. |
| **NO110 — Reserve Coil** | For this fight, at Fire add 3 damage per Charge still held after the barrel's optional payment, up to 24 extra damage. |

## Q01

**Shield replenishment during enemy actions — clarification · R03**

These gains happen after a Shield part was activated at End Turn, or through an ongoing effect during the enemy phase. The settled activation/reset rules alone do not fully specify whether this reactive replenishment is allowed. It is not labelled a definite retention violation.

**For our review:** Review secondary Shield replenishment as one group, keeping it separate from preparation-time grants and next-turn retention.

Exact excerpts below; full effects and costs remain in the catalogue and ledger.

| Recipe | Existing wording |
| --- | --- |
| **MA044 — Tempered Lip** | The first time an enemy attack reduces your Shield from a positive amount to 0 this round, gain 6 Shield after that attack finishes. |
| **MA071 — Rescue Valve** | After the first enemy attack that causes you to lose HP this round, lose 5 Heat and gain 12 Shield. |
| **MA090 — Burning Shelter** | For this fight, after an enemy takes a Burn tick, gain 2 Shield, up to 6 Shield per enemy phase. |
| **MA115 — Iron Refuge** | After each enemy attack this round finishes, restore the Shield that attack removed, up to 24 Shield restored in total this round. |
| **IV021 — Two Stage Cover** | Gain 5 Shield now. Choose an enemy that currently has Corrosion, if any. The next time its Corrosion removes HP this round, gain 6 Shield, even if that tick kills it. This triggers once and expires at round end. |
| **NO042 — Recovering Field** | After the first enemy attack this round finishes, restore up to 8 Shield that attack removed. |
| **NO083 — Restarting Field** | The first time an enemy attack reduces positive Shield to 0 this round, gain 16 Shield after that attack finishes. |
| **NO117 — Field Shelter** | Until the end of this enemy phase, after each enemy's first attack against you, gain 4 Shield and 1 Charge. |

## Q02

**Preparation-time active-Shield dependencies — clarification · R03**

The recipe spends or checks active Shield while preparing a shot. Ordinary staged Shield cannot supply it. An explicit permanent-upgrade grant/retention can make the effect legal, so this is a build dependency to review, not an automatic rule violation.

**For our review:** Check whether the intended recipe should require an active-Shield upgrade. Do not confuse pending parts with an available Shield balance.

Exact excerpts below; full effects and costs remain in the catalogue and ledger.

| Recipe | Existing wording |
| --- | --- |
| **SH085 — Pressure Release** | Spend up to 15 Shield now. |
| **MA027 — Armour Chips** | Pay 6 Shield when activated. |
| **MA037 — Shield Boiler** | Pay 5 Shield when activated, then gain 4 Heat. |
| **MA061 — Pressed Shield** | When activated, choose and pay 4 to 12 Shield. |
| **MA068 — Braced Shot** | If you have at least 12 Shield at Fire, also add 25% to this shot's damage. |
| **MA081 — Boiler Oath** | For this fight, the first two times each round you pay at least 4 Shield as a part activation cost, gain 2 Heat after paying. |
| **MA112 — Walking Foundry** | For this fight, at Fire add half your current Shield, rounded down, to the main shot's damage, up to 20 extra damage. |
| **MA118 — Hammer and Anvil** | Pay 12 Shield when activated. |
| **AD095 — Press the Plate** | Spend 12 of your current Shield as an activation cost. |
| **NO010 — Shield Tap** | Pay 6 Shield when activated, then gain 5 Charge. |
| **NO079 — Field Price** | Pay 10 Shield when activated. |
| **NO116 — Empty the Field** | When activated, choose and pay 10 to 20 Shield. |

## Q03

**Order within End Turn's Shield activation — clarification · R03**

The value or legality depends on another Shield part having activated, or on Shield accumulated so far. Values add at End Turn, but the owner has not selected ordering/payment validation for these interdependent Shield parts.

**For our review:** Establish the activation-order reading needed by these five rows; this also informs Heat/Charge payments across a staged Shield build.

Exact excerpts below; full effects and costs remain in the catalogue and ledger.

| Recipe | Existing wording |
| --- | --- |
| **MA028 — Rivet Screen** | Gain 2 more for each other Shield part already activated this round, up to 6 extra Shield. |
| **MA075 — Spare Furnace Door** | You cannot activate this part unless at least one other Shield part has already been activated this round. |
| **IV094 — Reinforced Position** | Gain Shield equal to your current Shield, up to 16. |
| **NO029 — First Field** | Gain 5 more if no other Shield part has been activated this round. |
| **NO038 — Field Peg** | For each other Shield part already activated this round, gain 1 Charge, up to 3 Charge. |

## Q04

**Remaining Shield sampled at enemy-phase end — clarification · R02**

The bonus reads or spends remaining Shield at/after enemy-phase end, where the selected reset also occurs. A precise snapshot/trigger ordering is needed; the recipe does not necessarily retain Shield.

**For our review:** Review the timing of the read/payment relative to reset without changing the reset itself.

Exact excerpts below; full effects and costs remain in the catalogue and ledger.

| Recipe | Existing wording |
| --- | --- |
| **SH066 — Sweep the Plates** | At the end of the enemy phase, spend up to 9 remaining Shield. |
| **SH091 — Wall Casting** | After this enemy phase, if any Shield remains, deal 6 support damage to each surviving enemy that attacked you this round. |
| **MA055 — Reclaimed Plate** | At the start of next turn, gain 1 Iron for every full 5 Shield remaining from your total Shield pool at the end of this enemy phase, up to 3 Iron. |
| **IV092 — Untouched Cover** | After this enemy phase, if that enemy is still alive, apply Mark to it equal to your remaining Shield, up to Mark 8. |
| **NO044 — Charge Cage** | If at least 5 Shield remains at the end of this enemy phase, gain 3 Charge. |

## Q05

**Utility item wording left after conversion — clarification · R01**

The immediate effect is usable without an inventory item, but leftover 'pack crafted' or 'copies of its part' wording describes the former Utility-item model. Unlike C03, the main effect does not require storing the Utility.

**For our review:** Review wording/cap ownership; no numerical retuning is implied by this finding.

Exact excerpts below; full effects and costs remain in the catalogue and ledger.

| Recipe | Existing wording |
| --- | --- |
| **SH029 — Copper Recovery** | The pack's Iron cost was spent when it was crafted. |
| **SH030 — Glass Recovery** | The pack's Copper cost was spent when it was crafted. |
| **SH031 — Iron Recovery** | The pack's Carbon cost was spent when it was crafted. |
| **SH074 — Quick Patch** | This recipe can restore at most 8 HP during one fight, across all copies of its part. |

## Q06

**Recipe crafting versus immediate Utility use — clarification · R01**

The effect counts crafting, crafting costs or a crafted numeric-cooldown recipe. It does not explicitly say whether immediate Utility recipe Uses count. Part-producing crafts and all recipe Uses are different sets after the Utility conversion.

**For our review:** Make the counted event explicit. Existing clauses that explicitly count physical parts can remain narrower.

Exact excerpts below; full effects and costs remain in the catalogue and ledger.

| Recipe | Existing wording |
| --- | --- |
| **SH023 — Wire Lattice** | Gain 5 Shield, plus 1 for each Copper you have spent crafting this round, up to 10 total Shield. |
| **SH070 — Iron Saver** | Next turn, the first recipe you craft that costs at least 2 Iron costs 1 less Iron. |
| **MA069 — Furnace Receipt** | At the start of next turn, gain 1 Iron for each different recipe you crafted after using this Utility recipe this round, up to 4 Iron. |
| **NO062 — Second Contact** | Add 10 more if you have crafted the same numeric-cooldown recipe at least twice this round. |
| **NO071 — Circuit Notes** | At the start of next turn, gain 1 Copper for each different numeric-cooldown recipe you crafted after using this Utility recipe this round, up to 4 Copper. |
| **NO109 — Circuit Heart** | For this fight, after you craft a numeric-cooldown recipe for a second or later time in the same round, grant 4 damage to your next main shot that round only, up to 20 bonus damage granted per round. |

## Q07

**Cooling rewards restricted to parts — clarification · R01**

The reward counts cooling performed by a part rather than by a Utility. That can be intentional: NO057 is an Ammo part that really cools recipes. Consequently NO094's old superseded marker is not enough to prove a contradiction.

**For our review:** Review intended eligible sources. Do not automatically broaden the reward to Utilities or mistake this for non-global cooling.

Exact excerpts below; full effects and costs remain in the catalogue and ledger.

| Recipe | Existing wording |
| --- | --- |
| **NO080 — Pulse Map** | Add 4 damage to this round's main shot for each different recipe from which cooldown counters were removed by parts this round, up to 16 damage. |
| **NO094 — Charge Receipt** | For this fight, the first time each round you remove a recipe's last cooldown counter with a part, gain 2 Charge after that part finishes. |

## Q08

**Status recipient implied rather than stated — clarification · R05**

The effect names a main hit/target condition but omits the recipient of one status application. The owner requires recipe-defined status scope, especially when a shot has extra targets. This is a wording finding, not a proposal that the status should affect everyone.

**For our review:** Make the intended recipient explicit when reviewing these rows; no blanket main-target or all-hit default has been applied.

Exact excerpts below; full effects and costs remain in the catalogue and ledger.

| Recipe | Existing wording |
| --- | --- |
| **SH013 — Hot Filling** | Add 4 damage and apply Burn 2 after the main hit. |
| **SH014 — Acid Filling** | Add 2 damage and apply Corrosion 2 after the main hit. |
| **SH016 — Joint Snare** | Add 5 damage and apply Weaken 4 after the main hit. |
| **SH055 — Short Fuse** | After the main hit, apply Weaken 3. |
| **SH086 — Clean Entry** | If the main target has no Shield, apply Corrosion 6 after the main hit. |
| **IV006 — Painted Acid** | After main damage, apply Corrosion 2 if the main target had Mark at Fire, before any Mark was spent. |
| **IV008 — Under the Plate** | After main damage, apply Corrosion 3 if the main target had any Shield immediately before main damage. |
| **IV015 — Read the Guard** | After main damage, apply Weaken 4 if the main target shows attack intent; otherwise apply Corrosion 3. |
| **IV016 — Follow the Line** | After main damage, apply Weaken 3 if the main target had Mark at Fire, before any Mark was spent. |
| **IV041 — Acid Booster** | After main damage, apply Corrosion 3, then apply 2 more if the main target now has at least 6 Corrosion. |
| **IV047 — Settled Powder** | After main damage, apply Corrosion 1 for each round since this part entered reserve, up to Corrosion 4. |
| **IV084 — Stored Acid** | After main damage, apply Corrosion 8 if this part was saved, otherwise Corrosion 4. |
| **IV085 — Break the Barrel** | After main damage, apply Weaken equal to the Mark consumed by this main shot, up to Weaken 8. |
| **AD038 — Screwdriver Tip** | After impact, apply Mark 3 if Bolt is active. |

## Q09

**Canonical sale basis for batches and generated subparts — clarification · R06**

The recipe creates multiple parts or a generated subpart without a recorded standard one-part production requirement for every resulting type. Preserve main-recipe-based resale; do not substitute the discounted producer's cost or divide a mixed batch arbitrarily.

**For our review:** Record canonical one-part valuation references/requirements for these outputs. Known fresh copies inherit their original type's basis; generic copying alone is not a pricing conflict.

Exact excerpts below; full effects and costs remain in the catalogue and ledger.

| Recipe | Existing wording |
| --- | --- |
| **SH076 — Plate Recasting** | Make one Recast Plate Slug adding that amount as shot damage, up to 14; it has no other effect. |
| **SH126 — Salvage Foundry** | On a Perfect manual grab next round, also receive 1 Salvage Slug and 1 Salvage Shield Plate in reserve after collection, with no further material cost. |
| **MA038 — Rivet Bundle** | Each Warm Rivet adds 4 damage to the main shot and gives 1 Heat after impact. |
| **MA107 — Warm Repair Bench** | At the start of next turn, gain 2 Ready Plates. |
| **MA119 — Spare Foundry** | At the start of next turn, gain 2 Siege Slugs and 2 Siege Plates. |
| **IV088 — Paired Needles** | Each part adds 6 damage and applies Corrosion 2 to the main target after main damage. |
| **IV118 — Spare Needle Bench** | For this fight, when a main shot consumes at least two saved Ammo parts, receive 1 Plain Needle at your next turn's start. |
| **AD017 — Spare Bolts** | Each Small Rivet adds 4 damage when used in a shot. |
| **AD083 — Work Order** | At the start of your next turn, receive 2 Small Rivets and 1 Guard Tab. |
| **AD117 — Night Shift** | At the start of your next turn, restore 10 Bolt HP and receive 2 Heavy Rivets and 2 Guard Tabs. |
| **NO031 — Copper Pins** | Each Copper Pin adds 3 damage and applies Mark 2 to the main target after impact. |
| **NO063 — Twin Field** | Each Small Field Ring gives 6 Shield when activated. |
| **NO107 — Shield Press** | At the start of next turn, gain 2 Stored Field Plates. |
| **NO120 — Pocket Generator** | At the start of next turn, gain 2 Pulse Tips and 2 Charged Plates. |

## Q10

**Spread Modifier lifecycle and placement order — clarification · R07**

The row is a Modifier used/consumed in planning, while the selected spread rule resolves contributions in their parts' bullet-placement order and Load remains reversible. The catalogue still leaves non-Ammo lifecycle/accounting open; the ordering record and reservation behaviour need specifying.

**For our review:** Define how these consumed Modifier effects retain the player's chosen bullet order through Load/unload. This is not permission to refund resolved effects.

Exact excerpts below; full effects and costs remain in the catalogue and ledger.

| Recipe | Existing wording |
| --- | --- |
| **SH005 — Split Outlet** | The next main shot also hits it for 50% of the shot's damage. |
| **SH051 — Three-Way Outlet** | The next main shot also hits each for 40% of the shot's damage. |
| **SH082 — Wide Burst** | The next main shot hits every other living enemy for 50% of its damage. |
| **SH110 — Full-Spread Outlet** | The next main shot hits every other living enemy for 100% of its damage. |
| **MA041 — Furnace Fork** | At Fire, choose one extra enemy. |
| **MA111 — Foundry Split** | Every extra enemy receives 60% of this round's shot damage before target-specific Mark and defence, then applies its own protection. |
| **IV076 — All Eyes** | This turn's main shot also deals 30% of its calculated damage before target-specific Mark and defence to each other enemy that has Mark at Fire, rounded down. |
| **NO050 — Branching Coil** | At Fire, choose one extra enemy. |
| **NO082 — Double Branch** | At Fire, choose up to two extra enemies. |
| **NO115 — City Arc** | This round's main shot deals 60% of its damage before target-specific Mark and defence to every other enemy, without copied payloads. |

## Cases deliberately not called rule conflicts

- Percentages, half-values and multipliers remain legal: the selected integer arithmetic floors their results. They are not automatically fractional-damage violations.
- The seven player-HP-cost recipes already state survival at 1 HP. Bolt HP costs and disablement are separate from player death. No additional player-HP-cost conflict was found.
- A numeric cooldown can be cleared and reused in the same turn. Self-cooling is allowed; identical effects or strong resource loops are balance concerns, not automatically violations. No new targeted-cooling or no-cooldown-refresh violation was found.
- Per-recipe damage/effect caps, minimum part requirements and explicit non-stacking clauses do not impose a global shot or part-count cap.
- A Utility may mark or sacrifice a real Ammo/Shield/Helper part already in reserve. That is different from requiring a nonexistent Utility part. MA030, MA066 and IV102 illustrate legal dependencies on real parts.
- Explicit on-kill transfers or newly triggered support attacks differ from automatically redirecting an already assigned hit whose target died. No new forced-retargeting contradiction was identified.
- Start-of-next-turn resource deliveries and temporary collection enhancements follow the existing fight-end expiry rule. They do not create an extra ordinary haul or another Precision attempt.
- SH060's explicit loss of remaining Shield is not a recipe retention exception; it can be a downside when an upgrade would otherwise preserve Shield. A weaker or redundant effect is not itself a rule conflict.
- The existing same-effect cooling pairs and relative recipe prices remain balance work. Renaming Burn/Corrosion/Mark/Weaken to the proposed robot vocabulary is a separate editorial migration.

## Shared specification gaps, not 606 separate questions

Modifier/Helper lifecycle, Shield-part payment ordering, final secondary-effect timing and precise stat sampling are still partly draft. Q03/Q04/Q10 identify concrete rows that expose these gaps. A global clarification may resolve several entries; it must not silently rewrite the selected End Turn or reset rules. Main-shot singular wording is generally readable through the existing next-shot/default-duration rules; it is not automatically a one-shot-per-turn restriction. This review does not label every ordinary row as conflicting merely because the eventual engine still needs an effect-resolution order.

**Next action:** review the grouped conflicts with Klaus and record chosen resolutions. Only then change affected recipe rows and re-audit them. This report selects no replacement designs and authorizes no gameplay implementation.
