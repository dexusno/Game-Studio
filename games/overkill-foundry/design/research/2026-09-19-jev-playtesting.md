# Jev as a graphics-free playtester

Researched 19 September 2026. Klaus confirmed that the intended model is **Jev from TypeSafe AI**, and that hosted use is acceptable in principle given its low price. This is an integration assessment, not a model benchmark or a commitment to a paid provider. No account, API request, installation or game implementation was used.

**Recommendation:** evaluate Jev as one interchangeable automated player once our shared rules core and runner exist. The game supplies exact legal choices and measures outcomes; Jev chooses actions; Codex analyses the evidence. Keep local scripted/search players for comparison and inexpensive bulk coverage. Human playtests and Klaus's graphics approval remain required.

## What the model actually does

Jev accepts text or structured state and returns a predefined choice, rubric score or yes/no probability. It cannot write a free-form explanation. TypeSafe calls this a System One model. Its public primer describes **reinforcement learning for calibrated decisions (RLCD)**: training toward useful decisions whose probabilities reflect accuracy across many examples. This is the vendor's description, not an independently reproduced training result. The reviewed materials do not disclose enough architecture/training detail to reproduce the model or establish that it is wholly unrelated to language-model technology. [System One](https://docs.typesafe.ai/concepts/system-one), [training primer](https://docs.typesafe.ai/introduction/machine-learning-primer).

