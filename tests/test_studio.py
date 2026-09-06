"""Behavior and safety tests. All Steam responses are mocked; no network needed."""
import contextlib
import copy
import importlib.util
import io
import json
from pathlib import Path
import shutil
import tempfile
import unittest
from unittest.mock import patch
from urllib.error import URLError

REPO = Path(__file__).resolve().parent.parent
SPEC = importlib.util.spec_from_file_location("studio", REPO / "scripts/studio.py")
studio = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(studio)


def write(path, content):
    path.parent.mkdir(parents=True, exist_ok=True)
    path.write_text(content, encoding="utf-8")


def write_json(path, data):
    write(path, json.dumps(data, indent=2))


def candidate():
    return {
        "schema_version": 1, "id": "small-idea", "title": "Small Idea",
        "status": "draft", "hook": "A testable hook", "audience": "Arcade players",
        "core_loop": "Aim, bounce, retry", "trend_stage": "unverified",
        "confidence": "low", "scores": {key: 3 for key in studio.WEIGHTS},
        "evidence": [{"url": "https://example.org/evidence", "accessed_on": "2026-09-06",
                      "claim": "Unverified hypothesis", "kind": "hypothesis"}],
        "risks": ["No playtest"], "next_test": "Observe a small prototype session",
        "decision_reason": "Draft only, no game selected",
    }


