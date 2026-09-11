# Magnet Sweep: progression and rewards

**Current recommendation: see the dated gameplay revision below.** The initial timed-shift proposal is preserved as design history and is superseded. The owner's correction removes development timelines from design judgments; they are not a reason to retain or reject a gameplay rule.

Designer 2 proposal, 2026-09-11. Revised through direct discussion with designers 1 and 3. This is a design hypothesis, not a tested claim of fun. Only this document is owned here; the producer owns integration and budget decisions. There is no engine project or playable build.

## The reward promise

Turn an awkward little magnet into a visibly more capable salvage tool, finish a short shift, then try a better route or tool configuration. The interesting improvement is what the player can collect and carry in one trip. Bigger score text alone is insufficient.

Recommend one shift containing three batches of **60, 90 and 90 seconds**, plus two brief intermissions and a result. Roughly five minutes is a tuning hypothesis, not a duration requirement. Each batch uses the same attractive fixed-view tray. Deposits produce immediate scrap movement, sound and a visible increase in banked value. The two intermissions give meaningful tool choices; the ending shows the accumulated result. No story campaign, production network or collectible catalogue is needed.

## Concrete action and economy

Hold the mouse button to attract nearby scrap and retain the carried load. Releasing over the furnace banks it; releasing elsewhere drops it. Movement has a capped speed, and capture selects nearest pieces first with a stable tie-break. These interaction rules require engineer verification. Cursor teleportation or unpredictable capture would invalidate the proposed routing choices.

Start with capacity 12. A small bit occupies one space and banks one point; a bulky core occupies four spaces and banks eight. At 9/12 capacity near a core, depositing the partial load frees room for a valuable return trip. Alternatively, the player can grab nearby bits and postpone the core route. Wide capture makes nearby dense patches attractive; travel cost and mixed pockets must prevent cores from being the automatic answer everywhere.

**Banked salvage value is the only numerical reward account.** It is the score, not spendable currency. After batches one and two, the workshop supplies one module choice regardless of performance. Completing each batch earns that opportunity. This guarantees that struggling players experience the defining improvement loop and avoids separate cash, upgrade costs, farming or a progression wall. The fiction is deposited scrap refitting the tool; do not present an unaffordable shop.

Only deposited pieces score. The final seconds must clearly remind players to bank. At timeout, unbanked pieces are lost for that batch and the intermission proceeds. Missing the shift target changes its result stamp, not access to upgrades or the ending. Idle play may reach the end but cannot achieve a successful salvage target.

## Exact initial upgrade alternatives

Choose one option at each intermission. Each can be chosen twice, with no additional prerequisites.

| Module | Each rank | Visible and practical effect |
| --- | --- | --- |
| Wide Field | Add 35% of the original capture radius | Broader field outline and a wider tool collar; gathers spread-out bits with fewer passes, but can fill on cheap surrounding material. |
| Deep Basket | Add four carrying spaces | A deeper visible basket and four additional capacity marks; permits larger mixed loads or another core before depositing. |

Two Wide ranks give 1.70 times the original radius, not multiplicative scaling. Two Basket ranks give capacity 20. The resulting three end configurations are wide, mixed and deep; these are three hypotheses about play style, not evidence of build diversity. Dense pockets should still contain more than 20 spaces of material, so the largest basket does not erase routing. Keep capture speed, material value and movement speed unchanged initially to make each choice interpretable.

Do not add more upgrades if these feel equivalent. First change their magnitude or the placement of scrap, then remove a redundant choice if necessary. A new mechanic would need producer review and an engineer estimate.

## Reward arcs and pacing

**First 20 seconds — readability and appeal.** The first pieces visibly attach, capacity responds immediately, and the first deposit transforms a modest load into an emphatic furnace payoff. The player understands collection and banking through action. Avoid long onboarding or a deliberately miserable starter magnet.

