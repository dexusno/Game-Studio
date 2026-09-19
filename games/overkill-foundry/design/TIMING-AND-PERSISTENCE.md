# Timing and persistence contract

20 September 2026. Implementation specification worked out under Klaus's instruction to resolve the obvious cases and use STS2 for inspiration. This completes the general timing/save design task; it is not a running save system or combat engine. Existing owner decisions and explicit character/recipe/upgrade exceptions take precedence. Catalogue numbers, MVP scope and unselected alternative abilities are unchanged.

## The rule the player should experience

Finish the action and its immediate consequences in a predictable order. Pay before receiving the effect. Shield protects during Fire and enemy attacks, and spent protection stays spent. Resolve remaining-Shield effects before resetting it. Save completed choices once. Restart an unfinished fight from its original entry state, while keeping profile discoveries already seen.

This contract supplies defaults where a row names a trigger but not an ordering tie-breaker. It never moves an expressly earlier/later trigger, removes a special ability or reopens the completed recipe audit.

## 1. Effect ordering

Use named phases and an event queue shared by the graphical game, preview and graphics-free runner. Animation completion, frame rate, dictionary iteration and UI sorting cannot choose rule order.

1. **Explicit timing wins.** A before-hit effect, an immediate reaction, an after-action reward and a next-turn delivery are different points. Resolve them at those points even if their source was obtained earlier/later.
2. **Otherwise use stable effect order.** Upgrades use acquisition order; ordinary part hooks use their first binding/installation order, and repeated movement does not give an old hook a new priority. Store one monotonically increasing binding sequence across these sources, then source ID/effect index for any remaining tie. Innate hooks are bound at character initialization. Physical spread hits still use current bullet placement order, and Shield payments still use current installation order: those are explicit different rules.
3. **Finish a triggered effect and its immediate reactions before the next sibling effect.** Nested events carry their parent event ID. A newly acquired listener does not retroactively receive an event already published. Its own acquisition event still runs.
4. **Check live state at resolution unless the rule requires a snapshot.** An event retains facts such as actual HP lost, actual costs paid and Heat at Fire. A later effect testing current Heat or remaining Shield sees earlier resolved changes. Already-recorded future amounts do not recalculate. A bound hook still obeys its stated installed/present/living-source requirement at the named boundary; old binding order does not make a removed part installed. A committed delayed delivery survives ordinary source consumption/removal unless its text explicitly keeps a source-presence condition.
5. **Claim a qualifying limited-use trigger before its payload.** Validate its stated condition first; consume the allowance even if its valid payload overheals or overflows a meter, unless its own rule counts actual gain. This prevents a callback from recursively claiming the same allowance.
6. **Survival checks interrupt ordinary work.** Named prevention/interception happens at its specified before-damage/HP-loss/death boundary. A matching death-prevention effect gets its opportunity before final death. Once player death is final, remaining ordinary effects stop. A later ordinary heal cannot revive the player.

Keep upgrade display order consistent with acquisition order. Show ordered part hooks in the effect breakdown when their order changes the result. A tooltip/log should explain a changing amount without exposing event IDs to players. A content-defined ordering cycle is a validation error to fix in that content, not a reason to pick a random order or add a gameplay action cap.

## 2. Action and hit transactions

