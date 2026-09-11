# Magnet Sweep: replay through routing and tool choices

**The initial proposal below is historical. The appended “Current replay recommendation” supersedes its timed runs, power resets, carrying limits, score competition and development-time assumptions.**

Design proposal, 2026-09-11. Owner: replay designer. No implementation or player evidence exists. The rules below reflect discussion with the interaction and progression designers; the integration owner selects the final baseline. The game-prototype skill informs the experiment boundary.

## Promise and repeatable loop

**Turn a messy tray into a satisfying salvage haul, then replay with a smarter route and a differently upgraded magnet.** The pleasure should come from physically readable collection and dumping; mastery should come from choosing which material deserves the next trip. Neither random decoration nor progressively larger numbers establishes replay value.

Recommend one short shift containing three finite batches. The provisional timing is 60/90/90 seconds, plus two untimed upgrade intermissions and a result. These are starting values, not a promised session length: density and timer must follow observed play. A finished shift should feel complete even when the player misses its performance target.

Move the mouse to guide the magnet toward the cursor with fast capped travel, initially about one second across the tray. Hold the primary button to attract nearby pieces, then release over the highlighted furnace pad to bank the cargo. Nearest pieces capture first, with stable ordering. A visible field and capacity display explain what is happening. Travel cost matters; instant cursor teleportation would erase much of the routing decision.

Begin with capacity 12 and two materials: bits occupy one space and score one; distinctive cores occupy four and score eight. A magnet carrying nine bits cannot take a core. The player can finish filling nearby, return early to free space, or deliberately approach a core without sweeping through clutter. These are testable choices, not automatically interesting ones. No full-load multiplier: making every partial deposit inferior would undermine the choice.

The held field remains active while carrying. Releasing outside the furnace visibly returns cargo to its original cells without destroying it. This is a recoverable input error and correction action. Banked salvage fills a small haul display. At each intermission, choose Wide Field (+35% attraction radius) or Deep Basket (+4 capacity), up to two ranks. Modules are guaranteed; there is no shop or accumulated currency requirement. Bigger sweeps and bigger hauls must visibly change the next batch.

## Ending, retry, and records

At timer expiry, stop capture; unbanked cargo returns and does not score. Signal the closing seconds clearly so banking is a decision rather than a surprise. Do not add a separate grace-period rule initially. If every piece has been banked, end the batch immediately.

The final result shows banked value against a stated work-order target, the completed haul, chosen modules, and personal best. Missing the target changes the performance stamp, not access to the ending or modules. Retry never costs currency. Rank by banked value; use total active time only to break equal-value results. No time-bonus formula, combo multiplier, or score conversion economy.

“Retry this order” restores the exact layout and baseline magnet. “Choose another order” selects another authored preset when included. Pause freezes time and input; focus loss must pause safely. Save completed-order records, best results, and settings. Initially, quitting an unfinished shift loses only that attempt; state this clearly before confirming exit. An active checkpoint is an engineer-costed option if observed session interruptions justify it, not a required new system. A clear-save action must be separate from ordinary retry.

## First and repeat sessions

On the first batch, nearby bits demonstrate attraction and one obvious core demonstrates why free capacity matters. The furnace highlight teaches banking through action. The first module arrives promptly after that batch. The player selects Wide Field, sees a much broader stream of bits in batch two, and learns that collecting indiscriminately can prevent another valuable pickup. The third batch provides the completed haul and a result worth understanding.

On the second shift, the player retries the same order, chooses Basket, takes a core pocket before nearby filler, banks a deliberately incomplete load to free four spaces, and compares the result. Alternatively, another authored arrangement should make the broad sweep attractive. A repeat is successful evidence only if the player notices a different decision or enjoys improving execution; pressing restart merely to follow a test instruction is insufficient.

Do not force a faster opening after the first clear. Different timings would spoil comparison while hiding an opening that needs improvement.

## Minimal replay content

The prototype uses one order. A conditional commercial ceiling is three presets, each using the same tray, materials, controls, and module choices. Start with deterministic layouts; procedural generation is not essential. Mirror or jitter variations can wait. Preset records must identify the layout/tuning version so changed rules do not overwrite comparable bests.

| Preset hypothesis | Spatial change | Expected decision difference |
| --- | --- | --- |
| Near-field sweep | Compact bit patches near the furnace; cores dispersed farther away | Wide Field may make several quick cheap loads beat long core detours. |
| Heavy route | Separated core pockets with filler on direct return paths | Basket may support two or three cores per trip; approach angle avoids unwanted bits. |
| Mixed pockets | Valuable pockets alternate with broad bit patches | Mixed upgrades may support opportunistic switching; a fixed core-only route should lose opportunities. |

These are balancing hypotheses. If core hunting wins everywhere, there are not three meaningful presets. If Basket is always best, there are not three meaningful final builds. Expect roughly four to eight additional production hours for three presets and initial tuning, subject to engineer/planner review; no new meshes or systems are required. Do not translate that estimate into a guaranteed amount of entertaining content.

## Cheapest decisive test and cut order

