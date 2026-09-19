# Overkill Foundry — robot library by campaign stage

**Research-informed draft · 16 September 2026**

This roster contains **42 encounter entries: eight Regular designs, three Officer designs and three boss alternatives for each of three campaign stages**. The [mercenary progression proposal](CAMPAIGN-PROGRESSION.md) assigns this library across four distinct three-city campaigns, twelve cities in total. C1/C2/C3 are stable threat-tier IDs, not a claim that every mercenary visits the same locations. Group entries can contain several robot bodies. Five additional summoner templates (the Boot Pod/cutter plus four factory helpers) and one boss helper (Brake Drone) are defined below. A city run still ends with one boss encounter; three alternatives do not mean fighting three city bosses.

Klaus authorized using Slay the Spire 2's enemy strengths and tactical patterns as inspiration, with original robot identities and renamed/adapted effects. The city names, individual roster entries, effects beyond settled rules and numerical values below are proposals. Source research does not establish balance in our game. The existing [drone/siege encounter](ENCOUNTER-DRONES-AND-SIEGE.md) remains the approved worked route.

Read the [effect definitions](ROBOT-EFFECTS.md) and [Mayors and opening upgrades](MAYORS.md) alongside this roster. Two research agents examined [Acts 1–2](research/2026-09-16-sts2-acts-1-2.md) and [Act 3/effects/Ancients](research/2026-09-16-sts2-act-3-effects-mayors.md). Source names in the inspiration column are research references, not names used by our game.

## Original setting anchors — now Mara's proposed cities

| City — working title | Robot identity and surroundings | Tactical progression |
| --- | --- | --- |
| **1. Cinderwall** | A fortified foundry city surrounded by seized scrap yards, pumping stations and municipal workshops. Riveted machines, furnace ceramics and exposed pistons. | Learn target priority, predictable attack cycles, Armor, finite hit protection and limited preparation windows. |
| **2. Coilbridge** | Freight bridges, rail depots and automated production districts. Modular loaders, cable bundles, magnetic cranes and assembly pods. | Break support formations, manage reinforcements, use defence to create openings, and decide when to finish paired threats. |
| **3. Glassward** | Surveillance avenues and the hostile AI's civic control infrastructure. Sensor crowns, enclosed security frames, optical emitters and command machines. | Combine earlier lessons against auras, scaling, several attack shapes and clearly explained boss phases. |

These are Mara's three proposed settings within the established robot-takeover world. Ivo, Ada and Noor have nine other city identities and different route shapes in the progression document. Mayors represent the fortified cities; they are not the city bosses. The headings below retain these original setting anchors for readability; their robot IDs are reusable stage libraries.

## Reading a robot entry

- HP and damage are **our provisional whole-number beta values**, not copied STS2 values. An attack written 4 × 3 means three separate hits of four, total twelve, before buffs/defence. Every source-linked or increasing value must be included in the intent preview.
- A listed sequence is the default/reference pattern: it starts at its first action and repeats unless expressly described as an opening, one-time threshold or conditional. [Bounded pattern generation](ENEMY-PATTERNS.md) may choose only explicitly authored variants after the appropriate gate; the approved drone/siege fixture stays fixed. Support actions spend the robot's action. Spawning never grants a new robot an immediate surprise attack.
- Ordinary enemies act only after player End Turn. Reactive Mesh is an explicit enemy reaction, not an extra normal turn. Armor is flat reduction per direct hit. Installed player Shield protects automatically, including during Fire/recoil when it has remaining strength and during enemy attacks after zero shots; it follows the normal enemy-phase-end reset and explicit upgrade exceptions.
- A robot remains targetable unless a row explicitly says otherwise; this first roster contains no untargetable leader behind an unexplained bodyguard. Killing a source removes its linked penalty. Killed-target hits are lost without retargeting.
- Escape always follows the owner's four-turn advance warning and uses the departure action instead of attacking. It is not an automatic consequence of another robot dying. Killed robots remain eligible for their assigned loot; escaped ones do not.

## City 1 — Cinderwall

