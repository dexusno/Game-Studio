# First beta tuning — Overkill Foundry

**20 September 2026 · profile `beta-balance-v1` · initial values, no gameplay evidence**

Klaus authorizes a concrete tuning pass using STS2 as a starting benchmark, then fine tuning through actual play. This document sets those starting numbers across four mercenaries and three city tiers. The [JSON profile](data/beta-balance-v1.json) holds the exact values. These are assistant-assigned beta inputs under that authorization, not individually owner-selected final values. Existing rules and explicit character/recipe/upgrade exceptions take precedence. The chosen size of the first playable is still separate.

The [calculation script](analysis/beta_tuning.py) and [reproducible results](analysis/beta_tuning.json) check costs, eligibility, prices and arithmetic. They do not execute combat. There is no independent Python combat engine, measured win rate, successful campaign, or graphical approval behind this profile.

## Benchmark and adaptation

Source observations checked on **20 September 2026**:

| STS2 observation | Our starting choice and reason |
| --- | --- |
| Ironclad starts at 80 maximum HP and its starting relic restores 6 HP after combat. [Character page](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Ironclad) | Start all four mercenaries at **80/80**. Their abilities already differ; establish their strength before adding a second difference through HP penalties. Our rules grant no automatic victory/city healing, so provide reliable access to a paid healing recipe. |
| Starting Gold is 99; ordinary enemy rewards are listed as 10–20, Elites 35–45 and bosses 100 at base difficulty. [Gold page](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Gold) | Start with **100 Credits**. City 1 core values use a comparable scale, with tougher-city core values increasing. These remain cores to collect and sell, not automatic cash drops. |
| Normal card reward base odds are 60% Common / 37% Uncommon / 3% Rare **before** an offset that changes over the run. Elite and shop tables differ. [Cards page](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Cards) | Use source-specific recipe weights and stronger later-city offers. Our fourth tier and selected ban on Regular Legendary drops require our own tables. Start without a pity accumulator; do not mislabel the source's base table as its actual fixed reward rate. |
| Mawler has 72 HP, opens with 4 × 2 damage, and can subsequently attack for 14; its random pattern has repeat restrictions. [Mawler page](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Mawler) | Visible pressure, preparation windows and constrained patterns are useful benchmarks. Those attacks represent 10% and 17.5% of an 80-HP bar before defence. Our uncapped part assembly, full recipe access and material economy make an identical enemy HP conversion unjustified. |

These are observations from the requested community wiki, not a claim of Mega Crit's internal tuning formula or an immutable current release. Search-indexed wiki extracts were available; a direct STS2 Merchant fetch returned 403. **Our shop prices below are original tuning choices**, not verified STS2 Merchant prices. The Cards page carries a beta-content warning. Confidence is adequate for reference scales, insufficient for declaring our game balanced.

## Player, materials and recipe power

| Input | Beta value |
| --- | --- |
| Starting/current maximum HP | 80/80 for Mara, Ivo, Ada and Noor |
| Recipe memory / starting kit | Existing 20 slots; 8 shared + 4 mercenary starters |
| Every ordinary haul, all cities | **3 Iron + 2 Copper + 1 Carbon + 1 Glass + 1 Circuit**, plus **2 of one chosen material**: ten total |
| Additional automatic fight-entry materials | 0; the first haul is the ordinary haul |
| Precision, once per fight before exceptions | Miss +0, Good +1, Perfect +2 of the selected material, subject to available bonus stock; baseline never lost |
| Mara | Heat 0 initially, cap 10, decay 2 after enemy phase; Hot Barrel +1 main-shot damage per full 2 Heat, without spending it |
| Ivo | First successful Acid Etch application per turn adds 2 Target Paint under the authored targeting rules |
| Ada | Bolt starts each fight at 6/12; Field Service repairs active Bolt for 2 after the first qualifying paid command per turn |
| Noor | Charge 0 initially, cap 12, no round decay; optional barrel payment 0–3 Charge for +2 main-shot damage each |

