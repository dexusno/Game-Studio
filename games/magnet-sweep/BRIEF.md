# Magnet Sweep — game brief

Working title. Revised gameplay recommendation, 2026-09-11. Owner: Klaus. Three designers and a gameplay critic have exchanged and challenged ideas directly. No implementation or player test exists. This integrated brief governs earlier alternatives in the designer notes.

## Governing direction

Make a simple, engaging, satisfying, rewarding and replayable magnet game. **Development timeline is not a consideration**, as the owner explicitly clarified. Simplicity means a coherent set of understandable actions; it does not mean accepting shallow play to meet a deadline. The critic assesses player experience and potentially bad design, not schedule compliance.

## Player promise

**Sweep up a satisfying haul, line up a powerful pull that breaks a mess of scrap loose, and forge a magnet that can do even more.**

The recommendation is an untimed collection game with expressive pulls and lasting tool improvements. Every piece of scrap is desirable. The pleasure of scooping, gathering and pouring material into the furnace is central; skill comes from choosing an attractive starting tangle and a useful direction for the pull.

The earlier timed score attack is superseded. It created decisions by making cheap scrap obstructive, made wider attraction potentially worse, and repeatedly removed earned power. Those rules conflicted with the collection fantasy. The revised game should feel cooperative and generous while offering opportunities for clever play.

## The complete loop

1. A delivery places loose scrap and a few visibly tangled clusters on a fixed-view workbench.
2. Sweep freely to attract loose material. Cargo stays attached when you stop collecting.
3. Start a drag on a highlighted tangle ring, move the magnet to preview a broad pull corridor, then release. The tangle comes toward the magnet, gathering loose scrap along the path.
4. With an earned Breakaway coil, a well-chosen pull also releases visibly linked neighbors inside that corridor. A straightforward pull still succeeds; a clever one produces a bigger, clearly anticipated burst.
5. Bring the haul to the furnace and deliberately dump it. The whole load pours in, the furnace responds, and its visible fill line advances toward the next magnet improvement.
6. Forge and retain the improvement, enjoy the payoff, then choose the next delivery. Once the initial journey is complete, fresh arrangements provide new pull opportunities with the complete rig.

There is no default countdown, cargo-loss penalty, shop, second currency, prestige reset or mandatory cleanup of every last piece.

## Controls and predictable response

One primary mouse button handles the actions through a clear hover preview. The magnet follows the pointer directly across the usable tray; ordinary movement must feel responsive rather than trail behind a remote cursor.

| Button-down context | Action until release | Result |
| --- | --- | --- |
| Highlighted furnace pad while carrying cargo | Dump | Bank the load once and play the full payoff. Releasing adds no further action. |
| Highlighted tangle ring | Latch and aim | Drag to preview the pull corridor; release commits that pull. |
| Any other usable tray position | Sweep | Attract nearby loose scrap while held; release stops attraction and keeps cargo. |

The action is chosen when the button goes down. Crossing a ring during a sweep never latches accidentally, and entering the furnace during a held sweep never silently dumps. Hover cues must explain the next click before it happens. Clicks or releases elsewhere never rewind collected material.

The furnace pad is generous, visually clear and accepts an ordinary deliberate click. There is no accuracy bonus. Pulls need no charging delay, rapid flick, exact angle or reflex timing. A short direct drag frees the selected tangle. Longer reach changes the available corridor, not whether the player passed a hidden strength test. Preview the actual reachable endpoint when dragging beyond the current reach.

There is **no hard cargo cap or load slowdown**. A successful large pull must remain a reward instead of creating compulsory unloading trips. Cargo visibly accumulates without hiding the field, ring or corridor. The furnace meter previews how much the carried material would contribute, so the player knows when banking can finish the goal.

Pause or focus loss retains all earned and collected state, cancels an uncommitted aiming gesture safely, and resumes only after fresh input. Precise accessibility bindings and hold/toggle behavior must be tested against this context-sensitive input; do not claim an untested toggle implementation works.

