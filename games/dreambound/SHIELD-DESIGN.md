# Segmented shield: selected direction and next experiment

Updated 2026-09-08. Owner direction, expanded with working proposals. Not implemented in the current 0.2.0-study1 whole-disc build. This supersedes that build's all-or-nothing deployment and global loss of guarding; it does not restore Sixfold's mandatory shoot-all/recall loop.

## What Klaus selected

- The shield consists of separate physical pieces. Holding LMB lights them one by one. Releasing launches exactly the lit pieces; unselected pieces stay at the arm for protection.
- A protecting piece loses energy when it blocks an attack. When spent, it disappears and that piece's timed regeneration begins immediately.
- Surviving launched pieces can be recalled and visibly reattach. Some enemies can destroy incoming pieces on impact; destroyed pieces instead begin regeneration immediately.
- The system must support creative upgrades and combinations, readable use of earned powers, and procedural levels. An ordinary full throw is a choice, never a compulsory attack cycle.

## Proposed baseline to test

Use six pieces for the first experiment, solely as a readable starting count. One ordinary successful block spends one piece; more durable pieces are an alternative to test. Exact count, charge interval, regeneration duration, guard/charge overlap and heavy-action input remain tuning/control proposals.

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

Keep tap-LMB close offense and movement usable when few pieces remain. A core/grip strike at zero pieces is a proposal to prevent helpless waiting; it must not provide free replacement shielding. Q should consistently recall surviving deployed pieces. A separate heavy-action button, provisionally F, would avoid today's Q ambiguity when some pieces are held and others deployed; do not silently finalize that input. Guard/charge cancellation needs a short input experiment, preserving useful partial protection rather than inheriting the whole-disc global lockout.

Elite destruction must be readable and answerable. Start with a frontal interception stance that destroys one incoming piece and then has a recovery. Players can flank, delay, fight close or deliberately sacrifice a piece to use that opening. Avoid passive unexplained destruction on every hit.

## Example allocation

With six pieces, light and launch three. Block one bolt: two remain attached, three are deployed, one is regenerating. An elite destroys one projectile: two attached, two deployed, two regenerating. Reposition and recall the survivors through another enemy: four pieces reattach while the two independent timers continue. The player can defend, fight close or select a new partial throw.

## Upgrades worth trying

- **Frost return:** outbound pieces chill; a returning piece shatters chilled enemies into a visible burst. The return route matters.
- **Storm anchors:** two lodged pieces create a damaging line between them. Leaving them deployed maintains the trap and costs held protection; destruction breaks that link.
- **Mirror counter:** a precise block stores the attack's energy for a clearly illuminated outgoing piece or close strike. The spending trigger and stored amount must be displayed.
- **Reforge bash:** landing a committed close bash advances one regeneration timer, once per attack. An exposed player earns faster recovery by taking a readable risk.

These are proposed behavior combinations, not installed content or promises of tested fun. Piece launch, outward hit, return hit, block depletion, destruction, reconstruction and catch provide distinct upgrade triggers. Keep causal chains bounded without silently flattening powerful earned combinations.

## Reward usability is part of the mechanic

Klaus could not tell how to use earned powers and did not see special effects activate. Every reward needs an explicit input or automatic trigger, the expected visible result, any cost/cooldown, and a retained explanation in the equipment screen. Rank upgrades must retain the base action instructions. Give an immediate optional practice target with the required setup: for example, two targets for a chain-lightning demonstration. Display relevant readiness, stored charges and piece state; use distinct animations/sounds/materials for different effects.

The current save/source review found Ram II and Frost I active, but does not establish visible activation in Klaus's play. Ram requires Q while the whole shield is held; Frost works through contacts. Current problems include disappearing instructions, silent cooldown rejection, Frost cues overwritten by generic hit flashes, and Storm arcs being hidden after the source enemy dies. These need actual runtime correction/inspection in the next playable build, not merely clearer documents.

## Next observable proof

Implement and inspect partial launch, continued protection, block depletion, recall docking, airborne destruction and independent reconstruction together. Then test one conspicuous upgrade combination against a mixed encounter and a clearly telegraphed counter enemy. Repeat in different generated layouts using the same modular art kit. Count/timings, challenge and fun remain untested; the existing 14 whole-disc checks do not certify this design.