| Boundary | Required resolution |
| --- | --- |
| Recipe Use | Validate the owned recipe copy, availability, selected options and every cost against the current state. Commit costs/use/cooldown atomically. Then resolve the Utility or create the ordinary outputs and their event consequences. A failed validation spends nothing. A later refund or output cannot fund the up-front price of its own Use. |
| First Shield installation | Validate/pay its additional costs, then calculate its once-only values and resolve first-installation effects in printed order. Store actual payment, calculated value, first-use round and hook sequence. Reinstalling preserves these and never repays, refunds, refills or rearms them. |
| Load / unload | Reserve the selected physical bullet and any explicitly required sacrifice; pay no Fire cost. The reserved objects cannot be sold or spent elsewhere. Unloading releases reservations, not earlier crafting/installation costs. |
| Fire validation | Require a nonempty legal bullet, legal targets and the full combined payment. Preview Noor's chosen Charged Barrel payment before other Charge checks and ensure mandatory part costs remain affordable. No cost-triggered future grant is borrowed to make this validation pass. |
| Fire commit | Commit the selected costs and consume bullet parts/sacrifices once. Resolve before-hit clocks, including character modifiers and explicit Fire modifiers, from the appropriate snapshots. Installed Shield automatically protects if it has remaining strength. A new Fire does not refill it. |
| Direct hit | Apply the existing integer damage pipeline and that target's defences, then Shield/HP. Record actual changes. Dead targets receive no later hit or payload; spread targets do not retarget automatically. A support hit is not another Fire. |
| Hit reaction | Capture qualifying robot recoil from this hit, including a killing hit; resolve it before ordinary after-hit gains/heals and later hits. Recoil uses remaining installed Shield and any applicable explicit protection before HP. Run a named rescue if eligible; otherwise final player death stops the action. Other explicit before/after reaction clocks retain their position. |
| After impact / shot | If alive, resolve that hit's printed after-impact payloads and eligible on-kill consequences, then subsequent hits in their specified order. After the entire shot and its reactions, resolve after-shot/after-action effects. Heat/Charge gained after impact cannot retroactively strengthen or fund that shot. |
| Terminal check | Final player death takes precedence over the last robot's death. Otherwise finish consequences already earned by the action before committing a surviving victory. Dead actors lose future actions immediately; record their legitimate kill once. Do not invent a remaining enemy phase solely to fire end-phase bonuses after the fight has ended. |

Enemy deaths update linked defences/intents immediately; the reward/ordinary on-kill payload waits behind the killing hit's compulsory reaction. This allows a robot's already-triggered Reactive Mesh to kill the player before an ordinary on-kill heal. A named rescue such as Reserve Heart Pump can instead prevent that death. No-kill escape completion is distinct from victory, as in the existing upgrade/achievement rules.

Different parts may have different status recipients. A main hit's Burn is not copied to Split Outlet/Three-Way Outlet hits; an explicitly multi-target or spread-owned status keeps its recipients and trigger. Whole-number reductions and bonus combination remain those of the reviewed catalogue.

### Protection allocation

Keep a single ledger of remaining protection. Installed parts contribute their own remaining values; explicit direct active/retained Shield contributes separate non-inventory balances. Defence drains oldest currently applied protection first, interleaving direct balances by their grant sequence and parts by current installation sequence. Removing a part removes only its unspent contribution from protection; reinstalling appends it to the current installation order. This never changes first-installation hook order.

Ordinary recipe costs/checks restricted to **currently installed parts** keep that restriction and drain those parts in the selected installation order. A direct active balance is not an invented physical part and does not satisfy a part-only sacrifice/count/payment. Upgrade effects explicitly using total remaining Shield include applicable active balances. Every effect operation declares eligible sources so a new direct-Shield exception cannot silently widen an old recipe's cost pool.

## 3. Full round sequence

