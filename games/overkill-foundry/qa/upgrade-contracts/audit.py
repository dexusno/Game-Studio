"""Independent read-only identity/link audit; never asserts semantic completeness."""
import argparse
import copy
import hashlib
import json
from pathlib import Path
import re

GAME = Path(__file__).resolve().parents[2]
AUTHOR = GAME / "core/tests/upgrade-contracts"


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def read(path):
    return json.loads(path.read_text(encoding="utf-8"))


def need(value, message):
    if not value:
        raise AssertionError(message)


def pointer(value, path):
    for component in path.split("/")[1:]:
        component = component.replace("~1", "/").replace("~0", "~")
        value = value[int(component)] if isinstance(value, list) else value[component]
    return value


def validate_links(record, contracts, proposal, archive):
    tests = {t["id"] for t in record["tests"] if t["result"] == "passed"}
    need(len(tests) == len(record["tests"]) == 106, "Distinct passed group inventory")
    executions = {e["suite"]: e for e in record["executions"]}
    log = (archive / "contracts.log").read_text(encoding="utf-8")
    named = ["contracts::" + line[5:] for line in log.splitlines() if line.startswith("PASS ")]
    need(set(named) == tests and len(named) == 106, "Actual log/group linkage")
    need("SUMMARY 106 passed, 0 failed, 6139 assertions" in log, "Actual summary")
    need(executions["contracts"]["assertions"] == 6139, "Assertion record")
    need(proposal["verified_build"] == record["verified_build"], "Build linkage")
    need(proposal["manifest_sha256"] == record["manifest_sha256"], "Manifest linkage")
    need(set(proposal["bindings"]) | set(proposal["unbound"]) == set(contracts["obligations"]), "Obligation partition")
    need(not (set(proposal["bindings"]) & set(proposal["unbound"])), "Overlapping proposal/residual")
    for operation, binding in proposal["bindings"].items():
        row = contracts["obligations"][operation]
        need(row["propose_full_binding"], "Bound residual")
        need(binding["tests"] and set(binding["tests"]) <= tests, "Missing actual PASS")
        need(binding["tests"] == [f["id"] for f in row["fixtures"]], "Wrong cited fixtures")
        for implementation in binding["implementations"]:
            need(record["inputs"][implementation["path"]] == implementation["sha256"], "Implementation hash mismatch")
    for row in contracts["obligations"].values():
        raw = pointer(read(archive / "inputs" / row["source"]["path"]), row["source"]["json_pointer"])
        need(raw["name"] == row["name"] and raw["effect"] == row["source_effect"], "Source effect mismatch")
        for fixture in row["fixtures"]:
            text = (archive / "inputs" / fixture["path"]).read_text(encoding="utf-8")
            line = text.splitlines()[fixture["line"] - 1]
            need(fixture["title"] in line and line.startswith("add("), "Fixture source/line mismatch")


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--records", type=Path, default=AUTHOR / "evidence", help="Directory containing the reviewed results/proposal and four suite logs")
    args = parser.parse_args()
    record = read(args.records / "results.json")
    proposal = read(args.records / "proposed-bindings.json")
    archive = GAME / record["archive"]
    contracts = read(archive / "inputs/core/tests/upgrade-contracts/contracts.json")
    identities = 0
    for relative, expected in record["inputs"].items():
        need(sha(archive / "inputs" / relative) == expected, "Captured input " + relative)
        identities += 1
    graph = hashlib.sha256((json.dumps(record["inputs"], indent=2, sort_keys=True) + "\n").encode()).hexdigest()
    need(graph == record["graph_sha256"], "Graph digest")
    for relative, expected in record["archive_artifacts"].items():
        need(sha(archive / relative) == expected, "Captured artifact " + relative)
        identities += 1
    for execution in record["executions"]:
        need(sha(archive / Path(execution["executable"]).name) == execution["executable_sha256"], "Executed binary linkage")
        need(sha(archive / execution["log"]) == execution["log_sha256"], "Executed log linkage")
        need(sha(args.records / execution["log"]) == execution["log_sha256"], "Recorded log linkage")
        need(execution["exit_code"] == 0, "Recorded failed execution")
    need(sha(args.records / "results.json") == proposal["evidence"]["sha256"], "Evidence digest")
    need(sha(archive / "inputs/content/cinderwall.manifest.json") == record["manifest_sha256"], "Manifest digest")
    baseline = read(archive / "inputs/content/runtime-evidence/coverage.json")
    remaining = {k for k, v in baseline["decisions"].items() if k.startswith("upgrade:") and v["status"] == "unbound"}
    manifest = read(archive / "inputs/content/cinderwall.manifest.json")
    need(remaining == set(contracts["obligations"]) and len(remaining) == 100, "Historical unbound inventory")
    need(all(k[8:] in manifest["eligible_upgrade_ids"] for k in remaining), "Eligibility")
    snapshot = read(archive / "inputs/content/catalogue.snapshot.json")
    for key, row in contracts["obligations"].items():
        need(snapshot["upgrades"][key[8:]]["effect"] == row["source_effect"], "Snapshot effect")
        raw = pointer(read(archive / "inputs" / row["source"]["path"]), row["source"]["json_pointer"])
        need(raw["id"] == key[8:], "Source id pointer")
    validate_links(record, contracts, proposal, archive)
    controls = []
    for name in ("invented PASS", "wrong source hash", "altered effect", "wrong fixture line"):
        bad_proposal, bad_contracts = copy.deepcopy(proposal), copy.deepcopy(contracts)
        key = next(iter(bad_proposal["bindings"]))
        if name == "invented PASS":
            bad_proposal["bindings"][key]["tests"] = ["contracts::not-executed"]
        elif name == "wrong source hash":
            bad_proposal["bindings"][key]["implementations"][0]["sha256"] = "0" * 64
        elif name == "altered effect":
            bad_contracts["obligations"][key]["source_effect"] += " altered"
        else:
            bad_contracts["obligations"][key]["fixtures"][0]["line"] = 1
        try:
            validate_links(record, bad_contracts, bad_proposal, archive)
        except AssertionError:
            controls.append(name)
        else:
            raise AssertionError("Negative control accepted: " + name)
    history = []
    for name in ("ugs095-failure.json", "ugs085-failure.json"):
        h = read(archive / "inputs/core/tests/upgrade-contracts/history" / name)
        directory = GAME / h["archive"]
        for relative, digest in h["inputs"].items():
            need(sha(directory / "inputs" / relative) == digest, "Historical input " + relative)
            identities += 1
        for relative, digest in h["artifacts"].items():
            need(sha(directory / relative) == digest, "Historical artifact " + relative)
            identities += 1
        text = (directory / "run.log").read_text(encoding="utf-8")
        need(h["exit_code"] == 1 and re.search(r"^FAIL ", text, re.M), "Failed history was relabelled")
        history.append({"record": name, "inputs": len(h["inputs"]), "artifacts": len(h["artifacts"]), "failures": [line for line in text.splitlines() if line.startswith("FAIL ")]})
    result = {"result": "passed", "scope": "Identity, exact source text, actual PASS and proposal linkage only; semantic sufficiency is in the independent review.",
              "source_graph_sha256": graph, "captured_inputs": len(record["inputs"]), "captured_artifacts": len(record["archive_artifacts"]),
              "checked_archive_identities_including_failed_history": identities, "source_rows_checked": 100, "actual_named_groups": 106,
              "proposed_ids": len(proposal["bindings"]), "residual_ids": sorted(k[8:] for k in proposal["unbound"]),
              "rejected_in_memory_negative_controls": controls, "preserved_failed_history": history}
    output = json.dumps(result, indent=2) + "\n"
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(output, encoding="utf-8", newline="\n")
    print(output)


if __name__ == "__main__":
    main()
