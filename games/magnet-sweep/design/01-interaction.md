# Magnet Sweep: interaction proposal

Designer 1, 2026-09-11. Proposal for producer integration. Static review only: no implementation, build or human fun evidence exists. Read with BRIEF.md and the other designer proposals.

## Promise and central rule

**Guide a hungry magnet through scrap, carry the best loads to the furnace, and build a satisfying sweeper.** Cheap bits can occupy room needed for a valuable core. The magnet remains active while carrying: switching it off releases everything. Returning directly or detouring toward another core are different decisions.

Hypothesis: responsive collection, visible cargo, deliberate routes and a decisive dump encourage repeated shifts. Upgrades alone do not establish depth.

## Proposed implementation contract

- Fixed overhead-oblique camera, one flat tray, one generous furnace pad. No camera controls, obstacles, piles or collision-based carrying. At most 64 pieces occupy nonoverlapping source cells.
- Mouse position sets the magnet's planar target. The magnet travels at a capped constant speed, initially about one tray-width per second, with no inertia. It never teleports to the pointer. The field and a small target marker explain the difference.
- Hold left mouse to activate attraction. Eligible pieces inside the visible field are collected nearest-first; equal distances use stable piece IDs. Reserve their capacity immediately, then animate them into authored attachment positions in about 0.15 seconds. No simulated forces, charge state or pickup timing minigame.
- Two classes: small steel bits occupy one space and score one; visibly larger glowing cores occupy four spaces and score eight. Start with 12 spaces. A piece that cannot fit stays put; other eligible pieces may fit. A blocked core briefly pulses with “Needs 4 free.” Values and sizes are initial tuning hypotheses.
- Release while the **magnet**, not merely the cursor, overlaps the highlighted furnace pad to bank the load. Cargo drops through a short authored animation; score counts up and the furnace flares. No accuracy bonus or mandatory waiting animation.
- Release elsewhere to return unbanked pieces to their recorded source cells, visibly reversing their collection movement. Nothing scores or disappears. This is a correction rule, not a way to rearrange the board. The deliberate stylization avoids introducing a scattering simulation.
- Escape and loss of focus pause without dropping or banking. Resume requires explicit input; stale held-button state must not move cargo. Accessibility can map activation/release to successive clicks using the same two states.

Only score, remaining time and occupied capacity require persistent UI. Color supplements distinct silhouettes and capacity labels. Keep the tray readable at ordinary laptop resolution; cargo must not cover the active field.

## Opening minute and a real decision

**0–10 seconds:** A nearby arc of bits, an isolated core and a distant mixed pocket invite collection. “Hold to collect; release over the furnace” appears beside the highlighted pad. A sweep produces quick clinks and an obviously heavier-looking magnet.

**10–20 seconds:** The first deposit gives immediate credit, a furnace bloom and a burst of sound. Core size and value appear directly in play.

**20–50 seconds:** Following a direct path through bits fills the basket. A nearby core cannot fit. The player learns to plan the next journey: collect the core first, approach from its clearer side, or fill with easy bits and make a quick return.

**At 60 seconds:** The opening batch ends and awards a choice of a wider field or larger basket. Preview the changed radius/capacity directly on the magnet. The next tray composition invites using that choice immediately.

Mid-shift example: the basket holds nine spaces; a four-space core is nearby, with three easy bits toward the furnace. Banking the partial load now frees room for a dedicated core trip. Taking the three bits secures nearby value but spends movement time before the core trip. With little time left, the safe deposit may win. There is no universal full-load multiplier declaring one answer correct.

## Ending, progression and repetition

Provisional baseline: three batches at 60/90/90 seconds, around five minutes with intermissions. Time runs only during collection. Banked scrap is score, not a wallet. At each intermission choose **Wide Field** (+35% of original radius per rank, additive) or **Deep Basket** (+4 spaces per rank), at most two ranks each. These configurations need testing for different routes.

Every player reaches the summary and workshop payoff. Missing the benchmark gives a lower stamp, never blocks the ending or forces farming. At expiry, stop captures and visibly return unbanked cargo without scoring. Warn at ten seconds and three seconds. Banking the entire board ends the batch immediately. Rank by score, using active completion time only for ties. Power resets next shift; save completion and local bests.

