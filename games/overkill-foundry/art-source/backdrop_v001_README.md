# Cinderwall distant city v001

Original environment raster generated on 20 September 2026 with the built-in
OpenAI image_gen tool. The exact prompt is [backdrop_v001_prompt.txt](backdrop_v001_prompt.txt).
No input image, external asset, reference-game pixels or geometry were used.
The unchanged 2172 × 724 PNG and its identity/provenance are in
`assets/production/cinderwall-backdrop-v001/`.

This is a distant matte for the selected controlled 2.5D presentation. It is
not a screenshot, human-approved art or a replacement for the dimensional
foreground. Original 3D scenery and playable actors remain responsible for
nearby contact, depth and shadows.

`backdrop_v001_build.py` creates an original fixed curved card with 120 triangles,
200 m radius and a 124-degree horizontal span covering both authored cameras.
The card remains in world space while the camera moves. The generator does not
edit the image. It actually exports and reimports the FBX, checking its vertex,
face, UV and world-bound inventory. Blender 5.2.1 LTS passed those checks.

```powershell
& D:/Blender/blender.exe --background --python-exit-code 1 --python games/overkill-foundry/art-source/backdrop_v001_build.py -- --output .local/overkill-foundry/art/backdrop-v001
```

Large editable blend/FBX and logs remain in that ignored output directory.
`unreal/Tools/import_backdrop_v001.py` imports the unchanged PNG, an unlit material
and the card, then adds its own non-colliding, non-shadowing tagged actor to the
host map. Run the importer only with the game closed. It does not reposition
existing geometry or change combat rules.

Current state: source image inspected and mesh interchange verified. Unreal
import, source/render-bound checks, camera continuity and visual acceptance are
still pending. The scene's camera height, foreground framing, close factories,
lighting and material finish must be checked together in the actual game.

Origin/licence: AI-generated art under applicable platform terms; original
Blender support geometry and project import code. Studio distribution licence
remains unset. No outside art credit requirement was introduced by this asset.
