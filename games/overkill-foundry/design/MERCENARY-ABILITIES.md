# Inherent mercenary abilities

**20 September beta tuning:** [BETA-TUNING.md](BETA-TUNING.md) adopts Hot Barrel, Find the Seam, Field Service and Charged Barrel with the printed numerical defaults for the first test profile; all four mercenaries start at 80/80 HP. This is the owner-authorized assistant tuning pass, not proof of equal strength. Quench Recovery and Residual Current remain unselected alternatives. The dated review below preserves why those alternatives were separated.

**18 September 2026 · earlier character effects restored; alternatives and beta values remain proposals**

Klaus requests an inherent ability for every mercenary, comparable in purpose to a character's starting passive but original to our crafting game. The requirement for distinct inherent abilities is selected. The earlier Hot Barrel and Charged Barrel designs are retained; their draft values have not been balanced. Quench Recovery and Residual Current remain unselected alternatives, not replacements or automatic additional traits. They build on the existing Heat, targeting, Bolt and Charge identities rather than adding four unrelated systems.

Each mercenary has their ability from campaign start. It occupies no recipe-memory slot, needs no purchase or Mayor selection, and cannot be lost by replacing recipes. It is one intrinsic ability, not another copy of a recipe or an additional item to equip. Proposed character meters/helper conventions remain as described in the [recipe catalogue](RECIPE-CATALOGUE.md).

| Mercenary | Inherent ability | Proposed tooltip | Play style it supports |
| --- | --- | --- | --- |
| **Mara** | **Hot Barrel — retained draft** | At Fire, add 1 damage per full 2 Heat to the main shot, once per shot. Heat is not spent. | Keep Heat for stronger shots or spend it on other effects. |
| **Ivo** | **Find the Seam** | Once per turn, your first successful Acid Etch application also applies 2 Target Paint to one affected enemy. | Corrode a priority target, then exploit it with a later assembled shot. Refines the existing draft trait. |
| **Ada** | **Field Service** | Once per turn, after a crafted Helper command deals direct damage to an enemy, repair active Bolt for 2 HP. | Use Bolt's paid commands to maintain him, then decide how much protection or other work to ask of him. |
| **Noor** | **Charged Barrel — retained draft** | At Fire, optionally pay up to 3 Charge for +2 damage per Charge to that main shot, before its other Charge checks. | Spend electricity for immediate damage or keep it for later effects. |

Character effects can override base rules within their explicitly described scope. Ordinary base gun damage stays zero; Hot Barrel and Charged Barrel add character bonuses to valid part-built shots and do not grant free attacks. Do not automatically combine the retained traits with their alternatives. Ivo modifies the existing target-status system after a recipe applies Acid Etch; the status helps a subsequent part-built shot, never retroactively increases the hit that applied it. Installed Shield protects automatically; none of these listed traits changes its reset. No trait refreshes a used no-cooldown recipe.

## Review of the four earlier character definitions — 18 September

Compared the archived recipe catalogue with the ability-replacement change at `5aff12e` and the current definitions. Exactly two character damage bonuses were invalidated by the assistant's interpretation of the zero-base-damage rule.

| Character | Finding | Correction |
| --- | --- | --- |
| Mara | Hot Barrel was marked superseded and displaced by Quench Recovery. | Restore +1 main-shot damage per full 2 Heat, once at Fire; keep Quench Recovery as an unselected alternative. |
| Noor | Charged Barrel was marked superseded and displaced by Residual Current. | Restore the optional 0–3 Charge payment at Fire for +2 damage each, before other Charge checks; restore all thirteen dependent recipes. |
| Ivo | Find the Seam retained its first Corrosion application → Mark 2 effect, with alias and targeting/eligibility refinements. | No removed ability found; preserve it. |
| Ada | The Bolt character-feature paragraph is unchanged, including 6/12 starting HP and paid Helper commands. Field Service was added as a proposal. | No removed character feature found; preserve Bolt and keep Field Service's draft status. |

Hot Barrel does not spend Heat. Charged Barrel pays once per main shot; support hits do not retrigger it. Heat/Charge gained after impact cannot boost or fund the shot that generated them. The Heat cap of 10, Charge cap of 12 and original decay/carryover conventions remain draft tuning. Replacement alternatives are not combined with retained traits without an explicit choice.

## Mara — Quench Recovery

**Unselected alternative to Hot Barrel.**

An actual positive Heat payment by a successfully used recipe or activated part qualifies. Ordinary end-of-round Heat decay, a failed use, a zero-cost payment or merely gaining Heat does not. Several Heat payments in one action still produce at most one trigger; the amount of Heat paid does not multiply the healing.

The turn's trigger is consumed even at full HP. The fight limit counts **HP actually restored**, so overhealing wastes that turn's opportunity without reducing the remaining six-HP allowance. Healing cannot exceed maximum HP. Show the remaining fight allowance beside the ability.

Resolve healing after the triggering activation and its immediate enemy reactions, while Mara is alive. For Shield parts, the secondary Heat-payment timing still needs reconciliation with the installed-Shield rule; do not invent an activation step for protection. Healing cannot pay an up-front HP cost or rescue a player already killed by recoil. If an action defeats the final robot, resolve its already-earned healing while Mara is alive before clearing fight state; this is a triggered ability, not an automatic heal at every victory.

**Starter example:** at 70/80 HP, use MA001 Fuel Brick to gain 3 Heat, then legally use MA004 Quick Vent and pay its 2 Heat. Quench Recovery restores 2 HP to reach 72/80 and leaves four HP of healing allowance for later turns. Quick Vent installs its ordinary Shield grant under the selected rule. This example illustrates the alternative healing trigger only; it is not an additional benefit on top of Hot Barrel.

