# Beta2 owner playtest — 2026-09-08

## Segmented experiment feedback and folding correction — 2026-09-08

Klaus found 0.3.0-segments1's basic allocation concept acceptable but its experience still lacking: obstructive idle shield, oversized/repetitive whooshes, no convincing rising charge, weak full attacks and dull very-short-range melee. He asked for stronger presentation and suggested a secondary weapon if shield combat could not become satisfying. He then explicitly selected a collapsed default/weapon form that animates into full defense on RMB.

0.3.1-combat1 tests that form change, a 1.8-second six-piece commitment with a distinct single-blast payoff, wider/longer aimed melee with a short wall-safe step and third-strike finisher, and separate mechanically voiced actions with a real rising charge loop. New original environment pieces improve enclosure and planting depth. These address specific failures but do not establish enjoyment. Current enemy art and the environment's surface detail still fall substantially below anchor B. Klaus subsequently rejected the kit-first recommendation. The next art proof is a guided AI/source-modeling and Blender workflow that can create distinct biome assets, assessed as real geometry in Unreal.

Technical fixtures and recorded mix levels can establish transitions, damage, collision and signal behavior. They cannot establish satisfying sound or combat; do not promote a green suite into another positive playtest claim. Assess whether partial versus full commitment changes decisions, whether the folded form keeps targets readable, and whether the new melee offers a useful alternative before expanding content.

## Shield-study visual comparison — 2026-09-08

Klaus describes the corrective study's graphics as slightly better and asks for an honest comparison with the selected anchor. Direct visual inspection of [anchor B, middle column](../../research/runs/2026-09-07-depth-reset/assets/art-style-comparison-v1.png) and the [actual shield-study frame](evidence/shield-study-courtyard.png) confirms that the gap remains substantial.

- B encloses the view with arches, stairs, upper architecture and a monumental rooted bell. The study exposes a broad, flat rectangular court with repeated low arches and large empty sky.
- B's roots weave into masonry and have varied, layered silhouettes. The study's tree and bell remain simple central forms with sparse repeated foliage.
- B distinguishes worn ceramic, worked metal and weathered stone through designed edges, surface detail and color variation. The study's foreground shield and walls appear smooth, uniform and insufficiently detailed at gameplay distance.
- B uses shaded framing, warm light, cooler shadows and distance to separate foreground, landmark and background. The study's broad exposure and sparse background flatten the scene.
- B's first-person equipment has overlapping armor, visible fasteners, joints, grip support and construction detail. The current shield reads as a decorative wheel; the central core and radial spokes do not establish that mechanical identity.

Integration assessment: the current scripted kit has not demonstrated the chosen style. The next art work needs a representative authored playable view with substantially better hero geometry, architecture, material definition and lighting. Compare it directly with B in Unreal before multiplying assets or rooms. This review inspected the two existing images; no new build, gameplay test, asset creation or claim of improved fidelity occurred. Enemy detail and animation were not assessed from the empty courtyard still.

## Historical beta2 review

**Outcome: rejected for experience and presentation.** Klaus tested the delivered 0.1.0-beta2. His feedback establishes a failed owner playtest, not a measured market result or proof that the larger concept cannot work. No corrective implementation was made during this review.

## Expected and observed

| Expected | What happened |
| --- | --- |
| One recognizable shield used creatively as a weapon | Default repeating shots, guarding while shooting and beam counterfire. No physical throw or recall. Klaus experienced a generic gun with a defense button. |
| Skill and meaningful attack/defense decisions | Klaus found play confusing and simple. Movement was usable but generic. Adding attachment ranks did not establish the missing central interaction. |
| Weight and power | Klaus reported no felt recoil and a weak laser sound. A source-level pose/effect trigger is not proof of a convincing attack, hit or catch. |
| Sculpted painterly art B | The [actual capture](evidence/beta-0.1.0-first-view.png) has uniform tiles, repeated columns, stretched roots and a plain segmented enemy. [Anchor B, middle column](../../research/runs/2026-09-07-depth-reset/assets/art-style-comparison-v1.png) has composed enclosing architecture, a monumental bell/tree, organic growth, layered materials, articulated equipment and deliberate detail. |
| A world with stakes | The dream-world premise has no established enemy faction, motive or player-facing explanation for these fights. Archived institution/recovery-officer ideas are proposals, not accepted canon. Klaus requested this story work later. |

