# Expedition encounter contracts

2026-09-13. Root accepted these two bounded encounter changes after discussion between the economy/gameplay designer and world engineer. Root owns integration and the authoritative status/decisions. This document records the agreed design and independent critique; it is not build or enjoyment evidence.

The inspected starting implementation is Expedition Lab commit `2045723`, with concurrent layout/save authoring by the world engineer. Sites 0 and 1, including the earned 362/366-output witnesses, remain unchanged. Existing schema-2 worlds retain their original `e1` layout. New layouts need saved identity/revision so a resume or retry cannot silently change the encounter.

## Why change the late sites

The original four sites repeat collar, ballast and brace requirements. Changing their coordinates and core mass does not create four different operations. Carried cargo also bypasses ordinary obstacle collisions, so a purported narrow transport maze would be misleading.

The selected changes make site 2 a maintained balance and optional recovery problem, and site 3 a physical counterweight exchange around the final 20 kg core. Basic recovery remains possible without a rare module. Tools can change the accessible salvage or the load being managed; a menu label alone is not a new capability.

## Site 2 — Balanced Recovery Rack

### Required operation

- Core: 16 kg at `(0,70)`.
- Platforms: `(-250,160)` and `(250,160)`, each radius 60.
- Each platform needs **10–14 kg** of actual available, stationary, non-goal mass; the two totals must differ by at most **2 kg**. Held/pulling material does not count. Display both real totals and all limits before the player moves anything.
- While balanced, the core becomes available. Actual core capture permanently latches recovery; removing a weight afterward never re-anchors or removes secured cargo. Before capture, disturbing the balance closes access again with a clear explanation, without destroying the core.
- A baseline route places the 12 kg ballast on one platform and the 8 kg brace plus two loose 2 kg iron pieces on the other. No paid tool is required. Spare iron permits alternate distributions.
- Disable the original press and swinging arm on this layout. The new challenge is maintaining useful weights while deciding which optional recoveries to attempt, not an unrelated copy of the old hazard sequence.

Suggested authored positions, subject to collision clearance verification:

| Body or marker | Position | Purpose |
|---|---|---|
| Ballast 2, 12 kg | `(-320,30)` | First baseline platform load |
| Brace 3, 8 kg | `(320,30)` | Second baseline platform load |
| Loose iron 12 / 13, 2 kg each | `(280,-20)` / `(324,-20)` | Visible small corrections to balance |
| Supported machine 9, 20 kg | `(-180,-130)` | Optional supported recovery |
| Counterweight support marker | `(-370,-130)` | Existing Hook/Gantry support behavior |
| Brittle iron body 10, 6 kg | `(150,-140)` | Optional physical fracture recovery |
| Intrinsically hot alloy 56, 4 kg | `(330,-170)` | Optional heat-transfer recovery |
| Unused cool iron sink 57, 2 kg | `(380,-170)` | Explicit finite destination for heat |
| Portable generator 58 / cage 59 | `(-420,180)` / `(-374,180)` | Preserve a real finite source |
| Remote receiver 4 | `(-180,-210)` | Release machine 9's mount |
| Remote receiver 11 | `(150,-230)` | Release brittle body 10's mount |

Keep required platforms clear of ordinary seeded piles. Generator docking markers must be within actual conductive contact of the respective receivers, not a visually plausible but unsupported 105-unit gap. Reposition other ordinary pockets when necessary to remove starting overlaps, without changing their values or stable identities.

### Reward distribution

The agreed world-engineer refinement below is authoritative. It preserves the original site-2 raw total **868** while ordinary recoverable pools stop at **208**, below the first **240** milestone. The three optional bodies each contain **220**. Actual damage changes appraisal; raw value is not a guaranteed payout.