### Regular robots

| ID / robot | HP | Opening and repeating actions | What changes the player's decision | Inspiration |
| --- | ---: | --- | --- | --- |
| C1-R01 **Rivet Mite** | 6 or 7 | Attack 5 every turn. Use the two stated HP fixtures in the introductory group; this does not select random HP generation. | Several small shots can remove several attacks. | Original Scrap Drone from our approved example; group-pressure lesson also appears in [Nibbit formations](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Nibbit). |
| C1-R02 **Breach Ram** | 24 | Armor 3. Charge, then Attack 18, repeat. Charge announces the next blast and deals no damage. | Combine damage against Armor and save unactivated Shield parts during Charge. | Our approved Siege Robot, preserved exactly; not a conversion of source Plating. |
| C1-R03 **Coil Sentinel** | 26 | Packet Filter 1. Open with gain 1 Drive Gain; then Attack 7 and gain 1 Drive Gain, Attack 7 and gain 1, Attack 4 × 2; repeat those three attacks. | Spend a modest status application to clear the filter; do not let scaling reach repeated multi-hits. | [Cubex Construct](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Cubex_Construct). |
| C1-R04 **Cable Binder** | 22 | Attack 4 × 2; Attack 6 plus Recipe Fouling 1 for the next player turn; Attack 12. | Save a useful part before interference or pay a small visible surcharge; Shield and Utilities remain available. | [Vine Shambler](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Vine_Shambler). |
| C1-R05 **Pressure Cask** | 24 | Armor 1. Attack 8; gain 2 Drive Gain without attacking; repeat. | Use its pressure-building turn to finish it or prepare for a stronger return attack. | [Sewer Clam](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Sewer_Clam), with original Armor instead of recurring Block. |
| C1-R06 **Narrowband Emitter** | 18 | Apply Drive Fault 3; Attack 6; Attack 10. | Drive Fault persists across shots and weakens by 1 after each player action phase in the mirrored player draft. Cleanse it or save parts while it fades; killing the emitter prevents another application. | [Shrinker Beetle](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Shrinker_Beetle); our flat amount is an adaptation, not its undocumented source formula. |
| C1-R07 **Coil Nest** | 28 | Open by deploying two Rivet Mites. Thereafter alternate Attack 6 with a deployment action that fills up to two live mites from the remaining supply. If no mite can be deployed, that action is Attack 6 instead. Lifetime supply: four mites total. | Burst the nest or control its limited support supply; pure waiting does not generate an infinite army. | [Fogmog](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Fogmog), without repeated resurrection or deck pollution. |
| C1-R08 **Knuckle Press** | 28 | Brace: gain Armor 2 through the next player turn; then Attack 4 × 2 plus Shield Leak 2; then Attack 13. Armor expires before the two-hit action. | The preparation window leads into weakened defence and a heavy attack; keep spare Shield parts. | [Punch Construct](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Punch_Construct). |

### Officers

| ID / robot | HP | Behaviour | Counterplay | Inspiration |
| --- | ---: | --- | --- | --- |
| C1-O01 **Redline Pursuer** | 52 | Runaway Motor 1. Alternate Attack 13 and Attack 3 × 3. | Damage rises naturally with time; use Drive Fault against each hit of the multi-hit attack and finish before stored supplies lose their advantage. | [Byrdonis](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Byrdonis). |
| C1-O02 **Foil Warden** | 44 | Ablative Tiles 3 at entry, no recharge. Attack 9; gain 2 Drive Gain; Attack 12. | Strip finite tiles with affordable separate hits, then combine the finishing payload. Armor-heavy encounters should not be the only assembly test. | [Vantom](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Vantom), adapted from a boss pattern into a smaller specialist. |
| C1-O03 **Split Chassis** | 48 | Attack 5 × 2; apply Thermal Runaway 2; Attack 4 × 3. On destruction releases two 7-HP Rivet Mites, shown in its inspection from the start. | Preserve damage for a visible second wave. Later hits aimed at the destroyed carrier are lost; the new entities require later legal targeting. | [Phrog Parasite](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Phrog_Parasite). |

