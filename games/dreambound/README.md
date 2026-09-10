# Cyborg between worlds — beta

First-person Unreal 5.8.2 prototype. Internal identifier `dreambound`; final title undecided.

The current playable build is **0.4.0-reverie**: three connected procedural sanctuaries, a six-piece folding shield, three encounters and two earned upgrade choices. The graphics package replaces the environment, shield/arms, enemies and effects with new Blender assets and three new textured TRELLIS landmarks. It also adds 23 replacement sound cues and persistent sound controls. Owner assessment of the art and combat feel remains pending. Read [STATUS.md](STATUS.md).

Start `BuildOutput/Reverie/Windows/Dreambound.exe`, or run `scripts/Start-Demo.ps1`. [Controls](PLAY.md), [actual packaged view](evidence/reverie-courtyard.png), [build evidence](evidence/reverie-build.json). Historical packages remain in their original directories. Generated Unreal Content and packages are ignored and reproduced with the included scripts.

- [Brief and player experience](BRIEF.md)
- [Current state and resume instructions](STATUS.md)
- [Decision history](DECISIONS.md)
- [Build and run instructions](BUILD.md)
- [Quality and playtest record](QA.md)
- [Release preparation](RELEASE.md)
- [Asset licensing register](assets/manifest.csv)

Implementation lives in `unreal/Source/`. Art sources and reproducible imports live in `art/` and `scripts/`. The previous Sixfold Recoil browser prototype remains parked.
