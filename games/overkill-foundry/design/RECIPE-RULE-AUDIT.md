# Recipe rule audit — review together

**17 September 2026 · all 606 recipes reviewed · installed Shield rule recorded; two recipe effects corrected**

Reviewed all recipe rows from `5e73bd8` against the settled owner rules, updated for automatic protection from installed Shield parts. Found **13 recipes with a definite conflict or obsolete dependency**, **110 additional recipes needing wording/dependency clarification**, and **483 with no direct conflict identified**. A recipe can have findings in several groups; group counts therefore overlap. Clarifications on a conflicting recipe are also retained.

**Owner resolutions:** C04/C05 use ordinary Shield parts and explicit future deliveries. Following Folding Brace, the owner approved Spare Metal Brace's 8-now/conditional-5-next-round schedule and authorized analogous replacements. All twelve remaining C01 retention recipes now deliver fresh parts under their stated conditions; SH034's old expiry is resolved in C10. Parts left installed protect when enemies attack, and active Shield resets at enemy-turn end. Parts remain removable/saveable; a delivery does not copy the source's delivery or secondary effects.

**Latest timing clarification:** Field Pocket counts remaining Shield before reset and delivers recorded Charge next round. The same pre-reset reading/payment now resolves five Q04 rows. Existing reward timing and explicit Shield costs are preserved.

This is an audit for joint review. The owner replaces Shield activation with protection from parts installed when enemies attack. IV107 and SH044 now count installed Shield parts at Fire, preserving saved-part eligibility where stated, values, costs and cooldowns. Other 604 rows are unchanged. Reactive grants can protect subsequent attacks, resolving Q01. Three remaining shot/install hooks move from C06 to Q12; Q13 records implicit secondary costs/snapshots that still require timing reconciliation. Earlier rule corrections remain selected. These findings do not imply tested balance or a finished secondary-effect specification. The four new inherent abilities remain proposals; their values are not treated as owner rules. No direct conflict identified means the row passed this written-rule review, not that it is implementation-ready, balanced or proven in every combination.

**Resolved starter:** MA004 Quick Vent automatically loads its 5-Shield part; the player may use it at this End Turn or remove and save it. SH005 Split Outlet still needs Q10 spread-Modifier lifecycle clarification. **Next joint review:** C08, references to the removed Charged Barrel payment. Shield protection and installed-part counting are settled; secondary costs/hooks are grouped in Q03/Q12/Q13.

## Coverage

| Pool | Reviewed | Conflict | Clarification only | No direct conflict identified |
| --- | ---: | ---: | ---: | ---: |
| SH | 126 | 0 | 23 | 103 |
| MA | 120 | 0 | 31 | 89 |
| IV | 120 | 0 | 16 | 104 |
| AD | 120 | 0 | 9 | 111 |
| NO | 120 | 13 | 31 | 76 |

All rows were read for: output kind and Utility lifecycle; installed Shield protection, reset and grants; per-copy cooldown, global cooling and no-cooldown uses; part-based damage, multiple shots and modifier duration; status recipients, hit order, explicit copy/stack limits; whole-number arithmetic and player HP costs/death; resource/part persistence, collection count and Precision protection; part consumption, saved-part age and canonical sale basis; removed character dependencies; proposed traits are not owner rules.

The [complete per-recipe ledger](analysis/recipe_rule_audit.json) contains every ID, name, original row, source line, disposition and finding references. The [audit renderer](analysis/recipe_rule_audit.py) verifies that all printed recipe rows still match the reviewed revision before regenerating this document. Rule prose may change with recorded owner clarifications; the ledger hashes the current full catalogue. It validates coverage and evidence preservation; it does not discover or prove semantic conflicts automatically.

## Findings index

