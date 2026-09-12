# Magnet Sweep — salvage career brief

Revised 2026-09-12 after Klaus rejected the first playable demo as boring and explicitly requested goals, challenge, risk, satisfying attraction and melting, better presentation, music, upgrades, leveling and loot. This brief describes the implemented rework. The unlimited, risk-free demo and its earlier design at Git revision `8af011f` are rejected historical alternatives, not governing policy.

Development timeline is not a consideration. The objective is a coherent, simple game with consequential choices and rewarding play. Compilation, agent agreement and screenshots do not establish that this rebuild meets that objective. The first demo received a negative human playtest; the rework still needs player observations.

## Player promise and complete loop

**Pull a valuable haul out of tangled scrap, manage the magnet's load, melt it for a real payout and build a more capable salvage rig.**

1. Take an unlocked contract with a visible salvage-value quota, optional gold target and **four fuel charges**. Each smelt or failed-risk emergency quench uses one charge. There is no overall countdown.
2. Energize the magnet. Nearby pieces accelerate toward it, and visibly connected bundles move together. Choose an approach and use the narrow field to separate attractive salvage from nearby hazards.
3. Decide whether to smelt the current load, attempt a richer bundle or drop the haul for recovery. Capacity and a visible instability fuse make the decision consequential.
4. Deliberately smelt stable cargo. A whole load pours into the furnace, produces ingots and pays spendable credits plus permanent XP. Crossing the contract or gold threshold pays its bonus once per attempt.
5. Finish after meeting the quota, or spend remaining batches pursuing gold and a rare core. Buy a visible rig improvement, unlock another contract and replay familiar situations with earned power.

The desired player effect is anticipation followed by a physical and economic payoff. An upgrade menu cannot compensate for weak attraction, indistinct impacts or an uninteresting batch decision.

## Guided demo: payout and purpose first

The owner rejected the first guided demo after actual play. It requested iron but secretly blocked smelting behind release/bundle lessons, exposed overload before a understood reward and failed to communicate why to collect anything. That feedback supersedes the earlier agent-led successful path. The corrected first objective is **earn 150 credits and fit a permanent improvement to this magnet**. Longer goals use the existing rig's nine improvements, six orders/gold targets and six banked collectible cores; no larger building, lore or new crafting economy is implied.

**Any ordinary metal mix counts.** Iron, copper, alloy and cores all have smelt value. Composition itself never fails an order. Red cells are hazards, not another requested material. A real **SMELT HAUL +N cr** button remains beside the furnace and reports the real refusal reason if empty/unsafe/ended. Clicking the furnace, pressing E, or releasing a tray-origin drag over the furnace/button reaches the same rule-valid deposit. Guidance never blocks a safe nonempty deposit because a lesson is incomplete. Pickup feedback means carried value; only the furnace pays banked credits and XP.

A labelled **training guard** prevents capturing red cells or exceeding current safe capacity until the first actual upgrade purchase. First useful capture exposes smelting without stopping attraction immediately. The first sweep gently stops at 36 carried value or safe capacity and points to the furnace; this is assistance, never a minimum-payout requirement. Earlier iron-only or mixed deposits must pay normally. Release keeps cargo; right-click returns the entire haul recoverably to the tray and never masquerades as a furnace deposit.

The sequence is collect → smelt → fund the first upgrade (120 order value plus 80 bonus readily affords it) → purchase and use the improved rig → risk rescue → precision. Links are explained by actual group mass/value tooltips rather than a mandatory collection gate. The main HUD prioritizes the first upgrade after quota instead of competing with optional gold. A purchase explicitly announces the end of training guard. The first unpractised unsafe pickup pauses its fuse before loss, and RMB remains usable for the actual rescue. Later warnings run under normal consequences. Skipping explicitly removes guidance/guard; skipping a held warning drops the real haul first.

Lesson state and actual progress are saved. Existing numeric lesson IDs remain compatible; obsolete Release/Bundle paths reconcile toward a payable haul rather than trap the owner. Actual already-banked value reconciles interrupted pours. Retry retains money, XP, installed upgrades and collection. Pause retains separate saved practice and Return to career. The owner's original 0.3 tutorial save is backed up and preserved; corrected 0.4 verification uses disposable profiles and a copy of that exact stuck state.

