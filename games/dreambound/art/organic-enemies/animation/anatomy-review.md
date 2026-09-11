# Organic anatomy candidate — 11 September 2026

Both existing creatures now have an actual opening jaw, a lined mouth with separate teeth and tongue geometry, and two deforming segments on every existing hand digit. MireSeer also has two weighted stages on each head frill. These are frozen Blender/FBX candidates for authored engine motion review. This review does **not** establish Monster Fantasy animation parity.

The source designs were inspected from front, side and oblique hand views before articulation. MireSeer's opposing thumb was confirmed in the oblique source view; the rejected three-digit candidate is retained privately. No new reference image or generated creature was introduced. The original `finished-performance` folders remain unchanged.

## Frozen files and integration

The full signed bone/angle table, materials, source hashes, exact preview identities and import paths are in [anatomy-contract.json](anatomy-contract.json). Portable copies of the complete preparation evidence are [Briarhide](anatomy-Briarhide-prep.json) and [MireSeer](anatomy-MireSeer-prep.json). The private output directories are workspace-relative:

| Candidate | Directory under `.local/ai3d/outputs/organic-enemies/` | FBX SHA256 | Bones / triangles |
|---|---|---|---|
| Briarhide | `briarhide/finished-anatomy/` | `ea4cd13bbb5d13683be8cf71a57b8c9f67b2a58459bd104aad903a2d14514806` | 38 / 190,536 |
| MireSeer | `mire-seer/finished-anatomy/` | `6f7c7d18b151f72e714c0b4064cafac91cea3eeeb1a7a23030aeee56546ba811` | 43 / 170,448 |

Each directory contains `SK_OE_<Asset>.fbx`, the neutral editable `SK_OE_<Asset>.blend`, copied original 4K maps and `prep-report.json`. Import fresh `SK_OE_Briarhide_Anatomy` and `SK_OE_MireSeer_Anatomy` skeleton/mesh families. Do not replace the preserved Performance skeleton. Root owns imports, runtime, manifests and acceptance records.

Both final candidates use preparation script SHA256 `cd1fa43b64a220cbd3f76c17f34ba944b772b0a7d1cdeacba5fa3b028f713400`, which applies the species-specific material definitions in the rig specs. Briarhide's earlier producing script, hash `a3e8634a2096c334f85889c59ea5442c63754ba6ed68792ce0431e3c9e7c0111`, and its pre-adjustment FBX/blend/spec/report/images are retained in the private `iteration-04-before-engine-oral-response` directory. The final Briar change is only the proven oral response recorded below; the source script already supports that spec override. The existing `prepare_organic_enemy.py` was not edited; its texture extraction helper is reused read-only.

## Anatomy and control contract

All 21 Briarhide and 22 MireSeer baseline bone names, parent links and global reference matrices remain exact. New controls are children of existing hand/head bones:

- `jaw` under `head`: Blender mesh +X rotation, maximum 18° Briarhide / 14° MireSeer.
- `digit_01_l/r`, `digit_02_l/r`, `digit_03_l/r`, `thumb_l/r` under the corresponding `hand_l/r`, each with a child ending `_tip_l/r`. Every original hand digit is articulated; none is newly invented geometry.
- Mire only: `frill_l/r` under `head`, each with `frill_tip_l/r`. The central face and eyes retain their head attachment. Antlers remain rigid with the head.

`Jaw` and each side's `Grip` range from 0 to 1. `Crest` ranges from 0 to 1 for Mire only. Use the per-bone **signed mesh-axis** rotation vectors in the JSON; compose X then Y then Z through the cached reference basis. Do not infer local hinge axes from bone names. Source coordinates are Blender metres, front −Y, left +X, up +Z. Imported positional conversion requires full transforms: earlier Unreal imports retained a 100× parent scale. Runtime timing, distal overlap and blended authored poses are integration responsibilities.

## What the inspected CPU views establish

The initial Briarhide views covered closed/half/open front jaws, open profile, both full grips and the combined bent-wrist cup. After the material-only revision, open front/profile views were rendered again; the earlier views remain in the comparison archive, with exact geometry and skin identity verified against the revision. Mire adds half grip and frill rest/full views. These are Cycles **CPU** renders at 700×700; the current preparation's PNG hashes are retained in the contract. The `crest-full` view also uses Jaw 0.7 and Grip 0.35; it is a combined expression stress pose, not an isolated crest comparison. Separate numeric displacement checks isolate each channel.

