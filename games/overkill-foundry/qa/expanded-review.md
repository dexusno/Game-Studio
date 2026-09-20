# Expanded shared-core independent review

Status: first bounded semantic batch complete. **43 independent semantic groups passed, zero remaining failures in this batch.** Five independently found defects were fixed by the implementation author and rechecked. This is not final QA, release approval, visual approval, or a claim that all 246 recipes are correct.

## Scope and artifact

Independent reviewer: `/root/foundation_qa`; implementation author: `/root/core_effects`. Review-owned sources are `qa/expanded/`; production changes belong to the engineer. Baseline Git revision is `b1e359050812bffb3551e08b8a410fed5fd1ca41`, with an uncommitted expanded implementation. Windows 11 Pro 10.0.26200, x64 Release, MSVC 19.44.35228, Windows SDK 10.0.26100.0, Python 3.11.5. Controls exercised are the public C++ `Action`/`Rules` API, not Unreal input.

Contracts: selected Shared/Mara rows and owner rules in `design/RECIPE-CATALOGUE.md`; relevant combat/lifetime rules in `design/TIMING-AND-PERSISTENCE.md`; Cinderwall robot definitions and committed-intent rules. Campaign, upgrades as a complete runtime, Unreal, rendering, audio, packaged startup/exit, human play, balance and fun are outside this batch.

Final frozen execution: **2026-09-20 12:34:10 UTC**. Rules `of-core-0.3`, content `cinderwall-recipes-0.2`, snapshot schema 3. The wrapper hashed all listed production/probe inputs before compilation and after execution; they were unchanged. Complete identities are saved in [evidence/identity.json](expanded/evidence/identity.json), with [runtime output](expanded/evidence/runtime-results.txt) and [metadata output](expanded/evidence/metadata-results.txt).

| Final artifact | SHA-256 |
| --- | --- |
| Independent `expanded_probes.exe` | `94215d5ce0862e5c6ff34007432bdf24d71eb246aa6a6d97f976c6e51975b989` |
| `core/src/core.cpp` | `0356e55180df4e4f3b5235c9cbf2023f5996343e6e4f7a609de2b10841537946` |
| `core/src/recipe_effects.inl` | `e6f45237878529d4afc4e5e7af4e153d10873c95ff6d805c3f60853da7ba3f22` |
| `core/src/serialization.cpp` | `5f2092ffd17e24987bd60ac2b848c6707dc409461c498b5a757c2e7fd9ac4500` |
| `core/src/robots.cpp` | `04128cf381f76558e491ba873544c6196d4e31c2f573aee13836b9ca063a5380` |

Reproduce from the repository root with `& games/overkill-foundry/qa/expanded/run.ps1`. The probe executable compiles directly against the frozen shared sources in its own ignored build directory. It uses public actions plus explicitly constructed arena preconditions; it does not reinterpret catalogue prose at runtime.

## Initial execution and findings

The first independently compiled executable ran 26 semantic groups: 22 passed and four failed. All four were ultimately accepted defects. This execution overlapped the author's serializer work and is provisional, not the final frozen artifact. Its executable SHA-256 was `54506b0b14159756e34ef47ec1b1ca6a8dd7b451c3d447ab0fda87cc4ff49781`. The observed `core.cpp` and `recipe_effects.inl` hashes match the author's subsequent frozen checkpoint: `9b8451ae10f3b65f2c8a5019a702c2d726d05e05307fcb5e55cda13087966a26` and `3217767a130f9968c98d30379f2eea7793f7ace01ad8beb30e75e0ca1947642c`.

| Finding | Severity | Reproduction / expected / observed | Source and implementation | Recheck |
| --- | --- | --- | --- | --- |
| E01: removed MA059 still reduces damage | P1 | Start at 80 HP with one enemy's 10-damage attack. Craft/install MA059, remove it, End Turn. Expected 70 HP; observed 75. | Catalogue first-install rule line 106 (reactive source must be installed), MA059 line 515; `recipe_effects.inl::attackReduction`. | Fixed; independent reruns produce 70 HP. |
| E02: removed MA086 still halves chosen attack | P1 | Same arena. Install MA086 targeting attacker, remove, End Turn. Expected 70 HP; observed 75. | Same source-presence rule, MA086 line 547; reduction binding stored enemy rather than physical source identity. | Fixed; independent reruns produce 70 HP, and duplicate installed sources halve once after flat reductions. |
| E03: SH087 payload proceeds after lethal secondary recoil | P2 | Player 2 HP; main target Burn 6; other target Mesh 4. Fire SH087 selecting other target. Support recoil causes Defeat. Expected later half-Burn removal to stop, retaining Burn 6; observed Burn 3. | Timing lines 20/34; SH087 `afterAmmo` case 87 lacked a defeat guard after `enemyDamage`. | Fixed; independent reruns retain Burn 6 after Defeat. |
| E04: untouched generated Shield excluded from copying | P2 | SH026 Use auto-loads a pure 4-Shield part; remove before damage/payment; SH102 rejects it as used. Expected two fresh printed 4-Shield copies next turn; observed rejection. | Catalogue immediate-Shield clarification line 116, SH026/SH102; `reserveChoice` equated ever installed with used. | Accepted after source reconciliation, then fixed. Independent replay receives two fresh copies, grants no recursive delivery, and preserves Common rarity. SH026 is Common under heading line 263; an intervening Base-rarity assumption was withdrawn. |
| E05: SH085 ignores chosen Shield payment | P2 | Install 6 then 4 Shield; activate SH085 with `amount=4`. Expected first part 2, second part 4, next-shot bonus 8. Observed first part 0 because implementation drains all 10 regardless of choice. | Catalogue SH085 line 353, selected variable-payment rule line 92, and combination line 418 explicitly says decide how much to spend/keep; `catalogueActivate` case 85 ignored `Action.amount`. | Fixed. Final probe leaves 2+4 Shield, deals 14 with a 6-damage slug, allows zero, and atomically rejects -1, 11 at 10 available, and 16. |

