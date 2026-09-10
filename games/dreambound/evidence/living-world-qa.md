# Living-world update — independent QA

2026-09-10. Owner scope: grounded Shift sprint with stamina, a separate forceful Left Alt dash, organic enemies and richer scenery. This report initially covers the movement increment; the previous Reverie reports remain historical evidence.

## Movement harness ready; execution pending

Reviewed source base `85b73d919992ba947ea74e1bddcd90656b908368` plus the current uncommitted movement implementation and QA additions. QA owns only the movement extension in `unreal/Source/Dreambound/DBShieldChecks.cpp/.h` and this report. The integration owner owns compilation, imports and packaged execution. No engine build or test pass is claimed here.

The new `-DBMovementCheck` branch uses the existing isolated save backup/restore and finish path. It bypasses the legacy combat and route cases. An ordinary ticking character settles onto a real collision floor before public `PressSprint`, `ReleaseSprint`, `Dash`, `ReleaseDash` and `AddMovementInput` calls. The full stamina run has an uninterrupted runway; actual actor displacement and swept wall contact supply the movement evidence. The fixture does not tick the world manually, set test velocities, or teleport during a measured phase.

Ten behavior observations plus the four existing isolation observations are planned:

- Stationary Shift retains stamina; actual walking settles near 590 cm/s.
- A full stamina sprint physically covers roughly 44 metres, reaches roughly 885 cm/s and exhausts in about five seconds.
- Recovery starts after roughly 0.8 seconds, reaches roughly 25 stamina one second later and does not automatically restart a held exhausted sprint.
- A fresh press above 20 stamina produces sprint movement; a fresh press below 20 remains walking.
- Holding sprint/movement against the solid wall stops draining stamina after contact.
- Open-floor dash covers approximately 4.44 metres with no vertical hop or stamina cost.
- Cooldown rejects an early press; continued held calls do not retrigger after cooldown; release and a new press rearm dash.
- Dash stops the capsule at the physical wall without passing through or hopping.
- Pause cancels an active moving dash and freezes position/resources; resume has no stale movement input.
- Run reset during a moving dash clears its motion, restores resources and clears held input state.

Expected duration is about 16–20 seconds of ordinary world ticks, with a 24-second tick budget and 45-second wall-time abort. The result is `Saved/QA/movement-checks.json` with suite `grounded-movement-v1`; the actual compiled executable and revision must be recorded after execution. Required flags: `-NullRHI -DBVerify -DBMovementCheck -DBSaveSlot=DreamboundQA_LivingMovement -DBSeed=552389 -unattended -nosound`. Use the real Shipping binary rather than its returning bootstrap. Require complete=true, aborted=false and failed=0, then inspect each measurement and the journal-restore observation.

## Static review and limits

No confirmed production defect was found in the reviewed movement paths. Sprint drain uses measured displacement, exhaustion requires a new Shift press, the root-motion dash is horizontal, and menu/reset paths remove its source and clear movement input. The dash's 0.13-second evasion immunity is separately stored from the retained Ram `DashTime`; this was inspected in source and is not a tested immunity result. The current input file maps Sprint to LeftShift and Dash to LeftAlt; it was read only and left unchanged.

Harness whitespace validation (`git diff --check` on the two owned source files) passed. Compilation and actual movement results are pending the integration owner's build/run. Native keys, input focus, ordinary player traversal, slopes/stairs/ledges, combat/immunity, visual quality, audio quality, performance and fun are outside this harness. No native input, owner-process action, game launch or GPU work was performed by this reviewer.
