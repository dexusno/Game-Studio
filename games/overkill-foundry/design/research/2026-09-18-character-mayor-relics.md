# Character and Mayor upgrade research — 18 September 2026

## Selected brief and working proposal

The owner requests relic-like permanent upgrades, including high-tech items that explicitly change defaults. Mayor upgrades draw specifically on Ancient relics. Specific upgrades are scoped exceptions; their presence does not repeal ordinary rules or existing mercenary traits. This is content design, with no runtime or playtest result.

Working count: 36 ordinary mercenary upgrades (9 each), plus 102 Mayor upgrades (34 for each city wave). Each Mayor wave contains 22 shared options and 3 options for each mercenary: the current mercenary sees 25 eligible entries. Mayor entries are exclusive to the assigned wave's entry reward and cannot be acquired early from shops, Officers or Mystery rewards. This guarantees enough unowned eligible choices without duplicate stacking or emergency fallback filler. Four innate abilities remain separate and are not counted as acquired upgrades.

A Mayor offer draws one shared infrastructure choice, one shared tactical choice and one eligible mercenary choice. All are Rare or Legendary; no exact power equality is asserted. Wave 1 establishes a build, wave 2 makes it repeatable, and wave 3 supports a large payoff or a powerful explicit exception. Power is relative to remaining encounters: a late payout must be useful before campaign end. The broad pool is authored content, not a request to implement all entries before testing the loop.

## Sources and access

Accessed 18 September 2026. The indexed [wiki relic list](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Relics_List) states 298 entries. The five character pages each list 7 ordinary/shop exclusives plus a starter and its upgraded variant: [Ironclad](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Ironclad), [Silent](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Silent), [Regent](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Regent), [Necrobinder](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Necrobinder), [Defect](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Defect). Character identity and category counts were directly checked in indexed wiki text.