| Phase | Order |
| --- | --- |
| Fight entry | Restore/construct the pre-start checkpoint described below. Initialize round 1, enemies, cleared cooldowns, fight-local meters/counters and innate features exactly once. Resolve entry supply deliveries and fight-start effects. These effects may change starting supplies/HP; keep their own acquisition/once-per-fight rules. |
| Player-turn start | Reset once-per-turn counters. Deliver previously recorded values due now, in scheduled order, then ordinary turn-start hooks in stable effect order. Each delivery consumes its schedule once. Explicit timing overrides these default subphases. This occurs after the previous enemy-phase reset. |
| Intent and collection | After due start effects, commit the next legal enemy intent before collection. Show the full left-to-right forecast. Perform one collection with chosen steering and optional available Precision. Extra resource grants are not extra collections. Later player actions update consequences, not the committed random move. |
| Preparation | Allow legal Recipe, Shop, install/remove, Load/unload, target and Fire actions. Each action resolves before accepting another. Shots do not tick cooldowns, reset Shield, reset recipe uses or collect again. |
| End Turn | Return the unfired bullet's unused parts and release reservations. Resolve named End Turn hooks in stable order; then expire unused this-player-turn/next-shot effects at their stated boundary. Apply the roster's explicit pre-enemy penalties and player status timing. Installed protection works even after zero shots. |
| Enemy actions | Act left to right. For each eligible living robot, resolve its stated pre-action status ticks, committed action if still alive, immediate reactions and post-action ticks. New summons cannot act before the next round's enemy phase. Damage/deaths update later forecasts and linked effects. |
| Pre-reset effects | Once enemy actions finish and combat continues, resolve ordinary remaining-Shield readings/payments and other same-boundary effects in stable effect order. Each reading uses then-current remaining protection and records any future amount. Each cost spends actual eligible remaining protection immediately; no double spending. |
| Explicit after-reading effects | Resolve effects explicitly after remaining-Shield readings/costs, such as Night Cell's Charge gain. Respect a named before/after constraint between effects; do not let the general tie-breaker override it. |
| Retention | Evaluate applicable retention conditions from the state after pre-reset effects. Freeze that reset's eligibility before retention-generated rewards. Allocate each allowance in effect order from a shared remaining budget; independent allowances add but cannot exceed actual remaining Shield. Record each item's actual retained contribution. Then resolve retention rewards. A reward cannot retrospectively qualify itself or another simultaneously assessed allowance. |
| Reset and decay | Keep only the allocated retained active balance. Clear the ordinary installed round build/remaining Shield. Preserve unused reserve parts with their recorded history/depletion. Apply each status/meter's own end-phase decay once; enemy Weaken loses 1. Tick numeric cooldowns once except copies used in this round. Explicit different lifetimes remain. |
| Next round | Increment the round and return to player-turn start. Scheduled amounts are delivered as recorded; new Shield is a fresh part unless its source explicitly grants a direct active balance. |

The pre-reset stage is **ordered**, not one frozen Shield snapshot shared by all effects. If a Shield spender resolves before a later reader, that reader sees what is left. A reader that already recorded its next-round amount keeps that amount. This follows the existing rule to pay/check available Shield at the named event. Retention is always assessed after those effects. For an automatic “up to” conversion with rewards in complete groups, pay only the smallest amount that earns the greatest available reward: Sweep the Plates spends `3 * min(3, availableShield // 3)`. It does not waste a remainder of 1–2 Shield for no Iron. Explicit player-selected optional payments retain their choice.

For example, Night Cell may bring Noor from 5 to 6 Charge after readings and thereby qualify Bridge Reserve Cell at the following retention stage. Talia's Phase Dividend instead checks its required 8 Charge before paying its retention reward; the Charge earned by that reward cannot establish eligibility. No resource rule or printed amount changes.

### Four-turn escape warning

Store the scheduled departure round. If round 2 is the first planning round showing the warning, show **4, 3, 2, 1** during rounds 2–5 while that robot performs its normal legal actions. Round 6 shows **Escape**, replacing its normal action. A warning created after the player's planning opportunity starts its four full warning rounds at the next planning round. Do not shorten the warning through a UI refresh or reload. Escaped robots grant no kill loot; actual kills in the same fight retain their normal eligibility.

## 4. State lifetimes

