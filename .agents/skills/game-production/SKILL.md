---
name: game-production
description: Develop an approved playable game into a scoped, testable product through small integrated increments. Use for game features, content, polish, performance, and production fixes within an existing game brief.
---

# Game production

Read [CHARTER](../../../studio/CHARTER.md), [WORKFLOW](../../../studio/WORKFLOW.md), and [TEAM](../../../studio/TEAM.md), then the game's instructions, manifest, brief, status, build guide, and recent decisions. Work from the actual implementation and recorded production scope, not a speculative feature backlog.

Choose the smallest increment that improves the player experience or resolves a release risk. State observable acceptance criteria and the affected build target. Keep the owner's time and game budget visible; if the feature grows, cut lower-value scope or present a concrete tradeoff before expanding the product promise.

Assign independent implementation to the `game-engineer`, focused mechanics or content work to the `game-designer`, visual assets to the `art-director`, and sound to the `audio-designer` only as needed. Give each exact file ownership and integration assumptions. Shared scene files, project settings, and manifests need a single writer or sequential handoff.

Preserve a runnable main loop while integrating work. Match the game's established systems when useful; avoid adding infrastructure for hypothetical later games. Consult current primary documentation when engine or SDK behavior is uncertain. Read existing code and diagnostics before replacing subsystems.

Track asset source, creator, license or generation terms, intended use, modifications, and attribution in `assets/manifest.csv`. A downloadable asset is not automatically licensed for commercial redistribution. Record missing rights evidence as unresolved before including the asset in a distributable build.

Run checks appropriate to the changed behavior: actual build, targeted logic checks, relevant gameplay flows, and visual or audio inspection where the change requires it. A repository schema check cannot certify an engine build. Use the `qa-playtester` independently for substantial gameplay changes and before a release candidate. Fix actionable findings, then rerun the affected checks.

Update the game's build, QA, and decision records with real evidence; send the producer an integration summary for status updates. Report the artifact or playable result, what changed for the player, validation performed, limitations, and the next useful step. Separate owner playtest feedback from agent technical verification.
