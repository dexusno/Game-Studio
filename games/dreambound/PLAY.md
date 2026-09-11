# Creature movement playtest

Current private playtest: **0.5.4-fire1**. Run `scripts/Start-Demo.ps1` or open `BuildOutput/OrganicFireCandidate/Windows/Dreambound.exe`. It includes the revised creature faces, internally gathered and hand-thrown fire, and Klaus's supplied fireball sound. [Watch the actual packaged sound preview](../../.local/organic-fire-review/owner-fireball-in-game.mp4). The earlier accepted animation build below remains available.

The weapon rests folded along your arm. **Hold RMB** to unfold the surviving pieces into a shield; release RMB to fold it again. **Tap LMB repeatedly** for close strikes, with a stronger third strike when chained. **Hold LMB for 1.8 seconds** to light all six pieces and reach full power, then release. Releasing earlier throws only the lit pieces and keeps the rest available to block. Full power requires all six pieces, and committing them leaves you without armor until recall or reconstruction. **Q** recalls survivors; **F** performs a heavy strike (or Ram when earned).


Open `BuildOutput/CreatureAnatomy2/Windows/Dreambound.exe` for the retained **0.5.3-anatomy2** animation demo. Creatures turn their attention before their heavy bodies follow, gather into attacks and regain support after impact. Their jaws and claws articulate; the caster gathers and releases with its chest and arms, and the guardian raises its claws overhead before driving them into the floor. The Hunter throws itself forward, absorbs its landing and steps back while facing you, making its next close attack readable. Death begins with failed support and continues through the falling body and limbs. Your preferred cyborg arms, folding weapon and controls are retained. [Watch the 34-second motion review](evidence/creature-anatomy-review.mp4); it is silent and uses consecutive frames from the identified packaged builds. Keep the whole Windows directory together.

Approach the glowing ward stone and press **E**. Clear the first court, return to its stone and choose an attachment. Try it on the safe practice targets, then follow the open passage to the next court and press E at its stone. The second court earns an elemental core; the third contains the heavy sentinel and a caster. Routes and cover arrangements vary with the seed.

| Action | Control |
| --- | --- |
| Move / aim | WASD / mouse |
| Close rim or core strike | Tap LMB |
| Select pieces, then launch exactly those lit | Hold LMB, release |
| Guard with remaining attached pieces | Hold RMB |
| Cancel a selection and guard | RMB during LMB hold |
| Recall surviving deployed pieces | Q |
| Heavy bash / installed Ram rush | F |
| Sprint | Hold Left Shift |
| Evasive dash | Left Alt |
| Jump | Space |
| Ward stone / reward choice | E / 1, 2, 3 or click |
| Cycle owned elemental cores and kinetic | R |
| Read current ability instructions / pause | Tab / Escape |

Hold **Shift** to sprint. The stamina bar lasts about five seconds of actual running and recovers after a short rest; pressing into a wall does not drain it. After exhaustion, release Shift and wait for at least 20% before pressing again. **Left Alt** makes a short, forceful dash in your movement direction (forward when stationary), covering about 4.4 metres on clear level ground. It has a separate 1.45-second cooldown and no stamina cost. Ground-speed-driven weapon/body motion, alternating footfalls and landing feedback communicate movement weight.

Six pieces are the starting tuning. Selection begins after 0.22 seconds and lights another every 0.316 seconds. Charging lowers your guard; after the throw's brief recovery, the pieces you kept can protect you. Each ordinary frontal block spends one piece. Its own three-second rebuilding timer starts immediately, independently of other losses. The HUD shows each piece as held, lit, away, returning or rebuilding. You can still move and strike with the core at zero pieces.

Q recalls every surviving piece through space toward you. Reposition to make the return cross an enemy. A destroyed piece reconstructs instead of returning. The heavy sentinel can warn and raise a frontal interception: it destroys one incoming piece, then recovers. Delay, flank, close in or sacrifice one piece to consume that counter.

| Earned ability | Try this |
| --- | --- |
| Ram | Approach a target and press **F**. Watch the readiness/cooldown display. Rank II also advances one rebuilding piece by 0.85 seconds on a landed bash/rush. |
| Mirror | Face an attack and raise **RMB within 0.20 seconds before impact**. A successful timed block spends a piece and stores one force charge. Your next physical hit spends it; misses preserve it. The HUD shows stored charges. |
| Anchor | Lodge pieces between you and a caster. Each visible frontal arc can stop bolts crossing it until its integrity is spent. Q removes that cover. It does not stop melee or heavy attacks. |
| Frost | Outbound pieces leave blue ice marks. **Q recall through a chilled target** shatters them. Close strikes also trigger shatter. |
| Ember | Hits automatically ignite a target. Orange flame and burn pulses continue for four seconds. |
| Storm | Hit a target **twice within five seconds**, with a visible neighbor within 6.2 meters. The repeat hit sends lightning into nearby enemies. A lone target cannot show a chain. |

Acquiring a core activates it. R also cycles through kinetic; the lower-left HUD identifies what is active. Tab retains instructions at the current rank. Safe practice provides two targets for elemental effects and a caster when Mirror/Anchor needs incoming bolts. Practice restores health and cannot award encounter kills. Leave through the passage when ready.

Learned patterns survive defeat. Select one as your starting attachment on later expeditions. Same-seed retry repeats the generated route; a new seed changes route and cover. The existing `DreamboundSegments` save profile is retained. A resumed fight restarts from its entry checkpoint; a resumed claimed ward restores its reward practice. Losing focus pauses. Mouse sensitivity and sound volume are adjustable in pause; sound starts at 85%, with plus/minus and mute/enable buttons. Settings persist with the profile.

This remains a three-court playtest in one biome. Two new biological enemy forms replace the robotic enemies. Damp spring margins and dry ruin gardens now use reeds, lilies, ferns, flowers, fungi, broken wood, scrub, roots, urns, fallen masonry and small animated insects. These are habitat variations inside the current biome; insects are ambient dressing. The connected routes, grounded stairs, walls, trees, fountains and water remain. Two edited owner-supplied Suno sounds now provide the charge motor and full shield release; 24 other cues use retained CC0 foley, including new footsteps and landing. This build does not establish the full game's content, duration or final visual quality.
