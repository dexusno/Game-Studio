# Independent physical-part lifecycle review — 20 September 2026

**Finite result: pass for the reviewed lifecycle sample; no reproducible
production defect found.** The independent native probes pass **36 groups /
1,908 assertions**. An isolated rebuild of the unchanged author suite passes
**207 groups / 59,169 assertions**, with its complete output identical to the
captured original. CTest passes 2/2. These results support the declared physical
lifecycle scope; they do not establish full semantics for every producer,
recipe, upgrade or interaction, and do not activate support bindings.

## Exact candidate and environment

Reviewed candidate: `native-part-lifecycle-2fa5fdc71858ffe3`, captured at
`2026-09-20T17:50:39.444617+00:00`. All compilation used its preserved copies in
`core/build/part-lifecycle/captures/native-part-lifecycle-2fa5fdc71858ffe3-6d015efbe363/inputs`.
The live 41-input graph still matched before and after this execution. Working
HEAD was `3611bd7d3eebfd4415e53426eb74193b8215470f`; the hashes below, rather than
HEAD alone, identify the reviewed uncommitted candidate.

| Artifact | SHA256 |
| --- | --- |
| Complete 41-input candidate graph | `6d015efbe36365278c4b22263cbe9ab1b733cf8c57681d76715344115ef8d13f` |
| Author executable | `2fa5fdc71858ffe34467e24e758b3e51d9bd607474ccdb445fe8959a782efd69` |
| Author static core library | `9ba5eff9203aa7e545f53e9469a5bb45b3f9bc4bc0887f6db22c25eea14815fc` |
| Independent executable | `0e04a1100e05a9c93e65a3c0ecddf0f16639c694135183fae5cccaa7ce4fee38` |
| Independently rebuilt author executable | `58089ce29c182bc54f35fb953a7c0db32158bcd0ffacb152c725b1e489048643` |
| Independently rebuilt static core library | `9c1dfbabf7584c24c2f54179e0e31eabad77115792ff2224b66824f94a001a95` |
| Candidate results JSON | `b6109b4352f41d39ecd65bae31678cd07b83477635de2d35172bbd6079f87496` |
| Candidate contracts | `9ef773eab5a9b3af9e082904ce53bd09d469bea65c9cafa94f2dd6d259afb87e` |
| Proposed bindings | `f366f2beed74930b7db235c0018f036dfe2350b936d046313fa81e72912067bd` |

Independent capture: `qa-parts-0e04a1100e05a9c9`,
`2026-09-20T18:16:07.982286+00:00`. Windows build 26200, AMD64, MSVC
19.44.35228.0, Windows SDK 10.0.26100.0, C++17 Release with `/W4 /WX /permissive-`.
Controls were typed `Rules` / `CampaignRules` actions in native CPU fixtures.
No GPU, Unreal instance or native mouse/keyboard session was used.

The captured core includes the narrow uncommitted terminal-notification guard:
`core/src/core.cpp` SHA256
`86198fda6e4f4c8d5dfd5a32a203242ba789ace9a91ecfba77ce702dc5e7895d`.
Its version remains **provisional corrected `of-core-0.4`**, with content
`cinderwall-upgrades-0.3`. The owner’s rules-version/save-migration decision is
pending. This review does not approve compatibility with historical snapshots
or traces and cannot be relabelled as acceptance of a later version/graph.

## Inventory and evidence audit

The independent [audit](parts/audit.py) passes **2,914 mechanical checks**:

- Exact manifest/contract/proposal/PASS correspondence for 207 unique types;
  41 archived source hashes, six archived native/build artifacts, proposal
  implementation identities and result/contract hash links match.
- All 178 recipe-backed case rows were compared directly with catalogue prose
  for source location/text, printed costs, heading rarity, physical kind,
  output count and one-part resale basis. This includes the suffixed Rare
  manual-grab heading; it does not rely on the author's snapshot to derive
  those expectations. The six catalogue Modifier rows explicitly loaded at
  Fire correctly map to runtime Spread.
- Counts reconcile to 64 recipe-backed Ammo (including Warm Rivet), 59
  recipe-backed Shield, 28 planning Modifiers, six Spreads, 21 Magnets,
  14 fixed-value generated Ammo and 15 generated Shield types.
- All 218 named non-free producer-path claims belong to their manifest
  origins. The evidence deliberately leaves **57 other origin-to-type links
  unclaimed across 15 types**; these are listed in the QA evidence JSON.
  This is a disclosed producer-coverage limit, not 57 missing type registrations.
- The eight compact files indexed by the original 123- and 206-type archives
  retain their hashes. Neither historical capture was modified or relabelled.
  The 206 report's mistaken SH066 choice interpretation remains historical.

The author's `generate_cases.py --check` and `capture.py --check` also pass;
these are consistency checks, not independent semantic execution. No author
capture was overwritten. The matrix dispatch, shared lifecycle helpers,
Modifier/Spread/Magnet cases and generated-producer branches were inspected.
The detailed semantic challenge is the finite runtime sample below, not an
independent effect-by-effect certification of 207 identifiers.

## Independent runtime sample

[Probe source](parts/probes.cpp), [observed output](parts/evidence/probes.log),
[CTest output](parts/evidence/ctest.log), and
[machine-readable identities](part-lifecycle-evidence.json) preserve the checks.
Inputs use controlled material stock and three recovering targets; actions
pay the real production costs. No transaction fixture callbacks replace
campaign effects.