### Boss alternatives — one per city run

| ID / boss | HP | Main pattern | Distinct test | Inspiration |
| --- | ---: | --- | --- | --- |
| C1-B01 **Gatebreaker Prime** | 92 | Phase one repeats Attack 14 and gains 1 Drive Gain afterward. Recovery Joint at 46 HP: any HP loss reaching that threshold while it survives schedules one Recover action and clears its accumulated Drive Gain. Phase-one scaling ends. Thereafter cycle three separate actions: Attack 4 × 4, Charge with no attack, Attack 20. | Time a threshold to prevent a dangerous attack. No forced one-shot turn or artificial survival at 1 HP. | [Ceremonial Beast](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Ceremonial_Beast). |
| C1-B02 **Standby Leviathan** | 100 | Two visible Boot actions, no attacks; Armor 3 during boot. It remains asleep for those two actions even if hit. Remove boot Armor afterward; then Attack 16, Attack 5 × 3, apply Drive Fault 4 and gain 2 Drive Gain. | A finite preparation window is strong in our stored-part economy; choose how much to spend on damage versus future defence. | [Lagavulin Matriarch](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Lagavulin_Matriarch); fixed boot replaces the source's wake-on-hit rule. |
| C1-B03 **Fuse Cathedral** | 84 | Ablative Tiles 5, Packet Filter 1. Attack 10; Charge and gain 2 Drive Gain; Attack 18. Tiles are finite for the whole fight. | Many economical hits open a target that later favours decisive damage. Neither a per-turn damage budget nor a fixed fight length is imposed. | [Vantom](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Vantom). |

## City 2 — Coilbridge

### Regular robots

| ID / robot | HP | Opening and repeating actions | What changes the player's decision | Inspiration |
| --- | ---: | --- | --- | --- |
| C2-R01 **Cargo Ram** | 34 | Attack 12 with Overbalance. After a fully Shield-absorbed ram, spend its next action Recovering, then ram again. | Defence buys a genuine opening instead of merely preserving HP. | Rock role from [Bowlbugs](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Bowlbugs). |
| C2-R02 **Screen Loader** | 26 | Reinforcement Link 2 to its named cargo partner; alternate Attack 5 and Attack 8. | Remove the softer support to make later hits efficient, or burst through the partner's Armor. | Defensive role from [Bowlbugs](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Bowlbugs), with an original linked-Armor implementation. |
| C2-R03 **Booster Cart** | 28 | Attack 5; once only grant a named ally 3 Drive Gain; thereafter repeat Attack 7. If the ally is dead, the one-time boost goes to itself. | Kill or debuff the future damage source before its boost; the inspection names the recipient. | Nectar's one-time escalation in [Bowlbugs](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Bowlbugs), adapted to a squad role. |
| C2-R04 **Signal Loom** | 26 | Apply Drive Fault 4; Attack 4 × 2. | An offensive debuff has different urgency from an enemy attacking now. | Silk role from [Bowlbugs](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Bowlbugs). |
| C2-R05 **Latch Harvester** | 38 | Once per fight, after its first damaging direct hit, gain Armor 4 for the rest of that player turn. Cycle Attack 8, gain 2 Drive Gain, Attack 14. | Make the opening hit count, then consider waiting until the one-time shutter has expired. | [Louse Progenitor](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Louse_Progenitor); source reactive Block becomes explicitly timed Armor. |
| C2-R06 **Feedback Pylon** | 34 | Raise Reactive Mesh 2 for the next player turn without attacking; next action removes Mesh and attacks for 14; next action Attack 8; repeat. | Attack during exposed windows or accept a clearly shown recoil cost. Pending Shield cannot protect firing. | [Spiny Toad](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Spiny_Toad). |
| C2-R07 **Brood Press** | 42 | Deploy two Boot Pods; then Attack 10 and apply Target Paint 2; then deploy replacement pods if below two live children. Lifetime supply: four. If supply is spent, use Attack 10 on deployment turns. | Kill pods before activation, focus their maker, or plan area damage. | [Ovicopter](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Ovicopter). |
| C2-R08 **Magpie Courier** | 30 | Ablative Tiles 2; repeat Attack 6. At entry announces Escape four turns in advance; departure replaces its attack under the existing rule. | Strip the finite protection and claim its ordinary assigned loot before departure. No recipe theft in this version. | [Thieving Hopper](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Thieving_Hopper), adapting the hit-removal/escape pressure only. |

