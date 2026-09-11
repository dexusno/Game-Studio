# Recessed creature faces — dread-v1

Both facial candidates are frozen with the owner-requested `owner-luminous-v1` eye materials and the preserved `status-mask-v2` color correction. Briarhide has green luminous irises; MireSeer has ember-orange luminous irises. The recessed sockets and heavy lids remain intact. Root imported only the existing eye materials and confirmed the colors in actual Editor views: green-eyes-editor-v1 Melee-Detail frames20/40/65 and fire-eyes-editor-v1 Caster-Detail frames15/40/60, under `.local/organic-fire-review`. The caster also reads orange in the side combat view. The eye can be occluded by its brow when facing down or away; no owner acceptance is claimed.

The editable specifications are [Briarhide.json](Briarhide.json) and [MireSeer.json](MireSeer.json). [appearance-contract.json](appearance-contract.json) records exact source/export/map/preview hashes, measured placements, material definitions and preservation results. [manifest-rows.csv](manifest-rows.csv) supplies proposed rows for the integration owner; the main manifest was not edited.

## What changed

Briarhide's separate projecting amber iris/pupil discs were removed. Surface measurements located the cranial orbit below and behind those old cosmetic anchors. The replacement eye is recessed 0.9 cm from that measured surface and enclosed by a broad upper brow and lower lid. The owner found the initial muted iris too hard to see and explicitly requested a green glow. Only the eye materials changed for that correction.

MireSeer's original eye bulbs included large white highlights painted into the skin atlas. Those local surfaces were removed, while the atlas itself remained byte-identical. The replacement eyes sit 2.1 cm behind the measured original surface, under fitted fleshy orbital hoods. The owner explicitly requested fiery eyes, so the previous drab iris now emits ember orange. Pupils remain dark and specular response is reduced; no painted white reflection was added.

## Current luminous eye materials

| Species | Linear emission hue | Iris strength | Surrounding eye strength |
| --- | --- | ---: | ---: |
| Briarhide | `(0.035, 1.0, 0.10)` | 6.0 | 0.15 |
| MireSeer | `(1.0, 0.085, 0.006)` | 8.0 | 0.20 |

Eye and iris roughness is 0.38, specular 0.10. The non-emissive pupil has roughness 0.44, specular 0.08. Only slots `M_OE_DreadEye`, `M_OE_DreadIris` and `M_OE_DreadPupil` changed. Dark sockets, skin, body, mouth, geometry, rig and all repaired vertex colors are unchanged. The first exported FBX color stream was checked again to preserve the localized body status mask.

New native CPU material previews: [Briarhide green eye](../../../../../.local/ai3d/outputs/organic-enemies/briarhide/finished-dread/luminous-oblique.png) and [MireSeer ember eye](../../../../../.local/ai3d/outputs/organic-enemies/mire-seer/finished-dread/luminous-oblique.png). Both show a small colored iris beneath the retained hood with a dark pupil. Older pose and perspective images below document the unchanged geometry and the previous non-emissive treatment; they do not depict the current glow. Unreal exposure and bloom still need the actual integrated view.

The new surrounding skin receives sampled original pigment through the linear `DreadFaceColor` vertex-color layer, only on DreadSkin corners. Original body corners retain their original `OrganicTell` RGBA status mask, including affine interpolation on newly clipped body edges. This avoids both disconnected-atlas UV streaks and contamination of the body's status-emission mask. The local cut clips intersecting source triangles at the socket boundary; it does not leave large original polygons crossing the opening. Original body and accepted mouth materials remain unchanged.

## Corrected integration defect

The initial Unreal import exposed a real source/export error: [dormant Briarhide frame 60](../../../../../.local/organic-fire-review/dread-v2-melee-detail/Saved/EnemyMotionStudy/Melee-Detail/Frame_00060.png) glowed orange across its entire body. The original anatomy FBX's first color set was `OrganicTell`, whose red channel is a localized status-emission mask. The initial facial exporter placed `DreadFaceColor` first with white body defaults. The CPU body shader did not consume that mask, so the studio previews failed to reveal the error.

The source now exports a CORNER-domain first set: original RGBA on retained body/cavity geometry, interpolated status RGBA on clipped body fragments, sampled pigment only on DreadSkin, and neutral non-pigment values on other new eye materials. Both corrected FBXs were parsed after export: every indexed first-layer RGBA equals the authored corner stream. Body R ranges are again 0–0.530830 for Briarhide and 0–0.686001 for MireSeer, with G/B=0, A=1 and no body corner at R=1.

