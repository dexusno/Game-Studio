# Exact Cinderwall campaign replay

This opt-in mechanics probe replays an existing **fully earned** city witness
through the production `CampaignSession` and the ordinary Stage presentation
boundary. It does not run a new policy, inject a saved START state, grant health
or materials, simulate hitboxes, or decide damage from animation timing.
Rules remain authoritative. Existing camera and animation holds only prevent
the next automated input from racing the presentation.

The historical input is baseline-5 **defensive / Auto / seed 148**, SHA-256
`36c77c625df0e2b40fba6391c012552a1a92669db90a304cdfc4b2227eb46fdd`.
It begins with canonical Mara New Game, chooses MY1-M1, later earns UGS-065, and
avoids the held composition families. All 19 recorded collections use Auto
(`precision=-1`); this is no evidence of human Precision performance.

Expected route: **257 successful commands**, 12 entered positions comprising
nine fights and three noncombat encounters, final position 13 / 80 HP / revision
258 / campaign hash `241e46199e519eef`. The route naturally includes 11 finite
shop openings, four purchases, 11 core sales, 57 crafts, eight installations,
six activations, 39 loads/shots, 11 End Turns, 18 reward claims and five memory
exchanges. The three noncombat positions include Exchange and Technician;
the witness contains two Leave Mystery commands and one accepted calibration.

Natural robot action IDs are `attack`, `charge`, `phase_one_strike`,
`quad_strike`, and `recover`. Other actions/identities, optional upgrade choices,
reward-skip confirmation, all Mystery variants, broad recipe balance, defeat,
and encounter Continue are **not claimed as coverage of this victorious route**.
Separate CPU branches below cover legal defeat/restart and encounter Continue.
Existing profile/Collection and individual roster probes remain separate evidence.

## Source and persistence boundary

Use the explicit reviewed core snapshot:
`Saved/Validation/roster-v001-runtime-20260920-1843/source/core`.
Its complete source digest is
`b915b36ef05991e865abed40e9ebcd6fe82db70e8bf20556ad47633a5e32f2cf`;
rules/content are `of-core-0.4` / `cinderwall-upgrades-0.3`, manifest
`d56bf01290f16567cd7009a64f4d49f825454a7149fbf6a922a695a4b71deab9`.
This deliberately predates the separately reviewed terminal/upgrade repairs in
the live core. A successful snapshot replay is not proof of those newer changes.

The shared `replay.cpp` strictly validates metadata, canonical New Game, run ID,
monotonic sequence, every action/result/hash/ordered event, and final full bytes
**before creating a save**. The recorded START and FINAL payloads are comparison
data only. The CPU adapter additionally checks every decoded command against the
original baseline-5 codec, including an all-fields optional-choice round trip.

The rendered adapter starts a fresh fixed-path production session with the same
seed/run ID, then submits each exact action to
`AFoundryStage::SubmitCampaignAction`. That Host seam shares the normal control
post-commit presentation path. It returns actual committed events before their
presentation consumes them. After each command, the probe compares complete
campaign bytes (including receipt identity/payload), ordered events, hash and
save revision, then opens the real envelope through a fresh `CampaignSession`
and compares its complete state again. Unrelated input/state drift fails the
run; no replacement action or recovery policy conceals it.

The fixed probe profile intentionally bypasses normal profile selection. Normal
independent profile creation/selection was tested by the separate profile
milestone. Existing saves and evidence directories are refused, never replaced.
CAS, flushing, atomic replacement, recovery and receipt policies are unchanged.

## Verified CPU checkpoint

[cpu-checkpoint.json](cpu-checkpoint.json) records the successful preparation:
32 copied source inputs, unchanged before/after; source graph
`ec597c8bb81f69a60f498c2ffee23e1e6203979ee489a770be9128b6c1e70f40`.
Each strict C++17/C++20 executable passes **2,148 assertions**, 257 newly saved
commands, 257 exact duplicate-receipt retries and 257 disk reloads. The two
transaction transcripts are byte-identical and finish at the expected hash.
The separate defeat branch reaches zero HP after 13 legal End Turns; the
subsequent New Game retains all 16 recipes discovered along its earned prefix.

Local immutable preparation is
`Saved/CityProbe/cpu-frozen-20260920-2134` (the identity file contains the actual
UTC completion time). Its `identity.json` SHA-256 is
`00c585d7e57adf40528cb29ce122d44db581af294e9a7c9618a99f86dceb7a35`.
Toolchain: CMake `3.31.6-msvc6`, MSVC `19.44.35228.0`, Windows SDK
`10.0.26100.0`. The earlier rejected pre-Collect End Turn and C++20 `u8path`
deprecation are retained as failed author attempts; neither is reported as
successful gameplay. The final wide-argument CPU executable fixes the latter
without changing gameplay or the shared parser.

Root separately owns the successful Editor `212056` / Game `212212` builds and
the ensuing `CityProbe212245` execution/archive. Their runtime result is not
inferred from this CPU checkpoint.

## CPU preparation

From the repository root, use a new output directory:

```powershell
python games/overkill-foundry/unreal/Tools/city-probe/prepare.py `
  --cmake 'C:/Program Files/Microsoft Visual Studio/2022/Community/Common7/IDE/CommonExtensions/Microsoft/CMake/CMake/bin/cmake.exe' `
  --core-root games/overkill-foundry/unreal/Saved/Validation/roster-v001-runtime-20260920-1843/source/core `
  --trace games/overkill-foundry/core/build/runner-baseline-5/sensitivity-121-170/defensive-auto-148.oftrace `
  --output games/overkill-foundry/unreal/Saved/CityProbe/new-cpu-run
```

