# Animation performance target

Owner goal, September 11, 2026: “make our animations on par with premium indie games like windrose, monster fantasy and the like.” The goal remains active. Passing the earlier v11 contact checks did not establish that quality.

## Reference observations

Inspected September 11, 2026. These are short official promotional excerpts, with different anatomy and cameras. Observations come from ordered decoded frames; continuous playback and the complete reference games were not assessed. Reference footage is private review material, not a game asset, and is not redistributed here.

- [Windrose, official Steam page](https://store.steampowered.com/app/3041230/Windrose/), the ten-second “Swashbuckling Adventure” loop. The 3.0–3.83s combat excerpt coordinates a weapon stroke with shoulder and torso movement. The later melee excerpts use distinct open, committed and recovering silhouettes. Application: the weapon and striking limb should participate in a coordinated body action, with movement continuing through contact.
- [Monster Fantasy, official Jotoyo Steam page](https://store.steampowered.com/app/4713940/Monster_Fantasy/), the 3.7-second hunting loop. At roughly 0.9–3.5s the creature moves through compressed, raised, asymmetric, turned and low silhouettes. The whole body carries the action. Its 4.9-second class-combat loop also shows clear changes in posture and level. Application: give our creatures distinct bodily preparation, commitment and recovery, readable from the gameplay camera.

The reviewed page assets were `3041230/extras/b65482d2ffe4b51a515cc922dbe42542.webm`, `4713940/extras/992ca6950095208f5a2bb96fa9dd0bcf.webm` and `4713940/extras/df23e18e19ba2f78501915d07bb9fe6b.webm`, served by `shared.fastly.steamstatic.com/store_item_assets/steam/apps/`. Decoded source rates were 60, 20 and 20 fps respectively. These are not measurements of either game's runtime performance. Their published motion is a craft reference, not a specification to duplicate characters, choreography or assets.

## Visible baseline gaps and intended performance

| Action | V11 gap observed | Required visible improvement |
|---|---|---|
| Approach and turn | Nearly fixed upright head/chest attitude over alternating feet; pendulum arms. | A purposeful moving stance, hips transferring support, chest counter-motion and an attentive head; visible preparation to stop and change direction. |
| Melee strike | Arm opening dominates anticipation; little bodily coil. | Load an asymmetric stance, lead the strike through hips/chest/shoulder, continue through contact and recover balance. Keep the claw outside the head silhouette when viewed by the player. |
| Caster | Nearly the same upright offering pose through gathering and both releases. | Change the whole silhouette during gathering; perform offset hand gestures and a distinct release impulse, then withdraw into a visibly different recovery. Keep the real projectile origin and visible gesture coherent. |
| Hunter | Upright hop with limbs hanging beneath the trunk. | Compress before launch, extend forward with purposeful reaching limbs, brace and absorb momentum on landing. Distinguish it immediately from Melee. |
| Guardian Slam | Floor contact exists, but loading and recovery contribute little sense of mass. | A visible load through the legs and back, a committed whole-body downstroke, grounded contact and effort in rising. |
| Hit and collapse | Small recoil and a controlled lowering with little overlapping response. | Directional response followed by balance recovery; loss of support on death, staggered limb/head response and a contact/settling beat. |
| First-person weapon | Stroke eased to a stop at contact; limited carried mass and evasion posture. | Carry stroke velocity through its damage time, shape follow-through and recovery, preserve input transitions, and brace visibly during evasion. Keep camera motion restrained and preserve mouse aim. |

## Implementation and review

The candidate adds lower-spine, chest, neck and clavicle articulation to the existing creature rigs. MireSeer's throat sac receives its own weighted bone. Existing global rest transforms, creature geometry, UVs and original maps are preserved. New skeletal assets use the `_Performance` suffix so the earlier skeletons remain available. Bone count alone is not a quality criterion.

Authored pose curves control the larger performance; contact solving adapts it to actual movement and terrain. Keep damage, phase timing, collision and player control authoritative. Review complete approach/turn/attack/hit/collapse sequences from side and player cameras, beginning with Melee, before accepting the approach for the remaining creatures. A good still pose cannot pass a broken transition; numerical contact checks cannot pass an unconvincing performance.

The existing v11 build/reel remains the comparison baseline. Candidate builds, rejected findings and the final inspected coverage belong in the game's existing STATUS and QA records. Neither this target nor successful compilation establishes reference parity, owner acceptance or enjoyable play.
