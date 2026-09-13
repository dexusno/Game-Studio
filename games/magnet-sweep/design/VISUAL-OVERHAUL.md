# Magnet Sweep â€” tactile salvage visual overhaul

Owner direction, 2026-09-13: major graphics overhaul after playtesting Expedition Choices. This increment changes presentation before any more gameplay work. Its acceptance is attractive, readable actual game footage; generated reference art alone is not delivery.

## Direction

Make a miniature industrial recovery machine worth owning. Teal enamel shells, exposed copper windings, satin steel contact surfaces, graphite gaskets and warm furnace light connect the hero to the workbench. Thick silhouettes, layered construction and deliberate negative space carry recognition at gameplay distance. Small chips, screws and brushed grain provide close-up credibility without replacing those broad forms.

| Role | Color | Use |
| --- | --- | --- |
| Graphite | #101C23 | Quiet recesses, panel backs and floor |
| Deep teal | #17616A | Player magnet and reusable machine housings |
| Oxidized blue teal | #284750 | Secondary bench frames |
| Copper | #B9723C | Coils, valuable conductors, warm bevel contrast |
| Satin steel | #AABAC0 | Functional contacts and steel salvage |
| Warm ivory | #E4DBC4 | Readable numbers and objective text |
| Active cyan | #64E4D4 | Restrained powered strips and current selection |
| Furnace amber | #FFAB50 | Heat, payout and receiving machinery |
| Hazard vermilion | #E76442 | Unsafe material and actual overload only |

Assume the existing fixed elevated workbench camera. Keep the interaction plane and object silhouettes uncluttered. Build large structural areas first, then bevels and seams; finish with sparse wear. Material identity must survive a thumbnail and a neutral light. Avoid polishing all surfaces to the same roughness. The player magnet should occupy a stronger visual hierarchy than loose salvage, but never cover the aim point or obscure a dangerous held object.

## Hero asset

[Original generated reference](../assets/visual-overhaul/hero/hero-electromagnet-reference.png): broad U-shaped enamel electromagnet with distinct copper rear winding and steel pole shoes. The transparent image is a modelling reference, not a screenshot. The built-in OpenAI tool was used as exposed by this session; no unverified model version is claimed.

Actual local TRELLIS.2 generation and Blender finishing completed. The hero directory contains FBX, embedded-PBR GLB, editable Blender source, 2K base-color/metallic-roughness maps, a neutral tangent map, three inspected authoring views and provenance. The delivered mesh has 159,999 triangles and one material slot. An aggressive reduction/bake and a new-UV bake were rejected after visual checks; the selected version preserves original UVs and more actual surface geometry. Exact units, pivot, texture contract, hashes and remaining close-up softness are recorded in its [provenance](../assets/visual-overhaul/hero/PROVENANCE.md). Root owns the shared asset manifest; this worker returns exact rows.

## Feedback and UI integration guidance

Power strips should become brighter with the existing energized state, while the visible field remains thin and readable. Pole lights and a brief contact flash can reinforce capture; use the existing physical events rather than inventing effects that promise a different rule. Overload uses hazard color and a restrained pulse. Deposits should emphasize the receiving machine and material becoming banked.

Replace generic boxes with a consistent instrument-panel grammar: inset dark screens, selective brass borders, a few mechanical edge breaks, clear condensed display numerals and generous text spacing. Hierarchy is objective, current physical state, actionable control, then detail. Do not fake screws and labels across every empty surface. Essential text remains engine-rendered and scales; raster art should frame information, not bake it into unreadable paragraphs.

## Verification boundary

The image reference has been visually inspected and its true alpha validated. The finished mesh has been inspected from top, front and rear and both exchange files reimport with exact final triangle counts, one material and usable UV data. Studio rendering proves only the asset's appearance under that lighting. In-game scale, material bindings, input readability, frame time and owner acceptance remain integration checks.

## Integrated result

Expedition Visuals 0.7.0 has been packaged and its actual 1600×900 starter/outfitter/worksite and two labelled late-site fixtures inspected. The hero material, scale and silhouette are verified at the gameplay camera. The kit, fonts, UI and animated furnace are integrated. Capture findings drove header/padding fixes, quieter trim, contextual machinery labels, a clear frame panel and checked material pin connections. Independent UI review accepts the corrected late-site presentation. Exact source/artifact evidence and remaining limits are in [visual verification](../evidence/visual-overhaul-verification.json). Owner appearance acceptance, live controls/sound and sustained performance remain separate judgments.
