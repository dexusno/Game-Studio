# Overkill Foundry — recipes for four mercenaries

**Independent game since 14 September 2026.** Owner-selected title **Overkill Foundry**; catalogue ID `overkill-foundry`. Design and art references were preserved from Magnet Sweep at `adf3a3c55e2c680f5f6a2b442d0ed93cd84c4883`; future gameplay is a fresh implementation. [Current handoff](../STATUS.md) governs. Earlier magnet imagery and names record the design history.

Design draft, 14 September 2026. **606 original recipes: 126 shared and 120 exclusive to each of four proposed mercenaries.** Six shared manual-grab enhancements (three Rare, three Legendary) extend the original 600 at the owner's request. Names, starting kits, material costs, damage values, helper rules and new effect details are proposals for review and later playtesting. The confirmed recipe-memory, cooldown and fight-reset rules are preserved. This document contains no graphics or game functionality.

**Updated owner rules, 14 September:** round Shield/builds clear after the complete enemy turn by default. Permanent upgrades provide explicitly stated exceptions and may grant beginning Shield each turn, at fight start or on a specified turn. Recipe-retention clauses conflicting with that rule are superseded, not pending exception candidates. Automatic cooldown reduction skips the recipe-use round; cooldown 1 blocks the following round and cooldown 2 blocks the following two rounds unless explicit cooling clears counters sooner. Utility recipes activate directly on Use and produce no stored Utility part; the 16 September Shield-grant clarification below defines the automatically loaded Shield-part exception. Cooling effects apply globally without selected recipe targets. Permanent claw upgrades from shops/Officer loot activate on purchase/acceptance; the owner also keeps all 23 temporary gathering recipes. Recipe loot remains zero or one choice from three, with free skipping and memory replacement when full. See [the authoritative clarification](REDESIGN-PLAN.md#round-resets-cooling-and-accepted-loot--14-september-2026). Printed costs, strengths and cooldown counts remain draft tuning; global cooling broadens effects and collapses some earlier target-count distinctions.

## Find a recipe pool

- [How recipes work](#how-recipes-work)
- [Manual grab enhancements](#manual-grab-enhancements)
- [Four mercenaries and their starting kits](#four-mercenaries-and-their-starting-kits)
- [Shared recipes — every mercenary](#shared-recipes--every-mercenary)
- [Mara Kiln Voss](#mara-kiln-voss)
- [Ivo Rook Vale](#ivo-rook-vale)
- [Ada Patch Flint](#ada-patch-flint)
- [Noor Flux Sayeed](#noor-flux-sayeed)
- [Reference and design notes](#reference-and-design-notes)

## How recipes work

**Recipes have different output types.** Gun and Shield recipes pay their listed materials to create parts in reserve; those parts may be used now or saved, and are consumed when used. **Utility recipes pay their listed materials and activate immediately when used; they produce no stored Utility part.** Immediate Shield grants follow the explicit automatically loaded Shield-part exception below. There is no stored Utility output or second part-Use action. Recipe Use applies its use/cooldown accounting whether it creates a part or activates a Utility effect. A part-producing recipe that makes two parts is still one recipe and one use.

These pools are possible discoveries, not a deck or the contents of starting memory. Keep only the recipes that fit memory capacity. **Owner decision, 15 September: exact duplicate recipes may coexist in memory, with each copy taking one slot.** Two Flat Plate entries occupy two slots. **Each stored copy has its own cooldown and once-per-turn use tracking.** Two Flat Plate copies may each be used once in the same turn, paying for both; using one copy does not lock the other. Copy availability does not override explicit per-effect non-stacking or trigger restrictions. Starter sets and catalogue recipe identities are unchanged. When memory is full, a new recipe can replace an old one; an offer can also be skipped. **Owner-selected starting capacity: 20 recipes, subject to balancing after beta testing.** Permanent upgrades can expand it. **Owner accepts the listed 12-recipe starter sets for now: 8 shared and 4 character-specific**, leaving 8 free slots. Both capacity and the starter set will be balanced through beta runs; individual recipe tuning remains draft. Capacity and the number of starting recipes remain distinct. **Profile Collection keeps discovered recipe history after death and across campaigns**, separately from active recipe memory. Seeing a recipe offered adds it to Collection even if skipped: all three recipes in a viewed reward offer count, independently of accepting zero or one into memory. Collection records consume no recipe-memory slots and do not expand the starter set. **Recipes have no permanent unlock requirements.** All non-starter recipes can appear during the first campaign under normal character, encounter, rarity and city/level offer rules; previous campaigns, achievements and Collection discoveries are not prerequisites. **Owner-selected character order: Mara → Ivo → Ada → Noor.** Mara is initially available; clearing a city with Mara unlocks Ivo, with Ivo unlocks Ada, and with Ada unlocks Noor for the active profile, persisting across campaigns. A city victory with the preceding mercenary is sufficient, without completing the whole campaign. The unlock order is selected; detailed character traits, identities and balance remain draft. Exclusive recipe eligibility follows the chosen unlocked mercenary, with no additional recipe-specific unlock requirements.

Each mercenary uses the 126 shared recipes plus their own 120 exclusive recipes: **246 possible recipes per character**. Shared recipes are written once in this document and belong to all four pools. Base recipes are starting knowledge. Common, uncommon, rare and legendary entries are discoveries. The owner's encounter and rarity rules still apply: regular fights do not give legendary recipes; stronger encounters and shops have their own reward pools. Pool sizes do not set drop chances or shop prices.

### Materials

| Material | Typical use |
| --- | --- |
| Iron | Shot mass, armour plates, helper frames |
| Copper | Conductors, coils, cooling and magnet parts |
| Carbon | Powder, furnace fuel and chemical mixtures |
| Glass | Lenses, insulation and sealed chemical containers |
| Circuit | Control chips, automation and advanced part behaviour |

All five are proposed scrap ingredients, potentially available from the first city. Circuit should be less common than the bulk materials. The exact haul size and mix still need balancing. The [15 September printed-cost audit](BALANCE-RESEARCH.md#current-beta-haul-recommendation--15-september-2026) recommends testing 10 materials per normal haul: an 8-material foundation plus 2 steered units. This is a beta proposal, not a selected haul rule; all recipe rows remain unchanged by that analysis. Energy cores and credits are not crafting ingredients. The costs in this document are material costs, not shop prices; **owner decision, 15 September: crafting and firing use no separate global energy/action-point budget**. Materials, available parts and recipe use/cooldown requirements provide the action limits. This selects the no-extra-energy rule, not the draft numerical costs, supply amounts or character traits. Explicit recipe-specific resource requirements remain subject to their individual design review. **Owner decision, 15 September:** unused materials and crafted parts have no reserve storage cap within a fight. They carry between rounds and clear at fight end. Individual bullets and shields also have no fixed part-count cap: only resources and available crafted parts under the existing recipe rules restrict builds. The 20-recipe starting memory capacity remains separate.

**Part resale, owner correction 15 September — main-recipe basis:** each part uses the normal ingredient cost of producing one through its main recipe, valued at the shop's current resource prices. Its sale price is `floor(0.5 × sum(main-recipe ingredient quantity for one part × current shop resource unit price))` credits. Rare-recipe rebates and discounts do not reduce that basis; alternative production recipes, copied parts, batch size and mixed outputs do not change the same part's base sale value. Different parts each use their own main recipe's one-part cost. **This supersedes the earlier actual-batch equal-cost allocation convention**, including the pending mixed-output question. The 50% factor, current-price valuation and final whole-credit round-down remain selected. For example, a 10-credit normal one-part ingredient value gives a 5-credit sale price even when an alternative recipe spends only 6 credits producing it. A copied part keeps the same part-type valuation reference as its original, without another 50% discount. Main-recipe mappings and standard one-part requirements still need defining wherever draft subparts/generated outputs lack them; do not substitute the creating recipe's discounted cost or invent a price. Printed resource counts are not credit prices. Raw materials cannot be sold to the shop; raw-material purchases and energy-core sales remain available. Utilities create no saleable Utility part. Recipe rows, actual crafting costs and effect text remain unchanged by this correction.

### Cooldown

**Per-copy availability, owner decision 15 September:** apply the following use rules independently to each stored recipe copy. Using one copy only starts that copy's cooldown or spends that copy's no-cooldown use. For example, a cooldown-1 copy used in round 4 is naturally ready in round 6; a second copy left unused stays available and, if used in round 5, is naturally ready in round 7. Explicit cooling can make either ready earlier. Global cooling applies to every cooling copy, including duplicates and the source if cooling. Each copy's automatic tick skips its own use round, and every copy starts each fight at zero counters. This changes availability per copy, not recipe-specific rules on stacking effects or counting distinct recipe identities.

| Table entry | Recipe-use rule |
| --- | --- |
| None | No cooldown ability. Use once per turn, whether crafting parts or activating a Utility recipe, unless a permanent upgrade explicitly allows more. Temporary cooling cannot reset this use. |
| 1–4 | Apply that many counters at recipe Use. Skip automatic reduction at the end of that use round; subsequent enemy-turn ends remove 1, stopping at zero. Cooldown 1 used round 4 blocks round 5 and is ready round 6. Cooldown 2 used round 4 blocks rounds 5 and 6 and is ready round 7. Explicit cooling may make it ready earlier. |

If an effect removes every counter from a cooldown recipe, it becomes usable immediately, including again in the same turn, if its cost can be paid. Using it again applies its cooldown again and starts a new use-round exemption from automatic ticking. Removing only some counters leaves it blocked. A cooldown recipe at zero counters is different from a recipe with **None** in this column.

**Global cooling:** apply a recipe's use/cooldown accounting before its direct Utility effect, preserving the existing rule that its source is affected if cooling. Explicit cooling can remove newly applied counters; it does not wait for the first automatic tick. An effect removing X counters removes up to X from every currently cooling recipe in memory, clamped at zero. It does not divide a budget among them, require recipe targets, restrict to Ammo/Helper recipes, or exclude its source recipe. A full-reset draft clears all active numeric cooldowns. No-cooldown once-per-turn use is unaffected. Check conditional bonuses after the whole cooling effect resolves; making several recipes ready does not multiply a once-per-effect reward. Non-cooling effects keep their stated targets and timing.

Raw resources and unused parts stay between turns and both clear at fight end. **Remaining player HP carries into the next fight, without automatic healing at fight transitions.** Explicit healing effects can restore HP; their values remain draft. Retained recipes and permanent systems last through the current game. **Owner-confirmed, 15 September:** every new fight starts with all recipe cooldown counters cleared. **Remaining draft convention:** Heat, Charge, helper state and other temporary effects start fresh in a new fight; these resets are not selected merely by confirming cooldown reset. Delayed supplies or effects do not carry through a fight ending before they trigger.

**Shop healing boundary — owner decision, 15 September:** shops offer no direct healing service. Healing recipes may be sold as knowledge to use under normal resource/cooldown rules. Shops may also offer permanent upgrades that raise maximum HP and heal for that same amount: +X maximum HP restores X current HP, not all missing HP. These upgrade entries, their values/prices and stock availability remain later content work.

### Utility catalogue reconciliation — 14 September

All 181 Utility rows now identify an immediate effect with no part. Former object names are retained only as effect labels, not inventory objects. In a Utility row, activation means using the recipe; extra effect costs and targets must be handled in that action, not postponed by storing an item. Exact validation/target-selection ordering remains to specify. Delayed/ongoing effect wording registers the effect now rather than manufacturing an activation part.

**Superseded dependencies, revise before use:** IV071/IV099 require saving the Utility itself; AD040/NO055 split one Utility recipe into separately stored activations; IV105/AD097 consume stored Utility parts; NO094 requires a part-based cooling trigger and needs reconciliation with direct Utility cooling. SH071/SH076/SH102/SH118, MA107/MA119, IV118 and NO107/NO120 explicitly create or copy parts through Utility effects: they require classification/effect review under the owner's no-part rule, not automatic approval as exceptions. Their original effect amounts remain drafts; no replacement reward or new category is invented here. References elsewhere to counting crafted recipes, part activations or part-only payments also require an explicit reading under the new distinction.

### Using parts

**Latest owner flow, 14 September:** the full-screen Recipe view's **Use** button pays resources and applies recipe availability rules. Gun/Shield recipes add crafted parts to reserve; Utility recipes activate their effect immediately and add no part. Selecting a build part stages it; Load prepares the build and may be undone before Fire. Select legal targets, then Fire consumes/resolves that shot and returns to preparation while combat continues. Multiple shots are allowed while parts/resources and recipe availability permit. Only **End Turn** ends the player turn and starts surviving enemies' actions, including after zero shots. See the [current turn sequence](REDESIGN-PLAN.md#owner-defined-encounter-and-turn-sequence--14-september-2026).

**Multiple-shot reconciliation:** a shot is not a new round. Firing does not repeat collection, restore recipe uses, tick cooldowns or clear Shield. The earlier single-main-shot convention is superseded. The earlier turn-flow clarification alone left recipe rows unchanged; the duration decision below now clarifies six Utility effects. The owner rejects the draft 4 base gun damage: all bullet damage and effects come from its assembled parts. Review remaining shot-versus-round modifiers, post-hit resource/status gains and non-Ammo activation under the new flow. Do not silently apply an effect written for one shot to every shot or introduce a firing cost/cap.

**Utility damage-bonus duration, owner decision 15 September:** an otherwise unspecified Utility damage bonus applies to the **next shot only**. Turn-long or fight-long bonuses must explicitly state their duration. A lasting source that grants a bonus on a trigger does not automatically make that granted bonus repeat: distinguish the source's duration from each next-shot grant. Preserve explicit per-shot lasting effects, target restrictions and per-turn caps. Old references to a round's single main shot are clarified below without changing costs or amounts. If a recipe already restricts its bonus to the current or following round, retain that restriction. **Unused next-shot Utility damage bonuses expire at End Turn; they do not carry over.** An explicitly scheduled future grant still waits for its trigger, but its next-shot bonus must be spent in the turn when it becomes available. Explicitly longer-lasting effects keep their stated duration.

**Utility activation settled:** Recipe Use directly activates Utility effects, including cooling; no stored Utility item or second Utility control exists. Immediate Shield grants follow the 16 September exception below. **Owner correction, 15 September:** only End Turn activates the prepared shield, before enemy actions, regardless of shot count. Crafting, staging, Load and Fire leave Shield parts pending. Values add at End Turn: prepared 8 + 5 gives 13 with no other changes. Protection lasts through enemy actions, then follows existing reset/upgrade exceptions; explicit upgrade Shield grants keep their timing. The earlier Fire-activation rule and zero-shot-only exception are superseded. Reconcile draft effects assuming crafted Shield can supply preparation-time costs or shot bonuses; do not silently move their timing or invent new grants. **Recipe and Shop remain usable while loaded:** craft available recipes, use Utilities and buy goods under normal costs/cooldowns and other requirements; closing returns to the loaded preparation state. Secondary-effect ordering, used-part accounting, remaining loaded-bullet reservation/validation and other draft Helper/Modifier/gathering classifications remain open. Permanent claw upgrades install on acquisition; temporary gathering recipes retain next-round craft-and-fit timing.

**Owner clarification, 16 September — immediate Shield grants:** a recipe Shield grant puts the granted amount on an ordinary Shield part and automatically loads it into the shield. The player may remove it, edit the assembly and save the part for a later round under the same rules as other parts; there is no special lock or no-saving rule. End Turn activates parts left in the shield for the enemy phase, and their active Shield value resets at enemy-turn end under the normal permanent-upgrade exceptions. An activated grant does not grant Shield again next round. An unused part removed and saved in reserve is ordinary part storage, not retention of active Shield. Utility recipe Use still resolves immediately with no stored Utility item or second Utility activation; a granted Shield part is an explicit exception to the generic no-part wording. **Quick Vent example:** pay its normal recipe cost and 2 Heat to automatically load an ordinary part worth 5 Shield. Leave it loaded for this End Turn, or remove and save it like any other part. Printed costs and values are unchanged.

**Folding Brace — owner-selected replacement, 16 September:** SH026 now automatically loads an ordinary 4-Shield part on recipe Use and schedules an ordinary 6-Shield part for the beginning of the next round. Both use ordinary removal, storage and End Turn activation. Their active Shield resets after the enemy turn; the later delivery is a new part, not retained protection. Its schedule starts at recipe Use, even if the first part is removed and saved. Cost remains 2 Iron + 1 Glass and cooldown remains 1. This replaces the old 6-Shield/retain-up-to-4 effect; the proposed flat 10-Shield replacement was rejected.

**Analogous Shield replacements — authorized 16 September:** Spare Metal Brace loads 8 Shield on recipe Use and schedules 5 Shield next round if 3 Iron remain after its cost. The other eleven retention recipes keep their normal crafted source parts and End Turn activation, preserving saved-part bonuses and activation-time conditions; their future regular parts are scheduled relative to that activation round. Values in future parts use the old retention caps, without depending on leftover active Shield. Last Wall keeps its cooling reward conditional on at least 10 active Shield remaining just before enemy-turn-end reset; that snapshot controls cooling at next-round delivery. Field Lock qualifies when any Fire in its activation round began with at least 8 Charge, for one delivery from that activation. Every delivered regular part contains only its stated Shield value, with normal removal, storage and End Turn activation; it does not inherit the source part's delivery or secondary effects. Schedules stay within the current fight and expire when it ends. Costs, cooldowns, saved-part conditions, optional payments and stated secondary rewards are preserved. Early Cover's old expiry clause is also replaced with normal Shield-part behavior.

| Kind | When it works |
| --- | --- |
| Ammo | Load into the next bullet. Added damage contributes to one assembled main shot. The part is consumed when fired. Extra payload effects happen after the main hit unless the recipe says otherwise. |
| Shield | Craft and stage in the shield build; only End Turn activates the prepared parts before enemy actions, after zero, one or multiple shots. Load and Fire leave them pending. Prepared values add at activation and protect through enemy-turn end under the existing reset/upgrade rules. Activation needs no ammunition or Load. Used-part accounting, secondary-effect ordering and conflicting draft dependencies remain to reconcile. |
| Modifier | Use during planning on the shot, defence or named part described in the row. Its effect waits for the stated trigger. The part is consumed on use. |
| Utility | Activate the recipe directly on Use, paying its material cost and applying its use/cooldown rules. No Utility item is produced or stored. Immediate Shield grants automatically load a Shield part under the rule above. Stated ongoing/delayed effects are activated now and follow their triggers; conflicting draft outputs require revision below. |
| Helper | Use during planning to repair, restore or command Ada's helper. Follow its stated timing; it is not an extra player turn. |
| Magnet | Craft and fit a temporary magnet part for **next round only**. Its effect applies to that round's one haul and expires after it. These special parts are not held for an arbitrary later round. A pending boost also expires if the fight ends before it triggers; it cannot carry into the next fight. |

The selected round is collection, preparation with zero or more shots, End Turn activating the prepared shield, surviving enemies' actions and the enemy-turn-end reset/tick. Fire consumes the bullet and returns to preparation with Shield parts pending. Helpers and delayed parts may make support hits, distinct from assembled main shots. Prepared Shield values add only when End Turn activates them. Used-part accounting, secondary-effect ordering, other non-Ammo timing and remaining round-boundary ordering remain open. Enemy intent stays visible while choosing attack and defence.

**Owner decision, 15 September — bullet damage source:** the gun contributes **zero base damage** and no innate shot effects. All bullet damage and effects come from the parts assembled into it. The earlier +4 draft is superseded.

**Owner decision, 15 September — recipe-defined status targeting:** the recipe decides which targets receive its effects. For multi-target bullets, do not apply a blanket main-target-only default or automatically copy every status to all enemies hit. Each recipe must specify its scope clearly, such as the main target, a chosen other enemy or every enemy hit. Preserve explicit recipe-local restrictions on copied effects. Missing or ambiguous scope requires recipe-text reconciliation; it is not permission to invent a targeting rule. The 606 recipe rows retain their draft text and numbers in this decision-recording pass.

**Owner decision, 15 September — percentage damage bonuses:** Eligible percentage damage bonuses from different parts add together by default before the combined bonus is applied: +20% and +30% give +50%. Preserve explicit recipe conditions and non-stacking restrictions. This does not change bonus duration, effect targeting or the number of effect triggers. This confirms the catalogue's additive percentage-increase convention. The subsequent whole-number rework below defines numerical results; other resolution ordering remains draft.

### Whole-number combat quantities — 15 September 2026

**Owner requirement:** rework damage so the game deals only in whole numbers. **Design choice for this rework:** retain the just-approved percentage stacking and existing numerical recipe values, and use integer arithmetic for every resulting amount. Percentage effects are not replaced with flat bonuses. Damage, HP, Shield, status counts and material quantities never store or display a fractional remainder. This applies to the player, robots, Bolt, main hits, split hits, support hits, damage-over-time and recoil whenever their recipe or enemy effect calls for scaling.

Calculate a whole output once at each named effect step. Integer division discards the remainder; it is not banked or added to a later hit. Combine eligible percentage increases before calculating their one damage bonus, while respecting each recipe's conditions and non-stacking text. Do not round each contributing bonus separately. Existing targeting and effect-count rules remain in force.

| Operation | Whole-number calculation | Example |
| --- | --- | --- |
| Add eligible percentage increases | `D + (D * P) // 100`, where P is their sum | 9 damage with +60% gains 5 damage and becomes 14 |
| Spread or copy a percentage of damage | `(D * P) // 100` for each eligible target | 50% of 9 damage gives a 4-damage hit |
| One stated percentage damage reduction | `(D * (100 - R)) // 100` | Reducing a 9-damage attack by 50% leaves 4 damage |
| Half of damage, Shield, healing, status or a resource count | `A // 2` | Removing half of 5 Shield removes 2 and leaves 3 |
| Whole multiples | `A * N` | Double 7 damage is 14 |

D and A are nonnegative whole amounts; P is a whole percentage and R is a single reduction from 0 through 100. These are calculation definitions, not a new rule for stacking separate reductions. Clamp resolved damage at zero; do not impose a new minimum of 1. A 40% split of 1 damage therefore deals 0. Health thresholds such as half or quarter maximum HP remain conditions, evaluated with integer comparisons such as `2 * current_hp <= max_hp`; they do not produce a fractional quantity.

The shot preview shows whole contributions and whole results. Risky Packing on a 9-damage bullet shows **+5 damage, total 14**. A 50% split of that shot shows **7 damage** for the extra target before its own protection. Recompute these numbers when the build, target or relevant state changes. Percentage text may explain the recipe, but no decimal damage or hidden fractional accumulator appears in gameplay. This is a design requirement; no interface or engine implementation is claimed.

The [executable arithmetic study](analysis/whole_number_damage.py) covers all 606 catalogue rows, identifies the 23 rows containing percentages, and checks 40,401 combinations of whole damage 0–200 and percentage values 0–200. Boundary examples cover combined bonuses, odd splits, reductions, zero damage, half-Shield removal and the existing worked shot. Its checks passed on 15 September. It is an arithmetic model, not combat simulation, balance evidence or a playable build. Costs, recipe IDs, printed bonus values and all 606 recipe rows remain unchanged.

**Owner decision, 15 September — additive spread damage:** When different eligible spreading parts reach the same extra robot, add their whole damage contributions. On a 20-damage bullet, contributions of 10 and 8 total 18 damage before defence. This replaces the draft rule that kept only the largest contribution. Preserve recipe-specific eligibility and explicit non-stacking restrictions; this does not copy every main-target effect to the extra target. Each eligible spreading part resolves its contribution as a separate hit on that extra robot: 10 and 8 are two hits, totalling 18 before defence. Evaluate per-hit defences and reactions for each qualifying hit using the state at that hit. A once-per-hit robot buff can therefore trigger twice if both hits qualify, subject to its own conditions and limits. Overlapping spread hits resolve in the order their contributing parts were placed in the bullet. Placing the 50% part before the 40% part on a 20-damage bullet gives 10 then 8; reversing their placement gives 8 then 10. Do not sort those hits by damage or recipe ID. If an earlier hit from the same bullet has killed a later spread hit's target, that later hit is lost. It deals no damage, triggers no hit-based reaction and is not automatically redirected. Fired parts remain consumed. Other explicitly triggered recipe effects retain their own rules. Apply the existing death-ends-fight rule: once player death is resolved, stop the remaining hit resolution and enter Game Over. Preserve already-resolved damage and the selected simultaneous-death outcome. This is a consequence of the settled rule, not a new owner decision to request. Each spread contribution already uses the whole-number calculation above. For example, 50% and 40% of a 9-damage bullet contribute 4 and 3, for a total of 7 before defence. Unlike percentage increases to the main shot, these are separate recipe outputs whose whole amounts are added. The [arithmetic study](analysis/whole_number_damage.py) checks separate hit amounts [10, 8] and [4, 3]. Their summed amounts are preview totals. The study preserves part placement order, including [8, 10] when the 40% part is placed first; it does not resolve defence, reactions, interruptions or target order within one part's multi-target effect.

**Remaining draft arithmetic and spread-resolution conventions:** `Add 6 damage` adds six to the assembled main shot, not six separate attacks. Flat additions add together. Use the whole-number calculations above; effects cannot resolve fractional damage. A multiplier changes damage, not the number of times every payload triggers. Status-effect recipients follow the explicit target scope of each recipe. A shot-spreading part uses the shot's damage before target-specific Mark or defence; each extra target applies its own protection. Use the selected additive rule for overlapping eligible spread contributions. Explicit small support hits still happen as written.

For a calculation, begin with whole damage supplied by the assembled bullet parts, with no gun base term. Under the remaining draft resolution order, sum their flat damage, apply the combined whole-number percentage bonus defined above, then apply any Weaken on the player. That is the damage used for spreading. Add the main target's ordinary Mark bonus afterward, then resolve Shield and HP damage. A part that explicitly spends Mark before damage does so before the ordinary Mark bonus; it never spends the same Mark twice. Shield bypass redirects existing damage. Add eligible bypass amounts but cap them at the damage being dealt; full bypass already redirects all of it.

Keep Mark spent by a special part separate from Mark consumed for the ordinary damage bonus. Effects that increase or restore the shot's consumed Mark use only that ordinary amount unless they explicitly say otherwise. Extra targets receive no ordinary Mark bonus and keep their Mark unless a row specifically changes it.

**Owner decision, 15 September — separate spread hits:** each eligible spread part produces its own damage event on the extra target. The earlier combined-hit proposal is superseded. Resolve defences and evaluate qualifying per-hit reactions for the individual hit, carrying changed state into later hits. Kill, Shield-absorption and HP-damage conditions refer to the relevant hit under each effect's text. Preserve once-per-turn/fight limits and explicit non-stacking rules; two hits do not automatically bypass them or copy main-target payloads. Spread hits follow the placement order of their contributing parts. Check whether a later hit's target survived earlier hits; if already killed, cancel that hit without damage, hit-based reactions or automatic retargeting. For example, a robot with 10 HP dies to the first 10-damage hit, and the queued 8-damage hit is lost. This does not refund fired parts or undo the first hit's valid kill effects. Secondary-effect ordering and target order within a multi-target part remain to define. Resolved player death stops remaining hit resolution under the existing death-ends-fight rule. Main-shot damage assembly retains its existing combination rules.

**Owner decision, 15 September — player survival:** If the player and the last robot die in the same resolution, the outcome is Game Over. Direct HP costs or self-inflicted HP loss from effects the player uses must leave at least 1 HP. Damage returned by a robot's recoil buff is enemy damage, even when the player's attack triggers it, and can kill the player if it reduces HP to 0. The nonlethal rule for the player's own effects does not prevent robot recoil or other enemy damage. The seven player-HP-cost rows (SH050, MA019, MA046, MA054, MA092, MA116 and NO054) already specify leaving at least 1 HP; their printed costs and effects remain unchanged. Bolt's separate HP/disable mechanics are not player death and are not rewritten by this decision.

**Shield:** absorbs incoming damage before HP. Remaining Shield resets after the complete enemy turn by default, unless a permanent upgrade explicitly says otherwise. Permanent upgrades may grant beginning Shield each turn, at fight start or on a specified turn. A fresh Shield grant does not automatically retain old Shield. Ordinary recipes do not authorize reset exceptions. Shield cannot fall below zero. An enemy's Shield works the same way when it takes damage; `lose Shield` removes protection and is not HP damage. Paying HP bypasses Shield, must leave at least 1 HP, and is not enemy damage. A part with an extra Heat, Charge, Shield or reserve-part cost cannot be activated without paying that cost under the existing draft. **Owner decision, 15 September — player HP costs:** A recipe with player HP cost C requires at least C + 1 current HP to use. Pay the full HP cost; no partial payment is allowed. A 3-HP cost is unavailable at 3 HP or less; at 4 HP it leaves 1 HP. Printed recipe costs remain draft balance values.

Old Shield clears at enemy-turn end under the default; apply upgrade grants or explicit exceptions at their stated triggers. **Retention reconciliation completed, 16 September:** SH026 and the twelve formerly flagged retention recipes now provide explicitly scheduled fresh regular Shield parts instead of retaining active protection. Their source-part conditions and normal activation clocks remain explicit in the rows. SH034 now uses ordinary part storage and enemy-turn-end Shield expiry. SH060's explicit loss of remaining Shield remains a possible downside to an upgrade; it grants no retention. **Field Pocket clarified, 17 September:** NO060 counts remaining active Shield after enemy actions and before reset, then grants the recorded Charge next round. It needs no retained Shield and is no longer a reset conflict. Enemy-phase grant timing remains a separate review item.

**End-of-enemy-phase Shield readings — owner-authorized clarification, 17 September:** effects that use remaining Shield at enemy-turn end read or pay it after enemies finish their actions and before the Shield reset. For a next-round reward, record its amount before reset and deliver that recorded amount at its stated time; do not read the new round's Shield pool. Keep rewards already due at enemy-phase end at that boundary. Explicit Shield costs still consume available Shield before reset, with no double spending. This resolves the reset-relative timing in Field Pocket, Sweep the Plates, Wall Casting, Reclaimed Plate, Untouched Cover and Charge Cage. Ordering among multiple interacting end-phase effects remains part of the general effect-resolution specification; this clarification does not invent a new universal order.

Printed enemy attack intent means the displayed attack totals, including declared multi-hit damage, before your protection and interception. Changes to an enemy's planned damage update that display.

An attack is fully absorbed or blocked by your Shield only if it absorbs at least 1 damage and none of that attack reaches your HP. An attack reduced to zero before Shield does not count. For a multi-hit attack, check the whole attack after all its hits.

**Four small effect definitions used in the rows:** these are enough to read this recipe draft, not the later complete buff catalogue.

| Effect | Proposed meaning |
| --- | --- |
| Burn N | After that enemy acts, deal N damage to it, Shield first, then lower Burn by 1. |
| Corrosion N | Before that enemy acts, remove N HP directly, then lower Corrosion by 1. |
| Mark N | The next main shot against that enemy gains N damage against it, then removes the Mark. New Marks applied by the bullet's payload normally help a later shot. |
| Weaken N | Reduce that enemy's next attack's total damage by N, to a minimum of zero, then remove Weaken. Against a multi-hit attack, subtract from the hits in order until the reduction is used up. |

Repeated applications add their amounts. Effects end at zero or fight end. Burning and corrosion do not trigger main-shot bonuses. If an enemy has several actions in a round, its Burn/Corrosion tick once for that round, after/before its action sequence. Dead enemies stop acting and ticking, but player-owned kill rewards and transfers still resolve. Keep a target's just-before-hit effects available for a shot's stated on-kill transfers. Mark affects only the target that holds it. Unless a row says otherwise, choose targets when using the part; choose Ammo targets when firing. A conditional main-shot bonus checks its condition when Fire begins, before that shot changes the battlefield. Explicit pre-hit payments and effects then resolve in loading order, followed by main damage and after-hit effects in that order.

**When these effects are on the player:** Corrosion removes HP at the start of planning, after the prior enemy-end Shield reset and before fresh start-of-turn benefits. Burn damages Shield then HP after the player's action phase, even if no shot was fired. Each ticks once and then falls by 1. Mark adds its amount to the next enemy attack's total damage, then clears; for a multi-hit attack add it to the first hit. Weaken subtracts its amount from the next main shot's calculated damage before target bonuses, then clears. These are draft definitions so cleansing and protection recipes have a clear use; enemy move lists remain later work.

**Ongoing and delayed effects:** `For this fight` lasts only in this fight. Reapplying the same named ongoing effect refreshes it; copies do not stack unless a row gives a specific stacking rule and limit. Different named effects can work together. Ordinary copies of Ammo and Shield parts add their stated amounts. A support hit does not trigger another support hit just because it dealt damage; only explicitly named triggers apply. A part cannot copy itself or recursively copy a copying effect. Resource grants arrive next turn unless a row explicitly offers a lossy conversion now. These drafting rules prevent a free crafting/reset loop; they are not evidence of tested balance.

Parts described as saved or older entered reserve in an earlier round. Starting parts count as arriving in round 1. A sacrificed part is consumed from reserve, without also firing or activating its normal effect; its recipe stays in memory. A part cannot be both loaded and sacrificed. Conditions counting parts refer to physical parts used, not recipes owned. All part and raw-resource payments are explicit; none is secretly a recipe deletion.

If a loaded part needs another reserve part as payment, reserve that payment until firing. It cannot pay for two effects or be used elsewhere. Cancelling that assembly releases the reserved part; firing consumes it once. Ordinary turn effects expire at round end unless they explicitly name a later trigger or stay attached to a saved part. Reapplying the same named Modifier to the same shot or stored part refreshes its bonus rather than stacking copies; different named Modifiers combine.

Copies arrive as newly received parts: their age starts again, and they keep the source recipe's rarity and identity. **Owner correction, 15 September:** original and copy use the same part's standard main-recipe one-part ingredient basis for resale, so they sell for the same amount at the same current shop resource prices, independent of the copying recipe's cost. Generated subparts inherit their creator recipe's rarity; copying one reproduces that subpart, not its generator. If a generator fixed its damage or Shield when creating it, that fixed value is copied. Later attached bonuses are not. Per-recipe healing or trigger caps are shared across all its outputs and copies.

Magnet boosts change the next haul's amount or preference; they do not add a magnet activation. An extra named material must exist in the pile. If fewer units remain than the stated bonus, take only those available. Different named temporary magnet boosts may combine in this draft; identical copies refresh the same boost. Permanent claw upgrades are bought or accepted from Officer loot, activate immediately and stay outside this recipe list.

### Manual grab enhancements

**Owner-selected rule:** each turn choose baseline ingredient options and press Collect for ordinary handling without a timing input, or choose Precision. Once per fight, choose a turn to use the manual timing marker for a bonus. That attempt is spent regardless of its accuracy. Baseline supplies and held resources cannot be lost through a miss. Each new fight has one fresh attempt; unused attempts do not accumulate.

SH121–SH126 are shared discoveries: [three Rare](#rare--manual-grab-enhancements) and [three Legendary](#legendary--manual-grab-enhancements). They retain the existing **Magnet** kind as a legacy gathering-system category. The owner's later turn sequence selects a physical claw; renaming this catalogue category and all older recipe names is separate work. Craft and fit them now to affect **next round's single collection event only**. They cannot be stored for a later round. Choose any listed materials when crafting. A recipe's unconditional addition works with automatic handling, a missed timing attempt, or an already-spent manual grab. Its perfect-only addition requires spending this fight's unused manual grab next round and achieving Perfect. Skipping the grab next round does not preserve the enhancement.

**Owner decision, 15 September — fight-end expiry:** pending temporary next-round gathering boosts expire if the fight ends before they trigger. They do not carry into the next fight. Recipe memory and permanent claw upgrades retain their selected persistence.

**Draft resolution and stacking:** an enhancement expires after that collection event, or at the end of its target round if no collection occurs. Identical enhancements refresh rather than stack, including after a cooldown reset. Different named enhancements can combine. None widens the timing window, turns a miss into Perfect, grants a retry, restores a manual attempt or adds an ordinary haul. Removing recipe cooldown counters only changes crafting availability.

For Perfect Salvage, the **unmodified precision bonus** is the material bundle the ordinary timing result would award before temporary or permanent bonus additions and multipliers. Use the same bundle once more; exclude baseline supplies, automatic supplies, every recipe/upgrade bonus and generated parts. Two Salvage Amplifiers still add only one copy. Other perfect-triggered effects each resolve once; doubling the reward never repeats the timing event or those effects. The normal haul resolves first, followed by extra material requests in the order the enhancements were fitted. Material extras use only stock remaining in the pile under the catalogue's existing availability rule; a missing bonus material never reduces the baseline. Generated parts in SH126 are a stated part reward and require no second material payment or separate recipe in memory.

**Example, not a selected haul size:** fit Calibrated Jaws choosing Copper and Perfect Salvage on turn 2. On turn 3, suppose the ordinary perfect bonus is 3 Copper and sufficient Copper is available. The resulting bonus is 3 ordinary + 3 from Perfect Salvage + 1 unconditional from each enhancement + 2 from Calibrated Jaws = **10 Copper above baseline**. Recrafting either recipe does not increase its duplicate effect or add an attempt. With a non-perfect result or automatic collection, only the two unconditional Copper remain from these enhancements, on top of that mode's ordinary supplies. If the fight ends before turn 3, nothing carries over. Costs are paid on turn 2 and are not refunded. All costs, quantities and cooldowns below are tuning proposals.

## Four mercenaries and their starting kits

Klaus requests an inherent ability for each mercenary on 16 September. The [current ability proposal](MERCENARY-ABILITIES.md) assigns Quench Recovery to Mara, Find the Seam to Ivo, Field Service to Ada and Residual Current to Noor. These are free character passives, not memory entries or Mayor purchases; their exact triggers, names and values remain proposals. Quench Recovery and Residual Current replace the superseded Hot Barrel/Charged Barrel independent shot-damage drafts. No innate gun damage is restored. Other recipe/effect interactions still need their individual review; existing immediate-Utility use and selected bonus durations remain recorded. Klaus accepted the listed starter sets on 15 September as a provisional beta-balancing baseline: 12 recipes per character, 8 shared plus 4 exclusive. This does not establish final character identities or tested recipe balance.

| Mercenary | Weapon and specialty | Starting recipes | Full discovery pool |
| --- | --- | --- | --- |
| Mara “Kiln” Voss | Furnace Cannon. Build Heat, turn metal defence into a heavy hit, decide when to cool down. | SH001–SH008 + MA001–MA004 | SH001–SH126 + MA001–MA120 |
| Ivo “Rook” Vale | Needle Cannon. Corrode a chosen enemy, mark weak points and save the right part for a later round. | SH001–SH008 + IV001–IV004 | SH001–SH126 + IV001–IV120 |
| Ada “Patch” Flint | Rivet Cannon and Bolt the helper robot. Spend parts on attack, protection or keeping Bolt working. | SH001–SH008 + AD001–AD004 | SH001–SH126 + AD001–AD120 |
| Noor “Flux” Sayeed | Coil Cannon. Store Charge, split it between protection and damage, and cool useful recipes. | SH001–SH008 + NO001–NO004 | SH001–SH126 + NO001–NO120 |

The weapon's name does not force a particular final model. Every character assembles each shot from parts and prepares defence; multiple shots are allowed before End Turn. Their separate mechanics sit beside the shared recipes, which provide enough basic damage, protection, targeting and cooling for all four.

| Pool | Base | Common | Uncommon | Rare | Legendary | Unique recipes |
| --- | ---: | ---: | ---: | ---: | ---: | ---: |
| Shared | 8 | 32 | 40 | 31 | 15 | 126 |
| Mara | 4 | 36 | 40 | 28 | 12 | 120 |
| Ivo | 4 | 36 | 40 | 28 | 12 | 120 |
| Ada | 4 | 36 | 40 | 28 | 12 | 120 |
| Noor | 4 | 36 | 40 | 28 | 12 | 120 |
| **Total** | **24** | **176** | **200** | **143** | **63** | **606** |

Rarity is a discovery tier, not a second hidden level. Stronger rank variants of the same recipe can be designed later; they are not counted as additional unique recipes here. Higher rarities generally buy more effect per material or add a stronger combination. Cooldowns, targets, setup costs and risks still matter, so a legendary recipe is not automatically the right choice for every memory set.

## Shared recipes — every mercenary

Every mercenary starts with SH001–SH008 and can discover the remaining shared recipes. This pool supplies a common foundation without granting Heat, Charge or Bolt to the other characters. Its flexible parts can support several exclusive builds.

### Base

| ID | Recipe | Output / effect label | Kind | Resources | Cooldown | Effect |
| --- | --- | --- | --- | --- | --- | --- |
| SH001 | Solid Casting | 1 Plain Slug | Ammo | 1 Iron | None | Add 6 damage. |
| SH002 | Flat Plate | 1 Basic Shield Plate | Shield | 1 Iron + 1 Copper | None | Gain 6 Shield. |
| SH003 | Powder Packing | 1 Powder Cap | Ammo | 1 Carbon | None | Add 3 damage. |
| SH004 | Simple Sighting | 1 Aim Fin | Ammo | 1 Iron + 1 Glass | None | Add 4 damage. Add another 4 if the main target intends to attack this round. |
| SH005 | Split Outlet | 1 Forked Nozzle | Modifier | 2 Iron + 1 Copper | 1 | Choose one other enemy. The next main shot also hits it for 50% of the shot's damage. |
| SH006 | Basic Insulation | 1 Insulating Pad | Shield | 1 Copper + 1 Glass | None | Gain 4 Shield. Your first Burn tick this round deals no damage. |
| SH007 | Cooling Mix | Immediate effect: Coolant Plug (no part) | Utility | 1 Copper + 1 Glass | None | Remove 1 cooldown counter from every currently cooling recipe. This cannot reset a recipe with no cooldown ability. |
| SH008 | Extra Lift | 1 Magnet Lift Ring | Magnet | 1 Copper + 1 Circuit | 1 | Next round, the magnet gathers 2 extra units of available scrap. Their types follow the normal haul mix. |

### Common

| ID | Recipe | Output / effect label | Kind | Resources | Cooldown | Effect |
| --- | --- | --- | --- | --- | --- | --- |
| SH009 | Rivet Packing | 1 Rivet Bundle | Ammo | 2 Iron | None | Add 8 damage. Add another 2 if the shot contains at least three other Ammo parts. |
| SH010 | Heavy Casting | 1 Heavy Slug | Ammo | 2 Iron + 1 Carbon | None | Add 12 damage against a target with Shield, or 8 against one without Shield. |
| SH011 | Cutting Edge | 1 Cutting Nose | Ammo | 1 Iron + 1 Glass | 1 | Before the main hit, remove 6 Shield from its target. Add 3 damage. |
| SH012 | Finishing Head | 1 Blunt Finisher | Ammo | 2 Iron | 1 | Add 7 damage. Add another 5 if the target is at or below one quarter of its maximum HP. |
| SH013 | Hot Filling | 1 Ember Capsule | Ammo | 1 Iron + 1 Carbon | None | Add 4 damage and apply Burn 2 after the main hit. |
| SH014 | Acid Filling | 1 Acid Capsule | Ammo | 1 Carbon + 1 Glass | None | Add 2 damage and apply Corrosion 2 after the main hit. |
| SH015 | Paint Marker | Immediate effect: Bright Paint Pod (no part) | Utility | 1 Carbon + 1 Glass | None | Apply Mark 5 to one enemy now. |
| SH016 | Joint Snare | 1 Snare Bolt | Ammo | 1 Iron + 1 Copper | None | Add 5 damage and apply Weaken 4 after the main hit. |
| SH017 | Side Chip | 1 Chipping Tip | Ammo | 2 Iron + 1 Glass | None | Add 5 damage. After the main hit, deal 4 damage to one other chosen enemy. |
| SH018 | Packed Thread | 1 Packed Thread Sleeve | Ammo | 2 Iron + 1 Copper | 1 | Add 2 damage for each other Ammo part in the shot, up to 12 added damage. |
| SH019 | Mesh Weaving | 1 Wide Mesh Plate | Shield | 2 Iron | None | Gain 7 Shield, plus 3 more if at least two enemies intend to attack this round. |
| SH020 | Sloped Bracing | 1 Sloped Plate | Shield | 1 Iron + 1 Copper | None | Gain 6 Shield. Apply Weaken 2 to the first enemy that attacks you this round, after its attack. |
| SH021 | Firm Mount | 1 Ground Brace | Shield | 2 Iron + 1 Copper | 1 | Gain 12 Shield if exactly one enemy intends to attack this round; otherwise gain 7. |
| SH022 | Heat Lining | 1 Heatproof Liner | Shield | 1 Iron + 1 Glass | None | Gain 6 Shield. Remove up to 3 Burn from yourself now. |
| SH023 | Wire Lattice | 1 Woven Grid | Shield | 2 Copper + 1 Iron | None | Gain 5 Shield, plus 1 for each Copper you have spent crafting this round, up to 10 total Shield. |
| SH024 | Soft Mounting | 1 Impact Damper | Modifier | 1 Glass + 1 Carbon | 1 | Reduce the total damage of the next enemy attack against you this round by 6. This acts before Shield absorbs damage. |
| SH025 | Scrap Catcher | 1 Catch Plate | Shield | 1 Iron + 1 Copper | None | Gain 5 Shield. If you fully block an enemy attack this round, gain 1 Iron next turn; trigger once. |
| SH026 | Folding Brace | 1 regular Shield part now + 1 next round | Shield | 2 Iron + 1 Glass | 1 | On recipe Use, automatically load a regular Shield part worth 4 Shield. At the beginning of the next round, automatically load a new regular Shield part worth 6 Shield. Both parts may be removed and saved normally; parts left loaded activate at End Turn and their active Shield resets at enemy-turn end under the normal upgrade exceptions. The second delivery is scheduled by recipe Use, not by activating the first part. |
| SH027 | Contact Wire | 1 Contact Liner | Shield | 1 Iron + 1 Copper | None | Gain 4 Shield. After the first enemy attack against you this round, deal 3 damage back to that attacker. |
| SH028 | Pitched Roof | 1 Angled Cover | Shield | 2 Iron | None | Gain 3 Shield per living enemy, up to 12. |
| SH029 | Copper Recovery | Immediate effect: Copper Recovery Pack (no part) | Utility | 2 Iron | None | Gain 1 Copper now. The pack's Iron cost was spent when it was crafted. |
| SH030 | Glass Recovery | Immediate effect: Glass Recovery Pack (no part) | Utility | 2 Copper | None | Gain 1 Glass now. The pack's Copper cost was spent when it was crafted. |
| SH031 | Iron Recovery | Immediate effect: Iron Recovery Pack (no part) | Utility | 2 Carbon | None | Gain 1 Iron now. The pack's Carbon cost was spent when it was crafted. |
| SH032 | Spare Sorting | Immediate effect: Sorting Tray (no part) | Utility | 1 Copper | None | Sacrifice one unused part from reserve. Next turn, gain 1 Iron and 1 Glass. |
| SH033 | Clean Wash | Immediate effect: Cleaning Bottle (no part) | Utility | 1 Glass + 1 Carbon | 1 | Remove up to 2 Burn and up to 2 Corrosion from yourself. Gain 3 Shield. |
| SH034 | Early Cover | Immediate effect: schedule 1 Shield part next round | Utility | 1 Iron + 1 Copper | None | On recipe Use, schedule a new regular 8-Shield part to load at the beginning of the next round. It may be removed and saved normally. If left loaded, it activates at End Turn and its active Shield resets at enemy-turn end under the normal upgrade exceptions. |
| SH035 | First Layer | 1 Base Reinforcement | Modifier | 1 Iron + 1 Glass | None | The next Shield part you use this round gains 4 extra Shield. This bonus applies once and is not multiplied by that part. |
| SH036 | Spare Fuse | 1 Reserve Fuse | Modifier | 1 Carbon + 1 Copper | None | Choose one Ammo part in reserve. The next shot using it gains 4 extra damage; the mark on the part lasts until used or fight end. |
| SH037 | Iron Pull | 1 Iron-Seeking Ring | Magnet | 1 Copper + 1 Circuit | 1 | Next round's haul gathers up to 2 extra Iron from the pile. No extra Copper, Carbon, Glass or Circuit is granted by this ring. |
| SH038 | Copper Pull | 1 Copper-Seeking Ring | Magnet | 1 Glass + 1 Circuit | 1 | Next round's haul gathers up to 2 extra Copper from the pile. |
| SH039 | Carbon Pull | 1 Carbon-Seeking Ring | Magnet | 1 Iron + 1 Circuit | 1 | Next round's haul gathers up to 2 extra Carbon from the pile. |
| SH040 | Glass Pull | 1 Glass-Seeking Ring | Magnet | 1 Carbon + 1 Circuit | 1 | Next round's haul gathers up to 2 extra Glass from the pile. |

### Uncommon

| ID | Recipe | Output / effect label | Kind | Resources | Cooldown | Effect |
| --- | --- | --- | --- | --- | --- | --- |
| SH041 | Saw Fitting | 1 Saw Sleeve | Ammo | 2 Iron + 1 Glass | 1 | Before the main hit, remove half the target's Shield, rounded down. Add 8 damage. |
| SH042 | Breakaway Filling | 1 Breakaway Core | Ammo | 2 Iron + 1 Carbon | 1 | Add 8 damage. If the main hit kills its target, deal 10 damage to one other chosen enemy. |
| SH043 | Needle Lining | 1 Piercing Liner | Modifier | 1 Iron + 2 Glass | 1 | Up to 12 damage from the next main shot bypasses its main target's Shield. This redirects damage; it does not add another 12. |
| SH044 | Guarded Loading | 1 Braced Slug | Ammo | 2 Iron + 1 Copper | None | Add 8 damage, plus 5 more if you used at least three Shield parts this round. |
| SH045 | Return Spring | 1 Shield Return Spring | Ammo | 1 Iron + 2 Copper | 1 | Add 5 damage. After the main hit, gain 2 Shield per Ammo part consumed by the shot, up to 12. |
| SH046 | Ember Scatter | 1 Scattering Ember Tip | Ammo | 1 Iron + 2 Carbon | 1 | Add 7 damage. After the main hit, apply Burn 2 to every enemy other than the main target. |
| SH047 | Acid Mist | 1 Mist Capsule | Ammo | 1 Carbon + 2 Glass | 1 | Add 5 damage. After the main hit, apply Corrosion 2 to each living enemy. |
| SH048 | Loose Wiring | 1 Disrupting Wire Bundle | Ammo | 1 Iron + 2 Copper + 1 Circuit | 2 | Add 8 damage. After the main hit, apply Weaken 4 to every enemy that intends to attack this round. |
| SH049 | Marker Spray | Immediate effect: Marker Spray Bottle (no part) | Utility | 1 Carbon + 1 Glass + 1 Circuit | 1 | Apply Mark 4 to one enemy and Mark 2 to every other enemy. |
| SH050 | Risky Packing | 1 Overpacked Charge | Modifier | 1 Iron + 2 Carbon | 1 | Pay 4 HP when used, leaving at least 1. Increase the next main shot's damage by 60%. |
| SH051 | Three-Way Outlet | 1 Three-Way Nozzle | Modifier | 2 Iron + 1 Copper + 1 Glass | 2 | Choose up to two other enemies. The next main shot also hits each for 40% of the shot's damage. |
| SH052 | Rested Powder | 1 Settled Charge | Ammo | 1 Iron + 2 Carbon | None | Add 7 damage. If this part entered reserve in an earlier round, add another 8. |
| SH053 | Last Scrap | 1 Last-Scrap Slug | Ammo | 2 Iron + 1 Carbon | 1 | Add 5 damage, or 17 if no raw resources remain when Fire begins. |
| SH054 | Salvage Packing | 1 Salvage-Fed Core | Ammo | 2 Iron + 1 Copper | 1 | Add 8 damage, plus 3 per reserve part sacrificed this round, up to 17 total. |
| SH055 | Short Fuse | 1 Quick Burst Tip | Ammo | 1 Carbon + 1 Glass + 1 Circuit | None | Add 8 damage if the main target has not lost HP this fight; otherwise add 4. After the main hit, apply Weaken 3. |
| SH056 | Late Spark | 1 Delayed Spark Capsule | Ammo | 1 Iron + 1 Carbon + 1 Circuit | 1 | Add 7 damage. At the start of next turn, deal 8 damage to the same target if it is still alive. |
| SH057 | Spare Metal Brace | 1 regular Shield part now + conditional part next round | Shield | 2 Iron + 1 Copper | None | On recipe Use, automatically load a regular Shield part worth 8 Shield. If at least 3 Iron remain in your raw pool after paying the recipe cost, schedule a new regular 5-Shield part to load at the beginning of the next round. Both parts may be removed and saved normally. The delivery is scheduled by recipe Use, not by activating the first part. |
| SH058 | Break Alarm | 1 Alarm Plate | Shield | 2 Iron + 1 Circuit | 1 | Gain 9 Shield. The first time an enemy attack reduces your Shield to zero this round, apply Weaken 3 to every living enemy. |
| SH059 | Emergency Rivets | 1 Emergency Plate | Shield | 1 Iron + 1 Copper + 1 Glass | None | Gain 7 Shield, or 13 if your HP is at or below half its maximum. |
| SH060 | One-Use Stays | 1 Sacrificial Stay | Shield | 2 Iron + 1 Glass | 1 | Gain 16 Shield. At the end of the enemy phase, lose any Shield still remaining; it cannot be retained that round. |
| SH061 | Watched Approach | 1 Sighting Plate | Shield | 1 Iron + 1 Glass + 1 Circuit | 1 | Gain 7 Shield. Gain another 3 for each living enemy with Mark, up to 16 total. |
| SH062 | Impact Catch | 1 Catch-and-Brace Plate | Shield | 2 Iron + 1 Copper | 1 | Gain 6 Shield now. After your next main shot this round deals HP damage, gain another 7 Shield once. |
| SH063 | Sealed Joints | 1 Sealed Joint Cover | Shield | 1 Iron + 2 Glass | None | Gain 8 Shield. Remove up to 5 Corrosion from yourself. |
| SH064 | Clean Screen | 1 Filter Screen | Modifier | 1 Copper + 1 Glass + 1 Circuit | 2 | The next application of Burn, Corrosion, Mark or Weaken to you this round is prevented. Gain 5 Shield. |
| SH065 | Salvaged Cover | Immediate effect: Salvage Cover Frame (no part) | Utility | 1 Iron + 1 Copper | None | Sacrifice one unused Ammo part. Gain 10 Shield now and 1 Iron at the start of next turn. |
| SH066 | Sweep the Plates | 1 Plate Collection Tray | Modifier | 1 Iron + 1 Circuit | 1 | After enemies finish their actions, before the Shield reset, spend up to 9 remaining Shield. Record 1 Iron per complete 3 Shield actually spent, up to 3 Iron. At the beginning of the next round, gain that recorded Iron. Pay from the Shield still available at payment; the same Shield cannot pay two effects. |
| SH067 | Deep Cooling | Immediate effect: Deep Coolant Bottle (no part) | Utility | 1 Copper + 2 Glass | 1 | Remove 2 cooldown counters from every currently cooling recipe. |
| SH068 | Split Cooling | Immediate effect: Twin Coolant Bottle (no part) | Utility | 2 Copper + 1 Glass | 1 | Remove 1 cooldown counter from every currently cooling recipe. |
| SH069 | Tomorrow's Stock | Immediate effect: Stock Order Token (no part) | Utility | 1 Iron + 1 Circuit | None | Choose Copper, Carbon or Glass. Gain 2 of that material at the start of next turn. |
| SH070 | Iron Saver | 1 Iron-Saving Mould | Modifier | 1 Copper + 1 Glass + 1 Circuit | 1 | Next turn, the first recipe you craft that costs at least 2 Iron costs 1 less Iron. Other costs and crafting limits still apply. |
| SH071 | Spare Casting | Immediate effect: Spare Casting Mould (no part) | Utility | 2 Iron + 1 Copper + 1 Glass | 2 | **Superseded Utility dependency; revise before use.** Choose an unused Common Ammo part in reserve. At the start of next turn, receive one fresh copy with its printed effect; do not copy later bonuses attached to it. |
| SH072 | Return Delivery | 1 Delivery Catch | Modifier | 1 Iron + 1 Glass + 1 Circuit | 2 | Choose one Base or Common Ammo part loaded in the next shot. Return a fresh copy to reserve next turn after it is consumed. Trigger once; do not copy attached bonuses. |
| SH073 | Clear Controls | Immediate effect: Control Cleaner (no part) | Utility | 1 Copper + 1 Glass | None | Remove up to 6 Weaken and up to 6 Mark from yourself. Add 3 damage to the next main shot this round. |
| SH074 | Quick Patch | Immediate effect: Small Repair Kit (no part) | Utility | 1 Iron + 1 Glass + 1 Circuit | 2 | Restore 4 HP, up to your maximum. This recipe can restore at most 8 HP during one fight, across all copies of its part. |
| SH075 | Core-Side Salvage | 1 Salvage Hook Tip | Ammo | 2 Iron + 1 Circuit | 1 | Add 8 damage. If the main hit kills its target, gain 2 Copper next turn. This creates no energy core or credits. |
| SH076 | Plate Recasting | Immediate effect: Plate Recasting Sleeve (no part) | Utility | 1 Carbon + 1 Copper | 1 | **Superseded Utility dependency; revise before use.** Sacrifice one unused Shield part with a printed fixed Shield gain. Make one Recast Plate Slug adding that amount as shot damage, up to 14; it has no other effect. |
| SH077 | Metal Pair Pull | 1 Twin Metal Ring | Magnet | 1 Glass + 1 Circuit | 1 | Next round's haul gathers up to 1 extra Iron and 1 extra Copper from the pile. |
| SH078 | Furnace Pair Pull | 1 Furnace Supply Ring | Magnet | 1 Copper + 1 Circuit | 1 | Next round's haul gathers up to 1 extra Carbon and 1 extra Glass from the pile. |
| SH079 | Chip Search | 1 Circuit Search Head | Magnet | 1 Iron + 1 Copper + 1 Glass | 2 | Next round's haul gathers up to 1 extra Circuit from the pile. |
| SH080 | Selective Grip | 1 Selective Grip Head | Magnet | 1 Copper + 1 Glass + 1 Circuit | 1 | Choose two resource types when crafted. Next round, fill the haul's first three available units from those types before taking other scrap. Haul size does not increase. |

### Rare

| ID | Recipe | Output / effect label | Kind | Resources | Cooldown | Effect |
| --- | --- | --- | --- | --- | --- | --- |
| SH081 | Shield Breaker | 1 Shield-Breaking Head | Ammo | 2 Iron + 1 Carbon + 1 Glass | 2 | Remove all Shield from the main target before the hit, then add 14 damage. |
| SH082 | Wide Burst | 1 Wide Burst Nozzle | Modifier | 2 Iron + 1 Copper + 1 Circuit | 2 | The next main shot hits every other living enemy for 50% of its damage. Main-target payload effects are not copied. |
| SH083 | Layered Powder | 1 Layered Charge | Modifier | 1 Iron + 2 Carbon + 1 Circuit | 2 | Increase the next main shot's damage by 8% per Ammo part it consumes, up to 80%. |
| SH084 | Different Metals | 1 Mixed-Metal Core | Ammo | 2 Iron + 1 Copper + 1 Glass | 1 | Add 12 damage. Add 3 more for each different resource type used in crafting the Ammo parts in this shot, up to 27 total. |
| SH085 | Pressure Release | 1 Pressure Release Collar | Modifier | 2 Iron + 1 Copper + 1 Circuit | 2 | Spend up to 15 Shield now. Add twice the amount spent to the next main shot's damage. |
| SH086 | Clean Entry | 1 Clean Entry Needle | Ammo | 1 Iron + 2 Glass + 1 Circuit | 2 | Add 16 damage. If the main target has no Shield, apply Corrosion 6 after the main hit. |
| SH087 | Hot Follow-Through | 1 Follow-Through Ember | Ammo | 1 Iron + 2 Carbon + 1 Glass | 2 | Add 14 damage. After the main hit, deal damage equal to its target's Burn to one other chosen enemy, then remove half that Burn, rounded down. |
| SH088 | Fault Finder | 1 Fault-Finding Tip | Ammo | 1 Iron + 1 Copper + 2 Glass | 1 | Add 10 damage, plus 5 for each different effect among Burn, Corrosion, Mark and Weaken present on the target when Fire began. |
| SH089 | Double Tap Charge | 1 Follow-Up Charge | Ammo | 2 Iron + 1 Carbon + 1 Circuit | 2 | Add 12 damage. After the main hit, deal a separate 12 damage to its target if it survives. That support hit does not repeat payload effects. |
| SH090 | Stored Momentum | 1 Stored-Momentum Slug | Ammo | 2 Iron + 1 Carbon + 1 Glass | 2 | Add 10 damage, plus 8 for each full round this part spent in reserve, up to 34 total. |
| SH091 | Wall Casting | 1 Heavy Wall Plate | Shield | 3 Iron + 1 Copper | 2 | Gain 24 Shield. After enemies finish their actions, before the Shield reset, if any active Shield remains, deal 6 support damage to each surviving enemy that attacked you this round. |
| SH092 | Hinged Wall | 1 Hinged Wall Brace + next-round Shield part | Shield | 2 Iron + 1 Copper + 1 Glass | 2 | When this part activates at End Turn, gain 15 Shield and schedule a new regular 15-Shield part to load at the beginning of the next round. After the first enemy attack that removes any of your Shield in this activation round, apply Mark 4 to that attacker. |
| SH093 | Measured Cover | 1 Measured Cover Gauge | Shield | 2 Iron + 1 Glass + 1 Circuit | 2 | Gain Shield equal to half the total damage currently shown by enemy attack intents, rounded down, up to 30 Shield. |
| SH094 | Rebound Wall | 1 Rebound Plate | Shield | 2 Iron + 2 Copper | 2 | Gain 16 Shield. After each of the first two enemy attacks against you this round, deal 7 damage to that attacker. |
| SH095 | Quiet Round | 1 Quiet-Round Brace | Modifier | 1 Iron + 2 Copper + 1 Circuit | 2 | Apply Weaken 8 to each enemy that intends to attack this round. Gain 5 Shield for each living enemy that does not, up to 15 Shield. |
| SH096 | Two-Stage Cover | 1 Two-Stage Plate | Shield | 2 Iron + 1 Copper + 1 Circuit | 2 | Gain 12 Shield now and another 12 at the start of next turn. |
| SH097 | Crack Sealing | 1 Crack-Sealing Foam | Shield | 1 Iron + 2 Glass + 1 Circuit | 2 | Gain 14 Shield. Until this enemy phase ends, the first time enemy damage would reach your HP, reduce that HP damage by up to 8. |
| SH098 | Stored Plate Casting | 1 Waiting Wall Plate | Shield | 2 Iron + 1 Copper + 1 Glass | 2 | Gain 10 Shield, plus 7 for each full round this part spent in reserve, up to 31 total. |
| SH099 | Full Cooling | Immediate effect: Full Coolant Canister (no part) | Utility | 2 Copper + 2 Glass + 1 Circuit | 2 | Remove all cooldown counters from every currently cooling recipe. Each still needs its material cost paid to craft again. |
| SH100 | Cooling Manifold | Immediate effect: Coolant Manifold (no part) | Utility | 2 Copper + 1 Glass + 1 Circuit | 2 | Remove 1 cooldown counter from every currently cooling recipe. |
| SH101 | Salvage Choice | Immediate effect: Salvage Choice Tray (no part) | Utility | 1 Copper + 1 Glass + 1 Circuit | 2 | Sacrifice up to two unused parts. For each, choose Iron, Copper, Carbon or Glass and gain 2 of it next turn. |
| SH102 | Reserve Pattern | Immediate effect: Reserve Pattern Mould (no part) | Utility | 2 Iron + 1 Glass + 1 Circuit | 2 | **Superseded Utility dependency; revise before use.** Choose one unused Common Ammo or Shield part. Receive two fresh copies next turn. Copies keep printed effects, not bonuses later attached to the original. |
| SH103 | Better Moulding | 1 Fine Mould | Modifier | 1 Iron + 1 Copper + 1 Glass + 1 Circuit | 2 | The next Ammo part crafted this round gains 8 extra damage for its first use. If that recipe makes several parts, choose only one. |
| SH104 | Careful Repairs | Immediate effect: Careful Repair Kit (no part) | Utility | 2 Iron + 1 Glass + 1 Circuit | 3 | Restore 8 HP, up to your maximum. If no enemies intend to attack this round, also gain 8 Shield. This recipe restores at most 16 HP per fight across all copies. |
| SH105 | Heavy Magnet Lift | 1 Heavy Lift Brace | Magnet | 2 Iron + 1 Copper + 1 Circuit | 2 | Next round, gather 5 extra scrap units. After that haul, choose 2 gathered units and leave them in the pile; keep the rest. |
| SH106 | Focused Extraction | 1 Focused Pull Head | Magnet | 1 Copper + 1 Glass + 2 Circuit | 2 | Choose one resource type when crafted. Next round, gather up to 4 extra units of that type from the pile. |
| SH107 | Broad Collection | 1 Broad Collection Ring | Magnet | 1 Iron + 1 Copper + 1 Glass + 1 Circuit | 2 | Next round, gather up to 1 extra Iron, Copper, Carbon and Glass from the pile. The normal haul still occurs. |
| SH108 | Patient Sorting | 1 Patient Sorting Head | Magnet | 1 Copper + 2 Glass + 1 Circuit | 2 | Choose one type when crafted. Next round, up to 4 normal haul positions prioritise that type. If fewer units of it exist, fill the other positions normally. After the haul, gain 6 Shield. |

### Legendary

| ID | Recipe | Output / effect label | Kind | Resources | Cooldown | Effect |
| --- | --- | --- | --- | --- | --- | --- |
| SH109 | Citybreaker Casting | 1 Citybreaker Core | Ammo | 3 Iron + 2 Carbon + 1 Circuit | 3 | Add 36 damage. After the main hit, remove up to 12 Shield from each other enemy. |
| SH110 | Full-Spread Outlet | 1 Full-Spread Head | Modifier | 2 Iron + 2 Copper + 1 Glass + 1 Circuit | 3 | Sacrifice one unused Shield part when used. The next main shot hits every other living enemy for 100% of its damage. After those hits, apply Weaken 3 to surviving extra targets. Main-target payloads are not copied. |
| SH111 | Perfect Packing | 1 Perfect Charge | Modifier | 2 Carbon + 2 Glass + 1 Circuit | 3 | Increase the next main shot's damage by 100%. If it uses at least 10 Ammo parts, also gain 15 Shield after it fires. |
| SH112 | Chain Breaker | 1 Chain-Breaking Head | Ammo | 2 Iron + 1 Copper + 1 Carbon + 2 Circuit | 3 | Add 24 damage. If the main hit kills its target, deal 20 damage to every other enemy once. These hits cannot repeat this effect. |
| SH113 | Last Wall | 1 Last Wall Plate + next-round Shield part | Shield | 3 Iron + 1 Copper + 1 Glass | 3 | When this part activates at End Turn, gain 38 Shield and schedule a new regular 20-Shield part to load at the beginning of the next round. Just before the Shield reset at this enemy-turn end, check whether at least 10 active Shield remains. If so, the next-round delivery also removes 1 cooldown counter from every recipe currently cooling at that delivery. The cooling condition is checked before the reset; the 20-Shield delivery does not require leftover Shield. |
| SH114 | Answering Wall | 1 Answering Wall Liner | Shield | 2 Iron + 2 Copper + 1 Circuit | 3 | Gain 25 Shield. This round, after each enemy attack fully blocked by your Shield, deal 10 damage to that attacker; trigger up to three times. |
| SH115 | Shelter Reserve | Immediate effect: Shelter Reserve Pack (no part) | Utility | 2 Iron + 2 Glass + 1 Circuit | 3 | At the start of each of your next three turns, gain 18 Shield. Reusing this named effect refreshes its duration instead of adding another copy. |
| SH116 | Whole-System Cooling | Immediate effect: Whole-System Coolant (no part) | Utility | 2 Copper + 2 Glass + 2 Circuit | 4 | Remove all cooldown counters from every currently cooling recipe. Each still needs its material cost paid to craft again. |
| SH117 | Emergency Stockroom | Immediate effect: Stockroom Order (no part) | Utility | 2 Iron + 1 Copper + 2 Circuit | 3 | At the start of next turn, gain 2 Iron, 2 Copper, 2 Carbon and 2 Glass. This grant is lost if the fight ends first. |
| SH118 | Exact Duplication | Immediate effect: Exact Casting Mould (no part) | Utility | 2 Iron + 1 Copper + 1 Glass + 2 Circuit | 3 | **Superseded Utility dependency; revise before use.** Choose one unused Base, Common, Uncommon or Rare Ammo or Shield part. Receive two fresh copies next turn with printed effects only. Cannot copy Utility, Modifier, Helper or Magnet parts. |
| SH119 | Targeted Crane | 1 Targeted Crane Ring | Magnet | 2 Iron + 1 Copper + 1 Glass + 2 Circuit | 3 | Choose two different resource types when crafted. Next round, gather up to 3 extra units of each chosen type from the pile. |
| SH120 | Balanced Haul | 1 Balanced Haul Controller | Magnet | 1 Iron + 2 Copper + 1 Glass + 2 Circuit | 3 | Next round, reserve up to five normal haul positions for one of each resource type. Fill unavailable positions normally, then gather 4 extra units using the normal mix. |

### Rare — manual grab enhancements

Shared additions SH121–SH123. The [manual grab rules](#manual-grab-enhancements) define next-round expiry, the perfect condition and stacking.

| ID | Recipe | Output / effect label | Kind | Resources | Cooldown | Effect |
| --- | --- | --- | --- | --- | --- | --- |
| SH121 | Calibrated Jaws | 1 Precision Coupler | Magnet | 1 Copper + 1 Glass | 2 | Choose Iron, Copper, Carbon or Glass when crafted. Next round's haul gains 1 extra unit of that material with any handling result. If you use the manual grab next round and achieve Perfect, gain 2 more units of that material. |
| SH122 | Clean Separation | 1 Sorting Gate | Magnet | 1 Iron + 1 Copper + 1 Glass | 2 | Choose two different materials from Iron, Copper, Carbon and Glass when crafted, naming a first and second. Next round's haul gains 1 extra unit of the first with any handling result. On a Perfect manual grab next round, gain 1 more of the first and 2 of the second. |
| SH123 | Chip Finder | 1 Chip Sieve | Magnet | 1 Copper + 1 Glass | 2 | Next round's haul gains 1 extra Glass with any handling result. On a Perfect manual grab next round, also recover 2 extra Circuit from the pile. This does not make Circuit exclusive to timing rewards. |

### Legendary — manual grab enhancements

Shared additions SH124–SH126. They enhance the same once-per-fight attempt and never grant another.

| ID | Recipe | Output / effect label | Kind | Resources | Cooldown | Effect |
| --- | --- | --- | --- | --- | --- | --- |
| SH124 | Perfect Salvage | 1 Salvage Amplifier | Magnet | 1 Copper + 1 Circuit | 3 | Next round's haul gains 1 extra Copper with any handling result. On a Perfect manual grab next round, collect one additional copy of the unmodified precision-bonus material bundle. Exclude baseline supplies, other recipe/upgrade bonuses and generated parts. Identical amplifiers do not stack. |
| SH125 | Full Spectrum | 1 Spectrum Sorter | Magnet | 2 Copper + 1 Glass + 1 Circuit | 3 | Next round's haul gains 1 extra Iron and 1 extra Copper with any handling result. On a Perfect manual grab next round, also collect 1 extra Iron, Copper, Carbon, Glass and Circuit. Each named material is checked separately for availability. |
| SH126 | Salvage Foundry | 1 Auto-Forge Die | Magnet | 1 Iron + 1 Copper + 1 Carbon + 1 Circuit | 3 | Next round's haul gains 1 extra Iron with any handling result. On a Perfect manual grab next round, also receive 1 Salvage Slug and 1 Salvage Shield Plate in reserve after collection, with no further material cost. The slug is Ammo adding 10 damage; the plate is Shield giving 10 Shield when used. Each is consumed on use and can be saved within this fight. |

### Shared combinations to try

- **Crowd clear:** SH001 Plain Slug + SH052 Settled Charge saved from an earlier round + SH083 Layered Charge + SH082 Wide Burst Nozzle. The multiplier strengthens the main shot and its spread; it does not repeat every payload.
- **Defence into damage:** SH091 Heavy Wall Plate, then SH085 Pressure Release Collar, with SH026 Folding Shield Brace. Decide how much of the protection to spend and how much to keep.
- **Plan a better next round:** SH069 Stock Order Token, SH037 Iron-Seeking Ring and SH034 Delayed Plate Pack. Pay now for supplies and protection later; all of it is lost if the fight ends first.
- **Save the manual grab:** fit SH121 Precision Coupler and SH124 Salvage Amplifier, survive the enemy phase, then spend the saved manual attempt next round. A Perfect result improves that one bonus haul; a miss retains the baseline and unconditional additions. The calculation above separates each contribution.

These are examples, not extra recipes. Shared cooling can help any mercenary reuse a recipe with counters, but cannot grant another use of a recipe whose cooldown is None.

## Mara Kiln Voss

**Weapon:** Furnace Cannon.


**Draft identity:** Mara keeps a heavy furnace running through enemy fire. She can stay hot for larger shots, spend that heat on protection, or turn spare armour into an attack. Her choices are about when to build pressure and when to release it.

**Proposed inherent ability — Quench Recovery:** the first time each player turn Mara actually spends Heat through a successfully used recipe or activated part, heal 2 HP, up to 6 HP actually restored per fight. Ordinary Heat decay does not qualify. A full-HP trigger uses that turn's opportunity but does not spend the fight's healing allowance. Resolve after the activation and immediate enemy reactions while Mara is alive; healing cannot pay an up-front cost or revive her. See the [complete ability rules](MERCENARY-ABILITIES.md#mara--quench-recovery). This replaces Hot Barrel's independent shot-damage grant.

**Retained Heat convention, still proposed:** Heat starts at 0 each fight, stays between 0 and 10, and carries between turns. After the enemy phase, lose 2 Heat. Gains above 10 are lost. A listed Heat payment happens when the part is activated; without that Heat, the part cannot be activated. Ordinary Heat loss never goes below 0. Ammo that grants Heat does so after impact, so that gain cannot satisfy an earlier condition on the same shot. No automatic damage or HP penalty comes from reaching 10 Heat. Heat itself adds no gun damage; recipes and parts define its uses.

**Three possible builds:**

- **Keep the furnace hot:** Fuel Brick (MA001), Thick Insulation (MA015), Steady Furnace (MA056), and Red Furnace (MA110). Build Heat early, preserve it through enemy phases, and invest in a strong continuing Burn plan.
- **Armour into impact:** Boiler Jacket (MA003), Armour Chips (MA027), Pressed Shield (MA061), and Walking Foundry (MA112). Prepare enough protection, then choose how much to spend on the shot.
- **Spend health carefully:** Blood Fuel (MA019), Emergency Grate (MA054), Pressure Debt (MA092), and Last Reserve (MA116). Exchange a small amount of HP for a burst, then cover the next enemy attack. These recipes do not restore the HP spent.

**Future permanent-upgrade hooks, not an upgrade catalogue:** start each fight with some Heat; reduce normal Heat loss between rounds; recover a small, limited amount of Shield next round after spending Shield on a shot. Exact values and availability remain to design.

### Base

| ID | Recipe | Output / effect label | Kind | Resources | Cooldown | Effect |
| --- | --- | --- | --- | --- | --- | --- |
| MA001 | Fuel Brick | Immediate effect: Fuel Briquette (no part) | Utility | 1 Carbon | None | Gain 3 Heat when activated. |
| MA002 | Hot Cast | 1 Fresh Slug | Ammo | 1 Iron + 1 Carbon | None | Add 6 damage. If Heat is at least 4 at Fire, apply Burn 2 to the main target after impact. |
| MA003 | Boiler Jacket | 1 Wrapped Plate | Shield | 1 Iron + 1 Copper | None | Gain 6 Shield and 1 Heat when activated. |
| MA004 | Quick Vent | Immediate effect: Vent Valve (no part) | Utility | 1 Copper | None | Pay 2 Heat when activated. Gain 5 Shield. |

### Common

| ID | Recipe | Output / effect label | Kind | Resources | Cooldown | Effect |
| --- | --- | --- | --- | --- | --- | --- |
| MA005 | Long Cast | 1 Long Slug | Ammo | 2 Iron | None | Add 9 damage. Add 5 more if this is the only Ammo part consumed in the main shot; Modifier parts do not break this condition. |
| MA006 | Iron Teeth | 1 Toothed Nose | Ammo | 2 Iron + 1 Glass | None | Add 7 damage. Before main damage, remove up to 6 Shield from the main target. If the full 6 Shield was removed, gain 2 Heat after impact. |
| MA007 | Deep Charge | 1 Denting Slug | Ammo | 2 Iron + 1 Copper | None | Add 6 damage. After impact, apply Mark 4 to the main target. |
| MA008 | Chain Cinder | 1 Cinder Collar | Ammo | 1 Iron + 2 Carbon | 1 | Add 5 damage. After impact, apply Burn 2 to every living enemy except the main target, then gain 1 Heat per enemy affected, up to 3 Heat. |
| MA009 | Ash Cork | 1 Cold Plug | Ammo | 1 Iron + 1 Glass | None | Add 8 damage. Add 6 more if Heat is 2 or less at Fire. |
| MA010 | Molten Collar | 1 Hot Collar | Modifier | 1 Iron + 1 Copper | None | Pay 3 Heat when activated. Add 10 damage to this round's main shot. |
| MA011 | Furnace Dust | 1 Furnace Ember Capsule | Ammo | 1 Iron + 2 Carbon | None | Add 4 damage. After impact, apply Burn 5 to the main target if Heat was at least 6 at Fire; otherwise apply Burn 2. |
| MA012 | Split Nose | 1 Forked Tip | Ammo | 2 Iron + 1 Glass | None | Add 5 damage. After impact, deal a separate 3 damage to one other enemy chosen at Fire. If it had Burn immediately before this support hit, gain 2 Heat afterward. If it is dead when the support hit would begin, both effects are lost. |
| MA013 | Flameproof Liner | 1 Ash Liner | Shield | 1 Iron + 1 Carbon | None | Gain 7 Shield. Gain 4 more if at least one living enemy has Burn when activated. |
| MA014 | Breathing Plate | 1 Vented Plate | Shield | 1 Iron + 1 Copper | None | Gain 5 Shield and 2 Heat when activated. |
| MA015 | Thick Insulation | 1 Furnace Wrap | Shield | 2 Iron + 1 Carbon | 1 | Gain 8 Shield. Skip the normal 2 Heat loss after this round's enemy phase. Extra copies do not skip a later round's Heat loss. |
| MA016 | Spare Lid | 1 Folded Cover | Shield | 2 Iron + 1 Copper | None | Gain 5 Shield now and 5 Shield at the start of next turn, after old Shield clears. |
| MA017 | Quench Ribs | 1 Cooling Frame | Shield | 1 Iron + 1 Copper | None | Pay 3 Heat when activated. Gain 10 Shield. |
| MA018 | Shard Apron | 1 Jagged Apron | Shield | 2 Iron + 1 Glass | None | Gain 6 Shield. After the first enemy attack against you this round, deal 3 damage to that attacker. If Heat is at least 6 after that attack, also apply Burn 2. These effects work even if Shield absorbed the whole attack. |
| MA019 | Blood Fuel | Immediate effect: Emergency Fuel Cell (no part) | Utility | 1 Carbon + 1 Copper | 1 | Lose 3 HP when activated, leaving at least 1 HP, then gain 5 Heat. Shield cannot pay this HP cost. |
| MA020 | Overfeed | Immediate effect: Slow Fuel Cake (no part) | Utility | 2 Carbon | None | Gain 1 Heat now and 5 Heat at the start of next turn. |
| MA021 | Flue Primer | Immediate effect: Soot Catcher (no part) | Utility | 1 Iron + 1 Copper | None | At the start of next turn, gain 2 Carbon in your raw resource pool. |
| MA022 | Scrap Weld | 1 Welding Clamp | Shield | 1 Iron + 1 Carbon | None | When activated, consume one unused Ammo part from reserve without its effect. Gain 12 Shield. Cannot activate without an Ammo part to consume. |
| MA023 | Heated Rivet | 1 Glowing Rivet | Ammo | 1 Iron + 1 Carbon | None | Add 7 damage. Gain 2 Heat after the main shot's impact. |
| MA024 | Front Brace | 1 Bracing Bar | Shield | 2 Iron + 1 Copper | None | Gain 9 Shield. Gain 3 more if the sum of currently shown enemy attack damage is at least 15 when activated. |
| MA025 | Narrow Bore | 1 Narrow Slug | Ammo | 2 Iron + 1 Copper | None | Add 9 damage. Add 5 more if exactly one enemy is alive at Fire. |
| MA026 | Cooling Mist | Immediate effect: Mist Canister (no part) | Utility | 1 Copper + 1 Glass | 1 | Lose 3 Heat, then apply Weaken 5 to one chosen enemy. Can activate even with less than 3 Heat. |
| MA027 | Armour Chips | 1 Shrapnel Press | Modifier | 1 Iron + 1 Glass | None | Pay 6 Shield when activated. Add 12 damage to this round's main shot. Cannot activate with less than 6 Shield. |
| MA028 | Rivet Screen | 1 Rivet Mesh | Shield | 1 Iron + 1 Copper | None | Gain 5 Shield. Gain 2 more for each other Shield part already activated this round, up to 6 extra Shield. |
| MA029 | Soot Stamp | Immediate effect: Blackened Stamp (no part) | Utility | 1 Carbon + 1 Glass | None | Apply Mark 3 to one chosen enemy. If you have at least 6 Heat when activated, also apply Weaken 2 to that enemy. |
| MA030 | Warm Storage | Immediate effect: Heated Rack (no part) | Utility | 1 Iron + 1 Copper | 1 | Choose one unused Ammo part already in reserve. If that exact part is consumed in a main shot next round, add 8 damage to that shot only. Multiple racks on the same part do not stack. |
| MA031 | Firebreak | 1 Broad Baffle | Shield | 1 Iron + 2 Carbon | None | Gain 4 Shield for each living enemy that has Burn when activated, up to 12 Shield. |
| MA032 | Buried Ember | 1 Buried Coal Tip | Ammo | 1 Iron + 1 Carbon + 1 Glass | 1 | Add 8 damage. At the start of next turn, apply Burn 4 to this shot's main target if it is still alive. |
| MA033 | Scrap Mallet | 1 Heavy Ram | Ammo | 3 Iron | None | Add 12 damage. If the main target intends to gain Shield this round, apply Weaken 4 to it after impact. |
| MA034 | Backplate | 1 Rear Guard | Shield | 2 Iron | None | Gain 8 Shield now. After this round's main shot, gain 4 more Shield if it consumed at most one Ammo part. Modifier parts do not count toward this limit. |
| MA035 | Clean Valve | Immediate effect: Fresh Valve (no part) | Utility | 1 Copper + 1 Glass | 1 | Pay 2 Heat when activated. Remove 1 cooldown counter from every currently cooling recipe. |
| MA036 | Furnace Hook | 1 Drag Hook | Ammo | 2 Iron + 1 Copper | None | Add 7 damage. After impact, apply Weaken 5 to the main target if it has at least 3 Burn. |
| MA037 | Shield Boiler | Immediate effect: Pressure Tank (no part) | Utility | 1 Iron + 1 Copper | None | Pay 5 Shield when activated, then gain 4 Heat. Cannot activate with less than 5 Shield. |
| MA038 | Rivet Bundle | 2 Warm Rivets | Ammo | 2 Iron + 1 Carbon | None | Each Warm Rivet adds 4 damage to the main shot and gives 1 Heat after impact. They may be used in different rounds. |
| MA039 | Dull Cap | 1 Blunt Cap | Ammo | 1 Iron + 1 Copper + 1 Glass | None | Add 7 damage. After impact, apply Weaken 3 to the main target; apply 3 more if it is showing an attack intent at Fire. |
| MA040 | Long Cooling | 1 Heavy Cooling Fin | Shield | 2 Iron + 1 Copper | 1 | Pay 4 Heat when activated. Gain 7 Shield now and 7 Shield at the start of next turn, after old Shield clears. |

### Uncommon

| ID | Recipe | Output / effect label | Kind | Resources | Cooldown | Effect |
| --- | --- | --- | --- | --- | --- | --- |
| MA041 | Furnace Fork | 1 Split Barrel Insert | Modifier | 2 Iron + 1 Copper + 1 Glass | 1 | Pay 4 Heat when activated. At Fire, choose one extra enemy. It receives 50% of the shot's damage before target-specific Mark and defence, then applies its own protection. After that hit, apply Burn 2 to it if alive. Main-target payloads are not copied. Extra copies do not repeat this spread. |
| MA042 | Slag Fuse | 1 Slag Charge | Ammo | 2 Iron + 1 Carbon + 1 Glass | 1 | Add 9 damage. Before main damage, remove up to 6 Burn from the main target and add 2 damage for each Burn removed. |
| MA043 | Side Flue | Immediate effect: Flue Fan (no part) | Utility | 1 Copper + 2 Carbon | 1 | Apply Burn 3 to every enemy that already has Burn. Gain 1 Heat for each affected enemy, up to 3 Heat. |
| MA044 | Tempered Lip | 1 Spring Plate | Shield | 2 Iron + 1 Copper | 1 | Gain 10 Shield. The first time an enemy attack reduces your Shield from a positive amount to 0 this round, gain 6 Shield after that attack finishes. This cannot protect against the hit that broke the Shield. |
| MA045 | Heat Balance | 1 Adjustable Collar | Modifier | 1 Iron + 1 Copper + 1 Carbon | None | When activated, choose and pay 2 to 5 Heat. Add 3 damage to this round's main shot for each Heat paid. |
| MA046 | Coal Debt | Immediate effect: Sealed Fuel Drum (no part) | Utility | 1 Iron + 1 Carbon | 2 | Lose 2 HP when activated, leaving at least 1 HP. Gain 2 Heat now and 3 Carbon at the start of next turn. Shield cannot pay the HP cost. |
| MA047 | Covering Impact | 1 Folding Slug | Ammo | 2 Iron + 1 Copper + 1 Glass | 1 | Add 8 damage. Gain 8 Shield after the main shot's impact, before enemy actions. |
| MA048 | Holdfast | 1 Locking Plate + next-round Shield part | Shield | 2 Iron + 2 Copper | 1 | When this part activates at End Turn, gain 10 Shield. You may pay 3 Heat at that activation. Schedule a new regular Shield part to load at the beginning of the next round: 12 Shield if you paid, or 4 Shield if you did not. |
| MA049 | Smothering Hood | Immediate effect: Smoke Hood (no part) | Utility | 1 Copper + 1 Carbon + 1 Glass | None | Choose an enemy with Burn. Apply Weaken equal to its current Burn, up to 8. Its Burn is not removed. Cannot activate without a burning enemy. |
| MA050 | Foreman's Stamp | Immediate effect: Warning Stamp (no part) | Utility | 1 Copper + 1 Carbon + 1 Glass | 1 | Choose one enemy. If it currently shows an attack intent, apply Weaken 6; otherwise apply Mark 7 and Burn 2. |
| MA051 | Worn Mould | 1 Recast Slug | Ammo | 2 Iron + 1 Carbon | 1 | Add 6 damage, plus 2 for each Recast Slug consumed in an earlier main shot this fight, up to 12 extra damage. Recast Slugs in the same shot do not increase each other's bonus. |
| MA052 | Scrap Compactor | 1 Compacting Sleeve | Modifier | 2 Iron + 1 Copper | 1 | When activated, consume 1 to 3 unused Ammo parts from reserve without their effects. Add 7 damage to this round's main shot per part consumed. Those parts do not count as Ammo consumed in the shot. |
| MA053 | Furnace Window | 1 Heat Lens | Modifier | 1 Copper + 2 Glass | None | If Heat is at least 6 when activated, add 8 damage to this round's main shot and apply Mark 4 to one chosen enemy now. Otherwise gain 2 Heat and add 3 damage. |
| MA054 | Emergency Grate | 1 Red Grate | Shield | 2 Iron + 1 Carbon | 1 | Lose 3 HP when activated, leaving at least 1 HP, then gain 18 Shield. If HP had already been lost this round before paying this cost, also gain 2 Heat. Shield cannot pay the HP cost. |
| MA055 | Reclaimed Plate | 1 Return Plate | Shield | 2 Iron + 1 Copper + 1 Carbon | 1 | Gain 10 Shield. After enemies finish their actions, before the Shield reset, count your total remaining active Shield and record 1 Iron per complete 5 Shield, up to 3 Iron. At the beginning of the next round, gain that recorded Iron. |
| MA056 | Steady Furnace | Immediate effect: Steady Burner (no part) | Utility | 2 Carbon + 1 Copper + 1 Circuit | 2 | For this fight, gain 1 Heat at the start of each turn after the current turn. Reusing Steady Burner does not add another Heat source. |
| MA057 | Banked Coals | 1 Coal Pocket | Shield | 1 Iron + 2 Carbon | 1 | Gain 8 Shield. If no enemy attack causes you to lose HP during this round's enemy phase, gain 4 Heat at the start of next turn. HP costs paid in planning do not break this condition. |
| MA058 | Open Exhaust | Immediate effect: Exhaust Nozzle (no part) | Utility | 1 Copper + 1 Carbon + 1 Glass | 1 | Pay 5 Heat when activated. Deal 6 damage to every enemy now. These are support hits, not a main shot. |
| MA059 | Soft Landing | 1 Crushable Pad | Shield | 2 Iron + 1 Glass | None | Gain 7 Shield. Until the end of this enemy phase, reduce the total damage of the first enemy attack against you by 5, to a minimum of 0. |
| MA060 | Long Ember | 1 Slow Ember Tip | Ammo | 1 Iron + 2 Carbon + 1 Circuit | 1 | Add 6 damage. After impact, apply Burn 3 to the main target. For its next two Burn ticks, do not reduce that enemy's Burn after the tick. Reapplying refreshes the two-tick duration. |
| MA061 | Pressed Shield | 1 Armour Press | Modifier | 2 Iron + 1 Copper | 1 | When activated, choose and pay 4 to 12 Shield. Add 2 damage to this round's main shot per Shield paid, and gain 1 Heat for each full 4 Shield paid. |
| MA062 | Cold Riveting | 1 Cold Rivet Pack | Shield | 2 Iron + 1 Copper | None | Gain 6 Shield. Gain 10 more if Heat is 0 when activated; otherwise lose 2 Heat after gaining Shield. |
| MA063 | Furnace Grips | 1 Heavy Grip | Modifier | 1 Iron + 1 Copper + 1 Carbon | 1 | Add 3 damage to this round's main shot for each different Ammo recipe represented by parts consumed in that shot, up to 15 damage. Multiple copies from one recipe count once. |
| MA064 | Copper Wash | Immediate effect: Wash Nozzle (no part) | Utility | 2 Copper + 1 Glass | 1 | Pay 3 Heat when activated. Remove 1 cooldown counter from every currently cooling recipe. |
| MA065 | Cinder Gate | 1 Hinged Fire Gate | Shield | 2 Iron + 1 Carbon + 1 Glass | 1 | Gain 10 Shield. After each enemy's first attack against you this round, apply Burn 2 to that enemy. Each enemy can trigger this once, whether or not HP is lost. |
| MA066 | Reheat Box | Immediate effect: Reheat Box (no part) | Utility | 1 Iron + 2 Carbon | None | Choose one unused part already in reserve. If it is activated or fired during next round, gain 4 Heat immediately after its effect resolves. It can receive this bonus from only one Reheat Box. |
| MA067 | Barrel Weight | 1 Barrel Weight | Modifier | 2 Iron + 1 Copper | 1 | Add 18 damage to this round's main shot. After impact, lose 4 Heat. Cannot activate if a spread effect is active or spreading Ammo is loaded. Until this shot resolves, no spread effect may be activated and no spreading Ammo may be loaded. |
| MA068 | Braced Shot | 1 Shoulder Brace | Ammo | 2 Iron + 1 Copper + 1 Glass | None | Add 10 damage. If you have at least 12 Shield at Fire, also add 25% to this shot's damage. Extra Shoulder Braces add their flat damage but do not repeat this percentage bonus. |
| MA069 | Furnace Receipt | Immediate effect: Batch Counter (no part) | Utility | 1 Copper + 1 Circuit | 1 | At the start of next turn, gain 1 Iron for each different recipe you crafted after using this Utility recipe this round, up to 4 Iron. Recasting the same recipe counts once. |
| MA070 | Stoked Armour | 1 Heat Cladding | Shield | 2 Iron + 1 Copper + 1 Carbon | None | Gain 8 Shield, plus 1 Shield for each Heat you have when activated. Heat is not spent. |
| MA071 | Rescue Valve | 1 Safety Valve | Shield | 1 Iron + 2 Copper | 1 | Gain 6 Shield. After the first enemy attack that causes you to lose HP this round, lose 5 Heat and gain 12 Shield. This protection arrives after that attack. |
| MA072 | Smelting Screen | 1 Smelting Mesh | Shield | 2 Iron + 1 Carbon + 1 Circuit | 2 | Gain 9 Shield. After the first enemy that has Burn attacks you this round, gain 1 Iron at the start of next turn. Extra meshes do not grant more Iron. |
| MA073 | Heavy Replacement | 1 Replacement Shell | Ammo | 3 Iron + 1 Carbon | 1 | Add 12 damage. Add 10 more if you consumed at least one unused part from reserve as another effect's cost this round. |
| MA074 | Heat Transfer | Immediate effect: Transfer Tube (no part) | Utility | 1 Copper + 2 Carbon | None | Pay 4 Heat when activated. Apply Burn 7 to one chosen enemy. |
| MA075 | Spare Furnace Door | 1 Second Door | Shield | 3 Iron + 1 Copper | 1 | Gain 18 Shield. You cannot activate this part unless at least one other Shield part has already been activated this round. |
| MA076 | Deep Freeze Cast | 1 Quenched Spike | Ammo | 2 Iron + 1 Copper + 1 Glass | 1 | Add 10 damage. If at least 5 Heat has been paid as part activation costs this round, remove 10 Shield from the main target before main damage. Ordinary Heat loss does not count. |
| MA077 | Smouldering Trail | 1 Trailing Wick | Ammo | 1 Iron + 2 Carbon + 1 Glass | 1 | Add 7 damage. After impact, choose one other enemy and give it Burn equal to half the main target's current Burn, rounded down, up to Burn 6. The main target keeps its Burn. |
| MA078 | Dense Pack | 1 Packing Ram | Modifier | 2 Iron + 1 Copper + 1 Carbon | 1 | Add 4 damage to this round's main shot per Ammo part consumed after the first, up to 16 damage. If the shot consumes at least five Ammo parts, lose 3 Heat and gain 6 Shield after impact. |
| MA079 | Furnace Rest | 1 Rest Plate | Shield | 2 Iron + 1 Copper | None | Gain 12 Shield. Gain 2 Heat at the start of next turn if no Heat was paid as an activation cost after this part was activated this round. |
| MA080 | Spare Heat Cell | Immediate effect: Heat Cell (no part) | Utility | 1 Copper + 1 Carbon + 1 Circuit | 1 | Pay 4 Heat when activated. At the start of next turn, gain 6 Heat and 1 Copper. |

### Rare

| ID | Recipe | Output / effect label | Kind | Resources | Cooldown | Effect |
| --- | --- | --- | --- | --- | --- | --- |
| MA081 | Boiler Oath | Immediate effect: Boiler Seal (no part) | Utility | 2 Iron + 1 Copper + 1 Circuit | 2 | For this fight, the first two times each round you pay at least 4 Shield as a part activation cost, gain 2 Heat after paying. Shield lost to attacks does not count. Reusing Boiler Seal does not add triggers. |
| MA082 | White Casting | 1 White Slug | Ammo | 2 Iron + 2 Carbon + 1 Circuit | 2 | Add 12 damage. After impact, apply Burn equal to your Heat at Fire to the main target, up to Burn 10. |
| MA083 | Furnace Door | 1 Flood Door | Shield | 3 Iron + 1 Copper + 1 Carbon | 2 | Gain 18 Shield. You may then pay all your current Heat to gain 2 more Shield per Heat paid. Choosing this payment with 0 Heat gives no extra Shield. |
| MA084 | Scarred Mould | 1 Scarred Slug | Ammo | 2 Iron + 1 Carbon + 1 Circuit | 2 | Add 10 damage, plus 2 for each HP you have paid as recipe-part activation costs this fight, up to 18 extra damage. Enemy damage does not count. |
| MA085 | Open Smelter | Immediate effect: Smelter Spray Head (no part) | Utility | 2 Copper + 2 Carbon + 1 Glass | 2 | Pay 6 Heat when activated. Apply Burn 5 and Weaken 4 to every living enemy. |
| MA086 | Officer's Brace | 1 Officer Brace | Shield | 2 Iron + 1 Copper + 1 Circuit | 2 | Gain 12 Shield and choose one enemy currently showing an attack. Reduce that enemy's next attack's total damage this round by 50%, rounded down after flat reductions. Only one Officer Brace can affect the same attack. |
| MA087 | Level Pressure | Immediate effect: Pressure Equalizer (no part) | Utility | 1 Iron + 2 Copper + 1 Circuit | 2 | Choose one enemy and pay all your Heat, at least 1. Apply Weaken 2 and Mark 1 to it per Heat paid, up to Weaken 16 and Mark 8. |
| MA088 | Full Flush | Immediate effect: Flush Manifold (no part) | Utility | 2 Copper + 1 Glass + 1 Circuit | 2 | Pay 5 Heat when activated. Remove all cooldown counters from every currently cooling recipe. This does not reset a recipe without cooldown. |
| MA089 | Offcut Press | Immediate effect: Offcut Press (no part) | Utility | 2 Iron + 1 Copper + 1 Circuit | 2 | When activated, consume one unused Shield part from reserve without its effect. At the start of next turn, gain 3 Iron and 1 Glass. Cannot activate without a Shield part to consume. |
| MA090 | Burning Shelter | Immediate effect: Fire Shelter Frame (no part) | Utility | 2 Iron + 1 Carbon + 1 Circuit | 2 | For this fight, after an enemy takes a Burn tick, gain 2 Shield, up to 6 Shield per enemy phase. This Shield can cover later attacks in that phase and clears normally next turn. Reusing the frame does not add triggers. |
| MA091 | Scattered Coals | 1 Coal Scatter Tip | Ammo | 2 Iron + 2 Carbon + 1 Glass | 2 | Add 16 damage. After other pre-hit effects, record the main target's Burn before main damage. If that damage kills it, apply the recorded amount, up to Burn 8, to one other enemy chosen at Fire, if still alive. |
| MA092 | Pressure Debt | Immediate effect: Overpressure Pump (no part) | Utility | 1 Iron + 2 Carbon + 1 Circuit | 2 | Lose 5 HP when activated, leaving at least 1 HP, then gain 10 Heat. At the start of next turn, gain 8 Shield after old Shield clears. Shield cannot pay the HP cost. |
| MA093 | Furnace Ledger | Immediate effect: Fuel Meter (no part) | Utility | 1 Iron + 1 Copper + 1 Carbon + 1 Circuit | 3 | For this fight, each round's first two part activations that pay at least 3 Heat each grant 1 Iron at the start of the following turn. Reusing Fuel Meter does not add triggers. |
| MA094 | Furnace Bore | 1 Penetrating Bore | Modifier | 2 Iron + 1 Copper + 2 Circuit | 3 | Pay 6 Heat when activated. This round's main-shot damage to its main target bypasses Shield. Damage to other targets and separate support hits still meet Shield normally. Extra copies do not repeat the damage. |
| MA095 | Cold Counter | Immediate effect: Cold Sight (no part) | Utility | 1 Copper + 2 Glass + 1 Circuit | 2 | Can activate only at 0 Heat. Apply Weaken 12 and Mark 8 to one chosen enemy. |
| MA096 | Heated Parapet | 1 Parapet Plate | Shield | 3 Iron + 1 Copper | 2 | When activated, choose and pay 3 to 8 Heat. Gain 6 Shield plus 3 Shield per Heat paid. |
| MA097 | Iron Rain | 1 Rain Cap | Ammo | 3 Iron + 1 Glass + 1 Circuit | 2 | Add 10 damage. After impact, deal a separate 8 damage to every enemy except the main target. These hits do not carry this shot's payload effects. |
| MA098 | Stored Weight | 1 Counterweight | Modifier | 2 Iron + 1 Copper + 1 Circuit | 2 | At Fire, add 3 damage for each unused crafted part still in reserve, up to 18 damage. Parts already loaded in the shot are not in reserve. The reserve parts remain unused. |
| MA099 | Settled Slag | 1 Slag Foundation + conditional next-round Shield part | Shield | 3 Iron + 1 Carbon + 1 Circuit | 2 | When this part activates at End Turn, gain 22 Shield. At the beginning of the next round, if Heat is 0 before that round's start-of-turn Heat gains, automatically load a new regular 14-Shield part. |
| MA100 | Red Warning | Immediate effect: Warning Flare (no part) | Utility | 1 Copper + 2 Carbon + 1 Circuit | 2 | Choose an enemy showing an attack intent. Until the end of this enemy phase, each time that enemy loses HP to a main shot or support hit, apply Weaken 2 to it, up to Weaken 8. Burn and Corrosion ticks do not trigger this. |
| MA101 | Emergency Forge Rivets | Immediate effect: Rivet Dispenser (no part) | Utility | 2 Iron + 1 Copper + 1 Circuit | 2 | For the rest of this round, after each part activation that costs HP, gain 5 Shield, up to 15 Shield. Damage from enemies does not trigger it. Reusing Rivet Dispenser refreshes the effect without resetting its Shield limit. |
| MA102 | Sealed Kiln | 1 Kiln Seal | Shield | 2 Iron + 2 Carbon + 1 Circuit | 3 | Gain 16 Shield and remember your current Heat, up to 8. After this enemy phase's normal Heat loss, restore Heat to the remembered amount if it is lower. Extra Kiln Seals keep only the highest remembered amount. |
| MA103 | Furnace Hammer | 1 Hammer Slug | Ammo | 3 Iron + 1 Carbon + 1 Circuit | 2 | Add 14 damage. If the main target still has Shield after main damage, deal a separate 14 damage to it after impact. This second hit does not carry shot payload effects. |
| MA104 | Close the Breach | 1 Breach Plate | Shield | 3 Iron + 1 Copper + 1 Circuit | 2 | Gain 12 Shield. If enemy attacks caused you to lose HP during the previous enemy phase, gain 14 more. This condition is false in the first round. |
| MA105 | Fire Exchange | Immediate effect: Fire Exchange Tube (no part) | Utility | 1 Copper + 2 Carbon + 1 Circuit | 2 | Choose two different living enemies. Remove up to 8 Burn from the first and apply twice the removed amount as Burn to the second. Cannot activate without two enemies and at least 1 Burn to move. |
| MA106 | Recovered Force | 1 Recoil Store | Modifier | 2 Iron + 1 Copper + 1 Circuit | 2 | Add 8 damage to this round's main shot. If it kills its main target, add 16 damage to next round's main shot. Extra Recoil Stores in this round do not stack the next-round bonus. |
| MA107 | Warm Repair Bench | Immediate effect: Repair Bench Clamp (no part) | Utility | 2 Iron + 1 Copper + 1 Circuit | 2 | **Superseded Utility dependency; revise before use.** Pay 4 Heat when activated. At the start of next turn, gain 2 Ready Plates. Each is a Shield part that gives 8 Shield when activated; they may be saved for later turns. |
| MA108 | Counterfire Door | 1 Counterfire Door | Shield | 3 Iron + 1 Carbon + 1 Glass | 2 | Gain 18 Shield. After the first enemy attack this round that is fully absorbed by your Shield, deal 10 damage and apply Burn 3 to that attacker. Only one attack triggers each Counterfire Door. |

### Legendary

| ID | Recipe | Output / effect label | Kind | Resources | Cooldown | Effect |
| --- | --- | --- | --- | --- | --- | --- |
| MA109 | Citybreaker | 1 Citybreaker Slug | Ammo | 3 Iron + 1 Carbon + 1 Glass + 1 Circuit | 3 | Add 24 damage. Before main damage, remove up to 20 Shield from the main target and add half the amount removed, rounded down, to this shot's damage. |
| MA110 | Red Furnace | Immediate effect: Red Furnace Liner (no part) | Utility | 2 Iron + 2 Carbon + 1 Circuit | 3 | For this fight, your main shot applies Burn 3 to its main target after impact. If Heat was 8 or more at Fire, apply Burn 6 instead. This triggers once per main shot; reusing the liner does not stack it. |
| MA111 | Foundry Split | 1 Foundry Splitter | Modifier | 2 Iron + 1 Copper + 1 Glass + 1 Circuit | 3 | Pay 7 Heat when activated. Every extra enemy receives 60% of this round's shot damage before target-specific Mark and defence, then applies its own protection. Gain 2 Heat per enemy killed by those spread hits, up to 6 Heat. Main-target payloads are not copied. Extra Foundry Splitters do not repeat this spread. |
| MA112 | Walking Foundry | Immediate effect: Foundry Harness (no part) | Utility | 3 Iron + 1 Copper + 1 Circuit | 3 | For this fight, at Fire add half your current Shield, rounded down, to the main shot's damage, up to 20 extra damage. Shield is not spent. This is counted once per main shot; reusing the harness does not stack it. |
| MA113 | Winter Furnace | 1 Winter Core Sleeve | Modifier | 2 Iron + 2 Copper + 1 Circuit | 3 | Can activate only with at least 8 Heat. Pay all your Heat. Add 30 damage to this round's main shot and gain 20 Shield. |
| MA114 | Closed Loop Kiln | Immediate effect: Closed Kiln Manifold (no part) | Utility | 2 Iron + 2 Copper + 1 Carbon + 1 Circuit | 4 | Pay 6 Heat when activated. Remove up to 2 cooldown counters from every currently cooling recipe. No resources or parts are refunded. |
| MA115 | Iron Refuge | 1 Refuge Wall | Shield | 4 Iron + 1 Copper + 1 Circuit | 3 | Gain 24 Shield. After each enemy attack this round finishes, restore the Shield that attack removed, up to 24 Shield restored in total this round. Extra Refuge Walls add their initial Shield but share this restoration limit. |
| MA116 | Last Reserve | Immediate effect: Last Reserve Pump (no part) | Utility | 1 Iron + 2 Carbon + 1 Copper + 1 Circuit | 3 | Lose 6 HP when activated, leaving at least 1 HP. Gain 6 Heat and 24 Shield, and add 18 damage to your next main shot this round only. Shield cannot pay the HP cost. |
| MA117 | Ash Crown | Immediate effect: Ash Crown Nozzle (no part) | Utility | 1 Iron + 2 Carbon + 1 Glass + 1 Circuit | 3 | Choose one enemy with Burn. Apply extra Burn equal to its current Burn, up to Burn 12, then apply Burn 4 to every other enemy. Cannot activate without a burning enemy. |
| MA118 | Hammer and Anvil | 1 Anvil Coupling | Modifier | 3 Iron + 1 Copper + 1 Carbon + 1 Circuit | 3 | Pay 12 Shield when activated. Add 28 damage to this round's main shot. At the start of next turn, gain 24 Shield after old Shield clears. |
| MA119 | Spare Foundry | Immediate effect: Foldout Foundry (no part) | Utility | 3 Iron + 1 Copper + 1 Carbon + 1 Circuit | 4 | **Superseded Utility dependency; revise before use.** At the start of next turn, gain 2 Siege Slugs and 2 Siege Plates. Each Siege Slug is Ammo that adds 10 damage; each Siege Plate is a Shield part that gives 10 Shield. Parts may be saved or used together and still clear at fight end. |
| MA120 | Furnace Echo | 1 Echo Chamber | Ammo | 2 Iron + 1 Copper + 1 Carbon + 1 Glass + 1 Circuit | 3 | Add 18 damage. After the enemy phase, deal a separate hit to this shot's main target if alive: half the original shot's calculated damage before target-specific Mark and defence, rounded down, up to 30. The delayed hit meets the target's current Shield, copies no payloads and cannot create another echo. |

## Ivo Rook Vale

**Weapon:** Needle Cannon.


**Proposed identity:** a patient robot hunter who weakens a chosen target, reads the enemy's plan and keeps useful parts for the right round. His gun rewards careful targeting rather than firing extra shots.

**Proposed inherent ability — Find the Seam:** once each player turn, Ivo's first successful Acid Etch application also applies Target Paint 2 to one affected living enemy. These are the proposed aliases for Corrosion and Mark. If several robots receive that application, choose one recipient through the action's targeting. A blocked Acid Etch application does not qualify; existing status ticks do not qualify. The Paint follows the application and helps a later main shot, not the hit that already resolved. See the [complete targeting and trigger rules](MERCENARY-ABILITIES.md#ivo--find-the-seam). Ivo has no additional meter.

For these recipes, a **saved part** entered reserve in an earlier round. Parts supplied at fight start count as entering reserve in round 1. Crafting and receiving parts record their round; storing them does not change their effects. A part spent from reserve is consumed without activating its normal effect. Unless a row says otherwise, check intent and other conditions when the part activates; check Ammo conditions when firing. The shared rules define Corrosion, Mark, Weaken, Shield and the order of Ammo effects.

Three possible builds, all proposals:

- **Eat Through the Armour:** use Acid Mix (IV001), Deep Etch (IV051), Stored Acid (IV084) and Spreading Rust (IV110) to place Corrosion early, protect yourself and make later rounds harder for the robots.
- **One Chosen Target:** use Sight Paint (IV003), Reserve Sight (IV037), Weak Joint (IV042) and Perfect Line (IV117) to prepare a marked target and spend a saved payload on a large main shot.
- **Read and Wait:** use Brace Pad (IV002), Feint Plate (IV019), Held Needle (IV013) and Quiet Workshop (IV097) to defend against declared attacks and carry useful parts into later rounds.

Three future permanent-upgrade directions, not an upgrade catalogue: increase the trait's Mark amount; grant a small starting stock of finished parts; improve protection when several enemies show attack intent. Exact values, sources and stacking remain for the upgrade pass.

### Base

| ID | Recipe | Output / effect label | Kind | Resources | Cooldown | Effect |
| --- | --- | --- | --- | --- | --- | --- |
| IV001 | Acid Mix | 1 Needle Acid Capsule | Ammo | 1 Iron + 1 Carbon | None | Add 3 damage. After main damage, apply Corrosion 2 to the main target. |
| IV002 | Brace Pad | 1 Angled Pad | Shield | 1 Iron + 1 Copper | None | Gain 6 Shield. Gain 2 more if any enemy currently shows attack intent. |
| IV003 | Sight Paint | Immediate effect: Target Paint Can (no part) | Utility | 1 Copper | None | Apply Mark 4 to one enemy now. |
| IV004 | Patient Loading | 1 Holding Sleeve | Modifier | 1 Glass | None | This turn's shot gains 4 damage if it consumes at least one saved Ammo part. |

### Common

| ID | Recipe | Output / effect label | Kind | Resources | Cooldown | Effect |
| --- | --- | --- | --- | --- | --- | --- |
| IV005 | Open Frame | 1 Frame Needle | Ammo | 2 Iron | None | Add 8 damage. After main damage, apply Mark 4 to the main target if it showed attack intent at Fire. |
| IV006 | Painted Acid | 1 Guided Acid Tip | Ammo | 1 Iron + 1 Carbon | None | Add 6 damage. After main damage, apply Corrosion 2 if the main target had Mark at Fire, before any Mark was spent. |
| IV007 | Tracer Mix | 1 Bright Tracer | Ammo | 1 Iron + 1 Copper | None | Add 4 damage. After main damage, apply Mark 5 to the main target. |
| IV008 | Under the Plate | 1 Seeping Tip | Ammo | 1 Carbon + 1 Glass | None | Add 5 damage. After main damage, apply Corrosion 3 if the main target had any Shield immediately before main damage. |
| IV009 | Plate Needle | 1 Plate Splitter | Ammo | 2 Iron + 1 Glass | None | Add 7 damage. Before main damage, remove up to 4 Shield from the main target. If this part removed any Shield, apply Corrosion 2 to that target after main damage. |
| IV010 | Twin Leak | 1 Forked Acid Capsule | Ammo | 2 Carbon + 1 Copper | 1 | After main damage, apply Corrosion 2 to the main target and one other enemy you choose. If no other enemy is alive, apply only the main target's Corrosion. |
| IV011 | Guarded Leak | 1 Covered Acid Tip | Ammo | 1 Iron + 1 Carbon + 1 Copper | None | Add 5 damage. After main damage, apply Corrosion 2 to the main target. Gain 3 Shield after firing. |
| IV012 | Side Needle | 1 Splinter Tip | Ammo | 2 Iron + 1 Glass | None | Add 7 damage. Choose one other enemy at Fire. After main damage, apply Mark 4 to it; if this part was saved, also deal a separate 4 damage to it. A missing or dead second target receives no effect. |
| IV013 | Held Needle | 1 Sealed Needle | Ammo | 2 Iron | None | Add 9 damage. If this part was saved, also apply Mark 3 to the main target after main damage. |
| IV014 | Full Pouch | 1 Stock Counterweight | Ammo | 1 Iron + 1 Copper | None | Add 6 damage. Add 4 more if at least three unused parts remain in reserve when you fire. |
| IV015 | Read the Guard | 1 Changeable Tip | Ammo | 1 Iron + 1 Carbon + 1 Glass | None | Add 4 damage. After main damage, apply Weaken 4 if the main target shows attack intent; otherwise apply Corrosion 3. |
| IV016 | Follow the Line | 1 Sight Needle | Ammo | 2 Iron + 1 Copper | None | Add 7 damage. After main damage, apply Weaken 3 if the main target had Mark at Fire, before any Mark was spent. |
| IV017 | Rust Guard | 1 Rustproof Plate | Shield | 1 Iron + 1 Copper | None | Gain 8 Shield. Gain 3 more if any enemy currently has Corrosion. |
| IV018 | Aim Breaker | 1 Glare Plate | Shield | 1 Iron + 1 Glass | None | Gain 6 Shield. If any enemy currently shows attack intent, apply Weaken 3 to one such enemy. With no eligible enemy, gain only the Shield. |
| IV019 | Feint Plate | 1 Folded Plate + next-round Shield part | Shield | 2 Iron + 1 Copper | None | When this part activates at End Turn, gain 7 Shield and schedule a new regular 3-Shield part to load at the beginning of the next round. If this source part was saved, also apply Weaken 3 to one enemy that currently has Mark; with no eligible enemy, gain only the Shield and scheduled delivery. |
| IV020 | Crossfire Cover | 1 Wide Cover Plate | Shield | 2 Iron + 1 Copper | None | Gain 9 Shield, plus 2 for each enemy that currently has Mark and shows attack intent, up to 6 extra Shield. |
| IV021 | Two Stage Cover | 1 Layered Pad | Shield | 1 Iron + 2 Copper | 1 | Gain 5 Shield now. Choose an enemy that currently has Corrosion, if any. The next time its Corrosion removes HP this round, gain 6 Shield, even if that tick kills it. This triggers once and expires at round end. |
| IV022 | Marked Cover | 1 Sight Shield | Shield | 1 Iron + 1 Copper + 1 Glass | None | Gain 8 Shield. Apply Mark 2 to one enemy now. |
| IV023 | Acid Lining | 1 Acid Guard Plate | Shield | 1 Iron + 1 Carbon + 1 Glass | None | Gain 7 Shield. After the first enemy attack this round, apply Corrosion 2 to its attacker. This triggers once even if the attack hits several times. |
| IV024 | Saved Brace | 1 Locked Brace | Shield | 2 Iron | None | Gain 7 Shield, or 12 Shield if this part was saved. |
| IV025 | Quiet Cover | 1 Low Cover Plate | Shield | 1 Iron + 1 Copper | None | Gain 7 Shield. If no enemy currently shows attack intent, also apply Mark 3 to one enemy now. |
| IV026 | Spare Padding | 1 Pouch Pad | Shield | 1 Iron + 1 Copper | None | Gain 6 Shield plus 1 for each saved part currently in reserve, up to 4 extra Shield. |
| IV027 | Acid Spray | Immediate effect: Hand Sprayer (no part) | Utility | 1 Carbon + 1 Glass | None | Apply Corrosion 2 to one enemy now. |
| IV028 | Joint Paint | Immediate effect: Fine Paint Can (no part) | Utility | 1 Copper + 1 Glass | None | Apply Mark 6 to one enemy that currently has Corrosion, or Mark 3 to any other enemy. |
| IV029 | Warning Flare | Immediate effect: Red Flare (no part) | Utility | 1 Copper + 1 Carbon | None | Apply Mark 3 and Weaken 2 to one enemy that currently shows attack intent. Cannot activate without an eligible enemy. |
| IV030 | Broad Spray | Immediate effect: Wide Paint Can (no part) | Utility | 2 Copper + 1 Glass | 1 | Apply Mark 2 to every living enemy now. |
| IV031 | Pocket Salvage | Immediate effect: Salvage Tag (no part) | Utility | 1 Copper | None | Consume one saved part from reserve. At the start of your next turn, gain 2 Iron and 1 Carbon. The consumed part gives no normal effect. |
| IV032 | Set Aside | Immediate effect: Supply Envelope (no part) | Utility | 1 Iron + 1 Copper | None | At the start of your next turn, gain 2 Glass. |
| IV033 | Acid Trap | Immediate effect: Contact Patch (no part) | Utility | 1 Carbon + 1 Glass | None | Choose one enemy that currently shows attack intent. After its next attack this round, apply Corrosion 3 to it. If it does not attack this round, the patch expires. |
| IV034 | Broken Aim | Immediate effect: Lens Flash (no part) | Utility | 1 Copper + 1 Glass | None | Apply Weaken 5 to one enemy with Mark, or Weaken 2 to an enemy without Mark. |
| IV035 | Delayed Paint | Immediate effect: Timed Paint Can (no part) | Utility | 1 Copper | None | Choose one enemy. At the start of your next turn, apply Mark 6 to it if it is still alive. |
| IV036 | Read the Ranks | Immediate effect: Range Note (no part) | Utility | 1 Glass + 1 Copper | None | Apply Mark 4 to one enemy. If some living enemies currently show attack intent and some do not, also gain 4 Shield. |
| IV037 | Reserve Sight | 1 Spare Sight | Modifier | 1 Glass + 1 Copper | None | This turn's main shot gains 6 damage against a target with Mark if at least one saved part remains in reserve when you fire. |
| IV038 | Safe Loading | 1 Loading Guard | Modifier | 1 Iron + 1 Copper | None | Gain 4 Shield now. This turn's shot gains 4 damage if the main target currently shows attack intent when you fire. |
| IV039 | Acid Sight | 1 Etched Sight | Modifier | 1 Carbon + 1 Glass | None | This turn's shot gains 2 damage for each Corrosion on its main target when you fire, up to 8 extra damage. |
| IV040 | Lasting Shell | 1 Dry Storage Sleeve | Modifier | 1 Iron + 1 Glass | None | This turn's shot gains 3 damage for each saved Ammo part it consumes, up to 9 extra damage. |

### Uncommon

| ID | Recipe | Output / effect label | Kind | Resources | Cooldown | Effect |
| --- | --- | --- | --- | --- | --- | --- |
| IV041 | Acid Booster | 1 Booster Capsule | Ammo | 2 Carbon + 1 Glass | 1 | Add 6 damage. After main damage, apply Corrosion 3, then apply 2 more if the main target now has at least 6 Corrosion. |
| IV042 | Weak Joint | 1 Joint Piercer | Ammo | 2 Iron + 1 Copper + 1 Glass | None | Add 10 damage. Add 8 more if the main target had at least 8 Mark at Fire, before any Mark was spent. |
| IV043 | Rust Splinters | 1 Rust Burst Tip | Ammo | 2 Iron + 1 Carbon + 1 Glass | 1 | Add 7 damage. After main damage, deal a separate 5 damage to each other enemy that currently has Corrosion. |
| IV044 | Clean Finish | 1 Finishing Needle | Ammo | 2 Iron + 1 Glass | None | Add 9 damage. Add 7 more if the main target is at or below half its maximum HP immediately before main damage. |
| IV045 | Deep Capsule | 1 Thick Acid Capsule | Ammo | 1 Iron + 2 Carbon + 1 Glass | 1 | Before main damage, choose and remove up to 4 Mark from the main target. Add 4 damage. After main damage, apply Corrosion 3 plus 1 for each Mark this part removed. Removed Mark does not also give ordinary Mark damage. |
| IV046 | Lift the Plate | 1 Plate Hook | Ammo | 2 Iron + 1 Copper | 1 | Before main damage, remove up to 6 Shield from the main target. Add 7 damage plus 1 for each Shield this part removed. |
| IV047 | Settled Powder | 1 Settling Needle | Ammo | 1 Iron + 1 Carbon + 1 Glass | None | Add 8 damage. After main damage, apply Corrosion 1 for each round since this part entered reserve, up to Corrosion 4. A part received this round applies none; one received in round 2 applies Corrosion 2 when fired in round 4. |
| IV048 | Spare Plate Shot | 1 Plate Holder | Ammo | 1 Iron + 1 Copper | 1 | Before main damage, consume one saved Shield part from reserve without its normal effect. Add 16 damage. Cannot load this part unless you have an eligible part to spend; reserve that part until firing. |
| IV049 | Exposed Wiring | 1 Wire Needle | Ammo | 1 Iron + 1 Copper + 1 Glass | None | Add 7 damage. After main damage, the main target loses 4 HP directly if it had Mark at Fire, before any Mark was spent. |
| IV050 | Returning Paint | 1 Paint Return Tip | Ammo | 1 Iron + 2 Copper | 1 | Add 8 damage. After main damage, restore half the Mark consumed by this main shot to its target, rounded down, up to Mark 6. |
| IV051 | Deep Etch | 1 Deep Acid Tip | Ammo | 1 Iron + 2 Carbon + 1 Circuit | 2 | Add 8 damage. After main damage, apply Corrosion 5 and Weaken 3 to the main target. |
| IV052 | Signal Splinter | 1 Signal Burst Tip | Ammo | 2 Iron + 1 Copper + 1 Circuit | 1 | Add 8 damage. After main damage, deal a separate 6 damage to each other enemy that currently has Mark. Those support hits do not consume Mark. |
| IV053 | Count the Barrels | 1 Barred Cover Plate | Shield | 2 Iron + 1 Copper | None | Gain 10 Shield. Apply Mark 2 to every enemy that currently shows attack intent. |
| IV054 | Acid Drain | 1 Drain Plate | Shield | 1 Iron + 1 Carbon + 1 Glass | None | Remove up to 3 Corrosion from one enemy now. Gain 5 Shield plus 4 for each Corrosion removed. |
| IV055 | Corroded Barricade | 1 Rust Barrier | Shield | 2 Iron + 1 Carbon | None | Gain 8 Shield plus 2 for each enemy that currently has Corrosion, up to 8 extra Shield. |
| IV056 | Narrow Guard | 1 Narrow Plate | Shield | 2 Iron + 1 Copper | None | Gain 8 Shield. If exactly one enemy currently shows attack intent, apply Weaken 6 to it. Otherwise, gain only the Shield. |
| IV057 | Stored Wall | 1 Packed Wall + next-round Shield part | Shield | 2 Iron + 1 Copper + 1 Glass | 1 | When this part activates at End Turn, gain 10 Shield, or 16 if this source part was saved. Schedule a new regular 4-Shield part to load at the beginning of the next round. |
| IV058 | Wrong Target | 1 False Sight Plate | Shield | 1 Iron + 2 Copper | 1 | Gain 9 Shield. Move all Mark, up to 8, from one enemy to a different enemy now. Mark beyond 8 remains on the first enemy. |
| IV059 | Late Cover | 1 Delayed Cover Pack | Shield | 2 Iron + 1 Copper | 1 | Gain 4 Shield now. If at least one saved part remains in reserve at the end of this enemy phase, gain 12 Shield at the start of next turn, after old Shield clears. |
| IV060 | Traced Retaliation | 1 Tracer Guard | Shield | 2 Iron + 1 Copper + 1 Glass | 1 | Gain 10 Shield. After each enemy's first attack this round, apply Mark 3 to that attacker. An enemy that attacks repeatedly still receives this effect only once. |
| IV061 | Hold the Gap | 1 Gap Plate + conditional next-round Shield part | Shield | 2 Iron + 1 Glass | None | When this part activates at End Turn, gain 11 Shield. If any enemy currently has at least 6 Mark at that activation, schedule a new regular 5-Shield part to load at the beginning of the next round. |
| IV062 | Needles in Cover | 1 Needle Guard | Shield | 2 Iron + 1 Carbon + 1 Glass | 1 | Gain 9 Shield. After the first enemy attack that removes any of your Shield this round, deal a separate 7 damage to that attacker. Triggers once. |
| IV063 | Acid Line | Immediate effect: Long Sprayer (no part) | Utility | 2 Carbon + 1 Glass | 1 | Apply Corrosion 3 to each of two different enemies now, or Corrosion 4 to one enemy if only one enemy is alive. |
| IV064 | Guard Study | Immediate effect: Defence Note (no part) | Utility | 1 Copper + 1 Glass | None | Apply Mark 7 to one enemy that currently does not show attack intent. If all living enemies show attack intent, apply Weaken 4 to one instead. |
| IV065 | Timed Etch | Immediate effect: Timer Sprayer (no part) | Utility | 2 Carbon + 1 Copper | 1 | Choose one enemy. Apply Corrosion 2 now and Corrosion 3 at your next turn's start if it is still alive. |
| IV066 | Strip and Save | Immediate effect: Recovery Tray (no part) | Utility | 1 Iron + 1 Copper | None | Consume one saved Ammo part from reserve without its normal effect. Gain 7 Shield now and 2 Carbon at your next turn's start. |
| IV067 | Coolant Needle | Immediate effect: Coolant Dropper (no part) | Utility | 1 Copper + 1 Glass + 1 Circuit | 2 | Remove 1 cooldown counter from every currently cooling recipe. Apply Mark 3 to one enemy now. |
| IV068 | Spare Plan | Immediate effect: Parts Ledger (no part) | Utility | 1 Copper + 1 Circuit | None | At your next turn's start, gain 1 Iron and 1 Glass if you still have at least two saved parts in reserve. |
| IV069 | Break the Signal | Immediate effect: Signal Jammer (no part) | Utility | 1 Copper + 1 Glass + 1 Circuit | 1 | Apply Weaken 3 to each enemy that currently has Mark. This does not remove their Mark. |
| IV070 | Wash Across | Immediate effect: Transfer Sprayer (no part) | Utility | 1 Carbon + 1 Glass + 1 Copper | 1 | Move up to 4 Corrosion from one enemy to another. Then apply Corrosion 2 to the receiving enemy. The first enemy loses exactly the amount moved. |
| IV071 | Pocket Screen | Immediate effect: Dust Screen (no part) | Utility | 1 Carbon + 1 Copper | None | On recipe Use, count the ordinary parts currently in reserve that were saved from an earlier round. Apply Weaken to every living enemy: 1 with 0 saved parts, 2 with exactly 1 saved part, or 3 with at least 2 saved parts. The counted parts are not consumed. This Utility activates immediately and produces no Utility part. |
| IV072 | Copper Scent | 1 Copper Lure | Magnet | 1 Copper + 1 Glass | 1 | Craft and fit for next round's single haul only: gain 2 additional Copper. This does not grant another haul or affect this round's haul. |
| IV073 | Fixed Sight | 1 Fixed Lens | Modifier | 1 Copper + 1 Glass + 1 Circuit | 1 | For this turn's main shot, increase the damage added by the target's consumed Mark by 50%, rounded down. This adds damage only; it does not apply or consume Mark twice. |
| IV074 | Stored Pressure | 1 Pressure Sleeve | Modifier | 1 Iron + 1 Carbon + 1 Glass | 1 | This turn's main shot gains 25% damage if it consumes at least two saved Ammo parts. It copies no part effects. |
| IV075 | Acid Reserve | 1 Acid Metering Sleeve | Modifier | 1 Carbon + 1 Glass | None | After this turn's main damage, apply Corrosion 1 to its target for each saved Ammo part consumed in the shot, up to Corrosion 4. |
| IV076 | All Eyes | 1 Wide Sight | Modifier | 1 Copper + 1 Glass + 1 Circuit | 1 | This turn's main shot also deals 30% of its calculated damage before target-specific Mark and defence to each other enemy that has Mark at Fire, rounded down. Extra targets receive no copied part effects and use their own Shield. Their Mark is neither consumed nor added as damage. |
| IV077 | Dead Angle | 1 Angled Lens | Modifier | 1 Iron + 1 Copper + 1 Glass | None | This turn's main shot gains 10 damage against an enemy that does not show attack intent when you fire. Against an attacking enemy, it instead gains 4 damage. |
| IV078 | Hold Back | 1 Reserve Lock | Modifier | 1 Iron + 1 Copper | None | This turn's shot gains 8 damage if it consumes exactly one Ammo part and at least two unused parts remain in reserve at firing. |
| IV079 | Covering Needle | 1 Cover Sleeve | Modifier | 1 Iron + 1 Glass + 1 Copper | None | After this turn's main shot, gain 2 Shield for each saved Ammo part it consumed, up to 10 Shield. |
| IV080 | Follow the Etch | 1 Etch Lens | Modifier | 1 Carbon + 1 Glass + 1 Circuit | 1 | If the main target has Corrosion when you fire, up to 10 damage from this turn's main shot bypasses its Shield and removes HP directly. The rest of the main damage uses Shield normally. |

### Rare

| ID | Recipe | Output / effect label | Kind | Resources | Cooldown | Effect |
| --- | --- | --- | --- | --- | --- | --- |
| IV081 | Exposure Drill | 1 Drilled Needle | Ammo | 2 Iron + 1 Glass + 1 Circuit | 2 | Before main damage, remove up to 12 Shield from the main target. Add 14 damage. After main damage, apply Mark 5 to it. |
| IV082 | Widen the Leak | 1 Acid Expander | Ammo | 2 Carbon + 1 Glass + 1 Circuit | 2 | Add 8 damage. After main damage, apply Corrosion equal to the target's current Corrosion, up to 8. A target with no Corrosion instead receives Corrosion 3. |
| IV083 | Cut the Pipe | 1 Pipe Needle | Ammo | 1 Iron + 2 Carbon + 1 Glass | 2 | Add 10 damage. After main damage, the main target loses HP equal to twice its current Corrosion, up to 12 HP. Then remove up to 2 Corrosion from it. |
| IV084 | Stored Acid | 1 Sealed Acid Charge | Ammo | 1 Iron + 2 Carbon + 1 Glass | 2 | Add 6 damage. After main damage, apply Corrosion 8 if this part was saved, otherwise Corrosion 4. |
| IV085 | Break the Barrel | 1 Barrel Needle | Ammo | 2 Iron + 1 Copper + 1 Circuit | 2 | Add 12 damage. After main damage, apply Weaken equal to the Mark consumed by this main shot, up to Weaken 8. |
| IV086 | Last Wire | 1 Wire Cutter Tip | Ammo | 2 Iron + 1 Carbon + 1 Circuit | 2 | Add 12 damage. After main damage, if the main target has Corrosion and 8 HP or less, it loses all its remaining HP directly. This checks once and does not affect another enemy. |
| IV087 | Clean Recovery | 1 Salvage Needle | Ammo | 2 Iron + 1 Copper + 1 Glass | 1 | Add 14 damage. If the main damage kills its target, gain 2 Carbon and 1 Glass at your next turn's start. No materials are awarded if the fight ends first. |
| IV088 | Paired Needles | 2 Split Acid Needles | Ammo | 2 Iron + 2 Carbon + 1 Glass | 2 | Each part adds 6 damage and applies Corrosion 2 to the main target after main damage. Use both in one shot or save either for a later round. Each is consumed separately. |
| IV089 | Mapped Cover | 1 Mapped Barrier | Shield | 2 Iron + 1 Copper + 1 Glass | 2 | Gain 18 Shield. After the first enemy attack this round, apply Mark 6 to its attacker. Triggers once. |
| IV090 | Stockroom Wall | 1 Stockroom Plate + conditional next-round Shield part | Shield | 2 Iron + 1 Copper + 1 Circuit | 2 | When this part activates at End Turn, gain 12 Shield. If at least three saved parts are currently in reserve at that activation, schedule a new regular 14-Shield part to load at the beginning of the next round. |
| IV091 | Rusted Crossfire | 1 Interference Barrier | Shield | 2 Iron + 1 Carbon + 1 Circuit | 2 | Gain 10 Shield. Apply Weaken 5 to each living enemy that currently has Corrosion. |
| IV092 | Untouched Cover | 1 Unbroken Plate | Shield | 2 Iron + 1 Copper + 1 Glass | 1 | Gain 16 Shield and choose one enemy now. After enemies finish their actions, before the Shield reset, if that enemy is still alive, apply Mark to it equal to your total remaining active Shield, up to Mark 8. |
| IV093 | Return the Leak | 1 Leak Guard | Shield | 1 Iron + 2 Carbon + 1 Circuit | 2 | Gain 14 Shield. After the first attack by an enemy with Corrosion this round, that attacker loses 6 HP directly. Triggers once; its Corrosion is not consumed. |
| IV094 | Reinforced Position | 1 Position Brace | Shield | 2 Iron + 1 Copper + 1 Circuit | 2 | Gain Shield equal to your current Shield, up to 16. Also apply Mark 4 to one enemy now. If you have no Shield, this grants only the Mark. |
| IV095 | Disrupting Paint | Immediate effect: Paint Disruptor (no part) | Utility | 1 Copper + 1 Glass + 2 Circuit | 3 | For this fight, the first time each round you apply Mark, also apply Weaken 3 to one recipient of that application. Using another Paint Disruptor refreshes this effect without stacking it. |
| IV096 | Rust Watch | Immediate effect: Rust Scanner (no part) | Utility | 1 Carbon + 1 Glass + 2 Circuit | 3 | For this fight, at your turn's start, apply Mark 3 to the living enemy with the most Corrosion, provided it has at least 1. Choose the recipient if tied. Identical scanners do not stack. |
| IV097 | Quiet Workshop | Immediate effect: Reserve Rack (no part) | Utility | 1 Iron + 1 Copper + 2 Circuit | 3 | For this fight, at your turn's start, gain 2 Shield for each saved part in reserve, up to 6 Shield. Additional Reserve Racks refresh this effect without increasing it. |
| IV098 | Sacrifice the Payload | Immediate effect: Emergency Sighting Kit (no part) | Utility | 1 Copper + 1 Glass + 1 Circuit | 1 | Consume one saved Ammo part from reserve without its normal effect. Apply Mark 10 to one enemy now and gain 6 Shield. Cannot activate without an eligible part. |
| IV099 | Twin Coolant | Immediate effect: Split Coolant Tube (no part) | Utility | 1 Copper + 2 Glass + 1 Circuit | 3 | **Superseded Utility dependency; revise before use.** Remove 1 cooldown counter from every currently cooling recipe. If this part was saved, also apply Corrosion 3 to one chosen enemy now. Cannot activate unless at least one recipe is cooling. |
| IV100 | Sorted Scrap | 1 Sorting Lure | Magnet | 1 Carbon + 1 Copper + 1 Circuit | 2 | Craft and fit for next round's single haul only: gain 1 additional Carbon for each saved part in reserve at that haul, up to 4 Carbon. If there are at least three saved parts, also gain 1 Circuit. |
| IV101 | Watch the Leak | Immediate effect: Leak Gauge (no part) | Utility | 1 Carbon + 1 Glass + 1 Circuit | 1 | Choose one enemy. The next time Corrosion removes HP from it this round, apply Mark equal to that HP loss, up to Mark 8. If it dies, no Mark transfers to another enemy. Expires at round end. |
| IV102 | Parts for Time | Immediate effect: Reclaimed Coolant Pack (no part) | Utility | 1 Copper + 1 Glass + 1 Circuit | 2 | Consume two saved parts from reserve without their normal effects. Remove 2 cooldown counters from every currently cooling recipe. Cannot activate without two eligible parts. |
| IV103 | Armoured Opening | 1 Armour Sight | Modifier | 1 Iron + 1 Glass + 2 Circuit | 2 | This turn's main shot gains 40% damage if its target has at least 10 Shield immediately before any parts in that shot remove Shield. Otherwise it gains 10%. |
| IV104 | Passing Leak | 1 Transfer Sleeve | Modifier | 1 Carbon + 1 Glass + 1 Circuit | 2 | If this turn's main damage kills its target, apply Corrosion to one other living enemy equal to the dead target's Corrosion immediately before main damage, up to Corrosion 8. |
| IV105 | Empty the Tools | 1 Tool Loader | Modifier | 1 Iron + 1 Copper + 1 Glass | 1 | **Superseded Utility dependency; revise before use.** Choose and reserve up to two saved Utility parts until firing. This turn's shot consumes them without their normal effects and gains 7 damage for each. They count as parts consumed by that shot. |
| IV106 | Keep the Line | 1 Tracking Lens | Modifier | 1 Copper + 1 Glass + 1 Circuit | 2 | After this turn's main damage, restore up to 8 of the Mark that its target lost to this main shot. This restores Mark only and does not repeat the shot. |
| IV107 | Needle Thread | 1 Threaded Sleeve | Modifier | 1 Iron + 1 Glass + 1 Circuit | 2 | After this turn's main damage, deal a separate 5 damage to the main target for each saved Shield part you activated during this planning phase, up to 15 separate damage in one support hit. |
| IV108 | Small Opening | 1 Tight Lens | Modifier | 1 Copper + 1 Glass + 2 Circuit | 2 | This turn's main shot gains 50% damage if it consumes exactly one Ammo part and the main target has both Mark and Corrosion when you fire, before any Mark is consumed. It gains no percentage boost otherwise. |

### Legendary

| ID | Recipe | Output / effect label | Kind | Resources | Cooldown | Effect |
| --- | --- | --- | --- | --- | --- | --- |
| IV109 | Seal the Leak | 1 Acid Lock Needle | Ammo | 1 Iron + 2 Carbon + 1 Glass + 1 Circuit | 3 | Add 12 damage. After main damage, apply Corrosion 4 to the main target. Its Corrosion does not fall after its next two Corrosion damage ticks. Another Acid Lock Needle refreshes this two-tick effect; it does not add extra protected ticks. |
| IV110 | Spreading Rust | Immediate effect: Rust Relay (no part) | Utility | 2 Carbon + 1 Copper + 1 Glass + 2 Circuit | 4 | For this fight, the first time each round Corrosion kills an enemy, apply Corrosion 3 to every other living enemy. Only Corrosion damage can trigger this; main shots and support hits cannot. Identical Rust Relays do not stack. |
| IV111 | One Red Target | Immediate effect: Paint Collector (no part) | Utility | 2 Copper + 1 Glass + 2 Circuit | 3 | Choose one enemy. Move up to 16 total Mark from other enemies onto it, choosing how much to move from each. Then apply Mark 6 to the chosen enemy. No Mark is copied; amounts moved are removed from their old targets. |
| IV112 | Tomorrow's Opening | Immediate effect: Timed Coolant Rack (no part) | Utility | 1 Copper + 2 Glass + 2 Circuit | 3 | At your next turn's start, remove all cooldown counters from every recipe currently cooling in memory and gain 12 Shield. Identical timed racks do not stack on the same turn. |
| IV113 | Last Safe Line | 1 Survival Plate | Shield | 3 Iron + 1 Copper + 2 Circuit | 4 | Gain 20 Shield. This enemy phase, the first enemy attack that would reduce your HP to zero instead leaves you at 1 HP after all that attack's hits, then this protection ends. Later attacks can kill you. Direct HP costs cannot trigger it. Additional Survival Plates do not add extra rescues in the same round. |
| IV114 | Recover the Ruin | Immediate effect: Rust Recovery Beacon (no part) | Utility | 1 Carbon + 1 Copper + 1 Glass + 2 Circuit | 3 | For this fight, the first time each round an enemy with Corrosion dies, schedule 2 Iron and 1 Carbon for your next turn's start. It may die to any damage. Identical beacons do not stack; scheduled resources expire if the fight ends. |
| IV115 | Through the Seam | 1 Seam Lens | Modifier | 2 Iron + 1 Copper + 1 Glass + 2 Circuit | 3 | This turn's main damage bypasses all of its target's Shield if that enemy has both Mark and Corrosion when you fire, before any Mark is consumed. Its Shield remains. Extra-target and support damage still use Shield normally. |
| IV116 | Empty the Pouch | 1 Emergency Barricade Frame | Shield | 2 Iron + 1 Copper + 1 Glass + 1 Circuit | 3 | Consume up to three saved Ammo parts from reserve without their normal effects. Gain 8 Shield plus 9 for each part consumed. Apply Weaken 2 to every living enemy for each part consumed, up to Weaken 6 each. |
| IV117 | Perfect Line | 1 Perfect Sight | Modifier | 1 Iron + 2 Copper + 1 Glass + 2 Circuit | 3 | Before this turn's main damage, remove up to 12 Mark from the main target. Add 4 damage for each removed if the shot consumes a saved Ammo part, or 2 damage each otherwise. Any remaining Mark then adds damage normally and is consumed. |
| IV118 | Spare Needle Bench | Immediate effect: Needle Bench Kit (no part) | Utility | 2 Iron + 1 Copper + 1 Glass + 2 Circuit | 4 | **Superseded Utility dependency; revise before use.** For this fight, when a main shot consumes at least two saved Ammo parts, receive 1 Plain Needle at your next turn's start. A Plain Needle is Ammo that adds 6 damage with no other effect. Trigger once per round; identical benches do not stack. |
| IV119 | Stop the Volley | 1 Volley Breaker Tip | Ammo | 2 Iron + 1 Copper + 1 Glass + 2 Circuit | 3 | Add 16 damage. After main damage, apply Weaken 10 to the main target and Weaken 4 to every other enemy that currently shows attack intent. If the main target survives with Corrosion, also apply Mark 6 to it. |
| IV120 | Patient Hunter | Immediate effect: Hunter's Ledger (no part) | Utility | 1 Iron + 1 Copper + 1 Glass + 2 Circuit | 4 | For this fight, at your turn's start, if at least four saved parts remain in reserve, gain 5 Shield and apply Corrosion 2 and Mark 4 to one enemy chosen at that turn's start. Without four saved parts, gain none of these effects. Identical ledgers do not stack. |

## Ada Patch Flint

**Weapon:** Rivet Cannon and Bolt.


**Proposed identity:** a field mechanic who turns a battered helper into protection, extra support hits and a stronger main shot. Ada decides when Bolt should take a hit, when to repair him and when to spend his health on a risky command.

**Proposed character feature:** Bolt is one helper robot. He starts each fight at **6 of 12 HP**, takes no actions on his own and produces no free materials. At 0 HP he is disabled. A **repair** restores HP only while he is active; a **restore** can bring him back from 0 HP. Neither can exceed his current maximum. All Helper parts require active Bolt unless their text explicitly allows a disabled Bolt. Their commands resolve during planning unless another time is stated. A command is a consumed crafted part, never an extra main shot. Bolt's damage is ordinary support damage and cannot trigger main-shot effects. Enemies normally attack Ada. Bolt's HP and any changes to his maximum reset each fight. His maximum HP cannot exceed 24 through these recipes. Added maximum HP does not heal him unless a row says so.

**Proposed inherent ability — Field Service:** once each player turn, after a crafted Helper command deals direct damage that removes an enemy's Shield or HP, repair active Bolt for 2 HP. Resolve after the command and immediate enemy reactions while Ada is alive; Bolt must still be active. The opportunity is spent even at full Bolt HP. It cannot restore disabled Bolt, grant a command or enable interception by itself. See the [complete trigger rules](MERCENARY-ABILITIES.md#ada--field-service).

**Interception order:** only active Bolt can intercept, and a part must enable it. First apply Weaken to the enemy attack. Next, use eligible interception commands in the order they were activated, assigning each a different portion of the incoming damage. An allowance counts damage assigned to it, even if prevention later cancels some of that damage. Identical interception commands refresh their remaining interception effect instead of adding another; different commands may share one attack without assigning the same damage twice. Apply Bolt's damage prevention next: prevented damage is cancelled, not passed back to Ada. Bolt loses HP for the remaining assigned damage, up to the HP he can lose. Any protection that keeps his last HP is applied before calculating what he cannot take. Unassigned damage and any amount Bolt cannot take go to Ada's Shield, then her HP.

**Small example:** Bolt has 2 HP, Catch Relay can intercept 7 damage, and Pad Bundle prevents 4. Against a 12-damage attack, assign 7 to Bolt and cancel 4. Bolt loses his 2 HP; the remaining 1 spills over. Ada receives that 1 plus the 5 never assigned to Bolt: 6 damage before her Shield.

**Three possible builds:**

- **Hold the line:** Steady Brace (AD003), Shock Apron (AD047), Guard Rotation (AD056) and Steel Friendship (AD109) turn repair and careful interception into a reliable defence.
- **Keep him running:** Workshop Slug (AD025), Repair Counter (AD057), Borrowed Time (AD078) and Last Good Part (AD113) reward taking Bolt close to failure and bringing him back at the right moment.
- **Prepare the crew:** Spare Magazine (AD032), Scheduled Cover (AD066), Work Order (AD083) and Night Shift (AD117) prepare parts and protection for a later round while assembling one main shot.

**Future permanent-upgrade directions, for the later upgrade pass:** a stronger starting frame for Bolt; one improved support command each round; a small repair supply delivered on a specified turn. These are hooks, not extra approved rules or an upgrade catalogue.

### Base

| ID | Recipe | Output / effect label | Kind | Resources | Cooldown | Effect |
| --- | --- | --- | --- | --- | --- | --- |
| AD001 | Emergency Patch | Immediate effect: Restart Patch (no part) | Utility | 1 Iron + 1 Copper | None | Restore 4 Bolt HP. Works while Bolt is disabled. |
| AD002 | Tap Command | 1 Tap Relay | Helper | 1 Iron | None | Bolt deals 5 damage to one chosen enemy now. |
| AD003 | Steady Brace | 1 Brace Relay | Helper | 1 Iron + 1 Copper | None | This enemy phase, Bolt intercepts the next 5 damage that would reach you. The allowance expires after that phase. |
| AD004 | Partner's Load | 1 Rivet Collar | Ammo | 1 Iron + 1 Carbon | None | Add 5 damage. Add 4 more if Bolt is active when you fire. |

### Common

| ID | Recipe | Output / effect label | Kind | Resources | Cooldown | Effect |
| --- | --- | --- | --- | --- | --- | --- |
| AD005 | Fresh Bearings | Immediate effect: Bearing Pack (no part) | Utility | 1 Iron + 1 Copper | None | Repair 7 Bolt HP. Cannot restore a disabled Bolt. |
| AD006 | Slow Weld | Immediate effect: Timed Weld Pack (no part) | Utility | 1 Iron + 1 Carbon | 1 | At the start of your next turn, repair 10 Bolt HP if he is active then. Cannot restore him. |
| AD007 | Shared Plate | 1 Split Guard | Shield | 2 Iron + 1 Copper | None | Gain 6 Shield and repair 3 Bolt HP. You gain the Shield even if Bolt is disabled. |
| AD008 | Loose Rivets | 1 Rattle Slug | Ammo | 2 Iron + 1 Carbon | None | Add 8 damage. Add 4 more if Bolt has 1–5 HP when you fire. |
| AD009 | Knee Tap | 1 Low Strike Relay | Helper | 1 Iron + 1 Copper | None | Bolt deals 5 damage to one enemy now and applies Weaken 2 to it. |
| AD010 | Chalk Target | 1 Chalk Relay | Helper | 1 Glass + 1 Copper | None | Apply Mark 6 to one chosen enemy now. |
| AD011 | Catch the Small One | 1 Catch Relay | Helper | 2 Iron | None | Choose an enemy. Bolt intercepts the first 7 damage of that enemy's next attack this enemy phase. |
| AD012 | Stay Behind Me | 1 Cover Panel | Shield | 2 Iron + 1 Carbon | None | Gain 8 Shield. If Bolt has 1–4 HP now, gain 4 more Shield. |
| AD013 | Heavy Hand | 1 Hammer Relay | Helper | 2 Iron + 1 Carbon | None | Bolt loses 2 HP, then deals 11 damage to one enemy now. He must have at least 3 HP to use this command. |
| AD014 | Brush Sparks | 1 Spark Brush | Helper | 1 Copper + 1 Carbon | None | Apply Burn 3 to one chosen enemy now. |
| AD015 | Clean the Joint | 1 Joint Scraper | Helper | 1 Iron + 1 Glass | None | Remove 7 Shield from one enemy now. If it had no Shield, apply Mark 4 instead. |
| AD016 | Repair Brace | 1 Welded Brace | Helper | 2 Iron + 1 Copper | 1 | Repair 4 Bolt HP. This enemy phase, Bolt intercepts the next 4 damage that would reach you. |
| AD017 | Spare Bolts | 2 Small Rivets | Ammo | 2 Iron | None | Each Small Rivet adds 4 damage when used in a shot. You may save either part for a later turn. |
| AD018 | Tucked Wire | 1 Wire Sleeve | Ammo | 1 Iron + 1 Copper | None | Add 6 damage. After impact, repair 3 Bolt HP if he is active. |
| AD019 | Guard Marker | 1 Warning Flag | Helper | 1 Copper + 1 Glass | None | Choose an enemy showing an attack intent. Apply Weaken 5 to it now. Cannot target a non-attacking enemy. |
| AD020 | Brace for Two | 1 Twin Brace | Helper | 2 Iron + 1 Copper | 1 | This enemy phase, Bolt intercepts up to 3 damage from each of the next two enemy attacks that would reach you. |
| AD021 | Little Broadside | 1 Sweep Relay | Helper | 2 Iron + 1 Carbon | 1 | Bolt deals 4 damage to every enemy now. |
| AD022 | Work Light | 1 Work Lamp | Modifier | 1 Copper + 1 Glass | None | This shot gains 8 damage if its main target already has Mark when you fire. Otherwise it gains 3 damage. |
| AD023 | Buffer Pads | Immediate effect: Pad Bundle (no part) | Utility | 2 Iron + 1 Carbon | None | Prevent the next 4 HP Bolt would lose from an enemy attack this enemy phase. Does not prevent HP spent on commands. |
| AD024 | Trade Places | 1 Rotation Relay | Helper | 1 Copper + 1 Glass | 1 | Cancel Bolt's remaining interception allowances for this round. Gain 8 Shield. |
| AD025 | Workshop Slug | 1 Service Slug | Ammo | 2 Iron + 1 Copper | None | Add 7 damage. Add 5 more if Bolt gained HP from a repair or restore during this turn. |
| AD026 | Quiet Shift | 1 Quiet Relay | Helper | 1 Copper + 1 Carbon | None | At the start of your next turn, repair 8 Bolt HP if he made no support hit after this command this round and is still active. |
| AD027 | Tag Team Plate | 1 Team Plate | Shield | 2 Iron + 1 Glass | None | Gain 7 Shield. Gain 3 more if Bolt has dealt support damage this turn. |
| AD028 | Spare Arm | 1 Arm Brace | Modifier | 2 Iron + 1 Copper | 1 | The next Helper part used this planning phase that deals immediate support damage adds 5 damage to its first hit against one chosen recipient, then repairs 2 Bolt HP if active. Expires when you fire. |
| AD029 | Pull the Plug | 1 Stop Relay | Helper | 1 Copper + 1 Carbon | None | Bolt loses 3 HP, then apply Weaken 7 to one enemy now. He must have at least 4 HP to use it. |
| AD030 | Backup Screen | 1 Backup Guard | Shield | 2 Iron + 1 Copper | 1 | Gain 8 Shield. Gain 6 more if Bolt is disabled now. |
| AD031 | Shield Punch | 1 Ram Relay | Helper | 2 Iron + 1 Glass | None | Bolt deals 8 damage to one enemy now. This hit deals 6 extra damage if the enemy has Shield. |
| AD032 | Spare Magazine | 1 Magazine Order | Helper | 2 Iron + 1 Carbon | 1 | At the start of your next turn, receive 2 Small Rivets. Each is an Ammo part adding 4 damage. Delivery does not require Bolt to remain active. |
| AD033 | Shared Sight | 1 Sight Link | Modifier | 1 Copper + 1 Glass | None | Choose one enemy. This shot gains 6 damage if that enemy is the main target and Bolt has dealt support damage to it this turn. |
| AD034 | Cooling Cloth | Immediate effect: Damp Cloth (no part) | Utility | 1 Copper + 1 Glass | 1 | Repair 3 Bolt HP if he is active. Remove 1 cooldown counter from every currently cooling recipe. Cannot reset a recipe with no cooldown ability. |
| AD035 | Pick Your Moment | 1 Patient Relay | Helper | 1 Iron + 1 Glass | None | Bolt deals 9 damage to one enemy now if it shows a non-attack intent; otherwise he deals 5. |
| AD036 | Patch the Edge | 1 Edge Guard | Shield | 2 Iron + 1 Copper | None | Gain 10 Shield if Bolt is at full HP now; otherwise gain 6 Shield and repair 2 Bolt HP if he is active. |
| AD037 | Salvage Hand | 1 Sorting Relay | Helper | 1 Copper + 1 Carbon | 1 | At the start of your next turn, gain 2 Iron. Delivery does not require Bolt to remain active and does not grant another magnet haul. |
| AD038 | Screwdriver Tip | 1 Driver Tip | Ammo | 1 Iron + 1 Glass | None | Add 6 damage. After impact, apply Mark 3 if Bolt is active. |
| AD039 | Feed the Welder | Immediate effect: Fuel Biscuit (no part) | Utility | 1 Carbon + 1 Copper | None | Consume one unused Ammo part from reserve as an additional cost. Restore 8 Bolt HP, including while disabled. The sacrificed part does not apply its effect. |
| AD040 | Service Pair | Immediate effect: Service Tabs (no part) | Utility | 2 Copper + 1 Iron | 1 | **Superseded Utility dependency; revise before use.** Each Service Tab repairs 3 Bolt HP while he is active. Use the tabs separately or save them. |

### Uncommon

| ID | Recipe | Output / effect label | Kind | Resources | Cooldown | Effect |
| --- | --- | --- | --- | --- | --- | --- |
| AD041 | Adjustable Tool | 1 Tool Relay | Helper | 1 Iron + 1 Copper + 1 Glass | None | Choose one when used: Bolt deals 10 damage to one enemy now, or repair 10 Bolt HP. |
| AD042 | Overflow Weld | Immediate effect: Wide Weld Pack (no part) | Utility | 2 Iron + 1 Copper | 1 | Repair 9 Bolt HP while active. Gain Shield equal to the amount of that repair which exceeded his missing HP. Cannot restore a disabled Bolt. |
| AD043 | Follow My Shot | 1 Follow Relay | Helper | 2 Iron + 1 Circuit | 1 | After this round's main shot and its payloads, Bolt deals 10 damage to its main target if Bolt is active and the target is alive. The command expires after this shot. |
| AD044 | Catch and Strike | 1 Return Relay | Helper | 2 Iron + 1 Copper + 1 Carbon | 1 | Choose an enemy. This enemy phase Bolt intercepts the first 6 damage of its next attack. After that attack, if Bolt is active, he deals 8 damage to that enemy. |
| AD045 | Flying Wrench | 1 Wrench Relay | Helper | 2 Iron + 1 Carbon | 1 | Bolt loses 4 HP, then deals 8 damage to every enemy now. He must have at least 5 HP to use it. |
| AD046 | Shelter the Helper | 1 Shelter Plate | Shield | 2 Iron + 1 Copper | None | Gain 8 Shield plus 1 for each HP Bolt is missing, up to 6 extra Shield. A disabled Bolt counts as missing all his HP. |
| AD047 | Shock Apron | 1 Wired Apron | Helper | 2 Iron + 1 Copper + 1 Circuit | 1 | This enemy phase, the first two enemies whose attacks remove Bolt HP each take 7 damage after that attack. The apron still deals this damage if that attack disabled Bolt. At most two support hits. Interception must come from another part. |
| AD048 | Recovered Hardware | Immediate effect: Recovery Bag (no part) | Utility | 1 Copper + 1 Glass | 1 | Consume one unused Helper part from reserve as an additional cost. At the start of your next turn, gain 2 Iron and 1 Copper. The sacrificed command does not activate. |
| AD049 | Full Service | Immediate effect: Full Service Pack (no part) | Utility | 2 Iron + 2 Copper | 1 | Repair 12 Bolt HP while active. If he is at full HP afterward, gain 6 Shield. Cannot restore him. |
| AD050 | Safety Pin | Immediate effect: Safety Catch (no part) | Utility | 2 Iron + 1 Circuit | 2 | This enemy phase, reduce the damage assigned to Bolt by the first intercepted attack that would disable him by 8, to a minimum of 0. He may still be disabled if the remaining damage is enough. Expires after this phase. |
| AD051 | Drip Oiler | Immediate effect: Oil Feed (no part) | Utility | 1 Iron + 2 Copper + 1 Carbon | 2 | At the start of each of your next three turns, repair 3 Bolt HP if he is active. Reusing Oil Feed restarts its three-turn duration. |
| AD052 | Thick Frame | Immediate effect: Frame Sleeve (no part) | Utility | 3 Iron + 1 Copper | 2 | For this fight, the first Helper or Modifier part each turn that spends Bolt HP costs 2 less HP, to a minimum of 0. Bolt must still meet that part's stated minimum HP before use. Duplicate sleeves do not stack. |
| AD053 | Battle Scars | 1 Dented Slug | Ammo | 2 Iron + 1 Carbon | 1 | Add 6 damage plus Bolt's missing HP when you fire, up to 12 extra damage. Works while Bolt is disabled. |
| AD054 | Welded Recoil | 1 Recoil Tie | Modifier | 2 Iron + 1 Copper | 1 | Bolt loses 3 HP as an activation cost and must have at least 4 HP. Gain 7 Shield and add 10 damage to this shot. |
| AD055 | Survey Sweep | 1 Survey Relay | Helper | 1 Copper + 1 Glass + 1 Circuit | 1 | Apply Mark 5 to every enemy now. |
| AD056 | Guard Rotation | 1 Guard Relay | Helper | 2 Iron + 1 Copper | 1 | Gain 6 Shield. This enemy phase, Bolt intercepts half the damage of the next enemy attack that would reach you, rounded down, up to 8 damage. |
| AD057 | Repair Counter | 1 Service Counter | Modifier | 1 Copper + 1 Glass + 1 Circuit | 1 | Until you fire this round, each HP Bolt actually gains from repair or restore adds 1 damage to your shot, up to 12 total. Missing HP only; extra repair at full HP adds nothing. Multiple Service Counters do not raise this limit. |
| AD058 | Acid Brush | 1 Acid Relay | Helper | 1 Copper + 1 Carbon + 1 Glass | 1 | Bolt loses 2 HP, then applies Corrosion 5 to one enemy now. He must have at least 3 HP to use it. |
| AD059 | Bolt's Assembly | 1 Assembly Collar | Ammo | 2 Iron + 1 Copper | None | Add 7 damage. Add 7 more if this shot consumes at least four other Ammo parts and Bolt is active when you fire. |
| AD060 | Busy Workshop | 1 Workbench Meter | Modifier | 1 Iron + 1 Copper + 1 Circuit | 1 | If you have used at least three Helper parts this turn, gain 8 Shield and add 8 damage to this shot. Those parts must have activated; merely crafting them does not count. |
| AD061 | Rescue Timer | Immediate effect: Timed Restart (no part) | Utility | 2 Iron + 1 Copper + 1 Circuit | 1 | At the start of your next turn, restore 10 Bolt HP if he is disabled then; otherwise repair 4 HP. |
| AD062 | Packed Lunch | Immediate effect: Stored Service Pack (no part) | Utility | 2 Iron + 1 Copper | None | Restore 5 Bolt HP, including while disabled. Restore 10 instead if this part was crafted on an earlier turn. |
| AD063 | Shift Change | 1 Changeover Guard | Shield | 2 Iron + 1 Copper + 1 Glass | 1 | Gain 6 Shield now and 8 Shield at the start of your next turn, after normal Shield clearing. |
| AD064 | Jam Two Joints | 1 Twin Clamp Relay | Helper | 1 Iron + 1 Copper + 1 Glass | 1 | Apply Weaken 4 to each of two different enemies now. With only one enemy alive, apply Weaken 6 to it instead. |
| AD065 | Cross the Line | 1 Crossfire Relay | Helper | 2 Iron + 1 Carbon | 1 | Bolt deals 9 damage to one chosen enemy and 5 damage to a different chosen enemy now. With only one enemy alive, deal just the 9 damage. |
| AD066 | Scheduled Cover | 1 Cover Order | Helper | 2 Iron + 1 Copper | 1 | At the start of your next turn, gain 12 Shield after normal Shield clearing. Delivery does not require Bolt to remain active. |
| AD067 | Shielded Hands | Immediate effect: Insulated Gloves (no part) | Utility | 2 Iron + 1 Glass | 1 | Prevent the next 7 HP Bolt would lose as costs of commands or modifiers this turn. He must still meet each part's stated minimum HP to activate it. Expires when you fire. |
| AD068 | Even Repairs | Immediate effect: Balance Pack (no part) | Utility | 2 Iron + 2 Copper | None | Repair 6 Bolt HP if he is active and gain 6 Shield. If Bolt is disabled, gain 12 Shield instead. |
| AD069 | Returning Current | 1 Return Wire | Ammo | 1 Iron + 2 Copper + 1 Circuit | 1 | Add 8 damage. After the main damage, repair Bolt by half the HP that damage removed from its main target, rounded down, up to 8 HP. Cannot restore him. Payload damage does not count. |
| AD070 | Cut the Feed | 1 Feed Cutter | Helper | 1 Iron + 1 Copper + 1 Glass | 1 | Apply Weaken 9 to one enemy now if it is showing an attack and has at least 6 Shield; otherwise apply Weaken 4. |
| AD071 | Pry Plate | 1 Pry Relay | Helper | 1 Iron + 1 Copper + 1 Glass | 1 | Remove up to 12 Shield from one enemy now. If at least 6 Shield was removed, gain 2 Glass at the start of your next turn. |
| AD072 | Tracking Wrench | 1 Tracking Relay | Helper | 1 Iron + 1 Copper + 1 Glass | 1 | Bolt deals 7 damage to one enemy now. At the start of your next turn, apply Mark 7 to that enemy if it is alive. This delayed Mark does not require Bolt to remain active. |
| AD073 | Strip the Cover | 1 Strip Relay | Helper | 1 Iron + 1 Copper | 1 | Consume one unused Shield part from reserve as an additional cost. Repair 6 Bolt HP and deal 10 damage to one enemy now. The sacrificed Shield part does not activate. |
| AD074 | Hold the Panel | 1 Locking Guard + conditional next-round Shield part | Shield | 2 Iron + 1 Copper + 1 Circuit | 1 | When this part activates at End Turn, gain 10 Shield. At the beginning of the next round, automatically load a new regular 8-Shield part if Bolt is active then. |
| AD075 | Covering Tap | 1 Watch Relay | Helper | 2 Iron + 1 Copper | 1 | After the first enemy attack this phase, Bolt deals 12 damage to its attacker if he is active and it is alive. Expires after this enemy phase. |
| AD076 | Rivet Collectors | 1 Catch Tray | Modifier | 1 Iron + 1 Copper + 1 Glass | 1 | Each of the next four Shield parts you use this planning phase repairs 2 Bolt HP if he is active. Repairs happen after that Shield part's own effects. Expires when you fire. |
| AD077 | Restart Shot | 1 Starter Slug | Ammo | 2 Iron + 1 Copper + 1 Circuit | 2 | Add 9 damage. After impact, restore 8 Bolt HP, including while disabled. |
| AD078 | Borrowed Time | Immediate effect: Borrowed Battery (no part) | Utility | 1 Iron + 1 Copper + 1 Circuit | 2 | Restore Bolt to 8 HP if he is disabled and gain 8 Shield. At the end of this enemy phase, he loses 4 HP. Cannot use while he is active. |
| AD079 | Tools in Reserve | 1 Reserve Guard | Shield | 2 Iron + 1 Copper | None | Gain 6 Shield plus 2 per unused Helper part remaining in reserve after this activation, up to 8 extra Shield. |
| AD080 | First Response | 1 Response Relay | Helper | 2 Iron + 1 Copper + 1 Carbon | 1 | If an enemy attack removed your HP last round, Bolt deals 14 damage to one enemy now; otherwise he deals 8. Self-paid HP does not count. |

### Rare

| ID | Recipe | Output / effect label | Kind | Resources | Cooldown | Effect |
| --- | --- | --- | --- | --- | --- | --- |
| AD081 | Officer's Frame | Immediate effect: Reinforced Frame (no part) | Utility | 3 Iron + 1 Copper + 1 Circuit | 2 | Increase Bolt's maximum HP by 6 for this fight, up to 24, and restore 6 HP, including while disabled. This named maximum-HP increase applies once per fight. |
| AD082 | Roadside Rebuild | Immediate effect: Rebuild Crate (no part) | Utility | 2 Iron + 2 Copper + 1 Circuit | 2 | Restore 12 Bolt HP, including while disabled. Gain 8 Shield if he was disabled before this Utility recipe was used. |
| AD083 | Work Order | 1 Mixed Order | Helper | 2 Iron + 1 Copper + 1 Circuit | 2 | At the start of your next turn, receive 2 Small Rivets and 1 Guard Tab. Each rivet is Ammo adding 4 damage; the tab is a Shield part granting 8 Shield. Delivery does not require active Bolt. |
| AD084 | Fast Hands | 1 Fast Relay | Modifier | 1 Iron + 2 Copper + 1 Circuit | 2 | Each of the next three Helper parts used this planning phase that directly deals support damage adds 5 damage to its first support hit against one chosen recipient. Delayed hits do not qualify. Expires when you fire. |
| AD085 | Running Repairs | Immediate effect: Running Kit (no part) | Utility | 2 Iron + 2 Copper + 1 Circuit | 2 | For your next three planning phases, including this one, Bolt's first support hit each phase deals 5 extra damage and then repairs 3 Bolt HP if he is active. Only one recipient gains the extra damage for a multi-enemy hit. |
| AD086 | Hand Crank | 1 Crank Relay | Helper | 2 Iron + 1 Copper + 1 Circuit | 2 | Bolt loses 5 HP, then remove 3 cooldown counters from every currently cooling recipe. He must have at least 6 HP to use it. |
| AD087 | Tool Reset | 1 Reset Relay | Helper | 1 Iron + 2 Copper + 1 Circuit | 2 | Bolt loses 3 HP, then remove every cooldown counter from every currently cooling recipe. He must have at least 4 HP. Does not reset recipes without cooldown. |
| AD088 | Set the Charges | 1 Charge Order | Helper | 2 Iron + 1 Carbon + 1 Circuit | 2 | At the start of your next turn, deal 10 damage and apply Burn 4 to every surviving enemy. Delivery does not require Bolt to remain active. These are support hits. |
| AD089 | Crew Effort | 1 Crew Slug | Ammo | 2 Iron + 1 Carbon + 1 Circuit | 1 | Add 8 damage plus 4 per Helper part you activated this turn, up to 20 extra damage. Crafting a command without using it does not count. |
| AD090 | Crash Guard | 1 Crash Relay | Helper | 3 Iron + 1 Carbon | 2 | This enemy phase, Bolt intercepts the next attack that would reach you, up to 18 damage. After it, deal support damage to the attacker equal to the HP Bolt lost to that attack, even if Bolt was disabled by it. |
| AD091 | Strip for Spares | 1 Stripdown Kit | Helper | 1 Iron + 1 Copper + 1 Circuit | 2 | Bolt must have at least 6 HP. Set his HP to 0 and disable him; this cannot be reduced or prevented by effects that lower HP costs. At the start of your next turn, gain 3 Iron, 2 Copper and 1 Circuit. No command or interception can use him until he is restored. |
| AD092 | Shared Scaffold | 1 Scaffold Guard + up to 2 scheduled Shield parts | Shield | 3 Iron + 1 Copper + 1 Circuit | 2 | When this part activates at End Turn, gain 16 Shield. At the beginning of each of the next two rounds, automatically load a new regular 12-Shield part if Bolt is active then. Check Bolt separately at each scheduled delivery. |
| AD093 | Reach the Core | 1 Core Drill Relay | Helper | 2 Iron + 1 Glass + 1 Circuit | 2 | Bolt deals 13 support damage to one chosen enemy now, bypassing its Shield. Bolt then loses 2 HP. He must have at least 3 HP before using the command. This is a support hit for support-damage bonuses and triggers. |
| AD094 | Service Rhythm | 1 Rhythm Relay | Modifier | 2 Iron + 1 Copper + 1 Circuit | 2 | After each of the next three Helper parts used this planning phase deals immediate support damage, repair 4 Bolt HP if active. One repair per part, even if it hits several enemies. Expires when you fire. |
| AD095 | Press the Plate | 1 Press Link | Modifier | 2 Iron + 1 Copper + 1 Carbon | 1 | Spend 12 of your current Shield as an activation cost. Repair 6 Bolt HP if active and add 20 damage to this shot. Cannot activate with less than 12 Shield. |
| AD096 | Break the Routine | 1 Disruption Relay | Helper | 1 Iron + 1 Copper + 1 Glass + 1 Circuit | 2 | Choose one enemy. If it shows an attack intent, apply Weaken 13 now. Otherwise remove up to 20 of its Shield and apply Mark 10. |
| AD097 | Supply Courier | 1 Courier Order | Helper | 1 Copper + 1 Glass + 1 Circuit | 2 | **Superseded Utility dependency; revise before use.** Consume two unused Utility parts from reserve as an additional cost. At the start of your next turn, gain 3 Iron, 2 Copper and 1 Carbon. The sacrificed parts do not activate. |
| AD098 | Two Shifts | 1 Double Service Order | Helper | 2 Iron + 2 Copper + 1 Circuit | 2 | At the start of each of your next two turns, gain 8 Shield after clearing and repair 5 Bolt HP if he is active. The Shield delivery works even while he is disabled. |
| AD099 | Launch the Fist | 1 Fist Launcher | Ammo | 3 Iron + 1 Carbon + 1 Circuit | 2 | Add 10 damage. At Fire, process each loaded Fist Launcher separately before main damage: if Bolt currently has at least 8 HP, he loses 7 HP and this part adds 20 more damage. Otherwise this part adds only its 10 damage and costs no Bolt HP. |
| AD100 | Ready on Three | 1 Third-Turn Sight | Modifier | 1 Iron + 1 Copper + 1 Glass + 1 Circuit | 2 | This shot gains 30% damage if the current fight turn is 3, 6, 9 or another multiple of 3 and Bolt is active when you fire. Otherwise it gains 10 damage. |
| AD101 | Collect the Marks | 1 Collection Relay | Helper | 2 Iron + 1 Glass + 1 Circuit | 2 | Choose up to three enemies with Mark. Remove each one's Mark and deal that enemy support damage equal to 8 plus the Mark removed, up to 22 damage per enemy. |
| AD102 | Drill Guide | 1 Guided Driver | Ammo | 2 Iron + 1 Copper + 1 Glass | 1 | Before the main damage, remove Shield from the main target equal to Bolt's current HP, up to 16. Add 12 damage. A disabled Bolt removes no Shield. |
| AD103 | Trip Alarm | 1 Alarm Relay | Helper | 2 Iron + 1 Copper + 1 Circuit | 2 | Until the start of your next turn, the first time Bolt becomes disabled, apply Weaken 9 to every enemy and gain 10 Shield. Triggers once; self-paid HP may trigger it. |
| AD104 | Last-Minute Brace | 1 Emergency Guard | Shield | 3 Iron + 1 Copper | 1 | Gain 12 Shield. Gain 4 more for each enemy showing an attack intent, up to 12 extra Shield. Bolt cannot intercept during this enemy phase; cancel his existing allowances for it. |
| AD105 | Trophy Rivets | 1 Trophy Guard | Shield | 2 Iron + 1 Copper + 1 Circuit | 2 | Gain 6 Shield plus half the support damage Bolt dealt last turn, rounded down, up to 18 extra Shield. Actual damage to enemy Shield or HP counts; overkill does not. |
| AD106 | Tight Formation | 1 Split Guide | Ammo | 2 Iron + 1 Copper + 1 Glass + 1 Circuit | 2 | Add 8 damage. If Bolt has at least half his maximum HP when you fire, choose one extra enemy to receive 50% of this shot's damage, rounded down. It receives no copied payloads. |
| AD107 | Dark Shift | Immediate effect: Blackout Reserve (no part) | Utility | 2 Iron + 1 Copper + 1 Circuit | 3 | For this fight, the first time Bolt becomes disabled after installing this reserve, gain 22 Shield. Triggers only once per fight; reinstalling does not restore a spent trigger. |
| AD108 | Cooling Bench | 1 Bench Order | Helper | 1 Iron + 1 Copper + 1 Glass + 1 Circuit | 2 | At the start of your next turn, remove 1 cooldown counter from every currently cooling recipe. Delivery does not require active Bolt. Does not reset recipes without cooldown. |

### Legendary

| ID | Recipe | Output / effect label | Kind | Resources | Cooldown | Effect |
| --- | --- | --- | --- | --- | --- | --- |
| AD109 | Steel Friendship | Immediate effect: Linked Guard Harness (no part) | Utility | 2 Iron + 2 Copper + 1 Circuit | 3 | For this fight, prevent the first 4 HP Bolt would lose to enemy attacks each enemy phase. At the start of each of your turns, repair 2 Bolt HP if active. Does not prevent HP paid as costs. Duplicate harnesses do not stack. |
| AD110 | City Workshop | Immediate effect: Workshop Frame (no part) | Utility | 3 Iron + 2 Copper + 1 Circuit | 3 | Set Bolt's maximum HP to 24 for this fight if lower and restore 12 HP, including while disabled. For this fight, his first support hit each turn adds 1 damage per 4 current Bolt HP, rounded down. For a multi-enemy hit, choose one recipient for the bonus. Duplicate frames do not stack. |
| AD111 | Finish Together | 1 Finishing Relay | Helper | 2 Iron + 1 Carbon + 1 Glass + 1 Circuit | 3 | Choose one enemy with no Shield and 25 or less HP. Bolt deals 25 damage to it now. If that defeats it, restore 8 Bolt HP and add 15 damage to this round's main shot. Cannot target an enemy above the threshold. |
| AD112 | The Long Arm | 1 Long-Arm Guide | Ammo | 2 Iron + 1 Copper + 1 Carbon + 1 Circuit | 3 | Add 16 damage. If Bolt is active when you fire, choose up to two extra enemies to receive 60% of this shot's damage each, rounded down. Do not copy payloads. Bolt loses 4 HP after impact, to a minimum of 0. |
| AD113 | Last Good Part | 1 Last-Part Relay | Helper | 2 Iron + 1 Copper + 1 Carbon + 1 Circuit | 3 | Bolt must have at least 8 HP. Deal 28 support damage to one enemy now, then set Bolt to 0 HP; this cannot be reduced or prevented by effects that lower HP costs. At the start of your next turn, restore him to at least 6 HP if below 6. This delayed restore works while disabled. |
| AD114 | Guard the Gate | 1 Gate Relay | Helper | 3 Iron + 1 Copper + 1 Glass + 1 Circuit | 3 | Gain 18 Shield. This enemy phase, Bolt intercepts up to 12 damage from attacks and cannot lose his last HP through this interception. Damage beyond that allowance or his safe HP reaches you normally. |
| AD115 | Perfect Repair | Immediate effect: Precision Service Kit (no part) | Utility | 2 Iron + 2 Copper + 1 Glass + 1 Circuit | 2 | Restore Bolt to full HP, including while disabled. Add damage to your next main shot this round only, equal to the HP actually restored, up to 24. Excess repair at full HP adds nothing. |
| AD116 | Crew Contract | Immediate effect: Crew Link (no part) | Utility | 2 Iron + 2 Copper + 1 Circuit | 3 | For this fight, the first Helper part you activate each turn grants 8 damage to your next main shot that turn only and grants 5 Shield. This is one bonus per turn, not per hit or per target. Duplicate links do not stack. |
| AD117 | Night Shift | 1 Overnight Order | Helper | 2 Iron + 2 Copper + 1 Carbon + 1 Circuit | 3 | At the start of your next turn, restore 10 Bolt HP and receive 2 Heavy Rivets and 2 Guard Tabs. Each Heavy Rivet is Ammo adding 8 damage; each Guard Tab is a Shield part granting 8 Shield. Delivery works while Bolt is disabled. |
| AD118 | Full Tool Rack | 1 Tool Rack Relay | Helper | 2 Iron + 1 Copper + 1 Glass + 2 Circuit | 3 | Remove all cooldown counters from every currently cooling recipe. Bolt loses 6 HP and must have at least 7 HP before use. Does not reset recipes without cooldown. |
| AD119 | Stand Back Up | Immediate effect: Recovery Harness (no part) | Utility | 2 Iron + 2 Copper + 1 Carbon + 1 Circuit | 3 | For this fight, the next two times Bolt is disabled, immediately restore him to 6 HP after the disabling action fully resolves. Each trigger grants 6 Shield. Any attack damage that already passed to you is not reversed. Duplicate harnesses do not add or replenish charges. |
| AD120 | Every Last Rivet | 1 Final Assembly Collar | Ammo | 3 Iron + 1 Copper + 1 Carbon + 1 Circuit | 3 | Add 14 damage. Add 2 more per other Ammo part consumed in this shot, up to 40 extra damage. If Bolt is active at Fire, gain 1 Shield per other Ammo part consumed, up to 20 Shield, after impact. |

## Noor Flux Sayeed

**Weapon:** Coil Cannon.


**Draft identity:** Noor saves electricity between rounds and decides how much to spend on protection, a stronger shot, or bringing an important recipe back online. She has no helper. Her strongest combinations need Charge prepared before the shot; electricity recovered from an impact is useful for a later round.

**Proposed inherent ability — Residual Current:** once each player turn, after a Utility actually removes one or more recipe cooldown counters, gain 1 Charge. Pay the Utility's full costs first. Several removed counters still grant only 1 Charge; a cooling effect with no counters to remove grants none. The turn's trait opportunity is consumed even at maximum Charge and cannot itself be refreshed through cooling. See the [complete ability rules](MERCENARY-ABILITIES.md#noor--residual-current). This replaces Charged Barrel's independent shot-damage grant.

**Retained Charge convention, still proposed:** Charge starts at 0 each fight, stays between 0 and 12, and carries between turns without ordinary gain or loss. Gains above 12 are lost. A listed part activation payment must be affordable before that part can be used. Ammo that grants Charge does so after impact and cannot pay for the shot that granted it. Support hits do not replay Ammo payloads. Charge powers recipe/part effects; there is no separate trait payment at Fire to add damage.

**Three possible builds:**

- **Keep a reserve:** Cell Winding (NO001), Full Cell Tip (NO025), Standing Charge (NO056), and Reserve Coil (NO110). Build a high Charge reserve and weigh every payment against the damage available from keeping it.
- **Make a field, then release it:** Field Plate (NO003), Shield Socket (NO023), Guarded Current (NO066), and City Grid (NO113). Protect the current round, then recover or spend electricity for the next attack.
- **Craft the right part again:** Quick Bridge (NO004), Twin Bridge (NO041), Total Recall (NO092), and Open Circuit (NO114). Spend Charge and raw materials to remove selected cooldown counters; recipes without cooldown remain once per turn unless a permanent upgrade says otherwise.

**Future permanent-upgrade hooks, not an upgrade catalogue:** start fights with a small Charge reserve; increase Charge capacity; gain a small, limited amount of Charge next round after successfully protecting against attacks. Exact values and availability remain to design.

### Base

| ID | Recipe | Output / effect label | Kind | Resources | Cooldown | Effect |
| --- | --- | --- | --- | --- | --- | --- |
| NO001 | Cell Winding | Immediate effect: Wound Cell (no part) | Utility | 1 Copper | None | Gain 3 Charge when activated. |
| NO002 | Flux Pin | 1 Conducting Pin | Ammo | 1 Iron + 1 Copper | None | Add 6 damage. Gain 1 Charge after the main shot's impact. |
| NO003 | Field Plate | 1 Field Plate | Shield | 1 Iron + 1 Copper | None | Gain 6 Shield. You may pay 1 Charge when activated to gain 4 more Shield. |
| NO004 | Quick Bridge | Immediate effect: Bridge Clip (no part) | Utility | 1 Copper | None | Pay 2 Charge when activated. Remove 1 cooldown counter from every currently cooling recipe. This does not reset a recipe without cooldown. |

### Common

| ID | Recipe | Output / effect label | Kind | Resources | Cooldown | Effect |
| --- | --- | --- | --- | --- | --- | --- |
| NO005 | Live Wire Tip | 1 Live Tip | Ammo | 1 Iron + 1 Copper | None | Add 7 damage. Add 5 more if Charge is at least 6 at Fire, after the barrel's optional Charge payment. |
| NO006 | Grounded Slug | 1 Ground Slug | Ammo | 2 Iron | None | Add 8 damage. Add 6 more if Charge is 0 at Fire, after the barrel's optional Charge payment. |
| NO007 | Arc Tooth | 1 Arc Tooth | Ammo | 1 Iron + 1 Copper + 1 Glass | None | Add 6 damage. Before main damage, remove up to 5 Shield from the main target. If this removes all 5, gain 1 Charge after impact. |
| NO008 | Closed Field | 1 Closed Field Ring | Shield | 1 Iron + 2 Copper | None | Gain 10 Shield. If Charge is at least 8 when activated, gain 3 more without spending Charge. |
| NO009 | Starting Field | 1 Small Field Coil | Shield | 1 Iron + 1 Copper | None | Gain 4 Shield and 2 Charge when activated. |
| NO010 | Shield Tap | Immediate effect: Shield Tap (no part) | Utility | 1 Copper + 1 Glass | None | Pay 6 Shield when activated, then gain 5 Charge. Cannot activate with less than 6 Shield. |
| NO011 | Coil Collar | 1 Discharge Collar | Modifier | 1 Iron + 1 Copper | None | Pay 3 Charge when activated. Add 10 damage to this round's main shot. |
| NO012 | Return Pin | 1 Return Pin | Ammo | 1 Iron + 2 Copper | None | Add 6 damage. After impact, gain 1 Charge for every full 6 main-shot damage absorbed by the main target's Shield, up to 4 Charge. Damage absorbed by other enemies does not count. |
| NO013 | Warning Lamp | Immediate effect: Warning Lamp (no part) | Utility | 1 Copper + 1 Glass | None | Apply Weaken 4 to one chosen enemy. If it currently shows an attack intent, also apply Mark 3 to it. |
| NO014 | Wide Field | 1 Wide Field Strip | Shield | 1 Iron + 1 Copper + 1 Glass | None | Gain 4 Shield for each living enemy, up to 12 Shield. Gain 1 Charge if at least two enemies currently show attack intent. |
| NO015 | Slow Cell | Immediate effect: Slow Cell (no part) | Utility | 2 Copper | None | Gain 1 Charge now and 4 Charge at the start of next turn. |
| NO016 | Copper Catcher | Immediate effect: Copper Catch Cup (no part) | Utility | 1 Iron + 1 Glass | 1 | Pay 1 Charge when activated. At the start of next turn, gain 2 Copper in your raw resource pool. |
| NO017 | Hot Contact | 1 Hot Contact Tip | Ammo | 1 Iron + 1 Copper + 1 Carbon | None | Add 7 damage. If at least 3 Charge was paid as part activation costs this round, apply Burn 3 to the main target after impact. The barrel's Fire payment does not count. |
| NO018 | Field Anchor | 1 Field Anchor | Shield | 2 Iron + 1 Copper | 1 | Pay 2 Charge when activated. Gain 10 Shield. If you end this enemy phase with at least 2 Charge, gain 6 Shield at the start of next turn, after old Shield clears. |
| NO019 | Safe Jumper | Immediate effect: Jumper Lead (no part) | Utility | 1 Copper + 1 Glass | 1 | Remove 1 cooldown counter from every currently cooling recipe. Gain 1 Charge once if this makes at least one recipe ready. |
| NO020 | Bare Coil | Immediate effect: Bare Coil (no part) | Utility | 2 Copper | None | Gain 5 Charge. You may use this Utility recipe only while you have 0 Shield. |
| NO021 | Reverse Pin | 1 Reverse Pin | Ammo | 1 Iron + 1 Copper + 1 Glass | None | Add 9 damage. If the main target is showing an attack intent at Fire, apply Weaken 3 to it after impact. |
| NO022 | Glass Guard | 1 Glass Guard | Shield | 1 Iron + 2 Glass | None | Gain 8 Shield. Gain 3 more if you have paid Charge as a part activation cost this round. |
| NO023 | Shield Socket | Immediate effect: Shield Socket (no part) | Utility | 1 Copper + 1 Glass | 1 | Until the end of this enemy phase, after the first enemy attack fully absorbed by your Shield, gain 3 Charge. Extra Shield Sockets do not increase this gain. |
| NO024 | Split Lead | Immediate effect: Split Lead (no part) | Utility | 1 Copper + 1 Circuit | 1 | Pay 2 Charge when activated. Apply Mark 4 to each of two different chosen enemies. Cannot activate without two living enemies. |
| NO025 | Full Cell Tip | 1 Full Cell Tip | Ammo | 1 Iron + 2 Copper | 1 | Add 8 damage. Add 12 more if Charge is 12 at Fire, after the barrel's optional Charge payment. |
| NO026 | Emergency Ground | 1 Ground Clamp | Shield | 1 Iron + 1 Copper | None | Pay all your Charge, at least 1, when activated. Gain 2 Shield per Charge paid, up to 16 Shield. |
| NO027 | Quiet Coil | Immediate effect: Quiet Coil (no part) | Utility | 1 Copper + 1 Carbon | None | Gain 4 Charge when activated. This Utility recipe cannot be used after any other part has paid Charge this round. |
| NO028 | Contact Mark | 1 Marking Contact | Ammo | 1 Iron + 1 Copper + 1 Glass | None | Add 7 damage. After impact, apply Mark 3 to the main target, or Mark 7 if the barrel paid at least 2 Charge at Fire. |
| NO029 | First Field | 1 First Layer Film | Shield | 1 Copper + 1 Glass | None | Gain 5 Shield. Gain 5 more if no other Shield part has been activated this round. |
| NO030 | Spare Wiring | Immediate effect: Spare Wire Reel (no part) | Utility | 2 Copper + 1 Iron | None | When activated, consume one unused Ammo part from reserve without its effect, then gain 6 Charge. Cannot activate without an Ammo part to consume. |
| NO031 | Copper Pins | 2 Copper Pins | Ammo | 1 Iron + 2 Copper | None | Each Copper Pin adds 3 damage and applies Mark 2 to the main target after impact. They may be consumed in different rounds; their Marks help later main shots. |
| NO032 | Sudden Field | 1 Fast Field Loop | Shield | 1 Iron + 1 Copper + 1 Glass | 1 | Gain 7 Shield. Reduce the total damage of the first enemy attack against you this round by your Charge when this part was activated, up to a reduction of 6. Charge is not spent. |
| NO033 | Pocket Spark | Immediate effect: Spark Button (no part) | Utility | 1 Copper + 1 Carbon | 1 | Pay 2 Charge when activated. Deal 6 damage to one chosen enemy now as a support hit. |
| NO034 | Late Field | 1 Delayed Field Loop | Shield | 1 Iron + 2 Copper | 1 | Gain 4 Shield now. At the start of next turn, gain 6 Shield and 2 Charge, after old Shield clears. |
| NO035 | Even Current | 1 Paired Contact | Ammo | 1 Iron + 1 Copper | None | Add 6 damage. Add 6 more if you have an even positive amount of Charge at Fire, after the barrel's optional Charge payment. Zero does not qualify. |
| NO036 | Shield Needle | 1 Shield Needle | Ammo | 1 Iron + 1 Copper + 1 Glass | None | Add 6 damage. After impact, gain 4 Shield, or 8 Shield if the main target still has Shield after main damage. |
| NO037 | Short Reserve | Immediate effect: Reserve Plug (no part) | Utility | 1 Copper + 1 Circuit | 1 | Gain 2 Charge. If you started this activation with 0 Charge, also remove 1 cooldown counter from every currently cooling recipe. |
| NO038 | Field Peg | 1 Field Peg | Shield | 1 Iron + 1 Copper | None | Gain 6 Shield. For each other Shield part already activated this round, gain 1 Charge, up to 3 Charge. |
| NO039 | Loose Contact | 1 Loose Contact | Modifier | 1 Iron + 1 Copper | None | Add 8 damage to this round's main shot. If you pay no Charge through the barrel at Fire, add 4 more damage. |
| NO040 | Clean Socket | Immediate effect: Clean Socket (no part) | Utility | 1 Copper + 1 Glass | 1 | Pay 1 Charge when activated. Remove up to 2 cooldown counters from every currently cooling recipe. Cannot activate unless at least one recipe has at least 2 counters. Does not reset recipes without cooldown. |

### Uncommon

| ID | Recipe | Output / effect label | Kind | Resources | Cooldown | Effect |
| --- | --- | --- | --- | --- | --- | --- |
| NO041 | Twin Bridge | Immediate effect: Twin Bridge (no part) | Utility | 2 Copper + 1 Glass | 1 | Pay 4 Charge when activated. Remove 1 cooldown counter from every currently cooling recipe. Gain 6 Shield once if at least two recipes become ready from this effect. |
| NO042 | Recovering Field | 1 Recovering Field Ring | Shield | 2 Iron + 1 Copper + 1 Glass | 1 | Gain 10 Shield. After the first enemy attack this round finishes, restore up to 8 Shield that attack removed. This restoration cannot protect against the attack that triggered it. |
| NO043 | Crossing Arc | Immediate effect: Crossing Arc Contact (no part) | Utility | 2 Copper + 1 Glass | 1 | Pay 4 Charge when activated. Deal 8 damage to one chosen enemy and 4 damage to one other chosen enemy now. If only one enemy is alive, the second hit is lost. |
| NO044 | Charge Cage | 1 Charge Cage | Shield | 1 Iron + 2 Copper | None | Gain 10 Shield. After enemies finish their actions, before the Shield reset, if at least 5 total active Shield remains, gain 3 Charge at that time. |
| NO045 | Held Pulse | 1 Held Pulse Ring | Modifier | 1 Iron + 1 Copper + 1 Circuit | 1 | Pay 4 Charge when activated. Add 8 damage to this round's main shot and 12 damage to next round's main shot. Extra Held Pulse Rings do not stack the next-round bonus. |
| NO046 | Sealed Battery | Immediate effect: Sealed Battery (no part) | Utility | 1 Iron + 2 Copper | 1 | When activated, choose and pay 2 to 6 Charge. At the start of next turn, gain the amount paid plus 3 Charge. The normal maximum of 12 still applies. |
| NO047 | Test Current | Immediate effect: Test Current Probe (no part) | Utility | 1 Copper + 1 Glass + 1 Circuit | 1 | Choose one enemy. For the rest of this round, the first three support hits that cause that enemy to lose HP each apply Mark 2 to it. Main shots and Burn or Corrosion ticks do not trigger this. |
| NO048 | Discharge Record | 1 Discharge Counter | Modifier | 1 Iron + 2 Copper | 1 | Add 10 damage to this round's main shot. Also add 5% to its damage per Charge paid as part activation costs this round, up to 30%. The barrel's Fire payment does not count. |
| NO049 | Broken Feed | 1 Feed Breaker Tip | Ammo | 1 Iron + 1 Copper + 1 Circuit | 1 | Add 9 damage. After impact, reduce the main target's next Shield gain this enemy phase by 12, to a minimum of 0. If it gains no Shield this phase, this reduction expires. |
| NO050 | Branching Coil | 1 Branch Coil | Modifier | 1 Iron + 2 Copper + 1 Glass | 1 | Pay 3 Charge when activated. At Fire, choose one extra enemy. It takes 40% of this shot's damage before target-specific Mark and defence, without copied payloads. After that damage, apply Mark 4 to it if alive. Extra Branch Coils do not repeat either effect. |
| NO051 | Ready Signal | Immediate effect: Ready Signal Lamp (no part) | Utility | 1 Copper + 1 Glass + 1 Circuit | 1 | At the start of next turn, remove 1 cooldown counter from every currently cooling recipe. Does not reset recipes without cooldown. |
| NO052 | Stored Circuit | Immediate effect: Store Counter (no part) | Utility | 1 Iron + 2 Copper | None | At the end of this enemy phase, gain 1 Charge for every two unused crafted parts still in reserve, up to 5 Charge. Loaded or consumed parts do not count. |
| NO053 | Lightning Post | 1 Lightning Post | Shield | 2 Iron + 1 Copper + 1 Glass | 1 | Gain 8 Shield. You may pay 1 to 3 Charge when activated. After the first enemy attack against you this round, deal 4 damage per Charge paid to that attacker. |
| NO054 | Emergency Cell | Immediate effect: Bloodline Cell (no part) | Utility | 2 Copper + 1 Carbon | 1 | Lose 2 HP when activated, leaving at least 1 HP. Gain 5 Charge and 6 Shield. Shield cannot pay the HP cost. |
| NO055 | Split Battery | Immediate effect: Small Cells (no part) | Utility | 2 Copper + 1 Glass | None | **Superseded Utility dependency; revise before use.** Each Small Cell gives 2 Charge when activated. They may be saved or activated in the same round. |
| NO056 | Standing Charge | Immediate effect: Standing Coil (no part) | Utility | 2 Copper + 1 Iron + 1 Circuit | 2 | For this fight, if you end an enemy phase with at least 6 Charge, gain 5 Shield at the start of the following turn, after old Shield clears. Reusing Standing Coil does not add another Shield source. |
| NO057 | Recall Pin | 1 Recall Pin | Ammo | 1 Iron + 1 Copper + 1 Circuit | 1 | Add 8 damage. After impact, remove 1 cooldown counter from every currently cooling recipe. Does not reset recipes without cooldown. |
| NO058 | Discharge Gate | 1 Discharge Gate | Shield | 2 Iron + 1 Copper | None | Gain 6 Shield plus 2 Shield for every Charge paid as part activation costs this round, up to 12 extra Shield. The barrel's Fire payment does not count. |
| NO059 | Quiet Return | 1 Quiet Return Contact | Ammo | 1 Iron + 2 Copper + 1 Glass | 1 | Add 10 damage. If the main shot kills its main target, gain 5 Charge after impact; otherwise gain 1 Charge. |
| NO060 | Field Pocket | 1 Field Pocket | Shield | 1 Iron + 2 Copper + 1 Glass | 1 | When this part activates at End Turn, gain 12 Shield. After enemies finish their actions, just before the Shield reset, count your total remaining active Shield and record 1 Charge per complete 3 Shield, up to 3 Charge. At the beginning of the next round, gain that recorded Charge under the normal Charge rules. Count before resetting; do not read a new Shield value next round. The Shield itself follows its normal reset or explicit upgrade exception. The recorded reward expires if this fight ends before delivery. |
| NO061 | Current Divider | Immediate effect: Current Divider (no part) | Utility | 1 Copper + 1 Glass + 1 Circuit | 1 | When activated, choose and pay 2 to 5 Charge. Apply Weaken 2 per Charge paid to one chosen enemy. |
| NO062 | Second Contact | 1 Second Contact Tip | Ammo | 1 Iron + 1 Copper + 1 Circuit | 1 | Add 8 damage. Add 10 more if you have crafted the same numeric-cooldown recipe at least twice this round. It need not be Second Contact. |
| NO063 | Twin Field | 2 Small Field Rings | Shield | 2 Iron + 1 Copper + 1 Glass | None | Each Small Field Ring gives 6 Shield when activated. They may be saved or used together. |
| NO064 | Empty Socket | 1 Empty Socket Ring | Shield | 1 Iron + 1 Copper + 1 Glass | 1 | Can activate only at 0 Charge. Gain 16 Shield, then gain 1 Charge. |
| NO065 | Copper Stamp | Immediate effect: Conducting Stamp (no part) | Utility | 1 Copper + 2 Glass | None | Apply Mark 3 to one chosen enemy. You may pay up to 3 Charge when activated to add 2 more Mark per Charge paid. |
| NO066 | Guarded Current | Immediate effect: Guarded Current Loop (no part) | Utility | 1 Iron + 2 Copper + 1 Circuit | 2 | For this fight, the first enemy attack in each round fully absorbed by your Shield gives 2 Charge after that attack. Reusing the loop does not add triggers. |
| NO067 | Furnace Bypass | Immediate effect: Bypass Lead (no part) | Utility | 2 Copper + 1 Circuit | 1 | Pay 3 Charge when activated. At the start of next turn, gain 2 Iron and 1 Glass. |
| NO068 | Long Contact | 1 Long Contact Tip | Ammo | 1 Iron + 2 Copper + 1 Glass | 1 | Add 10 damage. If the main target has Mark immediately before main damage, apply Weaken 7 to it after impact. The main shot still consumes Mark normally. |
| NO069 | Full Ground | 1 Full Ground Plate | Shield | 2 Iron + 2 Copper | 1 | Pay all your Charge, at least 6, when activated. Gain 18 Shield now and 1 Copper at the start of next turn for each full 3 Charge paid, up to 4 Copper. |
| NO070 | Charged Glass | 1 Charged Lens | Ammo | 1 Iron + 1 Copper + 2 Glass | 1 | Add 8 damage. If Charge is at least 8 at Fire, the main target loses 5 HP directly after impact, bypassing Shield. |
| NO071 | Circuit Notes | Immediate effect: Circuit Notebook (no part) | Utility | 1 Iron + 1 Copper + 1 Circuit | 1 | At the start of next turn, gain 1 Copper for each different numeric-cooldown recipe you crafted after using this Utility recipe this round, up to 4 Copper. Recrafting the same recipe counts once. |
| NO072 | Insulated Rack | Immediate effect: Insulated Rack (no part) | Utility | 1 Iron + 1 Copper + 1 Glass | 1 | Choose one unused Shield part already in reserve. If it is activated next round, gain 4 Charge after its effect. Multiple Insulated Racks on the same part do not stack. |
| NO073 | Polarity Plate | 1 Polarity Plate | Shield | 2 Iron + 1 Copper + 1 Glass | 1 | Gain 10 Shield. Choose an enemy currently showing an attack; after its next attack against you this round, apply Mark 8 to it. The Mark helps a later main shot. |
| NO074 | Supply Pulse | Immediate effect: Supply Pulse Plug (no part) | Utility | 2 Copper + 1 Glass | 1 | Pay 4 Charge when activated. At the start of next turn, gain 1 Circuit. |
| NO075 | Shot Ground | 1 Grounding Nose | Ammo | 2 Iron + 1 Copper + 1 Glass | 1 | Add 14 damage. Gain 2 Charge after impact if the barrel paid at least 2 Charge at Fire. |
| NO076 | Reserve Screen | 1 Reserve Screen | Shield | 2 Iron + 1 Copper + 1 Circuit | 1 | Gain 8 Shield. Gain 8 more if at least two recipes in memory currently have numeric cooldown counters when activated. |
| NO077 | Spark Trace | Immediate effect: Spark Trace Contact (no part) | Utility | 2 Copper + 1 Glass | 1 | Pay 3 Charge when activated. Deal 6 damage to one chosen enemy. If it loses HP to this hit, apply Mark 6 to it. This is a support hit. |
| NO078 | Delayed Contact | 1 Delayed Contact Tip | Ammo | 1 Iron + 2 Copper + 1 Circuit | 1 | Add 12 damage. At the start of next turn, gain 3 Charge if this shot's main target is still alive. |
| NO079 | Field Price | 1 Field Exchange Plug | Modifier | 1 Iron + 1 Copper + 1 Circuit | 1 | Pay 10 Shield when activated. Add 16 damage to this round's main shot and gain 2 Charge after its impact. |
| NO080 | Pulse Map | 1 Pulse Map Lens | Modifier | 1 Iron + 1 Copper + 1 Glass | 1 | Add 4 damage to this round's main shot for each different recipe from which cooldown counters were removed by parts this round, up to 16 damage. Normal round progression does not count. |

### Rare

| ID | Recipe | Output / effect label | Kind | Resources | Cooldown | Effect |
| --- | --- | --- | --- | --- | --- | --- |
| NO081 | Patient Coil | Immediate effect: Patient Coil (no part) | Utility | 1 Iron + 2 Copper + 1 Circuit | 2 | For this fight, gain 3 Charge after any main-shot impact for which you paid no Charge through the barrel at Fire. Other part Charge payments do not break this condition. Reusing Patient Coil does not add triggers. |
| NO082 | Double Branch | 1 Double Branch Ring | Modifier | 1 Iron + 2 Copper + 1 Glass + 1 Circuit | 2 | Pay 6 Charge when activated. At Fire, choose up to two extra enemies. Each takes 50% of this shot's damage before target-specific Mark and defence, without copied payloads. Afterward, gain 1 Charge per chosen enemy whose Shield absorbed any of that damage, up to 2. Extra rings do not repeat these effects. |
| NO083 | Restarting Field | 1 Restarting Field Frame | Shield | 2 Iron + 2 Copper + 1 Circuit | 2 | Pay 4 Charge when activated. Gain 16 Shield. The first time an enemy attack reduces positive Shield to 0 this round, gain 16 Shield after that attack finishes. Extra frames add their initial Shield but do not repeat this restart. |
| NO084 | Copper Reserve | Immediate effect: Copper Reserve Box (no part) | Utility | 1 Iron + 2 Copper + 1 Circuit | 2 | Pay 5 Charge when activated. At the start of next turn, gain 3 Copper and 2 Glass. |
| NO085 | Ground Burst | Immediate effect: Ground Burst Contact (no part) | Utility | 1 Iron + 2 Copper + 1 Carbon + 1 Circuit | 2 | Pay 8 Charge when activated. Deal 10 damage and apply Weaken 3 to every living enemy now. These are support hits. |
| NO086 | Return Network | 1 Return Network Plate | Shield | 2 Iron + 2 Copper + 1 Circuit | 2 | Gain 16 Shield. At the start of next turn, gain 1 Copper for each different enemy whose attack was fully absorbed by your Shield this round, up to 3 Copper. Each enemy counts once. |
| NO087 | Field Lock | 1 Field Lock + conditional next-round Shield part | Shield | 2 Iron + 2 Copper | 2 | When this part activates at End Turn, gain 18 Shield. If a Fire in this activation round began with at least 8 Charge, schedule one new regular 12-Shield part to load at the beginning of the next round. Several qualifying shots do not add deliveries from this activation; Charge is checked at Fire, not at delivery. |
| NO088 | Three-Way Bridge | Immediate effect: Three-Way Bridge (no part) | Utility | 2 Copper + 1 Glass + 1 Circuit | 2 | Pay 6 Charge when activated. Remove 1 cooldown counter from every currently cooling recipe, or 2 from each if this payment left you at 0 Charge. |
| NO089 | Live Guard | Immediate effect: Live Guard Frame (no part) | Utility | 2 Iron + 1 Copper + 1 Circuit | 2 | For this fight, the first enemy attack against you each round triggers a separate 4-damage hit against that attacker after the attack, but only if you then have at least 6 Charge. Charge is not spent. Reusing the frame does not add triggers. |
| NO090 | Repeated Pulse | 1 Repeated Pulse Tip | Ammo | 1 Iron + 2 Copper + 1 Circuit | 2 | Add 10 damage. Add 3 more for each Repeated Pulse Tip consumed in an earlier main shot this fight, up to 15 extra damage. Tips in the same shot do not improve each other. |
| NO091 | Busy Circuit | 1 Busy Circuit Nose | Ammo | 1 Iron + 2 Copper + 1 Glass + 1 Circuit | 2 | Add 10 damage, plus 3 for each recipe in memory that has cooldown counters remaining at Fire, up to 18 extra damage. Recipes without cooldown do not count. |
| NO092 | Total Recall | Immediate effect: Recall Manifold (no part) | Utility | 2 Copper + 1 Glass + 1 Circuit | 2 | Pay 6 Charge when activated. Remove all cooldown counters from every currently cooling recipe. Gain 6 Shield once if this removes at least 2 counters in total. Does not reset recipes without cooldown. |
| NO093 | Delayed Strike | Immediate effect: Timed Contact (no part) | Utility | 2 Copper + 1 Glass + 1 Circuit | 2 | Pay 5 Charge when activated and choose one enemy. At the start of next turn, deal 18 damage and apply Weaken 6 to it if it is still alive. This is a support hit. |
| NO094 | Charge Receipt | Immediate effect: Charge Receipt Counter (no part) | Utility | 1 Iron + 2 Copper + 1 Circuit | 2 | **Superseded Utility dependency; revise before use.** For this fight, the first time each round you remove a recipe's last cooldown counter with a part, gain 2 Charge after that part finishes. Normal round progression does not trigger this. Reusing the counter does not add triggers. |
| NO095 | Barrel Reserve | 1 Barrel Reserve Ring | Modifier | 1 Iron + 2 Copper + 1 Circuit | 2 | When activated, pay 2 to 6 Charge. Add 4 damage to this round's main shot per Charge paid. If you paid exactly 6, also gain 6 Shield. |
| NO096 | Shield Collector | Immediate effect: Shield Collector Probe (no part) | Utility | 2 Copper + 1 Glass + 1 Circuit | 2 | Choose one enemy with Shield. Remove up to 12 of its Shield now, then gain 1 Charge for each full 3 Shield removed. Cannot activate without an enemy that has Shield. |
| NO097 | Precision Recall | Immediate effect: Precision Recall Plug (no part) | Utility | 1 Iron + 2 Copper + 1 Circuit | 2 | Pay 4 Charge when activated. Remove 2 cooldown counters from every currently cooling recipe, then apply Mark 6 to one chosen enemy. |
| NO098 | Field Reservoir | Immediate effect: Field Reservoir (no part) | Utility | 1 Iron + 2 Copper + 1 Circuit | 2 | Pay all your Charge, at least 4, when activated. At the start of next turn, gain 2 Shield per Charge paid after old Shield clears, and recover half the Charge paid, rounded down. |
| NO099 | Paired Protection | 1 Paired Protection Frame | Shield | 2 Iron + 1 Copper + 1 Glass + 1 Circuit | 2 | Gain 12 Shield. Choose up to two enemies showing attack intents; reduce each chosen enemy's next attack's total damage this round by 4, to a minimum of 0. |
| NO100 | Measured Arc | Immediate effect: Measured Arc Contact (no part) | Utility | 2 Copper + 1 Glass + 1 Circuit | 2 | Pay 5 Charge when activated. Deal 12 damage to one chosen enemy. If that hit causes it to lose HP, remove 1 cooldown counter from every currently cooling recipe. |
| NO101 | Last Copper | 1 Last Copper Tip | Ammo | 2 Iron + 1 Copper + 1 Circuit | 2 | Add 14 damage. Add 14 more if your raw resource pool contains no Copper at Fire. Copper already spent on other parts is not counted. |
| NO102 | Charged Cover | 1 Charged Cover Frame | Shield | 2 Iron + 2 Copper + 1 Circuit | 2 | Gain 16 Shield. If an enemy attack causes you to lose HP this round, gain 6 Charge after the first such attack. HP costs paid in planning do not trigger this. |
| NO103 | Sustained Contact | 1 Sustained Contact Pin | Ammo | 1 Iron + 2 Copper + 1 Glass + 1 Circuit | 2 | Add 12 damage. Before main damage, note the main target's Mark, up to 8. After impact, reapply that much Mark to it for a later main shot. |
| NO104 | Cool Workshop | Immediate effect: Workshop Relay (no part) | Utility | 1 Iron + 2 Copper + 1 Circuit | 3 | For this fight, once each round after activating a Shield part, you may remove 1 cooldown counter from every currently cooling recipe. Reusing the relay does not add triggers or reset this round's use. |
| NO105 | Ground Shield | 1 Ground Shield Frame | Shield | 2 Iron + 2 Copper | 2 | Pay 6 Charge when activated. Gain 28 Shield. If you have 0 Charge after this payment, also apply Weaken 4 to every living enemy. |
| NO106 | Shot Receipt | 1 Shot Receipt Contact | Ammo | 1 Iron + 2 Copper + 1 Circuit | 2 | Add 12 damage. At the start of next turn, recover the Charge paid by Charged Barrel for this shot, up to 3 Charge, and gain 1 Copper. Extra Shot Receipt Contacts do not repeat the refund. |
| NO107 | Shield Press | Immediate effect: Field Press (no part) | Utility | 2 Iron + 1 Copper + 1 Circuit | 2 | **Superseded Utility dependency; revise before use.** Pay 4 Charge when activated. At the start of next turn, gain 2 Stored Field Plates. Each is a Shield part giving 9 Shield when activated, and may be saved for a later round. |
| NO108 | Planned Discharge | Immediate effect: Discharge Timer (no part) | Utility | 1 Iron + 2 Copper + 1 Circuit | 2 | Pay 6 Charge when activated. At the start of next turn, grant 20 damage to your next main shot that round only. If you activate a Shield part that round, also gain 4 Charge after the first such activation. Extra timers refresh these effects without stacking them. |

### Legendary

| ID | Recipe | Output / effect label | Kind | Resources | Cooldown | Effect |
| --- | --- | --- | --- | --- | --- | --- |
| NO109 | Circuit Heart | Immediate effect: Circuit Heart (no part) | Utility | 1 Iron + 3 Copper + 1 Circuit | 3 | For this fight, after you craft a numeric-cooldown recipe for a second or later time in the same round, grant 4 damage to your next main shot that round only, up to 20 bonus damage granted per round. Unspent grants add together for the next shot; firing consumes those grants without resetting the round's grant limit. Reusing Circuit Heart does not add triggers or reset the limit. |
| NO110 | Reserve Coil | Immediate effect: Reserve Coil (no part) | Utility | 2 Iron + 2 Copper + 1 Circuit | 3 | For this fight, at Fire add 3 damage per Charge still held after the barrel's optional payment, up to 24 extra damage. Charge is not spent. This is counted once per main shot; reusing Reserve Coil does not stack it. |
| NO111 | Full Restart | Immediate effect: Restart Battery (no part) | Utility | 1 Iron + 2 Copper + 1 Glass + 1 Circuit | 3 | Gain 6 Charge and 16 Shield now. At the start of next turn, gain 6 Charge. The maximum Charge remains 12; excess gains are lost. |
| NO112 | Master Bridge | Immediate effect: Master Bridge (no part) | Utility | 2 Copper + 1 Glass + 2 Circuit | 3 | Pay 8 Charge when activated. Remove up to 3 cooldown counters from every currently cooling recipe. Does not reset recipes without cooldown. |
| NO113 | City Grid | Immediate effect: City Grid Frame (no part) | Utility | 2 Iron + 2 Copper + 1 Circuit | 3 | For this fight, at the start of each turn after the current turn, gain 2 Shield per Charge held before that turn's Charge gains, up to 16 Shield, after old Shield clears. Charge is not spent. Reusing the frame does not stack it. |
| NO114 | Open Circuit | Immediate effect: Open Circuit Board (no part) | Utility | 1 Iron + 2 Copper + 1 Glass + 2 Circuit | 4 | Pay 8 Charge when activated. Remove 1 cooldown counter from every currently cooling recipe. Crafting them again still costs their full resources. |
| NO115 | City Arc | 1 City Arc Ring | Modifier | 1 Iron + 2 Copper + 1 Glass + 1 Circuit | 3 | Pay 7 Charge when activated. This round's main shot deals 60% of its damage before target-specific Mark and defence to every other enemy, without copied payloads. Afterward, apply Weaken 3 to each surviving extra target that lost HP to this spread. Extra City Arc Rings do not repeat either effect. |
| NO116 | Empty the Field | 1 Field Release Collar | Modifier | 2 Iron + 2 Copper + 1 Circuit | 3 | When activated, choose and pay 10 to 20 Shield. Add 2 damage to this round's main shot per Shield paid, and gain 4 Charge after its impact. |
| NO117 | Field Shelter | 1 Field Shelter Wall | Shield | 3 Iron + 2 Copper + 1 Circuit | 3 | Gain 24 Shield. Until the end of this enemy phase, after each enemy's first attack against you, gain 4 Shield and 1 Charge. Each enemy can trigger this once; extra walls do not add triggers. |
| NO118 | Piercing Current | 1 Piercing Current Lens | Modifier | 1 Iron + 2 Copper + 1 Glass + 1 Circuit | 3 | Pay 8 Charge when activated. Add 12 damage to this round's main shot, and its damage to the main target bypasses Shield. Damage to other targets and support hits still meet Shield normally. |
| NO119 | Total Discharge | Immediate effect: Total Discharge Contact (no part) | Utility | 1 Iron + 2 Copper + 1 Glass + 1 Circuit | 3 | Pay all your Charge, at least 6, when activated. Deal 2 damage per Charge paid to every living enemy now. These are support hits and do not consume the round's main shot. |
| NO120 | Pocket Generator | Immediate effect: Pocket Generator (no part) | Utility | 2 Iron + 2 Copper + 1 Glass + 1 Circuit | 4 | **Superseded Utility dependency; revise before use.** At the start of next turn, gain 2 Pulse Tips and 2 Charged Plates. Each Pulse Tip is Ammo adding 10 damage and giving 1 Charge after impact. Each Charged Plate is a Shield part giving 10 Shield. These parts may be saved or used together and clear at fight end. |

## Reference and design notes

### Reference used

The [Slay the Spire 2 Cards List](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Cards_List) showed **595 entries** when rechecked in its rendered browser page on 14 September 2026; its footer dated the last edit to 24 August. The count includes statuses, curses, quests, generated and multiplayer cards, not just normal rewards. This catalogue now offers **606 craftable recipes** across four characters and a shared pool. It does not reproduce the source list entry for entry. The database is a community reference; its figures are not our balance specification.

| Reference examples | Strategic question translated into parts |
| --- | --- |
| Bash; Body Slam; Rampage | Expose a target, spend protection for damage, or invest in repeat use. |
| Poisoned Stab; Blur | Mix an immediate hit with delayed harm, or protect a later turn. |
| Bodyguard; Unleash | Invest in a helper and decide when its health should protect or attack. |
| Refine Blade | Build future weapon strength instead of taking only immediate damage. |
| Zap; Dualcast | Keep a stored source of power or release it for a larger effect. |

The [earlier project study](BUILD-STRATEGY-STUDY.md#what-the-reference-study-actually-covered) records broader source coverage and differences from the installed game. Here the names, prose, material costs, cooldowns and recipe combinations are original proposals for the mercenary/robot setting. Drawing and discarding were translated into part storage, sacrifice, reuse and planned material supply. No hidden deck, multiplayer system or source artwork is introduced.

### Review evidence

The original **600 rows** received a second rules review beyond their author's pass. Integration corrected kill-trigger timing, separate payments across saved helper parts, Shield retention and interception, copied-part identity, next-round magnet timing, and several recipes that were chiefly stronger versions of existing functions. Automated document checks verify all IDs, five pool sizes, rarity totals, required fields, material names, positive costs, cooldown values and recipe references. These are document and arithmetic checks, not game tests.

The six later manual-grab additions received a scoped author consistency review for next-round timing, automatic/missed handling, spent attempts, duplicate enhancements, multiplier exclusions, generated-part use and fight-end clearing. Structural checks cover all 606 entries, their names, costs, cooldowns, IDs and rarity totals. The additions have not received an independent second review or gameplay testing. Their worked bonus example checks arithmetic only.

Worked calculations corrected for zero base gun damage on 15 September, under the remaining draft part arithmetic and with no other effects or enemy protection:

- A Plain Slug and a saved Settled Charge supply 6 + 15 = 21 damage. Layered Charge sees two Ammo parts, so its combined bonus is 16%: `(21 * 16) // 100 = 3` whole bonus damage, giving 24 main damage. Wide Burst gives each extra enemy `(24 * 50) // 100 = 12` whole damage. Its explicit draft effect does not repeat the main target's payloads.
- With Bolt at 8 HP, two loaded Fist Launchers supply 30 + 10 = 40 main damage: the first pays 7 HP, leaving the second unable to pay. There is no gun damage term; Bolt remains at 1 HP.
- Noor uses a Wound Cell for 3 Charge and fires one Conducting Pin. The part supplies 6 main damage and restores 1 Charge after impact. The old example's +4 gun damage is removed, and its +6 Charged Barrel trait grant is superseded pending a design consistent with parts-only bullet damage. Do not spend the 3 Charge through that unapproved trait to recreate the old damage total.

**Global-cooling revision, 14 September:** revised 34 cooling-effect rows to remove manual recipe selection, recipe-type restrictions and self-exclusions, including delayed and attack-triggered cooling. Costs, cooldown numbers and non-cooling payments remain the printed draft values. Conditional rewards remain once per effect; effects that formerly required two cooled targets now check whether at least two recipes become ready. Full-reset variants remain full resets across active cooling, with balance untested. These changes create identical effect text in SH068/SH100 and SH099/SH116, and additional overlaps across differently priced recipes; they are explicit tuning issues, not evidence that each remains a distinct worthwhile offer. The later owner correction makes cooldown 2 used in round 4 ready in round 7; the earlier same-round first tick is superseded.

### What still needs balancing

The document specifies what each proposed recipe does; it does not establish that the values are fair or fun. The five-material haul must support a useful attack and defence choice every round, including the starting recipes. More efficient rare recipes must still leave a reason to keep a dependable basic recipe with no cooldown. Resource bias should help a build without guaranteeing every desired combination.

In later paper tests and a playable prototype, check these concrete cases:

- Can every starting kit produce useful attack and defence from several different opening hauls? Can Ada restore Bolt after it is disabled? Can Noor build Charge before expensive discharges become worthwhile?
- Does a saved multiplier combined with a spread part create the stronger multi-enemy shot the owner described? Does the shot remain readable with 20 or more parts, without accidentally firing every payload several times?
- Does using material for cooling beat crafting a different ready recipe only in useful situations? Do finite positive costs still prevent a closed loop when several cooling, copying and delayed-refund recipes meet?
- Do Heat and HP costs make Mara's aggressive route meaningfully different from Ivo's corrosion and target planning? Is withholding an attack to save parts sometimes useful without making stalling the best strategy?
- Does preparing the one manual grab change when it is used? Does the reward justify materials and a memory slot without making a Perfect result compulsory? Check all six precision enhancements together for an excessive once-per-fight resource burst, then check their much smaller automatic fallback.
- Can players understand the final damage, Shield and delayed effects before committing? Which low-rarity recipes are consistently ignored, and which rare recipes dominate unrelated builds?

The interface must support at least 20 parts in each bullet and shield and scale to larger resource-permitted builds. There is no fixed part-count cap on either build. Starting recipe-memory capacity is 20, with balancing after beta testing still to come. The initial starter set is 12 recipes (8 shared, 4 character-specific), also subject to balancing through beta runs. Exact haul composition, remaining Load controls, shot-versus-round effect behaviour and non-Ammo activation details remain owner decisions or later balance questions. Fight-start cooldown reset is selected: all recipe counters start at zero. None is silently locked by a recipe count.

### What comes next

The character sections give a few permanent-upgrade directions solely to explain their identities. **The permanent-upgrade catalogue, complete buff/debuff catalogue, enemy modifiers and the remaining asset sets are separate later work.** The small effect definitions here make these recipes readable; they are not that full modifier catalogue. No shop prices, graphics, game implementation or runtime balance claim is included.