## Input and physical response

| Input | Implemented action |
| --- | --- |
| Move mouse | Move the magnet toward the pointer with responsive bounded motion |
| Hold left mouse over the tray | Attract metal; release on the tray retains cargo; release over the furnace/smelt button banks a safe haul |
| Space | Toggle the attraction field as an alternative to holding |
| Hold Shift / toggle Q | Narrow the field for precise approaches |
| Right mouse | Drop the entire haul recoverably and switch the field off |
| Click SMELT HAUL / furnace, or press E | Smelt any safe mixed cargo, independent of lesson order |
| Tab | Open or close the workshop |
| Escape / R | Pause or close an overlay / request confirmed retry |
| M / F11 | Toggle audio mute / toggle fullscreen |

Visible help must explain both hold and toggle controls. Pause, focus loss and menus suspend active play; entering pause cancels active attraction. Resuming an unsafe saved load starts paused. Toggle accessibility and focus behavior require native testing, not just implemented bindings.

Attraction is continuous movement with acceleration, momentum and attachment, rather than instant point collection. A connected component moves as a bundle and captures atomically with its combined mass and value. There is no ring-drag corridor mode or hidden recursive cascade rule. A group which would take the rig above **150% of its safe capacity** remains available instead of being partially credited. Links must visibly correspond to the group that actually moves.

## Materials, capacity and risk

| Material | Mass | Smelt value |
| --- | ---: | ---: |
| Iron | 2 kg | 4 credits |
| Copper | 3 kg | 12 credits |
| Alloy | 4 kg | 24 credits |
| Rare core | 4 kg | 40 credits |
| Hot cell | 4 kg | 0; hazardous while carried |

Safe capacity starts at **24 kg**; the initial hard capture ceiling is **36 kg**. Over-capacity cargo or any carried hot cell starts a **three-second instability fuse**. Releasing the field does not cancel it. The warning forecasts the exact highest-value salvage piece and credits that will be lost, with stable piece ID breaking ties.

Right mouse drops the **entire unbanked haul** onto the tray and switches attraction off. This rescue costs no fuel or salvage, but the player must rebuild the load; it does not automatically retain the best pieces. Nearby, separated spill placement must allow precise recovery even at tray edges. If the fuse expires, an emergency quench consumes **one of the four fuel charges**, its forecast highest-value salvage piece is destroyed, carried hot cells are removed and all other cargo spills recoverably onto the tray. Even a lone hot cell costs a fuel charge on failure. The warning must show both costs. Banked credits, XP, collection and upgrades remain safe. There is no separate integrity meter or permanent equipment damage.

The furnace refuses an overloaded or hot-cell load and explains how to drop it. Each accepted nonempty smelt spends exactly one fuel charge, regardless of load size; its hover preview must show that cost and the expected payout. Exhausting the four charges below quota fails the contract. An unreachable remaining quota also ends the attempt honestly. A later mistake cannot retract an already-earned completion or bonus. One quench leaves a three-smelt recovery route on the opening contract.

## Contracts and rewards

These are current tuning values, not established difficulty or replay evidence. A quota measures banked salvage value, excluding bonus cash. Each job includes one optional named core; no required rare item can soft-lock success.

| Contract | Quota / gold target | Completion / gold bonus | Required level |
| --- | ---: | ---: | ---: |
| First Pour | 120 / 180 | 80 / 40 | 1 |
| Copper Knot | 180 / 260 | 120 / 60 | 2 |
| Live Wire | 240 / 360 | 160 / 80 | 3 |
| Heavy Freight | 300 / 440 | 200 / 100 | 4 |
| Split Seam | 380 / 540 | 240 / 120 | 4 |
| Furnace Crown | 460 / 660 | 280 / 140 | 4 |

First Pour has an approachable three-batch route: two ordinary 24 kg iron loads earn 48 each, then a 9 kg copper bundle earns 36. The resulting 132 salvage value plus the 80 bonus affords one 150-credit mod. More valuable, deliberate hauls can finish sooner or pursue gold. This is a verified model route; physical readability and enjoyment still require play.

