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

## Initial Editor assessment and limits

The targeted visual correction is supported by this image; no additional actionable contact defect was found in the reviewed view. This is one angle and one seed in an Editor-game capture, not verification of the updated Shipping artifact. Collision, walking onto/off the stairs, other landings and seeds, hidden contact surfaces, guard operation, audio, and overall play quality were not tested. The existing native 0.4.0 demo was left untouched.

## Packaged follow-up inspection

The integration owner produced the updated Windows Shipping package from `ba0178e133f98ecbe9912743a8bb23fb9cd242c5`. This reviewer independently opened both actual packaged captures, each 1600 × 1000 with expedition 1 / seed 552389 visible:

- `evidence/reverie-update-stairs.png`, file timestamp 2026-09-10 20:37:06 +02:00; SHA-256 `F6C51F2466909C509A880B3AA3C211119F9719864932879C5E91A64E6ACBD8CB`.
- `evidence/reverie-update-combat-view.png`, file timestamp 2026-09-10 20:39:36 +02:00; SHA-256 `F3014CF6EE4FB8A1B0B9825AB2B4B9FAE38E4B7AE322C2CC7DB6A7715B675B56`.

**Visual result:** no concrete floating stair/landing or restored-combat-art defect is apparent in these views. The packaged stair capture retains the continuous apron, stair bedding and ground-reaching landing retaining face seen in the Editor correction. The combat capture clearly shows the preferred bronze/cream guardian standing on the courtyard paving and the cream shield plates on their dark mechanical frame. The visible world remains the Reverie environment. These stills support asset loading and the visible contact correction, without proving motion or interaction behavior.

**Root-executed check, independently read:** `evidence/reverie-update-art-checks.json` reports complete=true, aborted=false, 2 passed groups, 11 passed observations and 0 failures in Windows Shipping / UE 5.8.2-56702186, NullRHI, isolated slot `DreamboundQA_ReverieUpdate`, seed 833283. It includes 16/16 physical tread probes with successive 20cm rises, connected floors/gate lanes, functional arch alignment and preserved approach pads. SHA-256: `95FC9DD3EC27001D88F3457DB93D6BF2BFCAE9FE8B82F6767C0035F13DF9E2E9`. Execution belongs to the integration owner; this reviewer did not rerun it. Its seed differs from the captures and its scope excludes rendered graphics, ordinary play, combat, audio and performance.

No additional actionable defect was found in this bounded packaged-image review. Native traversal, shield unfolding/combat motion, other angles/seeds, audio quality and owner acceptance remain unverified here. No native input, render, launch or tests were performed for this follow-up; the owner's running demo was left untouched.
