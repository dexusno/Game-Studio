# UC01 independent recheck — 20 September 2026

**UC01 is repaired for newly attributed outputs on the reviewed graphs.** The source-specific UGS-121 copy exclusion now has provisional support. No new runtime defect was found. Old unmarked saves retain the previously excessive bonus; migration/fresh-start and version acceptance remain an owner hold. The original failed upgrade review and its artifacts are unchanged.

Exact identities and limits are recorded in [upgrade-copy-evidence.json](upgrade-copy-evidence.json). This is a narrow correction recheck, not a new full-upgrade, release or shipping approval.

## Source and isolated change

The controlling source is captured `design/data/shared-upgrades.json`, `/upgrades/120`: Jig grants +2 to only one physical SH001 Slug, including when copying creates more. UGS-015 (`/upgrades/14`) otherwise copies the first output's already committed properties without repeating Use or delivery effects. The specific Jig restriction therefore takes precedence over that generic copy behavior.

Reviewed the 58-input capture at `core/build/upgrade-copy-contracts/captures/native-uc01-f04393742ce59384-24edf61b936d`:

- Source graph: `24edf61b936dd5b8b034014de1c2331a10b7beedfd87464c5b1be97579b64ef5`.
- Author executable: `f04393742ce593845d20c25201c1882d07da99711316935edcad1cd20683cbf4`.
- `recipe_effects.inl`: `10ccb56c16fb0f3f3c0dd8a79ec22f97c3a440d11a55b7e88dcc0a05d638c8c8`.
- `upgrade_effects.inl`: `b47d7f3a3d56fd3588023217c2eaa491bdf58e96b6d7cdc77f9d7886c7315d1f`.

Also copied all 54 inputs of the original `49b5f504…` candidate into a separate ignored review directory and applied only `uc01.patch` (`66ea4e65a19ce61af7bd036e2a979056ccc0a413cf869d14ec95cef4a4dff74d`). The resulting overlay graph is `cd81a0f2b296918bb83a3801b7ac673d9f9901c4b7a32d86d3405d72793610a7`. Exactly two production files and one legacy test oracle change. Its only production difference from the full capture is the separately reviewed SH092/SH113 schedule correction. No later MA082 change is included.

The repair adds a zero-valued existing attachment to the original Jig-enhanced output. The shared receive path strips that attribution and exactly two aggregate damage from a committed copy, rejecting an insufficient aggregate before committing. Other attachments and ordinary committed bonuses are preserved. Header, serializer, schema and RulesVersion bytes are unchanged; that fact does not establish legacy compatibility.

## Actual execution

Windows x64, MSVC 19.44.35228.0, SDK 10.0.26100.0, C++17 Release with `/W4 /WX /permissive-`. No live production library or Unreal was used.

| Execution | Result |
|---|---|
| Unchanged full capture rebuild | CTest **5/5**: focused 7/671; legacy upgrades 1,130; author upgrade 106/6,139; Shared 113/27,881; physical lifecycle 207/59,169. |
| Original graph plus isolated patch rebuild | CTest **4/4**, including original upgrade and campaign regression suites. |
| Historical independent failed executable | Reproduced **8 passed / 1 failed**, 492 assertions: Jig-first original/copy deal 16 instead of 14. Exit 1 retained. |
| Exact original failed QA source, recompiled unchanged | **9 passed / 0 failed**, 503 assertions, on both corrected libraries. |
| New independent probes | **6 passed / 0 failed**, 1,333 assertions, on each corrected library. Outputs are byte-identical across the two graphs. |

The new probe executable hashes are `39f7f4fe554b7bd4978656ad8e04537a74099f1aaadc2e403f4858d554da02b6` (full) and `5e49ce8784b87fe71f4af035dfca516cb968249f0d625112e85c6283cc382097` (isolated overlay). Compact [probe output](upgrade-copy-contracts/full-probes.log), [unchanged original-source output](upgrade-copy-contracts/full-original-review.log) and [reproduced failure](upgrade-copy-contracts/original-failure-reproduced.log) are preserved separately.

