# Reverie ground-contact and preferred-combat update QA

Date: 2026-09-10. Independent review following the owner's request to restore the previous cyborg/weapon appearance and correct floating world geometry. The earlier `reverie-qa-notes.md` remains the historical 0.4.0 report.

## Artifact and scope

- Actual Editor-game staged detail: `.local/reverie-review/editor-grounded-stairs.png`, 1600 × 1000, Moss Terraces, expedition 1, seed 552389; file timestamp 2026-09-10 20:29:16.760 +02:00.
- Image SHA-256: `4C21F284DF585B9D2970B3970231505081A04A1A41CB3B3D35F4B4B6EA8A3608`.
- Source base: `f48abd09422da16a605a76a59f20e8983926fea2` plus the current uncommitted scene/PreferredCombat changes. Reviewed the relevant `unreal/Source/Dreambound/DBRecoveryScene.cpp` diff; file SHA-256 at review: `E68641CA660926DBB41881F3537934F7B7F758C5F0A393724637B210906D59D1`.
- Windows; read-only image and source inspection. The integration owner reported successful Editor compilation. This reviewer did not execute a build, game, GPU render, native input, or test suite for this update.

## Observed result

No concrete floating stair or landing defect remains visible in this angle. The bottom riser meets the paved apron, the stair sides have continuous masonry bedding, and the landing's visible retaining face reaches the ground. Dark bands beneath the paving read as masonry bases and shadow; no open daylight gap is apparent.

The source changes support that result: buried bedding under exposed paving and stairs, an approach apron, and retaining faces on all four landing sides preserve the intended stair/landing tops. Basin/rill supports and the embedded ward plinth are present in the diff, but are not independently established by this stair image.

The preferred solid cream shield plates and dark mechanical frame are clearly visible at the lower left. The complete arm, unfolded guard, and enemy visuals are outside this image's coverage.

## Assessment and limits

The targeted visual correction is supported by this image; no additional actionable contact defect was found in the reviewed view. This is one angle and one seed in an Editor-game capture, not verification of the updated Shipping artifact. Collision, walking onto/off the stairs, other landings and seeds, hidden contact surfaces, guard operation, audio, and overall play quality were not tested. The existing native 0.4.0 demo was left untouched.