This adopts the retained character drafts for the first test profile, including Field Service. Quench Recovery and Residual Current remain unselected alternatives. No trait supplies a free partless shot, free Helper command or automatic player healing. Meter/helper lifetimes and explicit upgrade overrides follow [timing and persistence](TIMING-AND-PERSISTENCE.md).

The 606 printed recipe costs, cooldowns and effects become the v1 candidate set **without a blanket rewrite**. Current mean material costs are approximately 1.67 Base, 2.36 Common, 3.17 Uncommon, 4.12 Rare and 5.24 Legendary. Costs are not effect values: Heat, Charge, HP, cooldowns, targets and conditions still matter. Full kits cost 21 materials to use each starter once (Ada 22), so a ten-material haul gives choices rather than every action.

For a concrete shared turn, SH001 + SH003 + SH004 produce **13 direct damage, or 17 against an attacking main target**. SH002 + SH006 provide **10 Shield**. Together they cost 3 Iron, 2 Copper, 1 Carbon and 2 Glass: eight materials, affordable by steering Glass. Those are additive printed values before enemy mitigation, reactions and mercenary effects. Every one of the twenty character/steering combinations also has affordable Ammo-and-Shield subsets; this does not prove every subset's effects are usable or a fight is winnable.

Keep ten materials in later cities initially: rarer efficient recipes, the growing collection and accumulating Mayor/permanent upgrades already increase power. Raising free supply simultaneously would obscure which system caused runaway growth. Later-city effective damage benchmarks are **17 / 24 / 32 per player turn**, to be measured after mitigation. They are test targets, not an assumed automatic bonus.

## Recovery and shop economy

Reserve one ordinary shop recipe slot for **SH074 Quick Patch at 75 Credits whenever no copy is owned**, including initial stock. It is optional to buy and occupies a normal recipe-memory slot. Its printed cost remains 1 Iron + 1 Glass + 1 Circuit; it heals 4 on Use, cooldown 2, at most 8 restored HP per fight shared by all copies. Purchasing it does not heal. It can be used on entry to a later fight to recover earlier damage, but the player still pays and uses a live combat action. Quick victories may give only one useful healing opportunity. Do not assume an 8-HP refund from every fight.

The starting 100 Credits can buy this recipe and leave 25. This avoids requiring a lucky healing offer or a Mystery visit to support a full route. It does not guarantee that every run needs it, or that this is the final recovery balance. Other recipes and explicit upgrades can provide different recovery plans.

| Product | Credit price |
| --- | --- |
| Raw Iron / Copper / Carbon / Glass / Circuit | **4 / 4 / 4 / 5 / 6** per unit |
| Recipe knowledge: Base / Common / Uncommon / Rare / Legendary | **30 / 50 / 75 / 150 / 250** |
| Permanent upgrade: Common / Uncommon / Rare / Legendary | **120 / 180 / 260 / 400** |
| Civic Credit Bond, UGS-085 | **110**, satisfying its printed undiscounted-price-above-100 condition |
| Vendor Access Badge, UGS-075 | **160**; its 20% discount needs substantial future spending to pay back |
| Ready-made Plain Slug / Flat Plate | **6 / 10**; main-recipe material value plus 2 Credits for immediate availability |

No random price variance in v1. Use printed discount exceptions and floor once, minimum 1 Credit for a priced product unless explicitly free. Raw-material stock per refresh is **4/3/2/2/1**, then **5/4/2/2/2**, then **6/4/3/3/2** across the three cities in the material order above. These full baskets cost **52 / 66 / 79 Credits**. Recipe offer slots are 4/5/6; permanent-upgrade slots 2/2/3; each knowledge/upgrade offer has quantity one. Stock also includes two Plain Slugs and two Flat Plates. All use the existing finite stock and completed-fight-only refresh rule. Between-fight purchases are staged under the timing contract.

Part resale remains half the fixed main-recipe material value, floored per part, with the current shop resource prices. For example a Plain Slug sells for 2, Flat Plate for 4, and Simple Sighting's output for 4. The recipe knowledge itself is not sold. No automatic end-of-fight sale or raw-material sale is introduced.

