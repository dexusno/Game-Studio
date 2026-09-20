# Independent Mara recipe contract review — 20 September 2026

**Finite provisional pass for the 107 proposed bounded semantic links.** MR01 and MR02 are independently closed on the reviewed capture. No additional runtime defect was reproduced. All declared residual coverage remains; this is **zero exhaustive recipe approvals**, not a claim that all Mara interactions are proven. No active binding, version, migration policy or shared status was changed.

All 107 printed source rows, their primary fixture bodies, the common payment/preview/reload helpers and 33 clause extensions were independently read. [clause-review.json](mara-recipes/clause-review.json) records each ID's actual fixture locations, source text, bounded disposition, retained residuals and any additional independent probes. Exact machine/artifact evidence is in [mara-recipe-evidence.json](mara-recipe-evidence.json).

## Exact artifact and execution

Reviewed copied sources under `core/build/mara-semantic-05/source`, not a later live checkout:

- 37-input graph: `254e4f72c531dcac6f70c1df74cf7a09592aaa15803a191767e54db936556ef3`.
- Author executable: `f93f37ccb67b3f778ccb5846590f25c0f0f836d52393e8b025f8ccc3f2d861ef`.
- Corrected `recipe_effects.inl`: `4adce1cd3c24c9e16a771e1ada3a2aac7141668a1ab4258f79be6a9bc6f82ebf`.
- Portable author results: `4c1f8ecf14015c36fb84eb8c259d635dcc108490ab99f293182563aa040a224e`.

The unchanged graph independently rebuilt on Windows x64, MSVC 19.44.35228.0, SDK 10.0.26100.0, C++17 Release with `/W4 /WX /permissive- /utf-8`. CTest passed **2/2**: 107 Mara groups / **20,526 assertions**, plus **2,055** existing recipe assertions. Rebuilt author executable: `cc94970d8339100078eb31343b9e181ce028093355146f8e276011419ec3724c`.

The separate [independent probes](mara-recipes/probes.cpp) pass **7 groups / 1,346 assertions** against that rebuilt library. Executable: `dbe4b2d59378cedaa7077a78b2404946d150f0453683ff78cb53a7c66caf96ce`. Typed successful actions compare nonmutating preview with complete committed state and ordered event bytes. Selected paths reload canonical snapshots; invalid payments check empty events and unchanged state/IDs/RNG. [Actual results](mara-recipes/independent-final.log) are retained.

These fixtures prepare memory, resources, HP, statuses and enemy intents. Actual production Rules pay, create, install, remove, activate, fire and advance the round. They are not naturally earned loadouts or campaign balance evidence. The author MA107/MA119 cases additionally execute actual Campaign progression/cleanup with explicitly controlled combat fixtures; their claimed scope was checked without treating them as natural campaign runs.

## Rechecked defects

**MR01 — MA082 exceeding its Burn cap (repaired; material combat error).** Source: `design/RECIPE-CATALOGUE.md:543`; affected code: `core/src/recipe_effects.inl:277`. Acquire actual MAU-02, pay Fuel Brick Uses to reach legal Heat14, craft/load/fire MA082. The preserved capture03 executable applies Burn14, violating the printed cap10. Capture05 applies Burn10 while retaining Heat14 and printed damage12.

Independent M01 goes beyond that author case: real capacity/payment/cooling actions reach Heat9, 10, 11 and14; a simultaneous MA041 cost changes remaining Heat after the Fire snapshot. One or two MA082 parts add separately capped payloads to an existing Burn3 stack, with no main payload copied to the spread target. All eight combinations pass. The same probe linked to failed03 rejects legal Heat11: observed total Burn14 versus expected13 (existing3 plus capped10). Its later assertions are not counted as executed on that failing graph.

**MR02 — MA098 counting used reserve Shield as unused (repaired; material combat error).** Source: `design/RECIPE-CATALOGUE.md:559`; affected code: `core/src/recipe_effects.inl:460`. Pay SH002 craft/install for Shield6, use MA037 to pay5, remove the remaining1-Shield source, then activate MA098 and fire SH001. Failed03 deals9; corrected05 deals6. The added predicate checks actual unused eligibility.

Independent M02 also covers fully depleted Shield and Shield damaged by real shot recoil, each removed, reloaded and reinstalled/removed again. Those parts add no reserve bonus. An untouched removed/reinstalled plain Shield still adds3, and counting preserves the exact saved part and protection. No new generated/copied/purchased-origin interpretation is accepted by this narrow repair.

The original capture03 binary was actually executed and again reported **105 passed / 2 failed**, 15,210 assertions, reproducing both defects. Historical03 predates the current `--case` parser: two attempted selected invocations ran its whole suite, with both raw logs retained. The separate independent old-library probes fail M01 and M02 with exit1, as expected; their [logs](mara-recipes/independent-old-M01.log) and [MR02 log](mara-recipes/independent-old-M02.log) remain separate from the passing results.

## Other independent boundaries

| Probe | Observed result |
|---|---|
| M03 | MA055 reads protection remaining after an actual enemy attack. An earlier SH060 discard prevents Iron; a later discard does not. Remove/reinstall changes protection order without moving the pre-reset binding. |
| M04 | Actual MAU-04 reduced decay still occurs when MA015 is removed. Later reinstall does not renew its original-round hook. Removed high-memory MA102 does not override an installed lower seal; saved later reinstall does not restore old Heat. |
| M05 | MA091 captures Burn after all pre-hit removal in either Ammo order. Lethal main-hit recoil prevents its subsequent transfer. |
| M06 | Exactly5 paid part Heat qualifies MA076's Shield removal;4 does not. An invalid chosen payment rejects atomically. |
| M07 | MA066's exact next-round Ammo gains its own2 Heat then attached4 after resolution when surviving; fatal recoil interrupts both later gains. |

These probes add only the named finite combinations. They do not remove unrelated per-ID residuals such as every robot immunity/reaction, every listener order, every cooldown producer or every generated origin. In particular, tests that seed cooldown counters do not establish each producer's own lifecycle.

## Integrity, history and limits

[audit.py](mara-recipes/audit.py) independently verified all 37 captured source hashes, seven archived log/identity/executable artifacts, 107 exact Markdown rows with costs/cooldowns/rarity headings, historical obligation inventory, 107 actual named PASS results and 33 source extensions. It checks each proposed observation against the corresponding registered fixture titles and keeps `proposed_only` status. Four negative controls reject a missing PASS, inflated assertion claim, altered clause and activated link. This structural audit supplements the manual assertion review; it is not semantic proof by itself.

All four earlier author failure archives remain hash-verified: 33, 33, 33 and34 inputs respectively, with original failed binaries/logs. Illegal Heat fixtures, wrong Mark/support arithmetic, unsupported event expectations, generic-intent setup, wrong payment arithmetic, payload-order assumptions and invalid Campaign robot identities remain classified as author-fixture errors. They were not recast as production defects. The production diff from failed03 to corrected05 is exactly the two declared lines; other core sources are unchanged. The author's read-only publisher check also passed.

QA sources, rebuilt binaries, old negative controls and the reviewed metadata are retained under ignored `core/build/qa-mara/frozen-review`. The candidate includes earlier UC01 and terminal-guard changes under provisional `of-core-0.4`; the owner-held version/migration decision and UC01's unmarked-old-save limitation are unchanged.

Not run: exhaustive source interactions, natural acquisition/economy, disk-save durability, packaged startup/restart/exit, hardware input, Unreal, graphics, audio, performance or human play. There is no fun, visual, balance, release, full-content or current-version acceptance claim.
