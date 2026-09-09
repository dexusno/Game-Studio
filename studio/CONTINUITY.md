# Working across tasks

This is the studio's handoff procedure, not a second project-status registry. Keep current game state in its game manifest and STATUS.md. Before a game directory exists, keep candidate state in its opportunity JSON and a short handoff referenced there.

## How Klaus can work

Use one task for one coherent outcome: refining the reward system, testing a weapon interaction, creating an enemy, or reviewing a beta. Follow-up questions about that outcome can stay in the same task. Start a new task when the objective changes or a milestone has a useful handoff; there is no required message count. Klaus should not have to reconstruct the project history.

For the current game, a new task can begin with: **“Continue the cyborg game from its project handoff. Today's focus is [the outcome].”** With no added focus, the agent should read the recorded next action. To prepare a transition, “Save a handoff for a new task” is sufficient, but agents also maintain it as part of normal project work.

A task is a working conversation with context. A project supplies the files and instructions used by those conversations. Continuing a task, forking it and starting a fresh one have different history, but all should consult current project records. A fork copies stored history; use it for a branch of an existing investigation. A fresh task with a short handoff is the studio's default for a new milestone.

## The small set of records

| Record | Responsibility |
|---|---|
| Root and nearest AGENTS.md | Startup, work and handoff instructions; routing rather than a full game bible |
| Game manifest or opportunity JSON | Authoritative stage/status, identity and pointers; optional content checkpoint |
| The referenced STATUS.md | Current outcome, settled foundations, unresolved choices, evidence, next action and active ownership if needed |
| Existing design, decisions, build and QA documents | Detailed reasoning, history and reproducible evidence, opened only for relevant work |
| Ignored config.local.json | Machine-specific tool paths; never a source of game design or a public memory file |

For the current game, [the game record](../games/dreambound/game.json) and [the selected candidate](../research/opportunities/reactor-raider.json) point to [the game handoff](../games/dreambound/STATUS.md). Use `context game:dreambound` to resume. These identifiers are retained for continuity; neither is the final title. The [old game](../games/scrapstorm/STATUS.md) stays parked.

Do not copy the full conversation into every document or create a writable global task dashboard. Keep the handoff concise; move detailed completed explanations into their existing topic documents and link them. A useful startup is the handoff plus a few relevant sections, not every design file and research result.

## What the agent does at startup

1. Read applicable instructions and run `python scripts/studio.py context`. Resolve the requested game or candidate explicitly; a qualified target such as `opportunity:reactor-raider` avoids collisions.
2. Run `context TARGET`, read its short handoff, and inspect the relevant linked sources. The command prints reference paths without adding their full contents to model context; it reads listed sources locally to compare fingerprints. Open the attachment catalogue only for equipment/reward work. The checkpoint covers the owning record and its listed sources, not every linked document in the archive.
3. Check the reported checkpoint and the actual checkout before trusting a continuation. If records changed, reconcile the handoff with those changes. If no checkpoint exists, report freshness unverified. An older branch with internally consistent files can still be behind the intended integration branch; a hash alone cannot detect that.
4. State the immediate outcome and continue within existing authorization. Do not ask Klaus to choose first person or the combined weapon/shield again. Identify an actual unresolved preference only when it materially blocks the requested work.

The command reads local records, not private configuration or chat databases. Optional local memories and past-task retrieval may help recover a missing decision, but verify against the owner's latest instruction and the current files before changing direction.

## What the agent preserves during work

Record material owner decisions when they occur, including what they replace. Label suggestions as proposals until accepted; catalogue counts, sample ranks and transfer experiments do not become requirements through repetition. Separate design review, build checks and actual human playtesting.

The integration owner maintains the handoff. Parallel workers receive exact file ownership, input paths, acceptance evidence and a stopping condition. They return changes and findings for integration rather than all writing the same STATUS.md. Active assignments belong in that handoff only while useful, with branches or file responsibilities; remove stale ownership after integration.

