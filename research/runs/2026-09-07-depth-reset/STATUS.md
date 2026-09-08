# Cyborg between worlds — resume here

Updated 2026-09-08. **Klaus authorized the beta; current implementation resumes at [games/dreambound/STATUS.md](../../../games/dreambound/STATUS.md).** An actual Unreal C++ project has compiled; rendered/package and playtest evidence belong in that game's records. This document retains the design foundations. The selected candidate is [reactor-raider.json](../../opportunities/reactor-raider.json); both identifiers are internal, not a final title. [Sixfold Recoil](../../../games/scrapstorm/STATUS.md) remains parked.

## Owner decisions and constraints to retain

- First person. One modular weapon also forms a shield, with freely chosen attacks and defense. The weapon is the central evolving object; no compulsory shoot-everything/recall cycle.
- Challenge, learnable skill, tactical decisions, exploration, rewarding upgrades and lasting achievements are essential. The prior beta was boring to Klaus despite passing technical tests. Attractive full 3D Unreal presentation is required; the old Canvas/SVG style does not meet the bar.
- Art style selected on 2026-09-08: **B — sculpted painterly 3D**, endorsed with “B looks great.” Substantial modeled shapes, tactile materials, painted color transitions and expressive lighting unite fantasy and technology; world palettes may differ. Actual in-engine finish remains to be demonstrated.
- A cyborg's apparent dreams reveal real reachable places. Different worlds can have their own technology, magic and biomes. This is the accepted story foundation; exact explanations and characters remain proposals.
- Procedurally varied levels and expeditions should change decisions while remaining navigable, readable and enjoyable. Meaningful campaign progress and an eventual ending matter. No replacement campaign duration is established; the earlier 90-minute discussion is not its completion estimate.
- Extensive attachments, tech, magic and possible crystals/jewelry should create fresh behaviors. Klaus welcomes earned OP combinations. Difficult acquisition, smart assembly and execution can lead to room clears or boss melts; do not secretly cancel a successful build.
- Stackable enhancements and behavioral evolution are being explored. The catalogue has 120 proposals: 100 behavior/rule concepts plus 20 supporting enhancement types. Ranks are not separate abilities. That count is neither a release commitment nor a ceiling.
- “Ten times the complexity” was qualitative, not a quota. Exact content, schedule, cash allocation and owner-time budget remain open. A representative beta must test enjoyable combat and progression before building the whole campaign.

## Current proposals and open choices

The integrated design proposes a short technological escape, one substantial generated medieval realm and a compact playable arrival elsewhere to test using a transferred reward. This is a beta proposal, not two full promised worlds.

The writer's leading explanation is borrowed senses from real people. Exact lore, final title, specific weapon/world visual designs, baseline control details, active equipment capacity, fusion/evolution rules and cross-world transfer remain open. **Two carried attachments was an experiment, not an owner-selected limit.** Preserve meaningful learned ownership and enough of a rewarding build when testing transfers.

The [art comparison](dreamworld-art-direction.md#visual-comparison--2026-09-08) shows cinematic realism (A), selected sculpted painterly (B) and graphic 3D (C) across fantasy and technology. Klaus asked whether the fidelity can actually be delivered before endorsing B. The board is generated concept art, not Unreal output. An actual small Unreal scene viewed in motion is the next evidence for the quality target; no such sample exists yet. Selecting B does not settle exact weapon geometry or guarantee the concept images' polish.

Klaus is exploring whether the weapon/shield should be self-aware through a blend of technology and magic; **sentience is not yet selected**. The [narrative proposal](dreamworld-narrative.md#weapon-self-awareness--proposal-for-discussion-2026-09-08) recommends a quiet, gradually awakening companion whose discoveries add meaning to major attachments and the dream mystery. Preserve player control, creative builds and remembered experiences across equipment changes; avoid constant chatter. This is a discussion proposal, not implemented character behavior.

The reward system proposes knowable pursuits alongside surprises, immediately usable major rewards, persistent learned patterns/milestones and clearly identified temporary amplification. Effect chains may trigger compatible effects; prevent duplicate consumption and nontermination without imposing an automatic damage ceiling.

## Evidence and environment

Unreal 5.8.2 and background editor-Python execution were verified locally on 2026-09-07. This was a tooling probe, not a new rendered or packaged game. Breakout supplies historical build/cook/package/native-input testing notes. Actual tool paths belong in ignored config.local.json; check that configuration before claiming Unreal is unavailable. Klaus subsequently installed Blender: **5.2.1 LTS background modeling and FBX/GLB export were verified on 2026-09-08**. Fusion 360 is also available according to Klaus; local Fusion operation has not been tested.

Klaus delegated the art workflow choice. The [updated pipeline](art-pipeline.md) uses Blender for assembly/finishing, Fusion selectively for precise mechanical parts, GPT Image 2 for reference-based visual development, and Meshy as the first image-to-3D candidate for a static-prop trial. Keep the transforming weapon's functional geometry controlled. AI 3D quality and end-to-end Unreal integration remain untested; no Meshy/Tripo connection was available in this session.

Klaus authorized computer control for Unreal work and playtesting. Prefer background work where practical and coordinate foreground input when needed. No new purchase, asset download, publication or installation was made in the current design work.

The old Sixfold beta passed 33 core/audio checks and 11 browser suites; Klaus then rejected its fun and presentation. Those results do not validate this replacement. Design specialists reviewed narrative, art, production, progression and combinations. The 120-entry catalogue was counted and linked records checked; none of those ideas has gameplay validation.

## Next concrete action

Visual direction agreed: B. Next, create a bounded Unreal visual sample with finished stone/metal/ceramic materials, a representative weapon section, lighting and first-person movement. Evaluate close-up finish, combat visibility and performance before committing to a production fidelity target. Keep this focused enough to support the gameplay beta rather than become a separate content-expansion project.

Turn the current equipment/reward proposal into a reviewable first-beta implementation brief: define the base attack/guard/movement interaction, choose a coherent subset of attachments, and map their acquisition through encounters and a boss. Include contrasting builds, a useful duplicate/evolution, an earned power spike and the consequence of a crossing. Reuse the existing specialist work. Resolve only consequential remaining choices with Klaus; proceed with reversible preparation.

The continuity task added startup routing, this handoff, a bounded context command and local integration. A reviewer given no game conversation recovered the settled choices, open proposals, actual evidence and next action from project files. CLI checks cover changed content, path handling and record preservation; native symlink creation is unavailable on this Windows host, with alias behavior also checked through mocks. It adds no game runtime. No additional owner answer is required to use the handoff.

## Read only what the assignment needs

| Focus | Detailed source |
|---|---|
| Overall player promise, worlds and carryover | [Integrated design](cyborg-worlds-design.md) |
| Controls, equipment, rewards and earned power | [Weapon/shield design](weapon-shield-design.md) |
| Ability invention, stacking and evolution | [120-entry catalogue](attachment-catalogue.md) |
| Acquisition and permanent ownership | [Progression review](progression-review.md) |
| Story specifics | [Narrative proposals](dreamworld-narrative.md) |
| Visual identity / actual asset routes | [Art direction](dreamworld-art-direction.md), [pipeline research](art-pipeline.md) |
| Beta feasibility | [Producer review](dreamworld-production-review.md) |
| Earlier alternatives and source history | [Research report](README.md), read as historical context |

Use the [studio continuity procedure](../../../studio/CONTINUITY.md) for task/worktree transfers. Update this handoff and the candidate when decisions change; review its sources before refreshing the checkpoint. Other project copies may be older even when their own checkpoint matches.