### Officers

| ID / robot | HP | Behaviour | Counterplay | Inspiration |
| --- | ---: | --- | --- | --- |
| C2-O01 **Coupled Hauler** | 26 per module, three modules | Each takes one combined action: Attack 5, then repair a different living module for 4. Display both effects in its intent. Prefer the ally with most missing HP and break ties left-to-right. It cannot heal itself or revive a killed module. | Coordinate damage or remove one module decisively to reduce both attacks and repairs. The source's revival puzzle is softened into living-target repair. | [Decimillipede](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Decimillipede). |
| C2-O02 **Backscatter Rig** | 62 | Reactive Mesh 1 throughout the fight. Attack 3 × 4; Attack 18; gain 2 Drive Gain. | Prefer fewer direct hits or available status damage; every chosen direct hit has a small visible HP risk. | [Entomancer](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Entomancer); a damaging-hit reaction replaces deck interference. |
| C2-O03 **Backfeed Array** | 64 | Utility Feedback 2, at most three activations per fight. Attack 14; Attack 5 × 3; gain 1 Drive Gain. | Decide whether cooling or another Utility is worth stronger future attacks. Utilities always remain usable. | [Infested Prism](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Infested_Prism). |

### Boss alternatives — one per city run

| ID / boss | HP | Main pattern | Distinct test | Inspiration |
| --- | ---: | --- | --- | --- |
| C2-B01 **Twin Gantry** | Press 72; Laser 64 | Press cycles Attack 16, Recover; Laser cycles Calibrate, Attack 5 × 3. Start them at the displayed offset. Each has Last Motor 4 when its partner dies. | Finish both together, or remove one and defend against the empowered survivor. No facing/back-attack system is required. | [Kaiser Crab](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Kaiser_Crab). |
| C2-B02 **Audit Engine** | 132 | Offer a Penalty Clause; Attack 16; Repair Cycle 8 on itself and gain 1 Drive Gain. Repeat, with at most three clause offers. After the third offer, replace future offer actions with Attack 10. | Choose the temporary handicap the current stock and recipes handle best; do not wait indefinitely through repairs and scaling. | [Knowledge Demon](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Knowledge_Demon). |
| C2-B03 **Conveyor Sovereign** | 138; two Brake Drones at 8 each | Press Countdown begins at 3. On each non-crush action attack for 8 and reduce the count by one. At zero, the following intent is Crush 26, replacing Attack 8; after Crush reset count to 3. Destroying a Brake Drone adds one to the count, maximum 4. Drones each attack for 3; no replacements. | Spend damage on delaying a known blast, save Shield, or race the main chassis. Crush is defendable enemy damage, not automatic death. | [The Insatiable](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:The_Insatiable), substantially adapting its lethal countdown. |

**Audit Engine clause details:** each offer presents (A) Recipe Fouling 2 for the next player turn or (B) Shield Leak 4 for that turn's End Turn. It is an enemy action with no simultaneous attack. Show the next attack when offering the choice. Clauses do not stack for the whole fight, impose HP costs, damage upgrades or remove recipe memory. These exact softer effects are our proposal, not the source's lasting debuffs.

**Conveyor clarification:** if destroying a Brake Drone raises a zero count before Crush happens, update the intent back to Attack 8 with the new countdown. The player can see this consequence before committing the shot. Destroying the main chassis stops its countdown; surviving Brake Drones remain ordinary enemies until killed or legitimately fled. No victory is awarded while they remain.

## City 3 — Glassward

### Regular robots

