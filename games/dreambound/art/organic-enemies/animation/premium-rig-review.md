# Performance rig handoff — 2026-09-11

The local Blender increment is ready for Unreal integration. Briarhide has 21 deform bones; MireSeer has the same chain plus a useful throat-sac control, for 22. This is a deformation handoff, not evidence that the animation now meets the owner's Windrose / Monster Fantasy quality target. Root owns import/build/capture; the enemy engineer owns the authored runtime performance.

## What the assessment found

The v11 melee side frames 155/164, caster side frames 43/58, player-facing melee frame 164 and boss front frame 465 showed a detailed design constrained by a single spine, no clavicles and no independent neck. The pelvis already existed, so some stiffness was an animation-use problem rather than a missing bone. The current pipeline exports `bake_anim=False` and drives `UPoseableMeshComponent` directly: a new FBX alone does not supply an authored performance.

The generated meshes already contain 189,560 / 170,000 Blender triangles. More surface triangles would not provide deliberate bend loops or better motion. The existing four-influence skin had useful knees, elbows, hands and feet, but its proximity-derived shoulder details sometimes borrowed weight from distant torso/head pivots.

## Implemented contract

Common hierarchy: `root → pelvis → spine_lower → spine → chest → neck → head`; `chest → clavicle_l/r → upperarm_l/r → forearm_l/r → hand_l/r`; `pelvis → thigh_l/r → shin_l/r → foot_l/r`. MireSeer adds `head → throat`.

All 16 previous bone head/tail coordinates are exactly unchanged. The original edit bones are retained rather than rebuilt. Reparenting introduces at most a few float32 ULPs in the recomposed basis; exact measured values and all global rest matrices are in the accompanying contract. A fresh skeleton is required because the spine, head and upper-arm parents change.

The new abdomen, rib/chest, neck and clavicle bones have substantial skin regions, not nominal skeleton entries. The original topology, vertex positions, UVs, vertex colors, material slots, atlas bytes, Briarhide eye geometry and pigment floor remain unchanged. The reviewed elbow, wrist, knee and ankle gates are retained.

Briarhide's central neck weights exclude the lateral mantle. Its horned shoulder plates now follow their clavicles instead of stretching toward the skull. MireSeer's upper lateral gill fan stays with the head; its lower upper-arm skin no longer pulls from the abdomen. Its throat envelope covers the anterior sac without moving the jaw or gill fan independently.

## Executed evidence

- Blender 5.2.1 LTS, background CPU preparation/rendering, four threads. No Unreal or GPU process was launched by this worker.
- Reviewed combined coil/reach poses, shoulder close-ups and the neck/shoulder or throat close-up. The final Briarhide close-up clears the visible stretched shoulder-horn defect; MireSeer's correction reduces the reach's edges over 2× from 1,402 to 576. These counts locate deformation problems; they are not an animation-quality score.
- The sampled combined poses include clavicle yaw ±15° / roll ±8°, distributed torso deltas below 20°, a 60° upper-arm reach and whole-hand flex up to 40°. They do not certify every combination of those limits. Use about 50° upper-arm reach for routine motion until the actual performance is reviewed. The earlier 78° knee test still shows inner-knee compression; the knee/ankle gates did not change in this pass.
- MireSeer's local throat scale `(1.10, 1.02, 1.10)` was reviewed: X/Z radial expansion and Y length. Runtime's 8% radial / 1.5% length falls inside that sample.
- CPU FBX reimport passed the complete 21/22 parent maps, real weights on every added bone, at most four normalized influences, and unchanged triangle counts relative to the previous FBXs. Briarhide's FBX retains its pre-existing 189,559-triangle roundtrip count; MireSeer retains 170,000.

Current outputs live in `.local/ai3d/outputs/organic-enemies/{briarhide,mire-seer}/finished-performance/`: FBX, editable neutral Blender source, exact atlas files, `prep-report.json`, CPU PNGs, `preview-poses.json`, `deformation-edge-diagnosis.json` and `fbx-roundtrip.json`. Rejected/intermediate shoulder views and their reports are retained in `iteration-02-before-shoulder-ownership`; Briarhide also retains the earlier mantle allocation iteration. Original `finished` and `finished-motion` folders were not overwritten.

## Integration and remaining limits

Import as fresh `SK_OE_Briarhide_Performance` and `SK_OE_MireSeer_Performance` assets/skeletons, preserving the existing material/texture names. Specs now select `output_folder: finished-performance`. Source coordinates are Blender metres, front −Y / left +X / up +Z; each bone's local +Y points head-to-tail. Use the full imported transforms for points. The previous Unreal import retained a 100× parent transform; inverse rotation alone is insufficient. The companion JSON contains complete matrices and hashes.

The strongest combined poses still stretch/compress fine cape and shoulder-boundary triangles, and deep knees lose volume. Whole hands and feet have no independent fingers or toe roll; hand pivots retain the existing palm landmarks. There is no jaw, twist chain, corrective shape, cloth, secondary frill physics or new LOD. The rendered neutral source is preserved, but dense generated/disconnected detail remains a deformation limitation.

The next useful acceptance step is the actual authored melee load → strike → recovery and caster release in engine, including the player's view and side view. Check shoulder-horn silhouette, neck/shoulder independence, hand reach, support contacts and recovery timing. No CPU ground translation or static pose proves runtime planting or game feel.

Further local work needs no purchase: a hand-authored joint cage with weight transfer or local bend-loop retopology can improve the remaining folds, while preserving the original atlas/design. Corrective shape keys would also require an Unreal import/runtime driver; the current poseable component does not automatically evaluate Blender corrective shapes. Authored motion and deliberate silhouettes are the earliest visible gain. LODs are separate performance work after motion/deformation is accepted, not a cure for stiff animation.

## Reproduction

Use the Blender executable from `config.local.json` with `--background --threads 4 --python-exit-code 1 --python games/dreambound/scripts/prepare_organic_enemy.py --`. For each asset pass `--asset Briarhide` or `--asset MireSeer`, its original `textured.glb` as `--source`, the corresponding `finished-motion/SK_OE_<Asset>.blend` as `--prepared-source`, and its new `finished-performance` directory as `--output`. `--preview-views performance-coil,performance-reach,shoulder-detail,neck-shoulder-detail` reproduces the Briarhide review; replace the final view with `throat-detail` for MireSeer. Launch through `Start-Process -WindowStyle Hidden` as in the recorded private preparation logs. `--preview-only` rerenders completed output without re-exporting it.

Only the preparation script, two rig specs and `premium-*` handoff records changed publicly. Asset provenance is unchanged original TRELLIS geometry/maps plus local Blender rig/weight modifications; root maintains the manifest and shared status.
