# Independent campaign, route and session review

Completed bounded review, 20 September 2026: **20 independent groups passed, with 12 actual Windows process exits**. Four findings (five failing probes) were repaired by the implementation author and independently rechecked. No reproduced failures remain in this batch. This accepts the tested transaction behavior on the exact frozen candidate below, not P08 upgrade effects, full-city gameplay or a release.

## Candidate and reproduction

Reviewer `/root/foundation_qa` owns `qa/campaign/` and this report. Integration owner `/root` authored and repaired the production campaign code. No production code was changed by the reviewer. The separate recipe review is [expanded-review.md](expanded-review.md).

The initial candidate was commit **`bebb22da70033d913831680189a6bc11b2c3e4df`**. Before P08 development began, the reviewer copied and hash-verified all 21 core/platform source inputs into an ignored build snapshot. Rechecks overlaid **only three root-authored fix files** on that baseline. Live P08 core and adapter changes are absent from this candidate. Rules are `of-core-0.3`, content `cinderwall-recipes-0.2`; the core snapshot remains schema 3 and the repaired campaign snapshot is schema 2 in `OFCAMP01`.

Final execution: **2026-09-20 12:50:29 UTC**, Windows 11 Pro 10.0.26200, x64 Release, MSVC 19.44.35228, Windows SDK 10.0.26100.0. All review source hashes were unchanged before/after compilation and execution. Controls are public `CampaignAction`, `Action`, `CampaignSession` and native Windows process APIs. No graphical client was used.

| Final artifact | SHA-256 |
| --- | --- |
| Independent `campaign_probes.exe` | `b11bffb99dd917c24e38b093695b50c7e273a4d1ff1b3d3cef7834979cda64ca` |
| Fixed `campaign.hpp` | `784290aba108c47e7777092752a24affee4d3647069004d9ee6134ca732d918f` |
| Fixed `campaign.cpp` | `a67fd30516284a0cf65d9b13fdc83dbf7f6797c3af6c740c7659fb33b239fb25` |
| Fixed `campaign_serialization.cpp` | `cd3f2027d218ab76d2c0d2b6317dd3db427f5e8bca0bee334dfdde53982d9699` |
| Frozen baseline `core.hpp` | `529790907046d36de3912ab5946b1993028b853809d5b6269ddf487d29d2c1cd` |
| Unchanged `campaign_session.cpp` | `4dfe4208bfa7d23041315901cf822c5f539715dbe00dbe57d9ae9b293ccb39c5` |

Evidence: [all source/probe/build identities](campaign/evidence/identity.json), [final execution log](campaign/evidence/runtime-results.txt), [initial failing log](campaign/evidence/baseline-results.txt), [initial source identity](campaign/evidence/baseline-source-identity.json), and [the three author fixes as a patch](campaign/evidence/root-fixes.patch). The patch preserves the precise reviewed candidate while later P08 integration proceeds. It is evidence, not another production implementation.

To reproduce, use an isolated checkout of the baseline commit, apply the evidence patch there, then run this review's `qa/campaign/run.ps1 -SourceRoot <that-checkout>/games/overkill-foundry`. The existing local ignored snapshot is also the script's default. The wrapper builds in its own directory and verifies source stability. Save fixtures stay inside its ignored `build/artifacts/` directory. Archived logs contain no private machine paths.

## Findings and repair outcomes

The initial 20-group candidate ran **15 passed / 5 failed**, executable SHA-256 `3ea691a3f50d0991537c5afc78122459c15d05e9d18d473742d8f082112d02ec`. The first eight flow checks and all twelve process-exit cases passed even before fixes; the failures below were not inferred from an author test count.

| Finding | Severity | Reproduction, expected and actual | Contract / affected source | Final recheck |
| --- | --- | --- | --- | --- |
| C01: queued transaction crosses New Game boundary | P1 | In a session, start `old-run`, choose Mayor, and retain a Buy command at sequence 2. Start `new-run` with the same seed, choose Mayor, then submit the old command. Expected rejection with the replacement run unchanged. Actual: the new run paid for and received the old purchase because object/sequence numbers coincided. | Timing §6 requires campaign identity plus monotonic transaction sequence. `CampaignAction`, `CampaignRules::apply` and receipt lookup. | Mandatory `runId` now rejects mismatch before checking receipts; old command fails and new-run state is byte-identical. Legitimate helpers explicitly copy the current run ID. |
| C02: contradictory death/phase snapshots accepted | P2 | Construct a controlled Between state with HP 0 / internal Defeat; separately mark a living HP-80 / Collection state as campaign Defeated. Expected save/read rejection. Both previously serialized and deserialized successfully. | Timing §§4–6: death closes the campaign and saves retain authoritative phase. `campaign_serialization.cpp::validate`. | Both contradictions reject; an actual lethal enemy action still produces a valid reloadable Defeated snapshot. |
| C03: live fight identity differs from committed encounter | P2 | Enter the saved Ram offer; replace live fight encounter key and increment its seed without changing entry/route. Expected snapshot rejection; actual serialize/read succeeded. | Timing §5 requires fixed selected seed/formation and deterministic entry restart. Campaign validation. | Active fight key/seed must match the committed offer, with the legitimate pre-start exception retained. |
| C04: selected formation changes under original entry identity | P2 | Enter Ram, then swap that selected offer's formation with another legal current formation, retaining offer ID/key/seed. Expected rejection because the checkpoint and current choice disagree; actual save/read succeeded. | Same fixed-encounter contract; checkpoint validation previously compared only offer ID and seed. | Exact committed offer comparison now includes kind, formation, Mystery, district and Binder-history flag. |