| Criterion | Observed result and limit |
|---|---|
| Physical mouth opening | The original lip surface is split at an actual seam. Upper palate, raised mandibular floor, rear/cheek lining, tongue volume and upper/lower teeth are visible when opened. No central solid-head seal remains in the inspected final views. Closed pose retains a 1 mm seam. |
| Useful hand articulation | Both claws/fingers and opposing thumbs curl relative to their palms. The inspected full grips and combined wrist poses remain connected, including Mire webbing. This is two-segment articulation, with no independent finger spread channel. |
| Frill deformation | Mire's weighted frill tips change silhouette relative to the head without detached membranes or rigid antler motion. These static images establish useful deformation, not the quality of runtime lag or settling. |
| Original surface continuity | Original body PBR and Briarhide eye slots remain. Original packed map bytes match both baseline and final copies. Local new seam vertices interpolate the original UVs; added oral geometry uses new named materials. |
| Neutral physical endpoints | Exact source/candidate position hashes match for 4,979 Briarhide and 4,149 Mire contact-region vertices. Jaw/Grip/Crest at zero moves no vertex by more than 1 mm. Boss claw-contact positions at Grip 0 are therefore preserved in source space. |

The maximum isolated source-space displacements are Briarhide Jaw 7.693 cm / Grip 15.402 cm, and Mire Jaw 5.693 cm / Grip 12.722 cm / Crest 6.623 cm. Every new control has vertices with more than 25% ownership. All vertices retain at most four normalized influences. These measurements establish that the new bones deform anatomy; they do not grade the resulting animation.

The first Mire oral material produced a broad pale specular patch in profile. A CPU ray query identified the new lining surface, and the same camera comparison showed the patch removed by oral roughness **0.68** and specular **0.12**, with base color unchanged at `[0.020, 0.006, 0.005, 1]`. This tested override is in the Mire spec and final prep report. Root namespaces imported extra materials per species so it does not change Briarhide. Teeth and tongue are unchanged. Rejected mouth-seal, plain-floor and specular views remain in private iteration directories.

## Remaining visual and technical limits

The original dense generated triangular skin and four-influence linear blend skinning remain. Earlier deep-knee compression and extreme limb bend limits are not solved by the new facial/digit controls. The mouth is a single hinge with a simple modeled cheek membrane and mucosa, no lip or eyelid rig, and a tongue that follows the jaw. At close profile angles the lining still has a simpler shape and material than the detailed external skin. This must be judged at actual engine camera distances; it is not a finished facial performance system.

No animation clips, retopology or LOD pass were added. Frill overlap, eye focus, jaw rhythm, grip timing, transitions and contact under combined nonzero channels require actual temporal engine review. Static endpoint preservation does not prove floor contacts at Grip > 0. No Unreal process, import, game capture, build or packaging was run by this anatomy worker.

## First integrated Melee review — candidate C766E18B

Root imported the exact frozen FBXs and captured Editor DLL SHA256 `C766E18B1F690AEEBA46DF51CE401B2D1C5E1270D5ABDC599F36692242A4C9F8`. The completed `.local/anatomy-v2-melee-side/Saved/EnemyMotionStudy/Melee-Side` has 961 actual frames / 32.033188 simulation seconds, and `.local/anatomy-v2-melee-detail/Saved/EnemyMotionStudy/Melee-Detail` has 241 / 8.033327 seconds. Both report zero missed readbacks. This is an anatomy review of ordered native PNGs; no continuous-playback acceptance is claimed.

Side coverage: 150, 154, 158, 162, 166, 170, 172, 174, 186, 209, 604, 615, 844, 874 and 920. Detail coverage: 20, 130, 140, 154, 158, 160, 162, 163, 166, 170, 174 and 188. The first full attack is Telegraph 130–156, Attack 157–173 and Recovery 174–209.

- **Mouth appearance requires correction.** In Detail 140/154/158/160, a broad, nearly flat blue-grey strip fills the opening beneath the upper snout. It does not reproduce the intended dark mucosa, separate teeth and tongue seen in the frozen CPU close-ups. The defect is visible across attack preparation, not just one ambiguous full-body pixel. The orange/cyan patch below it is the separate existing chest tell. The underlying cause remains under diagnosis; no new FBX was exported in response.
- **Jaw direction and closure:** the visible opening develops below the upper snout and reduces through recovery; there is no observed detached jaw. A CPU-only positive/negative hinge comparison confirms that the supplied positive source rotation opens downward, while its negative closes/intersects upward. The current evidence does not justify reversing runtime hinge signs. The mouth-appearance defect prevents accepting the integrated cavity as complete.
- **Claws and transition continuity:** changing curl is visible at the raised hand during load and at the extended/recovering hands. Sampled adjacent poses show no detached digits or gross stretched web/palm connection. Detail framing crops some low hands; complete fingertip/ground contact is not established by this view.