Build one legible tray, both materials, capacity, furnace dumping, a timer, two module choices, and restart. Temporary appearance and sound need enough clarity to judge pickup and deposit feedback. Engine constraints remain unverified; ask the engineer to confirm bounded source cells, deterministic capture, return-to-origin release, capped travel, and clean batch reset before accepting cost estimates.

Give Klaus roughly twelve minutes for a complete attempt and an optional repeat. Ask: **“What did you change on the second attempt, and did that make you want another?”** Observe deliberate partial banks, changed routes after a module, misunderstood rejection, accidental drops, full-clear times, and voluntary replay. Readability improves if blocked cores clearly show four required spaces; challenge improves if detours compete with volume; pacing improves if upgrades arrive before repetition; appeal improves if larger deposits produce an unmistakable payoff.

Critical risk: 64 pieces, capacity 12, and fast travel might clear a tray in 20–30 seconds. The proposed longer timers would then create no pressure. Measure first; shorten batches or reconsider the format through the producer. Do not create slow pickup, mandatory waiting, repeated waves, or extra upgrades merely to manufacture duration.

Cut random generation, extra presets, badges, and elaborate haul decoration before cutting routing, the contrasting module choice, or clear deposit feedback. Replace manual release with auto-bank only if observed input friction warrants that change. If two tuned attempts still feel like obvious core harvesting, revise one interaction rule before expanding content. Without voluntary replay, reject the proposed paid scope and revise or park. A four-minute loop can validate a mechanic while remaining too thin for a paid game; the producer must assess that honestly.

## Discussion record

- Interaction accepted my challenge to remove the full-load bonus, cap magnet travel, use stable pickup order, and test early full clears. I accepted its manual release-to-bank preference; auto-bank remains a fallback.
- Progression accepted shorter consistent shifts, no permanent power grind, and no extra starting-loadout tree. I accepted guaranteed modules and a soft performance target; quota-only play offered insufficient pressure.
- Both discussions converged on two materials, two module choices, deterministic layouts, and conditional additional presets. Unresolved: timer/density values, whether numeric modules change tactics enough, and sufficient paid-game substance. These require a playable experiment and producer decision.

---

## Current replay recommendation — 2026-09-11

This revision follows the owner's removal of development timelines as a consideration and the independent gameplay critique. It replaces my earlier recommendation. The interaction and progression revisions establish the current mechanics; the discussion below tests their replay promise. No game has been built or played.

### Judgment: a stronger toy, with an honest replay risk

The revised concept better serves the fantasy: collect desirable scrap, make a satisfying haul, and grow a magnet worth keeping. Safe cargo, unrestricted carrying and untimed jobs remove reasons to distrust the tool. Directional tugs offer an authored action rather than a choice between doing chores quickly or slowly.

However, unlimited time and useful salvage mean that most arrangements can eventually yield their entire supply. Total haul therefore describes completion; it does not demonstrate deep scoring. The meaningful distinction is **how enjoyable and intentional the individual pulls are**, not whether one eventually obtains the available material. Revisiting an early job with a powerful magnet could be pleasurable once without establishing lasting replay.

My concrete repeat motive is: **“I can see a different impressive pull to make in that next mess.”** After a finished delivery, the full rig remains equipped and another arrangement offers visible possibilities. A new starting ring, direction, or loose seam should invite an experiment. The result is a satisfying release, a growing haul and a deliberate furnace dump. No new mode, compulsory reset, escalating grind or recurring unlock is required. The unchanged basic action must remain worth doing after novelty fades.

After the rig is complete, the furnace must honestly promise a completed salvage delivery, not a nonexistent further upgrade. The familiar pour and forge can produce a recovered-metal block and “Delivery complete.” That is the finished work's visible payoff, not a new currency or collection catalogue. Exact-arrangement retry restores that delivery's scrap and fill state while retaining the earned rig and earlier progression.

### Rules that the arrangements must respect

The interaction designer confirmed these details directly:

- Primary-button down chooses one state: highlighted furnace with cargo dumps; otherwise highlighted ring latches; otherwise holding sweeps. The state does not change while held. Releasing a sweep retains cargo; releasing a latch commits its previewed yank.
- The yank corridor is a finite, broad capsule from the selected ring to the magnet's release position. Reach remains finite at the fully improved rig, visibly indicated before release. A short pull remains successful; no strength, speed or exact-angle test is added.
- The selected tangle releases. With retained Breakaway, only its **directly linked neighbors** whose ring centers lie inside that corridor also release. Newly released neighbors do not propagate another release. Loose salvage within the corridor joins the same haul.
- Preview the actual affected group. Cargo and effects must not conceal the path or rings. A spectacular burst should confirm the player's intention rather than become an unreadable whole-board event.

Finite reach creates recognizable local opportunities; it must not be secretly reduced or counteracted when the player improves. New deliveries give the completed rig appropriate space to express its reach. Furnace goals stay below total salvage and never interrupt a pull, its collection or its deposit. Completing the goal leaves optional cleanup and a clear choice to continue with another delivery.

### A remix grammar that changes actions

