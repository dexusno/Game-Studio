# Cyborg between worlds — first Unreal beta
Updated 2026-09-08. Stage: prototype. Owner: Klaus. Internal identifier: dreambound; final title undecided.

## Current state
The actual first-person Windows beta exists at `BuildOutput/Shipping/Windows/Dreambound.exe` on the development machine. Version 0.1.0-beta1, Unreal 5.8.2. Source, original art/audio and reproducible imports are in this directory. This is an internal playtest build; no publication or purchase occurred.

Implemented: aimed pulse, directional timed guard/capture, impact/Ram, dash/jump, nine combinable attachments with ranks one to three, three switchable elemental cores, three enemy archetypes and a guardian. The seeded route joins eight authored spaces: tech opening, medieval encounters, optional Mirror Trial, boss, and a compact tech arrival. Rewards teach starting patterns retained after defeat; two verified checkpoint slots preserve the run and a boss milestone. The crossing currently retains the whole build as a beta setting, not a settled campaign transfer rule.

Selected visual direction B remains sculpted painterly 3D. Twenty original Blender meshes, eighteen Unreal materials, two original surface textures and twelve original sound effects are integrated. Actual rendering led to fixes for import axes, instanced material usage, Lumen mesh distance fields, lighting, stone texture/palette and weapon framing. Current assets remain simpler than the concept direction; visual fidelity and feel need owner assessment.

## Evidence and limits
- Editor and Windows Development/Shipping packages compiled and cooked successfully. The player package omits the development trace listener.
- Final packaged staged checks: **31 passed, 0 failed**, including real directional combat calls, imported capsule/floor collision on both route parities, finite waves, capped/duplicate rewards, pattern replay and save recovery. [Exact report](evidence/beta-0.1.0-staged-checks.txt). These scenarios stage actors and state; they are not an ordinary playthrough.
- Actual offscreen first-person rendering was inspected. Normal packaged title startup was inspected through Computer Use. A camera initialization defect was fixed afterward; the corrected normal title/input check remains pending.
- Native interaction is waiting for Klaus to dismiss the Windows Security prompt left by the earlier Development run. He was asked to click Cancel; the Computer Use skill prohibits acting on security permission requests. The Shipping process was observed with zero TCP and zero UDP sockets. No firewall settings changed.
- Ordinary input/focus, audible mix, a complete natural journey, timing, difficulty and fun remain unverified. The 15–25 minute beta duration in BRIEF is provisional, not measured.

## Next concrete action
After the existing Windows prompt closes, launch the Shipping package with an isolated `-DBSaveSlot=DreamboundQA_Input`, verify normal start/reward/controls/pause/focus/relaunch, and fix any failure before calling technical readiness complete. Then launch the owner profile and ask Klaus to assess combat and rewards before expanding content. [Controls](PLAY.md), [build reproduction](BUILD.md), [independent findings](QA.md).

## Continuity and authority
Klaus authorized implementation, subagents, fresh tasks when useful, and computer control for game development. Installed tools and suitable vetted free software are authorized; purchases require asking. Do not re-ask permission for routine game work. Self-aware weapon treatment remains a proposal; precise origin, full campaign length and world-transfer limits are unsettled.

Integration owns shared world/progression/HUD/build/status. Completed specialists remain available for focused fixes: beta_combat (character), demo_core (enemies/projectiles), demo_audio (art), demo_qa (QA/fixtures). Assign one writer per area and read [BRIEF.md](BRIEF.md), [IMPLEMENTATION.md](IMPLEMENTATION.md) and [DECISIONS.md](DECISIONS.md) when resuming. The older Sixfold Recoil browser prototype remains parked.