## Independent boundary coverage

- **C01:** All 24 acquisition orders of Jig, Lease, Mayor damage and a permanent recipe tag, plus a real SH103 attachment. Actual copies retain only ordinary bonuses committed before their copy event, exclude Jig, preserve provenance/resale fields and keep the SH103 attachment. Saved next-round Fire confirms numerical results. The marker stays only on the original.
- **C02:** Actual MA030 next-round damage on the original, plus SH036 damage and MA066 timed Heat on its independently modified copy. Reload and later separate shots preserve each physical part's effects; the marker adds no Heat or damage of its own.
- **C03:** A real Iron discount makes the first SH001 Use resource-free. It grants neither Jig nor Lease copy and spends neither allowance; a later paid Use of another stored SH001 copy earns both correctly.
- **C04:** A controlled malformed pending copy with insufficient attributed aggregate is rejected after reload. An earlier material delivery, the round clock, all state/RNG and emitted events roll back atomically. This is an adversarial state test, not a naturally available game action.
- **C05:** Controlled pending committed copies and a copy descendant remove Jig once, preserve the original and other damage/attachments, and never subtract an unrelated bonus from an already stripped descendant. This tests the common receive boundary, not an additional claimed copy recipe.
- **C06:** Two actual old-library save fixtures distinguish the unmarked Jig excess from a valid ordinary Mayor bonus. See the compatibility limit below.

The seven focused author bodies were independently inspected. Their additional actual SH118/SH072 printed-copy, copy-descendant, payment failure, unused/resale and once-per-turn checks passed after independent rebuild. SH102 cannot copy Base SH001 under its Common-only rule; MY1-06 is memory capacity plus a recipe grant, not physical copying. Neither is counted as a tested Jig copy path.

## UC-L01 — retained old-save limitation

**Disposition:** known owner-held compatibility decision, not a newly introduced runtime defect.

Using the archived pre-fix library `c7905a329931f4d5062096301669e207683a8e334d4c0a875c209dc4cb6e7e7d`, [legacy-emitter.cpp](upgrade-copy-contracts/legacy-emitter.cpp) acquired Jig then Lease, performed the actual paid Use and serialized both resulting Slugs. No saved field was edited. The corrected reader accepts exactly those bytes, preserving two unmarked +2 aggregates. Firing both still deals **16**; a newly created corrected Jig pair deals **14**. A separately generated old Mayor-plus-Lease control also contains unmarked +2 on both parts and correctly remains **16**.

Thus the patch does not infer attribution or repair historical outputs. The marker's absence is a real remaining compatibility boundary. The two exact save hashes and emitter identity are in the evidence JSON. A migration/fresh-start policy remains necessary before compatible-save or release acceptance.

## Integrity and limits

[audit.py](upgrade-copy-contracts/audit.py) verified all 58 full-capture inputs, 14 captured artifacts and five named suite/log/binary links; all 54 original inputs; the isolated patch's exact three-file scope; and all four retained original failed QA artifacts. The original failed executable was executed again rather than its log merely relabelled. Current production and prior QA reports were not edited.

One new QA control initially attempted two Uses of the same `None` recipe copy in one round. The core correctly rejected it. The independent setup was corrected to use another stored copy; the first source, binary and failed control output remain under `core/build/qa-upgrade-copy/history/same-copy-control-error`. This was not a production failure. An initial CMake invocation also needed Windows paths normalized to forward slashes.

These are finite prepared native fixtures with typed actions, preview/event parity and snapshot reload. Not tested: natural campaign acquisition, external save-file durability, packaged startup/exit, input, graphics, audio, performance, balance or human play. The broad suite totals above are regression evidence, not proof of every clause or every interaction. No active bindings, schema, version, migration policy or shared status was changed.
