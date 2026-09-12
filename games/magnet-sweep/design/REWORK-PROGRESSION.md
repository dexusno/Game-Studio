# Magnet Sweep — contract and progression rework

Governing final correction, 2026-09-12: failure also spends one fuel charge, including a lone hot cell; RMB drops all unbanked cargo recoverably instead of selective sorting. BRIEF.md governs the implemented rules; earlier proposals below remain historical.

2026-09-12. Implementable design hypothesis following Klaus's negative playtest. The prior unlimited, risk-free collection loop was rejected. This note records the progression agreement with the loop designer, gameplay critic and integration owner; it does not establish that the revised game is fun. No development deadline governs the design.

## One complete loop

Choose an unlocked salvage contract. Inspect its target, optional gold target, available metals and **four furnace heats**. Hold the magnet to draw physical scrap toward it, using a narrow field with Shift to steer around hazards. Watch carried mass and the actual valuable piece at risk. Decide whether to bank a useful load now or make room for a richer linked bundle. Deliberately smelt a stable load: scrap pours into the furnace, becomes visible ingots, and produces cash, experience and an explicit jump in contract progress. Complete the contract for a substantial bonus; optionally use remaining heats for gold or rare salvage. Buy a visible rig improvement and try a new contract or a fresh arrangement with the earned rig.

Four heats are a planning limit, not a countdown. A nearly empty smelt spends one heat just as a full smelt does; the furnace hover preview says so before the click. A full load of ordinary iron is useful. Better value per kilogram, a rich bundle and an exposed rare core offer reasons to make more ambitious choices.

## Material and risk rules

| Material | Mass | Smelt credits / base XP | Readable role |
| --- | ---: | ---: | --- |
| Iron scrap | 2 kg | 4 | Common useful filler; a starter full load earns 48 |
| Copper salvage | 3 kg | 12 | Valuable mixed-load target |
| Alloy salvage | 4 kg | 24 | Heavy, valuable pieces near awkward bundles |
| Rare core | 4 kg | 40 | Optional valuable find; banking adds its unique relic to the album |
| Hot cell | 4 kg | 0 | Clearly marked hazard; starts the risk fuse while carried |

The root integration contract governs risk: capacity is 24 kg initially. Actual excess cargo or a carried hot cell starts a **3-second fuse**, which continues when the field is released. The warning names the highest-value carried salvage piece and its exact credits at risk; ties use stable piece ID. Expiry destroys that one salvage piece, removes the hot cells and spills the rest recoverably. There is no permanent equipment damage or separate integrity meter. Banked progress is safe.

Right mouse vents hot cells first, then the lowest-value-per-kilogram whole salvage pieces until the load is safe (stable ID breaks ties). Vented salvage is recoverable. Automatic vent sorting is a convenience whose possible dominance must be checked in play; it cannot establish a good decision by itself. Capturing a connected group is atomic and refused above 150% of capacity, with its physical pieces left available. Shift provides a narrow field from the beginning. The furnace accepts stable loads; an unsafe load gets a visible instruction to vent first, so the player cannot bypass capacity by teleporting an overloaded haul into the furnace.

Links identify physically connected groups that attract together, bringing their combined weight and value. They are neither a hidden corridor puzzle nor an extra currency. The group must look and move connected, with an inspectable combined mass/value. Core goals never require a particular rare item, so destroying or missing one cannot soft-lock a contract.

## Initial contracts and replay set

These are initial tuning values, to be changed when actual play demonstrates a problem. Counts exclude cells. Every job has four heats and one optional rare core. The first three teach distinct decisions; later contracts reuse familiar mechanics in authored arrangements with seeded variations, not new rule systems.