| State | Lifetime / save rule |
| --- | --- |
| UI focus, animation progress, hover, preview scratch state | Presentation only. Never changes a seed, payment or achievement. Continue opens the authoritative phase, not a half-played animation. |
| Bullet reservation, next-shot boosts, round counts | Their explicit player-turn/round boundary. End Turn releases an unfired bullet and expires ordinary unused next-shot bonuses. |
| Raw materials and unused ordinary parts | Carry between rounds; clear at the completed fight boundary. Preserve part source/resale identity, age, payment history and depletion while they exist. No stock cap is added. |
| Installed Shield and ordinary active protection | Deplete across attacks/Fire recoil; reset at enemy-phase end. An explicit retention exception carries only the actual retained balance without recreating old part payloads. |
| Recipe-copy cooldown/use tracking, fight buffs, meters, summons, pattern state, Precision-spent flag | Fight-local, with individual clocks. New fights clear/reset them. Continue rebuilds from the original entry state, not from the abandoned attempt's state. |
| Deferred effect | Record source, due boundary, fixed amount/targets where specified, effect order and lifetime. Ordinary next-round effects expire at fight end; explicitly next-fight/campaign effects survive to their stated trigger. |
| HP, Credits, cores, recipe memory/copy upgrades and permanent upgrades | Campaign state, subject to explicit fight-local modifications. HP does not automatically heal at fight/city transitions. New Game creates new campaign state; death closes the old one. |
| Shop stock, generated offers, acquisitions, pending choice cursor and completed purchases | Campaign state outside a fight; included in the entry checkpoint for rollback inside a fight. Reopening screens never refreshes or rerolls. |
| Recipe Collection and mercenary unlocks | Profile state. Legitimately seen recipes persist immediately; city-clear unlocks persist only after a committed qualifying result. Losing/restarting a fight cannot remove discoveries or create a city victory. |
| Achievement candidates | Follow the existing Steam plan: unfinished-attempt candidates roll back; terminal qualifying receipts and profile discoveries commit once. No external Steam call is needed for local correctness. |
| Settings and supported accessibility choices | User/profile configuration, separate from campaign rollback. |

Keep recipe-copy identity separate from recipe definition ID. Copying or discounting a part does not change its normal resale reference. New generated-only pure grants lock their standard reference from their original generated value; later buffs/depletion never reprice them.

## 5. Fight completion and between-fight flow

Use persistent phase values: `city_arrival`, `between_encounters`, `fight_active`, `rewards_pending`, `mystery_active`, `campaign_defeated`, `campaign_complete`. An ongoing choice has its own stored cursor under the owning phase. Shop is an overlay on its owning phase, not a second campaign. Steps 2–4 below form **one atomic result transaction**: calculate rewards, clear old inventory and restock in memory, then save them together before opening any interactive result screen. They are not three separately resumable writes.

1. **Finish the terminal action.** Resolve its already-earned consequences while the player lives. A named victory heal runs before recording outgoing HP. Player death blocks victory; escape-only completion is not a victory. Evaluate boss/node completion from the actual result.
2. **Commit the result once.** Assign a stable encounter-result receipt. In the same local commit, record outgoing campaign state, qualifying achievement/city/unlock facts, the exact reward offer IDs/quantities and each item's pending acceptance state. The old active-fight checkpoint can no longer be selected by Continue.
3. **Close the old fight inventory before new purchases.** Clear its raw materials, ordinary parts, round protection and fight-only schedules/counters. Keep explicit next-fight deliveries in a separate inbox and campaign items according to their lifetime. No implicit healing or auto-sale occurs.
4. **Restock once if the campaign continues.** Use that result receipt to advance the shop generation exactly once. Preserve the generated stock through the reward and between-fight screens and the next fight. A normal shop revisit, new round or noncombat Mystery cannot restock it. A Mystery containing a real completed fight can use that fight's receipt without counting the route node twice.
5. **Resume rewards until handled.** Each acceptance/skip is a committed choice. Accepting a recipe and exchanging its old memory copy is one operation. Other loot remains individually acceptable/skippable. A purchased/accepted upgrade applies its acquisition effects once; a forced nested choice stores its cursor and resolved substeps. Do not reroll the offer or repay/regrant on resume.
6. **Prepare the next encounter.** Buying resources now puts them in the next-fight entry inventory. Buying a recipe learns it; it does not use it or heal. Record money, stock and acquired object together. Any explicit immediate effect outside combat resolves once and its continuing product is staged for next entry if it is a part/material. Purely fight-only temporary effects cannot be activated from the between-fight shop UI. Staged fresh parts begin with round-1 age in the receiving fight; being purchased/granted between fights does not fabricate a previous-round saved-part bonus.
7. **City transition.** Resolve the boss result and accepted loot, then save the next city and its generated Mayor choices. The Mayor gift is acquired only upon acceptance; repeated opening grants nothing. The next city does not clear newly purchased supplies or invent an additional fight restock. Final-route completion is committed before any later optional endgame entry.

