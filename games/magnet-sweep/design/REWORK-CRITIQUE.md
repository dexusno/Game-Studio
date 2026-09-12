# Gameplay critique: consequential salvage rework

2026-09-12. Independent gameplay review requested after Klaus played the concept demo and found it boring. Owns this document only. No implementation or playtest of the rework is claimed. Development timeline is not a criterion.

## Governing follow-up after native play

This document preserves the initial design critique below. The implemented risk rule was subsequently corrected: an expired fuse consumes one fuel charge plus the named most valuable unbanked piece; RMB drops the entire haul recoverably and turns the field off. These supersede selective sorting and zero-fuel failure below. Native narrow approaches isolated the rare core and copper groups while a broad pull gathered adjacent cells. Free destruction of a lone cell was actually reproduced. Automatic-sort dominance was an inference, not a completed comparison. The new gameplay risk is tedious recollection after dropping a good haul; nearby readable component-preserving spills must allow useful precision recovery. Current exact results are in [QA-REWORK.md](../QA-REWORK.md); BRIEF.md governs the final rules.

## Evidence and verdict

Owner feedback is the first actual enjoyment evidence: the concept lacks meaningful goals, progress, melt payoff, capacity, risk, challenge, clear links and convincing magnetic impact; graphics/audio also disappoint. Earlier agent agreement and functional checks did not establish fun.

Static implementation inspection supports a particular concern: `SalvageModel::SweepAt` and `CommitPull` immediately transfer selected pieces to cargo; runtime animation subsequently interpolates them to attachment positions. That establishes ownership before the supposed magnetic journey matters. More particle effects around that interaction will not by themselves produce magnetic handling.

**The accepted rework is a haul-planning game with visible magnetic control, valuable recoveries, limited furnace heats and recoverable mistakes.** It is a promising contract to implement, not a validated answer to the boring demo. The chief remaining danger is replacing effortless sweeping with an equally automatic collect–vent–smelt routine.

## Settled contract after direct debate

- Four furnace heats per contract, shown as four fuel charges before play. No global countdown, field battery or integrity meter. A nonempty completed smelt consumes one heat. Preview its value, quota contribution and remaining heats on furnace hover; empty clicks consume nothing.
- Starter safe cargo mass is 24 kg. Actual attraction moves pieces before capture, and actual held mass determines stability. Releasing attraction retains cargo but does not remove an existing overload. Narrow-field control and visible repulsion permit deliberate extraction around nearby scrap and hot cells.
- Implementation alignment from the model owner adds a hard capture ceiling at 150% of safe mass (36 kg on the starter rig). Its preview must distinguish safe, unstable and too heavy to attach. Do not promise a whole linked bundle and then silently capture an arbitrary subset at that ceiling. This limits whole-tray brute force but does not by itself solve automatic vent sorting.
- A connected bundle has visible physical links and a shared mass/value preview before commitment. Remove the previous ring/corridor selection puzzle. A nearby cluster must move and tug as linked material, not teleport because a UI line crossed it.
- An overloaded or hot-cell-contaminated haul enters the same three-second warning fuse. Stabilizer tiers add one second each. Name the dangerous cause and the precise highest-value salvage piece at risk. Pause/focus loss freezes the situation safely.
- **The furnace refuses unstable cargo.** Hover must explain “Unstable load — eject excess/hot cell first”; it spends no heat and does not secretly bank the load. This supersedes the earlier proposed race to the furnace.
- RMB ejects captured hot cells first, then salvage with the lowest credit value per kilogram until the load is stable. Equal choices need stable deterministic ordering. Ejected salvage remains recoverable. The piece/order highlight must explain the automatic selection; it cannot feel like random valuable cargo loss.
- **On fuse failure, destroy the single highest-value unbanked salvage piece, consume captured hot cells, and scatter the remaining cargo recoverably.** Stable piece ID breaks equal-value ties. Show the endangered piece and its value during warning; show the actual destroyed value and recoverable spill afterward. Never use an undisclosed approximate percentage.
- Previously smelted quota, money, earned XP/ranks and purchased equipment remain safe. The lost piece removes only carried/projected progress. Four spent heats below quota fails that contract and forfeits its success reward; it does not confiscate previous earnings. An unreachable quota needs an honest result/retry path, never stranded play.

The root selected three upgrade families, three tiers each: capacity (+8 kg), coil (+35% range and +25% force), and stabilizer (+1 second fuse). These must visibly change handling or a useful decision. Rare cores bank into an album and are optional valuable discoveries, not required random drops that block progression.

Cash buys equipment; XP/ranks unlock actual contracts and equipment tiers. Keep XP on smelt results/workshop screens, not another active danger bar. Do not award XP repeatedly for capturing and ejecting the same object. A rank that unlocks nothing is not meaningful progression.

