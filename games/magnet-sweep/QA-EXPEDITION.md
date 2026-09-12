# Expedition implementation QA

Independent QA for the build-system implementation begun 13 September 2026. This is a separate successor mode; the existing Extraction/Clarity career and possible owner session remain protected. The reviewer owns only this record and `Tests/ExpeditionTests.cpp`. No native input, game launch, owner-profile access or parallel build is authorized in this pass.

## Evidence state

**Final verified report: `2026.09.12-23.45.53`, 52 Success, 0 failed, 0 warnings/errors, 0 not run.** I read the actual [automation report](unreal/Saved/Automation/index.json), counted **31 expedition cases and 21 legacy cases**, and verified SHA-256 `C589F887F6634CAF2ED453A0472D478965BB8DF8FE276508E68A80DE94CF9FD1`. Every case listed below passed. Root owns the successful integrated Editor compile/headless execution and the separate package; this reviewer did not run a competing build or launch a game. This report proves the tested callbacks and simulated physical outcomes, not native controls or fun.

The final Shear diagnostic records head `(317.6, 42.6)` beyond its upward plane at `y=65`, with zero links and 0.5 seconds remaining. Thus the repair passed an actual movement/cutting assertion. Both earned second-site routes also passed: Winch/Hook recovered and smelted 362 output, Coil/Jaw 366; each paid both milestones exactly once and restored its earned session.

Earlier reports retained for audit:

The integrated Editor build passed according to root's 13 September report, including a corrected SpaceBar key constant. I read the first actual [automation report](unreal/Saved/Automation/index.json): `reportCreatedOn=2026.09.12-23.07.18`, **31 Success, 0 failed**, comprising 21 existing cases and the first 10 expedition cases. Its SHA-256 at review was `7E6258F1F4A0B1501C384F8E11A3C23A9BF4F6921DCEA0240D991D6D51C28141`. The mutable report path may later contain a newer run.

The next actual report, **`2026.09.12-23.36.18`**, contains **50 Success, 1 failed, 0 warnings, 0 not run**: 29 of the then-current 30 expedition cases passed, together with all 21 legacy cases. Its SHA-256 at review was `93D05380C4FC7F5A319A54493E9E5C05C7187447CA5920D7FB5C4EEED6FF93F9`. The full Editor compile passed before that run. The sole failure occurred before the shear interaction: the controlled fixture omitted the equipped Coil needed by Cold Seam/Crack Follower's explicit `Sever` prerequisite. I corrected the fixture to two actives (Vector and Coil), preserving all validation, and added the actual error string to its assertion. The following report exercised that corrected fixture.

The third actual report, **`2026.09.12-23.39.09`**, again contains **50 Success, 1 failed, 0 warnings**. SHA-256: `C2599CAFF806E4011AFE05727CC8888AAB935FD74D9446CA5D0E6166EB5200CA`. The fixture now validated, revealing a real Shear mismatch: preview charged for a normal linked seam while integration required unshown brittleness. The corrected contract cuts an ordinary unanchored linked seam only after it physically crosses the paid plane. The test now uses a clear upward corridor, since the original sideways route could legitimately hit neighboring bundles before crossing; it retains actual movement/sever assertions and diagnostic positions.

The final stable source has **31 expedition tests**. Intact Recovery covers cutting either the generator or its cage, with and without the capstone; the cage variant separately collects the freed device before using it. These branches and the new earned two-route case all passed in the final report. The scoped whitespace check passed. No packaged behavior, visual quality, listening or enjoyment is claimed.

The intended full goal remains thirty meaningful module identities, six different action families, earned shop choices and interacting builds solving physical recovery objectives. A smaller implemented subset is an experiment, not evidence that the full goal is achieved.

## Acceptance targeted by this pass