TypeSafe describes parallel outputs rather than token-by-token prose generation and advertises 70–500 ms end-to-end latency, generally measured near its US West Coast service. Its Doom demonstration uses structured textual game state. The developer acknowledges that a conventional Doom bot could play better. This establishes a relevant interface demonstration, not proficiency at our campaign strategy. The advertised guarantee concerns matching the declared output schema; a permitted choice can still be wrong. [Launch explanation and demo](https://typesafe.ai/blog/introducing-system-one-models-and-jev).

The current direct model is `jev-1.13.0`. Published pricing is **$0.042 per million input tokens; outputs free**. Limits are currently 1,200 requests/minute and 250,000 tokens/second, subject to change. Requests allow 64k combined tokens, with 32k for state plus the longest question. Customer fine-tuning/LoRA is unavailable: playing our games does not itself retrain Jev. [Model reference](https://docs.typesafe.ai/models).

Hosted distribution also appears in the official [Vercel catalogue](https://vercel.com/ai-gateway/models/jev) and [Cloudflare documentation](https://developers.cloudflare.com/ai/models/typesafe/jev/). Provider prices, versions and quotas must be checked when choosing access. The public SDK is a client, not evidence of downloadable model weights. No local inference route was verified.

## How it could play our game

The runner would send observable HP, materials, installed/reserved parts, relevant recipes/upgrades, cooldowns, enemy intents and concise rules. It would supply legal action IDs and engine-calculated immediate consequences. Jev selects an action; the core validates and applies it; the next request sees the resulting state. Keep hidden draws and future RNG out of its input. Preserve all character and upgrade exceptions and the exclusive End Turn command.

The vendor explicitly reports weaknesses in arithmetic, counting, indirect reasoning and irrelevant long context. Our code must calculate damage, costs, affordability and timing. A typed answer is not a substitute for checking legality against the current state. The uncertain part is strategic foresight across preparation, several shots, shops and future fights. [Documented limitations](https://docs.typesafe.ai/model-jaggedness/jev-1.13).

A Choice supports at most 255 alternatives. Our 606 recipes and resource-limited combinations can exceed that, so use staged action/recipe/target decisions or a measured shortlist. Record the candidate generator: pruning can hide strong combinations and bias the apparent balance. Independent questions can share one request, but later game states depend on earlier actions. Parallelise separate games; do not assume a whole dependent campaign fits into one parallel call. [Choice interface](https://docs.typesafe.ai/primitives/choice).

## Where feedback comes from

**Observable state → Jev choice → shared game core → event log and metrics → Codex balance report.**

| Evidence producer | Useful feedback | Limit |
| --- | --- | --- |
| Game core and report code | Wins/losses by mercenary/city/tier, HP attrition, resource shortages/leftovers, recipe use, damage/Shield value, failure traces | Exact recorded outcomes still depend on the policy and collection-skill assumptions. |
| Jev | Choice probabilities, uncertainty, optional predefined assessments of a trace | No free-form explanation; retrospective labels are additional judgments, not access to its original reasoning. |
| Paired experiments and Codex analysis | Identify repeated failure patterns, compare alternative strategies or balance values, propose a focused change with supporting replays | A losing bot alone cannot establish that the game is too difficult. |
| Human testers | Comprehension, controls, pacing, enjoyment and rendered visual verification | Required alongside automated runs; simulated metrics cannot approve presentation. |

For example, a report might find deaths with unused defensive resources. We would inspect the legal alternatives and compare a defensive policy before increasing resource grants. If several competent policies instead fail on the same shortage across held-out seeds, that is stronger evidence for an economy adjustment. This is an illustrative diagnostic, not an observed result.

Jev's `confidence` summarises concentration of its answer distribution. A confidence of 0.9 when choosing Shield is **not** a measured 90% chance of winning. Calibration for our own decision questions needs separate evidence. Uncertain states are useful candidates for replay review. [Confidence reference](https://docs.typesafe.ai/confidence).

## Illustrative cost and throughput

Assume **300 API decisions per game**, one request per decision, and **2,000 total input tokens per request**, including state and option descriptions. These are hypothetical workload inputs, not estimates measured from our game.

| Batch | Requests | Input tokens | Direct advertised token cost |
| --- | ---: | ---: | ---: |
| 100 games | 30,000 | 60 million | $2.52 |
| 500 games | 150,000 | 300 million | $12.60 |

At 10,000 input tokens per decision, the 500-game scenario becomes **$63**. More actions, staged choices and extra analysis increase usage. These figures exclude runner compute, taxes/provider differences, retries and separate written-report analysis.

For 100 games, multiplying 30,000 sequential calls by the advertised latency gives **35–250 minutes** of API waiting alone. With enough independent games in parallel, the listed request quota still imposes a **25-minute lower bound** for that workload, or **125 minutes for 500 games**. Neither is a completion-time promise. Our location, actual context sizes, quotas and service load need measurement. Low cost is plausible; hundreds of entire campaigns in seconds is not supported by these assumptions. A local scripted player avoids network waiting and remains worth benchmarking.

## Proposed evaluation when a runner exists

1. Establish deterministic encounter fixtures, legal actions and replay before involving Jev. Test obvious tactical choices and the game's exceptional rules against exact results.
2. Compare Jev, aggressive/defensive/economy scripts and bounded search on matching seeds with the same player-visible information and collection model. Use held-out fixtures after prompt adjustment.
3. Start with a small pilot, then a labelled 100-campaign comparison. Report sample sizes, uncertainty, strategic failures, API errors/timeouts, actual tokens/cost, latency and campaigns/minute. Store model version, prompt/candidate-generator versions, responses and chosen actions. Replay recorded actions; identical seeds alone do not guarantee identical remote decisions.
4. Keep Jev if it contributes useful strategy diversity or finds failures the other players miss at acceptable measured cost/speed. Do not weaken enemies to compensate for a confused policy, silently switch policy on API errors, or add gameplay caps to handle runner loops.

Confidence: high in the documented interface/pricing; moderate in integration feasibility; unknown in campaign competence and measured throughput. The owner-supplied [YouTube reference](https://www.youtube.com/watch?v=2z-7pIj57f8) could not be retrieved because YouTube fetches were throttled; no claim here depends on having watched it. Technical findings use the primary documentation above. The next development dependency remains the shared combat core and graphics-free runner, not a Jev subscription.