**First minute — agency.** The opening batch ends and offers the two module illustrations, showing the actual changed radius or capacity. The next batch begins promptly. The player should be able to predict one route they will attempt because of the choice.

**Next two batches — challenge and competence.** Mixed pockets create chances to recognize a full basket early, deposit efficiently, or collect a core without filling on surrounding bits. The second choice reinforces or broadens that approach. Increasing density is acceptable; slower collection and rising prices are not pacing tools here.

**Shift ending — closure and replay.** Show banked value, the target result, the chosen modules and the comparison with the player's comparable best. A small filled salvage display or completed workshop job supplies visual closure without requiring an assembled-machine asset system. Offer replay of the same arrangement and another validated arrangement. If every piece is banked early, finish that batch immediately; use total active time as a tie-break rather than inventing bonus points that outweigh material value.

## Reset and persistence boundaries

A new shift resets capacity, radius, active scrap, batch time and shift score. Retain completed results, bests, completion flags and preferences locally. Associate comparable records with the preset, layout seed and balance version. No permanent strength, prestige reset, offline production or daily reward.

Pause and focus loss stop active time. The minimum planned save boundary is a completed shift; quitting an unfinished shift forfeits that attempt, with a clear quit message. A saved mid-shift checkpoint is a separate convenience proposal for the engineer to cost, not assumed infrastructure. It must never duplicate banked pieces or score if later included.

Designer 3's three layout presets are a **conditional commercial content ceiling**, using the same board and rules. They should be directly selectable after onboarding and favor distinct routes, not serve as unlock chores. More arrangements cannot establish sufficient paid value if the first shift is dull. The producer must assess whether this still fits the five-to-eight-day estimate; one shift remains the prototype boundary.

## Failure hypotheses and the smallest test

Watch for universally superior cores, Basket always beating Wide, accidental filling feeling unfair, upgrades being imperceptible, or waiting for the timer after meaningful decisions end. These are economic and pacing failures; adding loot rarity, purity multipliers or extra currencies would obscure them.

If almost everyone clears every board early, scores saturate and the assumed shift length disappears. Measure clear times before tuning density; do not slow capture or add waiting to manufacture duration. A much shorter time-trial would change the product promise and needs the producer's explicit reconciliation.

The one-day probe is two 60-second batches, one module choice, a result and same-arrangement retry. Observe a player deposit within 20 seconds, make one deliberate route or capacity decision, identify the module's effect, and reach a readable ending. Then offer a retry without asking them to improve a prescribed metric.

Scope floor: retain readable manual banking, a consequential capacity decision, an observable tool improvement and an ending/retry. Cutting any of these makes the test unrepresentative. Cut additional presets and presentation decoration first.

**Playtest question:** “Did changing the magnet make you want another attempt, and what would you do differently?” Record the actual answer and behavior. A promising observation is a voluntary retry with a concrete changed route or module choice. A polite positive remark, an improved score, or an agent's preference does not establish enjoyment. If the player sees no interesting choice after one tuning revision, revisit the core before expanding content.

## Collaboration record

- **Interaction designer:** challenged their full-load bonus because it made depositing full a universal rule; jointly cut it. Accepted mixed bits/cores, nearest-first capture and bounded carrying. They challenged my longer shift and strong/precise dwell branch; I shortened the shift and removed differing dwell. We agreed on two module choices and score-only rewards.
- **Replay designer:** challenged obligatory weak restarts and permanent stat growth; we accepted identical replay timing and no power grind. They challenged thin commercial content; accepted three presets only as a conditional producer proposal. Accepted their early-clear time tie-break instead of my proposed time bonus.
- **Shared conclusion:** 60/90/90-second shifts, two material types, two repeatable module options, guaranteed intermission progress and optional mastery replay. Unresolved evidence questions are upgrade differentiation, satisfaction of selective collection, and enough appeal for a paid game. No unresolved rule disagreement remains; all numerical tuning and content estimates remain provisional.

