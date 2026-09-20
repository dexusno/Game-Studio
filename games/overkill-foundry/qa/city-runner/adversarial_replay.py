"""Independent corruption checks against the authored CLI; bulk traces stay ignored."""
from __future__ import annotations
import argparse
import hashlib
from pathlib import Path
import subprocess


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--runner", type=Path, required=True)
    parser.add_argument("--trace", type=Path, required=True)
    parser.add_argument("--output", type=Path, required=True)
    args = parser.parse_args()
    args.output.mkdir(parents=True, exist_ok=True)
    original = args.trace.read_text(encoding="utf-8").splitlines()
    cases: list[tuple[str, list[str], bool]] = [("original", original[:], True)]

    def alter(name: str, index: int, value: str) -> None:
        lines = original[:]
        lines[index] = value
        cases.append((name, lines, False))

    alter("metadata_missing_fields", 1, "META ")
    alter("metadata_wrong_rules", 1, original[1].replace('"of-core-0.4"', '"wrong-rules"'))
    alter("metadata_wrong_policy", 1, original[1].replace('"defensive"', '"unknown-policy"').replace('"aggressive"', '"unknown-policy"'))
    alter("metadata_trailing_fields", 1, original[1] + " extraneous")
    final_index = next(i for i, line in enumerate(original) if line.startswith("FINAL "))
    final_fields = original[final_index].split(" ", 3)
    final_fields[2] = "defeat" if final_fields[2] == "city_complete" else "city_complete"
    alter("final_false_outcome", final_index, " ".join(final_fields))
    alter("final_trailing_fields", final_index, original[final_index] + " extraneous")
    altered_final = original[final_index][:-1] + ("1" if original[final_index][-1] != "1" else "2")
    alter("final_corrupt_bytes", final_index, altered_final)
    action_index = next(i for i, line in enumerate(original) if line.startswith("A "))
    alter("command_truncated", action_index, original[action_index][:-8])
    event_index = next(i for i, line in enumerate(original) if line.startswith("E "))
    alter("event_corrupt", event_index, original[event_index].replace('"amount":', '"wrong_amount":', 1))
    result_index = next(i for i, line in enumerate(original) if line.startswith("R "))
    alter("result_trailing_fields", result_index, original[result_index] + " extraneous")
    cases.append(("missing_final", original[:final_index], False))

    print("RUNNER SHA256 " + hashlib.sha256(args.runner.read_bytes()).hexdigest())
    print("BASE TRACE SHA256 " + hashlib.sha256(args.trace.read_bytes()).hexdigest())
    failures = 0
    for name, lines, should_pass in cases:
        path = args.output / (name + ".oftrace")
        path.write_text("\n".join(lines) + "\n", encoding="utf-8", newline="\n")
        result = subprocess.run([str(args.runner.resolve()), "--replay", str(path.resolve())],
                                capture_output=True, text=True, timeout=15)
        accepted = result.returncode == 0
        ok = accepted == should_pass
        failures += not ok
        detail = (result.stdout + result.stderr).strip()
        print(f"{'PASS' if ok else 'FAIL'} {name}: expected={'accept' if should_pass else 'reject'} "
              f"observed={'accept' if accepted else 'reject'} exit={result.returncode}; {detail}")
    print(f"SUMMARY {len(cases)-failures} passed, {failures} failed")
    return int(bool(failures))


if __name__ == "__main__":
    raise SystemExit(main())
