# Recipe rule audit — review together

**19 September 2026 · all 606 recipes reviewed · Shield calculations, installation history and hooks clarified**

All recipe rows match reviewed revision `3172708`; settled rules and explicit character/recipe exceptions remain preserved. Found **0 recipes with a definite conflict or obsolete dependency**, **69 additional recipes needing wording/dependency clarification**, and **537 with no direct conflict identified**. A recipe can have findings in several groups; group counts therefore overlap. Clarifications on a conflicting recipe are also retained.

**Owner resolutions:** C04/C05 use ordinary Shield parts and explicit future deliveries. Following Folding Brace, the owner approved Spare Metal Brace's 8-now/conditional-5-next-round schedule and authorized analogous replacements. All twelve remaining C01 retention recipes now deliver fresh parts under their stated conditions; SH034's old expiry is resolved in C10. Parts left installed protect when enemies attack, and active Shield resets at enemy-turn end. Parts remain removable/saveable; a delivery does not copy the source's delivery or secondary effects.

**Latest timing clarification:** Field Pocket counts remaining Shield before reset and delivers recorded Charge next round. The same pre-reset reading/payment now resolves five Q04 rows. Existing reward timing and explicit Shield costs are preserved.

This is an audit for joint review. The owner corrected the assumption behind all thirteen barrel findings: special character effects can override base rules. Hot Barrel and Charged Barrel are restored as earlier trait drafts, and the eight recipe cleanups are reversed. The other five rows retain their original payment conditions; no replacement Charge-spending tally is introduced. The four-character review found no comparable removal of Ivo's Find the Seam or Ada's Bolt feature. Quench Recovery and Residual Current remain unselected alternatives. Those restored interactions remain unchanged by the later Utility wording pass; earlier totals and removal claims are historical. Character trait values remain draft balance; proposed alternatives do not automatically replace retained traits. No direct conflict identified means the row passed this written-rule review, not that it is implementation-ready, balanced or proven in every combination.

**Resolved starter:** MA004 Quick Vent automatically loads its 5-Shield part; the player may use it at this End Turn or remove and save it. SH005 Split Outlet still needs Q10 spread-Modifier lifecycle clarification. **Latest owner answers:** stat-based Shield values are calculated once at first installation, retaining normal depletion/reset and explicit exceptions. Order-based recipes count earlier first installations this round even after removal, once per physical part. Q03/Q12/Q13 now close 21 more cases; costs, cooldowns and effect amounts are preserved. Prior resource-bonus/payment and Utility clarifications remain closed. **Pending joint question:** should spread Modifiers such as Split Outlet remain physical bullet parts until Fire, allowing normal unload/reorder/save and unfired End Turn return? That is the recommendation only; consuming them on earlier Use with a pending effect is the alternative. Crafting still pays the recipe cost normally.

## Coverage

| Pool | Reviewed | Conflict | Clarification only | No direct conflict identified |
| --- | ---: | ---: | ---: | ---: |
| SH | 126 | 0 | 18 | 108 |
| MA | 120 | 0 | 15 | 105 |
| IV | 120 | 0 | 15 | 105 |
| AD | 120 | 0 | 7 | 113 |
| NO | 120 | 0 | 14 | 106 |