## Gameplay revision — 2026-09-11 — current recommendation

This revision follows the owner's correction and a new direct critique round with the interaction designer and independent gameplay critic. The recommendation changes substantially: **a finite, untimed magnet-improvement journey, followed by fresh salvage deliveries using the earned rig.** Replace the short resetting score attack and its repeated numeric choices. Keep the game coherent through one collection interaction, one visible forging goal and a few improvements that change what the magnet can do. Satisfaction and replay remain hypotheses until observed play.

### One goal that makes collecting useful

Each delivery supplies loose scrap and pullable tangled clusters. Every collected piece is desirable. Banked material fills a clearly visible furnace to a marked forging line. Reaching that line produces the next recognizable improvement to the magnet itself. Use material amount consistently; discard the former rule that bulky cores pay twice as much per space, which could make ordinary scrap feel like a mistake.

Set the forging requirement comfortably below all available salvage. The player chooses appealing patches and pulls instead of hunting the last fleck. Do not increase requirements merely to cancel a more powerful tool. Faster clears and larger hauls are part of the earned reward. There is no shop, spendable wallet, cleanup percentage, special goal-piece puzzle or requirement to repeat an earlier delivery to afford progress.

When a deposit crosses the forging line, let the entire active pull, cascade and deposit presentation resolve. Count its full haul, celebrate the completed improvement, then let the player move on. Do not interrupt a wonderful chain reaction with a result screen. Surplus material contributes to that delivery's haul record and visual pile; it does not become another progression currency. Remaining scrap can be enjoyed or left behind.

### What the player actually does

Ordinary collection remains immediate: hold to attract loose scrap; release stops attraction while retaining cargo. Deposit deliberately at the generous furnace opening. Starting a drag on a clearly highlighted pull-ring latches a tangle. Drag to aim its visible pull corridor and release to yank it toward the magnet, sweeping loose scrap along its path. Crossing a ring during an ordinary sweep must not accidentally change actions.

A short drag still frees the tangle. Avoid a motor-speed test, hidden charge duration or repeated weak-tug failure. The interesting difference is what direction and reach collect: a direct pull gets the nearby cluster; a considered diagonal can pass through a seam of scrap. The later Breakaway improvement releases the selected tangle and its directly connected neighbors inside the previewed corridor. Highlight the entire affected group before release; newly released neighbors do not propagate again in that action. This is proposed deterministic motion, not an assumption that realistic magnetism or a physics pile will behave well. The engineer must verify readable aiming, movement and bounded reactions.

### Clear reward sequence, without a choice catalogue

Recommend three illustrative jobs for the initial complete journey, using these automatic milestones rather than an upgrade shop:

1. **Learn and forge Breakaway.** Start with sweeping and the isolated tug. The first furnace improvement adds a visible coil and the ability to release connected tangles in a cascade. Reach this reward early; the defining interaction must not be hidden behind prolonged ordinary cleaning.
2. **Explore cascades and forge longer reach.** Present an obvious short chain, then a seam where aiming changes the haul. Completing this job visibly extends the rig's arm, allowing longer corridors and more possible approach angles.
3. **Enjoy the completed rig.** The final job contains both broad loose patches and linked tangles that reward the player's new reach and experience. Complete a satisfying forge and restore the rig's finished appearance. This job demonstrates earned power; it does not secretly resist that power to preserve its previous duration.

Ordinary field growth can accompany the visible rebuild, but is not presented as a difficult decision between near-equivalent statistics. There are **no mandatory upgrade-menu choices** in this recommendation. Player agency comes from selecting patches, starting tangles and pull directions. The exact job count is a content hypothesis; these three jobs describe a complete learning-and-payoff arc, not proof of sufficient commercial value.

### Concrete rewarding moments

