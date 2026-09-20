"""Produce a proposed-only review index, never a runtime-support binding."""
import argparse
import hashlib
import json
from pathlib import Path
import re

HERE = Path(__file__).resolve().parent
GAME = HERE.parents[2]
RESIDUAL = {
    "UGS-011": "Full combined first-base-haul deduction composition is still owner-held. No full binding proposed.",
    "UGS-018": "Full combined first-base-haul deduction composition is still owner-held. No full binding proposed.",
    "UGS-126": "Full combined first-base-haul deduction composition is still owner-held. No full binding proposed.",
    "UGS-141": "Owner-held composition of the +2 first base-haul voucher. No substitute material default.",
    "MAU-05": "Executed paid-HP ordering, death interruption, actual6 cap, once-turn renewal and full-HP-before-Use. Full HP at the post-reaction trigger (zero actual healing but spent opportunity) has no demonstrated eligible natural producer chain; that exact branch remains unclaimed.",
    "UGS-029": "Executed three distinct actual Cinderwall enemy debuff types, existing-stack exclusion and cleared-type reapplication. The fourth distinct enemy debuff needed to distinguish a3 cap from an unbounded counter is not produced by the enabled C1 robots. No positive self-inflicted new debuff producer is present in the tested eligible recipe pool. These branches remain unclaimed.",
}
EXTRA = {
    "UGS-095": ["category:modifier-spread"],
    "UGS-031": ["edge:normal-rarity"],
    "MY1-01": ["edge:paid-output"],
    "UGS-121": ["edge:paid-output"],
    "UGS-037": ["edge:entry-setup"],
    "UGS-045": ["edge:late-officer"],
    "UGS-125": ["edge:late-officer"],
    "UGS-017": ["edge:credit-origin"],
    "UGS-085": ["edge:credit-origin", "edge:bond-reward"],
    "UGS-015": ["edge:unused-lease"],
    "UGS-039": ["edge:calibration-eligibility"],
    "UGS-040": ["edge:reward-origin"],
    "UGS-041": ["edge:reward-origin"],
    "UGS-134": ["edge:reward-origin"],
}
CAMPAIGN = {"UGS-012", "UGS-016", "UGS-017", "UGS-031", "UGS-033", "UGS-034", "UGS-040", "UGS-041", "UGS-062", "UGS-085", "UGS-101", "UGS-134", "UGS-147", "UGS-148", "MY1-13", "MY1-14", "MY1-16"}


def encoded(value):
    return (json.dumps(value, indent=2, sort_keys=True) + "\n").encode()


def make_contracts():
    coverage = json.loads((GAME / "content/runtime-evidence/coverage.json").read_text(encoding="utf-8"))
    snapshot = json.loads((GAME / "content/catalogue.snapshot.json").read_text(encoding="utf-8"))
    operations = sorted(key for key, value in coverage["decisions"].items() if key.startswith("upgrade:") and value["status"] == "unbound")
    assert len(operations) == 100
    fixtures = {}
    for path in sorted(HERE.glob("*.inl")):
        text = path.read_text(encoding="utf-8")
        matches = list(re.finditer(r'add\("([^"\n]+)","([^"\n]+)"', text))
        for index, match in enumerate(matches):
            code = text[match.start():matches[index + 1].start() if index + 1 < len(matches) else len(text)]
            fixtures[match[1]] = {
                "id": "contracts::upgrade:" + match[1] + " " + match[2],
                "path": path.relative_to(GAME).as_posix(),
                "line": text[:match.start()].count("\n") + 1,
                "title": match[2],
                "assertion_messages": sorted(set(re.findall(r',"([^"\n]+)"\);', code))),
            }
    cases = {}
    for operation in operations:
        id_ = operation.split(":", 1)[1]
        source = snapshot["upgrades"][id_]
        selected = [id_] + EXTRA.get(id_, []) if id_ in fixtures else []
        for name in selected:
            assert name in fixtures
        implementations = ["core/src/upgrade_effects.inl", "core/src/core.cpp", "core/src/upgrades.cpp"]
        if id_ in CAMPAIGN:
            implementations += ["core/src/campaign.cpp", "core/src/campaign_upgrades.cpp"]
        if id_ == "UGS-058":
            implementations += ["presentation/precision.hpp"]
        cases[operation] = {
            "name": source["name"], "source": source["source"], "source_effect": source["effect"],
            "propose_full_binding": id_ not in RESIDUAL,
            "residual": RESIDUAL.get(id_),
            "fixtures": [fixtures[name] for name in selected],
            "implementations": implementations,
            "interpretation": "UGS031 uses normal victory rarity odds, Shared-only and no duplicate candidates, per the owner clarification in design/UPGRADE-SYSTEM.md." if id_ == "UGS-031" else None,
        }
    assert sum(row["propose_full_binding"] for row in cases.values()) == 94
    assert len([key for key in fixtures if key in {op[8:] for op in operations}]) == 96
    return {
        "schema_version": 1,
        "status": "Author-proposed contract linkage; actual named execution required; independent sufficiency review pending.",
        "manifest_sha256": hashlib.sha256((GAME / "content/cinderwall.manifest.json").read_bytes()).hexdigest(),
        "baseline_coverage_sha256": hashlib.sha256((GAME / "content/runtime-evidence/coverage.json").read_bytes()).hexdigest(),
        "unbound_upgrade_obligations_reviewed": 100, "proposed_full_bindings": 94,
        "named_upgrade_cases": 96, "supplemental_cases": len(fixtures) - 96,
        "residual": RESIDUAL, "obligations": cases,
        "scope": [
            "All Rules actions and campaign transactions use the actual current C++ engine and production cinderwallUpgradeHooks.",
            "Fixed materials, recipe copies, stock, reward candidates and enemy conditions isolate clauses; they are explicit controls, not naturally rolled or balance-tested builds.",
            "Controlled plain500 Ammo only terminates real Campaign fights to exercise reward dispatch; it is not a player-accessible recipe or a city-winning policy.",
            "UGS062's restricted content pool isolates the spread-category filter and does not alter production eligibility or weights.",
            "Controlled CityStart hook calls test campaign-counter retention; UGS045's fourth controlled Officer checks the cap. No second playable city is claimed.",
            "An executed PASS proves its assertions only. This index's proposed full bindings remain subject to independent clause review.",
        ],
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    output = HERE / "contracts.json"
    expected = encoded(make_contracts())
    if args.check:
        assert output.read_bytes() == expected, "Contract review index is stale"
    else:
        output.write_bytes(expected)
    print("PASS authored contract index: 100 obligations, 94 proposed, 6 explicit residuals; not executed support.")


if __name__ == "__main__":
    main()
