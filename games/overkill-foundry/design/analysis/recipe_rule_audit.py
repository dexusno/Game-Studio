"""Render a manually curated, row-complete recipe audit; never edits recipes.

The assistant read all five pools. Membership below is that semantic review,
not the result of treating keyword matches as rule violations. This program
checks coverage, pins the reviewed source, and renders evidence for discussion.
"""

from collections import Counter
import hashlib
import json
from pathlib import Path
import re
import subprocess


DESIGN = Path(__file__).resolve().parents[1]
ROOT = Path(__file__).resolve().parents[4]
SOURCE = DESIGN / "RECIPE-CATALOGUE.md"
SOURCE_COMMIT = "a3b2e042b77ad221ef77837c54e59e1d886a8903"
SOURCE_REL = SOURCE.relative_to(ROOT).as_posix()


def finding(key, status, title, rule, reason, ids, focus, review):
    return dict(id=key, status=status, title=title, rule=rule, reason=reason,
                recipes=ids.split(), excerpt_focus=focus, review=review)


FINDINGS = [
    finding("C01", "resolved", "Recipe-granted Shield retention", "R02",
            "The owner authorized analogous fresh-part replacements after approving Spare Metal Brace. These twelve rows now schedule regular Shield parts instead of retaining active protection. Costs, cooldowns and meaningful conditions are preserved. Saved source parts retain explicit schedule clocks, checking that they are installed at those times. Last Wall checks leftover Shield before reset for its conditional cooling reward; its future Shield part is a separate fixed grant.",
            "SH057 SH092 SH113 MA048 MA099 IV019 IV057 IV061 IV090 AD074 AD092 NO087",
            r"next round|next two rounds|beginning",
            "Retention conflicts are resolved. Q09 retains standard one-part valuation mapping for the added outputs; numerical balance remains untested."),
    finding("C02", "resolved", "Copy effects and scheduled Shield grants are not Utility items", "R01",
            "The owner corrected the copying finding: production means resource-based crafting; a Utility copying an existing part is an effect, not crafting that copy from resources. SH071/SH102/SH118 retain their copy eligibility, amounts, timing and printed-effect restrictions. MA107/NO107 are already covered by the selected scheduled ordinary-Shield-part rule. The obsolete conflict markers are removed without changing functional effects, costs or cooldowns. The four other conversion/generated-output rows are clarification cases in Q11, not asserted violations based solely on part output.",
            "SH071 SH102 SH118 MA107 NO107",
            r"copy|copies|gain 2",
            "These five Utility-boundary findings are resolved. Q09 still covers the two Shield batches' canonical one-part resale references. No additional production-cost payment for a copied part is introduced."),
    finding("C03", "resolved", "Ordinary parts replace Utility-item sacrifices", "R01",
            "The owner approved sacrificing ordinary physical parts instead of nonexistent Utility items. Empty the Tools chooses up to two unused parts saved from earlier rounds for the next shot this turn, consuming them for +7 damage each instead of their normal effects. Selection is editable before Fire; End Turn without firing releases them unused. They still count as parts consumed by the shot. Supply Courier consumes two unused ordinary reserve parts as its additional cost, with no age requirement, and delivers 3 Iron, 2 Copper and 1 Carbon at next-turn start. Categories, material costs and cooldowns are preserved. Saved-item bonus conditions are resolved in C12, and split activations in C13.",
            "IV105 AD097",
            r"Choose|Consume",
            "The Utility-item sacrifice conflict is resolved. Normal reversible loading applies before Fire; paying Supply Courier's sacrifice is an actual consumption, not a refundable selection. Broader Helper/Modifier lifecycle details and numerical balance remain separate."),
    finding("C04", "resolved", "Shield granted during preparation, collection or after Fire", "R03",
            "Resolved by the owner's 16 September clarification: a grant automatically loads an ordinary Shield part worth that amount. It may be removed and saved like any part. Parts left installed protect when enemies attack; active Shield resets at enemy-turn end without an automatic grant next round. The original audit incorrectly read these grants as immediate active protection.",
            "SH033 SH045 SH064 SH065 SH095 SH104 SH108 SH111 MA004 MA047 MA078 MA101 MA113 MA116 IV011 IV036 IV038 IV066 IV079 IV098 AD024 AD042 AD049 AD054 AD056 AD060 AD068 AD078 AD082 AD103 AD107 AD114 AD116 AD119 AD120 NO036 NO041 NO054 NO092 NO095 NO111",
            r"gain.*Shield|Shield equal|Each trigger grants",
            "MA004 Quick Vent is resolved: automatically load its 5-Shield part, with ordinary removal/storage available. Other findings on these rows remain separate; AD103/AD107/AD119 also have enemy-phase triggers covered by Q01."),
    finding("C05", "resolved", "Explicit future-turn Shield schedules", "R03",
            "Resolved at the timing-rule level by the owner's Folding Brace revision: a recipe may schedule a new ordinary Shield part for the beginning of the next round. It loads then, remains removable/saveable and protects automatically against enemy attacks if still installed. This is neither active-Shield retention nor immediate active protection. SH034's expiry wording is now corrected in C10; MA118's active-Shield payment remains in Q02. Other printed values are still draft balance.",
            "SH034 SH096 SH115 MA016 MA040 MA092 MA118 IV059 IV097 IV112 IV120 AD063 AD066 AD098 NO018 NO034 NO056 NO098 NO113",
            r"start|next turn",
            "Interpret an explicitly scheduled Shield grant as a new ordinary part at its stated trigger. An immediate grant alone does not repeat next round. Preserve each row's stated conditions and other unresolved findings."),
    finding("C06", "resolved", "Shot bonuses count installed Shield parts", "R03",
            "The owner replaces Shield activation with automatic protection from parts installed at enemy attack time. Needle Thread counts older saved Shield parts installed at Fire for its existing 5-per-part support hit, capped at 15. Guarded Loading checks for at least three installed Shield parts at Fire for its existing +5 damage. Counting does not consume them. Costs and cooldowns are preserved. The three former shot-hook cases move to Q12; a nonexistent activation phase is no longer grounds to call them impossible.",
            "SH044 IV107", r"installed",
            "These installed-count conditions are resolved. Remaining shot-hook timing is recorded separately; do not reopen whether a shield needs activation."),
    finding("C07", "resolved", "Field Pocket counts before reset and rewards next round", "R02",
            "The owner confirmed that Field Pocket counts total remaining active Shield after enemy actions and before reset, then grants its recorded Charge next round. The clarified row uses 1 Charge per complete 3 Shield, capped at 3 Charge, while retaining 12 Shield, cost and cooldown. The mechanic needs a stored reward amount, not retained active Shield or a next-round Shield read.",
            "NO060", r"After enemies",
            "The timing issue is resolved by explicit count-before-reset wording. Noor's broader Charge mechanic remains a draft."),
    finding("C08", "resolved", "Charged Barrel is a specific character exception", "R04",
            "The owner corrected the audit's premise: a new general rule does not automatically revoke a special character effect. Charged Barrel is restored, with its original optional payment of up to 3 Charge at Fire for +2 main-shot damage per Charge, before other Charge checks. The five payment/no-payment/refund triggers therefore retain their original meaning. Their recipe rows were never changed; no generic recipe-spending tally is substituted.",
            "NO028 NO039 NO075 NO081 NO106", r"barrel|Charged Barrel",
            "This dependency finding is resolved by restoring the wrongly removed character effect. Its numeric values remain draft balance. Replacements based on unrelated Charge payments are withdrawn."),
    finding("C09", "resolved", "Folding Brace owner-selected replacement", "R03",
            "SH026's old 6 Shield plus retention of up to 4 is replaced by 4 Shield loaded on recipe Use and a new 6-Shield part loaded at the beginning of the next round. Both are ordinary parts. The second delivery is scheduled by recipe Use, regardless of whether the first part is activated or saved. Cost and cooldown are unchanged; the proposed flat 10 Shield was rejected.",
            "SH026", r"On recipe Use",
            "The retention conflict is resolved. Q09 retains the normal one-part resale-basis mapping work for the two generated outputs; no price has been invented."),
    finding("C10", "resolved", "Early Cover uses ordinary Shield-part expiry", "R03",
            "SH034 now schedules a regular 8-Shield part for the beginning of the next round. It loads then and may be removed or saved; if left installed, it protects when enemies attack and active Shield resets at enemy-turn end. The obsolete following-turn expiry clause was removed without changing its cost, cooldown or Shield amount.",
            "SH034", r"On recipe Use",
            "The old expiry conflict is resolved under the same ordinary-part rule."),
    finding("C11", "resolved", "Pocket Screen counts saved ordinary parts", "R01",
            "The owner replaced the nonexistent saved-Utility-item condition with a count of ordinary parts saved in reserve from an earlier round. Final corrected thresholds: 0 saved parts applies Weaken 1, exactly 1 applies Weaken 2, and 2 or more applies Weaken 3 to every living enemy. Count at immediate recipe Use without consuming those parts. Cost and availability are unchanged.",
            "IV071", r"Apply Weaken",
            "The Utility-item conflict is resolved. The owner confirmed flat per-hit enemy damage reduction, persistent until it decays by 1 per round to zero; the 25% damage-taken idea was withdrawn."),
    finding("C12", "resolved", "Equivalent Utility bonuses count saved ordinary parts", "R01",
            "The owner directed applying the same solution to the same problem. Twin Coolant and Packed Lunch now check for at least two ordinary parts saved from earlier rounds still in reserve on immediate recipe Use, without consuming them. Twin Coolant retains global cooling, conditional Corrosion 3 and its requirement for a recipe already cooling before Use. Packed Lunch restores 5 Bolt HP, or 10 when the saved-part condition is met, including while disabled. Costs and cooldowns are unchanged.",
            "IV099 AD062", r"On recipe Use",
            "Both saved-Utility bonus conflicts are resolved. Split activations are resolved separately in C13 and ordinary-part sacrifices in C03. Numerical balance is untested."),
    finding("C13", "resolved", "Split Utilities combine into one immediate effect", "R01",
            "The owner approved combining the original two activations on recipe Use. Service Pair immediately repairs 6 Bolt HP and can only be used while Bolt is active. Split Battery immediately grants 4 Charge. Neither creates a part or separately stored activation. Original totals, costs, cooldowns, normal use limits and existing HP/Charge limits are preserved.",
            "AD040 NO055", r"On recipe Use",
            "Both split-activation conflicts are resolved. Ordinary-part sacrifices are resolved separately in C03. Numerical balance is untested."),
    finding("C14", "resolved", "Original barrel timing and exclusions restored", "R04",
            "The prior eight-row cleanup relied on the same mistaken assumption that a general zero-base-damage rule removed Charged Barrel. Those edits are reversed. Five rows again read Charge after its optional payment, and Hot Contact, Discharge Record and Discharge Gate retain their explicit exclusion of that payment from part-cost spending. Original amounts, recipients, durations, costs and cooldowns are preserved.",
            "NO005 NO006 NO017 NO025 NO035 NO048 NO058 NO110", r"barrel|Charge|costs",
            "The original eight rows are restored exactly. Together with C08, all thirteen interactions are valid with the retained character exception. NO058's separate Q13 timing question remains; this is not a balance result."),
    finding("Q01", "resolved", "Reactive installed Shield protects subsequent attacks", "R03",
            "The installed-part rule removes the old missed-activation objection. A stated reactive Shield grant installs ordinary parts at its trigger and can protect against subsequent attacks while installed. It does not retroactively block the triggering damage or refill previously depleted Shield. Values, trigger conditions, reset and explicit upgrade exceptions remain unchanged.",
            "MA044 MA071 MA090 MA115 IV021 NO042 NO083 NO117 AD103 AD107 AD119",
            r"gain 6 Shield after|gain 12 Shield|after|restore",
            "Automatic protection is resolved. Q13 separately retains NO083's implicit Charge-payment timing; inter-effect ordering still follows the shared specification work."),
    finding("Q02", "clarification", "Preparation-time Shield readings and payments", "R03",
            "These recipes read or spend Shield during preparation. The installed-part clarification establishes protection at enemy attack time, but does not define how a preparation payment removes protection from installed parts or how removal/reinstallation interacts with that payment. It is no longer valid to claim that only an upgrade can make these effects usable.",
            "SH085 MA027 MA037 MA061 MA068 MA081 MA112 MA118 AD095 NO010 NO079 NO116",
            r"Shield",
            "Clarify installed-value sampling and real payment bookkeeping without inventing Shield activation or granting refunds by reinstalling a paid-down part."),
    finding("Q03", "clarification", "Installed Shield order and secondary payments", "R03",
            "These effects reference other Shield parts already used or a current Shield balance. The former End Turn activation sequence is superseded by installed protection. Exact readings of prior/other installed parts and secondary payment ordering still need explicit wording; there is no activation prerequisite for protection itself.",
            "MA028 MA075 IV094 NO029 NO038",
            r"already|current Shield|other Shield",
            "Reconcile the remaining order/payment clauses with installed parts as one group, preserving values and existing costs."),
    finding("Q04", "resolved", "Remaining Shield sampled at enemy-phase end", "R02",
            "The owner authorized the same pre-reset timing clarification for similar effects. These five rows now evaluate remaining Shield after enemy actions and before reset. Deferred Iron rewards are recorded then and delivered next round; support damage, Mark and Charge already due at enemy-phase end stay there. SH066 still pays available Shield before reset rather than receiving a free count-only reward.",
            "SH066 SH091 MA055 IV092 NO044",
            r"After enemies",
            "Reset-relative timing is resolved. Preserve actual payments and original reward timing; general ordering among interacting end-phase effects remains part of the shared resolution specification."),
    finding("Q05", "resolved", "Immediate Utility payments and shared healing cap", "R01",
            "Existing immediate-Utility rules resolve the obsolete pack/crafting wording. Copper Recovery, Glass Recovery and Iron Recovery pay their listed cost once on recipe Use and grant the conversion then. Quick Patch restores HP on Use and retains its existing 8-HP-per-fight cap shared across all Uses and duplicate copies of that recipe. No Utility inventory item or second payment is introduced.",
            "SH029 SH030 SH031 SH074", r"On recipe Use|shared across",
            "Four wording findings are resolved without changing costs, cooldowns or amounts. The healing cap remains recipe-wide; duplicate recipe availability still tracks each copy independently."),
    finding("Q06", "clarification", "Recipe crafting versus immediate Utility use", "R01",
            "The effect counts crafting, crafting costs or a crafted numeric-cooldown recipe. It does not explicitly say whether immediate Utility recipe Uses count. Part-producing crafts and all recipe Uses are different sets after the Utility conversion.",
            "SH023 SH070 MA069 NO062 NO071 NO109", r"craft",
            "Make the counted event explicit. Existing clauses that explicitly count physical parts can remain narrower."),
    finding("Q07", "resolved", "Explicit part-only cooling rewards retained", "R01",
            "Both recipes explicitly restrict their reward to cooling caused by a part. Preserve that narrower condition under the existing recipe-specific scope rule. Recall Pin (NO057) is an eligible Ammo source; direct Utility cooling and normal round progression do not qualify. Remove Charge Receipt's obsolete superseded marker without changing its functional effect. The underlying cooling still affects every eligible recipe under the global-cooling rule.",
            "NO080 NO094", r"parts this round|with a part",
            "These two eligibility findings are closed by retaining the explicit wording, not by inventing a broader trigger. Recipe amounts, timing and caps remain unchanged."),
    finding("Q08", "clarification", "Status recipient implied rather than stated", "R05",
            "The effect names a main hit/target condition but omits the recipient of one status application. The owner requires recipe-defined status scope, especially when a shot has extra targets. This is a wording finding, not a proposal that the status should affect everyone.",
            "SH013 SH014 SH016 SH055 SH086 IV006 IV008 IV015 IV016 IV041 IV047 IV084 IV085 AD038",
            r"apply",
            "Make the intended recipient explicit when reviewing these rows; no blanket main-target or all-hit default has been applied."),
    finding("Q09", "clarification", "Canonical sale basis for batches and generated subparts", "R06",
            "The recipe creates multiple parts or a generated subpart without a recorded standard one-part production requirement for every resulting type. Preserve main-recipe-based resale; do not substitute the discounted producer's cost or divide a mixed batch arbitrarily.",
            "SH026 SH057 SH076 SH092 SH113 SH126 MA038 MA048 MA099 MA107 MA119 IV019 IV057 IV061 IV088 IV090 IV118 AD017 AD074 AD083 AD092 AD117 NO031 NO063 NO087 NO107 NO120",
            r"Each|receive|Make one|gain 2",
            "Record canonical one-part valuation references/requirements for these outputs. Known fresh copies inherit their original type's basis; generic copying alone is not a pricing conflict."),
    finding("Q10", "clarification", "Spread Modifier lifecycle and placement order", "R07",
            "The row is a Modifier used/consumed in planning, while the selected spread rule resolves contributions in their parts' bullet-placement order and Load remains reversible. The catalogue still leaves non-Ammo lifecycle/accounting open; the ordering record and reservation behaviour need specifying.",
            "SH005 SH051 SH082 SH110 MA041 MA111 IV076 NO050 NO082 NO115",
            r"shot|Fire|extra enemy",
            "Define how these consumed Modifier effects retain the player's chosen bullet order through Load/unload. This is not permission to refund resolved effects."),
    finding("Q11", "clarification", "Other Utility conversions and generated outputs", "R01",
            "These four effects convert a Shield part or grant named parts without selecting an existing part to copy. The owner's copying clarification resolves the copying rows; it does not require calling every other generated output a rule violation. Their classification under the production/effect distinction remains to clarify. Existing effects, costs and cooldowns are preserved; only their review annotations change.",
            "SH076 MA119 IV118 NO120", r"Sacrifice|gain 2|receive 1",
            "Review their precise conversion/grant behavior only if it exposes a real unresolved distinction. Do not reopen copying, ordinary Shield grants or stored-Utility rules; do not invent extra production costs."),
    finding("Q12", "clarification", "Installed Shield parts and shot/install hooks", "R03",
            "Impact Catch, Backplate and Rivet Collectors still use the former Shield-use timing. Automatic installed protection removes the old impossible-activation argument, but their next-shot or next-installed-part hooks need explicit arming, expiry and removal/reinstallation accounting. Their effects and values have not been silently changed.",
            "SH062 MA034 AD076", r"next main shot|main shot|next four",
            "Specify these secondary hooks under the installed-part rule, without a separate Shield activation or repeat rewards from reinstalling the same part."),
    finding("Q13", "clarification", "Implicit Shield secondary-effect costs and stat sampling", "R03",
            "These Shield rows attach payments, other resource/status gains or conditional snapshots to an implicit activation. The owner's installed-part rule replaces the protection model, but does not by itself choose when each secondary payment or snapshot happens. Preserve printed costs, conditions and values; do not pay or grant them once per enemy attack or repeatedly through removal/reinstallation. Explicitly named End Turn schedules elsewhere retain their clock under the catalogue's legacy-wording rule.",
            "MA003 MA013 MA014 MA017 MA022 MA024 MA031 MA040 MA054 MA062 MA070 MA079 MA096 AD079 NO003 NO008 NO009 NO018 NO022 NO026 NO032 NO053 NO058 NO064 NO069 NO076 NO083 NO105",
            r"activated|activation|activate",
            "Choose a consistent secondary-cost/snapshot boundary for this group before implementation. These are timing questions, not a revival of Shield activation."),
]