## Exact cascade rule

The selected tangle always releases. Before Breakaway, the pull collects that tangle and loose scrap in the broad corridor.

After Breakaway, another tangle releases only if it is **directly connected to the selected tangle by a visible link and its ring lies inside the previewed corridor**. The corridor is a finite broad capsule from the selected ring to the final reachable magnet position, including its endpoints. A neighbor qualifies when its ring center is inside it. Highlight the entire affected group before release. Released neighbors do not recursively trigger additional neighbors during that action. Each tangle can release only once. Ordinary unrelated tangles remain available for another deliberate pull.

This makes the starting ring and pull direction matter while keeping the result readable. It supports a substantial local burst without a hidden whole-board chain reaction. The spectacle may unfold in a short sequence, but the selection rule is stable. Use controlled motion and a clear interaction model; realistic magnetic forces and physical scrap piles are not prerequisites.

The corridor is intentionally forgiving. The player should see an inviting sweep through material, not solve a narrow laser-alignment puzzle. If that distinction does not survive actual play, revise the interaction rather than adding more indicators around an unpleasant action.

## Objective, reward and progression

Any recovered material contributes consistently by its amount. Loose bits are not low-value mistakes; remove the old double-value-per-space core rule. Each delivery contains comfortably more salvage than the visible furnace goal requires. The fill line gives collecting purpose without making the last scraps mandatory.

No special goal-piece extraction puzzle or secondary cleanup quota is layered on top. Reaching the forging line never interrupts an active pull, cascade, deposit or its sound. Credit the complete action, celebrate the forged improvement, and let the player admire the result or keep collecting before choosing the next delivery. Extra material supports that delivery's visible haul and personal moments; it is not a currency needed to buy future progress. After completion, use “Next delivery” when carrying nothing, or “Pour & next delivery” when carrying surplus. The latter explicitly banks the remaining cargo using the existing pour payoff, waits for it to finish, then changes the arrangement. Never silently discard collected scrap or require another precision trip as a transition tax; uncollected leftovers may be left behind.

A proposed opening progression has three jobs. These are a clear learning-and-payoff arc, not a fixed content count or a promise of paid value:

- **Discover sweeping and the tug.** A crescent of washers makes the first sweep inviting; an isolated ring teaches a direct pull. The first forge produces the visible Breakaway coil. Bring this reward early enough that the defining burst is part of the opening experience.
- **Learn the bigger release.** Show an obvious linked pair, then a seam where a different starting ring or diagonal corridor releases more material. The next forge adds a longer-reaching arm and visibly improves the rig.
- **Enjoy the earned tool.** Present loose patches and linked groups that reward both learned aiming and the increased reach. The completed rig looks and feels more capable. Do not inflate every target merely to cancel its power.

Progress and the current delivery are saved: preserve the actual arrangement, recovered material, cargo, furnace state and earned rig. An explicit retry restarts the delivery while keeping previously earned improvements. Quitting must not quietly turn a relaxed collection session into lost work.

These are automatic meaningful milestones, not a menu of near-equivalent statistical choices. Ordinary field growth can accompany the rebuilt rig when it feels good. Agency belongs in selecting patches, starting tangles and pull directions. Further upgrades require a specific new enjoyable action, not an arbitrary need for a larger catalogue.

## What a satisfying moment looks like

A player spots a seam of loose scrap beyond two linked tangles. They could tug the nearest ring directly and still make useful progress. Instead, they choose the other ring and drag toward an open corner. The preview shows the linked neighbor and the loose seam inside the path. Release: the tether snaps taut, both tangles pop free, and a stream of material gathers into a large visible haul.

The player knows why it happened: they chose that line. A click at the furnace turns the load into a rich pouring sound, a warm flare and a visible leap toward the next coil. The intended reaction is: “That worked exactly how I hoped. What can I pull together over there?” This is a design intention, not a reported playtest.

## Replay that must survive the unlocks

Retain the rig and unlocked behaviors. Replaying an earlier job can demonstrate earned power, but that pleasant revisit alone does not establish replayability.