Do not promise to remember an important correction without recording it. Conversely, a simple factual question does not need a new task log or an implementation milestone.

## Ending and transferring a task

Update the existing topic files and short handoff: what changed, what was verified, known limits and the next concrete action. Review it against the sources before running `context TARGET --checkpoint`. This writes fingerprints to the same record. Normal context calls are read-only and compare those fingerprints; matching content does not prove that the design is correct or the game is fun.

Preserve the relevant changes in a local Git commit and integrate reviewed work into the saved project's starting branch. If integration is not yet appropriate, give the receiving task the exact branch/revision and pending work instead. Check concurrent changes and preserve them; never use a reset or overwrite to make a handoff look clean. No push, publication or new external commitment is implied by a local checkpoint.

A new worktree starts from a selected Git state. Another task's uncommitted files are not automatically a shared project memory. Existing worktrees do not automatically advance when the saved project is updated, either. Before reporting a handoff ready, verify it from the receiving checkout or a clean checkout of its starting revision. Make sure required ignored tool-path configuration is available locally; the repository's .worktreeinclude copies only config.local.json for local managed worktrees.

## Installed automation — September 9

[Project hooks](../.codex/hooks.json) call [the dependency-free helper](../scripts/continuity.py). Once trusted, `SessionStart` loads a bounded context index on a new task, or the selected handoff on resume and immediately after compaction. The agent binds an explicit game with `python scripts/continuity.py bind game:dreambound`; bindings are per task, so another game task does not inherit this task's selection. A task started inside a game directory can use that directory's scope.

`PreCompact` and `Stop` save a small local recovery copy of the selected handoff, checkout revision and changed filenames. Each event replaces its previous copy for that task. These copies and routing pointers live in ignored `.local/continuity/`; hooks do not read transcripts, call another model, run tests, block a turn, commit files or start a background service. They copy recorded state; the agent must still write decisions and current progress into the canonical files during work. Unsaved conversation-only reasoning cannot be reconstructed by this helper.

After reviewing the handoff, refreshing its checkpoint, committing and integrating the changes, run `python scripts/continuity.py handoff game:dreambound --focus "the next outcome"`. This writes a ready-to-use prompt in `.local/continuity/handoff-game-dreambound.md` with the exact source revision and reading order. It refuses stale checkpoints and a dirty checkout. This automates prompt preparation; it does not select a milestone, create a task or merge concurrent work. Existing worktrees still need their intended starting revision.

**Activation:** Codex requires a one-time review/trust of each new hook definition before executing it. Hooks are enabled in the local CLI, but installing these files alone is not proof that lifecycle execution is active. In the Codex CLI opened in this project, use `/hooks` to review and trust the three Game Studio hooks. Changed definitions require review again. No trust hashes, permission settings or bypass flags are written by this implementation. If hooks are unavailable or pending review, AGENTS.md and the helper commands remain usable by the agent. [Official hook behavior and trust requirement](https://learn.chatgpt.com/docs/hooks#review-and-trust-hooks).

## Codex guidance behind this workflow

Checked against official documentation on 2026-09-08:

- Codex reads repository instructions at startup. Keep them concise and route to detailed documents as needed. [Project instructions](https://learn.chatgpt.com/docs/agent-configuration/agents-md), [customization guidance](https://learn.chatgpt.com/docs/customization/overview).
- Local memories can carry useful context across sessions, but generation happens in the background and required rules belong in instructions or checked-in documents. This workflow does not require changing memory settings. [Memories](https://learn.chatgpt.com/docs/customization/memories).
- Worktrees isolate files and start from the selected Git state; ignored files require explicit local setup or supported copying. [Worktrees](https://learn.chatgpt.com/docs/environments/git-worktrees).
- Starting, resuming and forking conversations are distinct operations; a fork copies stored history. [App-server conversation lifecycle](https://learn.chatgpt.com/docs/app-server).

The project workflow is our chosen practice, not a claim that every conversation shares its entire history or that these files are automatically kept truthful without agent maintenance.
