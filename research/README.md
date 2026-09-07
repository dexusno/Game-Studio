# Research small games people may want to play

For next-game discovery, start with rising trends as inspiration, derive an original playable promise, then test it against game-specific evidence and the cheapest playable experiment. The owner clarified this order on 2026-09-06. Research should shorten the route to a good playable game. A popular category, polished competitor, or high score is not proof that our version will sell.

All subjects are eligible inspiration: technology, culture, entertainment, social behaviour, sport, food, fashion, nature, work, hobbies, and anything else a broad scan reveals. These examples are not an allowlist. Do not narrow discovery to crafts, cozy subjects, puzzles, or an existing game idea. Trends can inspire a mechanic, fantasy, setting, visual language, reward, or just one element; the game need not simulate or be about the trend. The game must be enjoyable and worth replaying or completing after that interest fades. A name that people following the topic might search for is a bonus, not a selection requirement. Organic discovery through searches, recommendations or creators is a potential benefit, not guaranteed advertising or evidence of purchase intent.

Keep current candidates in `opportunities/*.json`, source recipes in `sources.json`, and a short dated record in `runs/<date>-<name>/README.md`. One run note plus updated candidate records is usually enough. No game is selected by the studio setup.

## A practical run

1. Read the studio charter and existing opportunities, then inspect actual trend sources in `sources.json` before choosing game concepts. Record the source topic, publication/access dates, underlying measurement period, geography, direction and uncertainty. Prefer rising interest and assess whether the window can outlast production and store lead time. A recently updated article or a large lifetime growth number alone does not prove current acceleration. Record failed access and weaker/rejected signals.
2. Derive a varied slate of plausible hooks from the strongest relevant signals; three to six is a default, with more examples and genres when requested. Explain the source-to-idea connection and why the interaction still works without the trend. Direct simulation is only one possibility. Compare with archived ideas to avoid duplicates; do not retrofit a trend onto a predetermined shortlist. Preserve promising evergreen alternatives as such rather than presenting them as trend discoveries. An explicit owner concept or focused comparison can start directly with that concept.
3. For each hook, inspect a few close competitors and a less successful or recent comparable when available. Pair the proposed genre with games buyers purchase in our price range, using Steam and other accessible stores, developer disclosures and postmortems. Record what is actually sold, regular versus discounted price/region/date, scope, visible audience response, recurring praise/friction, and what our first ten seconds would communicate. Separate reported paid units from reviews and estimates. A strong incumbent is evidence of competition as well as category interest. Do not sample winners alone or mistake their budgets for our production timeframe.
4. Write a draft opportunity and score it. Include the largest uncertainty, a small prototype envelope, estimated owner effort, commercial assumption, and one next test with a decision threshold. Distinguish a missing measurement from a negative result.
5. Have another agent challenge the strongest trend claim, the natural connection to play, and the riskiest scope assumption. Verify pivotal citations, including whether several reports repeat the same underlying data. Correct the record or lower confidence; do not create review loops for cosmetic disagreements.
6. Present the short comparison and a recommendation for the next experiment. Follow existing user authorization; ask only for an unresolved choice that changes the work. Selection and public publishing are different decisions. The research pilot does neither.
7. After an authorized prototype, record observed controls, clarity, voluntary replay, frustration, and a useful change. After a release, connect actual costs, sales, refunds, and feedback to the originating opportunity. Re-score only when evidence changes.

Work may continue across these steps in one session. Record the next action before stopping. Do not require a separate approval for every idea or routine internal step.

## Timing and evidence

`trend_stage` accepts `rising`, `near-peak`, `steady`, `declining`, or `unverified`. Timing is separate from quality and confidence.

- **Rising:** repeated comparable observations show an upward trajectory, preferably supported by independent audience signals. Two pages repeating one announcement are one signal. Record dates, geography, filters, and the period compared.
- **Near-peak:** strong current attention with evidence that acceleration is flattening or crowding is high. It can still be attractive if a distinctive prototype and realistic release date fit the remaining window.
- **Steady:** repeated observations support persistent demand without a strong current slope. An older successful game alone does not establish this.
- **Declining:** comparable observations show decreasing attention. Assess any durable niche separately.
- **Unverified:** insufficient time-series evidence. This is the correct default for a one-day scan, including the initial pilot.

Use `observed` for something directly visible in the inspected source; `reported` for the source author's statement; `estimated` for an explicit calculation with assumptions; and `hypothesis` for an untested studio inference. Every evidence record has a source URL, access date, precise claim, and kind. A hypothesis may cite the observation that prompted it, but must say that the source does not validate the proposed game. Do not invent a URL for an internal playtest; keep local test artifacts in the run/game and link them through an additional field.