| Pool / IDs | Values | Total |
|---|---|---:|
| Loose iron 12–29 | 18 × 1 | 18 |
| Loose copper 30–35 | 6 × 3 | 18 |
| Rich loose alloy 36–45 | 10 × 8 | 80 |
| Bundle alloy 46 / 49 / 52 | 3 × 8 | 24 |
| Bundle iron 47 / 48 / 50 / 51 / 53 / 54 | 6 × 1 | 6 |
| Conductor 6 and ballast 7 / 8 | 10 + 1 + 1 | 12 |
| Generator 58 and cage 59 | 12 + 2 | 14 |
| Ballast 2 and brace 3 | 12 + 8 | 20 |
| Heat sink 57 | 16 | 16 |
| **Ordinary total** | | **208** |
| Supported machine 9 | 220 | 220 |
| Brittle iron body 10 | 220 | 220 |
| Intrinsically hot loose alloy 56 | 220 | 220 |
| **Raw site total** | | **868** |

Body 56 is intrinsically hot and **loose**: remove its former links to cell 55 and sink 57 symmetrically. Jaw insulates heat across a sever; it does not cool already-hot material. A loose hot body is therefore not another cheap Coil/Jaw extraction. Cell 55 remains separate and worth zero.

### Contrasting recovery routes

| Route | Actual capability and choice | Nominal upper-milestone witness |
|---|---|---|
| Mechanical | Winch + Counterweight Hook frees the supported machine; Rail fractures the anchored iron body with a real qualifying impact. Reserve a support weight instead of immediately refining it. | 220 machine + 220 iron + five 8-value loose alloys = **480** |
| Thermal and support | Vector or Relay with Heat-Sink Mould transfers the alloy's heat into the real nearby iron; Winch + Hook frees the other valuable machine. The sink remains hot and has lost its safe salvage use. | 220 cooled alloy + 220 machine + five 8-value loose alloys = **480** |
| Preserved source | Coil + Intact Recovery preserves the freed generator's two actual charges. Move it into contact with each isolated receiver and use Arc to release that receiver's corresponding optional mount. | 220 machine + 220 iron + five 8-value loose alloys = **480** |

These routes leave **168 ordinary output** uncollected and an entire third optional recovery untouched. They require several safe hauls: a 20 kg machine cannot share a 24 kg haul with five 4 kg alloys; even the 6 kg iron plus those alloys would exceed safe capacity. No instant payout or automatic bank is implied.

The static source 5 is disconnected/nonconductive on this layout. Receiver 11 has a remote receiver role, so it cannot use the ordinary terminal's built-in bypass. Each first successful receiver actuation consumes one real charge from an available, free, functional generator, and each receiver can reward its effect only once. A cheap freed conductor still has uses as wire or weight, but is not itself a source. Arc previews, normal Arc, Closed Circuit and delayed relay activation must all respect the chosen physical/source contract; no test-only path or duplicate pulse may create energy or repeat rewards.

Source inspection shows fracture unanchors the iron target without a direct appraisal reduction, and generic impact damage targets alloy/core. The nominal iron 220 is therefore a plausible witness, **not an executed trajectory result**. Collision damage to the machine/alloy, battery budgets, physical support and actual banked 480 still need engine tests. Do not silently award a completion bonus to repair a failed recovery witness.

The 240/480 milestones pay only +2/+2, and site completion retains its existing +14. A 24-credit Intact purchase is not financially repaid by those capped bonuses alone. The new physical source route is a capability hypothesis; it does not prove that the module's price, acquisition path or pleasure is right.

## Site 3 — Counterweight Exchange

This is the corrected contract accepted by root. It supersedes the discussed two-seat cover variant, which depended on a special parked-cover condition. Literal cover occlusion would also let a player park between both controls and expose both at once.