| ID / robot | HP | Opening and repeating actions | What changes the player's decision | Inspiration |
| --- | ---: | --- | --- | --- |
| C3-R01 **Litany Engine** | 58 | Open with Boot, enabling Runaway Motor 2; thereafter repeat Attack 12. Boot is its whole first action. | A short setup window becomes a race against growing damage. | [Devoted Sculptor](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Devoted_Sculptor). |
| C3-R02 **Aegis Tug** | 36 | Reinforcement Link 3 to its paired Beltgun Porter; repeat Attack 6. If the porter dies, the tug instead repeats Attack 14. | Kill the protector first or accept the survivor's stronger attacks after eliminating the gunner. | Protector role from [Living Shield/Turret Operator](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Turret_Operator). |
| C3-R03 **Beltgun Porter** | 48 | Attack 4 × 4; Attack 3 × 5; gain 2 Drive Gain. | Multi-hit damage magnifies accumulated Drive Gain; a paired protector makes target priority consequential. | Attacker role from [Turret Operator](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Turret_Operator). |
| C3-R04 **Municipal Assembly Rig** | 70 | Deploy helpers from the fixed visible queue: Screen Welder, Signal Mite, Seal Cutter, Arc Spool. On deployment turns fill up to two live helpers. Alternate deployment with Attack 12; when no helper can be deployed, Attack 12 instead. | Suppress a dangerous helper or focus the factory before all four are built. | [Fabricator](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Fabricator). |
| C3-R05 **Vault Ram** | 64 | Armor 2. Alternate Charge and Attack 18. Once per fight, if below 32 HP when selecting a Charge, announce an upgraded next attack of 26. | Cross a threshold with enough damage or saved defence to handle its announced response. No immediate surprise attack. | [Frog Knight](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Frog_Knight). |
| C3-R06 **Induction Auditor** | 54 | Utility Feedback 1, three activations maximum. Attack 10 plus Shield Leak 3; Attack 4 × 4; gain 2 Drive Gain. | Evaluate support actions against the future threat while keeping crafting choices open. | [Globe Head](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Globe_Head). |
| C3-R07 **Surveyor Crown** | 58 | Attack 12; Attack 4 × 4; Brace with Armor 5 through the next player turn and no attack; Beam 22, removing brace Armor before attacking. | A clear armoured preparation window precedes the strongest attack. | [Owl Magistrate](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Owl_Magistrate); Armor is our explicit brace definition. |
| C3-R08 **Siphon Pair** | Muzzle unit 46; Barrier unit 40 | Muzzle unit supplies Muzzle Damping 2 and repeats Attack 9. Barrier unit supplies Barrier Damping 4 and alternates Attack 6 with Repair Cycle 4 on itself. Each aura ends on its source's death. | Choose whether restoring offence or defence is more urgent; a kill restores the affected efficiency immediately. | [The Lost and Forgotten](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:The_Lost_and_Forgotten). |

### Officers

| ID / robot | HP | Behaviour | Counterplay | Inspiration |
| --- | ---: | --- | --- | --- |
| C3-O01 **Compliance Triad** | Riot 44; Muzzle 32; Barrier 32 | Riot alternates Attack 12 and gain 2 Drive Gain. Muzzle supplies Muzzle Damping 2 and alternates Attack 6 with Recover. Barrier supplies Barrier Damping 4 and alternates Recover with Attack 6. | Three different priorities: remove the scaler, restore damage, or restore Shield. All can be targeted. Do not combine this formation with the Siphon Pair. | [Knight Gang](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Knight_Gang). |
| C3-O02 **Citadel Breacher** | 88 | Armor 2, Packet Filter 3. Attack 8 plus Thermal Runaway 3; Wind Up and gain 3 Drive Gain; Attack 24. | Strip finite status protection, prepare during wind-up, and finish before the heavy cycle grows. | [Mecha Knight](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Mecha_Knight). |
| C3-O03 **Grid Conductor** | 90 | Beam 20; Attack 5 × 4; Attack 12 then apply Target Paint 4 and Drive Fault 4. | Handle different damage shapes and use the appropriate cleansing or assembly response before the next beam. | [Soul Nexus](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Soul_Nexus); source non-repeat randomness becomes a readable draft cycle. |

