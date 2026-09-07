# Sixfold Recoil

**Parked after the 2026-09-07 owner playtest.** Klaus found the loop boring, too narrow and visually below the studio target. The build and technical evidence are preserved; [STATUS.md](STATUS.md) records the decision. The instructions below describe the archived experiment.

A one-level combat beta for Klaus's playtest. Pip's six pieces are both protection and ammunition: throw, dodge, recall, repeat. Clear three groups in the Open Tray, then defeat the Foreman.

## Play

Open `build/Sixfold-Recoil-Beta.html` in a desktop browser. It is a single offline file; no engine installation, account or runtime download. For a localhost preview, see [BUILD.md](BUILD.md).

| Action | Keyboard / mouse | Standard gamepad |
| --- | --- | --- |
| Move | WASD or arrow keys | Left stick |
| Aim | Mouse | Right stick |
| Throw all held pieces | Left click, once per volley | Right trigger, once per volley |
| Recall | Hold right mouse | Hold left trigger |
| Pause / resume | Escape | Menu / Start |
| Fullscreen | F or top-right button | — |
| Menus | Mouse, Tab, Enter / Space | D-pad / left stick, A selects, B returns |

The game pauses when focus changes. Resume explicitly and release/repress controls. Physical gamepad testing is still outstanding; keyboard and mouse are the primary playtest input.

**Recover** is the default: fast, harmless returns. **Cut through** gives slower returns that can damage an enemy once per launched piece. In Settings, an optional next-attempt comparison lets a throw interrupt recall. Otherwise release recall before clicking to throw. Music, effects and shake can be adjusted separately.

Three hull points persist through the level. Attached pieces block hits; throwing everything leaves Pip exposed. Safe breaks gather all six pieces. Winning or losing offers immediate retry. Best clears/settings remain in this browser where storage is available; each recall/input variant has a separate record. There is no network telemetry. Results can be saved as a local JSON note.

## What this playtest decides

Try the default first, then the other recall style if you want to compare. There is no required session length.

- What felt satisfying, and what felt awkward or like waiting?
- Was danger readable, and did you understand when you were protected?
- Did cutting recall change how you moved, or mainly make enemies easier?
- After a clear, did you want another attempt?

This is a focused first playable beta. Campaign progression, upgrades and other levels are deferred until this feedback; this level's duration does not define the eventual game's length. Automated completion verifies that the level can end, not that it is enjoyable or long enough.

[Current status](STATUS.md) · [Build instructions](BUILD.md) · [QA evidence](QA.md) · [Brief](BRIEF.md) · [Decisions](DECISIONS.md) · [Asset origins](assets/manifest.csv)
