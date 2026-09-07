# Tooling

The studio helper uses Python 3.11+ and the standard library. Git tracks source and documents. Engine tools are selected per game; no engine, model subscription, or paid data service is installed by this scaffold.

Run from the repository root:

```powershell
python scripts/studio.py --help
python scripts/studio.py doctor
python scripts/studio.py status
python scripts/studio.py context
python scripts/studio.py context opportunity:reactor-raider
python scripts/studio.py validate
python -m unittest discover -s tests -v
```

`validate` checks studio records and configuration, not game runtime behavior. `doctor` separates required tools from optional engine tools. CI performs offline studio validation and tests; game-specific build workflows can be added when there is a real game to build.

`context` is a read-only discovery/resume command for both game manifests and research candidates. Use qualified targets such as `game:scrapstorm` or `opportunity:reactor-raider`; a bare identifier shared by both kinds is ambiguous. It shows the existing record's state, a bounded handoff and paths for further reading, without loading private configuration or the entire archive.

After reviewing and updating the handoff and its sources, run `python scripts/studio.py context opportunity:reactor-raider --checkpoint`. This writes content fingerprints into the same candidate record. Normal `context` calls flag changed content since that checkpoint, including changes on the same date. Matching fingerprints establish unchanged recorded content, not completeness or gameplay quality. See [continuity](CONTINUITY.md) for how to transfer reviewed work into a fresh task.

## Start a game after choosing one

```powershell
python scripts/studio.py new-game orbit-garden --title "Orbit Garden" --engine undecided
```

This example command creates a planning scaffold under `games/orbit-garden/`. It does not select a studio project or create an engine project until someone runs it. Existing directories are not overwritten. Fill its brief and budget, link the selected research opportunity if any, then implement the smallest playable test. The game directory contains instructions, status, build/QA/release notes, a source directory, marketing directory, and asset register.

## Local tools

Copy [config.local.example.json](../config.local.example.json) to `config.local.json` and use actual executable paths for optional tools. The latter is ignored by Git. The initial machine already has Python, Git, Node, GitHub CLI, and an Unreal installation in the earlier Breakout workspace; the ignored local file may reference that installation without moving it.

Keep studio-specific downloads/installations in `.local/` when practical. Installer-wide changes are not needed for this setup. Install a tool only for a concrete production need and verify the result. Do not store auth tokens in the example file or embed machine-specific paths in game build scripts.

Git treats asset formats as binary but **Git LFS is not configured**. Small source assets can be tracked when their license permits. Before adding large textures, models, audio, or videos, choose LFS or separate asset storage and document how another machine retrieves them. Builds, raw captures, and engine directories belong outside tracked source.

## Public Steam evidence

```powershell
python scripts/studio.py steam-snapshot --apps 3444870 2062430 --out .local/steam-example.json
python scripts/studio.py score research/opportunities/your-candidate.json
```

The first command fetches public store details and review summaries with source URLs and an access timestamp. Prices use the US storefront and USD; review counts use all languages and Steam purchases consistently. Missing/restricted data is reported as such. A zero review count is meaningful only when returned by a successful source response. Review totals are not copies sold, revenue, launch demand, or evidence of trend growth.

The second command scores an existing candidate file. Score weights and confidence rules are maintained in [research/README.md](../research/README.md). A score ranks assumptions; prototype and market evidence must still justify the decision.

## Codex loading

Project skills live in `.agents/skills/`; native agent definitions live in `.codex/agents/`. They inherit the selected model and execution permissions. A new trusted task in this project can discover them. Where an active runtime exposes only general collaboration tools, the producer reads the relevant role definition and includes it in the bounded delegation. No global Codex configuration has been changed.

Setup verification on 2026-09-06: the installed Codex 0.145.0 app-server `skills/list` endpoint discovered all six as enabled repository skills with no project skill errors. This was a metadata query, with no model turn started. Nine role TOMLs were structurally validated; role availability in the current app session still depends on its delegation interface, so the documented fallback remains relevant.

Current format references checked 2026-09-06: [Codex skills](https://developers.openai.com/codex/skills/) and [Codex subagents](https://learn.chatgpt.com/docs/agent-configuration/subagents). Recheck official documentation before changing the wiring for a future Codex version.