Fresh deliveries reuse the same actions and forge goal while remixing loose seams, ring positions, direct links and approach space. A valid remix must change a useful starting ring or pull direction. Cosmetic jitter is insufficient. Use a small understandable arrangement grammar and preserve readable, reachable opportunities; no new mode, forced weak reset or daily obligation is required.

The repeat motivation is discovering and executing another satisfying pull with a familiar tool. Give positive feedback for the haul caused by the current action, without mandatory stars, par scores or standing record pressure. The critic and replay designer identified that a largest-yank leaderboard—even a local one—could make ordinary sweeping feel like destroying a future scoring opportunity. Comparable personal bests remain a later hypothesis, only if play shows they improve the collection experience. Total delivery haul is descriptive: with untimed collection, clearing a tray saturates it and does not create deep score competition.

After the rig is complete, the same furnace line marks completion of a salvage delivery, not another nonexistent tool improvement. The deposit can forge a simple recovered-metal block and celebrate the delivery. No new currency, collection catalogue or false promise of endless upgrades is added.

Two concrete examples from the replay designer make the variation testable:

| Arrangement | Opportunity |
| --- | --- |
| Linked fan | A hub sits left of three directly linked neighbors and a loose seam. Starting at the hub and pulling right can gather the whole highlighted group. A leaf cannot release the other leaves through the hub. |
| Offset fork and seam | A hub's neighbors point in different directions, so one corridor cannot gather them all. A leaf beside a rich diagonal seam can make a more appealing start than the hub. Both approaches remain useful. |

The [replay proposal](design/03-replay.md) explains the geometry and comparisons; these are designed examples, not tested results. The critical human test is whether a player voluntarily requests another delivery **after earning the full rig**, notices a new useful setup and tries a different pull. Continuing only for the next unlock fails to demonstrate the requested replayability. Randomness or theoretical combinations do not substitute for that behavior.

## Presentation and practical boundaries

One attractive industrial workbench: readable metal silhouettes, a chunky magnet, visible coils and arm changes, bright rings and links, a warm furnace. Broad material streams, responsive tether motion and a decisive deposit carry the visual identity. Keep the board readable under the largest intended haul.

Sound should distinguish a sweep, latch, tug, linked release, deposit and forged improvement. Layer a large haul without clipping or burying important cues. Music is optional; dialogue and lore are unnecessary. Existing Unreal and Blender suit the proposed 3D scene; TRELLIS, Suno or ElevenLabs are optional asset sources when useful. Record origin and commercial rights for all assets actually used.

Offline single-player Windows is the current platform recommendation. No enemies, combat, open world, factory network, large crafting tree, online accounts, leaderboard service or live AI is needed for this concept. There is no development-time ceiling. The [production plan](PRODUCTION-PLAN.md) organizes dependencies, owned work and observable quality goals.

The commercial objective remains NOK 20,000 cash profit per game, with further games conditional on real results. Earlier research pricing and comparable successes are hypotheses, not evidence this game will sell. This assignment produces the design and plan; no game has been built or published.

## Play questions that govern iteration

- Does sweeping feel good before any reward menu or score explanation?
- Can a new player predict sweep, latch and dump from the hover cues without accidental action changes?
- Does a short pull feel worthwhile, and a considered pull feel noticeably better without requiring precision?
- Can the player explain the linked burst they caused, rather than just observe a noisy animation?
- Do unrestricted cargo and the furnace preview make a huge deposit satisfying, or does dumping become a perfunctory final click?
- Does the forge goal motivate another enjoyable haul without making the remainder feel like work?
- Do the retained improvements change what the player wants to try?
- At full power, do different arrangements produce different useful ideas and voluntary replay?

These questions assess the game, not adherence to a calendar. If a central action fails, improve or replace it before using more content to distract from it. The [gameplay critique](design/CRITIQUE.md) records the independent challenges and remaining risks; the designer notes retain the history of the debate.
