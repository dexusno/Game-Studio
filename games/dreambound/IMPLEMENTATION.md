# Current integration contract
Updated 2026-09-08. All workers share the workspace; preserve concurrent changes. The default build is the physical shield courtyard study. The previous pulse-gun route is retained only behind `-DBLegacyBeta`, and is not an accepted design or current QA target.

## Ownership and runtime
Integration owns DBGameMode.*, DBRecoveryScene.cpp, DBMotionDemo.cpp, DBHUD.*, configuration, packaging and shared records. Combat owns DBCharacter.* and DBThrownShield.*. Enemy work owns DBEnemy.* and DBProjectile.*. Art owns art/ and create_art.py/import_art.py. QA owns DBShieldChecks.* and its focused report. Root launches Unreal/imports and integrates all work.

The character exposes PressFire/ReleaseFire, PressGuard/ReleaseGuard, RecallShield, UseSpecial, SuspendCombatInput, OnRunReset and existing upgrade/health interfaces. `ShieldState` distinguishes Held, Charging, Outbound, Lodged and Returning. `bShieldReady`, `ThrowCharge`, `AttackRecovery`, IsShieldAway and GetShieldStateLabel drive the HUD. Tap LMB produces a delayed swept close contact; a hold of at least 0.22 seconds and release throws. Q recalls while away and performs the held heavy action otherwise. There is no gun damage path. Heat remains compatibility data, not a displayed resource.

DBThrownShield is the actual traveling disc, with separate outward/return victim sets, cover sweeps, owner-relative return, breadcrumb routing and a non-damaging emergency recovery. Held protection is unavailable while committed/away. Menus freeze the actor and reset destroys it. Installed attachment visuals travel with it. Anchor interception belongs exclusively to the actual unobstructed DBProjectile travel segment; never add a synthetic source-to-camera fallback that catches melee or spends integrity twice.

Mirror stores timed-guard force for physical contact. Ram changes the heavy action. Frost/Ember/Storm, Echo/Split/Stormfracture and Capacitor modify contact behavior with finite secondary chains. Character descriptions are authoritative for implemented ranks, not the older catalogue's proposed mechanics.

Enemies expose Configure, SetArenaBounds, ApplyCombatHit, phase/telegraph/health and once-only death notification. Their actual movement uses capsule sweeps/local steering; no baked navmesh is required. Melee commit/recovery, caster charge/volley/reposition and hunter movement need normal play validation. Collapsed bodies are retained briefly.

## Scene and progression
DBRecoveryScene creates one authored courtyard with three logical phases sharing it. E at the ward stone starts the current phase; first clear earns Anchor/Mirror/Ram, second clear earns an elemental core, and the final group contains a heavy sentinel. Claimed rewards advance the phase. The final clear leaves a short catch/collapse interval before the victory menu. The court is stable; seeded offsets/offers and learned starting patterns provide limited study variation. This is not the planned procedural campaign.

Default saves use DreamboundShieldStudy, separate from old DreamboundBeta. CRC journal slots, learned patterns and settings persist. Current encounter restarts at its checkpoint. QA requires an explicit isolated slot. DBShieldChecks advances through ordinary engine ticks; it does not recursively tick the world. Its 14 staged scenarios supersede the old 31 fixture for this scene.

## Assets and capture
SM_ShieldPlate is the complete 85cm disc, face+X, with grip-inclusive 21.3cm depth. Rear SM_Core attaches(-6,0,0), Forearm(-12,0,-12) and extends backward. Meshes carry separate value/edge/recess vertex channels and authored UVs. Keep the intended CraftUV as the exported first channel; the UV export defect found during this correction must not return. Tree collision is only the trunk base; foliage and decorative rubble are passable. Preserve source pivots, material slots and collision hulls on reimport.

Art source and FBXs are tracked. Generated Unreal Content and raw frame/audio captures are ignored. Use Build.ps1 for import/build. `-DBCapture` is a posed art screenshot with granted sample upgrades; `-DBMotionCapture` drives real combat APIs and movement from a script. Neither proves ordinary inputs, full completion, fun or performance. Native input observations and owner feedback must be reported separately. Current evidence and remaining work live in STATUS.md and QA.md.