**One narrow catalogue correction:** UGS-017 Salvage Contract Stamp now grants its 25% additional income **when an energy core is sold**, floored per core. Its earlier target was direct victory Credits, which our ordinary reward rules do not supply. It still excludes part sales and other direct grants; it obeys income restrictions. A 5-Credit core pays 6, a 15-Credit core 18, and a 100-Credit core 125. Group selling computes the same per-core results. All other 287 upgrade effects and all recipe rows are preserved.

## Rewards, cities and robots

Use the existing **twelve-city registry and four distinct mercenary route graphs**, with twelve encounters per city for this beta fixture. Preserve three nonadjacent Officer opportunities and three Mystery opportunities in the eleven ordinary positions, the opening gates, and a single final boss. There is always a Regular option. The new Mystery category weights are **40% Regular combat / 30% merchant / 30% tech support**; individual noncombat outcomes still need authoring. These weights do not create a healing service or extra restock.

Recipe rarity percentages per slot, ordered **Common / Uncommon / Rare / Legendary**:

| Source | City 1 | City 2 | City 3 |
| --- | --- | --- | --- |
| Regular | 65 / 32 / 3 / **0** | 55 / 39 / 6 / **0** | 45 / 45 / 10 / **0** |
| Officer | 20 / 60 / 18 / 2 | 10 / 55 / 30 / 5 | 5 / 45 / 40 / 10 |
| Boss | 0 / 0 / 85 / 15 | 0 / 0 / 70 / 30 | 0 / 0 / 55 / 45 |
| Shop, random recipe slots | 50 / 40 / 9 / 1 | 35 / 45 / 17 / 3 | 20 / 45 / 28 / 7 |

Show three distinct recipe IDs, choose one or skip. Already-owned knowledge remains eligible because duplicate copies are allowed. Quick Patch's reserved shop slot is not a random roll. Filter source, mercenary and city first, then roll rarity and a uniform eligible ID. Remove selected IDs within the offer; renormalize permitted nonempty rarities if necessary. Base recipes are the starting kit rather than random rewards. No hidden guarantee of synergy is added.

The JSON also specifies normal upgrade odds. Rarity does not override source or city restrictions: for example City 1 has no eligible ordinary Legendary upgrades, and some later mercenary pools also lack that tier. Renormalize within eligible tiers; do not import a Mayor-exclusive item to fill a slot. Existing Mayor lane/condition checks remain; within each lane, target Rare/Legendary weights **80/20 → 50/50 → 20/80**. Item effects already escalate by city, and rarity alone is not the power calculation. One Mayor gift is chosen from three each city.

The current **42 robot entries, their whole-number HP, attacks, statuses and authored pattern constraints** are the v1 enemy inputs, at 100% HP and +0 global damage. Default Rivet Mites use 7 HP; the approved teaching encounter retains its explicit 6-HP/7-HP pair and 24-HP, Armor-3, 18-damage Breach Ram. Do not multiply every later enemy by an additional city factor: they already have authored tier differences.

| City | Boss starting HP totals, including initial companions | At assumed effective throughput | HP/throughput envelope |
| --- | --- | --- | --- |
| 1 | 92 / 100 / 84 | 17 damage/turn | 6 / 6 / 5 turns |
| 2 | 136 / 132 / 154 | 24 damage/turn | 6 / 6 / 7 turns |
| 3 | 190 / 180 / 166 | 32 damage/turn | 6 / 6 / 6 turns |

This arithmetic supports starting near **2–4 turns for ordinary fights, 3–6 for Officers and 5–8 for bosses**. It is not a combat prediction: Armor, repairs, retaliation, target changes and mechanics can alter throughput and duration. Adopt the draft HP scale first rather than importing STS2 HP wholesale. Move a formation later or adjust its own values if a real entry build cannot handle it.

Core sale values are specified for **every roster body and finite helper** in the JSON. Examples: a Rivet Mite pays 5, Breach Ram 15, Cargo Ram 22, Vault Ram 30; ordinary Officer encounter totals are 40/50/60 and boss totals 100/125/150 across city tiers. Split Chassis splits its 40 between the chassis and its two death mites. A summoned helper pays only if deployed and destroyed; a Boot Pod's transformation does not create a second core. Escaped robots pay nothing.