RULES = {
    "R01": "Utilities activate on recipe Use without a stored Utility item. Production means crafting from resources; copying an existing part is a Utility effect, not resource-based production of that copy. Copying does not require paying the copied part's normal production cost; printed recipe-use costs are unchanged. Ordinary Shield-value grants follow the selected automatic loading, removal/storage and End Turn rules. Other named-part conversion/grant classifications remain clarification work, not proven conflicts merely because a part appears.",
    "R02": "Active Shield resets at enemy-turn end by default; only explicit permanent upgrades provide retention exceptions. End-phase remaining-Shield readings/payments happen after enemy actions and before reset. Record a deferred reward then and deliver it at its stated time; explicit payments still spend available Shield.",
    "R03": "Installed Shield parts protect automatically when enemies attack; neither Fire nor End Turn activates them. End Turn only ends the player turn. Parts are counted at a recipe's stated event without being consumed by counting. Installed values add and damage depletes remaining Shield across attacks; normal enemy-turn-end reset and explicit upgrade exceptions remain. Grants install ordinary removable parts at their stated time, protecting subsequent attacks without retroactive blocking or refilling prior loss. Implicit secondary costs/snapshots remain draft; explicit End Turn conditions retain that clock and check installed source parts.",
    "R04": "Ordinary gun base damage is zero; explicit character effects, upgrades and recipes may override a base rule within their stated scope. A later general rule does not automatically revoke a specific effect. Hot Barrel and Charged Barrel remain character bonuses to valid part-built shots, with their earlier draft values. If the owner's intent to override a specific effect is unclear, ask before deleting or replacing it. Alternatives are not automatically selected or stacked.",
    "R05": "Each recipe specifies its own status recipients; there is no blanket main-target-only or all-hit-target default.",
    "R06": "A part's sale value uses the main recipe's normal one-part ingredient requirement, current resource prices, a 50% factor and whole-credit floor. Discounts/copies do not change that basis.",
    "R07": "Spread contributions are separate hits resolved in contributing-part placement order. Load can be undone before Fire; resolved effects/crafting are not automatically refunded.",
    "R08": "Enemy Weaken N reduces each hit of its attacks by N, minimum zero, without being consumed by attacks. Reduce N by 1 once after the full enemy phase, including non-attacking rounds. The player counterpart is a draft mirror using the existing main-shot calculation scope and a player-action-phase-end tick. No percentage or rounding change was selected.",
}