This repair changed only color attributes on the reviewed meshes. Before/after fingerprints cover every position, polygon, normal, UV, material assignment, skin weight and bone/object rest transform; they match exactly. Every visible DreadSkin pigment corner also matches the reviewed candidate. Existing CPU previews are retained with their original hashes, not represented as new renders. Corrected Unreal review remains the integration owner's next step.

## Actual visual review

Matched baseline/candidate images use the same Cycles CPU lighting and camera. Native close views are 700 × 700. The perspective samples are 1280 × 800, camera angle 70 degrees, approximately 4.6 m from the target. These are Blender views; the filenames containing `game` do not mean an Unreal capture.

| Creature | Inspected candidate views | Result |
| --- | --- | --- |
| Briarhide | Front, side, fully open jaw, perspective whole creature | The discs are gone; eyes remain enclosed in front and profile. The brow reads as part of the cranium. No new side opening or floating eye appeared in the inspected jaw pose. Parent independently inspected the matched front and candidate side and cleared this candidate for the next engine review. |
| MireSeer | Front, oblique, side, fully open jaw, fully loaded frill, perspective whole creature | The oversized painted eyes are gone. Hood/eye attachments stay connected in the inspected jaw and frill poses. Small lateral eyes produce a more severe face without changing the amphibian design. The dark neutral side-mouth gap is also visible in the unchanged baseline; the accepted mouth was preserved. |

Representative matched sources, all in the private output root:

- [Briarhide baseline front](../../../../../.local/ai3d/outputs/organic-enemies/briarhide/finished-dread/baseline-front.png), [candidate front](../../../../../.local/ai3d/outputs/organic-enemies/briarhide/finished-dread/candidate-front.png), [candidate side](../../../../../.local/ai3d/outputs/organic-enemies/briarhide/finished-dread/candidate-side.png), [candidate jaw](../../../../../.local/ai3d/outputs/organic-enemies/briarhide/finished-dread/candidate-jaw-open.png).
- [MireSeer baseline side](../../../../../.local/ai3d/outputs/organic-enemies/mire-seer/finished-dread/baseline-side.png), [candidate side](../../../../../.local/ai3d/outputs/organic-enemies/mire-seer/finished-dread/candidate-side.png), [candidate jaw](../../../../../.local/ai3d/outputs/organic-enemies/mire-seer/finished-dread/candidate-jaw-open.png), [candidate loaded frill](../../../../../.local/ai3d/outputs/organic-enemies/mire-seer/finished-dread/candidate-crest-loaded.png).

Earlier Briar candidates were rejected for UV streaks, source triangles crossing the opening, and a high/forward anchor that exposed the eye from the side. Their available FBX/Blender/report/images are retained under `briarhide/finished-dread/iterations/20260911T130022Z`, `20260911T130502Z` and `20260911T131026Z`. These are rejected evidence, not alternate approved faces. Current producing script/spec copies accompany each final candidate; historical producing-script copies were not captured for those earlier attempts.

## Preservation and frozen files

| Creature | FBX SHA-256 | Triangles | Bones | Retained original vertices |
| --- | --- | ---: | ---: | ---: |
| Briarhide | `cfe9b44bf884af6f98c66dd0bb7dd5b86e3d30427cced5cc5422eec372f61881` | 202,036 | 38 | 243,249 |
| MireSeer | `8494a35668b597e08c028665b9c099222b4bcdd3019b35ab9d6a2089a6b811a0` | 183,783 | 43 | 220,414 |

Files are `.local/ai3d/outputs/organic-enemies/{briarhide,mire-seer}/finished-dread/SK_OE_{Briarhide,MireSeer}.{fbx,blend}` from the studio root. Each folder also contains `prep-report.json`, the original two 4K maps, `appearance-spec.json`, `preparation-source.py` and the matched previews. Prior `finished-anatomy` and `finished-performance` sources are intact. The color-defective facial exports and producing scripts are archived in Briarhide `iterations/20260911T133727Z` and MireSeer `iterations/20260911T133818Z`.

All accepted bone names, parents, rest matrices and anatomy channel definitions compare exactly with the anatomy baseline. Every retained original vertex keeps its position and skin weights. Evaluated neutral, full jaw, full grip and full crest comparisons found a maximum difference of **0.0 cm** on retained vertices. New clipped boundary and eyelid vertices are excluded from that exact comparison and were inspected visually. Four skin influences remain the maximum. The final focused check also verified actual baseline/export/source/spec/map/preview hashes and parsed the new Python source successfully.

