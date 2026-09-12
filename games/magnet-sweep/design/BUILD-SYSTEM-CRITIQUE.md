# Build-system critique

Reviewed 13 September 2026, Europe/Oslo. Independent gameplay design critique under the owner's expanded upgrade/build goal. This is a proposal and research record, not implemented behavior or a playtest. Only this document was written. No native input, game launch, owner-profile access, purchases or changes to existing careers occurred.

## Recommendation

**The current inert tray is not strong enough for the requested build-driven game. The magnet interaction could support one if the game becomes a sequence of physical salvage problems with several useful solutions.** A larger shop attached to the same collect-and-smelt loop would repeat the failure Klaus already identified.

Recommend a salvage expedition: choose a starting tool, build a constrained rig through depot choices, and finally release, transport and dispatch a large machine core. Keep attractive magnetic motion, tactile material transformation and clear payouts. Introduce different problems for the rig to solve; do not copy the inspirations' lore, character rosters or card presentation.

The emerging two-active/four-passive loadout, four-site expedition and permanent discoveries/records are reasonable hypotheses. Run equipment/cash resetting is **not yet an owner decision**. Preserve the existing career separately. Hollow Knight and Inscryption do not establish that one persistence model suits every game.

## Primary-source evidence and limits

Sources were checked during 12–13 September 2026. No wiki or player comment is used as authority for these mechanics.