| Job | Quota / gold | Completion / gold bonus | Iron / copper / alloy | Cells | Intended choice |
| --- | ---: | ---: | ---: | ---: | --- |
| 1. First Pour | 120 / 180 | 80 / 40 | 24 / 8 / 2 | 2 | Bank three useful ordinary loads, or selectively gather copper for gold; hazards are visibly avoidable |
| 2. Copper Knot | 180 / 260 | 120 / 60 | 24 / 12 / 6 | 3 | A linked rich bundle fits an empty starter rig but can overload a partly filled rig |
| 3. Live Wire | 240 / 360 | 160 / 80 | 24 / 16 / 10 | 4 | Use precision and venting around alloy/core clusters; capacity and coil investments visibly help |
| 4. Heavy Freight | 300 / 440 | 200 / 100 | 28 / 18 / 14 | 5 | Several dense bundles; decide the batch composition before committing |
| 5. Split Seam | 380 / 540 | 240 / 120 | 28 / 20 / 18 | 6 | Valuable seams approach hazards from different directions; a different route changes the safe haul |
| 6. Furnace Crown | 460 / 660 | 280 / 140 | 30 / 22 / 22 | 6 | Full-rig challenge with attractive rich hauls, separated safe patches and optional core recovery |

Job 1 contains 280 credits before bonuses. A starter load of 24 kg iron is worth 48; three such loads would exceed the base quota. Its actual mixed population must permit a similarly conservative three-load solution without a core or mandatory hazard capture. Later layouts must each have a demonstrated base-quota solution at their unlocked rig state. Merely having sufficient total value is not feasibility evidence.

A clear earns the completion bonus **once per attempt**, credited when the quota is reached. Gold similarly earns its bonus once. A later mishap cannot retract a completed contract. At quota, the result offers **Finish contract** or **Keep salvaging — N heats left**. Finish never silently discards carried salvage: require a stable smelt if a heat remains, or explicitly show the unbanked loss before abandonment. Four spent heats below quota produce an honest failure result. If remaining salvage plus cargo cannot reach the quota, identify the lost opportunity immediately and offer retry/return to workshop; do not leave an unwinnable board pretending otherwise.

## Cash, levels and real upgrades

Cash is spendable. XP is permanent career progress; it unlocks jobs and mod tiers. Both accrue only from committed smelts and earned contract bonuses, never from pickup, repeated capture or animation. Each credit of smelt value or contract bonus awards one XP. The XP bar appears in the smelt result and workshop, not as another permanent active-play meter.

| Level | Lifetime XP threshold | Tangible unlock |
| --- | ---: | --- |
| 1. Apprentice | 0 | First Pour, all basic controls |
| 2. Salvager | 120 | Copper Knot and tier-one mods |
| 3. Reclaimer | 350 | Live Wire and tier-two mods |
| 4. Master Operator | 700 | Heavy Freight, Split Seam, Furnace Crown and tier-three mods |

Unlocked jobs do not demand a separate grinding requirement. Replaying completed jobs remains legitimate and earns their normal smelt values and attempt bonuses. Completion bonuses make completing a sound contract more rewarding than repeatedly abandoning tiny safe loads. Do not punish failure by confiscating earned money to fight hypothetical farming. Monitor whether a boring farming route becomes attractive in actual play.

| Mod | Starter / tier 1 / tier 2 / tier 3 | Visible and expected player effect |
| --- | --- | --- |
| Basket | 24 / 32 / 40 / 48 kg | Larger carry assembly; safely combine bigger hauls and need fewer furnace heats |
| Coil | Radius 110 / 145 / 180 / 215; force 1.0 / 1.25 / 1.5 / 1.75 | Larger lit coils and stronger acceleration; more dramatic attraction, with narrow-field control retained |
| Stabilizer | Fuse 3 / 4 / 5 / 6 seconds | Visible insulation; more opportunity to vent a dangerous valuable haul before losing its best piece |

Per-branch purchase prices are 150, 300 and 500 credits. A purchase requires its preceding tier, the matching player-level unlock and sufficient wallet balance. Deduct once, apply immediately in the workshop, save together, and show the exact changed capability. First-contract quota plus its 80-credit bonus guarantees one tier-one choice without awarding every branch. No consumable upgrade purchases, loot fragments, crafting trees, random paid rolls or prestige reset.

The three early clears at their minimum quotas award 200 + 300 + 400 = 900 cash and XP before optional gold/surplus. That is several material upgrade choices and access to the advanced contracts, not a promise of long paid-game duration. Earned power remains useful on earlier jobs; do not dynamically raise their quotas to cancel it.

