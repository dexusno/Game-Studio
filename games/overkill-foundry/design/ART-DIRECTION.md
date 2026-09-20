# Overkill Foundry — visual direction atlas

**Independent game since 14 September 2026.** Owner-selected title **Overkill Foundry**; catalogue ID `overkill-foundry`. Design and art references were preserved from Magnet Sweep at `adf3a3c55e2c680f5f6a2b442d0ed93cd84c4883`; future gameplay is a fresh implementation. [Current handoff](../STATUS.md) governs. Earlier magnet imagery and names record the design history.

13 September 2026. **Owner prefers 16C as the foundation; refinement and a repeatable production method remain to be proved.** Generated concept boards compare possible identities. They are not screenshots of an implemented redesign. Characters, scenery, colour coding, health values and mechanisms are visual placeholders, not selected fiction or rules.

## Current owner preference

### Shooting composition reminder — 20 September 2026

Klaus explicitly reaffirms that the shooting scene must show the enemies in a way that makes sense, as in image 16. Inspect the settled 16C action view, with Mara/gun in the foreground and the opposing machines clearly ahead of the barrel, naturally separated in depth. The player must understand the firing direction and distinguish the target from its companions. Check multiple enemies, large/small bodies and the camera transition; an early side-on frame does not prove this composition. This is a presentation requirement, not a new distance, formation or targeting rule. Preserve the selected rear gathering area.

**Camera trigger, confirmed immediately afterward:** change from16B to16C on **Lock and load**, before Fire. Keep the loaded shooting view interactive for target selection, Fire and Unload. Follow the [camera flow](COMBAT-CAMERA-FLOW.md): a completed shot returns to preparation; End Turn alone starts enemy actions. Do not wait until the muzzle effect is already playing to start the shot camera transition.

### Required visual quality and enemy animation — 20 September 2026

**Owner requirement:** the MVP must deliver **visually stunning graphics and their actual implementation**. Merely serviceable or generically coherent visuals do not satisfy this bar. Judge the integrated game in motion: models, materials, lighting, composition, effects, animation, cameras and interface must work together. The owner approves the result through human play and visual review. Concept art, source renders and a technically successful import are intermediate evidence, not visual acceptance.

**Presentation:** use the controlled 2.5D format to concentrate detail and composition where the player sees them. Seamlessly combine 3D models where needed with suitable layered/background elements; a fully 3D game or explorable 3D world is not required. This clarification does not revoke the selected 16B preparation and 16C action viewpoints. Assets must preserve apparent scale, depth, lighting, shadows, contact and identity across those viewpoints, without exposing flat backdrops, mismatched renders or incomplete geometry during transitions. Every city retains its own backdrop and surroundings in the selected style.

**Enemy identity:** every enemy in a playable build, including enabled Officers, bosses and helper types, needs its own recognisable visual identity and animation treatment. Record silhouette, proportions, characteristic mechanisms, materials/accent colour, posture, motion style and effects. Rigs, components and production techniques may be shared, but their final use must deliver distinct robots and behaviour appropriate to their design. The complete roster inherits this requirement as it is implemented; it is not relaxed for the first playable's included enemies.

| Per-enemy coverage | Required outcome |
| --- | --- |
| Resting/ready presentation | A readable, living machine between actions, consistent with its weight and design; subtle loops or mechanical motion where appropriate. |
| Every authored action | **One enemy-specific animation or sequence per distinct action is sufficient.** Cover the actions the robot actually has: attacks, charge, recovery, deployment, buffs, repairs, escape and phase actions as applicable. An action with several hits uses a sequence with matching hit cues; numerical buffs to the same action do not demand a new animation. Multiple variants of every action are not required. |
| Nonlethal hits | Multiple clearly different reactions driven by attack strength **relative to the enemy's strength**. Initial production coverage is light, medium and heavy reactions, adapted to that enemy's mass, posture and mechanisms. A generic flash or universal shake is not the complete reaction set. |
| Lethal hit | A spectacular, readable fall/collapse or destruction sequence, followed by a fade/dissolve until the enemy has disappeared completely. Detached pieces and temporary death effects also finish and disappear. No persistent corpse, debris, residue or enemy-attached loot model remains. A kill receives its death response even when the final damage amount is small. |