E05's independently observed failing artifact was executable `a0112e9fc00f955e511a70cb5abc50e66eb9ee54ce924dd691f2f6faa7be9199`, compiled against stable `recipe_effects.inl` `aef1f58028316c017521af039fa98dbeb8c1c87232ad44cac5ba7a2e2ebb1182`. That run passed the other 42 groups. Final artifact identity above supersedes it for readiness.

The engineer also found two related defects during this review and added their fixes before independent checks: MA102's remembered Heat included removed sources, and MA093 treated a shot's aggregate Heat cost as one paid part. These were **author-found**, not independently observed failures in the earlier batch. Independent follow-up confirms that removing the 8-Heat source leaves only the installed source's 3-Heat restoration, and two physical MA041 parts each paying 4 Heat earn two scheduled Iron while their named spread occurs once. A combined 11-Heat shot at 10 Heat rejects atomically before any later kill refund.

## Content provenance check

`qa/expanded/source_metadata.py` independently reads all 606 catalogue rows with their actual rarity headings, including headings with suffixes, then compares manifest rarity. It passed with the corrected manifest SHA-256 `d56bf01290f16567cd7009a64f4d49f825454a7149fbf6a922a695a4b71deab9`: SH121–123 are Rare; SH124–126 Legendary; the selected Regular recipe pool has 207 entries. This is a source-to-manifest check, not a snapshot compared to itself. The earlier foundation report's 51 content checks did **not** detect the suffixed-heading parser defect; the integration owner identified and fixed it during expansion. That historical limitation is preserved here.

## Passed in the provisional first batch

- Original binding order across removal/reinstallation; already committed next-round deliveries survive source removal; a missed next-shot check does not re-arm.
- SH094 counts enemy attacks rather than individual hits; MA108 does not reward partial Shield blocking.
- A copied Common Ammo loses later attached bonuses, retains source/resale basis and gets fresh age; stored-momentum copies begin with zero reserve age.
- SH110 reservation/unload does not consume its Shield payment or trigger first-install bonuses; variable Heat and own-HP costs validate atomically and do not repay on reinstall.
- Part-only payment rewards exclude direct Utility payments; post-Armor Shield bypass redirects actual damage; pre-hit Burn removal precedes kill-transfer sampling.
- Split Chassis death-spawn identity and no creation-round action; Nest's committed attack after its live Mite dies; boss Corrosion/Burn thresholds before/after action; Ammo-only Fouling expiry; Shield Leak excludes direct upgrade protection.
- All six manual-grab enhancement outcome scopes (automatic/miss/Perfect), finite-pile bonus clamping, duplicate named amplifier refresh and multiplier exclusion.

Further passed checks cover MA066 delayed attachment refresh, MA079's part-only payment window, MA048 optional End Turn payments and all-or-nothing multiple costs, MA115's shared restoration cap across whole attacks, MA060 two-tick Burn hold, MA120 pre-defence echo sampling/current Shield/nonrecursion, SH115 duration refresh, SH101 optional zero sacrifice and illegal duplicate payment, SH070 next-turn Utility discount, and identical preview/events after canonical save/restore.

## Additional executed checks

- CTest: all five authored suites passed in this reviewer's execution before the last SH085 correction; the final frozen recipe/core/robot suites were then rerun and all three passed. See [authored test output](expanded/evidence/authored-results.txt) and [executable hashes](expanded/evidence/authored-identity.json). Their authorship and large assertion counts do not turn them into independent per-recipe acceptance.
- The original foundation probe target required two linkage additions (`catalogue.cpp`, `robots.cpp`) after expansion. With integration-owner authorization, only its CMake source list changed; all original **15 probe semantics passed unchanged** against the final core. That executable SHA-256 is `c7a0ce241823a7640e48e71c2c62eea434037afa5fa1a2261f03e6ca6f6dbc35`.
- Authored Mite/Ram fixture: success, 80 HP, round 3, four shots, final state hash `37978146880e1302` under `of-core-0.3` / `cinderwall-recipes-0.2`, snapshot schema 3.
- Content compiler `--check --self-test`: passed, including nine negative fixtures.
- Content compiler `--check --release`: expected exit 1, reporting 665 unsupported release operations. That fail-closed gate remains a release limitation; recipe registration and this batch do not justify changing it to supported.

## Limits and next review boundary

The independent robot scenarios use actual Split Chassis, Rivet Mite, Coil Nest and Gatebreaker definitions. Fouling and Leak checks inject those statuses into an otherwise controlled arena; they do not independently cover the Binder/Warden's complete random delivery patterns. No complete independent traversal of all ten robots or all 246 recipe semantics/combinations was performed. Other compiled suites were rerun, but their extensive assertion counts are reported as authored test evidence only.

This batch exercises in-fight canonical snapshots and one complex preview/apply parity trace. It does not test disk crash recovery, campaign fight-entry Continue, all terminal cleanup/progression, shop/reward flows, or malformed snapshot fuzzing. Startup/menu/input-focus, restart controls, animation, audio, exit, Unreal parity, packaging and human usability were not run here. The technical checks make no claim about game feel or balance.

Readiness assessment: the specified high-risk examples in this batch are suitable for the next integration step, with no unresolved reproduced failures. Broader recipe/robot semantic acceptance, campaign/session integration and graphical-host acceptance remain separate work. Do not infer release readiness from registering 246 IDs or passing this bounded set.
