---
name: game-prototype
description: Turn a selected game concept into the smallest playable experiment that tests its core interaction and scope. Use for first playable builds or a focused mechanics experiment, not a full production pass.
---

# Game prototype

Read [CHARTER](../../../studio/CHARTER.md), [WORKFLOW](../../../studio/WORKFLOW.md), and [TEAM](../../../studio/TEAM.md), then the game's `game.json`, `BRIEF.md`, `STATUS.md`, `DECISIONS.md`, and `AGENTS.md`. Preserve the selected concept and recorded constraints. If selection is still pending, prepare a concrete prototype proposal and continue only work already authorized.

Write one falsifiable play question: what should the player understand, do, or want to repeat, and what observation would change the decision? Set a short experiment budget, a playable boundary, and explicit exclusions. Choose the engine from user intent, existing code, required platforms, installed tools, and iteration cost; do not default to Unreal merely because a prior game used it.

For an authorized new game, use `python scripts/studio.py new-game SLUG --title TITLE --engine ENGINE`. The generated folder is a planning template, not an engine project. Inspect `BUILD.md` and actual files, then initialize the required engine project and implement the experiment using available tools. Document real build and launch commands as they become known.

The `game-designer` owns the loop and playtest question, `game-engineer` owns the assigned implementation, and `qa-playtester` checks the resulting build independently. Add art or audio only when it helps test readability, timing, or appeal. Assign disjoint files and keep the producer responsible for integrating the status.

Prioritize a complete loop: start, input, response, success or failure, and restart. Use readable temporary assets with provenance in `assets/manifest.csv`; track generated assets and licensing evidence as carefully as downloaded assets. Replace engine assumptions with a real build and a launch check before claiming a playable result.

Record build identity, commands, test environment, observed behavior, and failures in `BUILD.md` and `QA.md`. If UI or engine access is unavailable, report the exact unverified behavior and leave a runnable test package. Do not label compilation, screenshots, simulated input, or code review as evidence that the game is fun.

Give the owner a short play session with specific observation questions. Incorporate actual feedback into a continue, change, or park recommendation. Record the decision and next step; preserve the experiment even when it fails. Advance to production only when the current authorization and recorded decision support it.