### Boss alternatives — one per city run

| ID / boss | HP | Main pattern | Distinct test | Inspiration |
| --- | ---: | --- | --- | --- |
| C3-B01 **Civic Command Core** with **Demolition Bailiff** | Core 112; Bailiff 78 | Core has Armor 3 and alternates grant 2 Drive Gain to Bailiff with apply Drive Fault 4. Bailiff cycles Attack 18, Charge, Attack 6 × 4. After Bailiff dies, Core loses that Armor and switches to Attack 20, apply Shield Leak 4, Attack 5 × 4. If Core dies first, Bailiff keeps fighting without further grants. | Choose which half to finish, understand the survivor, and preserve enough defence for the change. | [Queen](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Queen). |
| C3-B02 **Continuity Engine** | 180 | Three operating modes at above 120 HP, 61–120 HP and 1–60 HP. Guard mode has Armor 3 and alternates Attack 20 with Repair Cycle 6. Feed mode has no Armor and repeats Attack 4 × 4, gaining 1 Drive Gain after each action. Exposed mode alternates Attack 26 with Brace: Armor 5 for the next player turn and no attack. | Change damage shapes and defence as the same living chassis changes mode. No mandatory revival, HP floor or damage cap. | [Test Subject](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Test_Subject), adapted to threshold modes rather than separate mandatory lives. |
| C3-B03 **Chronometric Governor** | 166 | Armor 2, Packet Filter 2, Shot Telemetry. Cycle Attack 18, Attack 4 × 5, Recalibrate and gain 2 Drive Gain. | Several small shots remain legal but strengthen future enemy attacks; combine parts when beneficial without surrendering a winning multi-shot turn. | [Aeonglass](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Aeonglass). |

**Continuity Engine transitions:** during preparation, evaluate mode after each completed player Fire or other completed damage action if it survives; individual hits within one Fire never change mode midway through that Fire. HP changes after End Turn schedule the corresponding mode change for the next preparation start, preserving the already-announced enemy action. It can only advance to a later mode, even if repaired. Entering a new mode removes its previous phase-specific Armor and Drive Gain, keeps other statuses, and announces that mode's first action before the player commits another turn. The player turn continues when a preparation-phase transition occurs. A large hit may skip a mode or kill the boss; no damage is discarded to force a transition. Brace Armor expires before its next attack.

## Helpers and reinforcement accounting

Boot Pods have 6 HP. Their first enemy action is Boot, with no attack; after that they become 10-HP cutters at 10 current HP and Attack 5 on later turns. This is one living robot changing state, not a rewarded kill followed by a new life. Destroying a pod prevents its boot. The 6/10 figures are a proposed transformation, not healing available to the player.

The Municipal Assembly Rig has these four helper templates:

| Helper | HP | Behaviour |
| --- | ---: | --- |
| Screen Welder | 14 | Reinforcement Link 2 to its maker; Attack 4. |
| Signal Mite | 10 | Alternate Drive Fault 2 and Attack 4. |
| Seal Cutter | 12 | Attack 4. Only if this attack removes player HP, apply Shield Leak 2 for the following player turn. |
| Arc Spool | 14 | Attack 4, Runaway Motor 1. |

Helpers enter with their next-turn intent visible. A helper created in round R never acts before the enemy phase of round R + 1, including the mites released when Split Chassis dies during preparation. It can still be targeted by a later player action immediately after appearing. A destroyed summoner cancels its undeployed queue; already-deployed helpers remain and must be defeated or legitimately escape. The exception is Split Chassis's explicitly announced one-time death deployment. An encounter ends only after no live enemies remain and the player survives. A summoned entity contributes at most its assigned core allocation once, and the encounter generates its normal single recipe offer; spawning never generates extra three-recipe offers or an infinite loot loop. Exact core values remain economy work.

## Encounter pools and pacing