| Area | Required observable result | Planned evidence |
|---|---|---|
| First earned choice and pivot | Actual world recovery/smelting/dispatch earns the funds used to buy/refit; a useful alternative remains, prices and slot costs apply | Engine automation through real rig/world callbacks |
| Cross-family conductor | Extract, place and conduct through the real arrangement; neither ingredient alone performs the same optional operation | World actions and state/position checks |
| Cross-family counterweight | Weld pays once without firing, switch to Winch and move a supported load using the conserved body | World actions, mass/source lineage and mechanism movement |
| Physical core route | Manipulation resolves real restraints and enables delivery; an anchored core cannot be claimed directly | Actual action/tick/receiver callbacks |
| Failure and correction | Field-off retains unsafe cargo/fuse; first RMB drops all; expiry spills and spends 12 once; goal remains recoverable | Real world update and input-gate checks |
| Save/retry integrity | In-memory save restores rig, world, pending work and checkpoint; retry rolls back all current-site awards/costs/world changes together | Runtime-used serialization and retry methods |
| No free loop | Recapture, transformation, repeated bank/source discharge and restoration cannot mint money/energy/source identity | Callback sequence regressions |
| Pause and cancellation | World/fuse stop together; cancelled Q/F key-up cannot fire; welding never falls through to launch | Runtime-used input gate and world/session update |
| Legacy isolation | `-Expedition` branches before old Start/Load/Save; new schema/path cannot overwrite a legacy career | Source review and in-memory schema tests; no real owner files |

## Early integration concerns

The old actor always constructed `FWorkbenchImpl` and called `Start`, which reads and immediately saves `Saved/Rework`. I flagged that the expedition branch must occur before this path and must route shutdown separately. Root implemented `-Expedition`, `ExpeditionProfile` and `Saved/Expedition`; scene/audio helpers are reused without running old `Start/Load/Save`.

For Q/F aim-and-release controls, root accepted a runtime-used input gate: only release of the matching live slot commits. Switching slots cancels the previous aim; key repeats do not buy another operation. RMB, pause/focus loss, bank/delivery and transitions cancel pending aim. Holding an active stops the ordinary field and freezes the magnet. This must be checked against actual dispatch, not inferred from the helper alone.

Serialization must validate a complete candidate before publishing it. A valid rig paired with an invalid world must not leave a half-restored session. Tests will use in-memory strings and controlled fixtures, never owner progress or a fabricated native earning claim.

## Authored engine coverage

**All 31 cases below passed in the final report.** All names share the `MagnetSweep.Expedition.` prefix:

