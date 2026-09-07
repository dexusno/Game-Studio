# Sixfold Recoil — build and run

Canvas 2D / Web Audio. No external JavaScript libraries, web fonts, media downloads or engine installation are used at runtime. Sources are ordinary JavaScript/CSS/HTML. The assembly script uses Node's standard library.

## Build and launch

From the repository root:

```powershell
node games/scrapstorm/scripts/build.mjs
```

Outputs `games/scrapstorm/build/Sixfold-Recoil-Beta.html` and `build-info.json`. Double-click the HTML and open it with a desktop browser. The package also works without a network connection. Generated builds are ignored by Git; rebuild from sources when cloning.

Optional preview:

```powershell
python games/scrapstorm/scripts/serve.py
```

Then open `http://127.0.0.1:8786/Sixfold-Recoil-Beta.html`. The server listens only on loopback, serves the build folder and disables caching. Use `--port NUMBER` if 8786 is already occupied. Stop that server with Ctrl+C. It is only a local convenience; the standalone file needs no server.

For source development, serve the game directory or open `index.html`; assembly is required before checking the owner package. The home screen shows the beta version. The readonly `window.__sixfold.snapshot()` exposes the complete build fingerprint, simulation summary and runtime fault list. `?qa=1` additionally exposes test control hooks, mutes audio and disables storage writes; owner play should omit it.

## Checks

```powershell
node --test games/scrapstorm/tests/core.test.cjs games/scrapstorm/tests/audio.test.cjs
python scripts/studio.py validate
```

The optional native audio and packaged browser tests use an existing Playwright installation and Chromium executable supplied through `SIXFOLD_PLAYWRIGHT` and `SIXFOLD_CHROMIUM` environment variables. Keep machine-specific paths in ignored `config.local.json` or the local shell, not this document. No such tooling is needed to play. See [QA.md](QA.md) and the test files for the exact browser check command and coverage.

## Build evidence

2026-09-07: Windows, Node v24.13.0, Python 3.11.9, headless Chrome 143.0.7499.170 (user agent reports 143.0.0.0). `node games/scrapstorm/scripts/build.mjs` produced a self-contained package; the source SHA-256 and byte count are recorded in its generated `build-info.json`. Source fingerprinting covers HTML, CSS, simulation, audio, renderer and app integration. This working tree is uncommitted; the fingerprint identifies the actual artifact instead of inventing a commit.

Actual package launch, input, loss/retry and presentation checks are recorded in [QA.md](QA.md). Parent visual inspection covered 1280 × 720 and 1920 × 1080; local raw captures are ignored. No visible browser or editor took the owner's keyboard/mouse focus. Audio testing used offline rendering / muted headless playback; physical listening and a real controller remain unverified.
