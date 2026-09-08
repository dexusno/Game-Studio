# Physical shield — courtyard playtest

Start `Dreambound.exe` from `BuildOutput/ShieldStudy/Windows` and choose **Enter courtyard**. Approach the glowing ward stone and press **E** to begin. There are three encounters: a close opponent, a mixed group, then a heavy sentinel and caster. After each of the first two encounters, return to the stone and choose an earned attachment. Press E again when ready to test it.

| Action | Control |
| --- | --- |
| Move / aim | WASD / mouse |
| Close rim strike | Tap left mouse |
| Charge and throw the shield | Hold left mouse, then release |
| Guard / timed guard | Hold right mouse while the shield is held |
| Recall the deployed shield | Q or left mouse |
| Heavy bash / installed Ram rush | Q while the shield is held |
| Dash / jump | Shift / Space |
| Interact / choose reward | E / 1, 2, 3 |
| Cycle installed elemental cores | R |
| Inspect equipment / pause | Tab / Escape |

Committing to a strike or throw gives up guarding. While the shield is away, reposition to cut a different return line, or recall early to recover protection. You can also keep it in hand and use strikes, guards and counters. Close fighting does not require throwing.

The first earned choice offers Mirror, Ram or Anchor. The second adds Frost, Ember or Storm. These are the current build's triggers:

| Reward | How to use it |
| --- | --- |
| Ram | Press Q with the shield in hand, after the previous attack finishes. It bashes and rushes forward. Q while the shield is away recalls it instead. Rank II restores guard on shield hits and reduces the rush's reuse cooldown to 1.3 seconds; full guard hides the gain. |
| Frost | Automatic on shield hits. The first hit chills for 4 seconds; another direct hit while chilled deals extra shatter damage and reapplies chill. There is no separate spell button. |
| Mirror I | Face a blockable attack and raise RMB within 0.20 seconds before it lands. Store one charge, up to two. Your next direct shield hit spends one, adding 180% of the blocked damage; misses keep charges. Holding guard early gives no rank-I charge. |
| Anchor | Throw so the shield stops between you and a shooter. Bolts must cross its front face to be blocked. Q removes that cover. It does not stop melee, heavy attacks or shots that miss the shield. |
| Ember | Automatic on shield hits. A surviving target burns for 4 seconds while you can reposition. |
| Storm | Hit a target, then hit it again within 5 seconds while other enemies are within 6.2m and visible. Lightning can hit two other targets. A lone enemy produces no arc. |

R switches between owned elemental cores; with only one core it has nothing to switch to. The current build has weak/missing effect and readiness cues, so this table explains the implemented rules without claiming they are clearly visible. Updated reward wording is pending the next package. The [segmented shield](SHIELD-DESIGN.md) is the newly selected direction and is not implemented in this executable.

Learned attachment patterns survive defeat. Choose a starting pattern for a new attempt or repeat a seed for practice. Checkpoints preserve earned equipment; resuming restarts the current encounter. This study has a separate save profile from the rejected beta2. Sensitivity is adjustable in pause; losing focus pauses the game.

This courtyard tests the corrected combat and presentation. Its layout is authored and stable; encounter offsets and offers use the run seed. It is not the planned campaign or proof of procedural world variety. The useful playtest questions are whether held combat and throwing create different useful choices, whether hits and catches feel substantial, and whether the earned attachment makes you want to experiment.