CHECKED_DIMENSIONS = [
    "output kind and Utility lifecycle", "installed Shield protection, reset and grants",
    "per-copy cooldown, global cooling and no-cooldown uses",
    "part-based damage, multiple shots and modifier duration",
    "status recipients, hit order, explicit copy/stack limits",
    "whole-number arithmetic and player HP costs/death",
    "resource/part persistence, collection count and Precision protection",
    "part consumption, saved-part age and canonical sale basis",
    "specific character exceptions, base defaults and unselected trait alternatives",
]


def read_rows():
    text = SOURCE.read_text(encoding="utf-8")
    original = subprocess.check_output(
        ["git", "show", f"{SOURCE_COMMIT}:{SOURCE_REL}"], cwd=ROOT
    ).decode("utf-8").replace("\r\n", "\n")
    row_pattern = r"^\| (?:SH|MA|IV|AD|NO)\d{3}\s*\|.*$"
    if re.findall(row_pattern, text, re.M) != re.findall(row_pattern, original, re.M):
        raise SystemExit("Recipe rows changed since semantic review. Re-audit affected rows before regenerating.")
    rows = []
    for number, line in enumerate(text.splitlines(), 1):
        if not re.match(r"^\| (SH|MA|IV|AD|NO)\d{3}\s*\|", line):
            continue
        cells = [cell.strip() for cell in line.strip("|").split("|")]
        assert len(cells) == 7
        keys = ("id", "name", "output", "kind", "cost", "cooldown", "effect")
        rows.append(dict(zip(keys, cells), source_line=number, source_row=line))
    expected = {f"{pool}{i:03}" for pool, n in (("SH", 126), ("MA", 120),
                ("IV", 120), ("AD", 120), ("NO", 120)) for i in range(1, n + 1)}
    assert len(rows) == 606 and {r["id"] for r in rows} == expected
    return text, rows


