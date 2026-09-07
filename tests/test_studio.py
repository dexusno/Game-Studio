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

    def context_candidate(self, identifier="small-idea", with_context=True):
        data = candidate()
        data["id"] = identifier
        path = self.root / "research/opportunities" / f"{identifier}.json"
        if with_context:
            data["context"] = {
                "entrypoint": f"research/runs/{identifier}/STATUS.md",
                "read_first": [f"research/runs/{identifier}/design.md"],
            }
            write(self.root / data["context"]["entrypoint"], "# Current handoff\n\nNext: Try the blocking mechanic.\n")
            write(self.root / data["context"]["read_first"][0], "Supporting detail is deliberately not concatenated.\n")
        write_json(path, data)
        return path, data

    def context_cli(self, *args):
        return self.run_cli(["context", *args, "--root", str(self.root)])

    def test_context_discovers_research_without_games_or_automatic_selection(self):
        path, data = self.context_candidate()
        before = path.read_bytes()
        code, output, error = self.context_cli()
        self.assertEqual((code, error), (0, ""))
        self.assertIn("opportunity:small-idea", output)
        self.assertIn("draft", output)
        self.assertIn("nothing selected or resumed", output)
        self.assertIn("does not verify handoff freshness", output)
        self.assertEqual(path.read_bytes(), before)
        self.assertFalse((self.root / "games").exists())

    def test_context_empty_index_is_explicit(self):
        code, output, _ = self.context_cli()
        self.assertEqual(code, 0)
        self.assertIn("No game or opportunity records", output)

    def test_context_parked_game_stays_parked_and_uses_existing_status(self):
        folder = studio.new_game(self.root, "parked-game", "Parked Game", "web")
        data = studio.read_json(folder / "game.json")
        data["stage"] = "parked"
        write_json(folder / "game.json", data)
        write(folder / "STATUS.md", "# Parked\nNext: Preserve this build and research another concept.\n")
        before = (folder / "game.json").read_bytes()
        code, output, error = self.context_cli("game:parked-game")
        self.assertEqual((code, error), (0, ""))
        self.assertIn("Recorded stage: parked", output)
        self.assertIn("Next: Preserve this build", output)
        self.assertIn("games/parked-game/STATUS.md", output)
        self.assertEqual((folder / "game.json").read_bytes(), before)

    def test_context_shared_ids_require_qualifier(self):
        studio.new_game(self.root, "small-idea", "Small Game", "web")
        self.context_candidate()
        code, _, error = self.context_cli("small-idea")
        self.assertEqual(code, 1)
        self.assertIn("Ambiguous", error)
        self.assertIn("game:small-idea", error)
        self.assertIn("opportunity:small-idea", error)
        for target, expected in (("game:small-idea", "Recorded stage: concept"),
                                 ("opportunity:small-idea", "Recorded status: draft")):
            with self.subTest(target=target):
                code, output, error = self.context_cli(target)
                self.assertEqual((code, error), (0, ""))
                self.assertIn(expected, output)

    def test_context_missing_and_malformed_targets_fail_without_fuzzy_matching(self):
        self.context_candidate()
        for target in ("small", "absent", "game:small-idea"):
            with self.subTest(target=target):
                code, _, error = self.context_cli(target)
                self.assertEqual(code, 1)
                self.assertIn("Unknown context target", error)
        for target in ("", "../small-idea", "opportunity:", "candidate:small-idea", "game:small-idea:extra", "Small-Idea"):
            with self.subTest(target=target):
                code, _, error = self.context_cli(target)
                self.assertEqual(code, 1)
                self.assertIn("Invalid context target", error)

    def test_context_malformed_records_and_duplicate_same_kind_ids_are_errors(self):
        path, data = self.context_candidate()
        write(path, "{invalid")
        code, _, error = self.context_cli()
        self.assertEqual(code, 1)
        self.assertIn("cannot read JSON", error)
        write_json(path, data)
        write_json(self.root / "research/opportunities/duplicate.json", data)
        code, _, error = self.context_cli("opportunity:small-idea")
        self.assertEqual(code, 1)
        self.assertIn("Duplicate context target", error)

    def test_qualified_context_remains_readable_with_unrelated_invalid_records(self):
        path, _ = self.context_candidate()
        before = path.read_bytes()
        write(self.root / "research/opportunities/other-idea.json", "<<<<<<< unresolved merge")
        write(self.root / "games/other-game/game.json", "{invalid")
        code, output, error = self.context_cli("opportunity:small-idea")
        self.assertEqual(code, 0)
        self.assertIn("opportunity:small-idea", output)
        self.assertIn("WARNING: skipped 2 unrelated invalid record(s)", error)
        self.assertIn("other-idea.json", error)
        self.assertEqual(path.read_bytes(), before)
        self.assertEqual(self.context_cli()[0], 1)

    def test_context_invalid_competing_record_cannot_hide_bare_id_ambiguity(self):
        self.context_candidate()
        write(self.root / "games/small-idea/game.json", "{invalid")
        code, _, error = self.context_cli("small-idea")
        self.assertEqual(code, 1)
        self.assertIn("Cannot confidently resolve a bare id", error)
        self.assertIn("opportunity:small-idea", error)
        self.assertEqual(self.context_cli("opportunity:small-idea")[0], 0)
        code, _, error = self.context_cli("game:small-idea")
        self.assertEqual(code, 1)
        self.assertIn("cannot read JSON", error)

    def test_qualified_context_rejects_invalid_target_and_known_duplicate(self):
        path, data = self.context_candidate()
        write(path, "{invalid")
        code, _, error = self.context_cli("opportunity:small-idea")
        self.assertEqual(code, 1)
        self.assertIn("cannot read JSON", error)
        write_json(path, data)
        duplicate = copy.deepcopy(data)
        duplicate.pop("title")
        write_json(self.root / "research/opportunities/differently-named.json", duplicate)
        code, _, error = self.context_cli("opportunity:small-idea")
        self.assertEqual(code, 1)
        self.assertIn("title", error)
        self.assertNotIn("WARNING: skipped", error)

    def test_context_rejects_malformed_context_metadata(self):
        path, original = self.context_candidate()
        for value in (None, [], {}, {"entrypoint": 42}, {"entrypoint": "README.md", "read_first": "README.md"},
                      {"entrypoint": "README.md", "read_first": ["README.md"] * (studio.CONTEXT_MAX_FILES + 1)},
                      {"entrypoint": "README.md", "checkpoint": {}},
                      {"entrypoint": "README.md", "checkpoint": None}):
            with self.subTest(context=value):
                data = copy.deepcopy(original)
                data["context"] = value
                write_json(path, data)
                code, _, error = self.context_cli("small-idea")
                self.assertEqual(code, 1)
                self.assertIn("context", error)

    def test_context_paths_refuse_traversal_absolute_and_nonfiles(self):
        path, original = self.context_candidate()
        unsafe = ("../README.md", "research/../../README.md", "/README.md", "C:/secret.txt",
                  "C:secret.txt", r"\\server\share\secret.txt", r"research\README.md", "./README.md",
                  "README.md:secret", "research//README.md", "research", "")
        for relative in unsafe:
            with self.subTest(relative=relative):
                data = copy.deepcopy(original)
                data["context"]["entrypoint"] = relative
                write_json(path, data)
                code, _, error = self.context_cli("small-idea")
                self.assertEqual(code, 1)
                self.assertTrue("context" in error.lower(), error)
        data = copy.deepcopy(original)
        data["context"]["read_first"] = ["../README.md"]
        write_json(path, data)
        self.assertEqual(self.context_cli("small-idea")[0], 1)

    def test_context_missing_configured_handoff_is_error_without_fallback(self):
        path, data = self.context_candidate()
        data["research_run"] = "README.md"
        data["context"]["entrypoint"] = "research/missing.md"
        write_json(path, data)
        code, _, error = self.context_cli("small-idea")
        self.assertEqual(code, 1)
        self.assertIn("Missing or invalid context file", error)
        self.assertTrue(any("Missing or invalid context file" in issue for issue in studio.validate(self.root)))

    def test_context_symlink_escape_refused_for_handoffs_and_records(self):
        outside = tempfile.TemporaryDirectory()
        self.addCleanup(outside.cleanup)
        outside_root = Path(outside.name)
        write(outside_root / "secret.md", "Outside content must not be read")
        path, data = self.context_candidate()
        link = self.root / "escaped.md"
        try:
            link.symlink_to(outside_root / "secret.md")
        except (OSError, NotImplementedError):
            self.skipTest("Host does not permit creating symlinks")
        data["context"]["entrypoint"] = "escaped.md"
        write_json(path, data)
        code, output, error = self.context_cli("small-idea")
        self.assertEqual(code, 1)
        self.assertIn("confined", error)
        self.assertNotIn("Outside content", output)
        path.unlink()
        write_json(outside_root / "record.json", candidate())
        path.symlink_to(outside_root / "record.json")
        code, _, error = self.context_cli()
        self.assertEqual(code, 1)
        self.assertIn("confined", error)

    def test_context_legacy_opportunity_fallback_order_and_no_handoff(self):
        path, data = self.context_candidate(with_context=False)
        code, output, _ = self.context_cli("small-idea")
        self.assertEqual(code, 0)
        self.assertIn("No handoff configured", output)
        self.assertIn("Freshness: unverified", output)
        data["research_run"] = "README.md"
        write_json(path, data)
        self.assertIn("1. README.md", self.context_cli("small-idea")[1])
        data["owner_selected_foundations"] = {"current_design": "studio/CHARTER.md"}
        write_json(path, data)
        self.assertIn("1. studio/CHARTER.md", self.context_cli("small-idea")[1])
        data["owner_selected_foundations"]["current_design"] = "missing-design.md"
        write_json(path, data)
        self.assertEqual(self.context_cli("small-idea")[0], 1)

    def test_context_resolved_aliases_cannot_escape_or_read_private_config(self):
        # Exercise confinement on hosts where native symlink creation is unavailable.
        link = self.root / "alias.md"
        write(link, "Nominal public path")
        write(self.root / "config.local.json", "Private configuration")
        original_resolve = Path.resolve
        for destination, message in ((self.root.parent / "outside.md", "confined"),
                                     (self.root / "config.local.json", "Private configuration")):
            with self.subTest(destination=destination):
                def resolved_alias(path, *args, **kwargs):
                    return destination if path == link else original_resolve(path, *args, **kwargs)
                with patch.object(Path, "resolve", resolved_alias):
                    with self.assertRaisesRegex(studio.StudioError, message):
                        studio.context_path(self.root, "alias.md")

    def test_context_private_paths_are_case_insensitive(self):
        path, data = self.context_candidate()
        for relative in ("CONFIG.LOCAL.JSON", ".LOCAL/private.txt", ".GIT/config", ".ENV.LOCAL"):
            with self.subTest(relative=relative):
                write(self.root / relative, "Private source")
                data["context"]["entrypoint"] = relative
                write_json(path, data)
                code, output, error = self.context_cli("small-idea")
                self.assertEqual(code, 1)
                self.assertIn("Private configuration", error)
                self.assertNotIn("Private source", output)

    def test_cli_configures_real_text_streams_as_utf8(self):
        raw_output, raw_error = io.BytesIO(), io.BytesIO()
        output = io.TextIOWrapper(raw_output, encoding="ascii")
        error = io.TextIOWrapper(raw_error, encoding="ascii")
        self.context_candidate()
        path = self.root / "research/opportunities/small-idea.json"
        data = studio.read_json(path)
        data["title"] = "Cyborg — owner’s world"
        write_json(path, data)
        with contextlib.redirect_stdout(output), contextlib.redirect_stderr(error):
            code = studio.main(["context", "--root", str(self.root)])
            output.flush()
        self.assertEqual(code, 0)
        self.assertIn("Cyborg — owner’s world", raw_output.getvalue().decode("utf-8"))
        self.assertEqual(error.encoding, "utf-8")

    def test_context_read_only_and_private_configuration_not_read(self):
        path, _ = self.context_candidate()
        private = self.root / "config.local.json"
        write(private, "Private invalid JSON that must never be parsed by context")
        before = {file.relative_to(self.root): file.read_bytes() for file in self.root.rglob("*") if file.is_file()}
        code, output, error = self.context_cli("small-idea")
        after = {file.relative_to(self.root): file.read_bytes() for file in self.root.rglob("*") if file.is_file()}
        self.assertEqual((code, error), (0, ""))
        self.assertEqual(before, after)
        self.assertNotIn("Private invalid JSON", output)
        self.assertNotIn("Supporting detail is deliberately", output)
        data = studio.read_json(path)
        for relative in ("config.local.json", ".local/private.txt", ".env"):
            write(self.root / relative, "Private source must not be displayed")
            data["context"]["entrypoint"] = relative
            write_json(path, data)
            code, output, error = self.context_cli("small-idea")
            self.assertEqual(code, 1)
            self.assertIn("Private configuration", error)
            self.assertNotIn("Private source", output)

    def test_context_checkpoint_only_changes_chosen_record_and_preserves_status(self):
        path, original = self.context_candidate()
        self.context_candidate("other-idea")
        original["status"] = "held"
        original["custom_owner_field"] = {"keep": [1, 2, 3]}
        write_json(path, original)
        before = {file.relative_to(self.root): file.read_bytes() for file in self.root.rglob("*") if file.is_file()}
        code, output, error = self.context_cli("opportunity:small-idea", "--checkpoint")
        self.assertEqual((code, error), (0, ""))
        self.assertIn("Checkpoint saved in the same record", output)
        self.assertIn("content unchanged since checkpoint", output)
        updated = studio.read_json(path)
        checkpoint = updated["context"].pop("checkpoint")
        self.assertEqual(updated, original)
        self.assertEqual(checkpoint["schema_version"], 1)
        self.assertEqual(set(checkpoint["files"]), {original["context"]["entrypoint"], *original["context"]["read_first"]})
        self.assertEqual(studio.validate(self.root), [])
        after = {file.relative_to(self.root): file.read_bytes() for file in self.root.rglob("*") if file.is_file()}
        changed = [relative for relative in before.keys() | after.keys() if before.get(relative) != after.get(relative)]
        self.assertEqual(changed, [path.relative_to(self.root)])

    def test_context_fresh_checkpoint_read_is_unchanged_and_read_only(self):
        path, _ = self.context_candidate()
        self.assertEqual(self.context_cli("small-idea", "--checkpoint")[0], 0)
        before = path.read_bytes()
        code, output, error = self.context_cli("small-idea")
        self.assertEqual((code, error), (0, ""))
        self.assertIn("content unchanged since checkpoint", output)
        self.assertNotIn("Freshness: STALE", output)
        self.assertEqual(path.read_bytes(), before)

    def test_context_same_day_source_change_is_stale_without_timestamp_heuristics(self):
        path, data = self.context_candidate()
        self.assertEqual(self.context_cli("small-idea", "--checkpoint")[0], 0)
        source = self.root / data["context"]["read_first"][0]
        before_record = path.read_bytes()
        stamp = source.stat()
        write(source, "Different source content on the same day.\n")
        studio.os.utime(source, ns=(stamp.st_atime_ns, stamp.st_mtime_ns))
        code, output, error = self.context_cli("small-idea")
        self.assertEqual((code, error), (0, ""))
        self.assertIn("Freshness: STALE", output)
        self.assertIn(data["context"]["read_first"][0], output)
        self.assertEqual(path.read_bytes(), before_record)

    def test_context_record_metadata_and_routing_changes_are_stale(self):
        path, original = self.context_candidate()
        for field in ("record", "routing"):
            with self.subTest(field=field):
                write_json(path, original)
                self.assertEqual(self.context_cli("small-idea", "--checkpoint")[0], 0)
                data = studio.read_json(path)
                if field == "record":
                    data["next_test"] = "A new next action without changing a date"
                else:
                    data["context"]["read_first"] = ["README.md"]
                write_json(path, data)
                code, output, _ = self.context_cli("small-idea")
                self.assertEqual(code, 0)
                self.assertIn("Freshness: STALE", output)
                self.assertIn("record metadata", output)

    def test_context_checkpoint_normalizes_newlines_bom_and_json_formatting(self):
        path, data = self.context_candidate()
        sources = [self.root / data["context"]["entrypoint"], self.root / data["context"]["read_first"][0]]
        for source in sources:
            source.write_bytes(b"\xef\xbb\xbf# Source\r\n\r\nTwo lines.\r\n")
        self.assertEqual(self.context_cli("small-idea", "--checkpoint")[0], 0)
        for source in sources:
            source.write_bytes(b"# Source\n\nTwo lines.\n")
        data = studio.read_json(path)
        path.write_bytes(json.dumps(data, sort_keys=True, separators=(",", ":")).encode("utf-8"))
        code, output, error = self.context_cli("small-idea")
        self.assertEqual((code, error), (0, ""))
        self.assertIn("content unchanged since checkpoint", output)

    def test_context_checkpoint_requires_target_and_real_handoff(self):
        path, _ = self.context_candidate(with_context=False)
        before = path.read_bytes()
        code, _, error = self.context_cli("--checkpoint")
        self.assertEqual(code, 1)
        self.assertIn("requires an explicit target", error)
        code, _, error = self.context_cli("small-idea", "--checkpoint")
        self.assertEqual(code, 1)
        self.assertIn("No handoff", error)
        self.assertEqual(path.read_bytes(), before)

    def test_context_checkpoint_cannot_hash_its_own_record_recursively(self):
        path, data = self.context_candidate()
        data["context"]["read_first"].append(path.relative_to(self.root).as_posix())
        write_json(path, data)
        before = path.read_bytes()
        code, _, error = self.context_cli("small-idea", "--checkpoint")
        self.assertEqual(code, 1)
        self.assertIn("already hashed separately", error)
        self.assertEqual(path.read_bytes(), before)

    def test_context_checkpoint_can_adopt_legacy_handoff_without_changing_other_fields(self):
        path, original = self.context_candidate(with_context=False)
        original["research_run"] = "README.md"
        write_json(path, original)
        code, output, error = self.context_cli("small-idea", "--checkpoint")
        self.assertEqual((code, error), (0, ""))
        updated = studio.read_json(path)
        self.assertEqual(updated["context"]["entrypoint"], "README.md")
        updated.pop("context")
        self.assertEqual(updated, original)
        self.assertIn("content unchanged since checkpoint", output)

    def test_context_checkpoint_failed_write_and_observed_concurrent_change_preserve_record(self):
        path, _ = self.context_candidate()
        before = path.read_bytes()
        with patch.object(studio.os, "replace", side_effect=OSError("disk error")):
            code, _, error = self.context_cli("small-idea", "--checkpoint")
        self.assertEqual(code, 1)
        self.assertIn("disk error", error)
        self.assertEqual(path.read_bytes(), before)
        self.assertFalse(list(path.parent.glob(".context-*")))
        with self.assertRaisesRegex(studio.StudioError, "Record changed while checkpointing"):
            studio.save_context_checkpoint(path, studio.read_json(path), b"older content")
        self.assertEqual(path.read_bytes(), before)
        self.assertFalse(list(path.parent.glob(".context-*")))

    def test_context_output_is_bounded_and_supporting_files_are_not_dumped(self):
        path, data = self.context_candidate()
        write(self.root / data["context"]["entrypoint"], "Long handoff " * 10000)
        write(self.root / data["context"]["read_first"][0], "SECRET-TO-EXCERPT " * 10000)
        data["owner_selected_foundations"] = {"equipment": "Many abilities " * 10000}
        data["next_test"] = "Long next action " * 10000
        write_json(path, data)
        code, output, error = self.context_cli("small-idea")
        self.assertEqual((code, error), (0, ""))
        self.assertLess(len(output), studio.CONTEXT_OUTPUT_CHARS + 150)
        self.assertIn("Handoff excerpt shortened", output)
        self.assertNotIn("SECRET-TO-EXCERPT", output)
        for number in range(studio.CONTEXT_MAX_INDEX + 5):
            self.context_candidate(f"idea-{number}", with_context=False)
        code, output, _ = self.context_cli()
        self.assertEqual(code, 0)
        self.assertIn("additional records", output)
        self.assertLess(len(output), studio.CONTEXT_OUTPUT_CHARS + 150)

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