Replay the same seed to improve a route or choose the other module; select a new bounded layout to adapt. The repeat promise is “I can collect a better shift with that tool,” not a longer upgrade ladder. Dense core pockets must still exceed an upgraded basket's capacity, without making every layout force the same core-first route.

**Saturation risk:** 64 pieces and fast travel might clear in 20–30 seconds. Measure this before fixing durations. Accept shorter satisfying batches; do not stretch them with slow movement, compulsory waits or extra waves.

## Feedback and expected player effects

- **Readability:** visible field, predictable pickup order, occupied-space count and blocked-core cue should explain an unsuccessful pickup immediately.
- **Appeal:** rising but capped pickup pitch, tiny attachment motion and one stronger furnace sound should make a sweep feel substantial without producing a wall of noise. No camera shake is needed.
- **Challenge:** limited space and timed return trips should reward planning, not precise clicking or reflex punishment.
- **Pacing:** first deposit within 20 seconds, first module at one minute, immediate restart. Never manufacture duration through compulsory waits.

## Prototype gate and uncertainty

One-day test: two 60-second batches, two materials, one module choice, bank/return behavior, end score and same-seed retry. Temporary meshes and a few sounds suffice. Before promising the estimate, obtain engineering confirmation of capped planar motion, reserved capacity, source-cell return and bounded layouts; all are unbuilt.

Observable acceptance: the owner makes a deposit unaided within 20 seconds; explains a rejected core; deliberately changes one route or bank timing; notices the module's effect; finishes and can restart without instructions. The minimum technical check covers deposit boundaries, capacity reservation, timeout, focus pause and reproducible restart.

Playtest question: **“After one shift, what would you do differently on another?”** Observe the replay, not just the answer. Repeatedly chasing only cores regardless of module or layout falsifies the proposed route depth. Repeated accidental releases question manual dumping. Neither agent approval nor passing checks proves fun.

Smallest next iteration: adjust core placement/value or basket size, then repeat the same short test. Do not answer boredom with a third material or another system.

## Designer discussion and cuts

Peer discussion accepted bulky cores, nearest-first capture, capped travel, soft targets and milestone modules. Both challenged my full-load bonus; removing it preserves useful partial deposits. Designer 2 rejected a wallet/retry economy; guaranteed modules remove farming and a shop. Designer 3 challenged local dropping; source-cell return avoids rearrangement exploits and placement edge cases. We adopted designer 2's shorter timings and designer 3's saturation warning.

Designer 3 initially preferred automatic furnace banking, then accepted manual release as the baseline with a generous pad. Auto-bank is the first friction fallback if observed misdrops recur; do not implement both initially. We reject polarity/purity sorting, heat, combos, perfect-dump bonuses, permanent power, realistic magnetism, lore, extra environments and online records. Largest scope risk is polishing pickup/carry/drop feedback into satisfying motion; protect that work by cutting content before weakening the representative loop.

---

## Gameplay revision — 2026-09-11, after owner correction

**This revision supersedes my gameplay recommendation above.** Preserve the earlier text as the reasoning history, not the implementation contract. Klaus clarified that development timeline is not a consideration for this critique. Simplicity, enjoyable action, rewarding growth and replay remain the objectives. There is still no implementation or player evidence.

### What I withdraw, and why

I do not defend compulsory attraction while carrying, cheap bits as obstructive filler, source-cell rewinds, a hard cargo cap or the default countdown on player-experience grounds. They can make the desirable action—sweeping a broad path through scrap—feel like a mistake. Wider attraction exacerbates the penalty, and early banking becomes damage control. The old design found decisions by making the tool less cooperative. Those decisions might be interesting to some players, but they do not best express this magnet fantasy.

Replace that model with **a cooperative sweeping toy that grows into an expressive salvage tool**. Keep retained improvements across a finite salvage journey. Give the player opportunities to create larger releases and cleaner sweeps; do not use a timer or score screen to make an otherwise empty action seem meaningful.

### Revised input and response