The owner selected relative reaction strength and the death/fade outcome. The three reaction bands and the following numeric mapping are **initial implementation/tuning choices**, subject to inspection in motion rather than individually owner-approved final values.

#### Relative-hit response contract

Start with the resolved direct hit's effective damage relative to the enemy's combat maximum HP, rather than its remaining HP. Maximum HP is an initial proxy for enemy strength; tune visual resistance per archetype if bulk, protection or animation character warrants it. Compare the same absolute hit against a weak robot and a strong one during review. A robot near death must not suddenly treat every tiny nonlethal hit as its maximum reaction just because little HP remains.

Use light below 5% of maximum HP, medium from 5% to below 20%, and heavy from 20% upward as the initial bands. A 6-damage hit on a 7-HP Mite is heavy; the same 6 damage on a 180-HP boss is light. Use integer comparisons (`100 * damage` against `5 * max_hp` and `20 * max_hp`); this introduces no fractional gameplay damage. Death always overrides the nonlethal band.

Read the combat result, not the raw bullet tooltip: mitigated, absorbed and HP damage must remain distinguishable. For the first implementation, effective direct damage is the amount removed from enemy Shield plus HP after mitigation, excluding unused overkill; use that magnitude for intensity while directing the effect to the Shield or body actually struck. A fully negated hit has an appropriate Armor/deflection cue, not a false heavy wound. Status ticks receive their own readable feedback; a lethal tick still triggers the full death lifecycle. Multi-hit attacks report each resolved hit, and later hits cannot damage an already-dead target through its lingering visual actor.

Reaction animation is presentation. It does not add a stun, skip an enemy action, change its logical position/targetability, modify damage or delay a rule trigger unless a separate explicit game effect says so. Support additive/layered responses or short sequences so rapid repeated shots and spread hits remain readable without a growing animation queue. Cosmetic variation must not consume encounter, move, reward or supply RNG draws.

#### Death and cleanup contract

At logical death, commit the normal combat outcome and existing kill/loot eligibility exactly once, stop accepting attacks against that living target, and dispatch the enemy's death presentation using its last visible state. Follow existing death-trigger and summon timing. Split Chassis's one-Mite death-spawn is still a game-rule event; neither waiting for the falling model nor removing it may repeat, cancel or re-time that event.

Normal presentation must allow the spectacular fall and subsequent disappearance to read, including on the final kill before leaving the encounter. Tune lengths against repeated play; presentation speed options can shorten the sequence while preserving a recognisable death. The headless core needs no animation delay. After the fade, remove the corpse, detached parts, attached effects and selection/intent labels; pooled components must reset fully before reuse. Visual disappearance does not remove earned cores or recipe rewards, which follow the existing reward flow without requiring persistent world debris.

#### Asset workflow and additional software

The owner names **ImageGen 2.5, TRELLIS and Blender** as available resources for coders and graphics agents. Use them as appropriate within the existing Unreal direction: establish approved visual references, create suitable asset starting points, refine geometry/materials and animation, then integrate and inspect the result at gameplay scale. Generated imagery or geometry does not bypass the modelling, rigging, animation, cleanup and in-engine work needed to achieve the quality bar. Verify actual installed paths, versions and tool access before relying on a particular production step; the stated tool availability is not a newly executed end-to-end pipeline test.

Raise any additional software requirement with the owner, whether free or paid. For a purchase recommendation, first identify the concrete production gap, show why the existing tools are insufficient, and present the exact proposed product, current verified cost, one-time/subscription terms and relevant intended-use conditions. The owner will decide whether to buy it. This requirement does not authorize a purchase, subscription, or silent new paid dependency. No additional software need or purchase has been established by this planning update.