| Group | Classification | Recipes | Topic |
| --- | --- | ---: | --- |
| [C01](#c01) | resolved | 12 | Recipe-granted Shield retention |
| [C02](#c02) | resolved | 5 | Copy effects and scheduled Shield grants are not Utility items |
| [C03](#c03) | resolved | 2 | Ordinary parts replace Utility-item sacrifices |
| [C04](#c04) | resolved | 41 | Shield granted during preparation, collection or after Fire |
| [C05](#c05) | resolved | 19 | Explicit future-turn Shield schedules |
| [C06](#c06) | resolved | 2 | Shot bonuses count installed Shield parts |
| [C07](#c07) | resolved | 1 | Field Pocket counts before reset and rewards next round |
| [C08](#c08) | conflict | 13 | Removed Charged Barrel payment |
| [C09](#c09) | resolved | 1 | Folding Brace owner-selected replacement |
| [C10](#c10) | resolved | 1 | Early Cover uses ordinary Shield-part expiry |
| [C11](#c11) | resolved | 1 | Pocket Screen counts saved ordinary parts |
| [C12](#c12) | resolved | 2 | Equivalent Utility bonuses count saved ordinary parts |
| [C13](#c13) | resolved | 2 | Split Utilities combine into one immediate effect |
| [Q01](#q01) | resolved | 11 | Reactive installed Shield protects subsequent attacks |
| [Q02](#q02) | clarification | 12 | Preparation-time Shield readings and payments |
| [Q03](#q03) | clarification | 5 | Installed Shield order and secondary payments |
| [Q04](#q04) | resolved | 5 | Remaining Shield sampled at enemy-phase end |
| [Q05](#q05) | clarification | 4 | Utility item wording left after conversion |
| [Q06](#q06) | clarification | 6 | Recipe crafting versus immediate Utility use |
| [Q07](#q07) | clarification | 2 | Cooling rewards restricted to parts |
| [Q08](#q08) | clarification | 14 | Status recipient implied rather than stated |
| [Q09](#q09) | clarification | 27 | Canonical sale basis for batches and generated subparts |
| [Q10](#q10) | clarification | 10 | Spread Modifier lifecycle and placement order |
| [Q11](#q11) | clarification | 4 | Other Utility conversions and generated outputs |
| [Q12](#q12) | clarification | 3 | Installed Shield parts and shot/install hooks |
| [Q13](#q13) | clarification | 28 | Implicit Shield secondary-effect costs and stat sampling |

## Rule references

These summaries refer to the selected-rule sections in [RECIPE-CATALOGUE.md](RECIPE-CATALOGUE.md) and the owner chronology in [DECISIONS.md](../DECISIONS.md). Draft conventions are used to explain dependencies, not promoted into owner decisions.

- **R01:** Utilities activate on recipe Use without a stored Utility item. Production means crafting from resources; copying an existing part is a Utility effect, not resource-based production of that copy. Copying does not require paying the copied part's normal production cost; printed recipe-use costs are unchanged. Ordinary Shield-value grants follow the selected automatic loading, removal/storage and End Turn rules. Other named-part conversion/grant classifications remain clarification work, not proven conflicts merely because a part appears.
- **R02:** Active Shield resets at enemy-turn end by default; only explicit permanent upgrades provide retention exceptions. End-phase remaining-Shield readings/payments happen after enemy actions and before reset. Record a deferred reward then and deliver it at its stated time; explicit payments still spend available Shield.
- **R03:** Installed Shield parts protect automatically when enemies attack; neither Fire nor End Turn activates them. End Turn only ends the player turn. Parts are counted at a recipe's stated event without being consumed by counting. Installed values add and damage depletes remaining Shield across attacks; normal enemy-turn-end reset and explicit upgrade exceptions remain. Grants install ordinary removable parts at their stated time, protecting subsequent attacks without retroactive blocking or refilling prior loss. Implicit secondary costs/snapshots remain draft; explicit End Turn conditions retain that clock and check installed source parts.
- **R04:** The gun contributes zero innate damage/effects. Old Hot Barrel/Charged Barrel independent damage grants are superseded. Recipe-authored effects and explicit Utility bonuses are distinct from gun base damage.
- **R05:** Each recipe specifies its own status recipients; there is no blanket main-target-only or all-hit-target default.
- **R06:** A part's sale value uses the main recipe's normal one-part ingredient requirement, current resource prices, a 50% factor and whole-credit floor. Discounts/copies do not change that basis.
- **R07:** Spread contributions are separate hits resolved in contributing-part placement order. Load can be undone before Fire; resolved effects/crafting are not automatically refunded.
- **R08:** Enemy Weaken N reduces each hit of its attacks by N, minimum zero, without being consumed by attacks. Reduce N by 1 once after the full enemy phase, including non-attacking rounds. The player counterpart is a draft mirror using the existing main-shot calculation scope and a player-action-phase-end tick. No percentage or rounding change was selected.

## C01

**Recipe-granted Shield retention — resolved · R02**

The owner authorized analogous fresh-part replacements after approving Spare Metal Brace. These twelve rows now schedule regular Shield parts instead of retaining active protection. Costs, cooldowns and meaningful conditions are preserved. Saved source parts retain explicit schedule clocks, checking that they are installed at those times. Last Wall checks leftover Shield before reset for its conditional cooling reward; its future Shield part is a separate fixed grant.

**For our review:** Retention conflicts are resolved. Q09 retains standard one-part valuation mapping for the added outputs; numerical balance remains untested.

Exact excerpts below; full effects and costs remain in the catalogue and ledger.

| Recipe | Existing wording |
| --- | --- |
| **SH057 — Spare Metal Brace** | If at least 3 Iron remain in your raw pool after paying the recipe cost, schedule a new regular 5-Shield part to load at the beginning of the next round. |
| **SH092 — Hinged Wall** | When this part activates at End Turn, gain 15 Shield and schedule a new regular 15-Shield part to load at the beginning of the next round. |
| **SH113 — Last Wall** | When this part activates at End Turn, gain 38 Shield and schedule a new regular 20-Shield part to load at the beginning of the next round. |
| **MA048 — Holdfast** | Schedule a new regular Shield part to load at the beginning of the next round: 12 Shield if you paid, or 4 Shield if you did not. |
| **MA099 — Settled Slag** | At the beginning of the next round, if Heat is 0 before that round's start-of-turn Heat gains, automatically load a new regular 14-Shield part. |
| **IV019 — Feint Plate** | When this part activates at End Turn, gain 7 Shield and schedule a new regular 3-Shield part to load at the beginning of the next round. |
| **IV057 — Stored Wall** | Schedule a new regular 4-Shield part to load at the beginning of the next round. |
| **IV061 — Hold the Gap** | If any enemy currently has at least 6 Mark at that activation, schedule a new regular 5-Shield part to load at the beginning of the next round. |
| **IV090 — Stockroom Wall** | If at least three saved parts are currently in reserve at that activation, schedule a new regular 14-Shield part to load at the beginning of the next round. |
| **AD074 — Hold the Panel** | At the beginning of the next round, automatically load a new regular 8-Shield part if Bolt is active then. |
| **AD092 — Shared Scaffold** | At the beginning of each of the next two rounds, automatically load a new regular 12-Shield part if Bolt is active then. |
| **NO087 — Field Lock** | If a Fire in this activation round began with at least 8 Charge, schedule one new regular 12-Shield part to load at the beginning of the next round. |

## C02

**Copy effects and scheduled Shield grants are not Utility items — resolved · R01**

The owner corrected the copying finding: production means resource-based crafting; a Utility copying an existing part is an effect, not crafting that copy from resources. SH071/SH102/SH118 retain their copy eligibility, amounts, timing and printed-effect restrictions. MA107/NO107 are already covered by the selected scheduled ordinary-Shield-part rule. The obsolete conflict markers are removed without changing functional effects, costs or cooldowns. The four other conversion/generated-output rows are clarification cases in Q11, not asserted violations based solely on part output.

**For our review:** These five Utility-boundary findings are resolved. Q09 still covers the two Shield batches' canonical one-part resale references. No additional production-cost payment for a copied part is introduced.

Exact excerpts below; full effects and costs remain in the catalogue and ledger.

| Recipe | Existing wording |
| --- | --- |
| **SH071 — Spare Casting** | At the start of next turn, receive one fresh copy with its printed effect; do not copy later bonuses attached to it. |
| **SH102 — Reserve Pattern** | Receive two fresh copies next turn. |
| **SH118 — Exact Duplication** | Receive two fresh copies next turn with printed effects only. |
| **MA107 — Warm Repair Bench** | At the start of next turn, gain 2 Ready Plates. |
| **NO107 — Shield Press** | At the start of next turn, gain 2 Stored Field Plates. |

## C03

**Ordinary parts replace Utility-item sacrifices — resolved · R01**

The owner approved sacrificing ordinary physical parts instead of nonexistent Utility items. Empty the Tools chooses up to two unused parts saved from earlier rounds for the next shot this turn, consuming them for +7 damage each instead of their normal effects. Selection is editable before Fire; End Turn without firing releases them unused. They still count as parts consumed by the shot. Supply Courier consumes two unused ordinary reserve parts as its additional cost, with no age requirement, and delivers 3 Iron, 2 Copper and 1 Carbon at next-turn start. Categories, material costs and cooldowns are preserved. Saved-item bonus conditions are resolved in C12, and split activations in C13.

**For our review:** The Utility-item sacrifice conflict is resolved. Normal reversible loading applies before Fire; paying Supply Courier's sacrifice is an actual consumption, not a refundable selection. Broader Helper/Modifier lifecycle details and numerical balance remain separate.

Exact excerpts below; full effects and costs remain in the catalogue and ledger.

| Recipe | Existing wording |
| --- | --- |
| **IV105 — Empty the Tools** | Choose up to two unused ordinary parts saved from an earlier round to sacrifice when firing your next shot this turn. |
| **AD097 — Supply Courier** | Consume two unused ordinary parts from reserve as an additional cost. |

## C04

**Shield granted during preparation, collection or after Fire — resolved · R03**

Resolved by the owner's 16 September clarification: a grant automatically loads an ordinary Shield part worth that amount. It may be removed and saved like any part. Parts left installed protect when enemies attack; active Shield resets at enemy-turn end without an automatic grant next round. The original audit incorrectly read these grants as immediate active protection.

**For our review:** MA004 Quick Vent is resolved: automatically load its 5-Shield part, with ordinary removal/storage available. Other findings on these rows remain separate; AD103/AD107/AD119 also have enemy-phase triggers covered by Q01.

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

**Explicit future-turn Shield schedules — resolved · R03**

Resolved at the timing-rule level by the owner's Folding Brace revision: a recipe may schedule a new ordinary Shield part for the beginning of the next round. It loads then, remains removable/saveable and protects automatically against enemy attacks if still installed. This is neither active-Shield retention nor immediate active protection. SH034's expiry wording is now corrected in C10; MA118's active-Shield payment remains in Q02. Other printed values are still draft balance.

**For our review:** Interpret an explicitly scheduled Shield grant as a new ordinary part at its stated trigger. An immediate grant alone does not repeat next round. Preserve each row's stated conditions and other unresolved findings.

Exact excerpts below; full effects and costs remain in the catalogue and ledger.

| Recipe | Existing wording |
| --- | --- |
| **SH034 — Early Cover** | On recipe Use, schedule a new regular 8-Shield part to load at the beginning of the next round. It may be removed and saved normally. If left loaded, it activates at End Turn and its active Shield resets at enemy-turn end under the normal upgrade exceptions. |
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

**Shot bonuses count installed Shield parts — resolved · R03**

The owner replaces Shield activation with automatic protection from parts installed at enemy attack time. Needle Thread counts older saved Shield parts installed at Fire for its existing 5-per-part support hit, capped at 15. Guarded Loading checks for at least three installed Shield parts at Fire for its existing +5 damage. Counting does not consume them. Costs and cooldowns are preserved. The three former shot-hook cases move to Q12; a nonexistent activation phase is no longer grounds to call them impossible.

**For our review:** These installed-count conditions are resolved. Remaining shot-hook timing is recorded separately; do not reopen whether a shield needs activation.

Exact excerpts below; full effects and costs remain in the catalogue and ledger.

| Recipe | Existing wording |
| --- | --- |
| **SH044 — Guarded Loading** | Add 8 damage, plus 5 more if at least three Shield parts are installed in your shield when you Fire this shot. |
| **IV107 — Needle Thread** | When you Fire the next shot this turn, count Shield parts saved from an earlier round that are currently installed in your shield. |

## C07

**Field Pocket counts before reset and rewards next round — resolved · R02**

The owner confirmed that Field Pocket counts total remaining active Shield after enemy actions and before reset, then grants its recorded Charge next round. The clarified row uses 1 Charge per complete 3 Shield, capped at 3 Charge, while retaining 12 Shield, cost and cooldown. The mechanic needs a stored reward amount, not retained active Shield or a next-round Shield read.

**For our review:** The timing issue is resolved by explicit count-before-reset wording. Noor's broader Charge mechanic remains a draft.

Exact excerpts below; full effects and costs remain in the catalogue and ledger.

| Recipe | Existing wording |
| --- | --- |
| **NO060 — Field Pocket** | After enemies finish their actions, just before the Shield reset, count your total remaining active Shield and record 1 Charge per complete 3 Shield, up to 3 Charge. |

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

## C09

**Folding Brace owner-selected replacement — resolved · R03**

SH026's old 6 Shield plus retention of up to 4 is replaced by 4 Shield loaded on recipe Use and a new 6-Shield part loaded at the beginning of the next round. Both are ordinary parts. The second delivery is scheduled by recipe Use, regardless of whether the first part is activated or saved. Cost and cooldown are unchanged; the proposed flat 10 Shield was rejected.

**For our review:** The retention conflict is resolved. Q09 retains the normal one-part resale-basis mapping work for the two generated outputs; no price has been invented.

Exact excerpts below; full effects and costs remain in the catalogue and ledger.

| Recipe | Existing wording |
| --- | --- |
| **SH026 — Folding Brace** | On recipe Use, automatically load a regular Shield part worth 4 Shield. |

## C10

**Early Cover uses ordinary Shield-part expiry — resolved · R03**

SH034 now schedules a regular 8-Shield part for the beginning of the next round. It loads then and may be removed or saved; if left installed, it protects when enemies attack and active Shield resets at enemy-turn end. The obsolete following-turn expiry clause was removed without changing its cost, cooldown or Shield amount.

**For our review:** The old expiry conflict is resolved under the same ordinary-part rule.

Exact excerpts below; full effects and costs remain in the catalogue and ledger.

| Recipe | Existing wording |
| --- | --- |
| **SH034 — Early Cover** | On recipe Use, schedule a new regular 8-Shield part to load at the beginning of the next round. |

## C11

**Pocket Screen counts saved ordinary parts — resolved · R01**

The owner replaced the nonexistent saved-Utility-item condition with a count of ordinary parts saved in reserve from an earlier round. Final corrected thresholds: 0 saved parts applies Weaken 1, exactly 1 applies Weaken 2, and 2 or more applies Weaken 3 to every living enemy. Count at immediate recipe Use without consuming those parts. Cost and availability are unchanged.

**For our review:** The Utility-item conflict is resolved. The owner confirmed flat per-hit enemy damage reduction, persistent until it decays by 1 per round to zero; the 25% damage-taken idea was withdrawn.

Exact excerpts below; full effects and costs remain in the catalogue and ledger.

| Recipe | Existing wording |
| --- | --- |
| **IV071 — Pocket Screen** | Apply Weaken to every living enemy: 1 with 0 saved parts, 2 with exactly 1 saved part, or 3 with at least 2 saved parts. |

## C12

**Equivalent Utility bonuses count saved ordinary parts — resolved · R01**

The owner directed applying the same solution to the same problem. Twin Coolant and Packed Lunch now check for at least two ordinary parts saved from earlier rounds still in reserve on immediate recipe Use, without consuming them. Twin Coolant retains global cooling, conditional Corrosion 3 and its requirement for a recipe already cooling before Use. Packed Lunch restores 5 Bolt HP, or 10 when the saved-part condition is met, including while disabled. Costs and cooldowns are unchanged.

**For our review:** Both saved-Utility bonus conflicts are resolved. Split activations are resolved separately in C13 and ordinary-part sacrifices in C03. Numerical balance is untested.

Exact excerpts below; full effects and costs remain in the catalogue and ledger.

| Recipe | Existing wording |
| --- | --- |
| **IV099 — Twin Coolant** | On recipe Use, check whether at least two ordinary parts currently in reserve were saved from an earlier round, without consuming them. |
| **AD062 — Packed Lunch** | On recipe Use, restore 5 Bolt HP, including while disabled. |

## C13

**Split Utilities combine into one immediate effect — resolved · R01**

The owner approved combining the original two activations on recipe Use. Service Pair immediately repairs 6 Bolt HP and can only be used while Bolt is active. Split Battery immediately grants 4 Charge. Neither creates a part or separately stored activation. Original totals, costs, cooldowns, normal use limits and existing HP/Charge limits are preserved.

**For our review:** Both split-activation conflicts are resolved. Ordinary-part sacrifices are resolved separately in C03. Numerical balance is untested.

Exact excerpts below; full effects and costs remain in the catalogue and ledger.

| Recipe | Existing wording |
| --- | --- |
| **AD040 — Service Pair** | On recipe Use, immediately repair 6 Bolt HP. |
| **NO055 — Split Battery** | On recipe Use, immediately gain 4 Charge. |

## Q01

**Reactive installed Shield protects subsequent attacks — resolved · R03**

The installed-part rule removes the old missed-activation objection. A stated reactive Shield grant installs ordinary parts at its trigger and can protect against subsequent attacks while installed. It does not retroactively block the triggering damage or refill previously depleted Shield. Values, trigger conditions, reset and explicit upgrade exceptions remain unchanged.

**For our review:** Automatic protection is resolved. Q13 separately retains NO083's implicit Charge-payment timing; inter-effect ordering still follows the shared specification work.

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
| **AD103 — Trip Alarm** | Until the start of your next turn, the first time Bolt becomes disabled, apply Weaken 9 to every enemy and gain 10 Shield. Triggers once; self-paid HP may trigger it. |
| **AD107 — Dark Shift** | For this fight, the first time Bolt becomes disabled after installing this reserve, gain 22 Shield. |
| **AD119 — Stand Back Up** | For this fight, the next two times Bolt is disabled, immediately restore him to 6 HP after the disabling action fully resolves. |

## Q02

**Preparation-time Shield readings and payments — clarification · R03**

These recipes read or spend Shield during preparation. The installed-part clarification establishes protection at enemy attack time, but does not define how a preparation payment removes protection from installed parts or how removal/reinstallation interacts with that payment. It is no longer valid to claim that only an upgrade can make these effects usable.

**For our review:** Clarify installed-value sampling and real payment bookkeeping without inventing Shield activation or granting refunds by reinstalling a paid-down part.

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

**Installed Shield order and secondary payments — clarification · R03**

These effects reference other Shield parts already used or a current Shield balance. The former End Turn activation sequence is superseded by installed protection. Exact readings of prior/other installed parts and secondary payment ordering still need explicit wording; there is no activation prerequisite for protection itself.

**For our review:** Reconcile the remaining order/payment clauses with installed parts as one group, preserving values and existing costs.

Exact excerpts below; full effects and costs remain in the catalogue and ledger.

| Recipe | Existing wording |
| --- | --- |
| **MA028 — Rivet Screen** | Gain 2 more for each other Shield part already activated this round, up to 6 extra Shield. |
| **MA075 — Spare Furnace Door** | You cannot activate this part unless at least one other Shield part has already been activated this round. |
| **IV094 — Reinforced Position** | Gain Shield equal to your current Shield, up to 16. |
| **NO029 — First Field** | Gain 5 more if no other Shield part has been activated this round. |
| **NO038 — Field Peg** | For each other Shield part already activated this round, gain 1 Charge, up to 3 Charge. |

## Q04

**Remaining Shield sampled at enemy-phase end — resolved · R02**

The owner authorized the same pre-reset timing clarification for similar effects. These five rows now evaluate remaining Shield after enemy actions and before reset. Deferred Iron rewards are recorded then and delivered next round; support damage, Mark and Charge already due at enemy-phase end stay there. SH066 still pays available Shield before reset rather than receiving a free count-only reward.

**For our review:** Reset-relative timing is resolved. Preserve actual payments and original reward timing; general ordering among interacting end-phase effects remains part of the shared resolution specification.

Exact excerpts below; full effects and costs remain in the catalogue and ledger.

| Recipe | Existing wording |
| --- | --- |
| **SH066 — Sweep the Plates** | After enemies finish their actions, before the Shield reset, spend up to 9 remaining Shield. |
| **SH091 — Wall Casting** | After enemies finish their actions, before the Shield reset, if any active Shield remains, deal 6 support damage to each surviving enemy that attacked you this round. |
| **MA055 — Reclaimed Plate** | After enemies finish their actions, before the Shield reset, count your total remaining active Shield and record 1 Iron per complete 5 Shield, up to 3 Iron. |
| **IV092 — Untouched Cover** | After enemies finish their actions, before the Shield reset, if that enemy is still alive, apply Mark to it equal to your total remaining active Shield, up to Mark 8. |
| **NO044 — Charge Cage** | After enemies finish their actions, before the Shield reset, if at least 5 total active Shield remains, gain 3 Charge at that time. |

## Q05

**Utility item wording left after conversion — clarification · R01**

The immediate effect is usable without an inventory item, but leftover 'pack crafted' or 'copies of its part' wording describes the former Utility-item model. Unlike the former C03 dependencies, the main effect does not require storing the Utility.

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
| **SH026 — Folding Brace** | On recipe Use, automatically load a regular Shield part worth 4 Shield. At the beginning of the next round, automatically load a new regular Shield part worth 6 Shield. Both parts may be removed and saved normally; parts left loaded activate at End Turn and their active Shield… |
| **SH057 — Spare Metal Brace** | On recipe Use, automatically load a regular Shield part worth 8 Shield. If at least 3 Iron remain in your raw pool after paying the recipe cost, schedule a new regular 5-Shield part to load at the beginning of the next round. Both parts may be removed and saved normally. The… |
| **SH076 — Plate Recasting** | Make one Recast Plate Slug adding that amount as shot damage, up to 14; it has no other effect. |
| **SH092 — Hinged Wall** | When this part activates at End Turn, gain 15 Shield and schedule a new regular 15-Shield part to load at the beginning of the next round. After the first enemy attack that removes any of your Shield in this activation round, apply Mark 4 to that attacker. |
| **SH113 — Last Wall** | When this part activates at End Turn, gain 38 Shield and schedule a new regular 20-Shield part to load at the beginning of the next round. Just before the Shield reset at this enemy-turn end, check whether at least 10 active Shield remains. If so, the next-round delivery also… |
| **SH126 — Salvage Foundry** | On a Perfect manual grab next round, also receive 1 Salvage Slug and 1 Salvage Shield Plate in reserve after collection, with no further material cost. |
| **MA038 — Rivet Bundle** | Each Warm Rivet adds 4 damage to the main shot and gives 1 Heat after impact. |
| **MA048 — Holdfast** | When this part activates at End Turn, gain 10 Shield. You may pay 3 Heat at that activation. Schedule a new regular Shield part to load at the beginning of the next round: 12 Shield if you paid, or 4 Shield if you did not. |
| **MA099 — Settled Slag** | When this part activates at End Turn, gain 22 Shield. |
| **MA107 — Warm Repair Bench** | At the start of next turn, gain 2 Ready Plates. |
| **MA119 — Spare Foundry** | **Utility-output classification requires review.** At the start of next turn, gain 2 Siege Slugs and 2 Siege Plates. |
| **IV019 — Feint Plate** | When this part activates at End Turn, gain 7 Shield and schedule a new regular 3-Shield part to load at the beginning of the next round. If this source part was saved, also apply Weaken 3 to one enemy that currently has Mark; with no eligible enemy, gain only the Shield and… |
| **IV057 — Stored Wall** | When this part activates at End Turn, gain 10 Shield, or 16 if this source part was saved. Schedule a new regular 4-Shield part to load at the beginning of the next round. |
| **IV061 — Hold the Gap** | When this part activates at End Turn, gain 11 Shield. If any enemy currently has at least 6 Mark at that activation, schedule a new regular 5-Shield part to load at the beginning of the next round. |
| **IV088 — Paired Needles** | Each part adds 6 damage and applies Corrosion 2 to the main target after main damage. |
| **IV090 — Stockroom Wall** | When this part activates at End Turn, gain 12 Shield. If at least three saved parts are currently in reserve at that activation, schedule a new regular 14-Shield part to load at the beginning of the next round. |
| **IV118 — Spare Needle Bench** | **Utility-output classification requires review.** For this fight, when a main shot consumes at least two saved Ammo parts, receive 1 Plain Needle at your next turn's start. |
| **AD017 — Spare Bolts** | Each Small Rivet adds 4 damage when used in a shot. |
| **AD074 — Hold the Panel** | When this part activates at End Turn, gain 10 Shield. At the beginning of the next round, automatically load a new regular 8-Shield part if Bolt is active then. |
| **AD083 — Work Order** | At the start of your next turn, receive 2 Small Rivets and 1 Guard Tab. |
| **AD092 — Shared Scaffold** | At the beginning of each of the next two rounds, automatically load a new regular 12-Shield part if Bolt is active then. |
| **AD117 — Night Shift** | At the start of your next turn, restore 10 Bolt HP and receive 2 Heavy Rivets and 2 Guard Tabs. |
| **NO031 — Copper Pins** | Each Copper Pin adds 3 damage and applies Mark 2 to the main target after impact. |
| **NO063 — Twin Field** | Each Small Field Ring gives 6 Shield when activated. |
| **NO087 — Field Lock** | When this part activates at End Turn, gain 18 Shield. If a Fire in this activation round began with at least 8 Charge, schedule one new regular 12-Shield part to load at the beginning of the next round. Several qualifying shots do not add deliveries from this activation; Charge… |
| **NO107 — Shield Press** | At the start of next turn, gain 2 Stored Field Plates. |
| **NO120 — Pocket Generator** | **Utility-output classification requires review.** At the start of next turn, gain 2 Pulse Tips and 2 Charged Plates. |

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

## Q11

**Other Utility conversions and generated outputs — clarification · R01**

These four effects convert a Shield part or grant named parts without selecting an existing part to copy. The owner's copying clarification resolves the copying rows; it does not require calling every other generated output a rule violation. Their classification under the production/effect distinction remains to clarify. Existing effects, costs and cooldowns are preserved; only their review annotations change.

**For our review:** Review their precise conversion/grant behavior only if it exposes a real unresolved distinction. Do not reopen copying, ordinary Shield grants or stored-Utility rules; do not invent extra production costs.

Exact excerpts below; full effects and costs remain in the catalogue and ledger.

| Recipe | Existing wording |
| --- | --- |
| **SH076 — Plate Recasting** | **Utility-output classification requires review.** Sacrifice one unused Shield part with a printed fixed Shield gain. |
| **MA119 — Spare Foundry** | **Utility-output classification requires review.** At the start of next turn, gain 2 Siege Slugs and 2 Siege Plates. |
| **IV118 — Spare Needle Bench** | **Utility-output classification requires review.** For this fight, when a main shot consumes at least two saved Ammo parts, receive 1 Plain Needle at your next turn's start. |
| **NO120 — Pocket Generator** | **Utility-output classification requires review.** At the start of next turn, gain 2 Pulse Tips and 2 Charged Plates. |

## Q12

**Installed Shield parts and shot/install hooks — clarification · R03**

Impact Catch, Backplate and Rivet Collectors still use the former Shield-use timing. Automatic installed protection removes the old impossible-activation argument, but their next-shot or next-installed-part hooks need explicit arming, expiry and removal/reinstallation accounting. Their effects and values have not been silently changed.

**For our review:** Specify these secondary hooks under the installed-part rule, without a separate Shield activation or repeat rewards from reinstalling the same part.

Exact excerpts below; full effects and costs remain in the catalogue and ledger.

| Recipe | Existing wording |
| --- | --- |
| **SH062 — Impact Catch** | After your next main shot this round deals HP damage, gain another 7 Shield once. |
| **MA034 — Backplate** | After this round's main shot, gain 4 more Shield if it consumed at most one Ammo part. |
| **AD076 — Rivet Collectors** | Each of the next four Shield parts you use this planning phase repairs 2 Bolt HP if he is active. |

## Q13

**Implicit Shield secondary-effect costs and stat sampling — clarification · R03**

These Shield rows attach payments, other resource/status gains or conditional snapshots to an implicit activation. The owner's installed-part rule replaces the protection model, but does not by itself choose when each secondary payment or snapshot happens. Preserve printed costs, conditions and values; do not pay or grant them once per enemy attack or repeatedly through removal/reinstallation. Explicitly named End Turn schedules elsewhere retain their clock under the catalogue's legacy-wording rule.

**For our review:** Choose a consistent secondary-cost/snapshot boundary for this group before implementation. These are timing questions, not a revival of Shield activation.

Exact excerpts below; full effects and costs remain in the catalogue and ledger.

| Recipe | Existing wording |
| --- | --- |
| **MA003 — Boiler Jacket** | Gain 6 Shield and 1 Heat when activated. |
| **MA013 — Flameproof Liner** | Gain 4 more if at least one living enemy has Burn when activated. |
| **MA014 — Breathing Plate** | Gain 5 Shield and 2 Heat when activated. |
| **MA017 — Quench Ribs** | Pay 3 Heat when activated. |
| **MA022 — Scrap Weld** | When activated, consume one unused Ammo part from reserve without its effect. |
| **MA024 — Front Brace** | Gain 3 more if the sum of currently shown enemy attack damage is at least 15 when activated. |
| **MA031 — Firebreak** | Gain 4 Shield for each living enemy that has Burn when activated, up to 12 Shield. |
| **MA040 — Long Cooling** | Pay 4 Heat when activated. |
| **MA054 — Emergency Grate** | Lose 3 HP when activated, leaving at least 1 HP, then gain 18 Shield. |
| **MA062 — Cold Riveting** | Gain 10 more if Heat is 0 when activated; otherwise lose 2 Heat after gaining Shield. |
| **MA070 — Stoked Armour** | Gain 8 Shield, plus 1 Shield for each Heat you have when activated. |
| **MA079 — Furnace Rest** | Gain 2 Heat at the start of next turn if no Heat was paid as an activation cost after this part was activated this round. |
| **MA096 — Heated Parapet** | When activated, choose and pay 3 to 8 Heat. |
| **AD079 — Tools in Reserve** | Gain 6 Shield plus 2 per unused Helper part remaining in reserve after this activation, up to 8 extra Shield. |
| **NO003 — Field Plate** | You may pay 1 Charge when activated to gain 4 more Shield. |
| **NO008 — Closed Field** | If Charge is at least 8 when activated, gain 3 more without spending Charge. |
| **NO009 — Starting Field** | Gain 4 Shield and 2 Charge when activated. |
| **NO018 — Field Anchor** | Pay 2 Charge when activated. |
| **NO022 — Glass Guard** | Gain 3 more if you have paid Charge as a part activation cost this round. |
| **NO026 — Emergency Ground** | Pay all your Charge, at least 1, when activated. |
| **NO032 — Sudden Field** | Reduce the total damage of the first enemy attack against you this round by your Charge when this part was activated, up to a reduction of 6. |
| **NO053 — Lightning Post** | You may pay 1 to 3 Charge when activated. |
| **NO058 — Discharge Gate** | Gain 6 Shield plus 2 Shield for every Charge paid as part activation costs this round, up to 12 extra Shield. |
| **NO064 — Empty Socket** | Can activate only at 0 Charge. |
| **NO069 — Full Ground** | Pay all your Charge, at least 6, when activated. |
| **NO076 — Reserve Screen** | Gain 8 more if at least two recipes in memory currently have numeric cooldown counters when activated. |
| **NO083 — Restarting Field** | Pay 4 Charge when activated. |
| **NO105 — Ground Shield** | Pay 6 Charge when activated. |

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

Modifier/Helper lifecycle, Shield-part payment ordering, final secondary-effect timing and precise stat sampling are still partly draft under the installed-part rule. Q03/Q10/Q12/Q13 identify concrete rows that expose open gaps. Q04's position before the reset is now clarified; ordering among interacting effects remains separate. Further clarification must preserve End Turn and the active-Shield reset. Main-shot singular wording is generally readable through the existing next-shot/default-duration rules; it is not automatically a one-shot-per-turn restriction. This review does not label every ordinary row as conflicting merely because the eventual engine still needs an effect-resolution order.

**Next action:** review C08's removed Charged Barrel payment references. Apply settled fixes to equivalent cases and distinguish remaining Shield secondary-effect timing from its settled automatic protection. Shield-part replacements, pre-reset timing clarifications, saved-Utility bonuses, combined immediate Utility effects and ordinary-part sacrifices are applied; unrelated replacements and gameplay implementation are not implied.
