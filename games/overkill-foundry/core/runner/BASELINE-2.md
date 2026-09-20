# Baseline-2 development pilot — 20 September 2026

This is the first unfiltered paired Cinderwall pilot: seeds **1–20**, both policies, **Auto collection**, search budget **1,200 previews per decision**, watchdog **30 seconds per city**. These seeds are development data, not a fresh evaluation set. Both policy processes ran concurrently; elapsed times include that machine load and trace IO.

Native rules/content baseline: `842300e`, `of-core-0.4`, `cinderwall-upgrades-0.3`. Manifest SHA-256: `d56bf01290f16567cd7009a64f4d49f825454a7149fbf6a922a695a4b71deab9`.

Runner executable SHA-256: `f92ba5b79576b4e3d2bf8a2f998b92e02a2adf42df4b16c32b2dff6a3057dd1d`. The preserved executable, source snapshot, both JSONL summaries, every command/event trace and replay results are under ignored `core/build/runner-baseline-2/`. Use that preserved executable for baseline-2 replay; a later policy version has a different metadata identity.

## Observations

| Outcome / metric | Aggressive | Defensive |
|---|---:|---:|
| Cities completed | 18 / 20 | 15 / 20 |
| Actual defeats | 0 | 4 |
| Policy watchdog stops | 2 | 1 |
| Rejected actions / unsupported choices / blocked rules | 0 / 0 / 0 | 0 / 0 / 0 |
| Final HP among completed cities, mean (range) | 75.1 (57–87) | 75.0 (41–98) |
| Boss rounds among resolved runs, mean (range) | 3.4 (3–6) | 17.5 (3–30) |
| Boss shots among resolved runs, mean | 7.6 | 9.6 |
| Commands among resolved runs, mean | 381.3 | 503.4 |
| Seconds among resolved runs, mean | 0.393 | 0.620 |
| Sum of all run seconds, including timeouts | 67.137 | 41.803 |
| Income / spending among resolved runs, mean Credits | 254.1 / 293.1 | 172.3 / 222.4 |
| Healing / damage / HP payments among resolved runs, mean | 5.8 / 10.5 / 0.2 | 12.4 / 25.3 / 7.9 |

“Resolved” means completed or actually defeated; watchdog stops remain in the outcome denominator but are excluded from the listed resolved-run means. Initial Credits explain spending above income. HP above 80 reflects legitimately acquired maximum-HP effects. This small pilot does not estimate human win rates or balanced pacing.

All aggressive runs completed except seeds **7 and 9**. All defensive runs completed except actual defeats on **5, 9, 12 and 15**, and the timeout on **13**. The four defeats occurred at Gatebreaker after respectively **27/30/27/30 rounds** and **8/7/7/8 shots**. No losses were retried or removed.

The three stalls are concrete policy defects:

- Aggressive 7 stopped at position 8, round 189, with Split Chassis at **1 HP**, player **80 HP**, 4 shots and 134 saved parts. A legal Plain Slug shot kills the Chassis and releases its helper. The policy penalizes the new helper's HP as negative progress, then stockpiles Shields. Repeated evaluation of old reserve parts consumes its search budget.
- Aggressive 9 stopped at position 8, round 213, with Split Chassis at **2 HP**, player **80 HP**, 4 shots and 91 saved parts. It shows the same death-spawn valuation problem and later search starvation.
- Defensive 13 stopped at position 7, round 715, with Coil Nest at **2 HP** and its Mite at **1 HP**, player **80 HP**, 5 shots and ample materials. It maintains defense indefinitely. Because its score rewards Shield only in proportion to current incoming damage, killing an attacker removes more scored Shield value than the perceived benefit of the kill.

These states admit legal damaging actions. They are not failed engine transactions or evidence of a mandatory gameplay action cap. The proposed baseline-3 repair changes policy evaluation and preview allocation only; the complete baseline-2 evidence remains intact.

## Naturally acquired content

The runs did not force offers, recipes, upgrades, HP or materials. Examples retained for deeper effect-level inspection include:

- Aggressive 6 acquired Irena's Heat Ledger, UGS-117, Standard Ammunition Jig and UGS-076; it completed all 12 fights and Gatebreaker in **3 rounds / 8 shots**, ending at **73 HP**. The trace records real paid Ammo manufacture and Heat/upgrade effects.
- Aggressive 13 acquired Cinderwall Ignition Bank, UGS-097, Standard Ammunition Jig and MY2-02; it used **MA068 Braced Shot 10 times** during the city and completed Gatebreaker in **5 rounds / 6 shots**, ending at **80 HP**. The Shield threshold must be checked on the actual Fire states before calling this a demonstrated Shield-to-impact build.
- Defensive 16 used **MA048 Holdfast 13 times**, reached Gatebreaker and won in **29 rounds / 14 shots**, ending at **41 HP**. This shows legitimate use of deferred defense but poor tactical efficiency.

These are observed earned loadouts and uses, not a claim that two distinct build archetypes have already passed balance acceptance. The fresh evaluation should identify actual Heat/Burn and Shield-to-impact event chains and their city outcomes.

## Verification and remaining limits

Strict MSVC `/W4 /WX /permissive-` build and **30,032 author runner assertions** passed. The legacy fixture remained **80 HP, round 3, 4 shots**, hash `965951ac6f2b9563`. All **40 pilot traces** replayed every exact command, result, event, hash and final serialized byte successfully, including the three stopped states and four defeats (`replay-all.jsonl`).

Independent review separately passed 12 selected CLI cases, 15 probe groups, 4,443 exact replayed production commands and 12 valid/corrupt trace cases. It reproduced and verified repairs to loose replay metadata/outcome validation and missing Technician HP-payment reporting. The repeated defensive seed 2 trace is byte-identical: SHA-256 `7d8a2e121f0c76d0bdd06d288852db55463c787f72ab9cb13b878e768ceecaac`. Its accounting is **80 + 14 healing − 6 damage − 8 Technician payment = 80 HP**.

After review, root identified a **Precision-model parameter mismatch**: baseline-2 Practised uses **15% Miss / 55% Good / 30% Perfect**, while canonical `design/data/beta-balance-v1.json` specifies **20% / 45% / 35%**. Both average 1.15, so the mean-only author assertion missed the distinction; Good/Perfect-specific effects make the full distribution important. The 40 Auto runs are unaffected. Preserve synthetic baseline-2 results with their actual distribution, and fix the source-defined bands with separate category assertions in baseline-3.

Next: repair the identified policy stalls and Practised bands, replay regression seeds plus the prior three seeds, obtain an independent source freeze, then evaluate fresh paired seeds **21–120** with Auto. Broader Precision comparisons remain explicitly conditional on uncalibrated models. No game balance, visual quality or human approval follows from these numerical results.
