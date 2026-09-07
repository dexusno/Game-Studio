# Sixfold Recoil — beta verification

**Owner outcome, 2026-09-07: current concept rejected and parked.** The technical checks below passed, but the owner found the demo boring and below the required gameplay/presentation scope. See the final section for reported playtest feedback.

**Technical record before the owner playtest, 2026-09-07.** Independent packaged-browser QA found no unresolved blocker in the tested flows and judged the artifact ready for testing. At that point enjoyment, human completion time and replay motivation were unverified. QA edited only this report and its browser harness. The later owner outcome above supersedes that delivery status.

## Exact artifact and environment

- Version `0.1.0-beta`, source/build identity `467e2a4ca6c3`.
- Full source digest: `467e2a4ca6c3e1b039f0d04ad9a5fe262b83ed2252005d5d46937cff1fd9d55f`.
- Artifact: [build/Sixfold-Recoil-Beta.html](build/Sixfold-Recoil-Beta.html), 110,351 bytes.
- Artifact SHA-256: `cf126ba772959b89eade857e41abd26cfe57c54a715a9dbeef59f38d1d35d0e9`.
- Windows x64; Node `24.13.0`; Playwright `1.62.1`; Chrome `143.0.7499.170`, headless with `--mute-audio`. Separate temporary browser contexts; no desktop input or speaker playback.
- Actual HTML executed offline through `file://` and an isolated `127.0.0.1` server. Existing toolchain; no downloads.
- Inputs: WASD/arrows, mouse aim/left throw/held right recall, Escape, keyboard menus and settings. Controller axes/triggers/Menu/disconnect were simulated.

## Reproduce

Follow [BUILD.md](BUILD.md); set `SIXFOLD_PLAYWRIGHT` to the installed module directory and `SIXFOLD_CHROMIUM` to Chrome/Chromium. `SIXFOLD_CHROME` is also accepted.

```powershell
node games/scrapstorm/tests/browser.test.cjs
```

The harness owns/cleans its browser and server, prints JSON evidence and fails nonzero. Optional `SIXFOLD_TEST_FILTER` selects cases. Final evidence: ten suites at 09:11 UTC plus the added full-level traversal at 09:14 UTC; unchanged runtime between runs.

## Passed — actual packaged browser execution

| Check | Evidence and coverage |
| --- | --- |
| Offline startup and ordinary controls | Production file launch; zero network requests. Keyboard start/movement, one throw per held press, six pieces launch/land/return/attach. |
| Mouse combinations | Both button orders work; default recall rejects throw. Optional interrupt throws, then requires fresh recall. |
| Pause, focus and menu exit | Pause/settings freeze time. Held keys, OS repeat and recall remain cleared after resume; fresh input works. Synthetic blur pauses. Menu/new start resets state. |
| Settings | Volumes, shake, recall and input options persist. Next-attempt input changes apply on restart. |
| Save failure | Corrupt JSON and denied storage retain playable defaults; failed saving is disclosed. |
| Missing audio API | Deliberately unavailable Web Audio leaves gameplay/settings usable without errors. |
| Natural defeat/retry/export | No-input loss at 13.45 active seconds: six blocks/three hull hits. Correct results/export; retry restores hull/pieces/stats. |
| Victory records — fixture | Constructed terminal states exercise results and separate persistent records for harmless/cutting recall. Fixture times are not observations. |
| Resize/fullscreen | Arena/Start fit 1920×1080, 1280×720, 1024×768 and 800×600; no horizontal overflow. Resized aim aligns. Headless fullscreen accepted. |
| Simulated controller | Movement/aim, edge-trigger throw, Menu, held-trigger resume safety and disconnect pause work. |
| Natural full-level traversal | Virtual standard controller reads snapshots and supplies axes/triggers; no direct simulation calls or state/health writes. Harmless clear: 25.30 active seconds, three hull, 19 kills, 34 volleys. All groups and Foreman fork observed; real victory screen, six held pieces, no enemies/shots/spawns remaining. Engineer supplied the strategy; QA independently exercised the browser input path. |

All eleven suites passed with no browser/runtime errors. The full-level controller has exact state knowledge and fast reactions; **25.30 seconds is not a human playtime or enjoyment estimate**.

## Findings fixed and independently rechecked

| Finding | Severity and affected build | Reproduction, expected/actual and evidence | Fix and retest |
| --- | --- | --- | --- |
| Second mouse button was ignored | Medium; `2823b1130f11`; `src/app.js` | Start, throw, hold right mouse until recovered, then click left without releasing right. Expected default rejection or an interrupt throw when that setting is enabled. Actual: no action. Chrome emitted `pointermove` with button 0/buttons 3 for the second press; the wrapper only handled `pointerdown`. Holding left and then pressing right also missed recall. | Engineer changed mouse button-edge listeners to `mousedown`/`mouseup`. Both orders, default rejection, interrupt mode and fresh-recall behavior passed on `467e2a4ca6c3`. |
| Held movement resumed through key repeat | Medium; `cd040c0d3cf2`; `src/app.js` | Hold D, pause/resume without releasing it, then deliver the OS's repeated keydown. Expected no movement until a fresh press. Actual: an observed repeat moved Pip from x=6.75 to x=8.25 over 300 ms. | Engineer ignored repeated movement keydowns. Held key plus repeated events now stay stopped, while release/new press moves; passed on `467e2a4ca6c3`. |

Regressions: [tests/browser.test.cjs](tests/browser.test.cjs).

## Other evidence and limits

The integration owner separately reported 33 core/audio checks passing, including native waveforms, and inspected arena/Foreman renders at 720p/1080p. These are attributed owner results, not additional independent passes; commands are in [BUILD.md](BUILD.md).

Not run: physical input latency/capture, controller hardware/hot-plug, display scaling/fullscreen, audible mix, other browsers/devices, long soak, low-end performance or human accessibility. Focus events were synthetic. Geometry/headless checks cannot establish human readability, feel or shipping performance.

## Human playtest — owner report received

2026-09-07, after delivery of beta `467e2a4ca6c3`: Klaus reported playing the demo and finding it boring. He specifically identified absent powerups/progression, a single all-at-once attack, repetitive move/shoot/recall, little strategy and a concept too narrow for the studio. He compared the presentation unfavorably with Breakout and requested better graphics using Unreal and proper asset/model workflows.

Source: direct conversation feedback. No session recording or run export was supplied; exact duration, outcome, recall mode, device and voluntary retry count are unknown. This is a negative owner assessment of this experiment, not proof that every retrieval mechanic or top-down game is inherently unfun. The designer's explanation—that the demo removed too much decision/progression depth—is an interpretation, not something the automated tests establish.

Decision: park this concept, preserve the evidence, and research substantially more ambitious first-/third-person Unreal directions. The owner clarified that "ten times the complexity" was a general indication, not a hard multiplier. Future owner-facing slices must include the meaningful actions, choices, rewards and presentation they are supposed to test.