C01 is a public session action sequence, requiring no malformed state. C02–C04 deliberately construct inconsistent states to test serializer validation; the review did not establish that normal gameplay creates them. All production fixes were made by the author. Campaign schema advanced to 2 for the changed transaction contract; this review does not claim migration of prior schema-1 saves.

## Passed behavior

- **T11:** completed-fight materials/parts/checkpoint clear before new purchases; bought supplies survive save and the next entry. The fixture's explicit +3-Iron entry hook occurs once. The price is the configured beta price, not a claim that the paper trace's 10-Credit example is the chosen economy.
- **T12:** three repeated Continue cycles restore the original fight bytes, shop stock, HP, charge and entry-hook count. Discovered recipes persist, while attempt-local purchases and controlled charge/HP mutations roll back.
- **T13 / T21:** pending recipe exchange survives save; Back cancels without replacing memory; reopening and committing replaces exactly one copy. Replayed acceptance returns its receipt without repeating acquisition.
- **T19–T22:** Back preserves fixed recipe offers, core claims remain unsold, confirmed abandonment keeps already owned loot, cancel retains pending loot, repeated confirmation advances once, all-loot abandonment reaches city completion, and a controlled empty-reward fixture advances without confirmation or another restock.
- **T18:** a selected noncombat Exchange Mystery completes one node, calls the explicit Mystery fixture hook once, and performs neither the combat hook nor ordinary fight restock.
- **Transaction failures:** changed payload with a reused sequence rejects atomically; an exception before replacement publishes no speculative core claim; an exception after replacement reconciles the committed purchase and makes retry idempotent.
- **Terminal state:** real core enemy damage closes a controlled low-HP campaign with no loot/restock/Continue checkpoint; discoveries survive. A controlled boss-kill transaction records one city-clear/unlock fact, survives reload, and ignores duplicate final Fire via its receipt. Route traversal to that fixture is not a played full-city run.
- **Missing P08 integration:** Mayor acquisition with no effect executor rejects without adding an inert upgrade, spending or committing a receipt.

## Actual process interruption evidence

The independent executable launches a hidden child process and requires observed exit code **73** at each of four save boundaries: temporary write, temporary flush, replacement, final commit flush. It tests three different operations: confirmed partial-loot abandonment, terminal Fire, and full-memory recipe exchange—**12 actual exits**, each logged individually.

After the first two boundaries, a new session recovered exactly the whole previous serialized campaign. After the last two, it recovered exactly the complete new campaign. Retrying the same command performed the missing transaction once or returned its existing receipt; the final bytes matched an uninterrupted reference. These checks cover process termination and injected errors on this Windows backend, not hardware power loss. The author's separate purchase/core-claim/Continue process tests are not counted as independently authored cases here. The independent purchase check used an exception immediately after replacement, rather than another purchase process-exit matrix.

## Limits and readiness

All upgrade/Mayor hooks are explicitly controlled fixtures: charges, a start grant, simple counters and optional part grants. They are not the production P08 effects. Large test Ammo accelerates terminal transaction fixtures; it establishes neither balance nor a feasible normal run. Profile rollback/defeat/unlock behavior was checked, but every Collection screen and every acquisition/nested choice was not traversed.

This batch did not independently enumerate the full route schedule space or every Mystery/offer combination. It exercised committed route choice/identity, a noncombat Exchange, ordinary rewards and a boss result. Calibration/Patrol alternatives, complete live P08 hooks and their future schema, graphical menu/input/focus/restart/audio/exit behavior, packaged startup, performance, full-city play and human usability remain untested here. Save compatibility migration and hardware power loss remain outside the demonstrated boundary.

The repaired **pre-P08** transaction candidate is ready for the next integration step with no open reproduced findings. Later P08 core/adapter builds need a new identity and relevant rechecks; these results must not be presented as acceptance of those later changes.
