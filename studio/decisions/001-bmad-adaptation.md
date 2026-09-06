# 001 — Native Codex studio with selected BMAD patterns

Date: 2026-09-06. Status: adopted for initial setup.

## Decision

Use original project-scoped Codex skills, agent roles, short game templates, and a small Python helper. Borrow the useful structure of BMAD: a clear brief, scoped implementation, specialist handoffs, a playable test, independent review, and a record that lets the next session continue. Do not install the entire BMAD framework for the initial short-game workflow.

This keeps setup aligned with a single owner who can get useful results from a few days of AI-assisted work. Full sprint administration and repeated human checkpoints would add work without answering the immediate questions: is the interaction enjoyable, is there an audience, and can we finish it economically? A larger validated game can adopt more formal planning later.

## Upstream inspection

Inspected [BMAD Method v6.12.0](https://github.com/bmad-code-org/BMAD-METHOD/releases/tag/v6.12.0), published 2026-09-04, and main revision `abe4eb1bce919c9d22cd18b3519353d5824c4b75`. Inspected [Game Dev Studio v0.7.2](https://github.com/bmad-code-org/bmad-module-game-dev-studio/releases/tag/v0.7.2), published 2026-08-31, revision `2486f5f5f3b8870baa6cee4615a870c0330f441c`.

BMAD's [Quick Flow versus full production guide](https://game-dev-studio-docs.bmad-method.org/explanation/quick-flow-vs-full/) supports choosing a short prototype loop for small work. Its [decision-focused research](https://docs.bmad-method.org/plan/research-a-decision/) informed keeping research tied to a choice rather than producing a large report for its own sake.

The tagged [Codex installer configuration](https://github.com/bmad-code-org/BMAD-METHOD/blob/v6.12.0/tools/installer/ide/platform-codes.yaml) supports `.agents/skills`. However, main README, published installation documentation, and the game module documentation differ in installation routes and some command names at inspection time. The game module source uses `gds-quick-dev` while README examples still use the older `bmgd` prefix. We use native current Codex formats rather than copying stale invocation names.

BMAD Method and Game Dev Studio are MIT-licensed, with their own notices. This repository does not vendor their code, skill text, or templates; its documents are original adaptations of workflow ideas. If actual upstream material is incorporated later, retain the applicable license and copyright notices and record the source revision. This setup does not claim BMAD affiliation.

## Appdrip adaptation

Read the existing Appdrip research command, research-team instructions, trend-source skill, archive checks, brief template, and pipeline status locally. Useful patterns were dated evidence, trend-source variety, opportunity scoring, rejected/held idea memory, independent research review, and explicit next steps. The inspected pipeline reported that the research process had not yet run; it is a design reference, not evidence of commercial effectiveness.

Game-Studio changes the unit of research from an app utility to a playable promise: player fantasy, interaction, feedback, replay motivation, and a reachable audience. It uses a smaller initial funnel, preserves near-peak and evergreen candidates, accounts for store lead time, and adds a mandatory distinction between predicted fun and observed playtest behavior. No private Appdrip source, business data, or credentials have been copied.

## Consequences

- New sessions can use native project skills and role files without a framework installer or global configuration changes.
- Research and game records are portable Markdown/JSON; the helper has no third-party Python dependency.
- Actual game production still requires engine selection, assets, playtesting, and a verified build. A workflow is not a substitute for these.
- We maintain a small amount of original studio tooling. Adopt more upstream infrastructure only when it solves a demonstrated coordination or maintenance problem.
