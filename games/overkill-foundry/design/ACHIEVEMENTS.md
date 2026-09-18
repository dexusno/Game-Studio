# Achievement roster

18 September 2026 · **40 proposed Steam achievements** · Original names and gameplay conditions

See [Lockdown and the finale](ENDGAME-PROGRESSION.md), [Steam integration and exact event semantics](STEAM-ACHIEVEMENTS.md), and [source observations](research/2026-09-18-endgame-and-achievements.md). The [structured definitions](data/achievements.json) contain stable proposed API names, typed conditions, events and launch ID sets. This is a generated design document; no Steam achievements have been configured or unlocked.

**Qualification:** use one profile wherever stated. Ordinary legitimate campaigns and the selected same-seed Continue behavior qualify. Combat feats commit at the fight's terminal result; explicitly victorious feats require survival. Collection discovery records visible offers, including declined offers. Debug-injected state does not qualify. Achievements never grant stronger recipes or modify another profile's gameplay progression.

**Inspiration:** adapt the challenge roles described in the [STS2 achievement reference](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Achievements). Its wiki lists unfinished content, so this is not a parity claim with released Steam features. Our conditions use material crafting, installed Shield, saved parts and the four mercenaries' existing mechanics.

## Progression

| API name | Achievement | Player-facing requirement | Hidden |
| --- | --- | --- | --- |
| `OF_FIRST_CITY` | **Breach Closed** | Clear your first city. | No |
| `OF_CAMPAIGN_MARA` | **Cinder to Glass** | Clear all three cities in one campaign as Mara. | No |
| `OF_CAMPAIGN_IVO` | **Salt, Steel and Silence** | Clear all three cities in one campaign as Ivo. | No |
| `OF_CAMPAIGN_ADA` | **Every Rivet Counts** | Clear all three cities in one campaign as Ada. | No |
| `OF_CAMPAIGN_NOOR` | **A City of Light** | Clear all three cities in one campaign as Noor. | No |
| `OF_TWELVE_CITIES` | **Twelve Cities Standing** | Clear all twelve mercenary cities in one profile. | No |

## Lockdown

| API name | Achievement | Player-facing requirement | Hidden |
| --- | --- | --- | --- |
| `OF_LOCKDOWN_1` | **Under Restrictions** | Clear a campaign at Lockdown 1 or higher. | No |
| `OF_LOCKDOWN_5_MARA` | **Heat Under Pressure** | Clear Lockdown 5 as Mara. | No |
| `OF_LOCKDOWN_5_IVO` | **Nothing Stays Sealed** | Clear Lockdown 5 as Ivo. | No |
| `OF_LOCKDOWN_5_ADA` | **Built to Outlast** | Clear Lockdown 5 as Ada. | No |
| `OF_LOCKDOWN_5_NOOR` | **Current Against the Grid** | Clear Lockdown 5 as Noor. | No |
| `OF_LOCKDOWN_10_MARA` | **The Furnace Holds** | Clear Lockdown 10 as Mara. | No |
| `OF_LOCKDOWN_10_IVO` | **The Last Seal Breaks** | Clear Lockdown 10 as Ivo. | No |
| `OF_LOCKDOWN_10_ADA` | **Unbreakable Partnership** | Clear Lockdown 10 as Ada. | No |
| `OF_LOCKDOWN_10_NOOR` | **The Grid Answers** | Clear Lockdown 10 as Noor. | No |
| `OF_ALL_LOCKDOWN_10` | **No City Left Behind** | Clear Lockdown 10 with all four mercenaries in one profile. | No |

## Finale

| API name | Achievement | Player-facing requirement | Hidden |
| --- | --- | --- | --- |
| `OF_CORE_REVEAL` | **The Voice Behind the Walls** | Reveal the source of the robot commands. | Yes |
| `OF_CORE_CLEAR` | **The Last Order** | Defeat CROWN-0 after clearing Lockdown 5 with every mercenary. | Yes |
| `OF_CORE_ALL` | **Fourfold Shutdown** | Defeat CROWN-0 with all four mercenaries in one profile. | Yes |

## Combat

