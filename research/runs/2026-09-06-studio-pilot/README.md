# Studio research pilot — 2026-09-06

**Outcome: three draft opportunities; no next game selected.** This small desk-research pass checks the studio's workflow and evidence format. It does not prove a trend, validate fun, or forecast sales. Candidate titles are working names with no name clearance performed.

The pass deliberately spans tactile puzzles, turn-based strategy and active arcade play. It uses established games as comparables rather than pretending that a single look at a storefront establishes current acceleration. All three candidates have `trend_stage: unverified` and `confidence: low`.

## What was actually checked

All URLs below were opened through web browsing on 2026-09-06. Web results may use cached pages; this is an access date, not a guarantee that every displayed value was fetched live at that moment. No authenticated storefront dashboards or private data were used.

| Source | Observation / author report | What it does not establish |
|---|---|---|
| [A Little to the Left on Steam](https://store.steampowered.com/app/1629520/A_Little_to_the_Left/) | A purchasable organizing-puzzle game with visible review activity; a relevant category comparable. | Demand for our packing rules, exact sales, or a current growth curve. |
| [Max Inferno press kit](https://www.maxinferno.com/press) | The developer describes drag-and-drop interaction, multiple solutions, illustration and audio. | Independent audience confirmation; this is the same product's developer account. |
| [ISLANDERS on Steam](https://store.steampowered.com/app/1046030/ISLANDERS/) | A paid minimalist strategy comparable; the description explains placement interactions and repeated runs. | That a tide-based board game is wanted, balanced or commercially viable. |
| [Superflight on Steam, US/English](https://store.steampowered.com/app/732430/Superflight/?cc=us&l=english) | The inspected page offered USD 2.99; its developer describes brief runs with riskier proximity scoring. | Net receipts, sales volume, current genre acceleration or acceptance of tether controls. |
| [Steamworks reviews](https://partner.steamgames.com/doc/store/reviews) | Valve explains that review eligibility and contribution to the displayed score differ. | Any defensible fixed review-to-sales multiplier. |
| [Steamworks Coming Soon](https://partner.steamgames.com/doc/store/coming_soon) | The documented minimum Coming Soon visibility for a new product is two weeks. | A guaranteed launch date; review/onboarding and readiness can add time. |

The browsing pass did not transcribe review counts or convert them to sales. No interview, competitor hands-on play, footage analysis, search-interest series or participant playtest was conducted. These comparables were intentionally easy-to-inspect established products, so the sample has survivorship bias. A selection-quality run still needs close recent and weaker alternatives.

### Live snapshot check

During tooling integration, the studio helper successfully fetched the three comparables from Steam's public endpoints at 2026-09-06 15:06 UTC. The [saved JSON](steam-snapshot.json) contains exact request URLs, USD list/current prices, and all-language Steam-purchase review counts. This verifies that the snapshot command works with real data; one snapshot cannot establish growth or sales. The candidate scores and unverified timing labels were not upgraded because of these totals.

| Comparable | USD list / current price | Reviews with the recorded filter |
|---|---:|---:|
| A Little to the Left | 14.99 / 14.99 | 16,803 |
| ISLANDERS | 4.99 / 4.99 | 16,095 |
| Superflight | 2.99 / 2.99 | 11,042 |

## Three directions to test

Scores are editorial planning judgments, not measured conversion or fun. Dimensions are demand / distinctiveness / fun potential / scope fit / distribution; weighted totals use the studio contract. Alphabetical presentation is not a recommendation to select the top score.

| Draft | Distinct player promise | Scores | Weighted / 5 | First uncertainty |
|---|---|---|---:|---|
| [Latchline](../../opportunities/latchline.json) | One-button tether delivery with a bank-or-risk decision. | 2 / 2 / 2 / 4 / 2 | 2.50 | Does the latch/release feel readable and good enough to repeat? |
| [Parcel Paradox](../../opportunities/parcel-paradox.json) | Pack parcels whose neighbour effects must satisfy delivery conditions. | 2 / 3 / 2 / 4 / 2 | 2.70 | Are the consequences understandable without turning into rule bookkeeping? |
| [Tidepool Turn](../../opportunities/tidepool-turn.json) | Place creatures for the board that will exist after the next tide. | 2 / 3 / 2 / 3 / 2 | 2.45 | Does a forecast produce deliberate choices or confusing cascades? |

Each JSON contains a bounded test and observable continuation thresholds. Those thresholds are pragmatic early discovery criteria for a small sample, not statistical validation. No test has passed or even started.

## Owner effort and timing

The proposed prototypes are roughly one to two agent working days and about 1.5 owner hours each, including a compact review and five short observed sessions if participants are already available. These are unvalidated planning estimates; recruitment is excluded and could dominate calendar time. Full production, commercial content, art/audio, support and store/marketing work are separate. No cash spend is committed.

A near-peak opportunity could be worthwhile with a strong visible hook, low owner effort, a verified release window and a credible evergreen fallback. This pilot has no evidence to assign that stage to any of its three ideas. Steam's documented lead time means even an excellent two-day build must not be pitched as an automatic two-day commercial launch.

Next useful work, after a direction is chosen or prototype experimentation is authorized: search close competitors for the chosen mechanic, inspect a recent weak comparable, then run its smallest control/clarity test. For this pilot, the correct next state remains `draft`.

## Access and quality notes

- First attempts to open US/English parameterized URLs for A Little to the Left and ISLANDERS returned a browser-tool URL safety error. The ordinary canonical pages were subsequently opened successfully. Their region-specific price was therefore not used.
- Superflight's parameterized US/English page opened successfully. Its visible price is recorded with that URL; differing cached review totals across search/open results were not reconciled into a made-up precise metric.
- The Max Inferno press kit contains differing puzzle-count wording. This pilot cites the types of interaction and polish, not a supposedly exact current content count.
- Public entry points listed in `research/sources.json` but not used here are explicitly labeled untested. No failed source was treated as zero demand.
- Evidence informs original hypotheses; no competitor art, code, level designs, text or private Appdrip source were copied into a game.
