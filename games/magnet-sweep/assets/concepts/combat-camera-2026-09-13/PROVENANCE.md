# Combat camera storyboard — provenance and inspection

Created 13 September 2026 with built-in ImageGen. The tool does not expose an exact model version. No CLI/API fallback or third-party stock assets were used. Reference: the project's generated [board 16](../art-direction-2026-09-13/16-depth-camera.webp), with 16B preparation and 16C action now explicitly selected by Klaus.

The first generation used [PROMPT.md](PROMPT.md) exactly. It showed the three ingredients in preparation and a combined airborne projectile in Fire, but omitted the visible loading channel. It is preserved as [the discharge companion](storyboard-discharge.webp), so the actual muzzle discharge remains reviewable. A targeted edit used [REFINEMENT-PROMPT.md](REFINEMENT-PROMPT.md) and that first result to expose the joined assembly entering the cannon. [storyboard.webp](storyboard.webp) is the refined loading result, 1536 × 1024. Both lossless WebP encodings have identical decoded RGBA pixels to their generated PNGs; no cropping, resizing or content edits were made during conversion. Original PNGs remain in ignored `raw/` locally.

| File | SHA-256 |
| --- | --- |
| Reference board 16 | `59f84be422c47cf3d8e1f7376ae84a6cfbe17daab4233f91dd7ee8ffde47c95d` |
| Initial raw/storyboard-v1.png | `bd462c639b4a3946ff7559f8130843f7a300bcd5e925ffc92922e28593371e09` |
| Refined raw/storyboard-v2.png | `8e085f78bfebe70372f938d4d4371d05511dcabd9df35516c3b8d0a1e5adb346` |
| Committed storyboard.webp | `a96df2a6f2220d47583705f7c3e4ec7371dadd526e0e1431902a123dcc07871e` |
| Committed storyboard-discharge.webp | `92149418c7d59ff823eff67c0a7e92016d4f19e0f355d2d436f43fac45d2a8dd` |

## Static inspection

- Preparation preserves the lateral layout, a hanging horseshoe magnet, recognisable stacked/attracted metal and distinct body/capsule/nose parts. The parts read as ingredients of one shot. The bottom channel and Fire control are concept UI; their connection to the in-world channel still needs authored animation.
- The refined Fire panel shows those three parts joined in a physical open channel on the cannon. It catches loading/ignition before discharge; the first version instead caught discharge. A four-panel still cannot establish the timing between these beats.
- The saw robot's shoulder contact, armour fragment and recoil are visible. The tall robot remains outside that impact. The enemy-response panel shows the tall robot firing toward the rig's plate, with the source, projectile direction and struck surface all readable.
- Metal volume, furnace glow, operator clothing and enemy silhouettes remain broadly consistent. The generated cannon housing changes when the loading lid is introduced, and the damaged robot's shoulder/health depiction is not a reliable persistent state. Do not use these stills as exact asset continuity or combat-state evidence.

The result is a concept for owner review, not final runtime assets or approved mechanics. Depicted health strips, energy pips, single-target impact and plate hit specify no values, turn-end rule or defence behaviour. No gameplay or animation was tested through this image. Shared geometry and motion are assessed separately in the [camera test](../../../art-tests/camera-motion/README.md).

AI-generated concept under applicable platform terms; studio distribution licence remains unset. The reference and both generated outputs use project-created imagery. No final character, story or broader art completion is claimed.