| API name | Achievement | Player-facing requirement | Hidden |
| --- | --- | --- | --- |
| `OF_SHIELD_40` | **Wall of Your Own** | Enter an enemy attack with at least 40 usable Shield. | No |
| `OF_BOSS_UNSCATHED` | **Unbroken Chassis** | Defeat a boss without losing mercenary HP to enemy damage during that fight. | No |
| `OF_ONE_HP` | **One Vital Sign** | Win a fight with exactly 1 HP remaining. | No |
| `OF_HIT_100` | **Overkill Certified** | Resolve at least 100 damage in one direct hit. | No |
| `OF_FIVE_SHOTS` | **Keep Feeding It** | Fire five valid shots in one player turn. | No |
| `OF_TEN_RECIPES` | **Assembly Line** | Successfully Use ten recipes in one player turn. | No |
| `OF_SAVED_SIX` | **Yesterday's Work** | Consume six parts saved from earlier rounds in one Fire. | No |
| `OF_NO_FIRE` | **Tools Do the Talking** | Win a fight without using Fire. | No |

## Mercenary

| API name | Achievement | Player-facing requirement | Hidden |
| --- | --- | --- | --- |
| `OF_MARA_HEAT` | **Running Hot** | Finish an Officer encounter with a main shot fired at 8 or more Heat. | No |
| `OF_IVO_ACID` | **Through the Casing** | Reach 20 Acid Etch on one living robot. | No |
| `OF_ADA_BOLT` | **Partner in Demolition** | Have Bolt deal 30 actual direct HP damage in one fight. | No |
| `OF_NOOR_CHARGE` | **Discharge Authority** | Pay a total of 12 Charge through Charged Barrel in one fight. | No |

## Collection

| API name | Achievement | Player-facing requirement | Hidden |
| --- | --- | --- | --- |
| `OF_RECIPES_50` | **Workshop Notebook** | Discover 50 distinct recipes in one profile. | No |
| `OF_RECIPES_ALL` | **The Complete Blueprint** | Discover every recipe in the launch collection. | No |
| `OF_UPGRADES_50` | **Field Technology** | Discover 50 distinct permanent upgrades in one profile. | No |
| `OF_UPGRADES_ALL` | **Every Circuit Accounted For** | Discover every permanent upgrade in the launch collection. | No |

## Mastery

| API name | Achievement | Player-facing requirement | Hidden |
| --- | --- | --- | --- |
| `OF_OFFICERS_5` | **Chain of Command** | Defeat five Officer encounters in one campaign. | No |
| `OF_PRECISION_3` | **Steady Claw** | Succeed at Precision collection in three different fights in one city. | No |
| `OF_NO_SHOP` | **Off the Market** | Clear a campaign without buying anything from a shop. | No |
| `OF_UPGRADES_12` | **Built for Anything** | Clear a campaign while owning twelve distinct permanent upgrades. | No |

## Completion

| API name | Achievement | Player-facing requirement | Hidden |
| --- | --- | --- | --- |
| `OF_ALL_ACHIEVEMENTS` | **Foundry Legend** | Earn all other launch achievements. | No |

## Collection and final-goal boundaries

The draft launch collections contain 606 recipes and 288 upgrades. Their explicit ID sets must match shipping content before publication; unreleased/debug content is excluded and later additions do not silently move the target. The two full-collection goals are optional, potentially long-term achievements. Neither is required for CROWN-0. The last achievement requires the other 39 launch achievements; it is separate from the hidden finale victory achievement.

**The Last Order** is the proposed special narrative achievement: defeat CROWN-0 after all four Tier 5 signatures. **Fourfold Shutdown** rewards repeating that finale with each mercenary. **No City Left Behind** covers all-four Tier 10 mastery. A distinct icon/crest can make the narrative reward special; Steam rarity is not an authored tier in this manifest.

## Validation

The design check validated 40 distinct API names, the 39-item nonrecursive meta set, the exact draft collection IDs, tier progression boundaries and the four-mercenary finale gate. It also produced a starter-cost sensitivity probe. None of these checks exercises a runtime achievement evaluator or Steam API.
