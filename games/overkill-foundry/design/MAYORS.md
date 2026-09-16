# Mayors — city-entry permanent upgrades

16 September 2026. Companion to the [three-city robot roster](ENEMY-ROSTER.md).

## Selected role

Klaus defines Mayors as the equivalent of Slay the Spire 2's Ancients: at the beginning of each City, the Mayor grants a **Rare or Legendary permanent upgrade**. Keep the existing presentation of **three upgrades to choose from**. An acquired upgrade lasts for the current campaign and activates on acquisition; it is not a profile-wide unlock or an automatic heal between fights.

The wiki describes an Ancient encounter at act entry and three offered boons. Its names, unlock gates, appearance probabilities and compulsory-selection rule are not adopted. Our Mayor role and rarity are owner decisions; the identities, offer contents and numerical values below are proposals. Sources: [Acts](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Acts), [Ancients](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Ancients), and the dated [agent research](research/2026-09-16-sts2-act-3-effects-mayors.md).

## City identities and first offer candidates

Mayors belong to the fortified cities, while bosses command hostile robots in the surrounding districts. These proposed civic identities do not select character models, portraits or a wider political story.

### Cinderwall — Mayor Irena Vale

A municipal engineer keeping the foundry city's defensive workshops operational. Her offer establishes whether the campaign leans toward stronger manufactured ammunition, greater survival capacity or a prepared opening supply.

| ID / upgrade | Rarity | Exact proposed effect | Why choose it |
| --- | --- | --- | --- |
| MY1-01 **Feed Press** | Rare | Once each player turn, the first Ammo recipe that produces a direct-damage part adds 2 damage to its first output part. The added damage belongs to that physical part and stays with it until used. Other outputs from a batch gain nothing. | More efficient ammunition, including a part saved for a later round. No innate gun damage is added. |
| MY1-02 **Impact Liner** | Rare | On acquisition increase maximum HP by 10 and heal 10 current HP. No further healing trigger. | Immediate survival and a larger recovery ceiling, using the selected max-HP/healing rule. |
| MY1-03 **Reserve Cassette** | Legendary | At each fight start grant one Plain Slug from SH001's part type and one Basic Shield Plate from SH002's part type: initially 6 damage and 6 Shield. These are fresh physical parts, not active Shield or retained inventory from the previous fight. Owning those recipes is not required. | A flexible opening and a choice between spending or storing the parts. |

### Coilbridge — Mayor Tomas Reed

A freight coordinator whose city survives by keeping damaged production and transport networks moving. His offer improves recipe reuse, Shield manufacture or longer-term build flexibility.

| ID / upgrade | Rarity | Exact proposed effect | Why choose it |
| --- | --- | --- | --- |
| MY2-01 **Coolant Recirculator** | Rare | Once each player turn, after a Utility use actually removes at least one cooldown counter, recover 1 Copper. Pay the Utility's full costs first. Clearing several recipes with that one global effect still returns only 1 Copper. | Supports cooling combinations without creating an uncapped refund on every reset. It does not refresh a no-cooldown recipe's spent use. |
| MY2-02 **Plate Binder** | Rare | Once each player turn, the first Shield recipe that produces a Shield-granting part adds 3 Shield to its first output part. The extra value remains on the part if saved. It activates with that part only on End Turn. | Supports saving defence for a known heavy turn. It neither activates Shield on craft nor preserves active Shield after the enemy turn. |
| MY2-03 **Archive Rack** | Legendary | Increase recipe memory capacity by 4. At the beginning of player turn 3 of each fight, also grant one fresh Powder Cap from SH003's part type, initially 3 damage. No grant if the fight ends before that trigger. | Allows a wider set of answers and a predictable later-turn part. Capacity alone neither fills new slots nor grants recipe knowledge. |

### Glassward — Mayor Nadi Sol

A civic signals archivist maintaining a protected communications district. Her offer addresses the late roster's reactions and heavy attacks without suppressing the player's earned combinations.

| ID / upgrade | Rarity | Exact proposed effect | Why choose it |
| --- | --- | --- | --- |
| MY3-01 **Backflow Ground** | Rare | Reduce each incoming enemy Reactive Mesh recoil event by 2, minimum zero. It does not reduce ordinary attacks, status ticks or HP costs. | Enables more aggressive multi-hit play against the stated reaction while retaining other enemy threats. |
| MY3-02 **Opening Relay** | Rare | At the start of the first player turn of every fight, grant 6 active Shield. This fresh upgrade grant is available during preparation and follows the ordinary enemy-turn-end reset. | Can protect early recoil and the first enemy phase. It is an explicit upgrade trigger, not a change to normal crafted Shield activation. |
| MY3-03 **Shield Vault** | Legendary | At enemy-turn end retain up to 6 remaining active Shield instead of clearing that retained amount. Everything above 6 resets. The retained Shield can absorb later damage and adds normally to future fresh Shield grants. | An explicit permanent-upgrade exception to the selected reset rule. It saves only existing unspent protection and grants none by itself. |

## Offer and stacking boundaries

These are the first three candidates per Mayor, illustrating one possible three-option screen. The draft does not select a guaranteed two-Rare/one-Legendary distribution, global rarity weights or a complete offer pool. Mayor identity is tied to the city for this proposal; variation between campaigns can be designed later if useful.

Proposed implementation default for these nine named modules: each can be acquired only once in a campaign; its own once-per-turn/fight trigger remains one trigger. Expanded pools should replace already-owned entries rather than offer accidental duplicate stacking. Different named upgrades may combine, with their actual effects previewed. This is a concrete draft rule, not a claim that the owner has already selected all upgrade equip/stack limits.

Existing choices remain intact: temporary gathering recipes stay separate from permanent upgrades; shop/Officer acquisition routes stay available; Mayor claw-upgrade inclusion is not silently added by these weapon/defence/memory examples. There is no shop healing service. Unused raw materials and parts still clear at fight end, and a subsequent fight's grants are fresh supplies.

The part-sale baseline remains each part's main recipe's normal one-part ingredient value. These upgrade bonuses do not replace that baseline with the part's current damage or Shield value. Duplication/refund/sale interactions still need later economy tests.

## Review and test questions

- Does each three-option offer give the current build a useful choice instead of one universally best upgrade?
- Does the first city's offer remain useful against all three proposed boss alternatives?
- Are fresh start-of-fight grants, retained active Shield and saved unactivated parts visibly different?
- Do cooling refunds and resale remain finite under repeated recipe use and duplicate copies?

All nine effects are proposed beta content. Their triggers have been checked against the written turn, cooldown and persistence rules, but their strength, rarity, prices elsewhere and player appeal have not been playtested. No source relic text, artwork or values were copied into these upgrades.
