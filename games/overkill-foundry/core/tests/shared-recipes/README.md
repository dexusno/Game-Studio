# Shared recipe semantic contracts

This separate author suite exercises the 101 Shared recipe operations left
unbound by the historical 133-operation runtime ledger. It uses the production
`Rules` engine and the real `CampaignRules(cinderwallUpgradeHooks)` cleanup path.
No global support binding or release eligibility is changed. The evidence map
is **proposed only**, pending independent clause review.

The suite has 101 primary recipe groups and 12 focused boundary groups. Expected
effects are authored from `RECIPE-CATALOGUE.md` and the selected precedence in
`TIMING-AND-PERSISTENCE.md`. `source_index.py` exports the exact source rows and
material/cooldown oracles from the catalogue snapshot; it reads no engine effect
implementation. Test helpers validate each real paid Use, its physical memory
copy, material event and cooldown, reject insufficient materials atomically,
and compare every successful action's preview with its exact committed state
and events. Named groups test the recipe effect itself, rather than treating
registration or materialization as semantic coverage.

There are **100 proposed full recipe bindings**. **SH064 stays unproposed**:
the actual Cinderwall Chassis action proves Burn prevention, its once-only and
round expiry, and the Fouling/Shield Leak exclusions. The enabled roster has no
demonstrated application producer for player Corrosion, Mark or Weaken through
the same Engine callback. Their three prevention branches are not claimed merely
because the conditional is visible in source. Controlled committed robot actions
exercise available mechanics; they do not prove natural formation availability.

The suite controls enemy HP/status/intent and plentiful supplies to isolate
mechanics, generally using quiet generic targets. It is not a balance run or
proof that every state/loadout occurs naturally. Fixed Shield/retention fixtures
make SH066's leftover observable; actual paid MA055/SH060 combinations also prove
the selected ordered-reader behavior. Copy cases preserve immutable source
references and exercise actual copied effects. The separate physical lifecycle
matrix remains supporting evidence for its narrower type-level claims, not a
substitute for these recipe effects. No rendering or human playability is tested.

## Source-defined correction

The failed native run archived by `history/delayed-shield-schedules.json` showed
SH092 and SH113 losing their next-round 15/20 Shield when removed after first
installation. Selected catalogue clauses 102/106 and the timing contract assign
implicit schedule origin to first installation and preserve committed deliveries
after removal. The authorized production change moves only those two unconditional
schedules from End Turn to first installation. Reactive SH092 Mark still requires
its source installed; SH113 cooling still checks current Shield and source
presence at its ordered pre-reset boundary. Tests cover saved/reinstalled sources,
no duplication/rearming, both reader orders, the 10-Shield threshold, live cooling
at delivery, death interruption and real campaign completion cleanup.

The failed executable and full copied inputs remain in the ignored archive
identified by the historical JSON. They are never relabeled as passing evidence.
The correction joins the other provisional `of-core-0.4` changes; the owner's
save migration/version decision remains pending. No version/serialization guard
was changed here. Earlier upgrade and lifecycle captures remain unchanged.

## Reproduce

From the game directory, with `cmake` on PATH:

```text
python core/tests/shared-recipes/source_index.py
cmake -S core/tests/shared-recipes -B core/build/shared-recipes -G "Visual Studio 17 2022" -A x64
cmake --build core/build/shared-recipes --config Release --parallel 4
ctest --test-dir core/build/shared-recipes -C Release --output-on-failure
python core/tests/shared-recipes/capture.py --capture
python core/tests/shared-recipes/capture.py --check
```

The capture helper accepts `--cmake PATH`; its default is the verified bundled
Visual Studio CMake. It performs a clean strict `/W4 /WX /permissive-` x64 build,
executes both suites and CTest, copies every input plus binaries/library/logs,
and compares source hashes before, after and against the copies. The archive
identity is derived from the actual executable and full input graph. `--check`
verifies all identities and actual named results without rebuilding. Evidence
contains portable game-relative paths; bulk binaries remain under ignored
`core/build/shared-recipes/captures/`.

`evidence/results.json` is the exact build/report record.
`evidence/proposed-bindings.json` lists every proposed ID, linked passing group,
source clause, implementation identity and explicit residual. A later source
change invalidates the live `--check`; the archived input graph remains the
historical artifact for independent review. Rebuild from that copy to review the
same candidate rather than silently combining it with later production changes.