## The intended risk, concretely

A player carries 12 kg of iron worth 24 credits and sees an 18 kg connected copper bundle worth 72. The preview clearly forecasts 30/24 kg and a warning. Banking now uses a heat for 24 credits. Extracting the valuable bundle and correcting the overload lets the player retain a richer 24 kg load in one heat; some cheap iron is ejected and remains available. Missing the warning destroys the highest-value copper piece and scatters the rest.

That trade gives risk a purpose: better material in a limited number of heats. Risk is not required merely to finish the introductory contract. Controlled, useful loads should complete its ordinary quota; better extraction can pursue a higher grade, bonus or rare recovery. The exact material values and quotas remain tuning hypotheses.

The proposed first contract pays its 120-credit quota plus an 80-credit completion reward, enough for a first 150-credit equipment choice. This gives successful play a clear advantage over repeatedly aborting tiny harvests while preserving earned cash. Test the economy; the numbers alone do not prove that farming is unattractive.

## Objections the implementation must answer

1. **Do not make safety an automatic sorting macro.** Root's cheap-first vent is understandable, but “hold until red, press RMB, smelt” may always produce the best safe load. If that routine beats thoughtful extraction everywhere, the warning is a recurring keystroke rather than an interesting risk. Meaningful field shaping, bundle selection and physical approach must actually matter in play.
2. **A furnace heat must be a valuable batch opportunity.** Four charges defeat infinite tiny trips only within a contract. If every deposit feels like paying a tax, or failure still farms equipment faster than success, revise the economy and goals. Avoid confirmation popups and long mandatory melt waits.
3. **Different materials must invite decisions rather than turn most scrap into trash.** Iron can support reliable base completion; copper/alloy and rare bundles can offer better returns with placement/handling complications. Do not put freely accessible superior-value material everywhere and call the result a risk game.
4. **Failure must feel caused and comprehensible.** It must be possible to see the incoming mass, watch attachment, hear the warning, identify the endangered item and correct the load. A sudden capture followed by deletion is arbitrary punishment. Destroyed salvage needs a convincing shatter/burnout effect distinct from recoverable spill.
5. **Physics and melting are gameplay evidence requirements.** Light pieces should start moving sooner than heavy linked bundles; release before contact must visibly affect them; pulse direction should move material where expected. Smelting should show the haul arriving, heating/melting, then producing a useful payout and installed improvement. A counter changing behind decorative motion repeats the demo's weakness.

Soothing music can support concentration without hiding strain warnings. Sound must distinguish pull, contact, loaded rattle, unstable strain, deliberate vent, actual failure and furnace completion. Attractive materials, readable links and meaningful animation must be judged in the running scene.

## Playable acceptance scenarios

| Scenario | Required observation |
| --- | --- |
| First haul | Without explanation, player identifies quota/four heats, visibly pulls material, predicts the first payout, and understands the melt result and next equipment choice. |
| Controlled versus greedy extraction | The same mixed patch supports a deliberate narrow extraction and an appealing risky bundle attempt. Player can explain why they chose one. |
| Warning correction | Enter overload, release LMB, verify warning persists; RMB ejects the previewed low-value material/hot cell, clears danger and preserves banked state. |
| Failed risky move | Let the fuse expire. Exactly the named highest-value piece is lost, other cargo visibly spills, previously banked money/quota/rank/mods stay unchanged; displayed loss matches settlement. |
| No furnace bypass | Attempt unstable deposit. No heat, payout or quota changes; refusal is legible. Stabilize and deposit normally. |
| Anti-trivial play | Compare four tiny deposits, thoughtful mixed loads and repeated collect–vent–smelt. Tiny loads should miss the contract; the vent macro must not erase the benefit of planning every extraction. |
| Reward and replay | Buy a chosen mod, use its effect in a different arrangement, then observe whether the owner wants another contract after the initial unlock novelty. |

Ask after the risky attempt: **“What did you stand to gain, what could you lose, and what would you do differently?”** Record unaided understanding and actual behavior. Disagreement, indifference or refusal to replay is useful evidence; do not coach a positive answer.

## Debate outcome and next step

The loop designer, progression designer and critic rejected a field battery meter and agreed on clearly advertised furnace heats. We challenged meaningless XP, hidden loss fractions, field-off overload immunity, instant magnetic ownership and failed-contract farming. Root settled exact loss, three equipment families and furnace rejection; those supersede earlier suggestions for percentage damage, a severe second timing band and racing an unstable load to the furnace.

Implement this coherent loop and test its actual controls, physical response and consequences before adding another meter or reward catalogue. The most important next observation is whether handling a tempting rich bundle creates a satisfying, understood decision rather than a routine safety-button press.
