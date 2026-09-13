# Roles and handoffs

These specialist roles are available when their specialization helps a concrete task. They are not persistent staff, scheduled workers, or a requirement to run a full team. The coordinating task usually acts as producer and brings in only the roles needed now. The owner supplies priorities, product judgment, and occasional playtests; routine coding, research, packaging, and integration belong to the working agents.

Read [CHARTER](CHARTER.md) for operating preferences and [WORKFLOW](WORKFLOW.md) for stages and decisions. Native role definitions live in `.codex/agents/`. Skills live in `.agents/skills/` and provide the stage workflow. Models, tools, and permissions inherit the active runtime; no role chooses a fixed model or creates extra authority.

| Role | Bring in for | Typical owned output |
| --- | --- | --- |
| [producer](../.codex/agents/producer.toml) | A milestone, prioritization, integration, or a blocked handoff | Current portfolio report and assigned game status or decision records |
| [trend-scout](../.codex/agents/trend-scout.toml) | A bounded audience or trend question | Assigned dated source notes and candidate records |
| [market-analyst](../.codex/agents/market-analyst.toml) | Competitors, positioning, pricing, commercial assumptions | Assigned comparison, opportunity analysis, or retrospective evidence |
| [game-designer](../.codex/agents/game-designer.toml) | A loop, mechanic, level, tuning problem, or playtest question | Assigned design notes, tuning data, or level files |
| [game-engineer](../.codex/agents/game-engineer.toml) | An implementation, fix, engine project, or package | Assigned code, scene, configuration, and build records |
| [art-director](../.codex/agents/art-director.toml) | Visual readability, style, reusable assets, store art | Assigned art specification and visual assets |
| [ui-ux-designer](../.codex/agents/ui-ux-designer.toml) | Distinctive game interfaces, clear HUD hierarchy, contextual teaching, equipment choices and risk/reward comprehension | Assigned player flows, interface designs or implementation, state bindings and usability evidence |
| [audio-designer](../.codex/agents/audio-designer.toml) | Sound feedback, music, levels, loops, audio integration | Assigned audio assets and trigger or integration notes |
| [qa-playtester](../.codex/agents/qa-playtester.toml) | Independent verification of a change or release artifact | Assigned QA evidence, reproductions, and targeted checks |
| [release-producer](../.codex/agents/release-producer.toml) | A store listing, release package, or authorized submission | Assigned release record, listing copy, and submission evidence |

## Route the work

Use [studio-director](../.agents/skills/studio-director/SKILL.md) for portfolio direction. The stage skills are [game-research](../.agents/skills/game-research/SKILL.md), [game-prototype](../.agents/skills/game-prototype/SKILL.md), [game-production](../.agents/skills/game-production/SKILL.md), [game-release](../.agents/skills/game-release/SKILL.md), and [game-retrospective](../.agents/skills/game-retrospective/SKILL.md). Read the selected local skill when the runtime does not list it automatically.

Use [game-ui-ux](../.agents/skills/game-ui-ux/SKILL.md) with the UI/UX specialist for interface and player-journey work at any stage. It owns both visual craft and comprehension; the art director supplies the wider visual language/assets, the game designer owns the rules and narrative purpose, and the engineer integrates behavior. Agree file ownership before edits. A UI assignment does not authorize inventing a story or loss system. Creating a specialist does not start game work.

For a small task, the coordinator can read a role and do the work directly. Delegate a concrete subtask when it can run independently alongside useful local work or adds independent judgment. Typical useful pairs are scout plus analyst on separate research notes, engineer plus artist on separate assets, or an implementation author followed by independent QA. Cross-dependent edits should be sequential.

Native custom agent support depends on the active Codex runtime. If its delegation interface offers a registered agent type, use the matching role. If it does not, read the relevant `.codex/agents/<role>.toml` and include its `developer_instructions` plus the bounded assignment in the real delegation tool's task payload. For example, the available `collaboration.spawn_agent` accepts `task_name` and `message`; put the role instructions and assignment in `message`, without inventing an `agent_type` argument. Use the tools actually exposed in the current session. If delegation is unavailable, apply the role directly and disclose when independent review remains outstanding.

The UI/UX role uses the same project-local TOML format, checked against [OpenAI's custom-agent schema](https://learn.chatgpt.com/docs/agent-configuration/subagents#custom-agents) on 2026-09-13. Adding its file does not prove that an already-running session's named role list has refreshed; use the fallback above when needed.

## Make each handoff executable

Provide only the context needed for the assignment:

1. The concrete question or artifact, current stage, and relevant prior decision.
2. Input files, sources, build identity if applicable, and intended player behavior.
3. Exact files or directories the worker owns, files they may read, and shared files they must hand back to their owner.
4. Observable acceptance criteria and the evidence to return.
5. Time, scope, tool or environment constraints, existing authorization, and the stopping condition.

Example assignment: "Act as game-engineer using the supplied role instructions. For games/example, fix loss-of-focus input sticking described in QA.md. Own only the input source file named in this handoff; read the brief and build guide. Preserve controls. Return the change, a real build result, and reproduction steps for independent QA. Stop if resolving this requires a broader input-system redesign; explain the tradeoff with the prepared fix or diagnostic evidence."

The receiver returns changed artifacts, checks performed, known limitations, and the next executable step. The coordinator integrates the result and updates the game's `STATUS.md`. A worker must report unexpected file overlap before editing another worker's assignment. Asset creators can return manifest rows to the manifest owner instead of competing to edit one CSV. No worker writes a global status file or second portfolio registry; `games/*/game.json` remains the discovery source and the current response carries a portfolio report when useful.

## Keep review independent and proportionate

The implementation author demonstrates the intended behavior; another agent reviews substantial changes and release readiness using the real artifact. QA reports reproducible observations, then the author fixes findings and QA rechecks affected behavior. Ordinary reversible document edits need an appropriate check, not a simulated production ceremony.

Record whether evidence came from static inspection, an actual build, an executed game flow, visual or audio inspection, or a human playtest. None is interchangeable with the others. A human fun assessment remains unverified until someone has played the relevant interaction. Market research similarly distinguishes observed facts, reported claims, estimates, and hypotheses.

Authorization and user preferences travel with the handoff. Do not ask again for an action already authorized, and do not expand authorization because another role is involved. Prepare a concrete artifact before presenting an unresolved product decision or external action to the owner. Keep independent authorized work moving while waiting for input.