- Final unique core: **20 kg**, cradle `(300,0)`, radius at least 30.
- Initial movable cover/counterweight: **20 kg**, `(220,0)`, physically distinct from the core to avoid starting overlap. Service staging marker `(-80,0)` keeps the return to the cradle within the Relay's 440-unit reach. It is a helpful staging location, not a role-keyed success requirement.
- Core access opens when the live cover is more than 130 units from the cradle. Once the core has actually been secured, access remains latched; no later motion re-anchors carried or staged core.
- Dispatch requires actual available non-goal replacement mass **at least 20 kg** in the vacated cradle, radius 28, **or** the isolated powered receiver latch. Any suitable real mass can substitute for the original counterweight. The core itself never satisfies the replacement test.
- The core's physical footprint prevents preloading that small replacement socket while it occupies the cradle. This requires a collision witness; do not use an invisible lesson/order gate instead.
- Safe core staging marker: `(430,-230)`, inside the tray. Do not direct a drop to the off-tray receiver, where existing Drop clamps bodies back into the worksite.
- Remote receiver 4: `(-250,-70)`; generator contact marker approximately `(-205,-70)`, subject to actual radii/contact verification. Static source 5 is disconnected, and ordinary terminal 11 cannot bypass the receiver. A real preserved source charge is spent on the first latch.
- Final dispatch remains an open cargo delivery to the existing receiver, with no claimed wall collision maze or new cargo physics.

Baseline action sequence: move the counterweight away → secure core → deliberately stage the core → carry the 20 kg counterweight into the vacated cradle → regrip core → dispatch. **Core 20 + counterweight 20 = 40 kg**, over the hard 36 kg limit; the UI should forecast why an attempted combined capture cannot work.

Winch or Relay instead moves the counterweight while the 20 kg core remains secured. That changes load management and avoids dropping/regripping the whole haul. It does not by itself prove a 10-credit tool is worth buying: nominal 8/10 battery versus two extra baseline grips at 12 battery is a modest benefit. The optional site-2 recoveries provide stronger progression comparisons.

Intact + Arc can preserve, position and spend the finite generator source to latch the receiver without filling the replacement cradle. This leaves the counterweight available for another use. A player who moves the core physically before capture or constructs a replacement from several pieces is making a valid alternative plan; do not invalidate it to force the demonstrated sequence.

Mistakes remain recoverable through existing whole-haul drop and full entry retry. Melting the original counterweight must not set the dispatch predicate; another real 20 kg load can substitute. Warn plainly if the player is about to consume needed replacement mass without an alternate route. This is a readability warning, not a hidden prohibition on otherwise valid refining. At zero battery a properly secured, counterbalanced/latch-ready core can still dispatch.

## Independent critique and acceptance targets

1. **Balance may still be chores.** Two simple weight drops are not automatically interesting. Observe whether players compare weights/recovery value, disturb a platform knowingly, or simply follow two markers. Keep the limits visible; do not add more bars to mask weak decisions.
2. **Coil/Jaw dominance is reduced, not disproven.** Anchored optional targets and intrinsic heat cannot be solved by their original extraction shortcut. Verify the real competing routes and depot access; a role tag or a theoretical combination is insufficient.
3. **Expensive preservation needs an honest physical source.** Disconnect the static source and forbid the local-terminal bypass on these circuits. Preserve finite charge on save/retry; a normal repeated Arc or alternate relay path must not manufacture powered recoveries.
4. **Every recovery must bank real appraisal.** Test both proposed 480 routes through normal commands and world Bank → AwardOutput; do not inject cash, output, body state or modules into a supposed player witness. The authored fixture can be selected deterministically through actual saved offers.
5. **Capture and support must not disagree.** Before site-2 capture, removing platform mass changes access. After actual core capture, releasing a platform must not claw the core back. Site-3 substitution uses available mass only, never carried cargo, banked bodies or the mission core.
6. **Final recovery is not a corridor.** Verify body footprint blocks early socket loading, then verify legal remote placement while the 20 kg core remains held. Repeated tiny casts or enforced waiting cannot substitute for the physical load choice.
7. **Readable purpose precedes operation.** Show a short recovery objective, the next unsatisfied physical condition, actual mass/output, and a concrete reward. Avoid reintroducing tutorial smelt refusals or presenting several unrelated commands as the active goal.

Short playtest question: **“Which recovery did your rig let you attempt, and what did you deliberately leave behind?”** For the final site ask **“Why did you stage the core—or how did your rig let you keep it?”** Record what the player actually does and understands, not only their answer after coaching.

