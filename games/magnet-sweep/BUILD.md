# Magnet Sweep — build and run

No engine project or playable build exists. No build or run command has been executed for this game.

## Read-only environment evidence — 2026-09-11

The configured Unreal Editor executable exists. Its Engine/Build/Build.version reports **5.8.2**. Build.bat and RunUAT.bat exist. The studio has an existing game build script showing an editor/content/package workflow, but that is not proof a new Magnet Sweep project compiles or packages.

`python scripts/studio.py doctor` reports that tools must map names to executable path strings. Read-only inspection found a nested ai3d metadata entry alongside the Unreal and Blender executable strings. This is a configuration/schema mismatch, not evidence Unreal is absent. Private configuration was not changed. Keep machine paths in ignored config.local.json.

## Next implementation evidence required

Before extending content, create the smallest isolated project under this game and verify a development build and standalone Windows launch. Record exact commands, engine and compiler versions, commit/build identity and output location here. Packaging approach may reuse the studio's Build.bat/RunUAT pattern; do not import the parked FPS or its assets into this game.

The production plan must verify direct manipulation, context-sensitive input, finite corridor selection, direct linked release, persistent collected state, a complete delivery and retained tool progression. The old timed/capacity/source-return interaction is superseded. None is verified yet. Shipping performance and hardware minimums remain unmeasured.
