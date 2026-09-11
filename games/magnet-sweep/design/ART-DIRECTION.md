# Magnet Sweep — art direction for the concept demo

2026-09-12. Scope: visual appeal and gameplay readability. This is an art recommendation grounded in the existing original meshes and `WorkbenchRuntime.cpp`; it is not evidence of sales or player enjoyment. Root owns the engine implementation and shared asset manifest. The art specialist owns this note and the clearly labeled concept image.

## Visual promise

**A little salvage machine you want to touch.** Chunky enamel, rounded machined metal, a warm crucible and a satisfying physical haul. Aim for a premium tabletop toy photographed in a calm workshop. Avoid rusty visual noise, heavy sci-fi ornament, flat glowing wireframes or an environment that competes with collecting.

The current source already contains the right ingredients: fixed orthographic three-quarter view, a dark teal rectangular tray, original beveled washers/bolts/plates/tangles, a teal horseshoe magnet with ivory poles and earned coils, and a copper circular furnace. Improve their relationship rather than adding rooms, characters or a new asset catalogue.

## Five implementation priorities

1. **Let the hero objects own contrast.** The broad tray should be a low-contrast blue-charcoal surface, with its grid barely visible at rest. Keep outer rails darker or only slightly lighter than the tray, except small bolt highlights. Use contact shading to separate pieces. The default board must read as scrap and inviting rings before it reads as a checkerboard or frame. Existing `Tray`, `Grid` and `Edge` parameters are sufficient.
2. **Make the magnet a recognizable, evolving toy.** Saturated teal enamel, cream pole faces, copper collars and brass coils establish its identity. Broad highlights should describe thickness; bevel edges should catch light without becoming neon. Earned Breakaway coils and reach arms must visibly change the silhouette at the normal gameplay camera. During the forge, a brief reveal/pulse may draw attention to the actual newly attached geometry; returning to the quiet idle state preserves that payoff.
3. **Separate materials before adding detail.** The enamel magnet should have a broad, clean reflection; steel scrap a slightly rougher silver/blue response; copper a warm metallic reflection. Avoid making every item equally shiny or equally emissive. Existing geometry supplies bolt heads, washer holes and curved rods. Preserve those silhouettes and variation in orientation. The plate must remain visibly thicker than a decal. Tiny procedural scratches are optional and should wait until the rendered scene demonstrates a specific need.
4. **Concentrate warmth in the furnace payoff.** Use the copper rim and orange interior as one strong destination on the right. Keep the opening legible as a dark-walled well with a hot inner surface, not a solid orange disk pasted on top. During dumping, an arcing material stream, small upward light pulse and visible fill rise express mass. Ordinary ring highlights stay below furnace brightness. Cap bloom so nearby scrap and the furnace boundary remain visible throughout the largest deposit.
5. **Protect the playable composition.** Maintain the near-top-down three-quarter camera so pieces have thickness while ring positions remain easy to judge. The entire tray, furnace pad and carried haul need comfortable margins from HUD text. The magnet remains the most distinct moving silhouette. The broad pull preview is a transparent mint corridor, with thin boundaries and solid endpoint/ring emphasis; links are quieter than actionable rings. Do not let the preview, cargo or celebratory text blanket the chosen path.

## Compact color and shape language

These sRGB swatches communicate the intended relationship, not literal replacements for the engine's linear-color values. Judge the actual rendered output before changing values.

| Role | Target swatch | Shape / material behavior |
| --- | --- | --- |
| Background | `#111D24` | Calm dark workshop plane; no busy props |
| Tray | `#294246` | Broad matte blue-charcoal, restrained grid |
| Magnet | `#168D86` | Teal enamel U profile; rounded highlight |
| Pole faces | `#E9E4C9` | Cream blocks that show the opening direction |
| Loose steel | `#A6B7B7` | Silver cool metal, visible bevels and holes |
| Copper / earned parts | `#C57E42` | Warm machined accents and substantial coils |
| Available ring | `#F3BD70` | Amber ring silhouette; mild emission |
| Selected group | `#84F4D0` | Mint ring + outline, not color alone |
| Furnace | `#FF9C39` | Warm core with dark rim and controlled pulse |

Round forms mean graspable/recoverable objects: washer holes, ring handles, coils and furnace mouth. Squared forms frame the tool and work surface. Retain both languages so a tangle's actionable ring is distinct from its bent rods. Amber availability becomes mint selection, with visibly emphasized geometry and the existing preview to avoid relying only on color.

## Animation and feedback states

- **Idle:** quiet tray and readable ring handles; only restrained movement on the tool/rings. Continuous decorative motion must not imply that a stationary tangle is drifting.
- **Sweep:** loose pieces lift and stream into a compact visible haul, with motion responding immediately to the pointer. The stream shows attraction without a carpet of particle effects.
- **Aim:** actual affected group is emphasized before release. Nonselected scrap remains visible. A large forgiving corridor reads as an invitation to gather, not a precision laser.
- **Release:** a short taut pull followed by connected pieces arriving in a legible sequence. Preserve the causal link from the chosen ring to its direct neighbors.
- **Deposit / forge:** the most intense warmth and largest motion occur here, then settle. An earned component appears on the player's actual magnet; the improvement should be visible after the toast has gone.

## Evidence and acceptance

