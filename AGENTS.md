# Game-Studio

Build small, distinctive, satisfying games with Klaus. This repository owns the studio workflow and one directory per game. Read [studio/CHARTER.md](studio/CHARTER.md) and [studio/WORKFLOW.md](studio/WORKFLOW.md) for a new assignment; use the closest game `AGENTS.md` and `STATUS.md` when continuing a game. User instructions and existing authorization take precedence over local workflow preferences.

## Start and resume

- Identify whether the request is research, a new game, iteration, release, or a question. Answer questions without treating them as permission to start a game.
- Use `python scripts/studio.py status` to discover the portfolio. A game's `game.json` is its stage record; its `STATUS.md` records the latest evidence and next action. Do not create competing status registries.
- Read only the relevant skill in `.agents/skills/`. Route bounded parallel work using [studio/TEAM.md](studio/TEAM.md). Give each worker a deliverable, owned paths, acceptance criteria, and a return condition. Integrate and verify their work; a role title is not evidence of success.
- Continue authorized reversible work autonomously. Ask only about a consequential missing preference or an external commitment not already authorized. Prepare the actual build, page, or release package before seeking any required publication approval.

## Work that matters

- Default to a few days of implementation and occasional owner playtests, with explicit scope and owner-time budgets. A modest commercial result can be worthwhile. Research scores do not predict sales.
- Test the enjoyable interaction before content expansion. Validate controls, readable feedback, sound, replay motivation, and a coherent visual identity in an actual playable build. Code review alone cannot establish game feel.
- Research must record source URLs, dates, observation versus inference, missing access, and confidence. Never report review counts as sales or one popular example as proof of a rising trend. Support evergreen ideas as well as trend opportunities.
- Use existing tools where appropriate; select an engine per game. Keep game sources and small original assets in `games/<slug>/`; private machine paths in ignored `config.local.json`, downloaded tools in `.local/`. Do not copy an engine into Git.
- Record asset origin, license, modifications, and required credit in the game's `assets/manifest.csv`, including generated assets and their generation tool. Free download does not establish commercial redistribution rights.
- Test the failure modes that matter: saves, input focus/capture, collisions, progression, pause, resolution changes, packaging, and performance. Record what was actually tested, build/version, results, and untested targets. A scaffold is not a playable game; a compiled editor build is not a verified shipping build.
- Keep changes inside the assigned game or shared studio task. Extract shared code after demonstrated reuse. Keep credentials, private business information, raw captures, tool binaries, and large build outputs out of this public repository.

## Finishing a turn

Update the affected game's `STATUS.md` with what changed, evidence, known issues, next concrete action, and any input truly required. Preserve decisions when holding or rejecting an opportunity. Run relevant checks once, fix failures, then report the result plainly. Do not add ceremonial documents, standing meetings, extra approval stages, or automatic schedules to make the workflow look complete.
