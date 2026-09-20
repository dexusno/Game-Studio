# Executed lifecycle candidate

Strict MSVC Release execution passed **207 named physical-type groups, 59,169
assertions, zero failures**, plus the isolated CTest target. All 207 physical
output types now have proposed lifecycle evidence. This is author-operated
coverage awaiting independent review, not full recipe-effect or release
acceptance. No active support binding or production rule was changed.

## Corrected SH066 interpretation

The prior 206-type report was wrong to require player-selected Shield payment.
`design/TIMING-AND-PERSISTENCE.md` section 3 explicitly settles this automatic
conversion as `3 * min(3, availableShield // 3)`, after enemy actions and before
retention/reset. T05/T06 require ordered current-balance reads and preserving
incomplete remainders. The existing production implementation matches it;
no new owner decision or production correction was required.

The added exact group, `part:recipe-SH066 materialization/copy/resale/place/lifetime`,
executes paid and granted physical identity, reserve storage, noneligible-copy
rejection, sale/receipt and cleanup through the common Modifier matrix, plus:

- Every active Shield amount from 0 through 14, including the three-Iron cap
  and preserved 1-2-point remainders; no action selector is supplied.
- Reserve exclusion and an actual enemy hit reducing 12 Shield to 8 before
  Sweep spends 6, records 2 Iron, and leaves 2 for retention.
- MA055 reading before or after Sweep: 5 versus 3 total delivered Iron from
  an initial 12 Shield, with only 3 retained in either case.
- Same-name refresh without stacking or moving the first binding's priority;
  SH060's pre-reset loss before/after Sweep yields 0 versus 3 recorded Iron.
- Exact preview/apply and snapshot parity, event ordering after enemies and
  before retention, next-turn delivery, one-round expiry and armed cleanup.

MY3-03 is acquired directly as a **controlled retention observation aid**.
It is in this manifest's enabled Cinderwall Mayor tactical pool and the
152-upgrade set; the source has `min_city=1, max_city=1`. Its ID prefix does not
make it out of scope. These fixtures do not prove natural acquisition of that
Mayor. The ordering probes use eligible MA055/SH060, not the out-of-pool Noor
and MY3-21 effects named in the source examples.

## Exact evidence and limits

[`../contracts.json`](../contracts.json) lists all 207 exact identifiers,
executed producer paths, source resale expectations, copy eligibility and
inapplicable lifecycle clauses. [`proposed-bindings.json`](proposed-bindings.json)
maps them to actual PASS groups; the checker verifies the complete exact
PASS/log/result/proposal correspondence.

| Identity | SHA256 / value |
| --- | --- |
| Build | `native-part-lifecycle-2fa5fdc71858ffe3` |
| Test executable | `2fa5fdc71858ffe34467e24e758b3e51d9bd607474ccdb445fe8959a782efd69` |
| Shared library | `9ba5eff9203aa7e545f53e9469a5bb45b3f9bc4bc0887f6db22c25eea14815fc` |
| Complete 41-input graph | `6d015efbe36365278c4b22263cbe9ab1b733cf8c57681d76715344115ef8d13f` |
| Manifest | `d56bf01290f16567cd7009a64f4d49f825454a7149fbf6a922a695a4b71deab9` |
| Results JSON | `b6109b4352f41d39ecd65bae31678cd07b83477635de2d35172bbd6079f87496` |
| Proposed pairs | `f366f2beed74930b7db235c0018f036dfe2350b936d046313fa81e72912067bd` |
| Contracts inventory | `9ef773eab5a9b3af9e082904ce53bd09d469bea65c9cafa94f2dd6d259afb87e` |

Captured UTC: `2026-09-20T17:50:39.444617+00:00`. [`results.json`](results.json)
records configure/build/CTest commands, process exits and all named results
from [`parts.log`](parts.log). Inputs matched before and after execution.
Source copies, executable, library and exact logs are retained under
`core/build/part-lifecycle/captures/native-part-lifecycle-2fa5fdc71858ffe3-6d015efbe363`. `capture.py --check` verifies current and archived
identities without re-executing the native tests.

The unchanged generated-family limits still apply: SH076 uses controlled fixed
printed Shield inputs, without proving natural availability of every amount.
Generated Shield 1 has no player-action interval between End Turn creation and
reset/terminal cleanup; no removal, positive sale or copying window is invented.
Six typed Magnets use paid Craft selectors, with no no-choice factory claim.
All producer triggers, every recipe effect, balance, visuals, usability and
the rest of the runtime gate remain separate obligations.

This graph includes the provisional terminal-notification correction at
`core.cpp` SHA256 `86198fda6e4f4c8d5dfd5a32a203242ba789ace9a91ecfba77ce702dc5e7895d`.
It remains **corrected of-core-0.4 pending the owner's save/version decision**.
Any later source/version change requires a new identity, not relabelling.
The original 123-type and 206-type graphs/reports/artifacts remain preserved;
their compact-file hashes were rechecked unchanged. The archived 206 report
retains the mistaken interpretation as historical text with a correction note
in its archive index. The earlier 133-binding audit is untouched.

Independent QA should rebuild `core/tests/parts/CMakeLists.txt`, compare the
new SH066 cases to the settled timing clauses, and sample proposed type-level
sufficiency. The [parent README](../README.md) gives commands and fixture limits.
Root owns review/integration; no bindings activate automatically.