Smallest next iteration: integrate these two layout definitions and their saved predicates, run the baseline and two earned 480-output routes plus the final core exchange, then inspect the packaged scenes and observe an uncoached play session. No new currency, metagame, mandatory rare key, lore or additional tool system is part of this contract. No enjoyment or economic-balance claim is currently established.

## Read-only integration review

The first source review found these concrete issues and returned them to their owners:

- The inherited ordinary copper row could start on a balance platform. The authored positions now clear both platforms. A separate iron/receiver starting overlap was moved by the world engineer.
- Generic Hook support could release the brittle target as well as the supported machine, making one tool duplicate the intended paired route. The world now restricts counterweight lifting to the physically supported payload. The initial cable/support relationship must also be visible, not only a role check.
- A delayed relay originally bypassed the isolated receiver's source check. The late-layout path now reserves a real generator charge before waiting for the sensor impact. Normal activation depletes a real source; powered-terminal history prevents repeating a reward.
- The final remote return path originally clipped the partition even though carried baseline cargo ignored it. The engineer moved the partition to `(80,-110)`, half-size `(12,60)`: its upper edge at -50 clears the 28-radius counterweight along the demonstrated y=0 path. This is geometric clearance, not an executed movement result.
- Old generic hints demanded an empty haul, promised another choice after final victory, and directed late Arc users to a nonexistent static conductor route. Runtime/world owners corrected those messages toward actual mass, final rig archive, and a preserved generator followed by an explicit Arc operation.
- Powering the supported machine after an earlier Hook lift must detach its obsolete support constraint; otherwise removing the weight can re-anchor a supposedly released machine. This interaction was returned to the world engineer for correction/verification.

The root is adding a `ClosedReturn` opportunity prerequisite for Closed Circuit, since a live generator alone does not create its required return circuit. This prevents a misleading late offer, but exposes a **remaining gameplay limitation**: a 24-credit Closed Circuit bought early has no function in either current late layout. Final-site removal of the supported payload also removes the current Gantry/Reaction opportunity. Honest filtering is necessary; it does not establish that this forced pivot is rewarding. An earned ultimate tool becoming resale inventory before the climax could repeat the owner's concern about upgrades that do not matter.

The smallest follow-on encounter question is whether to supply a real late return circuit or supported payload for an earned keystone, while retaining the baseline route and actual finite-source constraints. That is a proposal for root, not an implemented extra system or permission to rewrite these contracts silently. No current source review proves that all thirty modules have useful opportunities on every site, or that the final operation pays off every expensive build.

### Current boundary evidence update

Root reports **69/69 engine cases passed, report `00.38.45`**, before freezing source for 0.6.1 packaging. Reported executed cases include the default four-site completion, banked Winch/Rail 480, banked Coil/Arc 480 followed by final powered dispatch, final Winch/Relay handling, and mixed 12+8 kg replacement support. These use found-gear loadouts: physical output, credits and battery are earned, but the cases do not prove the priced shop-acquisition sequences. The thermal/Winch 480 route above is still a separate hypothesis unless independently reported later. This supersedes the earlier lack of route-execution evidence for the specifically named cases; it does not establish enjoyment, native feel, market appeal or 24-credit purchase value.

The final empty-cradle correction was implemented **before the passing 69-case boundary**: replacement recognition now checks physical clearance without requiring `bCoreSecured`. Actual capture remains the permanent access latch and delivery still requires cargo. A separate early-push/early-replacement route has not been independently witnessed; passing the other final routes does not prove every emergent sequence. Suggested guidance for that approach: **“Clear the cradle and replace 20 kg. Secure the core for delivery; stage it if you need room to move the weight.”**

## Next revision proposal — one recoverable power frame

