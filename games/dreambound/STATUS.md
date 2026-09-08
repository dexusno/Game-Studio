# Cyborg between worlds — first Unreal beta
Updated 2026-09-08. Stage: prototype. Owner: Klaus. Internal identifier: dreambound; final title undecided.

## Current state
The actual first-person Windows beta exists at `BuildOutput/Shipping/Windows/Dreambound.exe` in the saved Game-Studio project and this worktree. Version 0.1.0-beta2, Unreal 5.8.2. Source, original art/audio and reproducible imports are in this directory. This is an internal playtest build; no publication or purchase occurred.

Implemented: aimed pulse, directional timed guard/capture, impact/Ram, dash/jump, nine combinable attachments with ranks one to three, three switchable elemental cores, three enemy archetypes and a guardian. The seeded route joins eight authored spaces: tech opening, medieval encounters, optional Mirror Trial, boss, and a compact tech arrival. Rewards teach starting patterns retained after defeat; two verified checkpoint slots preserve the run and a boss milestone. The crossing currently retains the whole build as a beta setting, not a settled campaign transfer rule.

Selected visual direction B remains sculpted painterly 3D. Twenty original Blender meshes, eighteen Unreal materials, two original surface textures and twelve original sound effects are integrated. Actual rendering led to fixes for import axes, instanced material usage, Lumen mesh distance fields, lighting, stone texture/palette and weapon framing. Current assets remain simpler than the concept direction; visual fidelity and feel need owner assessment.

## Evidence and limits
- Editor and Windows Development/Shipping packages compiled and cooked successfully. The player package omits the development trace listener.
- Final packaged staged checks: **31 passed, 0 failed**, including real directional combat calls, imported capsule/floor collision on both route parities, finite waves, capped/duplicate rewards, pattern replay and save recovery. [Exact report](evidence/beta-0.1.0-staged-checks.txt). These scenarios stage actors and state; they are not an ordinary playthrough.
- Actual offscreen first-person rendering was inspected. Native launch/start, mouse aim/pulse, Q impact pose, pause/build menus, sensitivity adjustment, save/exit and relaunch/resume were observed through Computer Use using an isolated QA profile. Expedition 1 / seed 124055 and sensitivity 1.10 survived relaunch.
- The oversized weapon behind the title was fixed by hiding the player assembly during menus and restoring it during active play. The rebuilt title, pause screen and resumed first-person view were inspected. [Package identity](evidence/beta-0.1.0-build.json).
- The previous Windows prompt is gone; normal Shipping startup is unblocked. No firewall settings changed.
- Native movement/dash and sustained guarding remain unverified: the available Computer Use interface provides taps/chords, without held-key duration; W/Shift taps did not establish movement. Full focus-loss behavior, audible mix, a complete natural journey, timing, difficulty and fun also remain unverified. The 15–25 minute beta duration in BRIEF is provisional, not measured.

## Next concrete action
Klaus can launch the saved Shipping package for the first direct playtest. First verify held movement, dash and guard, then acquire the opening crystal reward and assess whether combat and attachments invite experimentation. Record any failure against the seed and fix it before expanding content. Complete the natural journey and audio/focus checks before claiming full technical readiness. [Controls](PLAY.md), [build reproduction](BUILD.md), [independent findings](QA.md).

## Continuity and authority
Klaus authorized implementation, subagents, fresh tasks when useful, and computer control for game development. Installed tools and suitable vetted free software are authorized; purchases require asking. Do not re-ask permission for routine game work. Self-aware weapon treatment remains a proposal; precise origin, full campaign length and world-transfer limits are unsettled.

Integration owns shared world/progression/HUD/build/status. Completed specialists remain available for focused fixes: beta_combat (character), demo_core (enemies/projectiles), demo_audio (art), demo_qa (QA/fixtures). Assign one writer per area and read [BRIEF.md](BRIEF.md), [IMPLEMENTATION.md](IMPLEMENTATION.md) and [DECISIONS.md](DECISIONS.md) when resuming. The older Sixfold Recoil browser prototype remains parked.
