"""Independent inventory/linkage checks. Does not regenerate author evidence."""
import collections
import hashlib
import json
from pathlib import Path
import re

HERE = Path(__file__).resolve().parent
GAME = HERE.parents[1]
AUTHOR = GAME / "core/tests/parts"
MATERIALS = ["Iron", "Copper", "Carbon", "Glass", "Circuit"]


def digest(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path):
    return json.loads(path.read_text(encoding="utf-8"))


def audit():
    checks = 0

    def check(value, message):
        nonlocal checks
        checks += 1
        if not value:
            raise AssertionError(message)

    evidence = read(AUTHOR / "evidence/results.json")
    archive = GAME / evidence["archive"]["root"]
    contracts = read(archive / "inputs/core/tests/parts/contracts.json")
    manifest = read(archive / "inputs/content/cinderwall.manifest.json")
    proposals = read(AUTHOR / "evidence/proposed-bindings.json")
    for name, sha in evidence["inputs"].items():
        check(digest(archive / "inputs" / name) == sha, "Captured source: " + name)
    graph = hashlib.sha256((json.dumps(evidence["inputs"], indent=2, sort_keys=True) + "\n").encode()).hexdigest()
    check(graph == evidence["source_graph_sha256"], "41-input graph digest")
    for name, sha in evidence["archive"]["artifacts"].items():
        check(digest(archive / name) == sha, "Captured artifact: " + name)
    check(digest(GAME / proposals["evidence"]["path"]) == proposals["evidence"]["sha256"], "Proposal result linkage")
    check(digest(GAME / proposals["contracts"]["path"]) == proposals["contracts"]["sha256"], "Proposal contract linkage")
    for name, sha in proposals["implementations"].items():
        check(evidence["inputs"][name] == sha, "Proposal implementation identity: " + name)

    expected = {"part:" + key for key in manifest["parts"]}
    check(set(contracts["obligations"]) == expected, "Exact manifest physical-type inventory")
    check({b["operation"] for b in proposals["bindings"]} == expected and len(proposals["bindings"]) == len(expected), "Unique proposal inventory")
    log = (archive / "parts.log").read_text(encoding="utf-8")
    groups = [line[5:] for line in log.splitlines() if line.startswith("PASS ")]
    check(len(groups) == 207 and len(set(groups)) == 207, "Exactly 207 unique PASS names")
    check({x.split()[0] for x in groups} == expected, "PASS exact operation inventory")
    check({"parts::" + x for x in groups} == {b["fixture"] for b in proposals["bindings"]}, "Proposal group linkage")
    check("SUMMARY 207 passed, 0 failed, 59169 assertions;" in log and "FAIL " not in log, "Authored summary/log consistency")

    # Parse the selected catalogue prose directly, without the author's snapshot
    # or generated oracle: the suffixed Rare heading is deliberately supported.
    text = (archive / "inputs/design/RECIPE-CATALOGUE.md").read_text(encoding="utf-8")
    rows = {}
    rarity = None
    for number, line in enumerate(text.splitlines(), 1):
        heading = re.match(r"### (Base|Common|Uncommon|Rare|Legendary)\b", line)
        if heading:
            rarity = heading.group(1)
        match = re.match(r"\| ((?:SH|MA)\d{3}) \|", line)
        if not match:
            continue
        columns = [x.strip() for x in line.split("|")[1:-1]]
        cost = {material: int(value) for value, material in re.findall(r"(\d+) (Iron|Copper|Carbon|Glass|Circuit)", columns[4])}
        rows[columns[0]] = {"line": number, "text": line, "kind": columns[3], "rarity": rarity, "cost": cost}
    parsed_cases = {}
    pattern = r'\{"([^"]+)","([^"]+)",Kind::(\w+),Rarity::(\w+),\{([^}]+)\},\{([^}]+)\},(\d+),(-?\d+)\},'
    cases = (archive / "inputs/core/tests/parts/cases.inc").read_text(encoding="utf-8")
    for key, source, kind, tier, cost, basis, count, reference in re.findall(pattern, cases):
        parsed_cases[key] = dict(source=source, kind=kind, rarity=tier, cost=list(map(int,cost.split(","))), basis=list(map(int,basis.split(","))), count=int(count))
    check(len(parsed_cases) == 178, "178 physical recipe inputs parsed")

    producer_claims = 0
    unexecuted_origins = {}
    for key, value in manifest["parts"].items():
        contract = contracts["obligations"]["part:" + key]
        for field in ["kind", "canonical_recipe", "origins", "resale"]:
            check(value[field] == contract[field], f"{key}: {field}")
        check(contract["selected"] is True, key + ": selected")
        paths = contract["producer_paths"]
        claimed = {p for p in paths if not p.startswith("free ")}
        origins = {p.split(":",1)[1] for p in value["origins"]}
        check(claimed <= origins, key + ": claimed producer not in source inventory")
        producer_claims += len(claimed)
        if origins - claimed:
            unexecuted_origins[key] = sorted(origins - claimed)
        if key in parsed_cases:
            case = parsed_cases[key]
            row = rows[case["source"]]
            source = contract["source"]
            check(source["exact_text"] == row["text"] and source["line"] == row["line"], key + ": source location/text")
            check(case["rarity"] == row["rarity"], key + ": direct heading rarity")
            check(case["cost"] == [row["cost"].get(m,0) for m in MATERIALS], key + ": direct printed paid vector")
            # Spread is an execution kind only for catalogue rows explicitly loaded at Fire.
            spread = "Load into the bullet; consumed at Fire." in row["text"]
            check(case["kind"] == ("Spread" if spread else row["kind"]), key + ": physical placement kind")
            basis = {"Iron":1,"Carbon":1} if key == "warm-rivet" else row["cost"]
            check(case["basis"] == [basis.get(m,0) for m in MATERIALS], key + ": source one-part basis")
            check(case["count"] == (2 if key == "warm-rivet" else 1), key + ": output count")
            kind = case["kind"]
            for copier, allowed in contract["copy_eligibility"].items():
                expected_copy = kind in {"Ammo","Shield"} and (case["rarity"] in {"Base","Common","Uncommon","Rare"} if copier == "SH118" else case["rarity"] == "Common" and (kind == "Ammo" or copier == "SH102"))
                check(allowed == expected_copy, key + ": copy applicability " + copier)
    # Historical compact files are checked from their preserved indices;
    # this audit never recaptures or updates those reports.
    histories = {}
    for name in ["recipe-backed-123", "physical-types-206"]:
        folder = AUTHOR / "history" / name
        record = read(folder / "archive.json")
        for file, sha in record["files"].items():
            check(digest(folder / file) == sha, "Historical file: " + name + "/" + file)
        histories[name] = record["files"]
    return {
        "checks": checks, "status": "passed", "source_graph_sha256": graph,
        "frozen_inputs": len(evidence["inputs"]), "named_groups": len(groups),
        "physical_types": len(expected), "direct_recipe_rows": len(parsed_cases),
        "runtime_kind_counts": dict(collections.Counter(c["kind"] for c in parsed_cases.values())),
        "non_free_producer_path_claims": producer_claims,
        "manifest_origins_not_claimed_by_type_fixture": unexecuted_origins,
        "historical_compact_files": histories,
        "live_sources_differing_from_capture": [name for name, sha in evidence["inputs"].items() if digest(GAME / name) != sha],
        "limits": "This checks complete inventory/linkage and direct prose fields, not semantic sufficiency of every asserted lifecycle or every producer branch."
    }


if __name__ == "__main__":
    result = audit()
    print(json.dumps(result, indent=2, sort_keys=True))