**Root reviewed and selected this as the next bounded mechanics experiment; it is not implemented in 0.6.1.** Retain the tested balanced-core and final-counterweight goals. Add one optional, working **40 kg power frame** opportunity to each of the two late sites. This is one encounter component, not another module collection or metagame. Forty kilograms exceeds the ordinary 36 kg capture limit, so the object must be handled on the floor and received by a visible heavy-load dock. Preserve the packaged `balanced_rack@1` and `counterweight_exchange@1` definitions, including their source lineage, saved worlds and depot previews; expanded fresh definitions need new revisions. An actual affordable acquisition route and competing cheaper method are acceptance requirements, not claims supplied by found-gear fixtures.

The frame is an actual source worth **320 output**, with **two finite electrical charges**, an initial mechanical mounting and a visible support connection. Its dock accepts the real available, settled frame through an explicit **E: RECOVER FRAME +320 OUTPUT** action. Settlement uses the existing source/Appraisal/Banked/Output ledger exactly once. Area entry, repeated pulses, cancelling a move and retry cannot create another payout. A clearly depicted installed frame remains at the dock after handoff; it cannot also be collected or refined elsewhere.

On site 2, this creates a strong optional recovery without changing the existing 208 ordinary pool or three 220 recoveries: the new total would be 1,188. A 320 recovery reaches the first 240 milestone; pairing it with an existing 220 target reaches 540 and the second 480 milestone. The credit upside remains capped at the existing +4. Existing 480 routes and the baseline core route remain available. On the final site, receiving the frame permanently supplies the existing counterbalance mechanism through a visible mechanical connection, and records its actual recovered output in the winning run. It pays **no invented post-victory credits**. This final payoff is a recovered large machine plus an alternate way to stabilize the same core operation, not a new currency or hidden score multiplier.

### Three ways to operate the same frame

| Equipped direction | Input and immediate physical response | What the purchase makes possible |
|---|---|---|
| Walking Gantry | Place real support mass at the frame's support marker, then commit a tow to the receiving dock. The frame and its supporting body move as a maintained pair. | Transport the oversized frame while retaining another haul; support arrives with it rather than remaining at the old foundation. The handoff checks both bodies and their live support, never a Gantry module ID. |
| Reaction Frame | Place a suitable ballast body on the marked opposing side, select it and the frame, then push the ballast away from the destination. The frame advances in the opposite direction according to their real masses. | Move the otherwise ungrippable frame through a different physical setup, using the existing opposite-impulse and centre constraint. Ballast and payload must actually arrive at their receiving positions. |
| Closed Circuit | Complete the visible return path from the frame's own charged source and pulse an exposed input. One branch releases the mount; a second branch reaches a contact physically covered by the frame and enables the receiving hoist. | Electrically reach a connected contact that cannot be aimed at directly while covered. This bypasses a physical reposition-and-expose operation, rather than merely combining two already available button presses. |

The electrical route needs two actual, independently useful receivers and a directed return to the **same real source**, generalized from the existing source-5-specific loop. Spend one real charge per receiver; stop when charges are exhausted. Contact wiring must be made from conductive contact and a real directional return piece. Existing `Links` also create mechanical bundles, so do not bolt the movable frame to fixed terminals by using those links as decorative wires.

The covered contact is a **flush, nonblocking floor contact** under the frame. Manual Arc and delayed-relay targeting use the same actual-footprint exposure rule, while electrical propagation can still reach its wired branch. Its position and circuit are visible as a marked underside connection. Refusal copy: **“Contact covered by the frame. Move the frame, or energize the exposed loop input.”** This must not be a Closed Circuit ID check or an invisible target. An ordinary collidable terminal overlapped with the frame would repel it on release and reveal the contact for free; that would invalidate this distinction.

The receiving hoist reuses committed physical tow/transfer behavior after its stored-power activation. It must actually move the released frame into the dock; no teleport or automatic output award is implied. A normal Arc + Winch route remains valid: release the mount, reposition the frame to expose the contact, restore any needed conducting connection, then activate the receiving contact. Closed Circuit removes that second-tool/setup requirement when the real loop is correctly arranged. This is the proposed new reach benefit that the current simple two-button example lacks.

