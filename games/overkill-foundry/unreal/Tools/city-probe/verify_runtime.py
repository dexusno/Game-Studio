"""Compare an executed Unreal city probe with its frozen CPU preparation."""
from __future__ import annotations
import argparse
import hashlib
import json
import pathlib
import re


def sha(path: pathlib.Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def need(value: bool, reason: str) -> None:
    if not value:
        raise RuntimeError(reason)


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--prepared", type=pathlib.Path, required=True)
    parser.add_argument("--runtime", type=pathlib.Path, required=True)
    parser.add_argument("--log", type=pathlib.Path, required=True)
    parser.add_argument("--binary-identity", type=pathlib.Path, required=True)
    parser.add_argument("--report", type=pathlib.Path, required=True)
    args = parser.parse_args()
    need(not args.report.exists(), "Use a new report path; prior evidence is never replaced")
    identity = json.loads((args.prepared / "identity.json").read_text(encoding="utf-8"))
    cpu = args.prepared / "cpu17"
    expected = json.loads((cpu / "summary.json").read_text(encoding="utf-8"))
    actual = json.loads((args.runtime / "summary.json").read_text(encoding="utf-8"))
    need(expected["ok"] and actual["ok"], "A probe reported failure")
    for field in ("commands", "revision", "final_hash", "position", "hp", "seed"):
        need(actual[field] == expected[field], f"Final {field} differs from the CPU session")
    need(actual["core_digest"] == identity["core_digest"] and actual["review_snapshot"] == 1, "Rendered source digest/override differs")
    need(sha(args.runtime / "input.oftrace") == identity["trace_sha256"], "Rendered input trace differs")
    need((args.runtime / "transactions.jsonl").read_bytes() == (cpu / "transactions.jsonl").read_bytes(),
         "Ordered action/state/revision/receipt transcript differs")
    expected_events = []
    step, index = 0, 0
    for line in (args.prepared / "input.oftrace").read_text(encoding="utf-8").splitlines():
        if line.startswith("A "):
            step += 1
            index = 0
        elif line.startswith("E "):
            expected_events.append({"step": step, "index": index, "event": json.loads(line[2:])})
            index += 1
    events = [json.loads(line) for line in (args.runtime / "events.jsonl").read_text(encoding="utf-8").splitlines()]
    need(events == expected_events, "Actual committed events differ in order, transaction identity or payload")
    captures = [json.loads(line) for line in (args.runtime / "captures.jsonl").read_text(encoding="utf-8").splitlines()]
    need(len(captures) == actual["captures"] == 37, "Expected finite capture set is incomplete")
    need(len({entry["file"] for entry in captures}) == len(captures), "Duplicate capture identities")
    for entry in captures:
        path = args.runtime / "screenshots" / entry["file"]
        need(path.is_file() and path.read_bytes().startswith(b"\x89PNG\r\n\x1a\n"), "Missing or non-PNG capture")
        if "-loaded.png" in entry["file"]:
            need(entry["action_view"] and not entry["busy"], "Load capture did not finish the camera transition")
        if "rewards-after-presentation" in entry["file"]:
            need(not entry["busy"], "Rewards capture preceded presentation completion")
    log = args.log.read_text(encoding="utf-8-sig", errors="replace")
    need(len(re.findall(r"CITY_TRANSACTION ok=1 step=", log)) == 257, "Wrong number of executed successful transactions")
    need("FOUNDRY_CITY_PROBE_COMPLETE ok=0" not in log, "A runtime failure was logged")
    need(re.search(r"FOUNDRY_CITY_PROBE_COMPLETE ok=1 commands=257 captures=37 hash=241e46199e519eef position=13 hp=80 revision=258", log) is not None,
         "Missing exact runtime completion marker")
    shots = re.findall(r"SHOT_PRESENT event=\d+ action_view=(\d) settled=(\d)", log)
    need(len(shots) == 39 and all(row == ("1", "1") for row in shots), "Recorded shots did not all present in the settled loaded view")
    binary = json.loads(args.binary_identity.read_text(encoding="utf-8-sig"))
    need(binary["exitCode"] == 0, "Unreal process did not exit normally")
    need(binary["sha256Before"].lower() == binary["sha256After"].lower(), "Unreal module changed during execution")
    report = {
        "ok": True,
        "scope": "Automated saved transaction/event/receipt parity; not human play or visual approval",
        "commands": 257, "ordered_events": len(events), "disk_reload_count": 257,
        "captures": len(captures), "settled_loaded_shots": len(shots),
        "final_hash": actual["final_hash"], "position": actual["position"], "hp": actual["hp"],
        "core_digest": identity["core_digest"], "cpu_source_graph": identity["source_graph_sha256"],
        "module_sha256": binary["sha256Before"].lower(), "trace_sha256": identity["trace_sha256"],
        "artifacts": {name: sha(args.runtime / name) for name in ("input.oftrace", "transactions.jsonl", "events.jsonl", "captures.jsonl", "summary.json", "final.ofsave")},
        "log_sha256": sha(args.log), "binary_identity_sha256": sha(args.binary_identity),
        "capture_sha256": {entry["file"]: sha(args.runtime / "screenshots" / entry["file"]) for entry in captures},
    }
    args.report.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({key: report[key] for key in ("ok", "commands", "ordered_events", "captures", "settled_loaded_shots", "final_hash")}))


if __name__ == "__main__":
    main()