For a full campaign, a Mystery is saved with its exact initial options and every committed selection. A cancel before confirmation changes nothing; a completed choice is not undone by exiting. Unresolved reward/Mayor/Mystery screens resume in place with the same options. This contract does not select the still-missing authored Mystery outcomes or prices.

### Exact fight-entry checkpoint

Commit the chosen encounter and its seed/formation before first combat setup. Save **one pre-start input snapshot**, including HP, supplies bought between fights, owned recipe copies/upgrades and charges, shop stock/Credits, pending next-fight grants, bound effect order, content version and RNG positions. Mark it `fight_active` with start initialization not yet run. Initialization and fight-start/turn-1 effects are replayable from this snapshot; they are not an extra persistent acquisition each time.

Continue during an unfinished fight restores that snapshot, then reruns deterministic start initialization once. The abandoned attempt's in-fight purchases, sales, HP, generated parts, consumed limited charges and feat candidates are discarded together. Seen recipe discoveries remain in the profile. Repeating the same choices, including a recorded equivalent Precision result, reproduces results; different choices may change the fight.

There is **no second general material/part clear at fight entry**. Previous-fight clearing already occurred before the between-fight purchases. Entry resets only fight-local state and delivers staged supplies. A missing checkpoint is an error to recover from the last valid save, not permission to create a new seed or duplicate a grant.

## 6. Save transactions and crash recovery

Use one versioned local save envelope/journal for the active profile's campaign changes, associated profile facts and stable receipts. An operation ID includes the campaign/encounter identity and a monotonically increasing transaction sequence; reapplying a committed ID returns its recorded result without another payment, RNG draw, trigger or reward. Offer IDs and result receipts are not inferred from the currently visible UI.

Commit an outside-fight purchase/choice before reporting success or allowing dependent actions. Save generated random offers before exposing them. A terminal victory/defeat commits before accepting Quit or opening the next interactive screen. Queue normal Quit until the current atomic action has resolved; during unfinished combat it still leads to an entry restart. In-fight action traces may be journaled for crash recovery/debugging, but are not a mid-fight Continue save mode. If a committed trace contains a terminal result, reconcile that result before deciding which screen to restore.

Write to a temporary file, flush, then atomically replace the active save, retaining a last-known-good revision. Keep a schema/rules/content version and integrity check. A crash while writing recovers either the old valid state or the complete new one, never a purchase with only money removed or only the item granted. If a terminal action never reached any durable record before abrupt power loss, recovery may return to the previous valid entry snapshot; this is an unavoidable durability limit, not a supported revive command. Actual runtime crash-injection testing is required.

Steam submission is an outbox following local commit. Retrying it cannot grant gameplay rewards again. Unsupported content/save versions must report incompatibility or use an explicit tested migration; never silently reroll a saved offer, substitute a different robot or turn a dead campaign into a live one. New Game replacement retains its existing confirmation and preserves profile discoveries/unlocks.

