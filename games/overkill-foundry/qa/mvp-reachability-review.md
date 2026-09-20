# Mara/Cinderwall reachability review

20 September 2026. Read-only source audit by the core worker, requested after the independent encounter/lifecycle and Precision reviews. This adds no passing gameplay fixture, production change or runtime-support binding. The earlier frozen reviews and their residual labels remain unchanged.

The three missing branches are unreachable through the currently enabled production Mara/Cinderwall campaign. This conclusion is bounded by the exact 28 inputs in [the evidence record](mvp-reachability-evidence.json), rather than an unsuccessful search for a seed.

| Missing branch | Result in a valid new Mara/Cinderwall run | Reason |
| --- | --- | --- |
| SH064 prevents player Corrosion, Mark or Weaken | Unreachable | No enabled action can apply these three statuses to the player. |
| UGS-029 rejects a fourth distinct enemy debuff in one fight | Unreachable | The whole city has three producer types; each legal formation can produce at most **one** distinct type. |
| UGS-029 excludes a self-applied debuff | Unreachable | No enabled recipe, upgrade, collection, shop or route action applies a player debuff to its user. HP/Heat/material payments are separate events. |
| MAU-05 consumes its opportunity while already at full HP **after** a qualifying HP payment and its preceding reactions | Unreachable | Every qualifying Use creates at least two missing HP; no applicable intervening effect can heal it before Suture. |

Zero Suture healing **after its six-actual-HP fight allowance is exhausted** is a different, reachable condition. Starting a Use at full HP is also reachable: the payment first creates a deficit, so it does not exercise the missing post-payment full-HP branch. Neither distinction is grounds to label the missing branch tested.

## Scope and identity

The starting point is `CampaignRules::newGame` with the production `Rules::completeContent`, `cinderwallUpgradeHooks`, generated city content and legal campaign actions. Continue restores an actual entry produced by those transitions. The manifest enables 246 recipes, 152 upgrades/Mayor gifts, 10 robots and 10 exact formations. Arbitrarily edited `State`, fabricated upgrade events, custom `Rules` definitions, callback injection, controlled multi-robot test formations and foreign/legacy saves are outside this reachability claim.

The LF-normalized input graph is `0cf7ab60a60c203661ce3fddeed6a0392680790a182c1d6c17e953c271627531`. It describes the current **provisional** core, including the terminal correction, Shared schedule repairs, Jig-copy attribution and MA082 cap. It is not the separate explicit-1843 graphical core or any earlier frozen review graph. `RulesVersion` is still `of-core-0.4`; the owner save/version decision remains pending. No version, schema, held haul composition or source decision is changed here.

## Status producer closure

The source clauses are SH064 in [RECIPE-CATALOGUE](../design/RECIPE-CATALOGUE.md), UGS-029 in [shared-upgrades.json](../design/data/shared-upgrades.json), the C1 roster, and the player mirrors in [ROBOT-EFFECTS](../design/ROBOT-EFFECTS.md). A status definition is not itself an application source.

`State` initializes all six player counters to zero. `CampaignRules::newGame` does not replace those defaults. `clearFight` creates a clean state, preserving HP, memory and upgrades but not player debuffs. The only positive writes to player Burn/Corrosion/Mark/Weaken/Fouling/Leak are in `Engine::callbacks().playerStatus` (`core.cpp:74–78`). The only production callers of that callback are these three branches in `robots.cpp:212–214`:

| Robot / committed action | Player application | Legal formation |
| --- | --- | --- |
| C1-R04 Cable Binder / `foul` | Recipe Fouling 1 | C1-F-BINDER |
| C1-R08 Knuckle Press / `double_punch` | Shield Leak 2 | C1-F-PRESS |
| C1-O03 Split Chassis / `thermal_runaway` | Burn 2 | C1-F-CHASSIS |

The factory's other seven robot definitions add no player status. The ten formation bodies match the manifest; the only mixed initial formation is Mite + Ram. Nest and Chassis can create only a Rivet Mite, which cannot apply any status. Patrol Mysteries draw from the same regular formation pool. Route replacement does not create custom mixtures. Boss recovery changes an enemy intent and Drive, not player status. Removing a robot, escaping, or surviving another packet cannot add a new producer type.

The complete recipe/upgrade status path `Engine::status` selects only `s.enemies`; its pointer-based write is to an `Enemy`, never `State`. Burn opcodes also select an enemy. Player-side recipe writes are cleansing only: SH022/033 reduce Burn, SH033/063 reduce Corrosion, and SH073 reduces Mark/Weaken. Decay, consumption and fight cleanup cannot create a positive absent status. Serialization copies counters but supplies no legal acquisition path to arbitrary injected values.

Consequently player Corrosion, Mark and Weaken stay zero by induction from New Game. SH064's other three prevention branches have no legal application to intercept. Its Burn branch and its non-prevention of Fouling/Leak remain meaningful MVP behaviors, already handled by separate finite fixtures; this audit does not re-execute them.

UGS-029 is called only by that enemy callback when the previous amount was zero. Its `seen` list persists within a fight and is cleared at real fight entry (`startUpgrades`); it is not a campaign-wide union. Even the second distinct application type is unavailable in a natural enabled fight. The existing controlled three-type tests demonstrate executor behavior, not an earned three-type encounter. All player recipe costs, collection drawbacks, Heat loss and sacrifices were checked separately: none enters `playerStatus` or writes a typed debuff. Choosing a risky action and then taking enemy Burn still attributes the application to the enemy; it is not a self-applied exclusion case.

## Suture payment and healing closure