Keep source/editable assets, export settings and asset provenance connected to the runtime assets. Artist handoff includes the rig/action/reaction/death inventory and import requirements; engineer handoff includes event bindings, transition/blend behaviour, cleanup, and actual rendered evidence. The art and code owners jointly own the implemented appearance.

#### Visual acceptance evidence

First establish the quality bar in one representative playable fight using the actual rig, cameras, lighting, UI and effects. Show the same hit against weak/strong enemies, each nonlethal reaction band, all included actions, rapid repeated hits, a lethal direct hit and lethal status damage, death-triggered spawning, and a final kill followed by a completely clear scene. Review shared 2.5D/3D continuity at ordinary gameplay size and after view/resolution changes; spectacle must preserve intent and target readability. Check for clipping, sliding, weightless movement, jarring loops, stale effects, premature scene transitions and accumulating presentation delays.

Then require the action/reaction/death coverage for **every enabled enemy**, not only the demonstration robot. Asset-completeness checks and frame/performance measurements support the review but cannot declare the graphics stunning. Klaus's visual approval of the integrated playable result remains required. No assets, animation, engine integration, visual approval or new software verification are delivered by this specification alone.

### Released-game quality benchmark — F.I.S.T., selected 20 September 2026

Klaus selects **F.I.S.T.: Forged in Shadow Torch** as the visual quality benchmark: "F.I.S.T. looks good, i agree." Use its dimensional machinery, material finish, lighting and environmental depth to make our required level of polish concrete. The existing STYLE anchor and 16B preparation / 16C action views continue to define Overkill Foundry's identity and composition.

Primary sources, accessed 20 September 2026:

