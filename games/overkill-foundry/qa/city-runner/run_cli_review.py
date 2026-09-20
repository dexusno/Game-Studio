"""A finite ordinary-run matrix, deterministic replay and honest stop categories."""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import re
import subprocess


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--runner", type=Path, required=True)
    parser.add_argument("--probe", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    parser.add_argument("--evidence", type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    args.evidence.mkdir(parents=True, exist_ok=True)
    lines: list[str] = []
    records: list[dict] = []
    traces: list[Path] = []
    failures = 0

    def log(message: str) -> None:
        lines.append(message)
        print(message, flush=True)

    cases = [(f"{policy}-s{seed}", seed, policy, "auto", 1200, 10.0, "normal")
             for policy in ("aggressive", "defensive") for seed in (1, 2, 3)]
    cases += [(f"skill-{skill}", 4, "defensive", skill, 1200, 10.0, "normal")
              for skill in ("learning", "practised", "expert")]
    cases += [("weak-policy", 2, "aggressive", "auto", 1, 10.0, "defeat"),
              ("watchdog", 2, "defensive", "auto", 1200, 0.000001, "policy_timeout"),
              ("repeat-defensive-s2", 2, "defensive", "auto", 1200, 10.0, "normal")]
    log("RUNNER SHA256 " + hashlib.sha256(args.runner.read_bytes()).hexdigest())
    log("INDEPENDENT PROBE SHA256 " + hashlib.sha256(args.probe.read_bytes()).hexdigest())
    for name, seed, policy, skill, budget, timeout, expectation in cases:
        destination = args.output / name
        command = [str(args.runner), "--city", "--seed", str(seed), "--policy", policy,
                   "--precision", skill, "--search-budget", str(budget), "--timeout", str(timeout),
                   "--output", str(destination)]
        result = subprocess.run(command, capture_output=True, text=True, timeout=25)
        record = json.loads(result.stdout.strip())
        record["case"] = name
        record["cli_exit"] = result.returncode
        record["configured_search_budget"] = budget
        record["configured_timeout_seconds"] = timeout
        records.append(record)
        trace = Path(record["trace"])
        traces.append(trace)
        expected_outcomes = {"city_complete", "defeat", "blocked_rule"} if expectation == "normal" else {expectation}
        expected_code = 0 if record["outcome"] in ("city_complete", "defeat") else 2
        ok = record["outcome"] in expected_outcomes and result.returncode == expected_code
        failures += not ok
        log(f"{'PASS' if ok else 'FAIL'} {name}: {record['outcome']} hp={record['hp']} "
            f"position={record['position']} actions={record['actions']} exit={result.returncode} "
            f"hash={record['state_hash']}")
        reconciled = record["hp"] == 80 + record["healing"] - record["hp_lost"] - record["hp_paid"]
        failures += not reconciled
        log(f"{'PASS' if reconciled else 'FAIL'} {name} HP ledger: "
            f"80 + {record['healing']} heal - {record['hp_lost']} damage - {record['hp_paid']} payment = {record['hp']}")
        replay = subprocess.run([str(args.runner), "--replay", str(trace)], capture_output=True, text=True, timeout=15)
        ok = replay.returncode == 0 and record["state_hash"] in replay.stdout
        failures += not ok
        log(f"{'PASS' if ok else 'FAIL'} {name} authored replay: {(replay.stdout+replay.stderr).strip()}")
    original = traces[[record["case"] for record in records].index("defensive-s2")]
    same = original.read_bytes() == traces[-1].read_bytes()
    failures += not same
    log(f"{'PASS' if same else 'FAIL'} repeat-defensive-s2 full trace bytes equal")
    independent = subprocess.run([str(args.probe), *map(str, traces)], capture_output=True, text=True, timeout=45)
    log(independent.stdout.strip())
    if independent.stderr:
        log(independent.stderr.strip())
    failures += independent.returncode != 0
    paid = [int(value) for value in re.findall(r"known_hp_paid=(\d+)", independent.stdout)]
    agree = len(paid) == len(records) and all(value == record["hp_paid"] for value, record in zip(paid, records))
    failures += not agree
    log(f"{'PASS' if agree else 'FAIL'} all independently replayed HP payments equal reported totals")
    (args.evidence / "cli-results.txt").write_text("\n".join(lines) + "\n", encoding="utf-8", newline="\n")
    (args.evidence / "runs.json").write_text(json.dumps(records, indent=2) + "\n", encoding="utf-8", newline="\n")
    print(f"SUMMARY {len(cases)} bounded CLI cases, {failures} failed checks")
    return int(bool(failures))


if __name__ == "__main__":
    raise SystemExit(main())
