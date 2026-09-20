"""Independent P08 source selection; structural evidence, never effect acceptance."""
from __future__ import annotations

import argparse
import hashlib
import json
from pathlib import Path

GAME = Path(__file__).resolve().parents[2]
SOURCES = (
    "design/data/shared-upgrades.json",
    "design/data/character-mayor-upgrades.json",
)
SELECTED = (
    "UGS-001", "UGS-009", "UGS-015", "UGS-017", "UGS-019",
    "UGS-032", "UGS-038", "UGS-039", "UGS-044", "UGS-046", "UGS-053",
    "UGS-055", "UGS-056", "UGS-066", "UGS-071", "UGS-075",
    "UGS-076", "UGS-089", "UGS-094", "UGS-100", "UGS-102", "UGS-106",
    "UGS-113", "UGS-116", "UGS-119", "UGS-123", "UGS-129", "UGS-133", "UGS-134",
    "UGS-136", "UGS-139", "UGS-140", "UGS-144", "UGS-145", "UGS-148",
    "UGS-150", "MAU-01", "MAU-02", "MAU-03", "MAU-04", "MAU-05",
    "MY1-05", "MY1-08", "MY1-09", "MY1-10", "MY1-13", "MY1-21", "MY1-23", "MY3-03",
)


def inspect() -> dict:
    rows = {}
    source_hashes = {}
    for source in SOURCES:
        raw = (GAME / source).read_bytes()
        source_hashes[source] = hashlib.sha256(raw).hexdigest()
        for row in json.loads(raw)["upgrades"]:
            assert row["id"] not in rows, f"Repeated source ID: {row['id']}"
            rows[row["id"]] = {**row, "source": source}

    # Re-derive the selected city from raw source fields. Do not use generated
    # pool membership or ID prefixes to infer character, city, or Mayor status.
    eligible = {
        key: row for key, row in rows.items()
        if row["mercenary"] in ("Shared", "Mara")
        and row["min_city"] <= 1 <= row["max_city"]
    }
    mayors = {key for key, row in eligible.items() if "Mayor" in row["acquisition"]}
    ordinary = set(eligible) - mayors
    manifest_path = "content/cinderwall.manifest.json"
    manifest_raw = (GAME / manifest_path).read_bytes()
    manifest = json.loads(manifest_raw)
    assert len(eligible) == 152, f"Selected source count changed: {len(eligible)}"
    assert len(ordinary) == 127 and len(mayors) == 25
    for field, expected in (("eligible_upgrade_ids", set(eligible)),
                            ("ordinary_upgrade_ids", ordinary),
                            ("mayor_upgrade_ids", mayors)):
        actual = manifest[field]
        assert len(actual) == len(set(actual)), f"Duplicate in {field}"
        assert set(actual) == expected, (
            f"{field}: missing {sorted(expected-set(actual))}; "
            f"extra {sorted(set(actual)-expected)}"
        )
    assert "UGS-067" not in eligible and rows["UGS-067"]["min_city"] == 2
    assert "MY3-03" in mayors, "Legacy ID prefix is not its current city gate"
    assert "MY2-03" in ordinary, "Legacy Mayor-style ID is now an ordinary upgrade"
    assert set(SELECTED) <= set(eligible), f"Proposed probe IDs outside selected city: {set(SELECTED)-set(eligible)}"
    source_hashes[manifest_path] = hashlib.sha256(manifest_raw).hexdigest()
    for source in ("design/UPGRADE-SYSTEM.md", "design/TIMING-AND-PERSISTENCE.md",
                   "design/RECIPE-CATALOGUE.md", "design/MVP-IMPLEMENTATION-PLAN.md",
                   "design/PART-RESALE.md", "design/MERCENARY-ABILITIES.md"):
        source_hashes[source] = hashlib.sha256((GAME / source).read_bytes()).hexdigest()
    return {
        "scope": "Mara / Cinderwall / city 1",
        "evidence_kind": "source selection and expectations, not runtime effect coverage",
        "eligible_count": len(eligible), "ordinary_count": len(ordinary),
        "mayor_count": len(mayors), "eligible_ids": sorted(eligible),
        "excluded_rescue": {key: rows["UGS-067"][key]
                            for key in ("id", "name", "min_city", "max_city", "effect")},
        "source_sha256": source_hashes,
        "selected_contracts": [{key: eligible[item][key]
                                for key in ("id", "name", "source", "effect")}
                               for item in SELECTED],
    }


if __name__ == "__main__":
    parser = argparse.ArgumentParser()
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    evidence = inspect()
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(evidence, indent=2, ensure_ascii=False) + "\n", encoding="utf-8")
    print("PASS raw source selection: 152 eligible / 127 ordinary / 25 Mayor; exact manifest sets")
    print("PASS city/identity exceptions: UGS-067 excluded, MY3-03 Mayor, MY2-03 ordinary")
    print(f"RECORDED {len(SELECTED)} selected source contracts; no semantic runtime claim")
