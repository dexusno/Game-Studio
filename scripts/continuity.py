#!/usr/bin/env python3
"""Small local handoff helpers and Codex lifecycle hooks; no model or service."""
from __future__ import annotations

import argparse
import contextlib
from datetime import datetime, timezone
import hashlib
import io
import json
import os
from pathlib import Path
import subprocess
import sys
import tempfile

import studio


def git(root, *args):
    result = subprocess.run(["git", "-C", str(root), *args], capture_output=True,
                            text=True, encoding="utf-8", timeout=5)
    if result.returncode:
        raise studio.StudioError(result.stderr.strip() or "Git command failed")
    return result.stdout.strip()


def context(root, target=None):
    output = io.StringIO()
    with contextlib.redirect_stdout(output):
        studio.project_context(root, target)
    return output.getvalue()


def selected_record(root, target):
    kind, separator, identifier = target.partition(":")
    if not separator or kind not in ("game", "opportunity") or not studio.slug_ok(identifier):
        raise studio.StudioError("Use a qualified target: game:ID or opportunity:ID")
    records = studio.context_records(root, (kind, identifier))
    matches = [record for record in records if (record["kind"], record["id"]) == (kind, identifier)]
    if len(matches) != 1:
        raise studio.StudioError(f"Unknown or ambiguous target: {target}")
    record = matches[0]
    studio.context_sources(root, record["data"], record["path"], kind)
    return record


def local_dir(root):
    path = root / ".local" / "continuity"
    path.mkdir(parents=True, exist_ok=True)
    return path


def session_path(root, session, suffix="binding"):
    if not session:
        raise studio.StudioError("No task id available; supply --session or use Codex's CODEX_THREAD_ID")
    name = hashlib.sha256(session.encode("utf-8")).hexdigest()[:24]
    return local_dir(root) / f"{name}-{suffix}.json"


def atomic_json(path, value):
    temporary = None
    try:
        with tempfile.NamedTemporaryFile(mode="w", encoding="utf-8", dir=path.parent,
                                         prefix=".write-", delete=False) as handle:
            temporary = Path(handle.name)
            json.dump(value, handle, ensure_ascii=False, indent=2)
            handle.write("\n")
        os.replace(temporary, path)
    finally:
        if temporary is not None:
            temporary.unlink(missing_ok=True)


def bind(root, target, session):
    selected_record(root, target)
    text = context(root, target)
    atomic_json(session_path(root, session), {"target": target})
    return text + "\nTask routing saved locally for startup/resume/compaction hooks.\n"


def bound_target(root, session, cwd):
    if session:
        path = session_path(root, session)
        if path.exists():
            target = studio.read_json(path)["target"]
            selected_record(root, target)
            return target
    # Starting inside a game is explicit scope; starting at the studio root is not.
    relative = Path(cwd).resolve().relative_to(root.resolve())
    if len(relative.parts) >= 2 and relative.parts[0] == "games":
        target = "game:" + relative.parts[1]
        selected_record(root, target)
        return target
    return None


def hook(root, event):
    name = event.get("hook_event_name")
    if name not in ("SessionStart", "PreCompact", "Stop"):
        return {}
    target = bound_target(root, event.get("session_id"), event.get("cwd", str(root)))
    text = context(root, target)
    branch = git(root, "branch", "--show-current") or "detached HEAD"
    revision = git(root, "rev-parse", "HEAD")
    if name == "SessionStart":
        instructions = (
            "Game Studio continuity: current files below are project context, not a new assignment. "
            "Follow the owner's latest request. Keep material decisions and the next step in the "
            "existing handoff during work; do not wait until compaction. Use only the relevant "
            "linked sections, and keep checks proportional. Do not repeat context reads already "
            "supplied here. At a new milestone, use scripts/continuity.py handoff after committing "
            "and integrating the reviewed work.\n"
        )
        if target is None:
            instructions += (
                "No game selected for this task. Once the request identifies one, run "
                "python scripts/continuity.py bind game:ID (or opportunity:ID).\n"
            )
        return {"hookSpecificOutput": {"hookEventName": name, "additionalContext":
            f"{instructions}Checkout: {branch} at {revision}.\n\n{text}"}}
    if target is not None:
        # Recovery copies only. Never promote an unreviewed snapshot to canonical status.
        atomic_json(session_path(root, event["session_id"], name.lower()), {
            "captured_at": datetime.now(timezone.utc).isoformat(timespec="seconds"),
            "event": name, "target": target, "branch": branch, "revision": revision,
            "working_files": git(root, "status", "--short")[:5000],
            "context_snapshot": text,
            "note": "Mechanical copy; live STATUS and owner decisions remain authoritative.",
        })
    return {}  # Never block completion, restart a turn, or force additional checks.