The selected city length remains 10–20 encounters including Mystery visits and the boss. These formations are candidates, not a fixed compulsory route or a copied STS2 map. Ordinary selection still offers three choices with at least one Regular; existing Officer/Mystery appearance preferences remain unchanged.

| City | Opening Regular candidates | Later Regular candidates | Officer slot | Boss slot |
| --- | --- | --- | --- | --- |
| Cinderwall | Two Rivet Mites; one Narrowband Emitter; the approved two-mite/Breach Ram lesson when ready | Pressure Cask + one mite; Cable Binder + one mite; Coil Sentinel alone; Coil Nest alone; Knuckle Press + one mite | One of C1-O01–03, initially without extra random support | Select one of C1-B01–03 and show it as the single final encounter |
| Coilbridge | Cargo Ram + Signal Loom; one Latch Harvester; one Magpie Courier | Cargo Ram + Screen Loader; Cargo Ram + Booster Cart; Feedback Pylon + Signal Loom; Brood Press alone | One of C2-O01–03; Coupled Hauler already contains its three bodies | Select one of C2-B01–03 |
| Glassward | Litany Engine alone; Aegis Tug + Beltgun Porter; one Vault Ram | Municipal Assembly Rig alone; Induction Auditor + one Signal Mite; Surveyor Crown alone; Siphon Pair | One of C3-O01–03; Compliance Triad already contains its three bodies | Select one of C3-B01–03 |

Initial pool guardrails: teach a new effect in isolation or with a familiar weak attacker; do not combine source-linked offence and defence suppression with permanent Reactive Mesh in the same formation; do not stack multiple summoners; and do not put every enemy on its heavy move at entry. Preserve at least one alternative formation that favours each of focused burst, multi-hit/spread play, defence and delayed setup. Strong builds are allowed to clear quickly; do not secretly increase enemy resistance to counter the player's chosen build.

Source easy/hard pools and differentiated act enemies informed the progression, but the source's exact room count, no-repeat rule, rewards and damage values are not adopted. [Acts](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Acts) and [monster formations](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Enemies) support that observation; our pool rules above are proposals.

## What to balance first

1. Use the existing ten-material haul proposal as one controlled fixture, not a locked economy. Repeat with different steering and actual character recipes. Verify complete legal turns, not just nominal damage per material.
2. Compare Armor encounters against Ablative Tiles encounters so neither one-large-shot nor many-small-shot play dominates the entire city.
3. Test the approved three-round example unchanged before adding new effects. Then test Cargo Ram's defensive opening, an Armor-provider pair, the finite summoner and one late boss.
4. Track HP loss across several fights with no automatic healing, not just isolated victories. Officer/boss values must reflect the recipe/upgrades available by that city, not a presumed Slay the Spire power curve.
5. Include global-cooling repeats, duplicated recipes, saved parts, part resale and finite shop stock in later economy/combat trials. Check whether indefinite stalling manufactures too much value; solve a demonstrated problem with visible enemy pressure, not an unrequested turn limit.

Two research agents reviewed the integrated roster and effects. Their findings corrected threshold triggers for damage-over-time, preserved announced attacks across boss transitions, clarified combined attack/repair actions, fixed exhausted summon queues and made summon/tick timing explicit. Numbers, tooltips and source coverage received document review only. No runtime, encounter simulation, player playtest or win-rate result exists. The useful next design review is choosing which city identities and enemy roles to retain, followed by a small set of costed example fights before implementation.

## Research provenance

Requested starting source: [Slay the Spire 2 wiki main page](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Main). Access date: 16 September 2026. Direct opens returned 403, while indexed individual wiki articles supplied substantial mechanics tables and update notes. Some extracts were months old or beta-marked. The studies record these limits, conflicting details and omitted deprecated content. This is not a claim to have tested the current STS2 build or transcribed every ordinary enemy.

All robot names, city concepts, written descriptions and proposed values here were composed for Overkill Foundry. Source links identify the tactical patterns used; source art, dialogue and encounter text were not imported. The 606 recipe rows and the owner's established turn, Shield, cooldown, death, escape and progression rules remain intact.
