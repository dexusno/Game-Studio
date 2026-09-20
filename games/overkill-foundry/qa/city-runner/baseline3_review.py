"""Finite policy repair review; preserves all baseline-2 evidence."""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path
import subprocess


def sha(path: Path) -> str:
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument("--runner", type=Path, required=True)
    p.add_argument("--probe", type=Path, required=True)
    p.add_argument("--policy-probe", type=Path, required=True)
    p.add_argument("--game", type=Path, required=True)
    p.add_argument("--output", type=Path, required=True)
    p.add_argument("--evidence", type=Path, required=True)
    p.add_argument("--policy-version", default="baseline-3")
    p.add_argument("--pilot", type=Path)
    p.add_argument("--evidence-stem", default="baseline3")
    args = p.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    args.evidence.mkdir(parents=True, exist_ok=True)
    lines: list[str] = []
    records: list[dict] = []
    failures = 0

    def log(text: str) -> None:
        lines.append(text)
        print(text, flush=True)

    def check(condition: bool, text: str) -> None:
        nonlocal failures
        failures += not condition
        log(("PASS " if condition else "FAIL ") + text)

    log("RUNNER SHA256 " + sha(args.runner))
    log("INDEPENDENT REPLAY PROBE SHA256 " + sha(args.probe))
    log("INDEPENDENT POLICY PROBE SHA256 " + sha(args.policy_probe))
    baseline = args.game / "design/data/beta-balance-v1.json"
    selected = json.loads(baseline.read_text(encoding="utf-8"))["gathering"]["precision_models_for_future_headless_tests_percent"]["Practised"]
    expected = [selected[key] for key in ("Miss", "Good", "Perfect")]
    log("SELECTED BETA SHA256 " + sha(baseline) + " Practised=" + "/".join(map(str, expected)))
    traces: list[Path] = []
    pilot = args.pilot or args.game / "core/build/runner-baseline-3-pilot"
    cases = [("aggressive", 7, "auto"), ("aggressive", 9, "auto"),
             ("defensive", 13, "auto"), ("defensive", 2, "auto"),
             ("defensive", 4, "practised")]
    for policy, seed, precision in cases:
        case = f"{policy}-{precision}-{seed}"
        result = subprocess.run([str(args.runner), "--city", "--seed", str(seed),
                                 "--policy", policy, "--precision", precision,
                                 "--timeout", "10", "--search-budget", "1200", "--output", str(args.output / case)],
                                capture_output=True, text=True, timeout=25)
        record = json.loads(result.stdout.strip())
        expected_outcomes = {"city_complete"} if precision == "auto" else {"city_complete", "defeat"}
        check(result.returncode == 0 and record["outcome"] in expected_outcomes,
              f"fresh {case}: {record['outcome']} commands={record['actions']} hp={record['hp']} seconds={record['seconds']} hash={record['state_hash']}")
        check(record["policy_version"] == args.policy_version, case + " declares " + args.policy_version)
        check(record["hp"] == 80 + record["healing"] - record["hp_lost"] - record["hp_paid"], case + " HP ledger reconciles")
        trace = Path(record["trace"])
        traces.append(trace)
        record["trace_sha256"] = sha(trace)
        record["cli_exit"] = result.returncode
        if precision == "auto":
            check(trace.read_bytes() == (pilot / (case + ".oftrace")).read_bytes(), case + " full trace equals frozen author regression")
        # Preserve compact metrics; full per-action choices remain in ignored traces.
        records.append({key: value for key, value in record.items() if key not in ("fights", "choices")})

    author_cases = [("aggressive", seed) for seed in (1, 2, 3, 7, 9)]
    author_cases += [("defensive", seed) for seed in (1, 2, 3, 13)]
    authored_traces = [pilot / f"{policy}-auto-{seed}.oftrace" for policy, seed in author_cases]
    traces += authored_traces
    for trace in traces:
        replay = subprocess.run([str(args.runner), "--replay", str(trace)], capture_output=True, text=True, timeout=15)
        check(replay.returncode == 0, "CLI replay " + trace.name + ": " + (replay.stdout + replay.stderr).strip())
    replay = subprocess.run([str(args.probe), *map(str, traces)], capture_output=True, text=True, timeout=45)
    log(replay.stdout.strip())
    if replay.stderr:
        log(replay.stderr.strip())
    check(replay.returncode == 0, "separate parser replayed five fresh and nine author traces")

    prior = args.game / "core/build/runner-baseline-2"
    stopped = [prior / "aggressive-auto-7.oftrace", prior / "aggressive-auto-9.oftrace", prior / "defensive-auto-13.oftrace"]
    policy_probe = subprocess.run([str(args.policy_probe), *map(str, expected),
                                   str(pilot / "defensive-auto-2.oftrace"), *map(str, stopped)],
                                  capture_output=True, text=True, timeout=45)
    log(policy_probe.stdout.strip())
    if policy_probe.stderr:
        log(policy_probe.stderr.strip())
    check(policy_probe.returncode == 0, "canonical categories, stopped-state recovery and observation probes")

    # Policy version is part of trace identity even when rule sources are equal.
    original = traces[3].read_text(encoding="utf-8")
    previous_version = "baseline-3" if args.policy_version == "baseline-4" else "baseline-2"
    changed = original.replace('"' + args.policy_version + '"', '"' + previous_version + '"', 1)
    check(changed != original, "version corruption actually changed metadata")
    wrong_version = args.output / "wrong-policy-version.oftrace"
    wrong_version.write_text(changed, encoding="utf-8", newline="\n")
    replay = subprocess.run([str(args.runner), "--replay", str(wrong_version)], capture_output=True, text=True, timeout=15)
    check(replay.returncode == 2 and "Unsupported trace policy metadata" in replay.stderr + replay.stdout,
          "old policy version rejects: " + (replay.stdout + replay.stderr).strip())

    log(f"SUMMARY five fresh CLI cases, nine author traces independently replayed, failed_checks={failures}")
    (args.evidence / (args.evidence_stem + "-results.txt")).write_text("\n".join(lines) + "\n", encoding="utf-8", newline="\n")
    evidence = {"selected_practised_percent": expected, "beta_sha256": sha(baseline), "fresh_runs": records,
                "author_traces": {path.name: sha(path) for path in authored_traces},
                "prior_stopped_traces": {path.name: sha(path) for path in stopped}}
    (args.evidence / (args.evidence_stem + "-runs.json")).write_text(json.dumps(evidence, indent=2) + "\n", encoding="utf-8", newline="\n")
    return int(bool(failures))


if __name__ == "__main__":
    raise SystemExit(main())
