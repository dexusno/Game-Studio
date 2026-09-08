# Folding shield combat playtest

The weapon rests folded along your arm. **Hold RMB** to unfold the surviving pieces into a shield; release RMB to fold it again. **Tap LMB repeatedly** for close strikes, with a stronger third strike when chained. **Hold LMB for 1.8 seconds** to light all six pieces and reach full power, then release. Releasing earlier throws only the lit pieces and keeps the rest available to block. Full power requires all six pieces, and committing them leaves you without armor until recall or reconstruction. **Q** recalls survivors; **F** performs a heavy strike (or Ram when earned).


Open `BuildOutput/CombatFeel/Windows/Dreambound.exe`. This is the folding six-piece combat experiment (0.3.1-combat1). The older whole-disc study remains in `BuildOutput/ShieldStudy`.

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
| Dash / jump | Shift / Space |
| Ward stone / reward choice | E / 1, 2, 3 or click |
| Cycle owned elemental cores and kinetic | R |
| Read current ability instructions / pause | Tab / Escape |

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

Learned patterns survive defeat. Select one as your starting attachment on later expeditions. Same-seed retry repeats the generated route; a new seed changes route and cover. The `DreamboundSegments` save profile is separate from previous prototypes. A resumed fight restarts from its entry checkpoint; a resumed claimed ward restores its reward practice. Losing focus pauses. Mouse sensitivity is adjustable in pause.

This remains a mechanics playtest with one biome and a small modular kit. The environment has not reached art anchor B, and this is not the full game's content or duration. For this playtest, try the folded strikes, repeated melee taps, a partial throw and a fully charged release. Judge whether each action feels distinct and forceful while keeping enemies visible.