Never treat review counts as measurements of sales, revenue, profit, or conversion. If a sales estimate is explicitly requested, use a sourced, relevant calibration model with stated assumptions and a broad uncertainty range; explain why the calibration fits, and decline to quantify when it does not. Such an estimate remains separate from observed data and is not produced by the snapshot tool. Store price is not net revenue per buyer. Steam's own documentation distinguishes review eligibility from the reviews that contribute to the score; preserve language, purchase, and date filters when comparing snapshots. [Steamworks review documentation](https://partner.steamgames.com/doc/store/reviews), accessed 2026-09-06.

Use `sources.json` as a starting catalog, not a claim that every source is connected. Public pages may be cached, gated, or unavailable. Log material access failures and use a clearly named alternative; an inaccessible source is not a zero. Avoid login/paywall bypasses. Use focused queries, cache a dated snapshot, space requests, and stop on rate limits rather than repeatedly retrying. No background monitoring is configured by this workflow.

## Scoring without false precision

Use integers from 0 to 5. Weighted score is `demand*0.20 + distinctiveness*0.20 + fun_potential*0.20 + scope_fit*0.25 + distribution*0.15`, out of 5. `python scripts/studio.py score research/opportunities/<id>.json` calculates it. Compare score explanations and confidence before rank.

| Dimension | Weight | What the score must explain |
|---|---:|---|
| Demand | 20% | Evidence for this audience and product promise; recognizable competitors are indirect evidence only. |
| Distinctiveness | 20% | A specific difference players can see and value, checked against close alternatives. |
| Fun potential | 20% | The expected tension, feedback, and replay reason; actual playtest evidence is required before calling fun validated. |
| Scope fit | 25% | Content, engineering, art/audio, QA, release, and support work against the proposed budget. |
| Distribution | 15% | How the intended audience can discover and understand this particular game. |

Common anchors: 0 = absent/unassessed; 1 = weak or serious unresolved contradiction; 2 = plausible but indirect/untested; 3 = supported with important gaps; 4 = strong relevant evidence; 5 = exceptionally strong evidence for this decision. Scope may score well as a bounded estimate, but its explanation must disclose that the build has not proved it. Do not upgrade fun merely because an agent likes the pitch. `confidence` describes the overall evidence: low for initial desk research, medium for multiple relevant checks with gaps, high only with strong direct validation. No score automatically selects a game.

## Owner time and commercial fit

Optimize for a worthwhile, shippable result per hour of owner attention. Record **agent build time, owner time, cash, calendar lead time, and ongoing support separately**. Include store setup, playtesting, reviewing art/audio, trailer/capsule work, compatibility fixes, refunds/support, and release waiting periods. Low owner time does not mean zero quality control or zero agent effort.

Use premium pricing, a paid download, a demo, or another model that suits the game and platform; the Appdrip $3 mobile unlock is not a studio rule. Price ranges in draft opportunities are experiment assumptions, not recommended live prices. Build a simple scenario only when useful: `contribution per copy = realized receipt after discounts, refunds, taxes, platform share, and variable costs`; `cash break-even copies = fixed cash cost / contribution per copy`. Keep owner-hour cost visible alongside cash. Unknown sales volume remains unknown; this arithmetic is not a demand forecast.

The owner accepts a premium price **up to USD 9.99** (2026-09-06), conditional on evidence that comparable games sell in the contemplated genre and price band, and that we can deliver a worthwhile version within our few-day work envelope. This is a ceiling, not a target or a reason to expand scope. Evaluate market/price evidence and production feasibility separately; mark either gap explicitly. A USD 14.99 game temporarily discounted below USD 10 is not a USD 9.99 regular-price benchmark. A developer's reported units plus list price also does not establish that every buyer paid that price.

A near-peak concept needs a fallback: if release readiness misses the attention window, does the core game still make sense as an evergreen product? Avoid a large story, many handcrafted levels, online services, or a content treadmill unless the expected value justifies them. A tiny game still needs intentional presentation, dependable controls, readable feedback, and a complete experience.

Separate build duration from launch timing. At this check, Steam requires a new product's Coming Soon page to be visible for at least two weeks; review and onboarding can add time. Refresh platform requirements before committing a launch date. [Steamworks Coming Soon documentation](https://partner.steamgames.com/doc/store/coming_soon), accessed 2026-09-06.

## Keep decisions resumable

Candidate status is `draft`, `shortlisted`, `selected`, `held`, `rejected`, or `shipped`. Preserve the file when parking/rejecting; set `decision_reason` and a dated decision record. `selected` means an actual decision, never the output of a ranking formula. Refresh stale competitor/timing evidence when reviving an old idea. Keep completed run notes as dated history; candidate JSON is the current state.

The [2026-09-06 studio pilot](runs/2026-09-06-studio-pilot/README.md) demonstrates the format with three drafts. It has no interviews, playtests, acceleration measurement, sales forecast, or selected next game.