| Reference | Evidence and relevance |
| --- | --- |
| [TiGames introduction, 1 July 2020](https://blog.playstation.com/2020/07/01/action-platformer-f-i-s-t-forged-in-shadow-torch-coming-to-ps4/) | The developer describes 2D gameplay in a 3D-rendered world. This is a relevant precedent for concentrating visual detail within our controlled presentation. |
| [Epic's TiGames interview, 10 November 2021](https://www.unrealengine.com/en-US/developer-interviews/f-i-s-t-forged-in-shadow-torch-is-a-critically-praised-anthropomorphic-dieselpunk-metroidvania-game) | TiGames describes a reusable layered-material library. Inspected official images show substantial metal forms, surface wear, warm local lighting and layered city depth. These qualities inform our model/material/environment benchmark. |
| [Red Hook introduction, 18 April 2024](https://blog.playstation.com/?p=391073) and [official Darkest Dungeon II page](https://www.darkestdungeon.com/darkest-dungeon-2/about/) | Red Hook documents 3D character art with full animation and turn-based combat. Official page images show distinct silhouettes and poses. Darkest Dungeon II was suggested as a complementary action-staging reference; **the owner selected F.I.S.T. only**, so this companion remains optional and unselected. |

This pass inspected official stills and read developer descriptions; it did not perform a frame-by-frame animation review or play either game. Their exact hit-response/death logic is not asserted to match ours. The rendering approaches are documented; their suitability for our production is an art-direction judgment. No reference assets were downloaded or added to our asset pool.

Prove the selected quality bar in **one actual Overkill Foundry encounter** at ordinary gameplay size: preparation, shot, enemy response and kill. Compare material finish, machine forms, light, shadows and scene depth with F.I.S.T.; judge action readability, relative hit reactions and complete death/fade against our own requirements above. A still cannot establish animation quality. Build this representative fight early, then apply its approved quality bar to every enabled enemy and scene. Current concepts and the old camera study do not yet demonstrate this benchmark in a playable build.

### Owner-supplied style anchor — 14 September 2026

Klaus designates [art anchor.png](art%20anchor.png) as the game's **STYLE anchor**, explicitly **not gameplay images**. The bottom **16C / 3D Perspective** panel anchors how the attack perspective should look. Its stylised machinery, materials, lighting, depth and camera treatment inform production; it does not prescribe literal gameplay props, HUD values or their locations.

For planning/loading, Klaus now places the magnet and scrap gathering **behind the player/gun**, away from the space between gun and enemies, and requires visible enemy-intent indicators to inform the current round's attack/defence. Every city must have a **unique backdrop and surroundings**, consistently rendered in this shared style. These owner requirements supersede the pictured gathering arrangement; the old boards and motion study remain concept/art experiments.

**Preservation/provenance:** the owner-supplied PNG is preserved without modification at `design/art anchor.png`, 1536 x 1024 pixels, 2,736,842 bytes, SHA-256 `52615934a21dc3304202cbab793ee13af2c35e84d7c1c0bff7e622bd9b54387c`. Decoded RGB pixels match [the archived board 16](../assets/concepts/art-direction-2026-09-13/16-depth-camera.webp) exactly. Its [existing generation provenance](../assets/concepts/art-direction-2026-09-13/PROVENANCE.md) and board register therefore remain the source record: built-in ImageGen, generated 13 September 2026 under applicable platform terms; studio distribution licence unset. This is an owner-selected reference asset, not a verified runtime asset or finished game screen.

Klaus explicitly reconfirmed board 16 in the continuation task: **16B for preparation/assembly and 16C for action**, with the same graphics treatment. The [four-panel storyboard](../assets/concepts/combat-camera-2026-09-13/storyboard.webp) now refines those moments. [Inspection and provenance](../assets/concepts/combat-camera-2026-09-13/PROVENANCE.md) distinguish its visual strengths from unresolved asset continuity and rule details.

Klaus likes both the graphics treatment and 3D perspective in **16C** for the actual shot. He proposes a separate **2.5D side-view preparation screen** showing enemies, available scrap and a component bar to build the shot. Fire then switches to 3D for discharge and impact; explore the enemy response in 3D as well. See [the camera-flow proposal](COMBAT-CAMERA-FLOW.md). This replaces the earlier request for a broad shortlist. It does not settle damage, targeting, whether Fire ends the turn, final fiction or the exact UI.

## How to review

Open the [local browser preview](http://127.0.0.1:8874/assets/concepts/art-direction-2026-09-13/index.html) to switch boards, enlarge the original artwork and collect favourite codes. The [review page source](../assets/concepts/art-direction-2026-09-13/index.html) and its images are saved together. Its shortlist and notes stay in that browser when storage is available; copy the resulting text into our conversation to communicate a preference. Nothing in the page selects or approves an art direction automatically. The temporary preview can be restarted from the studio root with `python -m http.server 8874 --bind 127.0.0.1 --directory games/overkill-foundry`.

Each archived board has one visual family and five coded variants, A–E. The earlier request for a shortlist is superseded by Klaus's 16B/16C selection. Use this archive for comparison only; continue refining the selected foundation.

The comparison uses the same functional scene: hanging magnet, airborne scrap, a physical loading stack, forge-cannon, protective plate and two enemies. This makes the style differences easier to see. The wide E panel gives more environmental space, so judge object treatment rather than panel size. The board labels are descriptions of appearance; “8-bit” and “16-bit” do not certify historical palette/hardware restrictions. Generated frames occasionally invent decorative details or ambiguous connections; a selected direction must be refined into mechanically correct objects.

## Camera and artwork are separate choices

| Presentation | What it would mean here | Practical strength | Main question to test |
| --- | --- | --- | --- |
| 2D sprites | Flat characters/parts with layered drawn scenery | Direct silhouette control; strong illustrated identity | Can rotated and stacked scrap remain clear without producing many redrawn poses? |
| 2.5D side view | Action stays on a lateral plane; art can combine 3D objects and layered illustration | Visible chain swing, object volume and pile depth while retaining a readable side view | Do depth and shadows help rather than hide selectable pieces? |
| 3D perspective | A spatial camera views appropriate 3D elements within a controlled encounter scene | Flexible camera and dimensional impact shots | Do occlusion, depth picking and camera movement add anything useful to this turn-based loop? |

The owner's preferred combination is **16B preparation and 16C action**, using the same graphics treatment throughout. The [first shared-scene motion study](../art-tests/camera-motion/README.md) now demonstrates two authored cameras with consistent geometry and remaining stock. Its simple models and scripted motion do not yet reproduce the reference's finish or establish interactive game feel.

## Boards

**15 style families × five variants = 75 examples**, plus a separate three-panel depth/camera comparison. All 16 boards were generated and inspected. [Exact prompts and image records](art-direction-boards.json).
| Board | Variants A–E (camera board A–C) |
| --- | --- |
| [01 — Pixel Art](../assets/concepts/art-direction-2026-09-13/01-pixel-art.webp) | A: 8-bit · B: 16-bit · C: Neon pixels · D: Miniature · E: Nano pixels |
| [02 — Watercolour](../assets/concepts/art-direction-2026-09-13/02-watercolour.webp) | A: Transparent washes · B: Ink and wash · C: Storm wash · D: Pastel storybook · E: Bold pigment |
| [03 — Anime](../assets/concepts/art-direction-2026-09-13/03-anime.webp) | A: Retro cel · B: Modern clean cel · C: Soft painted anime · D: Super deformed · E: Graphic action anime |
| [04 — Storybook Animation](../assets/concepts/art-direction-2026-09-13/04-storybook-animation.webp) | A: Golden-age cel · B: Adventure animation · C: Gouache folk tale · D: Luminous fairytale · E: Graphic mid-century |
| [05 — Vintage Cartoon](../assets/concepts/art-direction-2026-09-13/05-vintage-cartoon.webp) | A: Ink and ivory · B: Two-colour cel · C: Technicolor · D: Noir cartoon · E: Poster cartoon |
| [06 — Modern Art](../assets/concepts/art-direction-2026-09-13/06-modern-art.webp) | A: Bauhaus machinery · B: Cut-paper colour · C: Geometric surrealism · D: Psychedelic print · E: Minimal colour field |
| [07 — Line Art](../assets/concepts/art-direction-2026-09-13/07-line-art.webp) | A: Ink engraving · B: Clear-line comic · C: Pencil notebook · D: Coloured contour · E: Brush and ink |
| [08 — Playful 3D](../assets/concepts/art-direction-2026-09-13/08-playful-3d.webp) | A: Toy diorama · B: Round and bouncy · C: Adventure cel · D: Crafted miniature · E: Stylised spectacle |
| [09 — Realism](../assets/concepts/art-direction-2026-09-13/09-realism.webp) | A: Working steel · B: Cinematic salvage · C: Clean future · D: Retrofuture · E: Nature reclaimed |
| [10 — Low-Poly 3D](../assets/concepts/art-direction-2026-09-13/10-low-poly.webp) | A: Faceted sculpture · B: Retro console · C: Pastel geometry · D: Neon geometry · E: Painted facets |
| [11 — Clay And Stop Motion](../assets/concepts/art-direction-2026-09-13/11-clay-stop-motion.webp) | A: Plasticine · B: Ceramic · C: Felt and wool · D: Found-object puppets · E: Dark clay fable |
| [12 — Paper Craft](../assets/concepts/art-direction-2026-09-13/12-paper-craft.webp) | A: Layered cutout · B: Folded origami · C: Cardboard workshop · D: Pop-up theatre · E: Printed collage |
| [13 — Comic And Print](../assets/concepts/art-direction-2026-09-13/13-comic-print.webp) | A: Adventure comic · B: Halftone pop · C: Woodcut · D: Risograph · E: Graphic noir |
| [14 — Painterly](../assets/concepts/art-direction-2026-09-13/14-painterly.webp) | A: Oil impasto · B: Gouache adventure · C: Digital brushwork · D: Painted 3D · E: Pastel chalk |
| [15 — Flat Graphic](../assets/concepts/art-direction-2026-09-13/15-flat-vector.webp) | A: Bold vector · B: Soft vector · C: Industrial pictogram · D: Silhouette theatre · E: Retro futurist graphic |
| [16 — Depth And Camera](../assets/concepts/art-direction-2026-09-13/16-depth-camera.webp) | A: 2D sprites · B: 2.5D side view · C: 3D perspective |

**Earlier root shortlist, retained for comparison:** 07B Clear-line comic, 08D Crafted miniature and 14D Painted 3D. The owner subsequently preferred 16C; these alternatives do not override that preference.

### 01 — Pixel Art

![PIXEL ART comparison board](../assets/concepts/art-direction-2026-09-13/01-pixel-art.webp)

All five coded variants present. The very coarse treatment loses individual part identity; 16-bit/miniature variants preserve more detail. Pixel labels describe an aesthetic, not hardware-accurate sprites.

### 02 — Watercolour

![WATERCOLOUR comparison board](../assets/concepts/art-direction-2026-09-13/02-watercolour.webp)

All five present. Wash handling and mood differ; ink-and-wash gives useful edge definition. Fine rust detail would need simplifying at smaller gameplay sizes.

### 03 — Anime

![ANIME comparison board](../assets/concepts/art-direction-2026-09-13/03-anime.webp)

All five present. Clear cel and super-deformed forms differ from painted backgrounds. Graphic action panel uses a canted composition, so compare its line/colour treatment rather than adopting that camera.

### 04 — Storybook Animation

![STORYBOOK ANIMATION comparison board](../assets/concepts/art-direction-2026-09-13/04-storybook-animation.webp)

All five present, from classic inked animation to volumetric modern rendering. Character/world motifs are placeholders and need original identity development after selection.

### 05 — Vintage Cartoon

![VINTAGE CARTOON comparison board](../assets/concepts/art-direction-2026-09-13/05-vintage-cartoon.webp)

All five present. Black silhouettes and exaggerated poses communicate vintage appeal. Noir loses dark foreground detail; board includes decorative scenery text, not approved narrative/UI.

### 06 — Modern Art

![MODERN ART comparison board](../assets/concepts/art-direction-2026-09-13/06-modern-art.webp)

All five present. Shape and background language differ substantially. Psychedelic background competes with play; minimal colour-field version separates machinery clearly.

### 07 — Line Art

![LINE ART comparison board](../assets/concepts/art-direction-2026-09-13/07-line-art.webp)

All five present. Clear-line variant offers clean part edges; engraving/pencil/brush textures can obscure the pile. An outline workflow needs motion testing.

### 08 — Playful 3D

![PLAYFUL 3D comparison board](../assets/concepts/art-direction-2026-09-13/08-playful-3d.webp)

All five present. Rounded toy, cel and crafted-material approaches differ. Miniature surface treatment conveys weight; focus blur should not touch interactable parts.

### 09 — Realism

![REALISM comparison board](../assets/concepts/art-direction-2026-09-13/09-realism.webp)

All five present. Realistic material/lighting alternatives are visible. Dense rust/moss can make parts merge with the pile and raise asset-cleanup demands.

### 10 — Low-Poly 3D

![LOW-POLY 3D comparison board](../assets/concepts/art-direction-2026-09-13/10-low-poly.webp)

All five present. Large facets, retro texture, soft colour and emissive geometry vary. Retro-console panel has low-resolution smear and must be refined before serving as a production target.

### 11 — Clay And Stop Motion

![CLAY AND STOP MOTION comparison board](../assets/concepts/art-direction-2026-09-13/11-clay-stop-motion.webp)

All five present. Plasticine, ceramic, textile and found-object surfaces differ strongly. Felt/ceramic must preserve the meaning of metal or establish a clear handmade-world convention.

### 12 — Paper Craft

![PAPER CRAFT comparison board](../assets/concepts/art-direction-2026-09-13/12-paper-craft.webp)

All five present. Cutouts, folds, corrugation, theatre and collage are distinct. Origami makes small gears less recognisable; layered cutout keeps cleaner functional shapes.

### 13 — Comic And Print

![COMIC AND PRINT comparison board](../assets/concepts/art-direction-2026-09-13/13-comic-print.webp)

All five present. Print texture changes substantially. Halftone and woodcut backgrounds can compete with scrap edges; noir makes the bright magnet lead well.

### 14 — Painterly

![PAINTERLY comparison board](../assets/concepts/art-direction-2026-09-13/14-painterly.webp)

All five present. Oil/chalk textures differ from gouache and painted volume. Painted 3D gives a promising path to repeatable shapes; strongly textured alternatives need edge hierarchy.

### 15 — Flat Graphic

![FLAT GRAPHIC comparison board](../assets/concepts/art-direction-2026-09-13/15-flat-vector.webp)

All five present. Flat, soft, pictographic, silhouette and retrofuturist approaches differ. Flat treatments still include some volumetric shading; silhouette option hides part materials and needs inspection feedback.

### 16 — Depth And Camera

![DEPTH AND CAMERA comparison board](../assets/concepts/art-direction-2026-09-13/16-depth-camera.webp)

All three depth/camera variants present, with flat contours, side-view volume and three-quarter perspective. The perspective view demonstrates occlusion. Generated ammunition appears as a rack of completed rounds, so this board is only a camera/depth comparison and must not become the assembly/mechanism specification.


## Repeatable production routes

These are task-specific production assessments, not measured schedules. The owner has removed development timelines as a consideration. “Easier to extend” means fewer one-off redraws, reusable pieces, controllable animation and predictable consistency.

| Family | Credible route using our tools | Where consistency needs proof |
| --- | --- | --- |
| Pixel art | Author a small sprite/tile vocabulary; use fixed pixel sizes and palette rules; optionally render a model as an underdrawing | Clean pixel clusters, rotations and animation need deliberate cleanup; image generation alone is not a production sprite pipeline |
| Watercolour | Layer painted backgrounds; use crisp movable cutouts or brush-textured 3D foreground assets | Prevent soft washes from hiding edges and keep pigment texture stable during motion |
| Anime | Rigged cel-shaded models or separate drawn parts, painted backgrounds and hand-shaped effects | Consistent outlines, facial/pose appeal and shadow thresholds across movement |
| Storybook animation | Painted layers plus rigs or stylised 3D; give main machinery a distinct silhouette | The animated acting and material finish must live up to the still frame |
| Vintage cartoon | Cutout rigs with selectively drawn special poses/effects, or carefully shaded 3D | Authentic elastic motion is a major craft requirement; a vintage filter does not supply it |
| Modern art | Small geometric 2D/3D kit with a strict palette and composition system | Abstract treatment must preserve the meaning of scrap, armour, magnet and enemies |
| Line art | Toon/outline rendering or clean cutout drawings over illustrated layers | Stable line weight, no crawling hatching and strong selectable-object separation |
| Playful 3D | Reusable Blender models, a small material library, expressive rigs and art-directed lighting | Attractive proportions and materials at normal gameplay size |
| Realism | Coherent model kit, consistent physically based materials and controlled lighting | Surface detail and believable engineering can increase authoring/cleanup needs and visual noise |
| Low-poly 3D | Deliberate simple meshes, shared palettes/materials and reusable rigs | Avoid generic asset-pack appearance; preserve recognisable object geometry |
| Clay/stop motion | 3D meshes with tactile materials and restrained stepped animation | Keep handcrafted texture believable in motion; material choice must suit the fiction |
| Paper craft | Layered planes or shallow meshes, illustrated textures and small rigged assemblies | Paper joints, folding and occlusion must stay coherent as pieces stack and rotate |
| Comic/print | Clear silhouettes, toon shading, hand-drawn or procedural print textures | Halftone, grain and cross-hatching must not fight with important gameplay cues |
| Painterly | Brush-textured 3D foreground kit or cutouts with layered painted scenery | Surface strokes should stay attached to forms and remain consistent across lighting |
| Flat graphic | Reusable geometric artwork or flat-shaded models with a tightly controlled visual grammar | Needs excellent composition, motion and material distinctions to avoid the rejected slide-like appearance |

ImageGen is useful for choosing the look and producing candidate source art. Blender is configured locally and its executable was found; it is appropriate for repeatable geometry, materials, rigs and render tests. TRELLIS may help with hero-model candidates after a direction is selected, but generated meshes still need topology, scale, pivots, materials and animation inspection. No live TRELLIS service or new asset pipeline was verified during this breadth pass. Unreal should provide actual readable text, interactions and game-state feedback; text baked into these boards is concept artwork.

## Research that informs the choices

Sources accessed 2026-09-13. These are inspected documentation/interview text, not an assertion that every linked video or game was played.

- [Epic's Paper 2D documentation](https://dev.epicgames.com/documentation/unreal-engine/paper-2d-overview-in-unreal-engine) documents sprites, sprite sheets, flipbooks and tile systems for 2D and hybrid projects. This establishes a supported route within the available engine, not proof of this game's complete pipeline.
- [Octopath Traveler II developer interview, 22 August 2023](https://www.unrealengine.com/developer-interviews/octopath-traveler-ii-builds-a-bigger-bolder-world-in-its-stunning-hd-2d-style?lang=en-US) discusses pixel artwork in 3D surroundings, altered proportions and lighting. It supports separating medium from scene depth. Its embedded videos were not watched.
- [Studio MDHR's official press kit](https://studiomdhr.com/press-kits/) identifies Cuphead's traditional cel animation and watercolour backgrounds. The cited inspiration is the 1930s, adjacent to the owner's 1940s interest. Our vintage board explores a broader period and does not copy its cast or UI.
- [Dordogne developer interview, 6 March 2023](https://www.cnc.fr/web/en/news/dordogne-the-story-behind-a-watercolour-video-game_1905450) describes watercolour on paper combined with 3D/parallax. The artist's expertise made that method practical for that team; it does not establish that hand-painting is automatically easiest for us.
- A Nintendo Paper Mario developer-interview search result discussed papercraft, but the direct page timed out. Blender's Grease Pencil pages also failed to load through the web tool. Neither is used as evidence of a verified runtime/export workflow here.

Our visual comparison is generated original subject matter, not a collage of other games' assets. The referenced games inform techniques; their market performance has not been used to predict sales for these styles.

## What establishes the final choice

1. **Preference received:** Klaus chose 16C as the visual foundation and proposed the preparation/action camera split. Keep this distinct from approval of a finished style guide or all pictured objects.
2. Refine the preferred identity across a quiet scene, magnet pull, loading/fire sequence and enemy impact. Establish recognisable scrap/material shapes and a restrained real UI treatment.
3. Make a small reproducibility test: the same magnet from several angles, several consistent scrap types, one enemy and a short motion/render sample using the proposed asset route. It can be a separate art test; it need not implement the game.
4. Record the accepted shape language, palette, materials, line/texture rules, lighting, UI relationship and asset method. Only call the direction selected when the owner accepts it and the extension method has evidence.

The current goal remains active. These boards are a broad first pass, not a claim to cover every possible art style or to have established owner satisfaction.
