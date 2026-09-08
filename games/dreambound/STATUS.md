# Cyborg between worlds — first Unreal beta
Updated 2026-09-08. Stage: prototype. Owner: Klaus. Internal identifier: dreambound; final title undecided.

## Current state
**Klaus played and rejected 0.1.0-beta2 on 2026-09-08.** It failed the intended combat identity, enjoyment and visual standard. Retain the Windows package at `BuildOutput/Shipping/Windows/Dreambound.exe` as the tested baseline, not an accepted foundation for content expansion. Unreal 5.8.2; no publication or purchase occurred. [Owner feedback and corrective direction](RETROSPECTIVE.md).

Implemented: aimed pulse, directional timed guard/capture, impact/Ram, dash/jump, nine combinable attachments with ranks one to three, three switchable elemental cores, three enemy archetypes and a guardian. The seeded route joins eight authored spaces: tech opening, medieval encounters, optional Mirror Trial, boss, and a compact tech arrival. Rewards teach starting patterns retained after defeat; two verified checkpoint slots preserve the run and a boss milestone. The crossing currently retains the whole build as a beta setting, not a settled campaign transfer rule.

Selected visual direction B remains sculpted painterly 3D. Klaus judged the actual graphics nowhere near the anchor: repetitive pillars/vegetation, a stale environment and plain enemies. He reported generic movement, confusing/simple play, a weapon that feels like a gun with a guard button, no meaningful attack/defense tradeoff, no felt recoil, and a weak laser sound with no power. Movement was usable in his session; a completed journey and measured pacing were not reported. Asset counts and successful imports did not establish visual quality.

**Latest combat clarification:** the shield itself must act as the weapon, including physical launch/recall and a meaningful cost to committing protection to an attack. Preserve freely chosen actions and viable close fighting/guarding; do not reinstate Sixfold's forced shoot-everything/recall routine. Broad creative upgrades, earned powerful combinations, first person and dream-world exploration remain selected. Exact revised controls and attachment mechanics below are working proposals, not new owner decisions.

## Evidence and limits
- Editor and Windows Development/Shipping packages compiled and cooked successfully. The player package omits the development trace listener.
- Final packaged staged checks: **31 passed, 0 failed**, including real directional combat calls, imported capsule/floor collision on both route parities, finite waves, capped/duplicate rewards, pattern replay and save recovery. [Exact report](evidence/beta-0.1.0-staged-checks.txt). These scenarios stage actors and state; they are not an ordinary playthrough.
- Actual offscreen first-person rendering was inspected. Native launch/start, mouse aim/pulse, Q impact pose, pause/build menus, sensitivity adjustment, save/exit and relaunch/resume were observed through Computer Use using an isolated QA profile. Expedition 1 / seed 124055 and sensitivity 1.10 survived relaunch.
- The oversized weapon behind the title was fixed by hiding the player assembly during menus and restoring it during active play. The rebuilt title, pause screen and resumed first-person view were inspected. [Package identity](evidence/beta-0.1.0-build.json).
- The previous Windows prompt is gone; normal Shipping startup is unblocked. No firewall settings changed.
- Agent held-input coverage remains limited by the tap/chord interface. Owner play now provides negative feel, sound and visual evidence and reports usable but generic movement. Full focus-loss behavior, a complete journey, timing and performance remain unverified. The previous 15–25 minute beta estimate was never measured. The 31 technical passes do not contradict the failed owner playtest.

## Next concrete action
Stop extending the current pulse-gun route. The next corrective increment is one representative Bellroot courtyard with a physical shield, convincing throw/impact/catch feedback, contrasting readable threats and two earned behavioral upgrades. First settle the shield's held/deployed/returning rules and author its silhouette/rig for the gameplay camera. Build the scene around an arch, bell/tree landmark and purposeful paths; qualify any sourced supporting assets before use. Show actual moving packaged gameplay with sound against anchor B before claiming the quality target is achieved. This narrows the next proof, not the intended game's depth or campaign scope. No corrective implementation has been built yet. [Working correction](RETROSPECTIVE.md), [build reproduction](BUILD.md), [QA](QA.md).

## Story work deferred by Klaus
Who are we fighting, what do they want, why do they oppose the cyborg, and how do enemy/boss identities and recurring runs fit the worlds? The cyborg/dream-world premise alone does not answer this. Archived institution/recovery-officer ideas are unapproved proposals, not established antagonists. Develop the conflict later as requested; do not silently turn test opponents into canon during the combat correction.

## Continuity and authority
Klaus authorized implementation, subagents, fresh tasks when useful, and computer control for game development. Installed tools and suitable vetted free software are authorized; purchases require asking. Do not re-ask permission for routine game work. Self-aware weapon treatment remains a proposal; precise origin, full campaign length and world-transfer limits are unsettled.

Integration owns shared world/progression/HUD/build/status. Completed specialists remain available for focused fixes: beta_combat (character), demo_core (enemies/projectiles), demo_audio (art), demo_qa (QA/fixtures). Assign one writer per area and read [BRIEF.md](BRIEF.md), [IMPLEMENTATION.md](IMPLEMENTATION.md) and [DECISIONS.md](DECISIONS.md) when resuming. The older Sixfold Recoil browser prototype remains parked.