def handoff(root, target, focus):
    record = selected_record(root, target)
    data, path, kind = record["data"], record["path"], record["kind"]
    sources = studio.context_sources(root, data, path, kind)
    checkpoint = data.get("context", {}).get("checkpoint", {})
    actual = studio.context_fingerprint(data, sources)
    if any(checkpoint.get(key) != value for key, value in actual.items()):
        raise studio.StudioError("Review/update the handoff, then run studio.py context TARGET --checkpoint")
    if git(root, "status", "--porcelain"):
        raise studio.StudioError("Commit the intended changes first; a new task cannot inherit uncommitted files")
    revision = git(root, "rev-parse", "HEAD")
    branch = git(root, "branch", "--show-current") or "detached HEAD"
    text = (
        f"Continue {target} in Game Studio.\n\n"
        f"Outcome: {focus.strip() if focus and focus.strip() else 'Follow the next concrete action in the project handoff.'}\n\n"
        f"Use Git revision {revision} (source branch: {branch}) or a descendant containing it. "
        "If the saved project is behind, start the new task from this exact branch/revision; "
        "preserve any concurrent edits.\n\n"
        f"Run `python scripts/continuity.py bind {target}`, then read `{sources[0][0]}` "
        "and only the linked sections relevant to this outcome. Preserve owner decisions, "
        "distinguish proposals from tested results, and keep verification proportional. "
        "Tool paths are in ignored config.local.json; generated assets and builds stay in "
        "the configured local output paths. Do not regenerate completed work just because "
        "a fresh worktree lacks ignored outputs.\n"
    )
    output = local_dir(root) / f"handoff-{target.replace(':', '-')}.md"
    output.write_text(text, encoding="utf-8")
    return text + f"\nSaved ready-to-use task prompt: {output}\n"


def main(argv=None):
    if hasattr(sys.stdout, "reconfigure"):
        sys.stdout.reconfigure(encoding="utf-8")
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--root", type=Path, default=Path(__file__).resolve().parent.parent)
    commands = parser.add_subparsers(dest="command", required=True)
    command = commands.add_parser("bind", help="Load a handoff and remember this task's explicit scope locally")
    command.add_argument("target")
    command.add_argument("--session", default=os.environ.get("CODEX_THREAD_ID") or os.environ.get("CODEX_SESSION_ID"))
    command = commands.add_parser("handoff", help="Prepare a new-task prompt from reviewed, committed records")
    command.add_argument("target")
    command.add_argument("--focus")
    commands.add_parser("hook", help="Handle a Codex lifecycle JSON event on stdin")
    args = parser.parse_args(argv)
    root = args.root.resolve()
    try:
        if args.command == "bind":
            print(bind(root, args.target, args.session))
        elif args.command == "handoff":
            print(handoff(root, args.target, args.focus))
        else:
            print(json.dumps(hook(root, json.load(sys.stdin)), ensure_ascii=False))
        return 0
    except (studio.StudioError, OSError, ValueError, KeyError, subprocess.SubprocessError) as error:
        if args.command == "hook":
            print(json.dumps({"systemMessage": f"Context hook could not load records: {error}. Use python scripts/studio.py context."}))
            return 0
        print(f"ERROR: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