The [Ancients overview](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Ancients) describes act-entry choices and separate offer pools. The [public Spire Codex data](https://raw.githubusercontent.com/ptrlrd/spire-codex/main/data/eng/relics.json) supplies a machine-readable reference for 296 entries. This is community-extracted game data, not an official game API. Its mechanical values are a reference snapshot and are not assumed to match the latest beta. Dowsing Rod and Neow's Sacrifice explain two beta additions; see [v0.109.0 beta notes](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:V0.109.0_-_Beta_Patch).

Observed branch differences include Diamond Diadem (snapshot: low-card-turn damage reduction; indexed wiki: opening Block with one reset exception), Fur Coat (7 versus 8 marked rooms), Signet Ring (999 versus 888 Gold), Seal of Gold (5 versus 3 Gold), Divine Destiny (6 versus 7 Stars), and Toy Box (4 versus 5 temporary relics). These are source-version differences, not interchangeable definitions. Root's combined reference inventory records both where observed. No source flavor text or art is reproduced.

## Adaptation decisions

- Keep equipment, manufacturing, economy, memory, route information and claw technology in the same upgrade system. Weapon and Shield bonuses are only part of the pool.
- Adapt source card-draw ideas into immediate part supply, recipe access, controlled refunds or bounded recipe reuse. Our entire installed recipe collection is already visible; card draw is not a mechanic to import literally.
- Do not copy automatic between-fight healing from a source starter as an ordinary rule. An explicit named upgrade may heal at its printed trigger.
- Translate source Energy into materials or recipe availability only with full costs, trigger order and reset boundaries stated. No silent action cap is added. An explicit tradeoff item may limit a behavior within its own stated scope.
- Ivo's Weaken remains flat per-hit reduction with phase decay, not a percentage debuff. Ada upgrades preserve paid commands and repair versus restore. Noor upgrades do not remove Charged Barrel; Mara upgrades do not remove Hot Barrel.
- A physical Shield grant creates an ordinary removable Shield part; installed parts protect automatically. Explicit retained Shield is a separate carried balance after the usual enemy-phase reset. There is no new Shield activation button.
- Seven earlier Mayor prototypes migrate unchanged to the normal shared pool. Reserve Cassette stays an opening Mayor supply; Shield Vault moves to wave 1 because retaining 6 Shield is too small to headline a late Mayor. These are rarity/distribution proposals, not owner-approved balance.

## Implementation boundary and acceptance

No engineer-owned game implementation exists. The producer must agree event hooks, explicit upgrade actions, generated-part references, reward filtering, deterministic counter serialization and visible cooldown/limit indicators with the implementing engineer before scheduling these systems. Data below specifies behavior, not proof of feasibility or fun. No new combat engine, turn system, art or audio is created in this research pass.

Acceptance example: acquire Shield Vault, end an enemy phase with 9 Shield, then carry 6. On the next phase an attack spends that 6 normally; reinstalling a part cannot refresh the spent amount. A Mayor offer must contain three distinct unowned eligible entries and never another mercenary's technology. A free extra recipe use must still pay its material/HP/meter costs unless the item explicitly discounts those costs. All cap/once triggers survive reopening UI and same-seed Continue.

Playtest question: can the player explain why they chose their Mayor upgrade, then point to one later decision changed by it? Compare early reliability, middle engine choices and final-wave payoff separately; more damage alone does not demonstrate an interesting choice.

## Agent discussion and revisions

The two researchers exchanged actual draft criticism and revised their files:

1. The shared researcher challenged an extra-player-round service interval: it required a second round clock and ambiguous status/flee advancement. The proposed Mayor payoff became Emergency Haul Satellite: one extra haul after turn 4's actual ordinary Collect finishes, without any altered turn or enemy clock.
2. They found Servo Claw Coupler's offered steering change redundant with the existing choice. The item now supplies three extra units of that haul's chosen material and introduces no pretend choice.
3. They challenged Prototype Equipment Trunk's expiry because removing maximum-HP, memory or acquisition bonuses could strand state. Its temporary pool now excludes acquisition grants, permanent stat/capacity changes, recipe changes and persistent scheduled changes. Earned historical item IDs stay ineligible for reacquisition under the root contract.
4. Reciprocal review identified the shared Precision item's new per-turn prohibition as incompatible with the Mayor immediate-retry exception. That prohibition was removed. A pre-haul steered grant moved after Collect, and Coolant Recirculator now clearly grants fresh Copper even if no Copper was paid.
5. Root integration review caught a circular fourth-paid-use discount condition, absent enemy Shield in the current roster, the draft Field Service dependency, and a cumbersome shop-token expiry. Rotating Toolhead now checks three earlier paid uses; Thermal Punch bypasses Armor; Service Cradle explicitly requires Field Service; Council Purchasing Token persists until spent or replaced without UI reopening resets.
6. Root review caught Very Hot Cocoa's encoded [energy:4] quantity. All 21 source entries containing Energy/Star numeric icons were re-read as encoded amounts; the compact fact table now records four opening Energy. The other encoded amounts matched. Black Blood's boundary was also corrected from victory to combat ending.

These changes address implementability, rule compatibility and readability. They are static design corrections, not evidence of fun or calibrated balance.

Structural checks on the completed file: 138 unique IDs and names; 36 normal character items (9 each); 102 Mayor items (34 per wave). Every wave has 22 Shared entries and 3 per mercenary, providing 25 eligible candidates for each route. Infrastructure/tactical/mercenary lanes contain at least three candidates apiece. All 102 Mayor references use Ancient source category. Every normal item inspired by Common, Uncommon or Rare preserves that exact rarity. Shop/Starter adaptations explicitly state their proposed power-tier rationale. All 145 requested stable character/Ancient reference rows have one compact fact; zero missing or extra IDs.

## Compact source facts for integration

The following tables summarize source mechanics, not source wording. Numeric facts refer to the public data snapshot unless a variant is stated above. Icons encode Stars or Energy and must not be discarded when parsing.

| Source ID | Source category / owner | Mechanical fact paraphrase |
| --- | --- | --- |
| ALCHEMICAL_COFFER | Ancient / shared | Four additional potion spaces, initially stocked. |
| ARCANE_SCROLL | Ancient / shared | One random Rare card awarded on acquisition. |
| ARCHAIC_TOOTH | Ancient / shared | One starter card becomes its ancient form. |
| ASTROLABE | Ancient / shared | Three selected cards transform and upgrade. |
| BEAUTIFUL_BRACELET | Ancient / shared | Three selected cards receive Swift 3. |
| BIG_HAT | Rare / necrobinder | Opening hand receives two random Ethereal cards. |
| BIIIG_HUG | Ancient / shared | Remove four cards now; later shuffles create Soot. |
| BLACK_BLOOD | Starter / ironclad | Combat ending restores 12 HP. |
| BLACK_STAR | Ancient / shared | An Elite victory yields one extra relic. |
| BLESSED_ANTLER | Ancient / shared | One extra Energy per turn; three Dazed inserted each fight. |
| BLOOD_SOAKED_ROSE | Ancient / shared | One extra Energy per turn, with an Enthralled card added. |
| BONE_FLUTE | Common / necrobinder | An Osty attack supplies 2 Block. |
| BOOK_REPAIR_KNIFE | Uncommon / necrobinder | A non-Minion killed by Doom restores 3 HP. |
| BOOKMARK | Rare / necrobinder | At turn end one random retained card becomes 1 cheaper until played. |
| BOOMING_CONCH | Ancient / shared | Elite opening: two extra cards and one extra Energy. |
| BOUND_PHYLACTERY | Starter / necrobinder | Each player-turn opening summons 1. |
| BRILLIANT_SCARF | Ancient / shared | The fifth hand-played card each turn has zero cost. |
| BRIMSTONE | Shop / ironclad | Each turn adds player Strength 2 and enemy Strength 1. |
| BURNING_BLOOD | Starter / ironclad | Combat-end healing amount is 6 HP. |
| CALLING_BELL | Ancient / shared | Three relics come with a unique Curse. |
| CHARONS_ASHES | Rare / ironclad | Each exhausted card causes 3 damage to every enemy. |
| CHOICES_PARADOX | Ancient / shared | Opening choice among five random cards; chosen card gains Retain. |
| CLAWS | Ancient / shared | Up to six selected cards become Maul. |
| CRACKED_CORE | Starter / defect | One Lightning Orb at combat opening. |
| CROSSBOW | Ancient / shared | Each turn supplies a random Attack, free for that turn. |
| CURSED_PEARL | Ancient / shared | Greed added in exchange for 333 Gold. |
| DATA_DISK | Common / defect | Fight begins with Focus 1. |
| DELICATE_FROND | Ancient / shared | Fill vacant potion spaces at every combat opening. |
| DEMON_TONGUE | Rare / ironclad | The first HP loss during each player turn is followed by equivalent healing. |
| DIAMOND_DIADEM | Ancient / shared | Snapshot: enemy damage halved on turns with at most two card plays. Wiki beta differs. |
| DISTINGUISHED_CAPE | Ancient / shared | Trade 9 maximum HP for three Apparition cards. |
| DIVINE_DESTINY | Starter / regent | Opening Stars: six in snapshot; wiki beta reports seven. |
| DIVINE_RIGHT | Starter / regent | Opening resource grant: three Stars. |
| DRIFTWOOD | Ancient / shared | One replacement roll permitted for each card reward. |
| DUSTY_TOME | Ancient / shared | Acquisition provides an Ancient card. |
| ECTOPLASM | Ancient / shared | Each turn gets one Energy, but future Gold income is disabled. |
| ELECTRIC_SHRYMP | Ancient / shared | A selected Skill receives Imbued. |
| EMOTION_CHIP | Rare / defect | Prior-turn HP loss repeats every Orb passive at next turn opening. |
| EMPTY_CAGE | Ancient / shared | Remove two deck entries on acquisition. |
| FENCING_MANUAL | Common / regent | Combat opens with Forge 10. |
| FIDDLE | Ancient / shared | Draw two extra at turn opening; later in-turn drawing is prohibited. |
| FISHING_ROD | Ancient / shared | Each third normal combat upgrades one random deck card. |
| FUNERARY_MASK | Uncommon / necrobinder | Three Soul cards enter the draw pile each combat. |
| FUR_COAT | Ancient / shared | Snapshot marks seven future encounters whose enemies start at 1 HP; wiki beta reports eight. |
| GALACTIC_DUST | Uncommon / regent | Ten cumulative Stars spent produces 10 Block. |
| GLASS_EYE | Ancient / shared | Receive two Common cards, two Uncommon cards and one Rare card. |
| GLITTER | Ancient / shared | All future card-reward choices have Glam. |
| GOLD_PLATED_CABLES | Uncommon / defect | The rightmost Orb repeats its passive once. |
| GOLDEN_COMPASS | Ancient / shared | Act 2 route map becomes a special single path. |
| GOLDEN_PEARL | Ancient / shared | Immediate currency grant: 150 Gold. |
| HEFTY_TABLET | Ancient / shared | Select one of three Rare cards, and receive Injury. |
| HELICAL_DART | Rare / silent | Each Shiv play provides one temporary Dexterity for that turn. |
| INFUSED_CORE | Starter / defect | Opening has three Lightning Orbs; Lightning damage increases by one. |
| IRON_CLUB | Ancient / shared | Each four card plays produces one draw. |
| IVORY_TILE | Rare / necrobinder | Playing a card costing at least three Energy returns one Energy. |
| JEWELED_MASK | Ancient / shared | Opening hand gains a random deck Power, free for that combat. |
| JEWELRY_BOX | Ancient / shared | Add Apotheosis on acquisition. |
| KALEIDOSCOPE | Ancient / shared | Two rewards drawn from other characters' card pools. |
| LARGE_CAPSULE | Ancient / shared | Two random relics plus one extra Strike and Defend. |
| LAVA_ROCK | Ancient / shared | Act 1 boss reward includes two relics. |
| LEAD_PAPERWEIGHT | Ancient / shared | Select one of two Colorless card choices. |
| LEAFY_POULTICE | Ancient / shared | Transform one Strike and one Defend; maximum HP decreases by 12. |
| LOOMING_FRUIT | Ancient / shared | Immediate maximum-HP increase of 31. |
| LORDS_PARASOL | Ancient / shared | Visiting a Merchant grants their entire inventory. |
| LOST_COFFER | Ancient / shared | Immediate card reward plus one random potion. |
| LUNAR_PASTRY | Rare / regent | Turn ending grants one Star. |
| MASSIVE_SCROLL | Ancient / shared | Choose one of three multiplayer-card options. |
| MEAT_CLEAVER | Ancient / shared | Unlocks Cook as a Rest Site action. |
| METRONOME | Rare / defect | The seventh Orb channel in a combat deals 30 to all enemies, once. |
| MINI_REGENT | Rare / regent | First Star expenditure each turn grants Strength 1. |
| MUSIC_BOX | Ancient / shared | First Attack played each turn creates an Ethereal copy. |
| NEOWS_BONES | Ancient / shared | Two random Neow relics plus a random Curse. |
| NEOWS_TALISMAN | Ancient / shared | Upgrade one Strike and one Defend. |
| NEOWS_TORMENT | Ancient / shared | Add the Neow's Fury card. |
| NEW_LEAF | Ancient / shared | Transform one selected card. |
| NINJA_SCROLL | Shop / silent | Opening hand receives three Shivs. |
| NUTRITIOUS_OYSTER | Ancient / shared | Maximum HP increases by 11. |
| NUTRITIOUS_SOUP | Ancient / shared | Every Strike gains Tezcatara's Ember enchantment. |
| ORANGE_DOUGH | Rare / regent | Opening hand receives two random Colorless cards. |
| PAELS_BLOOD | Ancient / shared | Turn-opening hand draw increases by one. |
| PAELS_CLAW | Ancient / shared | Every Defend receives Goopy. |
| PAELS_EYE | Ancient / shared | Once per combat, a zero-card turn exhausts the hand and grants an extra turn. |
| PAELS_FLESH | Ancient / shared | From turn 3 onward, receive one extra Energy per turn. |
| PAELS_GROWTH | Ancient / shared | A selected card receives Clone. |
| PAELS_HORN | Ancient / shared | Add two Relax cards. |
| PAELS_LEGION | Ancient / shared | Doubles one card's Block contribution, then sleeps for two turns. |
| PAELS_TEARS | Ancient / shared | Leaving Energy unused grants two extra Energy next turn. |
| PAELS_TOOTH | Ancient / shared | Remove five cards; after each combat, one randomly returns upgraded. |
| PAELS_WING | Ancient / shared | Two forfeited card rewards can purchase one relic. |
| PANDORAS_BOX | Ancient / shared | Transform every Strike and Defend. |
| PAPER_KRANE | Rare / silent | Weak's damage suppression becomes 40 percent instead of 25 percent. |
| PAPER_PHROG | Uncommon / ironclad | Vulnerable's extra damage becomes 75 percent instead of 50 percent. |
| PHIAL_HOLSTER | Ancient / shared | One additional potion space; two random potion grants. |
| PHILOSOPHERS_STONE | Ancient / shared | One extra Energy each turn; every enemy opens with Strength 1. |
| PHYLACTERY_UNBOUND | Starter / necrobinder | Summon 5 at fight opening and Summon 2 each turn. |
| POMANDER | Ancient / shared | Upgrade one selected card. |
| POWER_CELL | Rare / defect | Opening hand receives two zero-cost cards from the draw pile. |
| PRECARIOUS_SHEARS | Ancient / shared | Remove two cards while losing 16 HP. |
| PRECISE_SCISSORS | Ancient / shared | Remove one selected card. |
| PRESERVED_FOG | Ancient / shared | Remove three selected cards and add Folly. |
| PRISMATIC_GEM | Ancient / shared | One extra Energy per turn; card rewards can use other colors. |
| PUMPKIN_CANDLE | Ancient / shared | One extra Energy per turn for five combats; Rest Sites can renew it. |
| RADIANT_PEARL | Ancient / shared | Opening hand receives Luminesce. |
| RED_SKULL | Common / ironclad | Strength increases by 3 while HP is at most half maximum. |
| REGALITE | Uncommon / regent | Creating a card supplies 2 Block. |
| RING_OF_THE_DRAKE | Starter / silent | First three turn openings each draw two extra cards. |
| RING_OF_THE_SNAKE | Starter / silent | Combat opening draws two extra cards. |
| RUINED_HELMET | Rare / ironclad | First Strength gain per combat doubles its amount. |
| RUNIC_CAPACITOR | Shop / defect | Three additional Orb spaces each combat. |
| RUNIC_PYRAMID | Ancient / shared | The hand is retained across ordinary turn endings. |
| SAI | Ancient / shared | Every turn opening supplies 7 Block. |
| SAND_CASTLE | Ancient / shared | Upgrade six random deck cards. |
| SCROLL_BOXES | Ancient / shared | Choose between two authored card packs. |
| SEA_GLASS | Ancient / shared | Inspect fifteen cards from another character and take any subset. |
| SEAL_OF_GOLD | Ancient / shared | Snapshot: pay 5 Gold per turn for one Energy; wiki beta price is 3. |
| SELF_FORMING_CLAY | Uncommon / ironclad | In-combat HP-loss events schedule 3 Block for next turn. |
| SERE_TALON | Ancient / shared | Three Wish cards arrive with two random Curses. |
| SIGNET_RING | Ancient / shared | Snapshot immediate grant: 999 Gold; wiki beta lists 888. |
| SILKEN_TRESS | Ancient / shared | Forfeit all Gold; first card reward receives Glam throughout. |
| SILVER_CRUCIBLE | Ancient / shared | First three card rewards upgraded; first treasure chest gives nothing. |
| SMALL_CAPSULE | Ancient / shared | Receive one random relic immediately. |
| SNECKO_EYE | Ancient / shared | Two extra turn-opening draws, but begin combats Confused. |
| SNECKO_SKULL | Common / silent | Poison applications receive one extra stack. |
| SOZU | Ancient / shared | One extra Energy per turn, with potion acquisition disabled. |
| SPIKED_GAUNTLETS | Ancient / shared | One extra Energy per turn; Powers cost one additional Energy. |
| STONE_HUMIDIFIER | Ancient / shared | Each Rest adds 5 maximum HP. |
| STORYBOOK | Ancient / shared | Add Brightest Flame on acquisition. |
| SYMBIOTIC_VIRUS | Uncommon / defect | One Dark Orb at each combat opening. |
| TANXS_WHISTLE | Ancient / shared | Add the Whistle card. |
| THROWING_AXE | Ancient / shared | First card played each combat resolves an additional time. |
| TINGSHA | Uncommon / silent | Discarded cards during your turn each deal 3 to a random enemy. |
| TOASTY_MITTENS | Ancient / shared | Each turn exhausts the draw pile's top card and adds Strength 1. |
| TOUCH_OF_OROBAS | Ancient / shared | Starter relic replaced by its Ancient variant. |
| TOUGH_BANDAGES | Rare / silent | Each card discarded during your turn grants 3 Block. |
| TOY_BOX | Ancient / shared | Snapshot grants four temporary Wax relics; eldest expires every three combats. Wiki beta grants five. |
| TRI_BOOMERANG | Ancient / shared | Three chosen Attacks receive Instinct. |
| TWISTED_FUNNEL | Uncommon / silent | All initial enemies receive Poison 4. |
| UNDYING_SIGIL | Shop / necrobinder | Enemy damage halves when its Doom is at least its HP. |
| VELVET_CHOKER | Ancient / shared | One extra Energy per turn, with at most six card plays that turn. |
| VERY_HOT_COCOA | Ancient / shared | Combat opening grants four additional Energy. |
| VITRUVIAN_MINION | Shop / regent | Cards named with Minion get double damage and Block. |
| WAR_HAMMER | Ancient / shared | Each Elite victory upgrades four random cards. |
| WHISPERING_EARRING | Ancient / shared | One extra Energy per turn; Vakuu controls the first turn. |
| WINGED_BOOTS | Ancient / shared | Three route selections can disregard path connections. |
| YUMMY_COOKIE | Ancient / shared | Upgrade four selected cards on acquisition. |

Two additional beta Ancient definitions are in the root reference inventory: Dowsing Rod grants the Dowsing card; Neow's Sacrifice gives Ambergris plus the Guilty card. Linked card mechanics require their own definitions; they are not silently inferred here.