At quota, the player can finish or continue for gold, surplus earnings and the core. Finishing with cargo attempts an explicit stable smelt while a batch remains. Abandoning an attempt is distinct from finishing: its unbanked cargo is forfeited, while already banked earnings persist. Retry restocks the same arrangement and preserves career progress. Legitimate repeated contracts earn their normal payouts; substantial completion bonuses favor finishing over repeatedly abandoning tiny safe loads.

## Career, upgrades and loot

Every committed credit of smelt value or earned contract bonus currently grants **one credit and one XP**. Credits are spendable; XP is lifetime progress. Pickup and repeated capture grant neither. Level thresholds are 0, 120, 350 and 700 XP. Levels 2, 3 and 4 unlock the corresponding contract set above and mod tiers 1, 2 and 3. XP belongs principally in reward/workshop presentation so the active HUD can emphasize quota, remaining batches, load and danger.

Each of three branches has three sequential purchases costing **150, 300 and 500 credits**. Purchases deduct once, persist and change the actual rig:

| Branch | Starter → tier 1 → tier 2 → tier 3 | Player effect |
| --- | --- | --- |
| Capacity | 24 → 32 → 40 → 48 kg | Carry larger useful mixed loads safely |
| Coil | Radius 110 → 145 → 180 → 215; force 1.0 → 1.25 → 1.5 → 1.75 | Stronger, wider attraction with precision control retained |
| Stabilizer | Fuse 3 → 4 → 5 → 6 seconds | More time to rescue a valuable unstable haul |

A core becomes a permanent named collection entry only when smelted. Collect all six identities; repeated cores still pay salvage value without duplicating the collection. Cleared and gold contract records and best banked haul persist. Six contract families use deterministic seeded arrangements; new completed-contract runs change the seed, while retry reproduces the current seed. No seed-selection UI or endless content promise is implied.

The finite replay goals are gold marks, missing cores, remaining meaningful upgrades and better approaches to changed arrangements. Earned power is retained on earlier jobs. Voluntary full-rig replay remains an unproven design hypothesis.

## Presentation, saving and scope

The implemented presentation uses a 3D industrial workbench, distinct salvage materials and hazards, a visibly upgraded magnet, moving bundles and cargo, attraction/impact effects, a responsive furnace and ingot payout. The audio set contains 19 original effect cues and a 96-second original music loop with separate music/SFX settings. Attractive graphics, satisfying impacts, soothing music and a balanced mix are player-experience requirements; asset counts do not prove them. Provenance belongs in `assets/manifest.csv` and the existing asset records.

The version-two save preserves wallet, XP, mods, collection and job records together with the actual current seed, piece states/positions, cargo, heats, banked value, bonus flags and elapsed fuse. Pause freezes the fuse; resume does not reset it. Validated atomic saves retain a recoverable backup. This career uses its own save location; the rejected demo ledger is not silently reinterpreted as new progress.

Scope remains offline Windows single-player in Unreal, one coherent workbench interaction and a finite salvage career. No lore campaign, online service, factory network, crafting catalogue, prestige reset or storefront commitment is required. The commercial objective remains NOK 20,000 profit per game, conditional on actual results; this rebuild provides no sales evidence.

## Questions governing the next iteration

- Before the first smelt, can the player explain the goal, four fuel charges and capacity?
- When nearly full beside a rich group, do they make an informed bank/precision/drop decision? Does recovering a dropped haul create an interesting second approach or merely tedious recollection?
- Does visible attraction feel powerful, and do impact sound, movement and melting make the payout satisfying?
- Does the first purchase change what the player wants to attempt? Are advanced jobs approachable with the equipment actually owned when level four arrives?
- After buying upgrades, do gold, rare finds and a changed arrangement motivate a voluntary replay?

[PRODUCTION-PLAN.md](PRODUCTION-PLAN.md) records implementation and acceptance dependencies. [QA-REWORK.md](QA-REWORK.md) and [BUILD.md](BUILD.md) own exact build evidence; the older demo's positive functional checks do not carry over as proof of this rework's quality.