Separate deterministic RNG streams for routes, formations, robot patterns, collection, offers/rewards, shop and nested choices. Derive them from a documented stable hash of the campaign seed and typed domain/encounter/entity keys; do not seed every stream with small numerical offsets. Persist resolved content and stream positions. Presentation never consumes them. This is our implementation requirement, informed by the STS2 RNG lesson below, not a claim to reproduce its implementation.

## 7. Worked traces and future runtime checks

Values below are controlled test inputs using existing catalogue amounts; they do not approve starting HP, prices, meter tuning or an MVP content list. Arithmetic/record inspection can establish the expected result, while runtime behavior remains untested.

| ID | Setup and action | Expected result |
| --- | --- | --- |
| T01 | 40 HP, 8 installed Shield; Fire triggers 3 recoil, then another shot triggers 7 recoil | First: 40 HP / 5 Shield. Second: 38 HP / 0 Shield. Firing does not refill it. |
| T02 | 2 HP, 1 Shield; kill the last robot and trigger 4 recoil; no rescue | 1 absorbed, 3 reaches HP, final death. Game Over; no later ordinary healing/victory reward. |
| T03 | Same lethal event with unspent UGS-067 Reserve Heart Pump and max HP 80 | Pump prevents death, is consumed and sets HP to 20. Continue remaining legal resolution; if no enemies remain, surviving victory is possible. |
| T04 | First-install Quench Ribs at 3 Heat; remove/reinstall the physical part | Pay 3 once. No second charge, refund, value recalculation or repeated first-installation effect. |
| T05 | 12 eligible remaining Shield; NO060 Field Pocket reads before SH066 Sweep the Plates; MY3-03 Shield Vault applies | Record 3 Charge; spend 9 Shield and record 3 Iron; retain only the remaining 3. Next turn deliver 3 Charge and 3 Iron, subject to normal meter caps. Shield is not duplicated. If the two part hooks were first bound in reverse order, the later Field Pocket reader records 1 Charge from the remaining 3 instead. |
| T06 | 12 remaining Shield; MY3-21 Shield Reinvestment Fund, then SH066, then MY3-03 | Fund pays 10 and schedules 4 Iron + 3 Copper + 2 Carbon. The remaining 2 cannot earn a complete 3-Shield group, so automatic Sweep spends 0 and grants 0 Iron. Vault retains those 2. Do not manufacture 3 Iron from the earlier 12. |
| T07 | 20 remaining Shield and two eligible retention allowances of 3 and 6 | Retain 9 total, clear 11. With only 5 remaining, retain 5 total; allocations are 3 then 2 in that order. |
| T08 | Shield next-round delivery is 6; old remaining Shield is 2 | Reset the old 2 unless an exception retains it, then deliver a fresh 6 at next-turn start. The delivery is not reset again. |
| T09 | Cooldown 1 used in round 4; no explicit cooling | Skip round-4 auto-tick; unavailable round 5; ready round 6. Clearing it explicitly in round 5 permits a legal use then. |
| T10 | First escape warning is shown in planning round 2 | Normal legal actions in rounds 2–5 with countdown 4/3/2/1; Escape action in round 6. No departure attack or fleeing loot. |
| T11 | Prior fight ends with 5 Iron; between fights buy 2 Iron for 20 of 50 Credits; quit | Old 5 clears before purchase. Resume with 2 Iron, 30 Credits and reduced shop stock. Next fight entry still has those 2 Iron, plus only its explicit entry grants/collection. |
| T12 | Entry has 2 Iron/30 Credits; during fight buy 1 Iron for 10 and spend a once-per-campaign pump; quit while fight remains unfinished | Continue starts with the original 2 Iron/30 Credits, entry shop stock and unspent entry pump. Seen recipe discoveries remain; none of the abandoned attempt's shop/charge changes leak outside rollback. |
| T13 | Reward offers A/B/C; accept B, exchange one memory copy, quit | Resume with B owned and that old copy exchanged exactly once. Offer IDs/other loot statuses are unchanged; no second B, reward reroll or second city-progress count. |
| T14 | At a shop transaction, crash before versus after durable commit | Before: old money/stock/inventory. After: new money/stock/inventory and one receipt. Neither recovery state contains half the purchase. Runtime crash testing must establish durability. |
| T15 | 5 Charge, Night Cell + Bridge Reserve Cell, 14 remaining Shield | After readings Night Cell gives 1 Charge; retention stage sees 6 and retains up to 12. Clear the remaining 2. Without Night Cell, 5 Charge would not qualify that allowance. |
| T16 | Talia's Phase Dividend alone, 7 Charge and 18 Shield | Its 8-Charge eligibility fails; it retains nothing and pays no Charge. At 8 Charge it retains 18 and then grants 3 Charge, subject to the meter limit. |
| T17 | An upgrade grants 2 Heat at turn start and another sets Heat to at least 6; current Heat 0 | With the +2 hook acquired first, result is 6. Reversing two otherwise identical test hooks would yield 8, but the real City-2/City-3 Mayor pair is normally acquired in city order. Preserve acquisition order and show it; do not secretly reorder to maximize the result. Explicit cap/rate exceptions still apply. |
| T18 | A noncombat Mystery completes, then its shop is reopened | Count the node once and save the selected outcome. Do not invoke combat-victory hooks or an ordinary fight restock. An explicit Mystery upgrade hook can run once. |