Use arrangement templates defined by **links, positions, loose seams and approach space together**. A template is successful when those relationships change an appealing action. A rotation, color swap or slight position jitter of the same obvious pull is not a new decision.

Each delivery should contain several generous opportunities, with a straightforward sweep or direct tug available alongside a richer alignment. Avoid making the whole tray one hub-and-spoke graph; finding its hub once would solve subsequent deliveries. Also avoid narrow access gates, mandatory preparation sequences and one precise correct corridor: those would turn an expressive collecting toy into a different puzzle game.

Two concrete examples using the same full rig and controls:

| Arrangement | Visible structure and satisfying approach | Why it changes play |
| --- | --- | --- |
| **Linked fan** | Put a starting hub on the left. Three directly linked rings lie to its right, within one broad eastward corridor; a loose seam runs among them. A pull from the hub toward the far right visibly highlights the hub, all three neighbors and that seam. A leaf-start pull can still collect its own clump and the hub, but cannot propagate through the hub to the other leaves. | Invites recognizing the useful starting ring and aiming beyond its neighbors, then authoring one broad multi-tangle release. It demonstrates Breakaway clearly. |
| **Offset fork and seam** | Spread a central hub's three directly linked leaves into separate directions, far enough apart that one corridor cannot include all three. Put a rich diagonal loose seam from an outer leaf toward the hub and beyond. Starting at that leaf and pulling along the seam collects the leaf, the hub and substantial loose salvage; the other leaves remain because they are not directly linked to the selected leaf. A hub-start pull toward a different arm remains rewarding. | The most connected ring is no longer automatically the most appealing start. The player can favor a seam-rich two-tangle pull or a different clump-focused pull. Geometry and loose material change the gesture, without adding another control or punishment. |

Keep overall supply comparable while comparing these examples; otherwise a richer-looking result could simply come from more available material. These are intentional test arrangements, not proof that a generator produces good content. Author further templates from successful interactions, then validate any remixing against readability, reachable corridors and preserved alternative opportunities. No procedural system is necessary to test the idea.

### Why a largest-yank record is not the baseline

I initially proposed an optional same-arrangement best, counting only material newly collected by one yank. The critic identified a remaining incentive conflict: even an optional comparison can make a pleasant earlier sweep feel like a mistake because it consumed material that could have inflated the yank. That recreates the former design's temptation to avoid collecting scrap.

I accept the stronger recommendation: **start with positive feedback for the current pull, without a standing best to protect.** A larger physical release, readable haul growth and an appropriate sound can communicate accomplishment. Do not attach mandatory yank targets, stars or a predicted maximum-score meter to every tangle. The player should want to pull because it looks enjoyable, not because a record makes ordinary sweeping suboptimal.

A largest-yank comparison remains a later hypothesis only if observed players find it helpful. It must exclude previously carried material and earlier sweeps, compare identical layout/rig/rules when implying improvement, and remain unnecessary for progress. Total untimed haul must not masquerade as an equally meaningful competition. Removing comparisons should not collapse the game's reason to repeat.

### What the next play must establish

Test with the fully improved rig and no pending unlocks, after the player already understands sweeping, tugging and dumping. Present the fan and fork without naming their intended tactics. Ask **“What looks worth trying here?”**, observe the action, then allow the player to choose whether to see another delivery. Distinguish prompted trials from spontaneous continuation.

Promising observations would be an unprompted change of starting ring or direction, satisfaction with the intended release, comfortable ordinary sweeping between tangles, and interest in another arrangement after the forge rewards stop. A player experimenting with a different pull after noticing a seam is stronger evidence than agreeing that the tool looks powerful.

Failure evidence includes the same indiscriminate gesture working everywhere; clicking rings only to watch a repeated animation; laboriously searching previews for one optimal answer; ignoring tangles because sweeping feels better; or stopping immediately when improvements end. A finite enjoyable journey would still be a valid finding, but would not meet the requested replay promise by itself.

The smallest next iteration is to change one spatial relationship in the weaker arrangement and observe whether it produces a different appealing action. If both arrangements are understood but neither invites repetition, revise the feel or expression of the pull before adding more templates. Neither a larger content catalogue nor random placement can establish missing enjoyment. No duration or labor ceiling governs this judgment.

### Revised discussion record

- Accepted the gameplay critic's rejection of the initial timed, capacity-limited loop. Retained power and harmless collection now govern the recommendation.
- Interaction confirmed the finite capsule, direct adjacency, contextual input and safe retry semantics. It accepted the fan/fork examples as consistent with the proposed mechanics; this is a rules review, not play evidence.
- Critic challenged my early bent-chain example: a downstream tangle cannot release without a direct link. Replaced it with the explicit fork/seam example, with no hidden propagation.
- Critic rejected my optional-best compromise as insufficient protection for enjoyable sweeping. I withdrew standing comparison from the baseline and retained only the conditional hypothesis. Root accepted that correction.
- Remaining uncertainties: whether changing geometry creates expressive play instead of preview searching, whether the act stays pleasurable with no unlock ahead, and whether fresh deliveries earn voluntary repetition. No additional mechanics are recommended before those observations.
