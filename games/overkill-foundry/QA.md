# Overkill Foundry — quality and playtesting

The first shared-core fight, fresh rendered Unreal action adapter and separate Windows storage envelope have been executed. The complete city, campaign saves/Continue, final visuals, full-city balance runs, standalone package and human approval remain pending.

## Verified foundation, 20 September 2026

Acceptance contract: [MVP-IMPLEMENTATION-PLAN.md](design/MVP-IMPLEMENTATION-PLAN.md). Technical increment: `of-core-0.2` / `cinderwall-staged-0.1`, snapshot schema 2, 19 staged recipes and a controlled Mite/Ram pair. Final eligibility remains the complete Cinderwall manifest. Passing a staged subset does not enable that pool.

The final author suite passes **443 assertions**. Coverage includes the authored fight; repeated Fire; Shield/recoil and final-death precedence; a controlled Reserve Heart Pump hook; first-install costs, gains, depletion, removal/reinstallation and saved history; retention and delayed Shield; cooldown-1/global cooling/duplicate copies; atomic material/HP rejection; Hot Barrel/integer percentages; separate/lost spread hits; Burn; escape; collection/Precision/Magnet; shared Quick Patch cap; nonmutating previews; uncapped 32-part builds; snapshot roundtrip/corruption/version rejection; RNG separation and repeatable event traces. These are authored behavioral tests, not evidence of fun.

[Independent foundation QA](qa/foundation-review.md) passes **15 separately authored runtime cases and 51 content checks**. It reproduced and closed four defects: removed insulation incorrectly warding Burn, same-named Magnet stacking, different named modifiers overwriting, and unsupported/unreachable effect definitions consuming costs as silent no-ops. Follow-up checks include 40-part stockpiles/Shield, seven legal shots in a round, source-name refresh across serialization, and opcode/kind/timing rejection. The withdrawn same-name percentage-stacking expectation is recorded as a QA correction, not a game defect.

The manifest/compiler reproduces exactly, checks 560 legal schedules and 6,160 offer states, and rejects nine negative mutations. The release gate correctly refuses 665 unimplemented operation obligations. Structural coverage is distinct from runtime effect coverage.

Rendered/headless parity passes **84 events plus the identical summary**: 80 HP, four shots, three rounds, hash `5de73f967dc79677`. Ordinary DX12 launch and both camera captures were inspected at 1600×900. An automated presentation-command probe passes; a separate native mouse/keyboard check passes Collect, recipe rejection/use, part selection/Load, Fire retaining the round, End Turn and restart. [BUILD.md](BUILD.md) records module/transcript identities and logs. This scene still uses temporary geometry and a technical HUD.

The Windows storage author suite passes replacement, backups, stale-writer refusal and actual child-process termination at four write boundaries. Independent [storage QA](qa/storage-review.md) reproduced and closed two defects: a stale-writer identity collision after backup repair and a short legacy-envelope version-detection error. Final recheck passes **15 independent groups, 1/1 author CTest target and 12 verified process exits** covering existing, first-save and repair writes. Reconciliation is verified after a real post-replacement Windows error. No campaign persistence or hardware power-loss claim follows from these envelope tests.

Original Ram/Mite/stage exports pass 81 art structure/interchange checks with six inspected source renders. No final Unreal art integration, animation-coverage acceptance or owner visual approval is established by those checks.

## Timing trace coverage

| Traces | Evidence or pending work |
| --- | --- |
| T01–T04 | Controlled core tests pass. T03 uses the named rescue hook; actual upgrade acquisition remains pending. |
| T05–T06 | Exact cases use Noor/later-city Mayors outside this MVP. Shared ordered Shield conversion and Mara-applicable interactions still need implementation. |
| T07–T10 | Controlled retention, delayed Shield, cooldown and escape tests pass. Eligible retention upgrades and complete roster behavior need integration. |
| T11–T14 | Campaign transactions, original fight-entry restart and reward exchange remain pending. Storage envelope tests alone do not pass these cases. |
| T15–T16 | Exact Noor/late-Mayor cases are outside this MVP; preserve specifications. |
| T17 | Exact later-city Heat Mayor pair is outside this MVP. Shared acquisition-order hooks still require eligible-content checks. |
| T18–T22 | Mystery and individual reward/reopen/skip/exchange persistence remain pending. |

## Human playtests

No human has played or visually approved this implementation. Record build/date, participant, task, observed behavior, duration, unprompted reactions, friction and changes worth testing for each future session. A model review is not a human playtest. Klaus must see actual representative gameplay before the visual gate can pass.

## Known limits and next checks

Only staged effects and a two-enemy fixture are executable. Full recipes/upgrades/robots, city, Mayor, offers, shops, rewards, profile/Collection and Continue are not implemented. The action suggestion helper is not an exhaustive enumeration of assemblies. Durable storage has no campaign caller yet. The current Precision control supplies a recorded outcome rather than a minigame. Robot graphics and HUD remain technical; final audio/FX/animation coverage, focus/resolution tests, packaged launch and performance checks are outstanding.

Continue with first-encounter art integration and the source-driven effect/campaign packages, preserving the final content boundary. Independent reviews must use the actual changed build; this foundation review cannot certify subsequent behavior. Fun and balance remain unvalidated.