A read-only Blender round trip of the frozen Briar FBX exited 0 and retained exact new-surface material-group triangle counts and bounds: Oral 342, Tooth 192 and Tongue 436, with no zero geometric normals in those groups. This rules out missing exported oral geometry at that level; it does not verify Unreal section-material evaluation. One degenerate original body triangle is discarded by Blender re-import, unrelated to the mouth. Private diagnostic images and `fbx-surface-correspondence.json` remain beside the frozen Briar files.

The initial oral-specular hypothesis was **ruled out for the v2 defect** by root's validated BaseColor capture, `.local/anatomy-v2-melee-basecolor-valid/Saved/EnemyMotionStudy/Melee-Detail/Frame_00158.png`, inspected at native size. Its invocation uses `viewmode VisualizeBuffer,r.BufferVisualizationTarget BaseColor`; the mouth strip is uniformly neutral grey before lighting. The earlier similarly named capture used an unsupported command and is excluded. Root then confirmed null oral/tooth/tongue interfaces in a fresh Editor read. The importer had modified copied material-slot structs without assigning them back into the array. Root corrected array writeback and forced package save; a second fresh process confirmed all six Briar/four Mire slot interfaces persisted. Before/after and persisted evidence are `.local/anatomy-material-binding.json` and `.local/anatomy-material-persisted.json`. No geometry or roughness change was made to compensate for this confirmed binding defect.

The completed Caster Side capture in `.local/anatomy-v2-caster-side/Saved/EnemyMotionStudy/Caster-Side` was also sampled at 30/46/52/62 after its `Capture.json` reported 961 frames and zero misses. The hands cup/release with connected webbing and the frill remains attached. At that camera distance the mouth is too small and dark to classify the new lining. Mire Detail remains the required next anatomical close-up; these full-body samples are not a cavity or temporal-overlap acceptance.

## Corrected material-binding review — v3

The completed v3 Melee and Caster Detail captures both use Editor DLL SHA256 `54C8F094DA7B90B0B95482C42ECA2863AE3C54C4328460A8DEB7B314F76530E0`, with 241 actual frames / 8.033327 simulation seconds and zero missed readbacks each. Material persistence was verified independently of these pictures. Paths are `.local/anatomy-v3-melee-detail/Saved/EnemyMotionStudy/Melee-Detail` and `.local/anatomy-v3-caster-detail/Saved/EnemyMotionStudy/Caster-Detail`.

In native Melee 140/154/158/160, separate teeth and a graded oral surface appeared: the default-material failure was visibly corrected. The oral floor still presented a broad cool pale response that competed with the cavity details. The proposed **isolated material A/B** was `M_OE_Oral_Briarhide` roughness **0.68**, specular **0.12**, keeping base color `[0.020,0.006,0.005,1]`, metallic 0 and two-sided true. Tooth and Tongue stayed unchanged to preserve their smaller highlights. These are the already source-tested Mire mucosa values. This proposal addressed the residual lit appearance after binding was fixed; it did not recast the confirmed v2 binding defect as a reflection problem. No rig or geometry change was warranted. The later successful A/B and source integration are recorded below.

Mire Detail coverage is 14/30/46/49/52/59/62/69/72/85/104/211/219/235, reviewed only after `Capture.json` reported complete. Its finger/thumb cupping and fold remain connected to the palm/webbing through gather, discharge and recovery. Frill and sac attachments remain joined as the head lowers and the silhouette settles. The jaw returns toward the closed line without a detached lining or the Briar-like pale floor. **No new Mire asset correction is required from this sequence.** The elevated camera hides much of the internal floor; this is an articulation and appearance decision for the reviewed angle, not an acceptance of every interior close-up. Ordered frames establish continuity of these shapes, while exact tip lag and premium performance timing still require the full temporal review. Mire geometry and materials remain unchanged.

## Proven oral response integrated into source