| Case suffix | Concrete coverage |
|---|---|
| `InputCancellation` | Matching key release, repeat suppression, replacement, cancel and focus recovery through the runtime-used gate |
| `PhysicalCaptureAndFieldOff` | Actual movement before capture; retained cargo and recoverable whole-haul drop |
| `PendingPhysicalPullSave` | Uninterrupted versus restored in-flight world movement and exactly-once payment |
| `UnsafeFieldOffDropAndQuench` | Actual extraction of a lone cell; persistent fuse, timely rescue and one 12-energy failure |
| `AnchoredObjectiveCannotBePlucked` | Final core cannot bypass its restraints, become an immediate receiver reward or disappear into an empty furnace |
| `RuntimeReleaseCancellation` | Real key handler cannot fire on Q-up after rescue/pause; a new deliberate press still works |
| `PhysicalFinalCoreRoute` | Paid collar pull, real ballast and brace placement, core capture and separate physical receiver |
| `EarnedDispatchShopAndPivot` | Actual first-site recovery pays the next shop; real buys, explicit fitting, paid half-price resale and a different active choice |
| `ExtractedConductorCircuit` | Extraction and actual socket placement make a previously impossible Arc operation available; no payment for operating alone |
| `WeldedCounterweightMovesMachine` | Conserved material becomes one paid slug, remains unfired, is placed and enables actual machine movement |
| `RuntimePauseFreezesCommittedWork` | Runtime Tick freezes/resumes position, pending pull, world clock, fuse and paid cost |
| `AtomicRuntimeSaveRetryAndAwards` | Actual rich capture/smelt/milestone, whole-session restoration, repeat bank, rollback, wrong-world/screen/run rejection |
| `LastActiveCannotStrandInitialRun` | Five-credit starting purchase cannot lead to an irrecoverable last-tool sale; unfit/refit remains usable |
| `MassBoundariesAndAtomicGroups` | Real crowded pull crosses safe capacity but respects 36 kg, whole groups and no-cost rejection |
| `LegacySchemaIsolation` | Both real in-memory decoders reject the other mode without replacing their state |
| `FourSiteRuntimeRecoveryAndEarnedRig` | Actual Shift/LMB/RMB/E handlers and Tick complete all four sites, spend earned funds, dispatch and restore the winning archive |
| `RuntimeWeldSubsetAndChosenLaunch` | Actual operation/cargo selection buttons weld only two chosen iron pieces, preserve the others and launch the selected non-first body separately |
| `ZeroEnergySecuredCoreDelivery` | A controlled 24-energy starting fixture spends four real pulls and can still deliver the secured final core at zero |
| `DamagedWeldPreservesSourceAndAppraisal` | Actual captured iron plus a labelled damaged-input fixture retains raw source value and reduced payout through weld, save and real smelt |
| `RuntimeReactionIsOptionalAndCoupled` | Basic six-energy Push remains usable with Reaction fitted; explicit target/partner/direction stages pay twelve and move two real coupled bodies |
| `RuntimeSplitKeepsBasicTransferAndConservesGroups` | Basic ten-energy Transfer remains usable; explicit source/two-receiver stages pay fourteen, move disjoint conserved groups and cancel without refund |
| `RuntimeHeatTransferMovesDangerToChosenIron` | Free preparation/cancel, then a selected four-energy hot-source/iron-sink action moves danger rather than deleting it; save and invalid-repeat protection |
| `RuntimeFieldLoomGuidesActualPaidProjectile` | Three real placement stages commit sixteen energy; a separately paid Rail shot physically follows the bend and consumes one guide |
| `IntactRecoveryPreservesUsableFiniteGenerator` | Actual device extraction and placement compare broken versus functional source; only the preserved device supplies a real charge to the receiver |
| `ClosedCircuitPowersDistinctBranchFromFiniteSource` | A placed conductor closes a physical return; an additional machinery branch consumes a stored charge without generating energy |
| `WalkingGantryMovesRealSupportWithPayload` | An ordinary eight-kilogram brace supplies actual support; the paid gantry moves both support and payload and persists their coupling |
| `ExtractionSupportsChangeHazardAndSeamOutcome` | Insulation changes live-cut heat; Cold Seam and Crack Follower alter the actual adjacent edges without recursively clearing other bundles |
| `VectorBrakeAndPhysicalShearHaveDistinctEffects` | Eddy Brake converts existing motion to heat; paid Shear only cuts after actual material crosses its plane and composes with seam supports |
| `RuntimeSensorRelayRequiresChosenPhysicalImpact` | Marked sensor/receiver reserve one charge; a separately paid shot's actual wall impact operates the terminal, rather than time alone |
| `CycloneConservesActualImpactFragmentsAndPaidRelaunch` | A real welded shot collides, conserves source fragments in a finite orbit, and a selected fragment leaves it only through another paid launch |
| `EarnedSiteTwoBuildsReachBothRefiningRewards` | Two actual starting-shop purchases and first-site runtime completions lead to Winch/Hook machine recovery or Coil/Jaw live-seam recovery; actual smelts plus six alloy must reach 362/366 output, pay both milestones once and restore the earned session |

The counterweight and conductor tests use explicitly labelled, validated equipment fixtures to isolate their world mechanics. They do not prove that their exact full rigs were earned. The separate shop case earns its first clear through actual world actions and uses real purchase/refit callbacks. None creates or reads a filesystem profile. Save callbacks encountered through runtime methods have an empty destination and perform no file operation.

## Source findings and corrections