class StudioTest(unittest.TestCase):
    def setUp(self):
        self.temporary = tempfile.TemporaryDirectory()
        self.addCleanup(self.temporary.cleanup)
        self.root = Path(self.temporary.name)
        shutil.copytree(REPO / "templates", self.root / "templates")
        for file in ("AGENTS.md", "README.md", "studio/CHARTER.md", "studio/WORKFLOW.md",
                     "studio/TEAM.md", "studio/IMPLEMENTATION-CONTRACT.md", "research/README.md"):
            write(self.root / file, "# Fixture\n")
        for role in studio.ROLES:
            write(self.root / ".codex/agents" / (role + ".toml"),
                  f'name = "{role}"\ndescription = "Bounded role"\ndeveloper_instructions = "Read current status."\n')
        for skill in studio.SKILLS:
            write(self.root / ".agents/skills" / skill / "SKILL.md",
                  f"---\nname: {skill}\ndescription: A useful scoped instruction.\n---\n# Instructions\n")
        write_json(self.root / "research/sources.json", {
            "schema_version": 1,
            "sources": [{"id": "example", "name": "Example", "url": "https://example.org",
                         "purpose": "Fixture evidence", "access": "public"}],
        })
        self.network = patch.object(studio, "urlopen", side_effect=AssertionError("Unexpected network call"))
        self.network.start()
        self.addCleanup(self.network.stop)

    def run_cli(self, args):
        stdout, stderr = io.StringIO(), io.StringIO()
        with contextlib.redirect_stdout(stdout), contextlib.redirect_stderr(stderr):
            result = studio.main(args)
        return result, stdout.getvalue(), stderr.getvalue()

    def test_generated_game_is_valid_and_still_a_plan(self):
        target = studio.new_game(self.root, "small-game", 'Small "Game" ☀', "unreal")
        self.assertEqual(studio.validate(self.root), [])
        data = studio.read_json(target / "game.json")
        self.assertEqual(data["title"], 'Small "Game" ☀')
        self.assertEqual(data["stage"], "concept")
        self.assertIsNone(data["source_opportunity"])
        self.assertIn("No engine project has been created", (target / "BUILD.md").read_text())
        self.assertIn("## DESIGN", (target / "BRIEF.md").read_text())
        self.assertIn("## EXPERIENCE", (target / "BRIEF.md").read_text())
        self.assertIn("## Resume here", (target / "STATUS.md").read_text())
        self.assertNotIn("{{", (target / "README.md").read_text())
        result, output, _ = self.run_cli(["status", "--root", str(self.root)])
        self.assertEqual(result, 0)
        self.assertIn("small-game", output)
        self.assertIn("STATUS.md", output)

    def test_hostile_slugs_cannot_create_paths(self):
        for slug in ("../escape", "..", "x/y", r"x\y", "C:", "con", "nul", "lpt1",
                     "com9", "-x", "x-", "two--hyphens", "Upper", "x.", "a" * 64, ""):
            with self.subTest(slug=slug), self.assertRaises(studio.StudioError):
                studio.new_game(self.root, slug, "Unsafe", "web")
        self.assertFalse((self.root / "games").exists())

    def test_existing_project_is_preserved(self):
        target = studio.new_game(self.root, "existing", "Existing", "web")
        marker = target / "src/user.txt"
        marker.write_text("do not change")
        with self.assertRaises(studio.StudioError):
            studio.new_game(self.root, "existing", "Overwrite", "godot")
        self.assertEqual(marker.read_text(), "do not change")
        self.assertEqual(studio.read_json(target / "game.json")["title"], "Existing")

    def test_bad_template_leaves_no_partial_game(self):
        write(self.root / "templates/game/game.json", "{broken")
        with self.assertRaises(studio.StudioError):
            studio.new_game(self.root, "rollback", "Rollback", "web")
        self.assertEqual(list((self.root / "games").iterdir()), [])
        self.assertTrue(any("cannot read JSON" in x for x in studio.validate(self.root)))

    def test_publish_failure_cleans_staging(self):
        with patch.object(studio, "rename_no_replace", side_effect=OSError("disk error")):
            with self.assertRaises(OSError):
                studio.new_game(self.root, "rollback", "Rollback", "web")
        self.assertEqual(list((self.root / "games").iterdir()), [])

    def test_atomic_publish_does_not_replace_racing_empty_directory(self):
        source, target = self.root / "source", self.root / "target"
        source.mkdir()
        target.mkdir()
        write(source / "owned.txt", "ours")
        with self.assertRaises(OSError):
            studio.rename_no_replace(source, target)
        self.assertTrue((source / "owned.txt").exists())
        self.assertEqual(list(target.iterdir()), [])

    def test_symlink_escape_is_refused(self):
        outside = self.root / "outside"
        outside.mkdir()
        try:
            (self.root / "games").symlink_to(outside, target_is_directory=True)
        except (OSError, NotImplementedError):
            self.skipTest("Host does not permit creating symlinks")
        with self.assertRaises(studio.StudioError):
            studio.new_game(self.root, "escape", "Escape", "web")
        self.assertEqual(list(outside.iterdir()), [])

    def test_record_validation_handles_bad_types_and_values(self):
        data = candidate()
        data["scores"]["demand"] = True
        data["scores"]["distribution"] = float("nan")
        data["evidence"][0]["accessed_on"] = "2026-02-31"
        data["evidence"][0]["url"] = "missing-source"
        data["evidence"].append("not a record")
        issues = studio.check_record(data, "fixture.json", "candidate")
        self.assertTrue(any("scores.demand" in x for x in issues))
        self.assertTrue(any("scores.distribution" in x for x in issues))
        self.assertTrue(any("accessed_on" in x for x in issues))
        self.assertTrue(any(".url" in x for x in issues))
        self.assertTrue(any("evidence[1]" in x for x in issues))
        self.assertTrue(studio.check_record([], "bad.json", "game"))

    def test_game_references_and_license_columns_are_validated(self):
        target = studio.new_game(self.root, "linked", "Linked", "web")
        data = studio.read_json(target / "game.json")
        data["source_opportunity"] = "missing"
        data["budget"]["human_hours"] = -1
        write_json(target / "game.json", data)
        write(target / "assets/manifest.csv", "path,license\n")
        issues = studio.validate(self.root)
        self.assertTrue(any("source_opportunity" in x for x in issues))
        self.assertTrue(any("budget.human_hours" in x for x in issues))
        self.assertTrue(any("license tracking columns" in x for x in issues))
        data["source_opportunity"] = "small-idea"
        data["budget"]["human_hours"] = 2
        write_json(target / "game.json", data)
        write_json(self.root / "research/opportunities/small-idea.json", candidate())
        shutil.copyfile(REPO / "templates/game/assets/manifest.csv", target / "assets/manifest.csv")
        self.assertEqual(studio.validate(self.root), [])

    def test_native_toml_is_parsed_and_pinned_model_rejected(self):
        path = self.root / ".codex/agents/producer.toml"
        write(path, 'name = "producer"\nname = "duplicate"\n')
        self.assertTrue(any("invalid TOML" in x for x in studio.validate(self.root)))
        write(path, 'name = "producer"\ndescription = "Role"\ndeveloper_instructions = "Work"\nmodel = "pinned"\n')
        self.assertTrue(any("remove model" in x for x in studio.validate(self.root)))

    def test_skill_frontmatter_rejects_ambiguous_or_invalid_yaml(self):
        path = self.root / ".agents/skills/studio-director/SKILL.md"
        for scalar in ("false", "bad: yaml", "[array]", '"unterminated', "42", "null"):
            with self.subTest(scalar=scalar):
                write(path, f"---\nname: studio-director\ndescription: {scalar}\n---\nInstructions\n")
                self.assertTrue(studio.skill_frontmatter(path))
        for scalar in ('"A: quoted description"', "'A: single quoted description'", ">\n  A block\n  description."):
            with self.subTest(scalar=scalar):
                write(path, f"---\nname: studio-director\ndescription: {scalar}\n---\nInstructions\n")
                self.assertEqual(studio.skill_frontmatter(path), [])

    def test_relative_links_are_checked_but_generated_dirs_are_pruned(self):
        write(self.root / "README.md", "[missing](missing.md)\n[external](D:/Other/learning.md)\n[web](https://example.org)\n")
        issues = studio.validate(self.root)
        self.assertEqual(len(issues), 1, issues)
        self.assertIn("broken relative link", issues[0])
        write(self.root / "README.md", "# Good\n")
        for relative in (".local/downloads", "node_modules/vendor", "build/cache", "Library/docs", "Intermediate/docs"):
            write(self.root / relative / "README.md", "[broken](absent.md)")
        self.assertEqual(studio.validate(self.root), [])

    def test_malformed_catalog_and_duplicate_ids_are_actionable(self):
        write_json(self.root / "research/sources.json", {"schema_version": 1, "sources": [{"id": []}]})
        self.assertTrue(any("sources[0].id" in x for x in studio.validate(self.root)))
        write_json(self.root / "research/opportunities/a.json", candidate())
        write_json(self.root / "research/opportunities/b.json", candidate())
        self.assertTrue(any("duplicate candidate id" in x for x in studio.validate(self.root)))

    def test_score_is_weighted_and_does_not_change_candidate(self):
        data = candidate()
        data["scores"] = {"demand": 5, "distinctiveness": 4, "fun_potential": 3, "scope_fit": 2, "distribution": 1}
        path = self.root / "idea.json"
        write_json(path, data)
        before = path.read_bytes()
        code, output, _ = self.run_cli(["score", str(path)])
        self.assertEqual(code, 0)
        self.assertIn("3.05/5", output)
        self.assertIn("unverified", output)
        self.assertIn("not a sales forecast", output)
        self.assertEqual(path.read_bytes(), before)

    def test_invalid_status_and_validate_return_failure(self):
        write(self.root / "games/bad/game.json", "{invalid")
        code, _, error = self.run_cli(["status", "--root", str(self.root)])
        self.assertEqual(code, 1)
        self.assertIn("cannot read JSON", error)
        code, _, error = self.run_cli(["validate", "--root", str(self.root)])
        self.assertEqual(code, 1)
        self.assertIn("IMPLEMENTATION-CONTRACT", error)

    def test_doctor_missing_optional_tools_does_not_fail(self):
        def which(name):
            return "/usr/bin/git" if name == "git" else None
        with patch.object(studio.shutil, "which", side_effect=which):
            code, output, _ = self.run_cli(["doctor", "--root", str(self.root)])
        self.assertEqual(code, 0)
        self.assertIn("Optional godot: not detected", output)
        with patch.object(studio.shutil, "which", return_value=None):
            code, _, _ = self.run_cli(["doctor", "--root", str(self.root)])
        self.assertEqual(code, 1)

    def test_steam_snapshot_uses_usd_and_consistent_purchase_filters(self):
        calls = []
        def fetch(url):
            calls.append(url)
            if "appdetails" in url:
                return {"123": {"success": True, "data": {"name": "Known Game", "is_free": False,
                    "price_overview": {"currency": "USD", "initial": 999, "final": 499}}}}
            return {"success": 1, "query_summary": {"total_positive": 90, "total_negative": 10, "total_reviews": 100}}
        out = self.root / "snapshots/test.json"
        with patch.object(studio, "fetch_json", side_effect=fetch):
            data = studio.steam_snapshot([123, 123], out)
        self.assertEqual(len(calls), 2)
        self.assertIn("cc=us", calls[0])
        self.assertIn("language=all", calls[1])
        self.assertIn("purchase_type=steam", calls[1])
        self.assertEqual(data["apps"][0]["price_usd"]["list"], 9.99)
        self.assertEqual(data["apps"][0]["price_usd"]["current"], 4.99)
        self.assertEqual(data["apps"][0]["reviews"]["total_reviews"], 100)
        self.assertEqual(data["status"], "complete")
        self.assertTrue(data["captured_at"])
        self.assertEqual(studio.read_json(out), data)

    def test_missing_steam_fields_do_not_turn_into_zero(self):
        detail = {"123": {"success": True, "data": {"name": "Unreleased", "is_free": False}}}
        review = {"success": 1, "query_summary": {"total_positive": 0, "total_negative": 0, "total_reviews": 0}}
        with patch.object(studio, "fetch_json", side_effect=[detail, review]):
            with self.assertRaisesRegex(studio.StudioError, "USD price unavailable"):
                studio.snapshot_app(123)
        detail["123"]["data"]["is_free"] = True
        with patch.object(studio, "fetch_json", side_effect=[detail, review]):
            self.assertEqual(studio.snapshot_app(123)["price_usd"]["current"], 0)
        del review["query_summary"]["total_reviews"]
        with patch.object(studio, "fetch_json", side_effect=[detail, review]):
            with self.assertRaisesRegex(studio.StudioError, "missing review count"):
                studio.snapshot_app(123)

    def test_steam_later_failure_publishes_nothing_and_returns_nonzero(self):
        out = self.root / "failed.json"
        with patch.object(studio, "snapshot_app", side_effect=[{"app_id": 1}, studio.StudioError("HTTP 429")]):
            code, _, error = self.run_cli(["steam-snapshot", "--apps", "1", "2", "--out", str(out)])
        self.assertEqual(code, 1)
        self.assertIn("FAILED", error)
        self.assertIn("HTTP 429", error)
        self.assertFalse(out.exists())

    def test_steam_network_errors_are_explicit(self):
        with patch.object(studio, "urlopen", side_effect=URLError("offline")):
            with self.assertRaisesRegex(studio.StudioError, "offline"):
                studio.fetch_json("https://store.steampowered.com/example")

    def test_steam_write_failure_does_not_publish_partial_json(self):
        out = self.root / "snapshot.json"
        with patch.object(studio, "snapshot_app", return_value={"app_id": 1}), patch.object(studio.json, "dump", side_effect=OSError("disk full")):
            with self.assertRaises(OSError):
                studio.steam_snapshot([1], out)
        self.assertFalse(out.exists())
        self.assertFalse(list(self.root.glob(".steam-snapshot-*")))

    def test_steam_existing_snapshot_is_untouched(self):
        out = self.root / "snapshot.json"
        write(out, "original evidence")
        with self.assertRaises(studio.StudioError):
            studio.steam_snapshot([1], out)
        self.assertEqual(out.read_text(), "original evidence")


if __name__ == "__main__":
    unittest.main()