Root's completed `.local/anatomy-v3-melee-oral-trial/Saved/EnemyMotionStudy/Melee-Detail` A/B has 241 actual frames / 8.033327 seconds with zero missed readbacks. Native 140/154/158/160 were inspected: the broad blue floor is removed and small teeth remain distinct against a dark brown cavity. The exact trial definition is `.local/anatomy-oral-trial.json`. **The isolated oral appearance correction is accepted for this reviewed view.** It is separate from the earlier null-material binding failure and is not a claim of full animation parity.

The tested roughness **0.68** / specular **0.12** override is now in `rig/Briarhide.json`. A CPU re-export through the existing preparation script completed with exit code 0, followed by a before/after preservation check with exit code 0. The resulting FBX changed from `4a0adfced934a07bc157efac37047344976f9499a16ec854f61d29c9b4d8ccd0` to `ea4cd13bbb5d13683be8cf71a57b8c9f67b2a58459bd104aad903a2d14514806`; its editable blend changed to `58699dbbe83a8a2604a6649dc9cdc12501ae809d08f26fa1592b8e0665244a63`. These container changes carry the new material response. The full indexed mesh/UV/color signature, all skin weights, corner normals and all 38 reference bone matrices are **exact matches**. The Base Color, Roughness, Metallic and Specular inputs queried for every other material also match. No geometry, UV, eye, texture, channel or rest-pose correction was made. Mire files remain unchanged.

[Material revision evidence](anatomy-Briarhide-material-revision.json) retains the old/new hashes, matching content signatures, tested definition and exact engine frame hashes. The former FBX/blend and source evidence remain in `briarhide/finished-anatomy/iteration-04-before-engine-oral-response`. New CPU open-front/profile images were inspected after export. Root already verified the matching material response in-engine; import of the new reproducible source container is root-owned and waits for its active capture to complete.

## v4 idle and Boss attachment spot-check

The completed `.local/anatomy-v4-melee-idle/Saved/EnemyMotionStudy/Melee-Side` has 241 frames / 8.033327 seconds, all in the passive Dormant state. The completed `.local/anatomy-v4-boss-side/Saved/EnemyMotionStudy/Boss-Side` has 961 frames / 32.033188 seconds. Both report zero missed readbacks and Editor DLL SHA256 `22393FBCA1166884ECAF77D2BD5F24A8C081D1B8A7DC82845425460A4992C89A`. The engine uses the already reviewed oral response; this spot-check does not claim that the later source FBX has been reimported.

Idle native frames 0/60/120/180/240 show the side jaw silhouette and hands remaining joined through small pose changes. Boss native frames 440/456/462/466/470/488/526/615/850/875/905 cover Slam load, release and recovery, plus stagger and collapse. Visible claw folds remain connected to the wrist/palm, with no new severed digit or gross stretched hand web in those samples. **No additional anatomy asset correction is required from this coverage.** The Boss Side camera looks from behind and hides most of the mouth and planted palm; it does not establish internal mouth appearance or exact claw-floor contact. These are ordered native-frame attachment observations, not continuous-playback or overall performance acceptance. The frozen assets remain unchanged, and Monster Fantasy animation parity remains unaccepted pending the full performance review.

## Reproduction and next review

Use the configured local Blender in background mode with `--threads 4 --python-exit-code 1 --python games/dreambound/scripts/refine_organic_anatomy.py -- --asset Briarhide` or `--asset MireSeer`. The script resolves the private output root from `config.local.json`, opens the immutable Performance `.blend`, and writes `finished-anatomy`. Full per-asset executed view arguments are recorded in the contract. The verified installed renderer was Blender 5.2.1 LTS, using Cycles CPU. Both final preparation process exit codes were observed as **0**. Briar's earlier pre-adjustment process emitted `ANATOMY_PREPARED` and completed its views, but that original process exit code was not retained.

The final focused record check parsed both specs/reports, matched FBX/blend/spec/texture hashes, compared every original reference matrix and parent, and checked meaningful new weights and neutral contact evidence. Root's next useful check is the actual candidate detail view during idle, gathering, discharge/claw release, recoil and collapse: verify mouth side walls, upper sac attachment, curled thumbs/webbing, frill tips and existing contact constraints through dense adjacent frames and continuous playback.

[Proposed manifest rows](anatomy-manifest-rows.csv) cover the two modified asset families and editable source. Provenance remains the original OpenAI-generated references and local TRELLIS.2 preparation plus original Blender geometry/rig work. Existing dependency terms and `local-evaluation-only` status are retained; this increment makes no new redistribution-rights assertion.