This does not replace **Hot Barrel** unless the owner chooses that alternative. Heat still starts at 0, caps at 10 and loses 2 after the enemy phase under the retained meter draft. There is no automatic heal on fight entry, at End Turn without a qualifying payment, or at a city transition. The [proposed 80-HP test](BALANCE-RESEARCH.md#hp-and-enemy-benchmarks-from-sts2--16-september-2026) must record which trait is under test; include actual healing only when testing this alternative.

## Ivo — Find the Seam

Keep the existing trait's identity, with **Acid Etch** and **Target Paint** as the proposed aliases for Corrosion and Mark. The trigger requires adding at least one Acid Etch to a living robot; a Packet Filter-negated application does not qualify. When one application affects several robots, choose one valid recipient for the extra Target Paint as part of that action's targeting. Only one receives the trait benefit.

Apply Target Paint after the successful Acid Etch application. It obeys ordinary target-status protections and consumption rules. If its chosen target cannot receive the application, it does not jump to another robot. An attempted trait application consumes the turn's trait trigger even if the extra Target Paint is itself prevented. Tick damage from existing Acid Etch is not a new application.

**Starter example:** IV001 Acid Mix's shot resolves its damage, then applies Acid Etch 2 to the surviving target. Find the Seam adds Target Paint 2. A later main shot against that target consumes the Paint under its normal rule. The first shot receives no retroactive bonus, and support hits do not consume it as if they were main shots.

Ivo gains no new meter, automatic extra shot or additional innate damage per Fire. This is his one inherent trait; do not stack an old and renamed copy of Find the Seam.

## Ada — Field Service

A crafted Helper command must produce a direct support hit that removes enemy Shield or HP. A blocked-to-zero hit, a status tick, mere command crafting, interception, repair or an Ammo main shot does not qualify. A command with several hits or targets can trigger the ability only once. Normal requirements to have an active Bolt and consume/pay for the command remain intact.

After that command and immediate enemy reactions resolve, repair up to 2 missing Bolt HP if Ada is alive and Bolt remains active. Consume the turn's opportunity even if Bolt is at full HP. This is **repair**, not restore: it cannot bring Bolt back from 0 HP. It is an Ada passive, not an autonomous Bolt turn, and it grants no free command or interception allowance.

**Starter example:** Bolt begins at the drafted 6/12 HP. Craft and activate AD002 Tap Command for its ordinary cost. If its support hit deals positive damage and the participants remain eligible, Field Service repairs Bolt to 8/12. AD003 Steady Brace is still needed to enable interception; the repaired HP alone does not absorb an enemy attack. Qualifying Reactive Mesh damage still returns to Ada, not Bolt, and resolves before Field Service.

Retain Bolt's existing helper feature as the basis for Ada's recipes: one helper, no autonomous attacks, active/disabled states, and the existing repair/restore distinction. Field Service is the added starting passive layered onto that feature.

## Noor — Residual Current

**Unselected alternative to Charged Barrel.**

One Utility must actually reduce at least one recipe's cooldown counter. Merely using a cooling effect when nothing is cooling does not qualify. Removing counters from several recipes, or removing several counters, still grants only 1 Charge. A non-Utility cooling source does not qualify for this proposed trait.

Pay the Utility's complete costs before resolving cooling and granting Charge. The Charge arrives afterward and cannot fund its own triggering use. Consume the turn's opportunity even if Charge is already at its cap of 12; overflow is lost. The trait's availability is not itself a recipe cooldown and cannot be refreshed by cooling.

**Starter example:** at 3 Charge, use NO004 Quick Bridge for 1 Copper and 2 Charge while a recipe is cooling. It leaves 1 Charge after payment; if a counter is removed, Residual Current brings Noor to 2 Charge. Without a cooling counter there is no gain. A subsequent qualifying Utility in the same turn grants no further trait Charge. At entry, SH005 and SH008 are the starter cooling targets; no-cooldown recipes remain once per turn unless an explicit permanent upgrade changes their uses.

This does not replace **Charged Barrel** unless the owner chooses that alternative. Charge starts at 0, caps at 12, and carries between turns without ordinary decay under the retained meter draft. The retained Charged Barrel has its own optional payment at Fire. Do not silently remove it, merge it into recipe/part spending, or combine both alternatives.

## Shared trigger boundaries and validation

For the abilities/alternatives that specify once per turn, reset those allowances at the start of the player's next turn, including the first turn of a fight. New shots, recipe cooling, shops and Load/unload do not reset them. Hot Barrel and Charged Barrel instead use their stated once-per-main-shot timing. A trait does not trigger itself recursively. The Quench Recovery alternative's six-HP budget resets only for a new fight; Continue restores the original fight-entry state, including the original trait allowances and seed.

Names and values are original proposals; source inspiration is the function of a character-specific starting passive. No exact source relic effect is adopted as an automatic rule. All thirteen Charged Barrel-dependent interactions retain their restored meaning. The later first-installation clarification makes NO058's Shield calculation explicit while preserving its exclusion of the barrel payment; it does not replace that character effect. Recipe costs and printed enemy values are preserved. The paper examples check trigger eligibility and simple whole-number outcomes only. There is no runtime, automatic balance simulation or evidence that these four benefits are equally strong.

For the first playable, record actual HP recovered, Target Paint applied/used, Bolt HP repaired, Charge gained/spent, zero-value triggers and the choices changed by each ability. Include full-HP/full-meter cases, blocked status/hits, disabled Bolt, repeated shots/cooling and lethal recoil. Compare otherwise matching fights with and without the trait to see whether it changes decisions rather than merely inflating output.
