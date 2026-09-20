# Independent Shared recipe contract review — 20 September 2026

Bounded provisional pass for **99 of the original 100 proposed IDs**. Withhold SH084 because its crafting-type definition needs an owner decision; retain SH064's existing unproved prevention branches. No additional runtime defect was reproduced. This review does not activate bindings or accept a save version. Exact per-ID dispositions, printed clauses and cited fixture locations are in [clause-review.json](shared-recipes/clause-review.json); machine identities are in [shared-recipe-evidence.json](shared-recipe-evidence.json).

## Reviewed artifact and method

Reviewed the frozen 48-input graph `9b4b7a2b909c9da1782b94336511442a743a99e75625d62f2754616464529d89`, captured under `core/build/shared-recipes/captures/native-shared-recipes-2fc3c09f61506dcf-9b4b7a2b909c`. Author executable SHA-256: `2fc3c09f61506dcf42dd0782118e9f1527da6004829f58aa64f7e9fa416aec83`. The captured source includes the narrow terminal guard and corrected SH092/SH113 schedules under provisional `of-core-0.4`; the owner save-version/migration decision remains unresolved. Later UC01 and MA082 edits are outside this graph.

Independently read all 101 historical unbound Shared source rows and all 113 primary/supplemental fixture bodies, including their common action/preview/snapshot helpers. Compared conditions, thresholds, caps, ordering, expiry and exclusions with the selected catalogue and timing contracts. The provisional set follows those actual finite assertions; registration, an ID label or a suite total is insufficient by itself. SH092/SH113 use the selected first-installation clarifications at catalogue lines 102–106, retaining SH113's explicit later pre-reset check, rather than the superseded generic Shield activation wording.

Rebuilt the **unchanged captured sources** on Windows x64 with MSVC 19.44.35228.0, SDK 10.0.26100.0, C++17 Release and `/W4 /WX /permissive-`. CTest passed **2/2**: 113 contract groups / 27,881 assertions and 2,055 legacy recipe assertions. Independently rebuilt contract executable: `1553772060cab6bb8b2e4ded90c7a763d9177cf70914c60e2651bac4ae195e38`.

The separate [independent probes](shared-recipes/probes.cpp) link only the rebuilt frozen library. Their **7 groups / 469 assertions pass**, including exact preview-versus-action events/state, nonmutating preview and canonical snapshot reload checks. Executable SHA-256: `d5980cf2d7db28b45e760c8f8ed7f412bfb050ba6722b9fecb25ffcd66003c12`. These are controlled CPU fixtures using paid typed Rules actions, explicit quiet targets and prepared materials; they are not naturally earned campaign play.

## Independent runtime coverage

| Probe | Observed result |
|---|---|
| S01 | Reserve ageing does not schedule SH092/SH113. First installation schedules once. Removal/reinstallation cannot duplicate or cancel the fixed promise; an old reinstalled source does not renew its original-round hook. |
| S02 | SH092 marks the first attack that actually removes Shield, ignoring a zero-damage attack. The separately delivered ordinary Shield does not inherit the reactive hook. |
| S03 | SH113 reads actual pre-reset Shield in original binding order. An earlier SH060 discard blocks cooling; a later discard does not. Remove/reinstall changes protection order without changing reader order. |
| S04 | SH056 delayed damage and lethal Mesh recoil respect delivery order. Death before a scheduled Shield stops its delivery; an earlier Shield absorbs recoil and permits survival. |
| S05 | SH113 cooling observes real per-copy clocks: old cooldown ticks normally then cools; a current-round Use skips natural ticking but can cool. Global cooling does not refresh a used `None` recipe. |
| S06 | Different physical delayed Shields keep both fixed promises after partial depletion and removal. Delivered amounts do not depend on the saved originals' remaining strength and do not recursively schedule. |
| S07 | Reapplying the same named Magnet refreshes its selected materials instead of stacking copies. Ordinary finite-pile removals conserve stock. |

See [probe-results.txt](shared-recipes/probe-results.txt). O01 is separately labelled an unresolved observation, not an eighth passed semantic group.

## SR-01 — SH084 full binding withheld: source interpretation

**Severity:** evidence/source hold; no confirmed runtime defect. Applies to the graph above. Source: `design/RECIPE-CATALOGUE.md:352`; cited author case: `core/tests/shared-recipes/ammo.inl`, “distinct printed crafting types across Ammo only capped27”. The recipe grants +3 per different resource type “used in crafting” the shot's Ammo. The cited assertions choose printed ingredients, while the selected explicit printed-cost resale rule does not establish the same definition for this damage bonus.

**Reproduction:** acquire actual UGS-116, select Glass discount, pay to craft SH084, load it alone and Fire. Observed actual payment uses two material types, with zero Glass; the main hit is **21**. Printed ingredient types imply 21; actual-paid types imply **18**. The independent O01 reports both interpretations without asserting either as authoritative. Discounted, free-granted and copied consequences remain unresolved.

**Required closure:** an owner/source definition and matching assertions for its consequences before full SH084 binding. Root has sent the owner question; no answer was available at this review freeze. The author agrees the present printed-cost cases are narrower proof. Preserve their results as such. The original 100-ID proposal was copied unchanged into the ignored QA archive before any proposed-binding revision.

SH064 remains unbound as originally declared: no demonstrated eligible application producer covers its player Corrosion, Mark and Weaken prevention branches. Its executed Burn, expiry and Fouling/Leak exclusion checks do not establish those missing branches.

## Evidence integrity and limits

[audit.py](shared-recipes/audit.py) independently verifies all **48 input hashes**, **8 captured artifact hashes**, all **101 exact Markdown source rows** with costs/cooldowns/rarity headings, original historical inventory and all **113 actual named PASS links**. Four in-memory negative controls reject missing results, wrong implementation identity, changed source text and invented PASS evidence. This is linkage validation, not a second semantic oracle. An initially omitted `Uncommon` heading was corrected in the independent audit parser before its final pass.

The earlier failing delayed-Shield archive remains intact: 35 source hashes, failed executable `47c6cb510fd511469a330c0b4b17b819efa3a556107c36d550d4183fb5ffec9f` and both SH092/SH113 failure lines were independently verified. It was not relabelled as passing. Current production was not changed. QA source/binary/records are retained under `core/build/qa-shared-recipes/frozen-9b4b7a2b`; raw rebuild logs remain ignored.

Not run: exhaustive interactions, natural campaign acquisition, filesystem save recovery, packaged startup/exit, input, graphics, audio, performance or human play. No fun, balance, visual, shipping, full-content or current-version approval follows from this finite review.
