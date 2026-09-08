# Segmented shield: selected direction and next experiment

Updated 2026-09-08. Owner direction, now implemented as the folding six-piece 0.3.1-combat1 experiment; STATUS.md records verification. Exact tuning remains provisional. This supersedes that build's all-or-nothing deployment and global loss of guarding; it does not restore Sixfold's mandatory shoot-all/recall loop.

## What Klaus selected

- The shield consists of separate physical pieces. Holding LMB lights them one by one. Releasing launches exactly the lit pieces; unselected pieces stay at the arm for protection.
- A protecting piece loses energy when it blocks an attack. When spent, it disappears and that piece's timed regeneration begins immediately.
- Surviving launched pieces can be recalled and visibly reattach. Some enemies can destroy incoming pieces on impact; destroyed pieces instead begin regeneration immediately.
- The system must support creative upgrades and combinations, readable use of earned powers, and procedural levels. An ordinary full throw is a choice, never a compulsory attack cycle.

## Implemented baseline to test

Klaus selected a collapsed weapon form that unfolds into the broad shield while blocking. Independent piece transforms animate that transition and provide the current docking sockets. A longer full charge earns a stronger volley and one shared impact burst; early release preserves the partial-attack tradeoff. Full power requires all six selected pieces. Close combat uses alternating sweeps and a stronger third strike, with a short forward step constrained by collision. These tuning choices still require owner playtesting.

The experiment uses six pieces, solely as a readable starting count. One ordinary successful block spends one piece; more durable pieces are an alternative to test. Current tuning: 0.22s initial selection, 0.316s per next piece (1.80s for all six), 0.22s throw recovery, 3s independent regeneration. Q recalls and F bashes/rushes. These are playtest choices, not final balance.

The lit count is literal: three lit means three launch, even when the shield is incomplete. Caught or regenerated pieces arrive unlit. Release uses the pieces actually present and lit at that moment; a piece lost while selecting cannot be launched. A second partial throw remains available while earlier pieces are away.

| Piece state | Player-visible meaning | Exit |
| --- | --- | --- |
| Attached, unselected | Available for protection and close offense | Selected for launch or spent by a block |
| Attached, lit | Selected by the current LMB hold; still physically present | Release launches it; cancellation clears selection |
| Outbound or lodged | Physically deployed and unavailable for held protection | Recall, or destruction by a qualifying enemy attack |
| Returning | Travels back, can contact enemies, then visibly docks | Attaches intact, or is destroyed by a permitted counter |
| Regenerating | Absent, with its own visible progress at the arm/HUD | Reconstructs the same piece at the arm, unlit |

Every piece has one identity and exactly one state. Normal flight does not also regenerate a spare copy. Destruction removes it from recall immediately. Its timer runs independently of other pieces, continues during combat, and is not reset when another piece is destroyed. Pause freezes simulation. Reset/save rules need explicit implementation coverage.

RMB raises the remaining attached pieces. Initially, partial protection should mean fewer available blocks within a predictable frontal area; avoid random gaps or unexplained percentage damage leakage. A connected energy surface can communicate protection while its physical sections are missing. Group repeated collision callbacks from one enemy attack so one sword swing does not accidentally spend every piece. Heavier multi-piece costs, if used, must have a distinct tell.

Keep tap-LMB close offense and movement usable when few pieces remain. A core/grip strike remains usable at zero pieces; it provides no free replacement shielding. Q should consistently recall surviving deployed pieces. F is the heavy-action input in this experiment, avoiding ambiguous Q behavior while pieces are both held and deployed. Guard/charge cancellation needs a short input experiment, preserving useful partial protection rather than inheriting the whole-disc global lockout.

Elite destruction must be readable and answerable. Start with a frontal interception stance that destroys one incoming piece and then has a recovery. Players can flank, delay, fight close or deliberately sacrifice a piece to use that opening. Avoid passive unexplained destruction on every hit.

## Example allocation

With six pieces, light and launch three. Block one bolt: two remain attached, three are deployed, one is regenerating. An elite destroys one projectile: two attached, two deployed, two regenerating. Reposition and recall the survivors through another enemy: four pieces reattach while the two independent timers continue. The player can defend, fight close or select a new partial throw.

## Upgrades worth trying

- **Frost return:** outbound pieces chill; a returning piece shatters chilled enemies into a visible burst. The return route matters.
- **Storm anchors:** two lodged pieces create a damaging line between them. Leaving them deployed maintains the trap and costs held protection; destruction breaks that link.
- **Mirror counter:** a precise block stores the attack's energy for a clearly illuminated outgoing piece or close strike. The spending trigger and stored amount must be displayed.
- **Reforge bash:** landing a committed close bash advances one regeneration timer, once per attack. An exposed player earns faster recovery by taking a readable risk.

Frost return, Mirror storage and Ram II rebuild advancement are implemented in the experiment. Storm links between anchors remain a proposal. None establishes tested fun. Piece launch, outward hit, return hit, block depletion, destruction, reconstruction and catch provide distinct upgrade triggers. Keep causal chains bounded without silently flattening powerful earned combinations.

## Reward usability is part of the mechanic

Klaus could not tell how to use earned powers and did not see special effects activate. Every reward needs an explicit input or automatic trigger, the expected visible result, any cost/cooldown, and a retained explanation in the equipment screen. Rank upgrades must retain the base action instructions. Give an immediate optional practice target with the required setup: for example, two targets for a chain-lightning demonstration. Display relevant readiness, stored charges and piece state; use distinct animations/sounds/materials for different effects.

The earlier study profile contained Ram II and Frost I; that did not establish visible activation. The new code retains descriptions at every rank, displays cooldown and charge state, adds persistent Frost/Ember cues and independent Storm arcs, and supplies reward-specific practice. Actual verification and remaining limits belong in STATUS.md.

## Observable proof

The new per-piece runner replaces the old whole-disc assertions. Verify partial launch, retained defense, depletion, recall docking, airborne destruction and independent reconstruction together, then visible Frost return and earned reward practice. Repeat generated layouts to check route/cover variation and navigability. Owner assessment must establish whether allocation is fun rather than a compulsory cycle or waiting penalty.
