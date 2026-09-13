# Magnet Sweep — prepare in 2.5D, resolve in 3D

13 September 2026. Owner-selected foundation for refinement; presentation proposal, not an implemented combat system.

## What Klaus chose

In the continuation task on 13 September, Klaus supplied the same board and explicitly confirmed **16B / 2.5D side view** and **16C / 3D perspective** as the two mechanic layouts, using this graphics treatment throughout. Following his preceding request, these map to preparation/assembly and action respectively.

Klaus likes **both the visual treatment and the perspective in 16C** of [the camera comparison](../assets/concepts/art-direction-2026-09-13/16-depth-camera.webp). He proposes preparing ammunition in a separate 2.5D side-view screen, then switching to 3D for the shot, impact and potentially the enemy turn. The preparation view shows enemies, available scrap and a component bar used to build the shot. Damage and targeting mechanics remain open.

## The visual foundation

The later design Q&A selects a **mercenary defending fortified cities against robots after an AI takeover**, with several character-specific weapons. See [the chosen campaign](REDESIGN-PLAN.md#chosen-world-and-city-campaign). This supplies the current world foundation; the exact pictured operator, cannon and environment are still illustrative assets rather than approved character/city specifications.

Develop the reference's dimensional, stylised machinery: substantial bevelled metal, restrained wear, warm furnace light, cooler background depth and recognisable gears, bolts and magnetic poles. Keep the tactile volume and dramatic camera of 16C while giving selectable scrap clear edges in the preparation view. Preserve visual continuity between views. The pictured operator, clothing and sunset scrapyard are working references, not a selected protagonist or final story. The rack of finished bullets in the reference is not the ammunition-building mechanic.

## Proposed player flow

**Updated mechanic foundation from the design Q&A:** Choose Character establishes an individual weapon and starting core recipes. One magnet use per turn gathers source materials; the player allocates them to affordable, available recipes in limited memory, producing ammo/defence/modifier parts. Cooldown recipes become usable again at zero counters, including within the same turn; crafting activates their cooldown again. Recipes without a cooldown ability allow one craft per turn unless a permanent upgrade explicitly grants more. Other recipes may remove cooldown counters. **Unused resources and parts remain between turns, then both clear at fight end.** Retained recipes and permanent upgrades carry through the game, with upgrades able to expand memory or grant fresh starting or turn-timed supplies. Victories provide cores for sale and normally one of three recipe rewards or a skip; recipes can be exchanged at full memory without exceeding its capacity. Officers also award a permanent upgrade. See [the authoritative recipe/part rules](REDESIGN-PLAN.md#owner-defined-recipe-and-part-system). The generic operator/cannon and three illustrated parts are not the final character roster or recipes; the board is concept art, not actual game screens.

| Moment | What the player sees and does | Information that must remain clear |
| --- | --- | --- |
| Prepare — 2.5D side view | Read enemy intentions, gather from a massive scrap pile with the magnet, open recipe cards through Recipe Memory and craft available recipes, then apply crafted/retained parts to the single upcoming shot and defence | Support at least 20 applied parts each in bullet and shield with legible combined effects; raw-resource reserves versus crafted-part reserves; recipe-memory usage/capacity; recipe costs and crafting output; remaining cooldown counters for cooldown recipes, or uses remaining this turn for recipes without cooldown; immediate readiness after the last counter is removed; available versus committed parts; energy; targets; expected effects before Fire |
| Fire — 3D perspective | The camera takes a view behind and beside the rig, preserving the selected target. Components seat and activate in sequence, then the cannon fires | The shot is already committed. Animation reveals the physical cause of the previewed result rather than introducing a new aiming challenge |
| Impact — 3D perspective | See the projectile reach its target and each affected enemy react | Who was hit, which effects applied and what protection or health changed. A spread attack needs framing that shows every affected target |
| Enemy turn — 3D action view | Watch enemy attacks and their effects on the rig and its protection | Which enemy is acting, what hit the rig and the remaining player health; use an angle that actually shows the rig being struck |
| Next decision — 2.5D side view | Return to the same preparation layout with current stock, surviving enemies and updated intentions | Resources and target identities persist across the transition; the camera cut must not imply a refill or reset |

**Fire is not automatically End Turn.** If the eventual rules allow several actions, return to preparation after a shot when another player decision is available. Enter the enemy sequence only when the player's turn ends under the chosen rules. If the design later chooses one shot per turn, the sequence can run directly from impact into the enemy response. This document deliberately does not select between those models.

**Later owner-defined prepare-screen requirements:** support at least **20 applied parts per bullet and 20 per shield**, with potentially different layered abilities, strengths and enhancements each round. A hard part cap versus resource-only balancing remains undecided. Either present the resulting strengths/abilities clearly or show physical added parts with an effects summary; the choice remains open and must suit the selected graphics quality. The earlier visible loading-channel proposal and three-part diagram are illustrative, not mandatory physical attachments or a slot limit.

Provide a **Recipe Memory button opening recipe cards** for selection and crafting according to available resources and established availability rules. Include the magnet and a massive scrap pile. Magnet upgrades improve haul yield and/or bias one or more resource types; acquire permanent magnet upgrades in the shop separately from weapon rewards, or craft special enhancements for the **next round only**. See [the prepare-screen requirements](REDESIGN-PLAN.md#main-prepare-screen--recorded-requirements). These are recorded requirements, not a request to build a layout or implement the screen.

## Keeping the change of view useful

- Use the same 3D objects and encounter state with two authored camera layouts where practical. Separate presentation screens do not require two unrelated asset sets.
- Keep the target selection and outcome preview in preparation. Do not add real-time aiming or unexpected accuracy rolls merely because the action camera is 3D.
- Keep health, turn ownership and essential consequences readable in the action view; retract the assembly controls while an action resolves.
- Show enemy impact from an angle where the rig and mounted protection are visible. An exact copy of the outbound shot camera may hide them.
- Return promptly to decisions. Offer a faster or reduced-motion presentation; avoid forcing a lengthy camera performance for every small shot.

## Still to decide and prove

Damage rules, single-target versus spread behaviour, effect order, defence timing, energy/hauling costs, the relationship between Fire and End Turn, final fiction and rewards remain separate design decisions. An illustrative explosion or blocked hit in concept art does not establish those rules.

The required first evidence is a storyboard followed by a reproducibility test using shared geometry and two camera views, recorded below. Its criteria are magnet movement, stacked-material clarity, loading-channel continuity, target visibility and return orientation. Generated stills alone do not prove camera continuity or animation quality.

## Delivered storyboard and first motion test

The [four-panel storyboard](../assets/concepts/combat-camera-2026-09-13/storyboard.webp) was generated and inspected in the continuation task on 13 September. The [original prompt](../assets/concepts/combat-camera-2026-09-13/PROMPT.md), [targeted loading refinement](../assets/concepts/combat-camera-2026-09-13/REFINEMENT-PROMPT.md) and [provenance/observations](../assets/concepts/combat-camera-2026-09-13/PROVENANCE.md) preserve the result. Its Fire panel catches the joined components entering an open chamber; impact and the later enemy response show contact direction clearly. Generated stills have some housing/damage-state drift, which is why the shared-geometry motion test remains necessary. The hypothetical single-target and plate hits illustrate framing only; they do not settle damage, defence or turn timing.

The [review page](../assets/concepts/combat-camera-2026-09-13/index.html) switches between the loading refinement and the preserved muzzle-discharge companion, and includes the separate motion study. This task's local preview uses [port 8876](http://127.0.0.1:8876/assets/concepts/combat-camera-2026-09-13/index.html), serving this worktree's `games/magnet-sweep`; port 8874 belongs to the older atlas preview. Restart from the intended checkout with `python -m http.server 8876 --bind 127.0.0.1 --directory games/magnet-sweep`. Neither the page nor its controls changes game state.

The [eight-second Blender study](../art-tests/camera-motion/camera-motion.mp4) now demonstrates one shared scene across two authored cameras: gather three example parts, join/load/fire them, react at the target, reframe for a visible plate hit, then return to the same preparation camera and remaining stock. Its [source, render evidence and limitations](../art-tests/camera-motion/README.md) are preserved. Revised framing separates the enemies and exposes the plate face. Blender rendered all 192 frames, the H.264 stream decoded successfully, and browser playback reached the end. No gameplay files or saves changed.

**Assessment:** shared geometry is a workable foundation for camera continuity at this simple level. It has not yet reproduced 16B/16C's visual finish: operator, substantial machinery detail, art-directed wear, depth, lighting and effects remain below the target. Preparation parts also need more screen space. Root recommends the next bounded art increment focus on one reference-quality forge rig, loading channel and representative scrap set in these same two cameras, then re-evaluate the motion. This recommendation is not an owner-approved recipe, final camera move or permission to implement combat. Chain physics, game feel, engine integration and runtime performance remain untested.
