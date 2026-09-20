"""Finite independent baseline-5 recheck; never accesses evaluation seeds 121-170."""
from __future__ import annotations
import argparse
import json
import subprocess
from pathlib import Path
from baseline3_review import sha


def main() -> int:
    p = argparse.ArgumentParser()
    for name in ("runner", "probe", "legacy-policy-probe", "policy-probe", "game", "output", "evidence"):
        p.add_argument("--" + name, type=Path, required=True)
    args = p.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    args.evidence.mkdir(parents=True, exist_ok=True)
    lines: list[str] = []
    failures = 0

    def log(value: str) -> None:
        lines.append(value)
        print(value, flush=True)

    def check(ok: bool, value: str) -> None:
        nonlocal failures
        failures += not ok
        log(("PASS " if ok else "FAIL ") + value)

    def execute(command: list[str], timeout: int = 45) -> subprocess.CompletedProcess[str]:
        return subprocess.run(command, capture_output=True, text=True, timeout=timeout)

    log("RUNNER SHA256 " + sha(args.runner))
    log("REPLAY PROBE SHA256 " + sha(args.probe))
    log("LEGACY POLICY PROBE SHA256 " + sha(args.legacy_policy_probe))
    log("NEW POLICY PROBE SHA256 " + sha(args.policy_probe))
    beta = args.game / "design/data/beta-balance-v1.json"
    selected = json.loads(beta.read_text(encoding="utf-8"))["gathering"]["precision_models_for_future_headless_tests_percent"]
    names = ("Miss", "Good", "Perfect")
    models = {key: [selected[key][band] for band in names] for key in ("Learning", "Practised", "Expert")}
    log("SELECTED BETA SHA256 " + sha(beta) + " models=" + json.dumps(models))
    pilot = args.game / "core/build/runner-baseline-5-pilot"
    records: list[dict] = []
    fresh_traces: list[Path] = []
    cases = [("aggressive", 57, "auto", 1200, "10", "city_complete"),
             ("defensive", 57, "auto", 1200, "10", "city_complete"),
             ("aggressive", 2, "auto", 1, "10", "defeat"),
             ("defensive", 2, "auto", 1200, "0.000001", "policy_timeout"),
             ("aggressive", 53, "expert", 1200, "10", "blocked_rule")]
    for index, (policy, seed, precision, budget, timeout, outcome) in enumerate(cases):
        result = execute([str(args.runner), "--city", "--seed", str(seed), "--policy", policy,
                          "--precision", precision, "--search-budget", str(budget), "--timeout", timeout,
                          "--output", str(args.output / f"case-{index}")], 25)
        record = json.loads(result.stdout.strip())
        expected_exit = 2 if outcome in ("policy_timeout", "blocked_rule") else 0
        check(record["outcome"] == outcome and result.returncode == expected_exit,
              f"fresh {policy}/{seed}/budget{budget}: {record['outcome']} exit={result.returncode} actions={record['actions']} hp={record['hp']} hash={record['state_hash']}")
        check(record["policy_version"] == "baseline-5", f"case{index} declares baseline-5")
        check(record["hp"] == 80 + record["healing"] - record["hp_lost"] - record["hp_paid"], f"case{index} HP ledger")
        trace = Path(record["trace"])
        fresh_traces.append(trace)
        if seed == 57:
            check(trace.read_bytes() == (pilot / f"{policy}-auto-57.oftrace").read_bytes(), f"{policy}57 equals frozen author trace bytes")
        if seed == 53:
            author = args.game / "core/build/runner-baseline-5/held-rule-controls/aggressive-expert-53.oftrace"
            check(trace.read_bytes() == author.read_bytes(), "natural held control equals author trace")
            check("owner decision" in record["reason"] and record["hp"] > 0, "held rule remains distinct from gameplay defeat")
        record["trace_sha256"] = sha(trace)
        record["exit"] = result.returncode
        records.append({k: v for k, v in record.items() if k not in ("fights", "choices")})

    authored = [pilot / f"aggressive-auto-{n}.oftrace" for n in (1, 2, 3, 7, 9, 57)]
    authored += [pilot / f"defensive-auto-{n}.oftrace" for n in (1, 2, 3, 13, 57)]
    for trace in fresh_traces + authored:
        result = execute([str(args.runner), "--replay", str(trace)])
        check(result.returncode == 0, "CLI replay " + trace.name + ": " + (result.stdout + result.stderr).strip())
    result = execute([str(args.probe), *map(str, fresh_traces + authored)])
    log(result.stdout.strip())
    if result.stderr:
        log(result.stderr.strip())
    check(result.returncode == 0, "separate decoder checks all 16 selected trace artifacts")

    ordinary = pilot / "defensive-auto-2.oftrace"
    old = args.game / "core/build/runner-baseline-2"
    stopped = [old / "aggressive-auto-7.oftrace", old / "aggressive-auto-9.oftrace", old / "defensive-auto-13.oftrace"]
    result = execute([str(args.legacy_policy_probe), *map(str, models["Practised"]), str(ordinary), *map(str, stopped)])
    log(result.stdout.strip())
    if result.stderr:
        log(result.stderr.strip())
    check(result.returncode == 0, "unchanged six prior policy groups")
    fourth = args.game / "core/build/runner-baseline-4/aggressive-auto-57.oftrace"
    percentages = [str(value) for model in models.values() for value in model]
    result = execute([str(args.policy_probe), str(ordinary), str(fourth), *percentages])
    log(result.stdout.strip())
    if result.stderr:
        log(result.stderr.strip())
    check(result.returncode == 0, "new category, Auto, retained Warden, visibility and held-boundary groups")

    original = fresh_traces[0].read_text(encoding="utf-8")
    changed = original.replace('"baseline-5"', '"baseline-4"', 1)
    check(changed != original, "false policy version mutation changes metadata")
    wrong = args.output / "wrong-policy-version.oftrace"
    wrong.write_text(changed, encoding="utf-8", newline="\n")
    result = execute([str(args.runner), "--replay", str(wrong)])
    check(result.returncode == 2 and "Unsupported trace policy metadata" in result.stdout + result.stderr,
          "old policy version rejected")
    result = execute(["python", "-X", "utf8", str(Path(__file__).with_name("adversarial_replay.py")),
                      "--runner", str(args.runner), "--trace", str(fresh_traces[0]),
                      "--output", str(args.output / "corruptions")])
    log(result.stdout.strip())
    if result.stderr:
        log(result.stderr.strip())
    check(result.returncode == 0, "unchanged trace-integrity adversarial cases")
    log(f"SUMMARY five fresh CLI cases, eleven author trace replays, failed_checks={failures}")
    (args.evidence / "baseline5-results.txt").write_text("\n".join(lines) + "\n", encoding="utf-8", newline="\n")
    payload = {"beta_sha256": sha(beta), "selected_models": models, "fresh_runs": records,
               "author_traces": {p.name: sha(p) for p in authored},
               "prior_stopped_traces": {p.name: sha(p) for p in stopped + [fourth]}}
    (args.evidence / "baseline5-runs.json").write_text(json.dumps(payload, indent=2) + "\n", encoding="utf-8", newline="\n")
    return int(bool(failures))


if __name__ == "__main__":
    raise SystemExit(main())
