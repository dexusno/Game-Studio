# Magnet Sweep — prepare in 2.5D, resolve in 3D

13 September 2026. Owner-selected foundation for refinement; presentation proposal, not an implemented combat system.

## What Klaus chose

Klaus likes **both the visual treatment and the perspective in 16C** of [the camera comparison](../assets/concepts/art-direction-2026-09-13/16-depth-camera.webp). He proposes preparing ammunition in a separate 2.5D side-view screen, then switching to 3D for the shot, impact and potentially the enemy turn. The preparation view shows enemies, available scrap and a component bar used to build the shot. Damage and targeting mechanics remain open.

## The visual foundation

Develop the reference's dimensional, stylised machinery: substantial bevelled metal, restrained wear, warm furnace light, cooler background depth and recognisable gears, bolts and magnetic poles. Keep the tactile volume and dramatic camera of 16C while giving selectable scrap clear edges in the preparation view. Preserve visual continuity between views. The pictured operator, clothing and sunset scrapyard are working references, not a selected protagonist or final story. The rack of finished bullets in the reference is not the ammunition-building mechanic.

## Proposed player flow

| Moment | What the player sees and does | Information that must remain clear |
| --- | --- | --- |
| Prepare — 2.5D side view | Read enemy intentions, move the chain-hung magnet, gather components and arrange them in the shot bar | Available versus already committed parts; remaining energy; intended targets; expected effects and cost before Fire |
| Fire — 3D perspective | The camera takes a view behind and beside the rig, preserving the selected target. Components seat and activate in sequence, then the cannon fires | The shot is already committed. Animation reveals the physical cause of the previewed result rather than introducing a new aiming challenge |
| Impact — 3D perspective | See the projectile reach its target and each affected enemy react | Who was hit, which effects applied and what protection or health changed. A spread attack needs framing that shows every affected target |
| Enemy turn — 3D action view | Watch enemy attacks and their effects on the rig and its protection | Which enemy is acting, what hit the rig and the remaining player health; use an angle that actually shows the rig being struck |
| Next decision — 2.5D side view | Return to the same preparation layout with current stock, surviving enemies and updated intentions | Resources and target identities persist across the transition; the camera cut must not imply a refill or reset |

**Fire is not automatically End Turn.** If the eventual rules allow several actions, return to preparation after a shot when another player decision is available. Enter the enemy sequence only when the player's turn ends under the chosen rules. If the design later chooses one shot per turn, the sequence can run directly from impact into the enemy response. This document deliberately does not select between those models.

The component bar should contain recognisable pieces in a visible loading channel. Its capacity, ordering restrictions and component effects are undecided. A basic diagram can show body, payload and tip without turning that example into a mandatory recipe or a fixed slot count.

## Keeping the change of view useful

- Use the same 3D objects and encounter state with two authored camera layouts where practical. Separate presentation screens do not require two unrelated asset sets.
- Keep the target selection and outcome preview in preparation. Do not add real-time aiming or unexpected accuracy rolls merely because the action camera is 3D.
- Keep health, turn ownership and essential consequences readable in the action view; retract the assembly controls while an action resolves.
- Show enemy impact from an angle where the rig and mounted protection are visible. An exact copy of the outbound shot camera may hide them.
- Return promptly to decisions. Offer a faster or reduced-motion presentation; avoid forcing a lengthy camera performance for every small shot.

## Still to decide and prove

Damage rules, single-target versus spread behaviour, effect order, defence timing, energy/hauling costs, the relationship between Fire and End Turn, final fiction and rewards remain separate design decisions. An illustrative explosion or blocked hit in concept art does not establish those rules.

Next art evidence: a short storyboard showing the same rig and enemies across these moments, followed by a reproducibility test using shared geometry and two camera views. Inspect magnet movement, stacked material clarity, loading-channel continuity, target visibility and return orientation. Generated stills alone do not prove the camera transition, animation quality or an easy production method.

The [exact four-panel storyboard prompt](../assets/concepts/combat-camera-2026-09-13/PROMPT.md) is saved. Its initial ImageGen attempt was interrupted; the handle is now missing and no new image was recovered. The receiving task should generate from this prompt with panel 16C as the visual reference, then inspect and preserve the output. The requested hypothetical single-target hit and plate impact illustrate framing only; they do not settle damage or defence rules.