1. **Sweep:** move the magnet directly with the pointer and hold the primary button to attract loose scrap. Release turns new attraction off and keeps existing cargo. All loose scrap contributes usefully to salvage progress. Moving through a rich patch should feel generous. There is no hard cargo cap or carrying slowdown.
2. **Dump:** bring the load over a generous, clearly highlighted furnace pad and click once to empty it. That explicit action triggers the substantial drop, furnace flare and progress payoff. Clicking elsewhere does not lose or reset cargo. Traveling to the furnace is a natural punctuation in collection, not a precision challenge.
3. **Tug:** a few visibly tangled clumps have a clear pull-ring. Starting a drag on a highlighted ring latches it; an ordinary held sweep crossing a ring never latches accidentally. Drag the magnet away to stretch a visible tether and preview a broad corridor. Release commits the yank: the clump comes toward the magnet, scooping loose scrap in its path. A short direct pull is valid and rewarding. No charge waiting, rapid flick, timing window or repeated strength check is required.
4. **Grow:** a later magnet evolution makes a pulled clump break visibly connected tangles loose. Their freed scrap produces a catchable cascade. Another improvement can extend tug reach, opening useful approach angles; a wider catch field then helps gather the release. These affect action and spectacle, rather than creating a catalogue of statistical options.

**Input priority is fixed when the button goes down:** over a highlighted furnace with cargo means dump; otherwise over a highlighted ring means latch; otherwise sweep. The chosen state persists until release. Entering a ring or furnace during a sweep never changes that state. Releasing a sweep retains cargo; releasing a latch commits its previewed pull; releasing after a dump has no further effect. The cursor, ring/tether and furnace highlight must clearly show the pending action before clicking. There is no extra mode button.

**Exact linked-release rule:** the selected tangle always releases. With the Breakaway improvement, a neighbor also releases only when it is visibly connected directly to the selected tangle and its ring lies inside the broad previewed pull corridor. Highlight the complete affected group before release. Those neighbors do not trigger further neighbors during this pull. Thus the chosen starting tangle, direction and visible connections matter, with no hidden whole-board chain reaction. Freed tangles become salvage drawn into the carried haul and cannot trigger twice. A straightforward pull still succeeds even when it catches no neighbor.

The pull corridor is forgiving and previewed. There are no ricochets or exact angle requirements. Released salvage is drawn into the carried haul; a successful cascade never becomes overflow to mop up. Keep the growing cargo visually offset from the field and tug preview, with abstracted bulk if individual pieces become unreadable. Show how the carried material would advance the furnace fill line. Realistic magnetic force and collision simulation are unnecessary to the proposed behavior.

Root has been informed of these changed dependencies. Before implementation, an engineer must review direct manipulation, contextual ring input, corridor selection, linked-clump state and persistent clearing. These systems are unbuilt; this document does not establish their feasibility or estimate their schedule.

### A vivid opening and a meaningful choice

The player first sweeps a crescent of washers and drops a visibly substantial haul into the furnace. The furnace fills part of a clearly displayed magnet improvement. Nearby, a tangled assembly has one obvious ring. Clicking and dragging it creates a springy tether; releasing produces a satisfying pop and sends several pieces toward the magnet. Even the simplest pull gives that payoff.

The next tangle sits beside a seam of loose scrap. A novice yanks it straight toward the current position and makes progress. A more attentive player walks the magnet around to the far side, sees the wider highlighted corridor cross the seam, then pulls. One authored gesture collects a richer burst. After the linked-release evolution, a similar setup erupts into several connected tangles coming apart. This is the intended memorable moment: **“I lined that up, pulled once, and the whole mess came loose.”**

The player can bank once the carried contribution will finish the current forge goal, or enjoy a longer sweep before the payoff. There is no risk of cargo loss and no forced return cycle. We should not pretend this harmless deposit choice creates deep strategy; it lets the player control the rhythm. Expressive choices come from selecting and aiming pulls that affect different visible groups.

### Progress, ending and replay