All rows were read for: output kind and Utility lifecycle; installed Shield protection, reset and grants; per-copy cooldown, global cooling and no-cooldown uses; part-based damage, multiple shots and modifier duration; status recipients, hit order, explicit copy/stack limits; whole-number arithmetic and player HP costs/death; resource/part persistence, collection count and Precision protection; part consumption, saved-part age and canonical sale basis; specific character exceptions, base defaults and unselected trait alternatives.

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
| [C08](#c08) | resolved | 5 | Charged Barrel is a specific character exception |
| [C09](#c09) | resolved | 1 | Folding Brace owner-selected replacement |
| [C10](#c10) | resolved | 1 | Early Cover uses ordinary Shield-part expiry |
| [C11](#c11) | resolved | 1 | Pocket Screen counts saved ordinary parts |
| [C12](#c12) | resolved | 2 | Equivalent Utility bonuses count saved ordinary parts |
| [C13](#c13) | resolved | 2 | Split Utilities combine into one immediate effect |
| [C14](#c14) | resolved | 8 | Original barrel timing and exclusions restored |
| [Q01](#q01) | resolved | 11 | Reactive installed Shield protects subsequent attacks |
| [Q02](#q02) | clarification | 12 | Preparation-time Shield readings and payments |
| [Q03](#q03) | resolved | 4 | Earlier first installations count for order-based recipes |
| [Q04](#q04) | resolved | 5 | Remaining Shield sampled at enemy-phase end |
| [Q05](#q05) | resolved | 4 | Immediate Utility payments and shared healing cap |
| [Q06](#q06) | clarification | 6 | Recipe crafting versus immediate Utility use |
| [Q07](#q07) | resolved | 2 | Explicit part-only cooling rewards retained |
| [Q08](#q08) | clarification | 14 | Status recipient implied rather than stated |
| [Q09](#q09) | clarification | 27 | Canonical sale basis for batches and generated subparts |
| [Q10](#q10) | clarification | 10 | Spread Modifier lifecycle and placement order |
| [Q11](#q11) | clarification | 4 | Other Utility conversions and generated outputs |
| [Q12](#q12) | resolved | 3 | First-installation and next-shot hooks |
| [Q13](#q13) | resolved | 14 | One-time Shield values and related installation checks |
| [Q14](#q14) | resolved | 3 | First-installation Shield resource bonuses |
| [Q15](#q15) | resolved | 12 | Once-only additional Shield installation payments |

## Rule references

These summaries refer to the selected-rule sections in [RECIPE-CATALOGUE.md](RECIPE-CATALOGUE.md) and the owner chronology in [DECISIONS.md](../DECISIONS.md). Draft conventions are used to explain dependencies, not promoted into owner decisions.

- **R01:** Utilities activate on recipe Use without a stored Utility item. Production means crafting from resources; copying an existing part is a Utility effect, not resource-based production of that copy. Copying does not require paying the copied part's normal production cost; printed recipe-use costs are unchanged. Ordinary Shield-value grants follow the selected automatic loading, removal/storage and End Turn rules. Other named-part conversion/grant classifications remain clarification work, not proven conflicts merely because a part appears.
- **R02:** Active Shield resets at enemy-turn end by default; only explicit permanent upgrades provide retention exceptions. End-phase remaining-Shield readings/payments happen after enemy actions and before reset. Record a deferred reward then and deliver it at its stated time; explicit payments still spend available Shield.
- **R03:** Installed Shield parts protect automatically when enemies attack; neither Fire nor End Turn activates them. End Turn only ends the player turn. Counts do not consume parts. Installed values add and damage depletes protection; normal reset and explicit upgrade exceptions remain. Grants install ordinary removable parts at their stated time, without retroactive blocking or refilling prior loss. Unless explicitly timed otherwise, immediate bonuses, additional Heat/Charge/HP/part payments and stat-based calculations use first installation once, with no refund, repeat or recalculation on reinstallation. Order-based used-this-round conditions count earlier first installations even after removal; explicit current-installed checks retain their scope. Next-shot and delayed hooks retain named clocks/expiry. Allocation of Shield payments remains open.
- **R04:** Ordinary gun base damage is zero; explicit character effects, upgrades and recipes may override a base rule within their stated scope. A later general rule does not automatically revoke a specific effect. Hot Barrel and Charged Barrel remain character bonuses to valid part-built shots, with their earlier draft values. If the owner's intent to override a specific effect is unclear, ask before deleting or replacing it. Alternatives are not automatically selected or stacked.
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
| **MA040 — Long Cooling** | Provides 7 Shield and schedules a new regular 7-Shield part to load at the start of next turn, after old Shield clears. |
| **MA092 — Pressure Debt** | At the start of next turn, gain 8 Shield after old Shield clears. |
| **MA118 — Hammer and Anvil** | At the start of next turn, gain 24 Shield after old Shield clears. |
| **IV059 — Late Cover** | If at least one saved part remains in reserve at the end of this enemy phase, gain 12 Shield at the start of next turn, after old Shield clears. |
| **IV097 — Quiet Workshop** | For this fight, at your turn's start, gain 2 Shield for each saved part in reserve, up to 6 Shield. |
| **IV112 — Tomorrow's Opening** | At your next turn's start, remove all cooldown counters from every recipe currently cooling in memory and gain 12 Shield. |
| **IV120 — Patient Hunter** | For this fight, at your turn's start, if at least four saved parts remain in reserve, gain 5 Shield and apply Corrosion 2 and Mark 4 to one enemy chosen at that turn's start. |
| **AD063 — Shift Change** | Gain 6 Shield now and 8 Shield at the start of your next turn, after normal Shield clearing. |
| **AD066 — Scheduled Cover** | At the start of your next turn, gain 12 Shield after normal Shield clearing. |
| **AD098 — Two Shifts** | At the start of each of your next two turns, gain 8 Shield after clearing and repair 5 Bolt HP if he is active. |
| **NO018 — Field Anchor** | At enemy-phase end in that installation round, if this part is still installed and you have at least 2 Charge, schedule a new regular 6-Shield part to load at the start of next turn, after old Shield clears. |
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

**Charged Barrel is a specific character exception — resolved · R04**

The owner corrected the audit's premise: a new general rule does not automatically revoke a special character effect. Charged Barrel is restored, with its original optional payment of up to 3 Charge at Fire for +2 main-shot damage per Charge, before other Charge checks. The five payment/no-payment/refund triggers therefore retain their original meaning. Their recipe rows were never changed; no generic recipe-spending tally is substituted.

**For our review:** This dependency finding is resolved by restoring the wrongly removed character effect. Its numeric values remain draft balance. Replacements based on unrelated Charge payments are withdrawn.

Exact excerpts below; full effects and costs remain in the catalogue and ledger.

| Recipe | Existing wording |
| --- | --- |
| **NO028 — Contact Mark** | After impact, apply Mark 3 to the main target, or Mark 7 if the barrel paid at least 2 Charge at Fire. |
| **NO039 — Loose Contact** | If you pay no Charge through the barrel at Fire, add 4 more damage. |
| **NO075 — Shot Ground** | Gain 2 Charge after impact if the barrel paid at least 2 Charge at Fire. |
| **NO081 — Patient Coil** | For this fight, gain 3 Charge after any main-shot impact for which you paid no Charge through the barrel at Fire. |
| **NO106 — Shot Receipt** | At the start of next turn, recover the Charge paid by Charged Barrel for this shot, up to 3 Charge, and gain 1 Copper. |

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

## C14

**Original barrel timing and exclusions restored — resolved · R04**

The prior eight-row cleanup relied on the same mistaken assumption that a general zero-base-damage rule removed Charged Barrel. Those edits are reversed. Five rows again read Charge after its optional payment, and Hot Contact, Discharge Record and Discharge Gate retain their explicit exclusion of that payment from part-cost spending. Original amounts, recipients, durations, costs and cooldowns are preserved.

**For our review:** The eight rows were restored on 18 September. NO058 now additionally has the owner-selected first-installation sampling text from Q13, preserving its amount and barrel-payment exclusion. All thirteen character interactions remain valid; this is not a balance result.

Exact excerpts below; full effects and costs remain in the catalogue and ledger.

| Recipe | Existing wording |
| --- | --- |
| **NO005 — Live Wire Tip** | Add 5 more if Charge is at least 6 at Fire, after the barrel's optional Charge payment. |
| **NO006 — Grounded Slug** | Add 6 more if Charge is 0 at Fire, after the barrel's optional Charge payment. |
| **NO017 — Hot Contact** | If at least 3 Charge was paid as part activation costs this round, apply Burn 3 to the main target after impact. |
| **NO025 — Full Cell Tip** | Add 12 more if Charge is 12 at Fire, after the barrel's optional Charge payment. |
| **NO035 — Even Current** | Add 6 more if you have an even positive amount of Charge at Fire, after the barrel's optional Charge payment. |
| **NO048 — Discharge Record** | Also add 5% to its damage per Charge paid as part activation costs this round, up to 30%. |
| **NO058 — Discharge Gate** | On first installation, count Charge already paid as part-use costs this round. |
| **NO110 — Reserve Coil** | For this fight, at Fire add 3 damage per Charge still held after the barrel's optional payment, up to 24 extra damage. |

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
| **NO083 — Restarting Field** | The first time an enemy attack reduces positive Shield to 0 in that installation round while this part is installed, gain a fresh regular 16-Shield part after that attack finishes. |
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

**Earlier first installations count for order-based recipes — resolved · R03**

The owner selects earlier first installations this round, including parts later removed. Count each physical part once and exclude the source; reinstallation does not add history or repeat an eligibility check, gain or calculation. Failed installation adds no history. Preserve other recipes' explicit current-installed conditions. Reinforced Position, the fifth original Q03 row, is resolved with the one-time values in Q13.

**For our review:** Four order-based cases are resolved without changing costs, cooldowns or amounts. A first installation from an earlier round does not enter the current round's history.

Exact excerpts below; full effects and costs remain in the catalogue and ledger.

| Recipe | Existing wording |
| --- | --- |
| **MA028 — Rivet Screen** | On first installation, count other Shield parts first installed earlier this round, including any since removed. |
| **MA075 — Spare Furnace Door** | First installation requires at least one other Shield part to have been first installed earlier this round, even if it was later removed. |
| **NO029 — First Field** | On first installation, set this part's Shield to 5, plus 5 if no other Shield part was first installed earlier this round. |
| **NO038 — Field Peg** | On first installation, count other Shield parts first installed earlier this round, including any since removed, and gain 1 Charge per counted part once, up to 3 Charge. |

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

**Immediate Utility payments and shared healing cap — resolved · R01**

Existing immediate-Utility rules resolve the obsolete pack/crafting wording. Copper Recovery, Glass Recovery and Iron Recovery pay their listed cost once on recipe Use and grant the conversion then. Quick Patch restores HP on Use and retains its existing 8-HP-per-fight cap shared across all Uses and duplicate copies of that recipe. No Utility inventory item or second payment is introduced.

**For our review:** Four wording findings are resolved without changing costs, cooldowns or amounts. The healing cap remains recipe-wide; duplicate recipe availability still tracks each copy independently.

Exact excerpts below; full effects and costs remain in the catalogue and ledger.

| Recipe | Existing wording |
| --- | --- |
| **SH029 — Copper Recovery** | On recipe Use, gain 1 Copper after paying this recipe's listed Iron cost once. |
| **SH030 — Glass Recovery** | On recipe Use, gain 1 Glass after paying this recipe's listed Copper cost once. |
| **SH031 — Iron Recovery** | On recipe Use, gain 1 Iron after paying this recipe's listed Carbon cost once. |
| **SH074 — Quick Patch** | On recipe Use, restore 4 HP, up to your maximum. |

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

**Explicit part-only cooling rewards retained — resolved · R01**

Both recipes explicitly restrict their reward to cooling caused by a part. Preserve that narrower condition under the existing recipe-specific scope rule. Recall Pin (NO057) is an eligible Ammo source; direct Utility cooling and normal round progression do not qualify. Remove Charge Receipt's obsolete superseded marker without changing its functional effect. The underlying cooling still affects every eligible recipe under the global-cooling rule.

**For our review:** These two eligibility findings are closed by retaining the explicit wording, not by inventing a broader trigger. Recipe amounts, timing and caps remain unchanged.

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

**First-installation and next-shot hooks — resolved · R03**

Apply the settled installation and explicit-clock rules. Impact Catch and Backplate arm once on first installation for the next main shot in that round, checking that the source is installed at the stated trigger. That shot consumes the check even if its condition fails; reinstallation does not re-arm it, and unused checks expire with the round. Rivet Collectors counts the next four first installations after its Modifier Use, repairs after each part's own effects if Bolt is active, and still expires at Fire. Amounts and costs are unchanged.

**For our review:** Three hook cases are resolved as consequences of existing rules. This introduces neither another Shield activation nor repeated rewards from reinstallation.

Exact excerpts below; full effects and costs remain in the catalogue and ledger.

| Recipe | Existing wording |
| --- | --- |
| **SH062 — Impact Catch** | On first installation, arm a check for your next main shot this round. |
| **MA034 — Backplate** | On first installation, arm a check for your next main shot this round. |
| **AD076 — Rivet Collectors** | After using this Modifier, each of the next four Shield-part first installations this planning phase repairs 2 Bolt HP if he is active after that part's own effects. |

## Q13

**One-time Shield values and related installation checks — resolved · R03**

The owner selects calculating Shield values once. Stat-based values are sampled on first installation and retained across later stat changes or reinstallation, with ordinary depletion and reset preserved. Apply equivalent timing to the remaining eligibility checks, one-time effects and relative schedules while retaining explicitly later checks. Reinforced Position samples existing available Shield before adding its own contribution and grants Mark once. NO058 retains its explicit barrel-payment exclusion. Furnace Rest preserves its next-turn check over Heat payments after installation in that installation round.

**For our review:** Fourteen cases are resolved, including Reinforced Position moved from Q03. Actual-payment values from Q15 retain their own basis. Copy/grant exceptions, explicit clocks and draft meter balance remain unchanged.

Exact excerpts below; full effects and costs remain in the catalogue and ledger.

| Recipe | Existing wording |
| --- | --- |
| **MA013 — Flameproof Liner** | On first installation, set this part's Shield to 7, plus 4 if at least one living enemy currently has Burn. |
| **MA024 — Front Brace** | On first installation, set this part's Shield to 9, plus 3 if the sum of currently shown enemy attack damage is at least 15. |
| **MA031 — Firebreak** | On first installation, set this part's Shield to 4 per living enemy currently having Burn, up to 12 Shield. |
| **MA062 — Cold Riveting** | On first installation, set this part's Shield to 6, plus 10 if Heat is currently 0. |
| **MA070 — Stoked Armour** | On first installation, set this part's Shield to 8 plus 1 per Heat currently held, without spending Heat. |
| **MA079 — Furnace Rest** | On first installation, schedule a check at the start of next turn: gain 2 Heat if no Heat was paid as a part-use cost after this installation during its round. |
| **AD079 — Tools in Reserve** | On first installation, count unused Helper parts remaining in reserve after this installation. |
| **NO008 — Closed Field** | On first installation, set this part's Shield to 10, plus 3 if Charge is currently at least 8, without spending Charge. |
| **NO022 — Glass Guard** | On first installation, set this part's Shield to 8, plus 3 if you have already paid Charge as a part-use cost this round. |
| **NO032 — Sudden Field** | On first installation, record your current Charge, capped at 6, without spending it. |
| **NO058 — Discharge Gate** | On first installation, count Charge already paid as part-use costs this round. |
| **NO064 — Empty Socket** | First installation requires exactly 0 Charge. |
| **NO076 — Reserve Screen** | On first installation, set this part's Shield to 8, plus 8 if at least two recipes in memory currently have numeric cooldown counters. |
| **IV094 — Reinforced Position** | On first installation, read your current available Shield before adding this part. |

## Q14

**First-installation Shield resource bonuses — resolved · R03**

The owner accepts Boiler Jacket's extra Heat on first installation. Apply the same rule to Breathing Plate and Starting Field. Each physical part grants its printed Heat/Charge once; crafting to reserve alone does not trigger it. Removal does not undo the resolved gain, and reinstallation, even after saving, does not repeat it. These three rows were previously in Q13. Costs, cooldowns, amounts and explicit alternative clocks are preserved.

**For our review:** The bonus timing is selected. Normal Shield protection, depletion and reset remain unchanged; Heat/Charge meter limits and decay values remain draft tuning.

Exact excerpts below; full effects and costs remain in the catalogue and ledger.

| Recipe | Existing wording |
| --- | --- |
| **MA003 — Boiler Jacket** | When this part is first installed, gain 1 Heat once. |
| **MA014 — Breathing Plate** | When this part is first installed, gain 2 Heat once. |
| **NO009 — Starting Field** | When this part is first installed, gain 2 Charge once. |

## Q15

**Once-only additional Shield installation payments — resolved · R03**

The owner selects Quench Ribs' extra payment once on first installation, sufficient funds required, with no refund on removal or second payment on reinstallation. Apply to analogous Heat/Charge, HP and Ammo-sacrifice costs. Optional choices are committed once, and payment-derived values use the actual payment. Preserve cost-plus-1 HP eligibility, explicit clocks, source-installed reactive checks, committed deliveries and fight-end expiry. Reinstallation repeats no payment, gain or schedule. These twelve rows were previously in Q13.

**For our review:** Payment timing and bookkeeping are resolved without changing recipe costs, cooldowns or effect amounts. Q13 now records the selected one-time stat sampling; Shield-spending allocation in Q02 remains separate.

Exact excerpts below; full effects and costs remain in the catalogue and ledger.

| Recipe | Existing wording |
| --- | --- |
| **MA017 — Quench Ribs** | On first installation, pay 3 Heat once. |
| **MA022 — Scrap Weld** | On first installation, consume one unused Ammo part from reserve without its effect, once. |
| **MA040 — Long Cooling** | On first installation, pay 4 Heat once; cannot install without enough Heat. |
| **MA054 — Emergency Grate** | On first installation, pay 3 HP once; requires at least 4 HP and leaves at least 1 HP. |
| **MA096 — Heated Parapet** | On first installation, choose and pay 3 to 8 available Heat once. |
| **NO003 — Field Plate** | On first installation, you may pay 1 available Charge once to add 4 Shield to this part. |
| **NO018 — Field Anchor** | On first installation, pay 2 Charge once; cannot install without enough Charge. |
| **NO026 — Emergency Ground** | On first installation, pay all your Charge once, requiring at least 1 Charge. |
| **NO053 — Lightning Post** | On first installation, you may pay 1 to 3 available Charge once; record the actual payment. |
| **NO069 — Full Ground** | On first installation, pay all your Charge once, requiring at least 6 Charge. |
| **NO083 — Restarting Field** | On first installation, pay 4 Charge once; cannot install without enough Charge. |
| **NO105 — Ground Shield** | On first installation, pay 6 Charge once; cannot install without enough Charge. |

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

Spread Modifier lifecycle, allocation of payments from installed Shield, counted recipe-use events, exact targets and canonical output definitions remain catalogue work. Resource bonuses, additional Heat/Charge/HP/part payments, stat calculations and related Shield hooks now follow the selected first-installation rules. Q02/Q06/Q08/Q09/Q10/Q11 retain the open findings. Q04's position before the reset is clarified; general ordering among interacting effects remains separate. Further clarification must preserve End Turn and the active-Shield reset. Main-shot singular wording is generally readable through the existing next-shot/default-duration rules; it is not automatically a one-shot-per-turn restriction. This review does not label every ordinary row as conflicting merely because the eventual engine still needs an effect-resolution order.

**Next action:** review remaining clarification cases as grouped wording/timing work, respecting explicit character and recipe exceptions. Do not reopen the restored barrel-payment interactions or treat unselected alternative traits as replacements. Shield-part replacements, pre-reset timing clarifications, saved-Utility bonuses, combined immediate Utility effects and ordinary-part sacrifices are applied; unrelated replacements and gameplay implementation are not implied.