def excerpt(row, group):
    effect = re.sub(r"^\*\*Superseded[^*]+\*\*\s*", "", row["effect"])
    sentences = re.split(r"(?<=[.!?])\s+", effect)
    chosen = next((s for s in sentences if re.search(group["excerpt_focus"], s, re.I)), effect)
    # Quotes are verbatim; shortening is visibly indicated.
    if len(chosen) > 280:
        chosen = chosen[:280].rsplit(" ", 1)[0] + "…"
    assert chosen.rstrip("…") in row["effect"]
    return chosen


def render():
    text, rows = read_rows()
    by_id = {r["id"]: r for r in rows}
    group_by_id = {g["id"]: g for g in FINDINGS}
    assert len(group_by_id) == len(FINDINGS)
    for g in FINDINGS:
        assert len(g["recipes"]) == len(set(g["recipes"]))
        assert set(g["recipes"]) <= by_id.keys() and g["rule"] in RULES
    for row in rows:
        ids = [g["id"] for g in FINDINGS if row["id"] in g["recipes"]]
        row["finding_ids"] = ids
        row["resolved_finding_ids"] = [i for i in ids if group_by_id[i]["status"] == "resolved"]
        row["status"] = ("conflict" if any(group_by_id[i]["status"] == "conflict" for i in ids)
                         else "clarification" if any(group_by_id[i]["status"] == "clarification" for i in ids)
                         else "no_direct_conflict_identified")
        row["review"] = "Reviewed against the recorded rule dimensions; not an approval or balance result."
    totals = Counter(r["status"] for r in rows)
    for status in ("conflict", "clarification", "no_direct_conflict_identified"):
        totals.setdefault(status, 0)
    ledger = {
        "audit_date": "2026-09-19", "initial_audit_date": "2026-09-16", "source_commit": SOURCE_COMMIT,
        "rule_revision": "2026-09-19: Q05 closes through immediate Utility wording and the existing shared healing cap; Q07 retains explicit part-only cooling conditions. Character exceptions and restored barrel interactions remain. The Q13 secondary-bonus timing question is pending, not decided.",
        "source_commit_scope": "All 606 printed rows match the reviewed snapshot. Compared with 9d40e07, exactly SH029/SH030/SH031/SH074/NO094 have changed effect text. Every name/output/kind/cost/cooldown and the other 601 rows are preserved. All thirteen restored barrel interactions remain unchanged.",
        "source_path": SOURCE_REL,
        "source_sha256_normalized_utf8": hashlib.sha256(text.encode()).hexdigest(),
        "method": "Assistant semantic reading of all 606 rows; manually curated findings; mechanical coverage and source-preservation checks.",
        "checked_dimensions": CHECKED_DIMENSIONS, "rules": RULES,
        "counts": dict(totals), "findings": FINDINGS, "recipes": rows,
        "limitations": "Not runtime verification, a balance simulation, an exhaustive search of recipe combinations, or owner acceptance of replacement designs.",
    }
    (DESIGN / "analysis/recipe_rule_audit.json").write_text(json.dumps(ledger, indent=2) + "\n", encoding="utf-8")

    lines = ["# Recipe rule audit — review together", "",
             "**19 September 2026 · all 606 recipes reviewed · six routine clarifications closed; Shield bonus timing pending**", "",
             f"All recipe rows match reviewed revision `{SOURCE_COMMIT[:7]}`; settled rules and explicit character/recipe exceptions remain preserved. "
             f"Found **{totals['conflict']} recipes with a definite conflict or obsolete dependency**, "
             f"**{totals['clarification']} additional recipes needing wording/dependency clarification**, and "
             f"**{totals['no_direct_conflict_identified']} with no direct conflict identified**. "
             "A recipe can have findings in several groups; group counts therefore overlap. Clarifications on a conflicting recipe are also retained.", "",
             "**Owner resolutions:** C04/C05 use ordinary Shield parts and explicit future deliveries. Following Folding Brace, the owner approved Spare Metal Brace's 8-now/conditional-5-next-round schedule and authorized analogous replacements. All twelve remaining C01 retention recipes now deliver fresh parts under their stated conditions; SH034's old expiry is resolved in C10. Parts left installed protect when enemies attack, and active Shield resets at enemy-turn end. Parts remain removable/saveable; a delivery does not copy the source's delivery or secondary effects.", "",
             "**Latest timing clarification:** Field Pocket counts remaining Shield before reset and delivers recorded Charge next round. The same pre-reset reading/payment now resolves five Q04 rows. Existing reward timing and explicit Shield costs are preserved.", "",
             "This is an audit for joint review. The owner corrected the assumption behind all thirteen barrel findings: special character effects can override base rules. Hot Barrel and Charged Barrel are restored as earlier trait drafts, and the eight recipe cleanups are reversed. The other five rows retain their original payment conditions; no replacement Charge-spending tally is introduced. The four-character review found no comparable removal of Ivo's Find the Seam or Ada's Bolt feature. Quench Recovery and Residual Current remain unselected alternatives. Those restored interactions remain unchanged by the later Utility wording pass; earlier totals and removal claims are historical. "
             "Character trait values remain draft balance; proposed alternatives do not automatically replace retained traits. "
             "No direct conflict identified means the row passed this written-rule review, not that it is implementation-ready, balanced or proven in every combination.", "",
             "**Resolved starter:** MA004 Quick Vent automatically loads its 5-Shield part; the player may use it at this End Turn or remove and save it. "
             "SH005 Split Outlet still needs Q10 spread-Modifier lifecycle clarification. "
             "**Latest consolidation:** Q05's four Utility descriptions now state immediate recipe Use and the existing shared cap; Q07 preserves two explicit part-only cooling conditions. Five effect texts changed, with all numeric values/costs/cooldowns and the other 601 rows retained. "
             "**Pending joint question:** Boiler Jacket grants 6 Shield plus 1 Heat. Should the extra Heat arrive once on first installation or at End Turn while installed? First installation, without a repeated bonus on reinstallation, is the recommendation only. Preserve explicit recipe clocks. The secondary bonus is under discussion; automatic Shield protection is settled. Related payment/snapshot cases remain pending.", "",
             "## Coverage", "", "| Pool | Reviewed | Conflict | Clarification only | No direct conflict identified |",
             "| --- | ---: | ---: | ---: | ---: |"]
    for pool in ("SH", "MA", "IV", "AD", "NO"):
        selected = [r for r in rows if r["id"].startswith(pool)]
        c = Counter(r["status"] for r in selected)
        lines.append(f"| {pool} | {len(selected)} | {c['conflict']} | {c['clarification']} | {c['no_direct_conflict_identified']} |")
    lines += ["", "All rows were read for: " + "; ".join(CHECKED_DIMENSIONS) + ".", "",
              "The [complete per-recipe ledger](analysis/recipe_rule_audit.json) contains every ID, name, original row, source line, disposition and finding references. "
              "The [audit renderer](analysis/recipe_rule_audit.py) verifies that all printed recipe rows still match the reviewed revision before regenerating this document. Rule prose may change with recorded owner clarifications; the ledger hashes the current full catalogue. "
              "It validates coverage and evidence preservation; it does not discover or prove semantic conflicts automatically.", "",
              "## Findings index", "", "| Group | Classification | Recipes | Topic |", "| --- | --- | ---: | --- |"]
    for g in FINDINGS:
        lines.append(f"| [{g['id']}](#{g['id'].lower()}) | {g['status']} | {len(g['recipes'])} | {g['title']} |")
    lines += ["", "## Rule references", "",
              "These summaries refer to the selected-rule sections in [RECIPE-CATALOGUE.md](RECIPE-CATALOGUE.md) "
              "and the owner chronology in [DECISIONS.md](../DECISIONS.md). Draft conventions are used to explain dependencies, not promoted into owner decisions.", ""]
    for key, rule in RULES.items():
        lines.append(f"- **{key}:** {rule}")
    for g in FINDINGS:
        lines += ["", f"## {g['id']}", "", f"**{g['title']} — {g['status']} · {g['rule']}**", "",
                  g["reason"], "", "**For our review:** " + g["review"], "",
                  "Exact excerpts below; full effects and costs remain in the catalogue and ledger.", "",
                  "| Recipe | Existing wording |", "| --- | --- |"]
        for recipe_id in g["recipes"]:
            r = by_id[recipe_id]
            quote = excerpt(r, g).replace("|", "\\|")
            lines.append(f"| **{r['id']} — {r['name']}** | {quote} |")
    lines += ["", "## Cases deliberately not called rule conflicts", "",
              "- Percentages, half-values and multipliers remain legal: the selected integer arithmetic floors their results. They are not automatically fractional-damage violations.",
              "- The seven player-HP-cost recipes already state survival at 1 HP. Bolt HP costs and disablement are separate from player death. No additional player-HP-cost conflict was found.",
              "- A numeric cooldown can be cleared and reused in the same turn. Self-cooling is allowed; identical effects or strong resource loops are balance concerns, not automatically violations. No new targeted-cooling or no-cooldown-refresh violation was found.",
              "- Per-recipe damage/effect caps, minimum part requirements and explicit non-stacking clauses do not impose a global shot or part-count cap.",
              "- A Utility may mark or sacrifice a real Ammo/Shield/Helper part already in reserve. That is different from requiring a nonexistent Utility part. MA030, MA066 and IV102 illustrate legal dependencies on real parts.",
              "- Explicit on-kill transfers or newly triggered support attacks differ from automatically redirecting an already assigned hit whose target died. No new forced-retargeting contradiction was identified.",
              "- Start-of-next-turn resource deliveries and temporary collection enhancements follow the existing fight-end expiry rule. They do not create an extra ordinary haul or another Precision attempt.",
              "- SH060's explicit loss of remaining Shield is not a recipe retention exception; it can be a downside when an upgrade would otherwise preserve Shield. A weaker or redundant effect is not itself a rule conflict.",
              "- The existing same-effect cooling pairs and relative recipe prices remain balance work. Renaming Burn/Corrosion/Mark/Weaken to the proposed robot vocabulary is a separate editorial migration.", "",
              "## Shared specification gaps, not 606 separate questions", "",
              "Modifier/Helper lifecycle, Shield-part payment ordering, final secondary-effect timing and precise stat sampling are still partly draft under the installed-part rule. "
              "Q03/Q10/Q12/Q13 identify concrete rows that expose open gaps. Q04's position before the reset is now clarified; ordering among interacting effects remains separate. Further clarification must preserve End Turn and the active-Shield reset. "
              "Main-shot singular wording is generally readable through the existing next-shot/default-duration rules; it is not automatically a one-shot-per-turn restriction. "
              "This review does not label every ordinary row as conflicting merely because the eventual engine still needs an effect-resolution order.", "",
              "**Next action:** review remaining clarification cases as grouped wording/timing work, respecting explicit character and recipe exceptions. Do not reopen the restored barrel-payment interactions or treat unselected alternative traits as replacements. "
              "Shield-part replacements, pre-reset timing clarifications, saved-Utility bonuses, combined immediate Utility effects and ordinary-part sacrifices are applied; unrelated replacements and gameplay implementation are not implied.", ""]
    (DESIGN / "RECIPE-RULE-AUDIT.md").write_text("\n".join(lines), encoding="utf-8")
    print(json.dumps({"reviewed": len(rows), "counts": dict(totals),
                      "groups": {g['id']: len(g['recipes']) for g in FINDINGS}}, indent=2))


if __name__ == "__main__":
    render()