Use a finite sequence of salvage jobs. Each furnace has a visible fill line showing the material needed to forge the next real improvement to the magnet rig. **Any scrap contributes by material amount.** There is no high-value core efficiency trap, secondary wallet, shop or requirement to farm completed jobs. Supply more salvage than the goal requires: broad sweeps, rich tugs and mixed approaches can all finish the delivery while leaving flecks behind.

Crossing the fill line never cuts off the current pull, cascade, dump or its feedback. Finish the whole action, credit the whole haul, illuminate the completed forge and let the player admire or keep sweeping before proceeding. The installed improvement remains owned and visible. Do not scale every new fill requirement upward to cancel stronger tools; their increased effectiveness must be felt. Tune the target from enjoyable actions and substantial progress per haul, not desired minutes.

Failure here is a missed opportunity for an elegant pull or an inefficient sequence, not erased work. A poor angle still frees the chosen clump. Playtests must show whether this forgiving structure feels satisfying or inconsequential. If it is dull, add a better interaction or job arrangement rather than an arbitrary punishment.

Revisiting an earlier job with the improved rig should demonstrate earned power. After the finite progression, offer remixed deliveries using the same complete rig and actions. Change loose seams, starting tangles, their connections and approach space so a different starting point or direction creates the best release. Cosmetic position jitter does not qualify. Keep the salvage budget comparable when comparing arrangements. A new arrangement should present a new opportunity to make a satisfying sequence, not merely another quantity to grind.

Neither permanent ownership nor remixed placement proves replayability. Observe whether the player returns with the full rig and invents a different sequence after all unlock novelty is gone. Do not force resets or add a timed mode to compensate for absent interest.

### Actual dialogue, unresolved judgment and tests

The independent critic and I separately rejected cargo-loss controls. The critic challenged my first alternative, a held charge for anchored scrap, as hovering and waiting; I withdrew it. Designer 2 proposed a backward tug and connected release. I rejected a velocity or weak-tug check, favoring deliberate drag/release with no reflex requirement. Both accepted the revised loop. The critic challenged exact corridor puzzles and repeated tugging of tiny objects; I accepted broad previews, occasional large tangles and rewarding direct pulls. The critic also accepted an explicit forgiving dump rather than mandatory auto-banking.

For the job goal we considered extracting a special component from a frame. Designer 2 challenged this as a drift into structural extraction puzzles, or an immediate prize rush that bypasses sweeping. The three of us instead selected the furnace fill line: it supports the original collect–haul–improve fantasy and permits different salvage routes without demanding complete clearance. The critic correctly distinguishes a visible, rewarding forge goal from a quota that merely delays another number. That distinction remains a playtest question, not a settled success. Structural prize/support puzzles are rejected from this recommendation.

Root subsequently challenged preserving a hard cargo cap after removing scarcity routing. Designer 2 and I agreed: it would interrupt the best cascade without providing a useful new decision. Remove it, preserve a visible growing haul and preview its contribution to the forge. This is the final recommendation, superseding even the temporary overflow fallback discussed during this revision.

Predicted effects: retained cargo improves trust; an intentional dump preserves tactile authorship; tugging introduces spatial expression; a cascade supplies anticipation and earned power. These are hypotheses, not findings. Biggest uncertainty: whether contextual sweep/tug input remains natural, or whether ring targeting interrupts the sweeping pleasure.

The next playable comparison needs loose scrap, a forgiving dump and one rich tangle, then a second arrangement and the linked-release evolution. Observe comprehension, accidental latches, voluntary angle changes, delight or indifference at release, and whether the evolved tool prompts an unrequested return. Check that the group preview matches every actual linked release and that filling the furnace preserves the payoff. Test remixed deliveries with the complete tool, not only enticing unlocks. Ask: **“Which part would you want to do again, and what would you try differently?”** If players repeatedly prefer simple sweeps and ignore pulls, reconsider the tug; if sweeps feel empty between sparse tangles, reconsider density and feedback. Do not announce either route successful before play.

Rejected additions: charge meters, required rapid flicks, precision sorting, rewind penalties, default deadlines, obligatory cleanup of every scrap, unrelated trophy progression and large skill trees. The smallest next iteration is an observed sweep–tug–dump–improve loop, not a larger list of mechanics.