The 31 staged technical checks cover selected combat/save/collision invariants. They did not test whether the signature interaction existed, whether the sound had impact, or whether the scene met B. The prototype was offered to Klaus before demonstrating those central qualities. That was an integration and quality-judgment failure, not missing permission, a lack of installed tools or insufficient owner guidance.

The implementation also misread the rejection of Sixfold's forced shoot-all/recall cadence as a reason to remove physical shield offense. The latest owner clarification restores the shield itself as the hero while retaining free choice. The generated kit favored asset coverage, repeated profiles and shared surface noise over deliberate modeling, composition and animation.

## Working correction to test

- **Held:** the shield face guards/parries; its rim delivers close strikes and counters. Committing to a strike opens a readable vulnerability. Close fighting remains a useful strategy.
- **Deployed:** deliberately launch the actual shield. Broad protection leaves with it; movement and cover matter. The player can choose a short direct hit or a longer spatial setup. No free substitute gun/guard should erase that commitment.
- **Returning:** recall toward the player's current position so repositioning shapes a damaging return path. Early recall restores defense; delay can line up a stronger return. Recovery must remain reliable around obstacles without attacking freely through walls.
- **Caught:** a visible, forceful catch returns protection; a timing bonus may support immediate counterplay, with ordinary catches remaining dependable. Exact inputs/timings and whole-shield versus segmented implementations need focused testing.

Two possible earned rewards are an **anchor** that turns a deliberately placed shield into temporary directional cover, and a **stored-force rim** that spends successful timed-defense energy through a strike, throw or recall impact. They demonstrate spatial and counterattack builds. Names, implementation and numbers are proposals; broad future tech/magic/elemental combinations and earned overpowering payoffs remain intended.

## Next production proof

Build one representative Bellroot courtyard with an enclosing arch, bell/tree landmark, useful elevation and coherent paths. Author the hero shield and one enemy family with contrasting close/ranged roles: purposeful silhouette, topology/UVs, sculpted/baked detail, painted masks and animation. Use a small supporting masonry kit with designed damage/proportion variants. Secondary foliage, rubble, textures or animation foundations may be sourced after rights and style checks; no purchase is authorized. More random prop placement or another material-noise pass cannot supply the missing composition.

Show uninterrupted actual packaged gameplay with audible sound: entry, close fighting, heavy blocked hit, exposed launch, repositioned damaging recall, catch, enemy stagger/defeat, earned upgrade and a changed tactic. Inspect release/impact/catch weight, enemy tells, motion readability, input response and frame times. Match the anchor from the actual gameplay camera and multiple positions; a new concept image or selected still does not establish this result.

Use contextual action prompts and an encounter that makes the protection/attack choice understandable. Keep attacks viable before loot. The small proof is a production correction, not a new small-game mandate, campaign-duration decision or return to the rejected arcade concept. Do not expand procedural room count or the catalogue until this actual interaction and presentation justify it.

## Open story work — explicitly later

Define the opposition's identities and motives, the cyborg's immediate reason to fight, relationships between local inhabitants and any cross-world adversary, and the role of bosses and recurring expeditions. Explain conflict through visible behavior and objectives. Do not declare every world's inhabitants hostile by default or silently canonize a placeholder enemy. No antagonist proposal was selected during this review.

## Limits and resources

This review used Klaus's direct report, the saved images, relevant source and focused combat/art specialist analysis. It did not run another build or repeat the technical suite. No new assets, downloads, purchases, publication or software installation occurred. The prior adjustable two-agent-day estimate describes the initial checkpoint, not a proven cost or deadline for achieving B. Actual total effort and play duration are unmeasured. Our ability to achieve the anchor's polish remains unproven; demonstrate the next actual slice rather than promise it from the tools alone.
