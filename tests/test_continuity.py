"""Focused checks for local context restoration and portable task handoffs."""
import contextlib
import io
import json
import os
from pathlib import Path
import shutil
import subprocess
import sys
import tempfile
import unittest

REPO = Path(__file__).resolve().parent.parent
sys.path.insert(0, str(REPO / "scripts"))
import continuity
import studio


class ContinuityTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        shutil.copytree(REPO / "templates", self.root / "templates")
        shutil.copytree(REPO / "scripts", self.root / "scripts", ignore=shutil.ignore_patterns("__pycache__"))
        (self.root / ".gitignore").write_text(".local/\n__pycache__/\n", encoding="utf-8")
        studio.new_game(self.root, "example", "Example", "unreal")
        self.target = "game:example"
        self.status = self.root / "games/example/STATUS.md"
        self.record = self.root / "games/example/game.json"
        continuity.git(self.root, "init", "-b", "main")
        self.checkpoint()
        self.commit()

    def checkpoint(self):
        with contextlib.redirect_stdout(io.StringIO()):
            studio.project_context(self.root, self.target, checkpoint=True)

    def commit(self):
        continuity.git(self.root, "add", ".")
        continuity.git(self.root, "-c", "user.name=Test", "-c", "user.email=test@example.invalid",
                       "commit", "-m", "fixture")

    def event(self, name, session="task-one"):
        return {"hook_event_name": name, "session_id": session, "cwd": str(self.root), "source": "compact"}

    def test_compaction_reads_current_handoff_and_keeps_other_tasks_unbound(self):
        continuity.bind(self.root, self.target, "task-one")
        self.status.write_text("# Current handoff\nOwner decision: keep the shield.\n", encoding="utf-8")
        result = continuity.hook(self.root, self.event("SessionStart"))
        text = result["hookSpecificOutput"]["additionalContext"]
        self.assertIn("Owner decision: keep the shield", text)
        self.assertIn("STALE", text)
        other = continuity.hook(self.root, self.event("SessionStart", "another-task"))
        self.assertIn("No game selected", other["hookSpecificOutput"]["additionalContext"])
        self.assertNotIn("Owner decision: keep the shield", other["hookSpecificOutput"]["additionalContext"])

    def test_recovery_copies_do_not_change_records_or_continue_turns(self):
        continuity.bind(self.root, self.target, "task-one")
        before = self.record.read_bytes()
        for event in ("PreCompact", "Stop", "Stop"):
            self.assertEqual(continuity.hook(self.root, self.event(event)), {})
        copies = list((self.root / ".local/continuity").glob("*.json"))
        self.assertEqual(len(copies), 3)  # routing + one copy per event
        self.assertEqual(self.record.read_bytes(), before)
        self.assertEqual(continuity.git(self.root, "status", "--porcelain"), "")

    def test_handoff_requires_reviewed_committed_state_and_pins_revision(self):
        result = continuity.handoff(self.root, self.target, "Make one rewarding enemy")
        self.assertIn(continuity.git(self.root, "rev-parse", "HEAD"), result)
        self.assertIn("Make one rewarding enemy", result)
        source = self.root / "new-source.txt"
        source.write_text("unfinished", encoding="utf-8")
        with self.assertRaisesRegex(studio.StudioError, "Commit"):
            continuity.handoff(self.root, self.target, None)
        self.status.write_text("# Changed handoff\n", encoding="utf-8")
        self.commit()
        with self.assertRaisesRegex(studio.StudioError, "checkpoint"):
            continuity.handoff(self.root, self.target, None)

    def test_configured_hook_runs_from_game_subdirectory(self):
        continuity.bind(self.root, self.target, "task-one")
        config = json.loads((REPO / ".codex/hooks.json").read_text(encoding="utf-8"))
        handler = config["hooks"]["SessionStart"][0]["hooks"][0]
        command = handler["commandWindows" if os.name == "nt" else "command"]
        event = self.event("SessionStart")
        event["cwd"] = str(self.root / "games/example")
        result = subprocess.run(command, shell=True, cwd=event["cwd"], input=json.dumps(event),
                                capture_output=True, text=True, encoding="utf-8", timeout=10)
        self.assertEqual(result.returncode, 0, result.stderr)
        output = json.loads(result.stdout)
        self.assertEqual(output["hookSpecificOutput"]["hookEventName"], "SessionStart")
        self.assertIn(self.target, output["hookSpecificOutput"]["additionalContext"])


if __name__ == "__main__":
    unittest.main()