The script checks the exact historical trace and core digest, verifies all three
original runner source hashes against the reviewed baseline-5 index, copies the
input graph and verifies every source before/after execution. It builds strict
Release C++17 and C++20 variants (`/W4 /WX /permissive- /utf-8`) and requires their
transaction transcripts to match byte for byte. `identity.json` records complete
commands, source/binary/artifact hashes, tool logs and results.

Each CPU run also submits the same transaction a second time and verifies that
the exact receipt replays without changing revision or any envelope bytes.
Malformed trace/codec inputs reject before the save path is created. Two
separate fresh sessions use only the recorded earned prefix: one collects, then
reloads its disk save and explicitly Continues into the original fight entry; the other uses
ordinary Auto Collect(3) followed by End Turn without attacks until genuine
defeat, checks defeat persistence and rejected Continue, then verifies New Game
and retained discovery facts. These branches do not modify the victorious run.
Their action/event trace and pre-restart defeated save are preserved separately.

## Executed Unreal checkpoint

Editor212056 and Game212212 build successfully. CityProbe212245 completes all
257 saved commands and matches all 781 ordered events and every disk reload
against CPU preparation. The final state is position13, 80 HP, revision258,
hash `241e46199e519eef`. All39 shots present after the loaded camera settles;
the probe checks return to preparation and waits for terminal presentation
before capturing Rewards. All37 expected captures exist.

The ordinary CampaignProbe212713 regression also passes through the normal
control path: position2, 68 HP, 40 receipts, revision42,
hash `58e1d02c7e1f809e`, 16 captures, `physical_input=0`.
The repaired profile C1 validation source is included in these new binaries.

[runtime-checkpoint.json](runtime-checkpoint.json) records93 unchanged source
inputs,68 archived artifacts, both binaries, and296 unchanged content files
reused from the preserved2032 content archive. The module hash is
`10564019ce4ba890cab9607ca9f3d8da432fad24f9845e3c0bdd364693949dc9`;
the Game executable is
`bed36dd7adc1c8aa4e0d1173bd3f2762e7616c2ed8c52a820b54d7c63c203035`.
Root inspects six recorded loaded, multi-enemy, Rewards, Mystery, boss and
completion frames for functional presentation. The owner explicitly rejects
the current graphics/UI quality; that future work remains deferred.

This is an automated earned route using the explicit1843 core. It does not
verify the later live core corrections, human input, all recipe clauses,
balance, package performance or visual approval.

## Unreal invocation and acceptance

Build both Unreal targets with `tools/unreal.ps1` and the same explicit
`-CoreRoot`. The opt-in arguments are:

```text
-FoundryCampaignProbe
-FoundryCityTrace="<absolute reviewed defensive-auto-148.oftrace>"
-FoundrySave="<absolute project Saved/CityProbe/new-render-run/campaign.ofsave>"
-FoundryCityOutput="<absolute project Saved/CityProbe/new-render-run/evidence>"
```

The save parent may exist; `evidence` itself must be new. The adapter runs only
through this opt-in path and leaves the original CampaignProbe unchanged.
It captures a finite set of existing screens at Arrival, Mayor commitment, the
first shop/memory, each encounter/Mystery, first loaded view per fight, two
committed shots, each settled Rewards screen and completion. Expected count is
37, with the exact captured state and camera/busy flags in `captures.jsonl`.
These are mechanical execution records, not renewed visual polish or approval.

Required runtime evidence, still separate from CPU success:

1. Successful Editor and Game target builds with the explicit core digest;
   archive sources and binary hashes before launch and verify them afterward.
2. Exactly 257 `CITY_TRANSACTION ok=1` records, matching the CPU transaction
   transcript, ordered event stream and all durable receipts/reloads.
3. Success marker
   `FOUNDRY_CITY_PROBE_COMPLETE ok=1 commands=257 ... hash=241e46199e519eef position=13 hp=80 revision=258`,
   ordinary process exit, and complete final envelope.
4. Successful Load settles into action view before Fire; Fire and End Turn
   presentation drain before the next command; Rewards follow terminal holds.
   The probe checks these existing boundaries without hitbox gameplay.
5. Inspect a bounded selection of captured stage/Rewards/boss/completion frames
   for an executed ordinary scene. Do not infer human usability, final visual
   acceptance, all-content coverage or shipping-package correctness.

Failure logs `FOUNDRY_CITY_PROBE_COMPLETE ok=0`, preserves partial evidence and
ordinary-exits. The real-time watchdog is a failed test, never a game turn cap or
an alternate gameplay outcome. A later build of the repaired live core needs its
own updated witness/reference; this historical parity contract must not be
silently rewritten to absorb differences.

After runtime execution, `verify_runtime.py --prepared <CPU directory> --runtime
<evidence directory> --log <Unreal log> --binary-identity <wrapper binary JSON>
--report <new report.json>` independently compares the complete ordered
transaction/event transcripts, all 39 shots' settled loaded-view log markers,
finite capture files, final summary and unchanged module/process identity.
It does not inspect artwork quality or claim receipt idempotency was tested by
the rendered pass; that duplicate-command coverage belongs to CPU preparation.