For mechanical receiving, the frame must be available, unanchored and physically supported, with both bodies at their marked receiving positions. A fixed counterweight left at the old foundation does not satisfy the new receiving support. **Hook + Ratchet must remain a legitimate cheaper staged solution** if its real latch supports the frame while the player repositions the counterweight. Do not disallow it by checking for a particular capstone; Gantry offers continuous coordinated transport, while the cheaper setup spends more handling steps and accepts support-management risk.

### Bounded geometry hypothesis

One compact starting fixture for the final layout, to be verified with the current physics rather than treated as exact-feel tuning:

- Frame 40 kg starts at `(200,-150)`; receiving pad `(264,-102)`, radius 24.
- Receiving support pad `(60,100)`, radius 20.
- Gantry support starts at `(-4,52)`: the intended pair translation is `(64,48)`.
- For Reaction with a real 20 kg ballast, stage the ballast at `(188,196)`. Its intended displacement `(-128,-96)` gives the 40 kg frame `(64,48)` by the existing opposite-mass relationship. The weighted centre is unchanged.
- The proposed short displacement is deliberate. A single damped impulse may stop short while still entering both receiving tolerances; only an actual motion witness can establish that. Reject a layout that requires repetitive identical shoves to cover an arbitrary long distance. Repeated blind micro-adjustments are not the intended challenge.
- Author the support/dock tolerances so the stationary original support cannot also count as arrived. Current counterweight support uses a 55-unit radius; the proposed 80-unit displacement exceeds that radius plus the 20-unit receiving radius. Verify the moving frame, support, terminals, core and existing partition do not start overlapped or collide unexpectedly.

These numbers describe a proposed fixture, not executed movement. The world engineer confirmed the general feasibility of source-based return loops, available-body receiving, active support-pair arrival and shared manual-target occlusion. It still requires bounded receiver/occlusion metadata, transactional source settlement, preview/input support and real tests; it is not just a catalogue-description change.

### Prices, counterarguments and acceptance decision

A relevant free starter plus the existing starting 12 credits and first site's 10+4 payout can afford a 24-credit keystone at the next depot, leaving 2 credits. That early purchase would now have authored uses on both late sites, rather than being forced into resale before the finale. The 24 price and two passive sockets still consume roughly half the run's 48–60 total credits and half its passive capacity; one successful demonstration cannot justify that opportunity cost.

The strongest counterarguments are material:

- **Closed Circuit must actually skip inaccessible physical setup.** If its contact is already manually targetable, moves out from under the frame automatically, or is not needed by the receiver, it has reverted to an eight-battery saving. Do not retain a 24-credit ultimate classification on that evidence alone; reprice/reclassify it in a separate explicit economy decision if the stronger interaction fails.
- **A 320 payout cannot alone repay a 24-credit purchase.** Site-2 credits are still capped at +4, and final output matters only as actual recovery/records. Do not call the capstone financially self-funding. The purchase must earn its appeal through oversized, controllable recovery and a different plan.
- **The frame could become a tedious bonus chore.** Keep one conspicuous optional assembly, clearly show its payout and contribution, and permit immediate baseline core completion. Do not expand it into several identical frames or another mandatory lock checklist.
- **Cheaper methods may still dominate.** Hook + Ratchet or Arc + Winch should remain valid. Compare their actual setup, mistakes, battery use, active/passive occupancy and player preference against the capstone route. Do not manufacture exclusivity by rejecting successful physical arrangements.
- **The new dock adds another destination.** Its heavy-load label, explicit E handoff, actual installed frame and single payout must distinguish it from the scrap furnace and core receiver. It must not repeat the earlier tutorial's unexplained deposit refusal.

The next acceptance increment is therefore one complete optional frame with all three physical approaches, one valid cheaper staged approach, honest settlement and clear previews, while preserving the current default core routes. Show that a pre-existing 24-credit purchase remains useful across the last two sites. Ask the player: **“Would you choose this rig again for another large recovery, and what did its expensive part let you avoid or accomplish?”** Native handling, voluntary choice and price appeal remain unproved until those comparisons are played.