## Rare loot and finite replay goals

Each of the six contract families has one recognizable core identity. Its first successful smelt adds a named object to the visible workshop album/shelf and celebrates **New find**. Merely touching it does not secure it. Duplicate finds remain worth their displayed cash and XP; the album entry is not duplicated. A rare find is optional and physically placed in an interesting recoverable situation, not a random reward popup that masks ordinary play.

After initial clears, replay motivation is a concrete set of remaining goals: earn each job's gold mark, recover all six cores, buy the remaining distinct rig tiers, and improve a comparable job result using a chosen seed. A seed remixes authored valuable clusters, links, hazard approach angles and safe patches. It must change a useful route or batch decision rather than only jiggle coordinates. Keep a best banked haul and fewest heats per job for comparable attempts; no online service or daily obligation.

The completed collection and upgraded rig are a finite achievement. Do not display nonexistent next levels or promise infinite content. Continued full-rig play depends on enjoyable attraction, batch planning and new arrangements; only voluntary player replay can support that claim.

## Save and transaction contract

Persist career wallet, lifetime XP, purchased tiers, discovered cores, unlocked/cleared/gold records and best attempts. Persist the current job identity and seed, each piece's material/mass/value/state/position, cargo acquisition order, heats used, banked value, bonus-paid flags and fuse elapsed. Runtime pause/focus loss freezes the fuse; save/relaunch cannot silently reset an unsafe load's risk.

Capturing, venting, overload, smelting, settling and purchasing are authoritative state transactions. An animation may represent a committed transaction but cannot award it twice. Retry creates a fresh attempt and discards its unbanked cargo, while preserving previously banked money, XP, completed records, purchased upgrades and discovered cores. Retry does not re-award old bonuses or restore the wallet to an earlier checkpoint. Saves validate piece identity, ledger totals, capacity stats, bonus flags, level unlocks and collection bounds before replacing live state. Use a separate versioned save so the rejected demo's unrelated ledger is not misinterpreted as career progress.

## Acceptance and smallest next iteration

- A new player can state the quota, remaining four heats and carried capacity before the first smelt. The first smelt visibly earns spendable money, XP and contract progress, and produces an ingot and substantial audio response.
- A nearly full rig facing a rich linked group offers a meaningful bank/vent/precision choice. A careless large attraction has the forecast loss; release alone does not erase its fuse. A deliberate response can save the valuable piece.
- The first clear affords exactly a meaningful mod choice at minimum success. That purchase changes the next board's actual carrying, attraction or risk response, and survives retry/quit.
- A failed attempt preserves banked progress, explains why it ended and can restart without a soft-lock or duplicated rewards. An already-cleared attempt stays cleared after a later mistake.
- A core visibly risks loss while unbanked and becomes a permanent named find only when smelted. A repeat core earns honest salvage value without duplicating the collection.
- Demonstrate a conservative first-job completion and a riskier gold/core route in the actual package. Then compare owner observations of those routes; passing ledger tests is not evidence of challenge or fun.

Short playtest question: **When your magnet was almost full, what did you choose to do next, what did you risk, and did the smelt reward make you want another haul?** The smallest useful iteration is one representative contract with physical attraction, capacity/fuse/vent, four smelts, its first substantial reward and one selectable permanent mod. Integrate the agreed remaining jobs and presentation only around that functioning loop; do not ship a reward menu wrapped around the rejected interaction.

## Peer critique outcome

Loop and progression designers independently favored limited furnace heats over a battery timer; the critic accepted it conditionally on clear pre-smelt cost and an approachable safe route. The critic challenged empty XP, too many simultaneous meters and abort farming. XP therefore has explicit unlocks, is presented mainly at rewards/workshop, and the economy pays large completion bonuses while protecting honest banked progress. Root selected one instability fuse and one forecast valuable-piece loss instead of an additional integrity system, and three simple visible upgrade branches. These are reconciled design decisions pending play evidence, not unanimous proof of enjoyment.
