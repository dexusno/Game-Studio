# Steam achievements — implementation plan

18 September 2026. Required product feature; **not implemented or configured on Steam**. The game currently has design records but no playable project to attach an achievement service to. This plan supplies the [40 proposed definitions](ACHIEVEMENTS.md), their [structured manifest](data/achievements.json), event semantics and acceptance cases for future implementation.

## Platform facts checked today

Valve supports stable API names, localized titles/descriptions, optional progress stats, hidden achievements and separate achieved/unachieved icons. Its current initial allowance is 100 achievements, so the proposed 40 fit. Steam's client supports offline achievement/stat caching; it later synchronizes that state. Our profile save should record gameplay facts, rather than maintain a competing copy of Steam's cache. [Valve: Stats and Achievements](https://partner.steamgames.com/doc/features/achievements).

`SetAchievement` changes local Steam state; `StoreStats` submits it and enables notification. `SetStat` writes an absolute value, not an increment. Store at meaningful boundaries and shortly after unlocks, rather than for every hit. `IndicateAchievementProgress` only shows progress; it does not itself complete the achievement. The current API page marks `RequestCurrentStats` deprecated while retaining older instructions beneath it: use the actual shipped SDK/provider's readiness contract, not an unexamined older sample. [Valve: ISteamUserStats](https://partner.steamgames.com/doc/api/ISteamUserStats).

Unreal provides an achievement interface for querying and writing achievements through an online provider. Keep the gameplay evaluator independent of the selected Steam/Unreal adapter so the same rules work when Steam is unavailable and in deterministic fixtures. Verify the supported interface against the actual engine/plugin selected during implementation. [Epic: Achievements Interface](https://dev.epicgames.com/documentation/en-us/unreal-engine/achievements-interface-in-unreal-engine).

## Data, ownership and progress

Keep API names such as `OF_CORE_CLEAR` stable even if the display name changes. Every definition has a readable requirement, event, typed condition, commit scope, hidden flag and optional progress stat. Use the same evaluator for the in-game achievement journal and Steam; do not maintain independently interpreted sets of conditions.

Gameplay unlocks belong to one profile. City/tier/signature/Core checks requiring all four mercenaries must be completed inside that profile. Steam achievements belong to the signed-in account: the account may earn separate achievements from different profiles, but Steam ownership never unlocks another profile's Lockdown or Core access. Associate pending platform requests with the Steam account that earned them; switching accounts must not publish someone else's results.

Record immutable committed gameplay receipts with profile ID, campaign UUID, encounter ID, rules version, tier, mercenary and an event ID. Track sets for cities, discovered item IDs and qualifying encounters; track maxima for tier clearances. A repeated callback or loading the same receipt must be a no-op. A completed encounter is not counted again when its result screen is reopened.

For account-facing progress bars, use the greatest valid single-profile progress for these definitions, not the sum of profiles. A profile with 30 recipes and another with 25 does not satisfy the 50-recipe requirement. Reconcile with existing Steam progress without decreasing it when the player starts a new profile. Published account unlocks stay earned even if the local profile is later deleted. The completion meta-achievement uses the account's 39 other launch achievements; it does not grant gameplay unlocks.

The recipe and upgrade completion requirements reference **versioned launch ID sets**, not the current array length. The draft sets contain 606 recipes and 288 upgrades and must be reconciled with the actual shipping content before publication. Exclude debug, inaccessible and unreleased entries. Later content does not silently extend an already-published requirement. Discovery means a legitimate visible offer or initial ownership; a full developer catalogue view does not discover every item. Recipes already use seen-offer discovery; extending this convention to upgrades is an explicit proposal. Do not add a no-duplicate/rebalanced reward algorithm merely to accelerate collection.

## Event and transaction semantics

The [shared timing/save contract](TIMING-AND-PERSISTENCE.md#6-save-transactions-and-crash-recovery) now owns the local atomic commit, fight-entry rollback, stable reward/choice receipts and post-commit Steam outbox. These achievement predicates consume that contract; they do not create a second save or reward mechanism.

| Event family | Qualification and commit |
| --- | --- |
| City / route / Core | Commit only after the required enemies are defeated and the mercenary survives. A normal route is all three cities in one campaign. A Core defeat cannot undo the route clearance already committed before entry. |
| Fight feats | Accumulate a candidate during the fight; commit at its terminal victory or defeat. Quit/Continue discards the unfinished attempt's candidates and restores the original fight-entry state. Feats explicitly requiring victory fail on defeat; a completed damage/production feat may still qualify in a lost fight. |
| Discovery | Commit once when the legitimate offer becomes visible or a starter is owned. Declining, dying or restarting later does not erase an already-seen item. |
| City/campaign counters | Add only committed distinct encounter receipts; terminal wins alone advance victory counters. An escape-only ending is not a victory for these achievements. |
| Meta-completion | Evaluate after constituent unlocks are reconciled. Exclude the meta-achievement itself; repeated sync is idempotent. |

Specific conditions prevent ambiguous or trivial accounting:

- **Overkill Certified:** one positive direct hit attributed to the mercenary, a part or Bolt; after actual damage reductions and before clamping to the target's remaining HP. Count valid overkill, but never combine separate main/spread/support hits or count a hit against an already-dead target. Recoil and damage-over-time are not direct player hits.
- **Unbroken Chassis:** count actual HP lost to any enemy-origin damage during a boss encounter, including recoil and enemy damage-over-time. Healing does not erase it. Own HP payments do not disqualify it. Explicit interception that prevents HP loss is allowed.
- **Wall of Your Own:** usable active Shield immediately before a real enemy attack, including an explicitly retained balance; uninstalled reserve parts and depleted contributions do not count.
- **Assembly Line:** count successful recipe Use events, including legitimate free/extra Uses permitted by specific effects. Creating a copied part or a passive upgrade grant is not another recipe Use. Failed attempts do not count.
- **Yesterday's Work:** six distinct ordinary parts created in earlier rounds and consumed by the same Fire, including an explicitly permitted Fire sacrifice. Shield parts merely installed alongside that shot are not consumed by it. Loading/unloading cannot change age.
- **Running Hot:** the final required enemy in an Officer encounter must die to that main shot; use the Heat snapshot supplied by the existing Hot Barrel calculation. Achievement evaluation never changes the order of costs or later gains. A side hit or an escape ending cannot qualify.
- **Through the Casing:** the enemy must be alive immediately after reaching the stack amount; a blocked application or pre-application preview is insufficient.
- **Partner in Demolition:** count actual direct enemy HP lost to Bolt-attributed hits; exclude overkill, status ticks, blocked damage and the mercenary's unrelated hits.
- **Discharge Authority:** count Charge actually paid by Charged Barrel. Refunds do not reverse a valid payment, but another payment must be legal. Failed payments and other Charge costs are excluded.
- **Steady Claw:** three different committed fights with a successful Precision attempt in the same city. Extra attempts in one fight remain one qualifying fight; quitting and replaying cannot multiply it.
- **Off the Market:** a completed purchase in any normal or Mystery shop disqualifies the campaign, even if an upgrade makes its price zero. Selling is allowed; free loot/Mayor grants outside shops are allowed. Do not turn mandatory Mayor acceptance into an impossible no-upgrade requirement.
- **Built for Anything:** twelve distinct currently active permanent upgrade IDs at route clear. Temporary borrowed items, expired items and innate abilities are excluded.

On defeat, commit eligible fight feats and the death result atomically before clearing the run. On victory, commit rewards, city/tier facts and eligible achievement receipts once. A crash between the gameplay save and Steam submission must cause a safe replay of the same unlock request, not another reward or another count. Steam absence or a failed submission must never block play or invalidate a legitimate local achievement. Reconcile the committed journal when the provider is available; do not reset the user's Steam statistics to repair a local problem.

## Backend configuration and presentation

When the game's Steam App ID and development access are available, configure the 40 stable names, English text, supported localizations, hidden flags and appropriate client-set stats. Produce original achieved/unachieved artwork and record asset provenance. Four campaign crests, four mercenary mastery emblems, mechanical feat symbols and collection motifs give the set a coherent visual family. Give **The Last Order** a distinct command-core emblem and profile celebration. This is presentation, not an invented Steam rarity category.

The three finale entries are hidden on Steam; the in-game four-signature goal remains visible without exposing the boss name. Once the Core is revealed, show the actual encounter objective in game. Avoid popup spam during shot/impact resolution: queue the in-game announcement for a readable boundary, while preserving provider submission timing.

Configure, publish and validate the schema through the normal Steamworks development workflow before claiming integration. Do not use SpaceWar or another product's ID as evidence that our own achievements are configured. No Steam account, App ID, application record, API call, icon upload or public achievement definition has been changed in this assignment. The [Valve implementation guide](https://partner.steamgames.com/doc/features/achievements/ach_guide) is the setup reference, not evidence of completed setup.

## Acceptance cases and implementation order

1. **Local evaluator:** feed boundary cases just below/at thresholds, independent profiles, all four tier records and all manifest IDs. Verify the meta list excludes itself. Test invalid/dead targets, paid versus passive events and achievement-specific damage accounting.
2. **Save/restart:** quit an unfinished fight after a feat, restart with the same seed, finish once, reopen the result and reload. No duplicated totals, rewards or Steam requests; discovery remains a set. Test route-clear commitment followed by Core defeat.
3. **Steam adapter:** query known schema, earn one real test achievement, store it, inspect success/failure callbacks and confirm on an authorized test account. Restart offline, earn another, reconnect and verify preservation. Test provider unavailable, account switch and retry after storage failure.
4. **Presentation/build:** inspect achieved/locked icons, localization, hidden finale text, progress bars and packaged-game behavior. No debug console or developer injection should satisfy release progression; ordinary accessibility options and the selected same-seed Continue behavior remain allowed.
5. **Release reconciliation:** freeze reachable launch collections and verify every achievement against actual shipped content. Full collection/mastery achievements are optional and never gate the story finale. Record the App ID, build and actual test evidence at that stage.

Current evidence is a data/design validation only. A static manifest or arithmetic probe does not constitute working Steam achievements.