MAU-05's source clause and MY1-M3's possible competing Heat heal are in [character-mayor-upgrades.json](../design/data/character-mayor-upgrades.json). The selected timing contract requires real payment, surviving reactions and ordered triggers; the upgrade system distinguishes HP payment from enemy damage and preserves the missing-HP deficit when maximum HP increases.

The entire enabled Shared/Mara catalogue has four positive HP payments on recipe **Use**:

| Recipe | HP paid | Immediate relevant effect | Heat actually paid by this Use |
| --- | ---: | --- | ---: |
| MA019 Blood Fuel | 3 | Gain 5 Heat | 0 |
| MA046 Coal Debt | 2 | Gain 2 Heat; next-turn Carbon | 0 |
| MA092 Pressure Debt | 5 | Gain 10 Heat; next-turn Shield | 0 |
| MA116 Last Reserve | 6 | Gain 6 Heat, Shield and next-shot damage | 0 |

These rows agree with the primitive effects and `catalogueUtility`. `payment` removes HP first, requires at least one HP remain, and records whether it occurred during recipe Use. A Craft action cannot include another recipe Use, Fire, End Turn, purchase or reward acceptance. Each of the four Uses produces one positive HP payment record and no Heat-payment record. Material discounts cannot waive or convert it. Automatic plain Shield grants do not install an HP-cost Red Grate or activate another recipe.

SH050's four HP are paid when its physical Modifier is activated; MA054's three HP are paid at first installation. Neither is an HP payment during recipe Use in this implementation. They cannot be combined into a concurrent qualifying Utility payment through Load or Action fields. The enabled Fire effects have no HP-payment opcode. This is the current paid-Use boundary, not a claim about unspecified future recipes.

The payment queue is local to one Engine/action. `resolvedUseUpgrades` resolves applicable upgrades in acquisition order and drains that action's payment records; `finishPaidUpgrades` closes the action before control returns. No payment/reaction cursor exposes an intervening shop or healing action. Pending upgrade choices block ordinary combat actions. The following exhausts the current player-HP restoration paths and challenges the tempting alternatives:

| Healing path | Why it cannot restore the qualifying deficit before Suture |
| --- | --- |
| SH074 / SH104; extra UGS-106 recipe healing | Separate healing recipe Uses with no HP cost. HP-cost Utilities do not call the recipe-heal hook or invoke another recipe. |
| MY1-M3 cumulative 6 Heat payment | Only advances/heals on a positive **actual Heat payment**. The four qualifying Uses gain Heat and pay zero Heat. Earlier action payments have already resolved; decay and Heat gain are not payment. |
| UGS-014 next-turn recovery; UGS-013/090 entry healing | Complete at their turn/fight boundary before the later HP-cost Use, not during it. |
| UGS-036/065/074 victory healing | Campaign completion runs only after `Rules::apply` has finished the Use and Suture/reactions. A killing UGS-066 pulse cannot move victory healing ahead of Suture. Terminal defeat blocks completion healing. |
| UGS-016 acceptance, UGS-073 purchase, UGS-097 noncombat healing | Distinct campaign actions. A combat shop can be used between actions, not inside an unresolved HP-cost Use. |
| Maximum-HP grants (including UGS-029/033/129) | Add the same amount to max/current HP, preserving the deficit; their trigger clocks also do not introduce healing inside these Uses. |
| Hit/kill/payment reactions | `paidBeforeEffect`, `producedUpgrades`, `resolvedUseUpgrades`, `playerLostUpgrades`, `enemyHitUpgrades` and sacrifice reactions grant Shield/materials/bonuses, apply damage, or schedule later recovery. UGS-066 may inflict recoil before Suture depending on acquisition order; it can increase the deficit or cause terminal death, not remove it. |
| Reserve Heart Pump | UGS-067 is city 2–3 and absent from the enabled 152. Its legacy state flag starts false and has no enabled setter. Even the controlled quarter-max rescue is not a full-HP restoration. |
| Continue, nested choices, duplicate acquisitions | Continue restores an earlier complete entry, not a half-paid action. No applicable choice interleaves healing. Named upgrades are acquired once; a second Suture cannot heal before the first. |

Let `D = maxHp − hp` immediately before the qualifying payment. Legal state has `D ≥ 0`; after payment `D ≥ 2`. Every applicable path before the first Suture payload preserves or increases that deficit, or terminates the action in defeat. If alive and Suture's opportunity is unspent, its requested healing is `min(2, 6 − healedThisFight)`. Thus full HP cannot be the reason for zero healing at that checkpoint. In this current action set the first three successful qualifying opportunities heal two each, and subsequent qualifying opportunities may heal zero because the six-HP allowance is exhausted. A later action that starts at full HP pays again before its own check.

## Reopening conditions and limits

Reopen the status conclusions when enabling another mercenary/city, a mixed formation containing different debuff producers, a non-Mite summon, a self-debuff/transfer/reflection effect, persistent cross-fight status, or a new callback/write path. Reopen Suture when a qualifying Use can also heal or actually pay Heat, an immediate damage/kill reaction heals the player, another effect invokes a nested recipe, payment queues span player actions, rescue restores full HP, duplicate Suture ownership becomes legal, or trigger ordering changes.

No new executable was built or run for this source-only task, and no native test count is claimed. The evidence records exact inputs, source inventories and the reasoning obligations; it is neither a model-checker proof nor future-content coverage. The conclusion permits an explicit **MVP-inapplicable branch** note in a later reviewed linkage decision; it does not create a passing fixture or automatically complete any full catalogue binding. The earlier full-contract residual labels, held balance decisions and save migration question remain intact.