| Source and publication date | Observation | Design inference for this game |
|---|---|---|
| [Team Cherry: Revealing the Power of the Charms](https://www.teamcherry.com.au/blog/revealing-the-power-of-the-charms), **23 February 2016** in page metadata | Charms have different notch costs; fitting occurs at benches. The developer describes utility, recovery, low-health damage and risky wealth effects, alongside collecting charms through exploration and purchases. | Collection and equipped power are different rewards. A limited loadout can make convenience, safety and ambitious combinations compete. Our depot is a natural place to compare and change equipment. |
| [Daniel Mullins: Kaycee's Mod release announcement](https://steamcommunity.com/games/1092790/announcements/detail/3132822729058776859), **17 March 2022** | The separate replay mode adds selectable challenges and unlocks cards, further challenges and logs through challenge levels. | A repeatable mode needs its own goals and changing constraints; replayability is not established by keeping the narrative campaign running forever. |
| [Developer patch V0.28, official Steam news API](https://api.steampowered.com/ISteamNews/GetNewsForApp/v0002/?appid=1092790&count=1&maxlength=0&enddate=1643908623&format=json), **3 February 2022** | Fecundity's copied card no longer inherits Fecundity in Kaycee's Mod; extra attack effects can stack. | Preserve strong combinations while closing the particular recursive edge that produces unbounded resources. Do not flatten all powerful effects to avoid specifying their interactions. |
| [Developer patch V0.30](https://steamcommunity.com/games/1092790/announcements/detail/3132821459653119350), **7 March 2022** | Additional sigils transfer to generated Bees and Rabbits; other transformations also retain added effects. | Shared properties should survive compatible transformations consistently. Extraction, welding, conduction and transport should affect each other's physical results. |
| [Developer patch V0.20](https://steamcommunity.com/games/1092790/announcements/detail/3141821682388976753), **16 December 2021 UTC** | Early difficulty was adjusted using failed-encounter analytics, and an extra special-event row was added before bosses. | Players must encounter useful choices and have a plausible early success route. Expert theory cannot substitute for observing where players fail. |

The charm article is prerelease: some example names/effects changed before or after release. It supports the stated design rationale, not a complete current charm/synergy catalogue. Its publication year was recovered from `datePublished`; the visible page shows only month/day. Several Steam article pages returned image-only extraction, so their developer-authored text and dates were verified through the [official Steam news API](https://api.steampowered.com/ISteamNews/GetNewsForApp/v0002/?appid=1092790&count=100&maxlength=0&enddate=1650000000&format=json), filtering `steam_community_announcements` and author `TheDanman`. These historical patches establish deliberate changes, not that every interaction is balanced today. No source proves our proposed game will be fun or commercially successful.

## What a build must change

The systems discussion now supports these **six candidate action patterns**. None counts as delivered until the required opportunity exists and behaves clearly.

| Build | Repeated player action and meaningful choice | Required worksite opportunity | Weakness or competing use |
|---|---|---|---|
| Surgeon | Aim at a particular joint/component, extract it, then choose the next cut or use the saved room. | Mixed connected assemblies with valuable components, structural joints and consequential surrounding weight. | Cuts cost energy; repeatedly plucking everything should lose efficiency against intact recovery. Automatic material filtering would remove the decision. |
| Slug demolition | Gather expendable metal, weld a slug, choose its mass, aim and launch it to breach or displace something. | Casing, supports and lanes where impact location matters. | Metal used as ammunition is temporarily unavailable for another purpose; a poor shot spends energy and changes the board. If every target simply accepts any slug, this is another key. |
| Conductor | Arrange a conductive path, choose a live source and discharge through the useful sequence. | Finite sources, conductive pieces and geometry that changes what receives the arc. | A source is exhausted once; a bad connection spends potential on a poor route. Passive nearest-target zaps do not qualify as this build. |
| Winch rigger | Set an anchor/counterweight, tension from a useful angle, and move an intact assembly through a route. | Heavy or awkward machinery, obstructions and different viable anchoring positions. | Setup and repositioning cost energy; compact loose salvage can favor other rigs. A universal winch button that ignores geometry is only increased capacity. |
| Vector/repulsion | Position the magnet, aim a shove, separate debris or turn a loose piece into a useful impact. | Clutter, movable blockers and surfaces that make direction and order consequential. | Pushing the wrong object can obstruct the later haul. If repulsion just ejects unwanted material automatically, it becomes a trivial filter. |
| Remote-field architect | Place and reposition field endpoints, then route salvage through the resulting attraction geometry. | Separated working areas, obstructed approaches and a reason to collect indirectly. | Setup competes with direct work; endpoints consume finite energy. An unattended free collector is passive income inside the game, not a distinct strategy. |

All use the same aiming and magnetic vocabulary, with no more than two equipped active-tool bindings. Orbiting protection was challenged and removed from the required six: without actual interceptable threats, it is decorative. It may later be a support effect, but cannot stand in for a demonstrated build.

A scalable catalogue needs a consistent contract for each module: **trigger, eligible target, operation, cost, changed property, allowed interaction and feedback**. An added row can compose existing operations; it cannot claim a new physical capability that the runtime does not implement. More tiers of the same effect are not more upgrade kinds. Require each new module to name a changed decision and an opportunity where it matters, plus an alternative worth equipping instead. Passive modules may modify an active operation, but automatic value filters and universal refunds should not fill the catalogue merely to raise its count.

The final operation must visibly require releasing the core's restraints, clearing/making a transport route, crossing a hazardous working lane and dispatching the intact core. The proposed 20 kg core fits the baseline 24 kg empty rig; that avoids a mandatory capacity purchase. Each required obstacle needs a credible basic method and at least two substantially different tool solutions. Specific tools can make optional routes spectacularly efficient. They must not become coloured keys required to finish.

Three release clamps are not automatically three interesting decisions. Their placement, connections, working windows or effect on the remaining structure must change a sensible sequence. Three stationary hold-to-service bars would remain chores. A bare final core that is immediately plucked and sold would also fail the intended climax.

## Two cross-family combinations with observable consequences

1. **Extraction → conduction:** free a conductive component from an awkward mixed assembly, place it across a gap and extend a finite-source circuit into previously unreachable salvage or a useful mechanism. Without extraction, that component cannot be positioned alone there; without conduction, positioning it cannot operate the circuit. The main objective retains another route. Preserve the component's identity; dropping and recapturing it cannot replenish its charge.
2. **Welding → winching:** make a slug from expendable loose metal and use its mass as a counterweight for intact machinery recovery. Its mass and position change the tension/route the rig can achieve. Without welding, the available loose pieces cannot supply that stable counterweight; without winching, the slug cannot move the machinery through this route. The slug remains the same material, not a second payout.

These were proposed directly to the systems designer and accepted. Both are stronger tests than stacking two percentage bonuses. Their placement, structural and circuit interactions are unbuilt dependencies, not capabilities claimed for Extraction E1.

## Direct debate and economic challenge

The systems designer initially proposed two active/four passive slots **plus** an eight-point power budget, battery use and existing four furnace heats. I challenged the redundant resources. The revised proposal removes the extra power budget and furnace heats from active play: finite site battery is the main expenditure; unsafe cargo retains a situational warning. Fitting constraints live in the depot.

The economy designer initially separated spectacular salvage score completely from spending money. Root challenged decorative score; the revised offer pays two visible once-per-site output milestones, **+2 credits each**, in addition to fixed clear pay. With starting 12 and clear pay 10/12/14, the first three sites provide **48–60 total credits**, before spending. This lets a strong haul improve the next purchase without unbounded score multipliers buying the entire catalogue. This is proposed tuning, not a simulated or played balance result.

I accept that bounded economic consequence, with two conditions: show the next useful reward before the haul, and allow multiple efficient ways to reach it while leaving residue behind. If both milestones require cleaning everything, the system rewards chores. If everyone reaches them automatically, combinations do not meaningfully alter purchases.

Proposed price bands of 4–6, 8–12, 14–18 and 22–26 credits need competing choices, not an automatic quality ladder. A 26-credit keystone consumes more than half the guaranteed expedition income. Require an actual affordable pivot after an early specialist purchase, at least two useful affordable choices at every main depot, and visible tradeoffs between another active tool, a synergistic passive and saving. Half-price selling and free fitting at depots support that experiment. No permanent power grind should be required for the first complete expedition. Permanent blueprint discoveries, records and archived winning rigs are recommended; the owner's progression preference remains pending.

### Remaining disagreement: universal locked casts

The systems proposal briefly specified 100 battery per site and a baseline six-energy, 1.2-second locked-target pull, without automatic repeat. I challenged fixed per-object waiting: it can replace satisfying sweeping with click-and-wait, and make cheap loose iron an irrational energy purchase compared with whole connected groups.

Compare paid continuous attraction with finite-cost discrete field activation using a shared, visible cost framework. The latter should attract several eligible pieces rather than require one click per cheap object. Preview its target set and cost; let motion resolve without an artificial wait afterward. Root accepted testing both in a representative build before selecting a winner. Neither cadence is approved by this critique. Battery costs and durations remain hypotheses. Dropping, travelling and unloading can be free, but productive setup, full-load transformation and intact recovery must then be more efficient for a physical reason; do not restore tension by arbitrarily taxing every small deposit.

## Strongest failure cases

- **A single best rig:** selective extraction plus automatic filtering resolves every layout, or one refund passive is necessary for all successful builds. Test different geometry and finite opportunities before adding more items.
- **Mandatory keys:** a randomly absent arc/weld/anchor module prevents completion. Main-path alternatives must work with plausible remaining energy, not a nominal fallback requiring tedious service bars.
- **Recursive economy:** recapture, weld/split, generated scrap, arcs and reflected effects grant fresh value or refill one another forever. Track source identities through transformations; pay output once, exhaust finite sources once, and break the precise self-generating edge. Allow large finite cascades across fresh targets.
- **Retry and shop farming:** restoring battery/layout while retaining that site's newly earned money or offers. Continue the exact saved state; if a site-entry retry exists, restore wallet, equipment, offers, rewards and resources together. Do not punish ordinary pausing or genuine failure with concealed penalties.
- **Six names, four playstyles:** remote fields and repulsion must alter action sequences and useful targets, not merely automate ordinary sweeping. The required six remain unproven.
- **Failure after removing furnace fuel:** this was initially unspecified. The resolved proposal below defines battery loss, recovery, withdrawal and retry; its comprehensibility and difficulty still need play evidence.
- **Stronger effects without satisfying work:** casting, magnetic impact, welding and final dispatch still need physical and audio payoff. The owner has repeatedly rejected technically functional but dull results. More triggers cannot repair weak interaction by themselves.

## Acceptance before calling the redesign successful

1. On representative worksites, demonstrate all six rows' different input sequence, preferred targets, risk and advantage. Compare equal-cost loadouts. Each needs a situation where another rig is attractive; no one rig should dominate every tested problem merely through universal efficiency.
2. Perform both cross-family combinations and remove each ingredient in turn. The advertised optional operation must disappear or materially change, while a normal completion route remains available.
3. Trace several full shop sequences with the actual offer rules, including an early purchase followed by a pivot. Show the useful decision, price and slot cost; catalogue size and theoretical combination counts do not count.
4. Complete the physical final operation with substantially different rigs and a credible basic route. The player can state what they achieved beyond earning a higher number, and sees the intact machine leave the site.
5. Exercise resource loops, retry restoration and save interruption; verify exact ownership and awards. Preserve impressive finite chains rather than making every effect weak.
6. Observe an unfamiliar player choose an upgrade, predict its use, execute it and explain why the alternative could have been useful. Then observe whether they voluntarily start another expedition to try a different rig after the novelty of the first reward.

The smallest useful next iteration is one representative multi-solution worksite and a final-operation mock-up with contrasting test rigs, followed by direct owner play. It tests whether this expanded salvage promise deserves a large module catalogue. If the experience still reduces to selecting objects and waiting for money, change the interaction or reconsider the concept instead of protecting it with more progression.

## Final failure and stock review — 13 September

Root accepted the following revisions after independent challenge. These are the integrated design recommendation, still not implemented rules or an owner selection of run persistence.

### Failure that the player can predict and correct

1. **Commit and preview:** before an action, show the actual selected material, energy cost and resulting mass/hazard. The starting rig has 24 kg safe capacity and a 36 kg hard attachment limit. Invalid input spends nothing. A paid action retains its spent energy if cancelled; its moved material and world changes remain.
2. **Unsafe haul:** exceeding safe capacity or carrying a live cell starts a visible **3.0-second** fuse. Switching the field off does not erase it. The warning states **“Drop haul before discharge: 12 battery at risk; cargo will spill.”** Planning/pause freezes the world and fuse; resuming restores elapsed time. This is a thinking aid, not free world manipulation.
3. **Player correction:** RMB cancels pending tool/pour intent, switches the field off and drops the entire haul recoverably. It spends no additional energy, but does not refund the action or restore consumed sources. Reacquisition requires a paid operation, initially 6 energy for a valid baseline pull. Place the spill clearly and within reach; do not turn rescue into hunting pieces outside the worksite.
4. **Expired fuse:** subtract 12 battery, clamped at zero; switch tools off, cancel queued work and spill the whole haul. **Do not destroy the item with the highest sale value.** That was an arbitrary financial penalty on top of the lost energy and recovery work, and would encourage banking each rare immediately. Valuable salvage can instead be damaged by the actual hazardous event that strikes it.
5. **Physical hazard:** a clearly marked press stroke or electrical event can disrupt the rig and damage the actually struck fragile salvage's intact state. That direct incident charges the same 12-energy failure cost once. If it also causes an unsafe-haul response, do not charge twice for the same incident. Separate later actions or press strokes remain new events. The full hazard/response must be previewed or visibly telegraphed; do not silently select unrelated valuable cargo. There is no general health bar.
6. **Objective protection:** the unique mission core remains physically recoverable after a mistake, although intact-quality recognition may be lost. Its marked receiver accepts it separately; the furnace accepts scrap and cannot accidentally consume a mission object. Level bounds, supports and recovery placement must prevent an irretrievable core. Protection of completion is not immunity from battery loss or a worse recovery route.
7. **Brownout:** insufficient energy disables paid actions, not movement, inspection, dropping or delivery of secured material. Do not auto-fail at zero while the player is carrying a deliverable objective or a previously paid action is resolving. Show the unavailable action's cost and offer withdrawal/retry. A safe final delivery can still win at zero. Do not pretend to know a complex layout is unsolvable from battery alone.
8. **Withdrawal and retry:** from a clear menu, **Withdraw expedition** ends the attempt and forfeits current-site unbanked materials and unfinished rewards. Previously dispatched permanent discoveries/records remain; ordinary run money/equipment do not become permanent wealth. **Retry site** restores the whole site-entry transaction: world, battery, money, equipment, offers, source states and rewards together. It rolls back new site milestones/loot too. **Continue** resumes the exact current state. None changes the previous separate career.

Permanent discoveries should commit on successful site dispatch. A failed-site find may remain noted as encountered, but cannot unlock a valuable module repeatedly by picking it up and withdrawing. Successful earlier sites stay recognized. Root is recommending fresh expedition equipment with persistent discoveries; the owner has not yet selected that progression policy.

An overload therefore has a legible consequence even without deleting salvage: the committed action is spent, 12 more energy is lost, and rebuilding takes further work. A player can trace the correction to their aim, selected load, timing or route. The proposed 12/100 cost is tuning, not evidence that this amount is forgiving or punishing enough.

### A solvable basic route is not guaranteed victory

The final goal must not require a rare drop, a particular family or buying a capacity upgrade. A complete baseline route should exist from site entry with enough energy for a meaningful mistake/recovery allowance. This does **not** promise success after spending the battery on optional work or mishandling machinery. Author and execute the route; do not infer feasibility from the core weighing only 20 kg.

The systems designer's final revision replaces service timers with three physical operations: pull the preload collar, move ballast into its catch, and place an iron brace before withdrawing the arm pin. E confirms a physically satisfied fitting for free; it is not a hold-to-unlock bar. Actual transport through the press lane follows. This is stronger than three identical services, but remains unproved. Each successful interaction must visibly alter support, access or motion. Tool routes should change sequence, geometry, preservation or energy use; simply replacing one service animation with another does not justify a build.

The capped output reward also remains a real concern. The catalogue mainly supplies physical operations, not universal score multipliers. Each intended milestone needs an authored material/output witness and multiple plausible routes: what gets recovered, why the tool helps, what energy is spent and what residue is intentionally abandoned. Four correct shop ledgers cannot prove any of those physical outcomes. Beyond the credit cap, spare material can still be ammunition, a conductor or ballast; it should be acceptable to dispatch without cleaning it up. Do not add endless reward bars just to prevent sensible departure.

### Concrete four-offer generator

Generate four distinct module offers once at depot entry and save them. Use capability prerequisites, slot fit, current wallet and **actual tagged opportunities in the previewed next site**, not a family colour or a vague synergy score.

| Offer | Required selection rule |
|---|---|
| 1: Improve the current rig | An unowned support that works with an installed tool and has a concrete upcoming use. It must be affordable now without requiring an unoffered prerequisite. |
| 2: A different useful direction | An independently useful active or support that offers a different decision. It must be affordable separately and fit through a clearly previewed refit; show any displaced/deactivated supports and sale proceeds. |
| 3: Other-family possibility | Weighted wildcard from a different family. Missing prerequisites can appear explicitly here, but such an offer cannot satisfy the two-useful-options guarantee. |
| 4: Ambition | A compatible powerful specialist or keystone. It can require saving, trading slots or a later discovery. It does not promise the player's desired combination partner. |

No duplicate offers or already-owned identical passive masquerades as a new choice. A found/free module has zero resale basis; selling and rebuying cannot generate money. There is no reload/retry reroll. The optional 4-credit precharge remains available but **does not substitute for either required module choice**. When the catalogue cannot provide offers 1 and 2, that is a content/stock coverage failure to fix, not permission to relabel an inactive item useful.

This guarantees useful decisions, not a completed build. Selection within eligible pools is weighted and varied; only one current-tool support is guaranteed, exact partners are not, prices/slots remain binding, and the wildcard can tempt a later pivot. Validate the generator across rig states and site tags, including a full rig and an already-owned compatible support.

**Route B repair:** its final depot previously left 21 credits largely useless because the relevant stock was thin. At its 25-credit entry wallet, offer **Rebound Plate 5, Vector Emitter 10, Anchor Winch 10 and Salvage Cyclone 24**, plus precharge 4. The previewed final site must actually support a corner shot, a directional shove route and a viable tow. The player can deepen demolition, refit Arc into Vector/Winch, or commit money and two sockets to Cyclone. Rebound must really work alongside the delayed Impact Fuse; otherwise show a Fuse-to-Plate refit instead of promising an incompatible stack. This repaired row replaces the old stock example, not its historical arithmetic. Spending all remaining cash is not the success criterion.

Root accepted this generator and sent the repaired row to the economy designer. The useful next evidence is an actual generated choice the player can explain, including a rational decision to decline the expensive item. Forced spending, guaranteed perfect combinations and generic coin sinks would hide the problem.

### Remaining play questions

- Can an unfamiliar player predict the exact consequence of expiry, correct it, and still want to continue after one mistake?
- Does the protected core feel recoverable through physical work, rather than silently rescued by the game?
- Do different rigs release and deliver the same goal through visibly different sequences, with the basic route remaining credible?
- Are paid continuous attraction and bounded activations both satisfying enough to compare fairly, without per-object clicking or hidden resource drain?
- Do milestone earnings come from a clever operation, and do the repaired offers give that money a use the player wants?
- After a completed expedition, does the player voluntarily choose another rig rather than merely repeat the easiest route to unlock power?

No paper answer closes these questions. The expanded goal remains open until a representative game demonstrates them.

## Independent integrated-design audit — 13 September 2026

Read the integrated [BUILD-SYSTEM.md](BUILD-SYSTEM.md), the current mechanics/economy papers and the authored economy report. This is a static design audit. I did not run a game, manipulate a window, read an owner profile, implement a feature or conduct another playtest. Existing negative owner feedback remains the strongest actual evidence about the earlier loop.

**Verdict: recommend implementing and challenging this salvage-expedition design; do not describe it as a proven build-driven game.** The integrated proposal now supplies the missing relationship between tools and tasks. It is a substantial change to the old inert tray, openly presented through the integration owner. Its success depends on real constrained motion, circuits, supports and approachable multi-solution worksites. Thirty catalogue identities and six affordable purchase paths do not establish those capabilities.

### Concrete inconsistencies found and their resolution

- **Failure contract:** the mechanics paper retained highest-value deletion and possible destruction of the mandatory device after the integrated recommendation had rejected them. Its current correction now agrees: recoverable spill, 12 battery once, causal damage to actually struck salvage, recoverable mission object and separate receiver. This closes the paper contradiction, not the balance question.
- **Welded counterweight:** a launch-preparation modifier could not credibly promise a slug available to the winch. The revised operation explicitly commits **WELD 4**, produces a real paid body and requires a separate deliberate **LAUNCH 8**. Switching to Winch preserves the body and spent cost. This makes the advertised cross-family chain implementable without assuming a hidden ability.
- **One reliable rescue input:** contextual RMB aim-cancel initially retained cargo, contradicting whole-haul rescue. Root accepted the simpler correction: **LMB release/field-off retains secured cargo; RMB always cancels work and drops the whole haul recoverably.** Welding never arms an automatic shot; press the other active directly to use the slug. Literal active keys are Q and F. A danger response must not require a hidden second right-click because an aiming mode happened to be open.
- **Misleading stock:** Route A's last depot contained an already-owned Jaw; Cold Seam replaces that unused offer. Route C's last depot offered Coil plus supports requiring Coil, so the apparent choice forced the same pivot; Induction Bridge now provides a useful Arc alternative. Route B's repaired stock offers Rebound, Vector, Winch and Cyclone rather than leaving its remaining money without a credible use.
- **A stricter guarantee than the checker:** Route D's second depot arrives with 22 credits and Winch/Hook/Twin Anchor. Gantry 24, Coil 10, Arc 10 and Jaw 5 provided affordable pivots, but no affordable new support for the installed tool. The integrated generator promises both. Root adopted my **Jaw 5 → Ratchet Pawl 6** repair, with a next-site suspended-load/second-task opportunity. The stronger support assertion found two more gaps: C / depot 1 now offers Induction Bridge 16 instead of Jaw, and E / depot 2 offers Heat-Sink Mould 16 instead of Rail. I read all three corrected rows. No chosen transaction or final balance changes. All 24 authored shops now have the reported logical support/alternative coverage; their electrical-gap, heat-transfer and retained-support opportunities remain unbuilt content requirements.

The report read during this audit contains 30 identities and six routes ending with **1 / 16 / 2 / 0 / 3 / 2 credits**, with four visits per route. It explicitly marks physical opportunity verification false. These are authored ledger and capability checks, not generator coverage, attainable refining thresholds, completed physical routes or evidence of enjoyment. Parent-reported negative-case tests are not represented here as a separate test run performed by this reviewer.

With those corrections, I found no remaining blocking contradiction in the integrated paper. I also flagged the mechanics ledger's stale word “XP” for removal: the successor proposal should not silently inherit the previous demo's progression currency. This editorial correction does not establish or alter the new reward design.

### Requirement trace and what would falsify it

| Requirement | What the integrated paper supplies | Observable failure that still rejects the result |
|---|---|---|
| Clear accomplishment | Visible machinery objectives, then release and dispatch of the 20 kg core | The player can only describe filling a bar, or the climax is three disguised wait buttons. |
| Many meaningful upgrades and price tiers | Thirty operation-based identities, competing prices, two active/four passive slots and costly two-slot keystones | Purchases mostly improve the same universal action, or expensive items are automatic replacements. |
| Six different build approaches | Surgery changes release order; demolition prepares/aims mass; conduction builds a path; winching creates supported movement; Vector chooses force direction; Relay places a route | Equal-cost rigs take effectively the same actions in the same order, or one specialist is best everywhere. |
| Cross-family combinations | Extracted conductor plus Arc; welded slug plus Winch; additional reflection/guide and preserved-device chains | Removing one advertised ingredient leaves the same optional operation available, or a prerequisite exists only in descriptive text. |
| Extensible, intelligible interactions | Shared operations, explicit capabilities, source lineage, finite reaction edges and conserved material | Every new row secretly requires an unbuilt bespoke system; an impressive chain duplicates value or is silently truncated. |
| Meaningful risk and correction | Previewed costs, safe/hard mass limits, shared-clock warning, recoverable spill, finite energy and atomic retry | Failure looks unrelated to the chosen move, an ordinary spill strands the core, or repeating safe harvesting dominates every interesting operation. |
| Reward and replay | Earned shop money, two bounded refining rewards, physical optional finds, varied rigs and a recommended run/reset policy | Milestones reward exhaustive cleanup, capped output makes later skilled work pointless, or repetition exists only to unlock required power. |
| Readability and satisfying control | Explicit connections, contextual result previews, field/body feedback and a fair comparison of attraction cadences | Players cannot distinguish field-off from drop, predict what a tool does, feel magnetic weight or voluntarily try another approach. |

The shop should preview an observable next-site condition, such as a hanging gate and a payload, rather than spoil an exact solved sequence. Ratchet Pawl can then suggest a retained-support approach while Coil suggests cutting another route. Capability tags only select plausible offers; the player still needs a reason to want either.

### Strongest remaining objections

**The universal fallback can make the entire shop optional in the wrong sense.** Keeping a basic route protects fairness; making it as easy, safe and rewarding as every specialist makes purchases irrelevant. Compare the same site with a baseline rig and a bought combination. The latter must change reachable geometry, preserved function, setup sequence or useful resource margin. It need not make the baseline impossible. Do not solve this by inserting a coloured key or making the basic route tedious.

**Resource accounting can still replace magnetic play.** A 6-energy activation with a bounded target set may encourage selecting a mathematically full group, waiting, and repeating. Paid continuous attraction may instead feel like a tax on experimentation. Build both around the same real target and consequence previews; compare actual steering, correction and satisfaction. Neither cadence wins by this audit.

**The final machinery can become a puzzle with one intended script.** The collar, ballast and brace are better causal interactions than service timers, but alternative tools must genuinely change order or access. Directional and remote-field rigs especially need physical opportunities that survive after the novelty of their visual effect. Random cosmetic repositioning will not supply another decision.

**The reward plateau remains unresolved by arithmetic.** Author multiple physical recovery routes to each proposed refining threshold with residue left behind. Show when a preservation tool, clever trajectory or shorter setup earns the next purchase; also make departure after the cap sensible. Optional named finds can extend a satisfying operation, but must not become compulsory full-clear work or a prerequisite lottery. The recommended fresh-run equipment/persistent-discovery policy remains an owner preference to settle; it must not silently replace the existing career.

### Focused acceptance scenarios for the next playable increment

1. During a paid weld, release the baseline field, switch to Winch and support the machine without firing. Repeat with RMB: the body drops, remains recoverable and retains its source/cost. Under an unsafe warning, the **first** RMB always rescues by dropping; no mode consumes it as an invisible cancel.
2. Reach the final receiver at zero battery with an already secured core. Separately spill that core near every edge/hazard and recover it through an actual route. Trigger one causal impact/fuse incident and observe one 12-energy charge, not two; retry restores all entry state and claims together.
3. At the repaired B, C and D depots, have an unfamiliar player identify two affordable choices and a concrete next-site use for each before buying. Execute the selected operation. A legally equipable but irrelevant offer fails this test.
4. On shared representative worksites, compare all six family traces above at plausible purchased budgets. Perform the two cross-family chains and remove each ingredient in turn. Demonstrate both a baseline completion and an outcome the old rig could not achieve.
5. Earn the proposed refining rewards through actual salvage, energy and output traces, including a weaker route and an alternative strong route. Leave irrelevant residue. Test a finite impressive chain and adversarial recapture/weld/retry loops without inventing fresh awards.
6. After a complete expedition with a strong rig, ask only: **“What would you do differently on another run?”** A concrete desired approach, followed by voluntary replay, is stronger evidence than agreeing that the catalogue contains many combinations. Record confusion and actual choices as well as completion.

These are acceptance scenarios for implementation and observation, not a claim they have passed. The immediate useful experiment remains one representative multi-solution worksite plus the physical finale mock-up and earned shop choices. It is a test of the full build-driven promise, not a reduction of that promise to whichever systems are easiest to implement.
