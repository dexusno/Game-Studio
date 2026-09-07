# Tactile salvage candidate — 2026-09-06

Assigned question: can a premium Windows microgame built around a magnet/crane offer a distinctive, replayable interaction in roughly four to six focused agent working days? This is competitor validation and a design hypothesis, not evidence of a rising trend. No game has been selected, built, played, or greenlit by this note.

Read: studio charter, workflow, team instructions, game-research skill, research method/source catalog, and the three existing pilot opportunities. The producer owns candidate JSON and the shared Steam snapshot; this note owns only the tactile lane.

## Recommendation

**Conditional shortlist: “Scrap Swing” (working title; name clearance unknown).** Swing a magnetic load into a wreck to collect scrap. Every pickup locks onto the outside of the load, making its silhouette harder to fit back through the exit. Bank the load now, or risk adding one more valuable piece and losing it against the walls.

The intended player enjoys short arcade mastery, physical-looking toys, and improving a visible result. This is a skill challenge with forgiving restarts, not a promise of stress-free cleanup. Its first ten seconds should show a narrow load slipping through a gap, one extra piece attaching sideways, and the now-awkward return journey. Whether that moment is readable or satisfying is **hypothesis**.

This is meaningfully different from the existing Latchline draft: the player operates a crane and manages the changing geometry of carried cargo; there is no courier platforming or release-to-fly traversal. Both share swing control and bank-versus-risk tension, so keep only one if playtests reveal that their decisions feel identical. It also differs from claw roguelikes through continuous spatial handling rather than randomized item synergies, combat, or deck growth. This desk scan is not exhaustive novelty clearance.

**Trend stage: unverified. Confidence: low.** Several current products establish competition and visible response, but no consistent time series or audience test establishes rising demand, willingness to pay, or demand for this exact hook. The producer is separately checking trend-discovery sources; do not re-label this competitor scan as trend-led research.

Suggested decision-aid scores: demand 2, distinctiveness 3, fun potential 2, scope fit 3, distribution 3. Weighted score 2.60/5. Demand is indirect; distinction is mechanically specific but untested; fun is unplayed; scope depends on strict physics cuts; distribution has a potentially clear visual action without measured response. Confidence and trend stage remain separate from that score.

## Comparable evidence

All sources below accessed **2026-09-06**. Prices and counts are **observed** in the producer's [Steam snapshot](steam-snapshot.json), captured **2026-09-06T20:14:13.835876+00:00**. Price requests use `cc=us&l=english`; USD list/current are dated offers, not realized receipts. Review requests use `language=all`, `purchase_type=steam`, `review_type=all`, `filter=all`, `num_per_page=0`. These are all-language Steam-purchase API counts, not English store-header counts, player counts, or sales. Off-topic filtering was not explicitly set by the snapshot command and remains the API default.