For purchasing arithmetic only, reference Regular encounters worth **15 / 25 / 35** plus the respective bosses produce **265 / 400 / 535 Credits per all-Regular city**, or **1,300 including starting money** over three cities if all cores are sold. Taking three Officers instead gives reference city income 340/475/610, plus the three upgrade rewards. Actual formations have different totals; these are budgeting examples, not sampled average payouts. Sales, Mystery outcomes, spending and economy upgrades are additional variables.

## Pressure targets, known issue and next measurement

For a competent developing build at base difficulty, begin by looking for **0–6 gross HP lost in ordinary fights, 8–16 in Officers and 12–24 in bosses**. Across an all-Regular city, a working paid recovery plan should generally leave roughly **10–20 net HP lost**. Keeping similar pressure while opponents and builds both grow is the objective. These are investigation bands, not limits on enemy attacks, player damage, or permitted strategy; do not secretly alter rolls to meet them. Officer routes must be judged with their acquired upgrades, not simply by adding damage to an unchanged starter build.

**Confirmed numerical loophole, not merely an untested win rate:** two starter Rivet Mites attack for 10 total each round. With Glass steering, the player can spend eight of ten materials on SH002 + SH006 for 10 Shield and sell unused outputs of SH001 + SH003 + SH004 for **2 + 2 + 4 = 8 Credits**. All five recipes have no numeric cooldown and can be used again next round. The player never fires, loses no HP, and gains Credits without limit under that formation's current pattern. Twenty rounds already produce 160 Credits. Finite shop inventory does not prevent banking money for later shops.

An owner question is pending: announce overtime in round 5 and, from round 9, give living robots in ordinary fights +1 attack damage per hit each round. **This is not active in v1.** It changes enemy behaviour, so it is not smuggled in as arithmetic tuning. Nor does this pass introduce a global shot cap, storage limit or sale limit. If selected, specify the exact timing and check the same witness again; even escalating pressure would still require tests of powerful recovery/defence engines.

Adopt the existing [ten-tier Lockdown numerical ladder](ENDGAME-PROGRESSION.md) as later test inputs: ordinary hauls fall from 10 to 7, starting base HP from 80 to 64, enemy HP rises to 110%, and tier 6 onward adds 1 to each positive attack hit. Preserve integer rounding, phase thresholds, the two steered units and full later max-HP upgrade gains. Unlock/finale choices remain separately labelled proposals; Lockdown is not a prerequisite for the first playable.

After the shared core exists, use at least **100 paired seeds per mercenary** for the first automated pass, retaining initial states, content version and full action/event traces. Start with Auto collection; then compare Learning/Practised/Expert precision assumptions, adding an expected 0.55/1.15/1.65 materials **per fight**, not per round. Compare ordinary routes and Officer-taking routes, then supply changes and individual outliers. Measure gross/net HP, turns, resource deficits/surpluses, recipe and upgrade selection, damage prevented, actual healing, Credits by source and stalling. Match seeds and change one major input at a time. More runs and uncertainty estimates follow observed variation; a hundred seeds is an initial batch, not proof of parity.

Human sessions must still establish comprehension, meaningful choices, pacing, precision feel and graphics approval. No bot result can supply that approval. Prioritize normal, recovery-poor and deliberately exploit-seeking policies; a policy that always attacks will miss the proven farming loop.

## Reproduction and current evidence

Run `python games/overkill-foundry/design/analysis/beta_tuning.py --write` to reproduce the compact report. It checks the pinned 606 recipe rows, 288 upgrade records, all 42 core mappings, permitted reward pools, twenty starter/steering fixtures, shop affordability, the farming witness and eleven Lockdown fixtures. Source or profile changes require a reviewed version/pin update. The upgrade catalogue validator separately checks its generated review documents and eligibility structure.

The remaining MVP work is the chosen boundary's runtime content manifest, individual Mystery outcomes, implementation, and actual shared-core and human playtests. Baseline HP, ordinary material quantities, initial economy prices and offer weights are now **specified for the first tests**, rather than waiting for another numerical design discussion.