| Group | Independent expectation and observed result |
| --- | --- |
| P01 | Paid MA038 makes two unique Warm Rivets with one source-copy identity. Per-part floor at a five-credit reference gives 2 each. SH036 adds 4 to only the selected consumed rivet; SH072 returns exactly one fresh four-damage rivet next turn, strips the attachment, preserves reference/source and does not repeat. The sibling retains round-1 age. Passed. |
| P02 | MA096 rejects insufficient Heat atomically at both selected endpoints, pays 3/8 once, stores 15/30 Shield, preserves depletion and first binding through removal, snapshot and another round, then reinstalls without another payment. MA054 rejects at HP3, leaves HP1 at HP4 and never repays on reinstall. Passed. |
| P03–04 | SH076 converts a paid SH058 carrying UGS-140 to printed 9 damage, and caps SH113 at 14; copied output keeps its own fixed-price reference and loses Fuse. Variable MA096 input rejects. A removed pristine SH026 four-Shield grant remains copyable; two SH102 copies provide only four Shield each and do not repeat the generator's six-Shield delivery. Passed. |
| P05 | Two SH050 physical activations each pay four HP but refresh a single +60%; different MA010 adds its paid +10. A six-damage Slug deals `floor((6+10)*1.6)=25`, then a second Slug deals 6. Passed. |
| P06, six cases | SH005/051/082/110 and MA041/111 operate on an odd 11-damage pre-target shot. Spread floors 50/40/60% as printed, excludes both targets' Mark from its base, uses the extra target's own Armor/Shield, applies its own Burn/Weaken only, and rejects unaffordable Fire before consuming anything. SH110 reservation cannot be installed, Unload releases it and Fire consumes it. Passed. |
| P07, 21 cases | Every Magnet uses Good with Circuit steering and different selectors from the author's main oracle, then duplicate fitting and a partially depleted finite pile. Checks cover amount, preference slots, named material availability, Heavy Lift's two-unit return, Shield grant, same-name refresh, preview/reload event+state parity and no later-haul repetition. Passed. |
| P08 | Perfect Copper stock is just sufficient for baseline, ordinary Precision and earlier SH124; later SH121 cannot invent missing Copper. SH126's two fixed ten-value parts arrive without a pile charge. Unfitted expiry and ordinary bonus with already-spent Precision also pass. |
| P09 | UGS-028's one-Shield object absorbs real damage before reset. With actual MY3-03 retention, its balance can carry but the physical object still disappears; it cannot later be removed. This supports the declared lack of a positive reserve/sale/copy action window. Passed. |
| P10 | SH066 automatically processes totals 0–5, 8–11, 14–16 and 20, including cap/excess, complete groups, remainders and reserve exclusion. Events order enemies → recorded payment → retention → next-turn delivery. A real three-damage attack leaves 14; MA055 before Sweep earns 2+3 Iron, after Sweep earns 3+1 from the remaining five. Duplicate Sweep retains first binding priority. SH060 before/after Sweep yields 0/3 Iron respectively. No repeat next round. Passed. |
| P11 | Production campaign New Game/Mayor/route/collection setup; actual sales grant eight credits for the two Warm Rivets and receipt replay grants nothing further. An escape result clears a saved part and armed SH126, then survives canonical campaign reload. Passed. |

The initial QA draft had two incorrect oracle assumptions: it expected Heat11
despite the selected cap10, and zero MA055 Iron at five remaining Shield despite
its one-per-five rule. Those expectations were corrected from the source before
the final capture; neither was a production defect. No production changes were
needed for this review.

## Sufficiency and remaining limits

The declared distinctions are supported in this sample: fresh copies versus
used/depleted parts; immutable one-part prices versus actual producer/batch
costs; source payment history versus first installation; activated versus
loaded physical objects; and ordinary reserve life versus Magnet/Shield1 expiry.
SH066's automatic contract is explicit in TIMING section 3, and the eligible
MA055/SH060 cases exercise the T05/T06 ordering principle without claiming
out-of-pool Noor or MY3-21 implementation.

The 207 proposed links remain **type-level lifecycle evidence**. They must not
silently become full recipe/upgrade bindings. In particular, the author's
SH076 full 1–14 output sweep supplies controlled fixed Shield inputs; natural
availability of every input/output is unproved. Additional producer origins,
all conditional payload branches, all upgrade interactions and every combined
Magnet/finite-pile ordering were not independently tested. Six choice-bearing
Magnets use actual paid Craft; the no-choice `grantPart` factory is not claimed
for them. Shield1's absent action window is an explicit inapplicability, not a
successful removal/sale/copy test.

Normal reset, snapshot reload and one production campaign escape cleanup were
independently executed. Broad victory/defeat cleanup remains author-operated
matrix evidence, reproduced by the unchanged rebuilt suite. Historical save
migration, crash recovery, real-time input/focus, audio, graphical startup/exit,
packaging, natural full-city reachability, balance, fun and human visual approval
were not run in this CPU-only assignment. The existing held haul compositions
remain outside this review.

The candidate is ready for root's scoped evidence-integration decision with
these limits. Version/save policy and any subsequent source change require
their own exact checkpoint. No production, active bindings, version files or
shared status records were edited by this reviewer.