| Comparable and direct source | Release event displayed | USD list / current; reviews | Reported offering and relevance |
|---|---|---|---|
| [Dungeon Clawler](https://store.steampowered.com/app/2356780/Dungeon_Clawler/) — app **2356780** | Early Access 2024-11-21; full release 2026-04-30 | $14.99 / $7.49; **3,680** (3,254 positive, 426 negative) | Developer describes building a claw-machine item pool, combat, upgrades and synergies. Visible response supports examining claw interaction; its roguelike content and combat make it a poor four-day production template. |
| [Claw Machine Sim](https://store.steampowered.com/app/2456120/Claw_Machine_Sim/) — app **2456120** | 2024-03-28 | $6.99 / $4.89; **236** (223 positive, 13 negative) | Developer advertises simulated physics, fun/realistic modes, and over 200 collectible prizes. A single familiar action can support a paid offer, but collection breadth and physics tuning are material expectations. |
| [Let's Aim! Crane Game](https://store.steampowered.com/app/4794370/Lets_Aim_Crane_Game/) — app **4794370** | 2026-06-24 | $7.99 / $7.99; **1** (0 positive, 1 negative) | Developer advertises 36 stages, several claw types, timed challenges, and a collection room. This recent low-response example is counterevidence to “crane interaction plus levels is enough.” One review cannot establish quality, financial failure, or its cause. |
| [Spilled!](https://store.steampowered.com/app/2240080/Spilled/) — app **2240080**, not demo 2646600 | 2025-03-26 | $5.99 / $5.99; **4,876** (4,570 positive, 306 negative) | Developer describes about one hour of calm cleanup, eight areas, boat upgrades and animal rescue. Supports a short tactile cleanup offer, but its forgiving linear promise differs from our repeated skill challenge. |

The listed releases are **observed store metadata**, not independently reconstructed launch histories. Offering descriptions are **reported developer claims**, not verified gameplay. Review/price API URLs for every row are preserved in the snapshot. The canonical web pages opened successfully, but three explicit US/English page URLs returned internal errors; canonical pages displayed mixed currencies. Therefore the table uses the verified central US API snapshot, never inferred currency or cached search prices. Canonical Dungeon Clawler and Spilled! web headers showed English counts, which must not be compared directly with the table totals.

Additional overlap check: [Magnet Fishing Simulator](https://store.steampowered.com/app/1151980/Magnet_Fishing_Simulator/) — app **1151980**, accessed 2026-09-06. **Observed:** Coming Soon; no released-game response or price to compare. **Reported:** recover metal, restore/sell/collect, improve equipment, explore multiple waters. **Interpretation:** a generic magnet collect/sell/upgrade pitch is already occupied and exceeds our scope. An unreleased listing is supply evidence, not proof of demand or a failed competitor.

Spilled!'s developer also **reports** work starting at the end of 2022 on its [store page](https://store.steampowered.com/app/2240080/Spilled/). Its small playtime should not be mistaken for proof that its commercial presentation took a few days.

## Player response sample and reasons to doubt

Read ten Claw Machine Sim reviews through the public API: five most recent positive and five most recent negative **English Steam-purchase** reviews, accessed 2026-09-06. [Positive query](https://store.steampowered.com/appreviews/2456120?json=1&language=english&purchase_type=steam&filter=recent&num_per_page=5&review_type=positive); [negative query](https://store.steampowered.com/appreviews/2456120?json=1&language=english&purchase_type=steam&filter=recent&num_per_page=5&review_type=negative). This is a small sentiment-stratified convenience sample, intentionally oversampling dissatisfaction; it does not estimate complaint prevalence. Direct community listing requests failed in the web tool; the public API succeeded. No login was used and no players were contacted.

- **Reported, 2026-05-18:** one reviewer found it a satisfying inexpensive substitute for visiting a claw machine and valued the manageable achievements. [Review](https://steamcommunity.com/profiles/76561197989071695/recommended/2456120/).
- **Reported, 2026-05-21:** another praised prize variety but reported high Steam Deck power draw. Performance numbers were not independently tested. [Review](https://steamcommunity.com/profiles/76561198090665034/recommended/2456120/).
- **Reported, 2026-05-13:** a negative reviewer praised simulation while objecting to repeating one machine and limited content. Their later assertion about abandonment is unverified and is not adopted here. [Review](https://steamcommunity.com/profiles/76561197985289217/recommended/2456120/).
- **Reported, 2025-09-13:** another tied value for money to more content. [Review](https://steamcommunity.com/profiles/76561198041076410/recommended/2456120/).
- **Reported, 2024-06-30:** another described frustrating physics, mismatched music, and shallow content. These are subjective responses, not reproduced defects. [Review](https://steamcommunity.com/profiles/76561198071762171/recommended/2456120/).

**Observed sample pattern:** three of the five selected negative reviews raise content/value concerns; two of the selected positive reviews explain satisfaction through the familiar claw activity or prize variety. **Interpretation:** clever physics alone may not sustain the perceived value of a paid microgame. Eight short contracts and seeded scoring do not automatically solve this; voluntary replay must appear before content is expanded.

Other material doubts:

- Growing load geometry could be immediately understandable, or could make apparently successful paths fail at invisible collider edges. The proposed solution is a stable visible silhouette and generous collision tolerance, not simulation realism.
- Mass alone does not justify claiming that heavier cargo has a different simple-pendulum period. The hook should depend on **load shape and clearance**. Any changes to motion are deliberately tuned game rules and must be communicated through play.
- Attaching six freely jointed pieces creates collision/constraint combinations beyond a prudent few-day scope. If the hook only works with freeform pile physics, reject this scope estimate.
- A timed skill game may lose the calm audience suggested by Spilled!, while being too thin for the build-synergy audience suggested by Dungeon Clawler. Actual target-player fit is unknown.
- The recent crane counterexample and readily understood familiar interaction weaken any claim that a good short clip will reliably find buyers. No creator access, clicks, wishlists, conversions, or sales have been measured.

## Compact loop and hard production boundary

**Hypothesis loop:** choose one three-minute salvage contract; move a crane trolley and winch; touch a piece to preview its fixed attachment socket; accept it onto the swinging load; navigate past two readable obstructions; either return to the hopper for a growing multi-piece payout or add another piece. A hard collision sheds the most recently added piece with a clear warning; a deposit is safe. Finish, see the best haul and one missed opportunity, restart or choose another contract.

**Estimated small commercial target:** five to six focused agent working days including the first experiment, assuming that experiment passes. Four days is optimistic. One Windows build; one side-view harbor art set; one crane rig; one damped pendulum body with a compound cargo silhouette; at most six snapped cargo pieces; three obstacle layouts; eight authored contracts using twelve readable scrap silhouettes; local best scores; instant retries; practice without a timer; pause, audio/settings and robust local persistence. Working estimate is not a delivery guarantee or release date.

Estimated allocation: 1 day for interaction, collision readability and first runnable test; 1 day for contract/scoring/save UI; 1 day for one coherent visual direction and compact sound palette; 0.75 day for three layouts/eight contract tuning; 1 day for packaging and input/focus/pause/save/resolution/performance QA; 0.5 day for honest capsule direction/screenshots/short gameplay capture. Remaining 0.75 day is tuning/fix contingency. No content expansion beyond this boundary.

**Presentation:** strong crane-yellow shape against a cool harbor cutaway; scrap identified by silhouette as well as color; quiet water and motor bed; distinct attachment click, danger scrape, shed piece and deposit flourish. Budget includes inspection at gameplay size and in motion, not just asset production. Use original simple art/foley or separately cleared assets with provenance recorded during authorized development.

**Owner effort estimate:** roughly 1.5–2 hours in short sessions for two control/playtest rounds, one art/audio review, and one product/store presentation review. Recruitment or scheduling additional target players is a separate unknown; no contacts have been authorized or sent. Agent implementation time, owner attention and calendar time are distinct. Steam onboarding, page lead time/review and store approval are outside the build estimate; the producer handles those constraints separately.

**Cuts:** no 3D, realistic rope, independent cargo joints, arbitrary magnetic pile, fracture, fluid simulation, open world, restoration, shop/economy, upgrade tree, combat, online scores, achievements dependency, multiplayer, level editor, controller/Steam Deck promise, or localization promise. Full release-platform setup is not assumed to fit into these five to six days.

**Commercial hypothesis:** a deliberately small premium Windows offer around **USD 3.99–5.99**, tested against the finished amount of mastery/replay value. No live price, spend, revenue forecast, sales estimate or commercial result is implied. Cash spending is not authorized; optional assets, store charges and ongoing support cost remain separate. Reserve post-launch defect response if a release is later selected; support volume is unknown.

## Cheapest next test and stop rule

If selected for an experiment later: spend at most **0.75–1 agent day** on one fixed harbor, three cargo shapes, one obstruction, three-minute score loop, reset and basic audio. Use snapped sockets and one pendulum from the start. No shop, content unlocks or commercial art are needed to test this interaction.

Have Klaus play three runs in a 15–20 minute session; when available, repeat with four target players in short sessions. Record each player's initial explanation, first pickup/deposit, collisions, early banks, and voluntary restarts. One owner's response is a useful feel check, not market validation.

Provisional continuation thresholds for five players: four complete a pickup and deposit within 90 seconds without verbal coaching; four correctly predict whether a shown load fits a gap; three intentionally bank early because the extra piece changes clearance; three voluntarily start another run. Also require the packaged test to complete repeated pickup/collision/deposit cycles without a stuck load or unrecoverable state. These are directional decision thresholds, not statistical proof of fun or demand.

**Stop/hold:** if two tuning passes cannot make silhouette-based collisions understandable, or the bank decision never changes behavior, park this version. If it is only enjoyable with many freely moving pieces, a large upgrade tree, or dozens of handcrafted scenarios, the few-day candidate fails. Do not add progression to hide a weak swing-and-carry action.

Next executable action: producer compares this conditional candidate with the two other lanes, records its draft/shortlisted status and uncertainty, and recommends the smallest experiment. This note authorizes no prototype, game selection, spend, contact or publication.