- **Immediate pleasure:** a small trail snaps into the field, follows the magnet with readable weight, then pours into the furnace. The forging line rises by an amount the player can recognize.
- **Anticipation:** the furnace shows the coil it is building, and a short preview makes clear that the next improvement will release linked scrap. The player has a reason to complete the current haul.
- **Competence:** the player notices that a diagonal tug crosses two useful patches, tries it, and sees the larger result they intended. A poorer angle still collects something and moves forging forward.
- **Earned dominance:** the improved magnet returns to an earlier arrangement and makes an awkward cluster collapse effortlessly. Preserve that payoff without claiming it alone sustains replay.

**Remove the hard cargo cap.** The revised interaction has no need to ration carrying space, slow a loaded magnet or sell capacity improvements. Let visible cargo grow without hiding the field or pull preview, and show how much its next deposit will add toward the forging line. Banking remains deliberate: deposit for the immediate payoff and ready improvement, or enjoy a longer sweep. That comfortable cadence is not claimed as deep strategy; meaningful choice lies in the pulls themselves.

### Persistence and a concrete replay hypothesis

Retain unlocked tool behaviors, completed jobs and the rig's appearance. A new delivery resets its scrap arrangement and forging/haul state, not the player's earned magnet. Save completed-job progress, preferences and local haul records. Exact interruption recovery is an implementation dependency, not an excuse to reset earned progress.

After the journey, fresh deliveries use the same collection loop and full rig. Remix loose seams, tangle positions and connections so the attractive first pull, direction or cascade setup changes. Merely jittering decoration does not qualify. Let the player see their largest single-yank haul and delivery haul; these support self-chosen experiments without imposing a clock, score penalty or new mode.

The intended second-play motivation is: “That arrangement might let me start over there and pull a much bigger chain.” It is not “I must repeat a weak opening to regain my magnet.” Replaying an old job with greater power may be pleasing once; lasting replay needs fresh useful spatial decisions and a toy the player wants to keep touching. If those do not occur, do not claim that remixes make the game replayable.

### Open gameplay tests

Observe a normal sweep and deposit, an isolated aimed tug, an earned Breakaway improvement and a small cascade in the representative play session. Ask: **“What made you want the next delivery, and what would you try differently?”** Watch whether the player actually initiates another delivery after using the full rig.

Specific failure evidence includes finishing by dull sweeps while avoiding the supposed hook; choosing the same obvious pull on every arrangement; cargo hiding useful information; understanding improvements only as a higher number; or treating the fill line as work to endure. Test whether two arrangements produce different deliberate starting pulls, whether players can explain a satisfying chain they caused, and whether they continue after progression rewards stop. Praise, score increases and designer agreement are insufficient evidence.

### Revised collaboration record

- Accepted the critic's challenge to timed resets and numeric upgrade branches; withdrew both. Accepted retained power, a visible magnet rebuild and early behavioral rewards. Rejected mandatory money farming and an automatic extra timed mode.
- Accepted the interaction designer's retained cargo, deliberate ring drag and visible yank corridor. Withdrew my proposed velocity-based tug and weak-tug failure. Accepted short chains before claiming a broad cascade game.
- Challenged the critic's named goal-piece proposal and the interaction designer's linked-support extraction variant: both could shift collecting into access puzzles or allow goal-only rushing. Proposed the visible furnace fill line instead; both peers accepted it. No combined goal system remains.
- Accepted the critic's fresh-delivery grammar, with the explicit requirement that geometry changes useful pulls. Agreed with interaction that collecting remains useful, leftover flecks remain optional and targets follow satisfying actions rather than a desired duration.
- Accepted the producer's removal of hard cargo limits, agreeing with interaction that the old scarcity rationale no longer applies. Preserved visible cargo and deliberate banking, without slowdown or capacity grinding.
- No remaining disagreement on the goal or reward structure. Remaining gameplay uncertainties are whether the tug feels like magnet play, whether cascades stay readable and player-directed, and whether full-rig deliveries earn voluntary repetition.
