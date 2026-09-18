# Shared relic research and upgrade design

18 September 2026. Research and authored design proposals, with no runtime implementation or balance/playtest claim. The 150 authored rows live in [shared-upgrades.json](../data/shared-upgrades.json); the producer integrates their rules and Mayor/mercenary companions.

## Source and coverage

The requested [wiki relic index](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Relics_List) is indexed by search, but direct retrieval failed during this study. [STS2.app's relic index](https://sts2.app/docs/relics/) explicitly credits the public [Spire Codex project](https://github.com/ptrlrd/spire-codex) for extracted game data. Its [English relic snapshot](https://github.com/ptrlrd/spire-codex/blob/05dbf2eb917d754ab28360b598a0c82931a309ef/data/eng/relics.json) provides a reproducible fallback. It is a community-maintained extraction, not an official Mega Crit API or a claim about the owner's installed game version.

The last commit touching that stable file is **05dbf2eb917d754ab28360b598a0c82931a309ef**, dated **19 June 2026**, with its message referring to v0.107.1. Observed count: **296 entries**; 251 Shared plus nine each for five characters. Rarity categories: 30 Common, 40 Uncommon, 50 Rare, 30 Shop, 35 Event, 100 Ancient, 10 Starter and one unspecified Circlet entry. This is the stable snapshot's count, not a current all-branches count. The producer separately verified two later beta additions, Dowsing Rod and Neow's Sacrifice, against the [v0.109.0 beta notes](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:V0.109.0_-_Beta_Patch). The combined reference inventory therefore has 298 named entries with branch labels; definitions from different versions must not be silently mixed.

My shared subset is **all 150** non-Ancient, non-placeholder Shared entries: 25 Common, 30 Uncommon, 35 Rare, 25 Shop and 35 Event. Every one maps to one authored original upgrade. Character and Ancient definitions belong to the companion agent's inventory section. The final reference retains concise mechanical facts, source identities and links, without flavor text, artwork or copied descriptive prose.

Scaling inference, not a source fact: keep the 150 shared ordinary/event/shop slots and 102 Ancient-inspired Mayor slots, replace five sets of nine character slots with four sets of nine, and omit Circlet as a gameplay placeholder. This yields **288 acquirable upgrades**. Existing innate character abilities are not counted again. The finished allocation is a design proposal informed by source scale, not a requirement to implement all 288 for the demo.

## Translation principles

Cards' draws and energy do not map literally into this game. The player sees their stored recipes and pays materials, so those source roles become supply timing, recipe reuse, selected discounts, memory options or new offer choices. Deck size and rest sites are not imported as hidden new systems. The aim is a comparable range of decisions: reliable openings, saved-part builds, repeated firing, costly production, Utility chains, claw timing, recovery, shop investment and route information.

Upgrades may explicitly modify normal defaults. Examples include one additional Precision attempt, four raw materials carried between fights, a loaded bullet retained through one End Turn, extra no-cooldown recipe Uses and a once-per-campaign defeat prevention. These exceptions apply only in the stated scope. They never repeal Hot Barrel, Charged Barrel, Find the Seam, Bolt or an unrelated recipe exception.

All direct Common/Uncommon/Rare inspirations preserve their source tier, following the owner's rarity clarification. Shop and Event are preserved as source categories; each adaptation also has a proposed power rarity and rationale. The shared result is 35 Common, 57 Uncommon and 58 Rare after assigning those category-only entries. Seven earlier Mayor prototypes are preserved by ID and effect in the normal pool. Their old draft rarity may change to match the owner's newer parity instruction; this is not a silent change to their functionality.

Fake, inert and punishment relics are recorded faithfully as source facts, but do not force fake or useless rewards into our new game. Their authored slots become information tools, finite supply leases or explicit benefit/cost bargains. These are looser role inspirations, clearly distinguishable from one-to-one mechanical copies.

## Implementation dependencies

There is no playable implementation in this game's src directory. The producer must agree these data needs with the implementing engineer before assigning runtime work: ordered event hooks; stable recipe-copy and physical-part identities; item-local counters; five-material cost calculation; reward provenance; deterministic forecasts; additional memory-slot types; bounded extra-use and retention rules; save serialization; and a preview of consequences before optional paid equipment actions.

Seven shared items preserve the earlier Mayor prototypes: Feed Press MY1-01; Impact Liner MY1-02; Coolant Recirculator MY2-01; Plate Binder MY2-02; Archive Rack MY2-03; Backflow Ground MY3-01; Opening Relay MY3-02. Reserve Cassette and Shield Vault remain with the Mayor agent. Plate Binder now says installed parts protect automatically; no Shield activation button is introduced.

Generated plain damage and Shield parts use the producer's canonical SH001/SH002 mapping, with explicit named exceptions such as SH003. Changing a part's printed value or giving it free does not change its main-recipe resale basis. Resource returns and free-part grants have finite item-local triggers where necessary. No global shot, part or resource cap has been added.

## Observable checks and smallest experiment

- With Utility Recall Buffer, use one no-cooldown Utility, observe its one extra use, pay again, then see that the item supplies no third use. A second recipe copy retains its own ordinary allowance.
- With Interfight Freight Seal, choose four surviving material units after victory; the next fight receives exactly those units and no other old inventory. Reloading the same entry cannot duplicate cargo.
- With Plate Binder, produce the first eligible Shield part, save it, install it later and take two attacks. The extra 3 belongs to that part; reinstalling cannot refill depleted Shield.
- With Second Precision Coupler, see two attempts at fight entry, spend each on separate chosen turns regardless of success, and still take only one ordinary haul each round.
- With Dry Hopper Reserve plus discounts, empty material stock through a paid Use, receive the one payout, then confirm subsequent zero-stock states in that turn do not repeat it.
- With Bounty Transponder and escape-only completion, receive no victory payout. Player death stops remaining rewards.
- With a source-tagged copied recipe, verify whether the exact effect says the enhancement follows that physical copy or a newly generated copy. Do not infer inheritance.

The smallest integration experiment is one fight fixture and six contrasting items: Ready Rack, Plate Binder, Coolant Recirculator, Haul Sorter, Authorized Pattern Seal and Interfight Freight Seal. The first five exercise opening, protection, reuse, collection and explicit use-limit exceptions; cargo requires a second fight entry. This is a proposed test slice, not a reduction of the commissioned roster.

Playtest question: **Which upgrade changed a choice you made—what to craft, when to fire, what to save or where to spend—and was its trigger visible enough to plan around?** Hypothesis: predictable timings and narrow exceptions produce more useful planning than invisible percentage gains. Readability, challenge and pacing need player evidence; catalogue size and static checks do not establish fun.

## Agent discussion and revisions

The character/Mayor researcher proposed an extra player interval with a private cooldown/haul boundary. I challenged the new partial-turn state and recommended a scheduled extra haul or a fully defined skipped enemy phase. The revised Mayor draft uses an extra fourth-turn haul without changing turn clocks, cooldown ticks or flee timing. This preserves a large tempo reward with fewer new timing concepts.

I also flagged retirement of borrowed upgrades with capacity/acquisition effects as ambiguous, requested the extra-haul trigger wait for the player's ordinary Collect resolution, and identified redundant steering text. The sibling integrated these points by restricting temporary acquisitions, moving the haul trigger and simplifying the steering effect.

The sibling found three real shared-roster ambiguities: the second Precision attempt must not prohibit another item's immediate retry; a periodic steered-material grant cannot read an unchosen steering option before the haul; and Coolant Recirculator's Copper is a fresh grant, not necessarily a refund of Copper paid. All three are corrected. The producer additionally caught an information item revealing already-known bosses and an Armor Breach Probe expecting enemy Shield absent from the current robot roster. The former now previews two Mystery categories; the latter reacts to actual Armor mitigation. These revisions improve useful choices and observable triggers without weakening unrelated character exceptions.

Final static checks passed: 150 unique shared upgrade IDs/names/source mappings; all 298 source names have mechanical summaries and provenance; 296 stable plus two beta entries; 102 Ancient source entries; all direct ordinary rarity mappings match; all seven migrated prototype IDs remain. Numeric source checks preserved Energy icon amounts, with the companion table corrected to four Energy for Very Hot Cocoa before integration. I independently checked all 102 Mayor rows for Ancient provenance and Rare/Legendary tier, and sampled late-wave extra uses, cooling, equipment acquisition and retirement for finite triggers and explicit timing. This is document/data verification, not combat execution, economic calibration or player feedback.