The source was inspected; the original mesh set was already rendered and inspected separately. The [visual-target illustration](../assets/concepts/salvage-toy-visual-target.png) was generated with the built-in image tool and visually reviewed. Its second pass corrects an initially misleading V-shaped beam to a single broad diagonal gathering corridor. It is visibly marked **CONCEPT ART - NOT GAMEPLAY**, and cannot be used as a store screenshot or evidence that the engine achieves the look. [Provenance and exact prompts](../assets/concepts/PROVENANCE.md) are recorded with the image.

The illustration guides hierarchy, materials, mass and lighting. It is not an exact mesh or layout specification: its plate holes, continuous arch coil and dense tangle detail are illustrative variations, and the furnace at the right edge needs more comfortable margin in the playable camera. Keep the current simple plate, reusable coil pieces and existing mesh set; do not add those illustrated details unless an actual rendered readability/appeal issue warrants it. The capsule selection calculation remains defined by BRIEF.md and the game model.

Root should compare a clean actual gameplay screenshot and a deposit/pull frame against these priorities. At normal window size, verify: washer holes and bolt heads remain distinguishable; the magnet opening is apparent; every available ring is discoverable; selected neighbors can be counted; the furnace opening remains visible during pouring; and a large haul does not hide the usable route. Repeat the comparison at 1280×720 and on the largest intended haul. A grayscale check can reveal weak contrast, but does not replace normal color and motion inspection.

This direction is intended to improve recognizability and perceived care. Whether it improves buying interest requires actual viewer/player evidence; no art style guarantees better sales.

## Actual packaged-render review — 2026-09-12

Inspected the actual 1600×900 PNG at `BuildOutput/ConceptDemo/Windows/MagnetSweep/Saved/Screenshots/MagnetSweep00000.png`, supplied by root after the first packaged launch. This is static visual evidence from the executable. No native input, frame pacing, deposit animation or human enjoyment was assessed by this review. The concept image remains a separate reference.

**Recommendation: fix the following before presenting the concept demo to Klaus.** The current image does not yet achieve the intended tactile visual hierarchy. These issues are visible in the rendered result, not inferred solely from source.

| Priority | Actual observation | Action and visible acceptance |
| --- | --- | --- |
| 1 — Framing | The complete workbench/furnace occupies roughly 780×380 pixels near the center. A large unused band separates it from both HUD bars; the magnet and scrap consequently appear miniature. | Fix the camera/aspect calculation first. Target approximately 1120–1200 pixels of total workbench width and 520–570 pixels of height at 1600×900, centered within the area between HUD bars. Preserve comfortable edge margins and the full furnace. Scale the view, not each scrap object. Root is investigating the apparent world-to-screen scale. |
| 2 — Value hierarchy | The tray and large base render pale grey-teal, approaching the steel scrap's brightness. The intended blue-charcoal surface and material separation have been lost. | Darken Tray/Base/Background first and moderate the broad fill, preserving metal edge highlights. Do not lift global exposure to solve small objects. Steel and cream poles should clearly separate from the tray; the grid and frame should remain subordinate. Compare a rendered frame, since linear source values are not output swatches. |
| 3 — Furnace depth | The furnace looks like a bright cream-yellow pad/button. The hot surface visually fills the ring at its top, and the label partly overlaps the upper furnace region. | Give the furnace a visible dark-walled cavity and place the hot surface below the rim. Current source places the hot cap at Z73 near/above the ring center at Z72, explaining the flat reading. The rim must occlude the rear/side edges of the hot surface from this camera. Use a restrained orange core and keep the label outside the mouth. Existing cylinder/ring primitives can build the cavity; no new feature is needed. |
| 4 — Hero and tactile detail | The U magnet is recognizable, but too small to appreciate the enamel, cream poles or later coils. Washers/bolts/plates are distinguishable only with attention. | After reframing, confirm the magnet's opening and pole faces at normal viewing size. Keep a clean teal enamel highlight, rougher steel and warmer copper. Preserve geometric bevels; soften excessively hard floating shadows only if they still distract at the corrected framing. Do not add texture noise to compensate for scale. |
| 5 — Information separation | Top/bottom bars contain readable large labels but several very small secondary lines; the large UI bars overpower the currently small board. The furnace callout overlaps the play object. | Root owns HUD corrections. Enlarge the key control/instruction text and progress labels, reduce unnecessary secondary copy if needed, and anchor contextual labels clear of objects. After the camera fix, verify that available rings, thin links, selected corridor and cargo remain distinct without extra UI covering the path. |

Positive evidence: the original beveled meshes are visibly present, material families already distinguish copper from steel, the teal U silhouette is recognizable, and the board/furnace arrangement is coherent. This supports improving the current scene rather than replacing the asset set.

The next art check should inspect a new actual idle frame after these changes, plus one aimed linked-pull frame and a mid-deposit frame with a large haul. Passing the idle composition alone cannot establish readable effects in motion. Root remains the writer for engine changes and their verification; this review changes only the art-direction note.

## Integrated demo review — 2026-09-12

Root and independent QA inspected the corrected native scene. The workbench now spans about 1120 pixels at 1600x900, the tray is dark enough to separate metal, runtime font text is sharp, and the orange furnace has visible walls with its label below the mouth. The full-rig block was moved above the furnace after QA found the label obscured it. Curated unedited engine captures are in [evidence](../evidence/README.md). The asset set and gameplay scope stayed small.

This is a coherent concept-demo treatment, not a claim of final commercial polish or a pixel match to the illustration. Live held-preview inspection, perceptual sound assessment and human judgments of appeal/replay remain open. The generated target's extra surface wear, plate holes and richer tangle construction remain illustrative rather than committed content.