- **Pause cancelled paid motion:** `SetPaused` originally called `World.CancelPull`, ending an in-flight action rather than freezing it. I reported this; root removed that call. The runtime pause case tests the corrected behavior directly.
- **Placement missed the shown destination:** the first dropped component originally landed approximately 85 units from the magnet. This exceeded the circuit socket, counterweight pad and ballast catch tolerances. I reported this; the world author centered the first/single component on the shown position. Tests use the advertised positions, without hidden counter-offsets.
- **Last-owned-active sale could strand an initial run:** the economy reviewer identified the seven-credit/five-credit-support edge. The rig author added a pre-mutation refusal; the test reproduces that actual sequence.
- **Combined-save phase and checkpoint mismatches:** the economy reviewer identified a forged victory screen and valid same-site checkpoint from another run. Root strengthened transactional phase/seed/history/output validation. The atomic session case exercises rejection and preservation of the valid earned state.
- **Legacy routing:** source now selects `-Expedition` before constructing the legacy runtime. Tick, painting, shutdown and saving route to that branch separately. Expedition uses `ExpeditionProfile` and `Saved/Expedition`; `DemoFresh`/`DemoProfile` are ignored. Asset helpers are reused without old Start/Load/Save. This is static boundary evidence, plus the in-memory schema regression; no real legacy career was inspected.
- **Damaged material welding:** after the first report, I found that welding discounted raw `Value` while invariant validation required raw source value conservation. The first attempted fix using minimum quality would instead destroy healthy inputs' appraisal. The author added a separate conserved `Appraisal` for current payout, leaving structural `Quality` and raw source `Value` distinct. The new regression uses a labelled validated damaged-input fixture, then real welding and smelting; it passed in the second report.
- **Mandatory extra operations:** root's input audit found that merely fitting Reaction Frame or Flow Splitter could force their multi-stage operation. Source now exposes explicit optional Reaction/Split modes and retains ordinary Push/Transfer. The new tests exercise the real mode buttons and matching key releases, including the different costs.
- **Grounding unrelated material:** source review found a global heat suppression check based only on whether any ground endpoint existed. The author changed it to require a reachable circuit branch. This is a static correction; no isolated passing physical Ground witness is claimed.
- **Split receiver range:** source initially omitted receiver B's source-distance limit. The author added the same advertised reach check; the positive real split case passed, but a separate out-of-range B rejection is not covered.
- **Shear support fixture:** the sole second-run failure was the QA fixture's missing Coil capability, before physical shear executed. Source allows seam effects from a Shear crossing but requires an equipped Coil to activate those passive supports. The corrected fixture respects that actual loadout constraint. This is not evidence that Shear alone provides the `Sever` capability.
- **Shear's hidden precondition:** the third run reached the action and failed the real sever checks. Source charged for ordinary linked scrap but required `bBrittle` at crossing, with no such instruction/refusal in preview. The author removed that hidden prerequisite while preserving physical crossing and anchored-object protection. The final test passed through an unobstructed upward route, with actual trajectory and severing evidence.
- **Intact Recovery could be bypassed from the other end:** the world author found that cutting the cage preserved the unselected generator without the capstone. Source now applies preservation/loss and its forecast cost to the assembly losing its last mounting, whichever member was selected. The final passing test covers both selection directions with and without the upgrade, followed by real separate collection/placement/source use.

## Coverage limits

Engine automation can establish callback/state behavior and deterministic motion in the implemented world. It cannot establish actual key dispatch through a native window, target readability, comfortable aiming, attractive rendering, sound quality, magnetic feel, novice comprehension or voluntary replay. Those remain pending a separately coordinated owner/native playtest. Earlier demo tests and catalogue/economy design checks do not satisfy these new implementation cases.

At the first report the reviewed world advertised 23 implemented module identities; seven proposed catalogue entries were excluded from stock. The second report contains passing physical/callback witnesses for all six new keystones and Flow Splitter. It does not individually verify every advertised support or all six complete earned build strategies. The four site indices currently share the same machinery/circuit/route arrangement, with different objective mass, values and small salvage scatter changes. This is multi-site state progression, not evidence of meaningfully different replay geometry.

The first counterweight implementation used a welded-role prerequisite. It now accepts real resting mass and has a persistent physical constraint: removing an unsecured payload's support settles/damages it, while Gantry moves support with the load. Both new behaviors passed in the second report. The full four-site callback case also passed, including earned purchases, actual recovery/dispatch and archive restoration. It uses the baseline recovery route; it does not prove four-site viability of every advanced build.

Individual support coverage is uneven. Direct passing witnesses cover Insulated Jaw, Cold Seam, Crack Follower, Slug Press, Counterweight Hook, Eddy Brake, Shear Gate, Heat-Sink Mould, Escapement Relay and all six keystones plus Flow Splitter. Rebound Plate is fitted in the Cyclone composition but its separate reflection quality is not independently established. Ground Clip has only the reported static correction. Conductive Tether, Twin Anchor, Ratchet Pawl, Induction Bridge, Impact Fuse and Punch-Through Collar still need isolated causal effect checks; their presence in a catalogue or an earned loadout is not evidence that their advertised effects work. All six active families have passing real-operation witnesses, within the fixture and callback limitations above.

The two second-site reward routes prove that different tools can earn real spending money from physical recovery. They do not prove balance: Coil/Jaw uses a shorter and cheaper setup than Winch/Hook while both receive the same capped four-credit reward. Site-specific opportunities still need to justify the more elaborate capability. A single baseline four-site completion and two earned reward routes do not prove six compelling, viable full-run archetypes.

Final verdict: **all 31 authored expedition tests pass, with no remaining blocker in that automated coverage. The full thirty-module gameplay promise remains only partly verified; native input/presentation, build balance and voluntary replay are untested.** Preserve the isolated package and obtain coordinated actual-player evidence before treating this as a validated game experience.