## Integration

Use `output_folder=finished-dread`, `appearance_revision=dread-v1` and the fresh `SK_OE_Briarhide_Dread` / `SK_OE_MireSeer_Dread` assets. Scale, axes and runtime Jaw/Grip/Crest interfaces are unchanged. Both exports have nine material slots: original body; Oral, Tooth and Tongue; DreadSkin, DreadSocket, DreadEye, DreadIris and DreadPupil. The old `eye_placements` list is empty and old EyeIris/EyePupil slots are absent. New measured surfaces are in `appearance_eye_placements`.

For the current eye-only integration, run `import_organic_enemies.py` in the full Editor with `-OrganicEnemyEyesOnly`. The importer now connects optional `emissive_color` multiplied by `emissive_strength` to Emissive Color and validates the existing mesh's material bindings before updating the three eye definitions per species. Geometry and bindings are identical, so no skeletal mesh reimport is needed. Matching new FBXs are supplied for later clean imports. Require `Saved/OrganicEnemies/import.json` to report complete with `eye_material_only: true`.

When performing a full import, retain the first linear corner-color set with replacement enabled. `M_OE_DreadSkin` requires `VertexColor RGB`; the body continues reading the same stream's R channel as its original status mask. Skin roughness stays 0.68 and specular 0.25. All face materials remain opaque; only Eye and Iris emit. Namespace materials by species, preserve body atlas wiring and verify persisted slot bindings in a fresh engine process.

The useful next check is the same dormant Melee close view to confirm normal body shading, then the ordinary combat distance including an open jaw and Mire's charged frill pose. Inspect eye shading, socket attachment and retained claw/foot contacts during actual motion. No additional body pose or combat-suite changes are part of this facial pass.

## Reproduction and limits

Actual preparation ran sequentially with installed Blender 5.2.1 LTS, four CPU threads and Cycles CPU. Both processes returned exit code 0. The commands, run from `D:/Game-Studio`, were equivalent to:

```powershell
& 'D:/Blender/blender.exe' --background --threads 4 --python-exit-code 1 --python games/dreambound/scripts/refine_creature_faces.py -- --asset Briarhide --views front,side,jaw-open,game
& 'D:/Blender/blender.exe' --background --threads 4 --python-exit-code 1 --python games/dreambound/scripts/refine_creature_faces.py -- --asset MireSeer --views front,oblique,side,jaw-open,crest-loaded,game
```

The actual background launches used `Start-Process -WindowStyle Hidden` with redirected output. Original preparation logs are `briarhide/face-preparation-04.log` and `mire-seer/face-preparation-01.log`, with matching `.err` files. The script reads the private output root from `config.local.json` and imports the existing anatomy/texture helpers; their handoff hashes are recorded in the contract. Re-running archives the current candidate first and exports a new candidate; compare its resulting hashes and views before substituting it for the frozen files.

The color-only repair used the same hidden CPU launch for each asset with `--refresh-color-stream` replacing `--views ...`. Both exited 0; logs are each asset's `finished-dread/color-fix-01.log` and `.err`. That mode derives corrected color correspondence from the authoritative source, checks it against the frozen candidate, and transfers only color attributes onto the existing mesh before export. It verifies the actual first FBX color layer and retains the prior renders with an explicit reuse note. The normal full preparation path also contains the corrected status-mask handling.

The current eye-material update used `--refresh-eye-materials` in the same sequential hidden CPU commands. Both exited 0; logs are each asset's `finished-dread/luminous-eyes-01.log` and `.err`. That mode edits the three eye materials on the frozen Blender scene, exports the same geometry/color streams, and renders one oblique material preview. Exact previous export hashes and archive locations are recorded in `eye_material_update` inside the prep and portable contract. The appearance specifications record the owner-requested hues and strengths, and the normal preparation path produces the same material definitions.

There are no new gaze controls or animated eyelids. The eyes are fixed to the accepted head rig and the outer skin blends into sampled surrounding weights. Original dense, sometimes faceted body surfaces and mouth geometry remain. This pass adds local eye/lid topology rather than optimizing whole-creature LODs. The small visible eye can occupy few pixels at combat distance, and Unreal's illumination may alter its wet highlight. Those limits need the actual integrated view, not another static-pose claim of fear or animation quality.

No new generative assets, external designs or texture sources were introduced. Original reference/TRELLIS provenance and dependency terms remain as recorded in the organic-enemy generation evidence and `AI3D.md`. These derived candidates retain the existing local-evaluation asset status.