Before enabling the corresponding content, implement these cases against the shared core, then replay the same inputs through Unreal. Include invalid-cost atomicity, repeated resume, death during nested effects and mid-choice save recovery. Do not treat these paper traces as executed gameplay tests or proof of balance.

## 8. STS2 inspiration and evidence limits

Research accessed 20 September 2026. These observations inform the contract; the exact queue, snapshot boundary and save format above are our design choices.

| Reference | Observed evidence | Application here |
| --- | --- | --- |
| [STS2 Mechanics](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Deck) | Wiki-indexed text describes alternating player/enemy turns, visible intentions and left-to-right enemy actions | Keep explicit phase boundaries and an honest ordered forecast. Our exclusive End Turn and recipe economy remain owner rules. |
| [STS2 Buffs](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Buffs), [Dexterity](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Dexterity) and [Relics List](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Relics_List) | Next-turn Block is a scheduled grant; the Dexterity page describes its previously calculated amount surviving a later stat change. Sturdy Clamp is an explicit limited-retention effect | Keep recorded future grants separate from retaining existing protection; do not recalculate past snapshots. Our reset remains at enemy-turn end. |
| [Mega Crit v0.100.0 announcement](https://steamcommunity.com/games/2868840/announcements/detail/503978984819655260), readable in the [official Steam news feed](https://store.steampowered.com/news/posts/?enddate=1773972046&feed=steam_community_announcements) | The 19 March notes fix a predetermined relic reward changing after save/quit and clarify the HP-zero trigger for named rescue items | Preserve generated reward identity across reloads and give rescue an exact interception point. This is direct developer evidence, not a claim that their complete save system matches ours. |
| [Mega Crit Major Update #2, v0.107.1, in the official news feed](https://steamcommunity.com/app/2868840/allnews/?l=bulgarian) | The 18 June engineering explanation describes separate seeded PRNGs and unexpected correlations between their results | Separate domains carefully and test repeatability/cross-domain independence; merely naming separate streams is insufficient. The article body is English despite the feed locale. |

Wiki direct opens failed in this research session; gameplay definitions above were accessible through indexed extracts. Official March patch text and June engineering text were read in Steam's feeds; the individual March announcement rendered only an image in the reader. Confidence is high for those narrow observations. No source inspected establishes a universal STS2 relic tie-breaker or exact crash-safe transaction implementation. Our stable binding order, transactional saves and worked traces are explicit implementation decisions under the owner's authorization. No source code extraction, STS2 runtime test or Overkill Foundry runtime test was performed.
