# Endgame and achievement references

Researched 18 September 2026. Source observations are separate from the original Overkill Foundry proposals. Direct wiki requests returned 403; indexed extracts provided the two requested pages. They may lag the live game. No STS2 executable, owner Steam account or achievement backend was inspected.

## STS2 observations

The [achievement page](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Achievements) labels its list unfinished and unavailable in game. Its patterns include character victories, maximum-difficulty clears, combat extremes, collections and an ending. We use those roles, not a claim about released Steam features. Confidence: high about the retrieved warning and listed concepts; limited about current implementation.

The [Ascension page](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Ascension) describes ten cumulative levels, independent character advancement after a final-act win, and continued access to lower levels. Its constraints affect recovery, economy, rewards and enemy pressure; the highest tier changes the final boss sequence. Confidence: high about the indexed design description, not exact parity with every current branch.

## Adaptation decisions — proposals, not source facts

| Reference role | Our proposed adaptation | Reason |
| --- | --- | --- |
| Independent character mastery | Ten Lockdown levels per mercenary after the proposed all-four entry gate. | Retains long-term specialization while following the owner's completion wording. |
| Cumulative difficulty | Reduced ordinary haul and base HP, with two enemy-stat pressure steps. | Our materials and HP carryover matter more than absent card draw/rest-site systems. |
| Combat extremes | Whole-number hit, Shield, recipe-use, saved-part and mercenary-meter feats. | Conditions can use existing actions and character exceptions. |
| Restriction challenge | Win without shop purchases, while Mayor and loot upgrades remain allowed. | A no-upgrade condition would conflict with the existing mandatory Mayor choice. |
| Collections | Versioned recipe/upgrade discoveries, with no new recipe unlock gates. | Seen offers already count for recipe Collection. |
| Ending and mastery | All-four Tier 5 signatures reveal CROWN-0; a separate hidden victory achievement celebrates its defeat. | Creates an original robot-command payoff before the highest optional tiers. |

The names, quantities, 40-item achievement set, Lockdown modifiers and CROWN-0 encounter are authored proposals. No source enemy/artwork/flavor text is imported. The ladder deliberately does not copy recovery penalties from a game with a different healing economy. The 24-win minimum and 864-encounter illustration are arithmetic from our own proposed threshold and existing 12-encounter fixture, not estimates of STS2 completion time.

## Implementation evidence

Official references: [Valve Stats and Achievements](https://partner.steamgames.com/doc/features/achievements), [ISteamUserStats](https://partner.steamgames.com/doc/api/ISteamUserStats), [Valve setup example](https://partner.steamgames.com/doc/features/achievements/ach_guide), and [Epic's achievement interface](https://dev.epicgames.com/documentation/en-us/unreal-engine/achievements-interface-in-unreal-engine). Specific API facts and the current deprecation note are recorded in [the integration plan](../STEAM-ACHIEVEMENTS.md), avoiding a second conflicting recipe for initialization.

## Next evidence needed

Calibrate all four mercenaries' HP and early recipe budgets before accepting tier penalties. Measure the full route's healing access rather than assuming city-entry recovery. Check whether players reach the four-signature goal while still wanting another run; the current Tier 5 threshold may be too late. Validate the final boss with real surviving City 3 builds, including supply/upgrades that override defaults. Test Steam storage and offline behavior only when the actual game project, provider and app configuration exist. No player, balance or platform-integration result is claimed by this research.
