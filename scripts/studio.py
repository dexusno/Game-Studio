#!/usr/bin/env python3
"""Small, dependency-free studio tools. Requires Python 3.11 or newer."""
from __future__ import annotations

import argparse
import csv
import ctypes
import hashlib
import json
import math
import os
from pathlib import Path, PureWindowsPath
import re
import shutil
import sys
import tempfile
import tomllib
from datetime import date, datetime, timezone
from urllib.error import HTTPError, URLError
from urllib.parse import unquote, urlparse
from urllib.request import Request, urlopen

ROOT = Path(__file__).resolve().parent.parent
ENGINES = ("unreal", "godot", "unity", "web", "custom", "undecided")
STAGES = ("concept", "prototype", "production", "release-candidate", "released", "maintenance", "parked")
CANDIDATE_STATES = ("draft", "shortlisted", "selected", "held", "rejected", "shipped")
WEIGHTS = {"demand": .20, "distinctiveness": .20, "fun_potential": .20, "scope_fit": .25, "distribution": .15}
SKILLS = ("studio-director", "game-research", "game-prototype", "game-production", "game-release", "game-retrospective")
ROLES = ("producer", "trend-scout", "market-analyst", "game-designer", "game-engineer", "art-director", "audio-designer", "qa-playtester", "release-producer")
GAME_FILES = ("game.json", "AGENTS.md", "README.md", "BRIEF.md", "STATUS.md", "DECISIONS.md", "BUILD.md", "QA.md", "RELEASE.md", "assets/manifest.csv")
GAME_DIRS = ("src", "marketing")
ASSET_COLUMNS = ("asset_id", "path", "source_url", "creator", "license", "proof_path", "modifications", "approval_status")
RESERVED = {"con", "prn", "aux", "nul", *(f"com{i}" for i in range(1, 10)), *(f"lpt{i}" for i in range(1, 10))}
CONTEXT_MAX_FILES = 24
CONTEXT_MAX_INDEX = 40
CONTEXT_OUTPUT_CHARS = 12000
CONTEXT_EXCERPT_CHARS = 8000


class StudioError(Exception):
    """An actionable, expected user-facing failure."""


def slug_ok(value):
    return isinstance(value, str) and bool(re.fullmatch(r"[a-z0-9]+(?:-[a-z0-9]+)*", value)) and len(value) <= 63 and value not in RESERVED


def read_json(path):
    try:
        return json.loads(path.read_text(encoding="utf-8-sig"))
    except (OSError, UnicodeError, json.JSONDecodeError) as exc:
        raise StudioError(f"{path}: cannot read JSON: {exc}") from exc


def nonempty(value):
    return isinstance(value, str) and bool(value.strip())


def number(value, low=0, high=None):
    return isinstance(value, (int, float)) and not isinstance(value, bool) and math.isfinite(value) and value >= low and (high is None or value <= high)


def iso_date(value):
    try:
        return isinstance(value, str) and date.fromisoformat(value).isoformat() == value
    except ValueError:
        return False


def web_url(value):
    if not isinstance(value, str):
        return False
    parsed = urlparse(value)
    return parsed.scheme in ("http", "https") and bool(parsed.netloc)


def within(path, base):
    return path.resolve().is_relative_to(base.resolve())


def check_context_shape(data, path):
    """Optional context metadata lives in the existing game/opportunity record."""
    if "context" not in data:
        return []
    context = data["context"]
    if not isinstance(context, dict):
        return [f"{path}: context: provide an object with entrypoint and optional read_first/checkpoint"]
    errors = []
    if not nonempty(context.get("entrypoint")):
        errors.append(f"{path}: context.entrypoint: provide a repository-relative handoff file")
    read_first = context.get("read_first", [])
    if (not isinstance(read_first, list) or len(read_first) > CONTEXT_MAX_FILES
            or not all(nonempty(item) for item in read_first)):
        errors.append(f"{path}: context.read_first: provide up to {CONTEXT_MAX_FILES} repository-relative file paths")
    checkpoint = context.get("checkpoint")
    if "checkpoint" in context:
        if not isinstance(checkpoint, dict):
            errors.append(f"{path}: context.checkpoint: expected an object written by context --checkpoint")
        else:
            if type(checkpoint.get("schema_version")) is not int or checkpoint["schema_version"] != 1:
                errors.append(f"{path}: context.checkpoint.schema_version: expected integer 1")
            captured_at = checkpoint.get("captured_at")
            try:
                stamp = datetime.fromisoformat(captured_at) if isinstance(captured_at, str) else None
                if stamp is None or stamp.tzinfo is None:
                    raise ValueError("missing timezone")
            except ValueError:
                errors.append(f"{path}: context.checkpoint.captured_at: expected an ISO timestamp with timezone")
            if not isinstance(checkpoint.get("record_sha256"), str) or not re.fullmatch(r"[0-9a-f]{64}", checkpoint["record_sha256"]):
                errors.append(f"{path}: context.checkpoint.record_sha256: expected a SHA256 digest")
            files = checkpoint.get("files")
            if (not isinstance(files, dict) or not files or len(files) > CONTEXT_MAX_FILES + 1
                    or not all(nonempty(key) and isinstance(value, str) and re.fullmatch(r"[0-9a-f]{64}", value)
                               for key, value in files.items())):
                errors.append(f"{path}: context.checkpoint.files: expected repository-relative paths mapped to SHA256 digests")
    return errors


def check_record(data, path, kind):
    """Return all schema errors for one candidate or game."""
    errors = []
    def check(condition, field, message):
        if not condition:
            errors.append(f"{path}: {field}: {message}")
    if not isinstance(data, dict):
        return [f"{path}: expected a JSON object; see studio/IMPLEMENTATION-CONTRACT.md"]
    check(type(data.get("schema_version")) is int and data["schema_version"] == 1, "schema_version", "set to integer 1")
    if kind == "game":
        check(slug_ok(data.get("slug")), "slug", "use lowercase letters/digits separated by hyphens (max 63); avoid Windows reserved names")
        check(nonempty(data.get("title")), "title", "provide a non-empty string")
        check(data.get("engine") in ENGINES, "engine", f"choose one of {', '.join(ENGINES)}")
        check(data.get("stage") in STAGES, "stage", f"choose one of {', '.join(STAGES)}")
        platforms = data.get("platforms")
        check(isinstance(platforms, list) and bool(platforms) and all(nonempty(x) for x in platforms), "platforms", "provide a non-empty array of platform names")
        check(iso_date(data.get("created_on")), "created_on", "use a valid YYYY-MM-DD date")
        check("source_opportunity" in data and (data["source_opportunity"] is None or nonempty(data["source_opportunity"])), "source_opportunity", "provide a candidate id or null")
        budget = data.get("budget")
        check(isinstance(budget, dict), "budget", "provide target_days, human_hours and cash_usd")
        if isinstance(budget, dict):
            for key in ("target_days", "human_hours", "cash_usd"):
                check(number(budget.get(key)), f"budget.{key}", "provide a finite nonnegative number")
    else:
        for key in ("id", "title", "hook", "audience", "core_loop", "next_test", "decision_reason"):
            check(nonempty(data.get(key)), key, "provide a non-empty string; explicit unknowns are acceptable")
        check(data.get("status") in CANDIDATE_STATES, "status", f"choose one of {', '.join(CANDIDATE_STATES)}")
        check(data.get("trend_stage") in ("rising", "near-peak", "steady", "declining", "unverified"), "trend_stage", "choose rising, near-peak, steady, declining or unverified")
        check(data.get("confidence") in ("low", "medium", "high"), "confidence", "choose low, medium or high")
        scores = data.get("scores")
        check(isinstance(scores, dict), "scores", "provide all five rubric scores")
        if isinstance(scores, dict):
            for key in WEIGHTS:
                check(number(scores.get(key), 0, 5), f"scores.{key}", "provide a finite number between 0 and 5; booleans are not scores")
        risks = data.get("risks")
        check(isinstance(risks, list) and all(nonempty(x) for x in risks), "risks", "provide an array of non-empty strings")
        evidence = data.get("evidence")
        check(isinstance(evidence, list), "evidence", "provide an array (empty is allowed for an unverified draft)")
        if isinstance(evidence, list):
            for i, item in enumerate(evidence):
                prefix = f"evidence[{i}]"
                if not isinstance(item, dict):
                    check(False, prefix, "provide an object with url, accessed_on, claim and kind")
                    continue
                check(web_url(item.get("url")), prefix + ".url", "provide an absolute HTTP(S) source URL")
                check(iso_date(item.get("accessed_on")), prefix + ".accessed_on", "use a valid YYYY-MM-DD date")
                check(nonempty(item.get("claim")), prefix + ".claim", "state what this source supports")
                check(item.get("kind") in ("observed", "reported", "estimated", "hypothesis"), prefix + ".kind", "choose observed, reported, estimated or hypothesis")
    errors.extend(check_context_shape(data, path))
    return errors


def skill_frontmatter(path):
    """Check top-level YAML string fields and |/> text blocks, not general YAML.

    Supported fields use plain text, JSON-style double quotes, YAML single quotes
    or block text. Nested metadata and complex YAML require a full validator.
    """
    text = path.read_text(encoding="utf-8-sig")
    lines = text.splitlines()
    if not lines or lines[0].strip() != "---":
        return [f"{path}: start with YAML frontmatter delimited by ---"]
    end = next((i for i in range(1, len(lines)) if lines[i].strip() == "---"), None)
    if end is None:
        return [f"{path}: close YAML frontmatter with ---"]
    fields, errors, block_key = {}, [], None
    for line in lines[1:end]:
        if not line.strip() or line.lstrip().startswith("#"):
            continue
        if line[0].isspace():
            if block_key and not line.startswith("\t"):
                fields[block_key] += line.strip() + " "
            else:
                errors.append(f"{path}: unsupported YAML indentation; use top-level strings or | / > text")
            continue
        block_key = None
        match = re.fullmatch(r"([A-Za-z_][\w-]*):(?:\s+(.*))?", line)
        if not match:
            errors.append(f"{path}: unsupported/malformed YAML frontmatter line: {line!r}")
            continue
        key, value = match[1], (match[2] or "").strip()
        if key in fields:
            errors.append(f"{path}: duplicate frontmatter key {key}")
        fields[key] = ""
        if value in ("|", ">", "|-", ">-", "|+", ">+"):
            block_key = key
        elif value.startswith('"'):
            try:
                parsed = json.loads(value)
                if not isinstance(parsed, str):
                    raise ValueError("expected a string")
                fields[key] = parsed
            except ValueError:
                errors.append(f"{path}: {key}: use a valid JSON-style double-quoted string")
        elif value.startswith("'"):
            if not re.fullmatch(r"'(?:[^']|'')*'", value):
                errors.append(f"{path}: {key}: invalid single-quoted YAML string")
            else:
                fields[key] = value[1:-1].replace("''", "'")
        elif (not value or value.lower() in ("null", "true", "false", "yes", "no", "on", "off", "~", ".nan", ".inf", "-.inf")
              or re.fullmatch(r"[-+]?\d[\d.eE+_-]*", value) or re.search(r":(?:\s|$)|\s#", value)
              or value.startswith(("[", "{", "&", "*", "!", "@", chr(96), "- ", "? ", ": "))):
            errors.append(f"{path}: {key}: unsupported/ambiguous YAML scalar; use a quoted string")
        else:
            fields[key] = value
    for key in ("name", "description"):
        if not nonempty(fields.get(key)):
            errors.append(f"{path}: frontmatter {key} must be a non-empty YAML string")
    name = fields.get("name", "")
    if name and name != path.parent.name:
        errors.append(f"{path}: frontmatter name must match folder {path.parent.name!r}")
    if not "\n".join(lines[end + 1:]).strip():
        errors.append(f"{path}: add skill instructions after frontmatter")
    return errors


def relative_links(path, root):
    """Check inline local Markdown links; external URLs/absolute machine paths are portable references."""
    errors = []
    source = re.sub(r"```.*?```", "", path.read_text(encoding="utf-8-sig"), flags=re.S)
    for match in re.finditer(r"(?<!!)\[[^\]\n]+\]\((<[^>]+>|[^)\s]+)(?:\s+\"[^\"]*\")?\)", source):
        target = unquote(match[1].strip("<>"))
        if not target or target.startswith(("#", "/", "\\")) or re.match(r"^[a-zA-Z][a-zA-Z0-9+.-]*:", target) or "{{" in target:
            continue
        local = target.split("#", 1)[0].split("?", 1)[0]
        if not local:
            continue
        resolved = path.parent / local
        if not within(resolved, root):
            errors.append(f"{path}: relative link escapes repository: {target}; use an explicit external reference")
        elif not resolved.exists():
            errors.append(f"{path}: broken relative link {target}; create the target or fix the link")
    return errors


def validate(root):
    errors = []
    required = ("AGENTS.md", "README.md", "studio/CHARTER.md", "studio/WORKFLOW.md", "studio/TEAM.md", "studio/IMPLEMENTATION-CONTRACT.md", "research/README.md", "research/sources.json")
    for relative in required:
        if not (root / relative).is_file():
            errors.append(f"{root / relative}: missing required studio file")
    for relative in GAME_FILES:
        if not (root / "templates/game" / relative).is_file():
            errors.append(f"{root / 'templates/game' / relative}: missing required game template")
    for relative in GAME_DIRS:
        if not (root / "templates/game" / relative).is_dir():
            errors.append(f"{root / 'templates/game' / relative}: missing required game template directory")
    template_manifest = root / "templates/game/game.json"
    if template_manifest.is_file():
        try:
            errors.extend(check_record(read_json(template_manifest), template_manifest, "game"))
        except StudioError as exc:
            errors.append(str(exc))
    for role in ROLES:
        path = root / ".codex/agents" / (role + ".toml")
        if not path.is_file():
            errors.append(f"{path}: missing native agent TOML")
    for path in sorted((root / ".codex/agents").glob("*.toml")):
        try:
            data = tomllib.loads(path.read_text(encoding="utf-8-sig"))
            for field in ("name", "description", "developer_instructions"):
                if not nonempty(data.get(field)):
                    errors.append(f"{path}: {field}: provide a non-empty native agent string")
            if data.get("name") != path.stem:
                errors.append(f"{path}: name must match filename stem {path.stem!r}")
            for field in ("model", "model_reasoning_effort", "sandbox_mode", "approval_policy"):
                if field in data:
                    errors.append(f"{path}: remove {field}; agents inherit user model and permissions")
        except (OSError, UnicodeError, tomllib.TOMLDecodeError) as exc:
            errors.append(f"{path}: invalid TOML: {exc}")
    for skill in SKILLS:
        path = root / ".agents/skills" / skill / "SKILL.md"
        if not path.is_file():
            errors.append(f"{path}: missing native skill")
    for path in sorted((root / ".agents/skills").glob("*/SKILL.md")):
        try:
            errors.extend(skill_frontmatter(path))
        except (OSError, UnicodeError) as exc:
            errors.append(f"{path}: cannot read skill: {exc}")
    sources = root / "research/sources.json"
    if sources.is_file():
        try:
            catalog = read_json(sources)
            if not isinstance(catalog, dict) or type(catalog.get("schema_version")) is not int or catalog["schema_version"] != 1 or not isinstance(catalog.get("sources"), list):
                errors.append(f"{sources}: expected schema_version: 1 and sources: array")
            else:
                seen = set()
                for i, record in enumerate(catalog["sources"]):
                    if not isinstance(record, dict):
                        errors.append(f"{sources}: sources[{i}] must be an object")
                        continue
                    for key in ("id", "name", "url", "purpose", "access"):
                        if not nonempty(record.get(key)):
                            errors.append(f"{sources}: sources[{i}].{key}: provide a non-empty string")
                    if not web_url(record.get("url")):
                        errors.append(f"{sources}: sources[{i}].url: provide an HTTP(S) URL")
                    source_id = record.get("id")
                    if isinstance(source_id, str):
                        if source_id in seen:
                            errors.append(f"{sources}: duplicate source id {source_id!r}")
                        seen.add(source_id)
        except StudioError as exc:
            errors.append(str(exc))
    candidates = {}
    for path in sorted((root / "research/opportunities").glob("*.json")):
        try:
            data = read_json(path)
            errors.extend(check_record(data, path, "candidate"))
            if isinstance(data, dict) and "context" in data and not check_context_shape(data, path):
                context_sources(root, data, path, "opportunity")
            if isinstance(data, dict) and nonempty(data.get("id")):
                if data["id"] in candidates:
                    errors.append(f"{path}: duplicate candidate id {data['id']!r}")
                candidates[data["id"]] = path
        except StudioError as exc:
            errors.append(str(exc))
    games = root / "games"
    if games.exists():
        for folder in sorted(games.iterdir()):
            if folder.name.startswith(".") or not folder.is_dir():
                continue
            if not within(folder, games):
                errors.append(f"{folder}: game directory must remain inside games/")
                continue
            path = folder / "game.json"
            for relative in GAME_FILES:
                if not (folder / relative).is_file():
                    errors.append(f"{folder / relative}: missing required game file; see templates/game/")
            for relative in GAME_DIRS:
                if not (folder / relative).is_dir():
                    errors.append(f"{folder / relative}: missing required game directory")
            if path.is_file():
                try:
                    data = read_json(path)
                    errors.extend(check_record(data, path, "game"))
                    if isinstance(data, dict) and "context" in data and not check_context_shape(data, path):
                        context_sources(root, data, path, "game")
                    if isinstance(data, dict):
                        if data.get("slug") != folder.name:
                            errors.append(f"{path}: slug must match folder name {folder.name!r}")
                        ref = data.get("source_opportunity")
                        if ref is not None and (not isinstance(ref, str) or ref not in candidates):
                            errors.append(f"{path}: source_opportunity {ref!r} is not an id in research/opportunities/")
                except StudioError as exc:
                    errors.append(str(exc))
            manifest = folder / "assets/manifest.csv"
            if manifest.is_file():
                try:
                    with manifest.open(encoding="utf-8-sig", newline="") as handle:
                        columns = csv.DictReader(handle).fieldnames or []
                    missing = set(ASSET_COLUMNS) - set(columns)
                    if missing:
                        errors.append(f"{manifest}: missing license tracking columns: {', '.join(sorted(missing))}")
                except (OSError, UnicodeError, csv.Error) as exc:
                    errors.append(f"{manifest}: invalid asset CSV: {exc}")
    ignored = {".git", ".local", ".venv", "venv", "node_modules", "__pycache__", ".godot", ".unity", "Library", "Temp", "Obj", "Logs", "UserSettings", "Binaries", "Intermediate", "Saved", "DerivedDataCache", "build", "builds", "dist", "target", "vendor"}
    for folder, dirs, files in os.walk(root, followlinks=False):
        # Prune before traversing downloaded tool documentation and engine caches.
        dirs[:] = [name for name in dirs if name not in ignored and not (Path(folder) / name).is_symlink()]
        for filename in files:
            path = Path(folder) / filename
            if path.suffix.lower() != ".md" or path.is_symlink():
                continue
            try:
                errors.extend(relative_links(path, root))
            except (OSError, UnicodeError) as exc:
                errors.append(f"{path}: cannot check Markdown references: {exc}")
    return errors


def rename_no_replace(source, target):
    """Publish a directory atomically without replacing an existing destination."""
    if os.name == "nt":
        os.rename(source, target)  # Windows rename fails when the destination exists.
    elif sys.platform.startswith("linux"):
        libc = ctypes.CDLL(None, use_errno=True)
        rename = getattr(libc, "renameat2", None)
        if rename is None:
            raise StudioError("Atomic non-overwriting directory publication requires Linux renameat2 or Windows")
        rename.argtypes = [ctypes.c_int, ctypes.c_char_p, ctypes.c_int, ctypes.c_char_p, ctypes.c_uint]
        rename.restype = ctypes.c_int
        if rename(-100, os.fsencode(source), -100, os.fsencode(target), 1) != 0:
            code = ctypes.get_errno()
            raise OSError(code, os.strerror(code), str(target))
    else:
        raise StudioError("Atomic new-game publication is currently supported on Windows and Linux")


def new_game(root, slug, title, engine):
    if not slug_ok(slug):
        raise StudioError("Invalid slug: use lowercase letters/digits separated by hyphens, max 63 characters; no paths or Windows reserved names")
    if not nonempty(title):
        raise StudioError("--title must contain text")
    if engine not in ENGINES:
        raise StudioError(f"Unknown engine: {engine}")
    root = root.resolve()
    if not root.is_dir():
        raise StudioError(f"Repository root does not exist: {root}")
    template = root / "templates/game"
    if template.is_symlink():
        raise StudioError(f"Template symlinks are not allowed: {template}")
    for relative in GAME_FILES + GAME_DIRS:
        path = template / relative
        if not path.exists() or not within(path, template):
            raise StudioError(f"Missing or unsafe template: {path}")
    for path in template.rglob("*"):
        if path.is_symlink() or not within(path, template):
            raise StudioError(f"Template symlinks are not allowed: {path}")
    games = root / "games"
    if games.is_symlink() or not within(games, root):
        raise StudioError(f"Unsafe games directory: {games}")
    games.mkdir(exist_ok=True)
    target = games / slug
    if target.exists() or target.is_symlink():
        raise StudioError(f"Refusing to overwrite existing path: {target}")
    staged = Path(tempfile.mkdtemp(prefix=f".{slug}-", dir=games))
    try:
        shutil.copytree(template, staged, dirs_exist_ok=True)
        today = date.today().isoformat()
        for path in staged.rglob("*.md"):
            content = path.read_text(encoding="utf-8-sig")
            for key, value in {"SLUG": slug, "TITLE": title.strip(), "ENGINE": engine, "DATE": today}.items():
                content = content.replace("{{" + key + "}}", value)
            path.write_text(content, encoding="utf-8", newline="\n")
        data = read_json(staged / "game.json")
        data.update(schema_version=1, slug=slug, title=title.strip(), engine=engine, created_on=today)
        problems = check_record(data, staged / "game.json", "game")
        if problems:
            raise StudioError("Invalid game template:\n" + "\n".join(problems))
        (staged / "game.json").write_text(json.dumps(data, ensure_ascii=False, indent=2) + "\n", encoding="utf-8", newline="\n")
        rename_no_replace(staged, target)
    finally:
        if staged.exists():
            # Only remove the exact temporary directory created above, still within games/.
            if staged.parent.resolve() == games.resolve() and staged.name.startswith(f".{slug}-"):
                shutil.rmtree(staged)
    return target


def status(root):
    paths = sorted((root / "games").glob("*/game.json"))
    if not paths:
        print("No games scaffolded. No game has been selected by this tooling.")
        return 0
    failures = 0
    for path in paths:
        if path.parent.name.startswith("."):
            continue
        try:
            data = read_json(path)
            problems = check_record(data, path, "game")
            if problems:
                raise StudioError("; ".join(problems))
            print(f"{data['slug']} | {data['title']} | {data['engine']} | {data['stage']}")
            print(f"  Resume: {path.parent / 'STATUS.md'}")
        except StudioError as exc:
            failures += 1
            print(f"ERROR: {exc}", file=sys.stderr)
    return int(bool(failures))


def context_path(root, value):
    """Resolve an existing public text source without permitting traversal or escapes."""
    if (not nonempty(value) or len(value) > 512 or "\\" in value or ":" in value
            or any(ord(character) < 32 for character in value)
            or value.startswith("/") or PureWindowsPath(value).drive
            or any(part in ("", ".", "..") for part in value.split("/"))):
        raise StudioError(f"Unsafe context path {value!r}: use a repository-relative file path with forward slashes; no traversal or absolute paths")
    path = root / value
    try:
        resolved = path.resolve(strict=True)
    except (OSError, RuntimeError) as exc:
        raise StudioError(f"Missing or invalid context file {value!r}: {exc}") from exc
    if not resolved.is_relative_to(root.resolve()) or not resolved.is_file():
        raise StudioError(f"Unsafe context file {value!r}: must be an existing file confined to this repository (including symlink targets)")
    for relative in (Path(value), resolved.relative_to(root.resolve())):
        if any(part.casefold() in (".git", ".local", "config.local.json") or part.casefold().startswith(".env") for part in relative.parts):
            raise StudioError(f"Private configuration is not a context source: {value!r}")
    return path


def context_sources(root, data, path, kind):
    """Return ordered (relative path, resolved path) sources, with old-record fallbacks."""
    problems = check_context_shape(data, path)
    if problems:
        raise StudioError("; ".join(problems))
    context = data.get("context")
    if context is not None:
        relatives = [context["entrypoint"], *context.get("read_first", [])]
    elif kind == "game":
        relatives = [(path.parent / "STATUS.md").relative_to(root).as_posix()]
    else:
        owner = data.get("owner_selected_foundations", {})
        design = owner.get("current_design") if isinstance(owner, dict) else None
        fallback = design if nonempty(design) else data.get("research_run")
        relatives = [fallback] if nonempty(fallback) else []
    sources = [(relative, context_path(root, relative)) for relative in dict.fromkeys(relatives)]
    if any(source.resolve() == path.resolve() for _, source in sources):
        raise StudioError("Do not list the owning JSON record as a context source; it is already hashed separately without its checkpoint")
    return sources


def context_records(root, qualified_target=None):
    """Discover both authoritative record kinds; no separate active-project registry."""
    records, errors = [], []
    for kind, pattern in (("game", "games/*/game.json"), ("opportunity", "research/opportunities/*.json")):
        for path in sorted(root.glob(pattern)):
            if path.parent.name.startswith(".") or path.name.startswith("."):
                continue
            data = None
            try:
                context_path(root, path.relative_to(root).as_posix())
                data = read_json(path)
                problems = check_record(data, path, "game" if kind == "game" else "candidate")
                if problems:
                    raise StudioError("; ".join(problems))
                identifier = data["slug"] if kind == "game" else data["id"]
                if not slug_ok(identifier):
                    raise StudioError(f"{path}: context target id must be a lowercase slug, not a path")
                if kind == "game" and identifier != path.parent.name:
                    raise StudioError(f"{path}: slug must match game directory name")
                records.append({"kind": kind, "id": identifier, "path": path, "data": data})
            except StudioError as exc:
                identifier = data.get("slug" if kind == "game" else "id") if isinstance(data, dict) else None
                errors.append({"kind": kind, "id": identifier, "path": path, "message": str(exc)})
    seen = set()
    for record in records:
        key = (record["kind"], record["id"])
        if key in seen:
            errors.append({**record, "message": f"Duplicate context target {key[0]}:{key[1]}; fix the authoritative records before resuming"})
        seen.add(key)
    fatal = errors
    if qualified_target is not None:
        kind, identifier = qualified_target
        # A broken record's conventional filename still identifies a possible match.
        # Parsed ids also catch invalid duplicates stored under a different filename.
        fatal = [error for error in errors if error["kind"] == kind and
                 (error["id"] == identifier or (error["path"].parent.name if kind == "game" else error["path"].stem) == identifier)]
    if fatal:
        report = "\n".join(error["message"] for error in fatal[:8])
        if len(fatal) > 8:
            report += f"\n... {len(fatal) - 8} more record errors; run validate for details."
        raise StudioError(report[:6000])
    if errors:
        print(f"WARNING: skipped {len(errors)} unrelated invalid record(s) while reading {qualified_target[0]}:{qualified_target[1]}; run validate for details.", file=sys.stderr)
        for error in errors[:3]:
            print(f"- {compact_text(error['path'].relative_to(root).as_posix(), 160)}: {compact_text(error['message'], 240)}", file=sys.stderr)
        if len(errors) > 3:
            print(f"- {len(errors) - 3} additional unrelated record errors omitted.", file=sys.stderr)
    return records


def compact_text(value, limit=300):
    text = " ".join(str(value).split())
    return text if len(text) <= limit else text[:limit - 3] + "..."


def print_context(lines):
    text = "\n".join(lines)
    if len(text) > CONTEXT_OUTPUT_CHARS:
        text = text[:CONTEXT_OUTPUT_CHARS].rsplit("\n", 1)[0] + "\n[Output shortened; open the record and listed handoff files for the rest.]"
    print(text)


def context_fingerprint(data, sources):
    """Hash record semantics and UTF-8 source text, independent of checkout newlines."""
    canonical = dict(data)
    if "context" in canonical:
        canonical["context"] = {key: value for key, value in canonical["context"].items() if key != "checkpoint"}
    try:
        encoded = json.dumps(canonical, ensure_ascii=False, sort_keys=True, separators=(",", ":"), allow_nan=False).encode("utf-8")
    except (ValueError, TypeError) as exc:
        raise StudioError(f"Cannot checkpoint noncanonical record data: {exc}") from exc
    files = {}
    for relative, path in sources:
        digest = hashlib.sha256()
        try:
            # Universal newlines normalize CRLF/LF, including across chunk boundaries.
            with path.open(encoding="utf-8-sig", newline=None) as handle:
                while chunk := handle.read(65536):
                    digest.update(chunk.encode("utf-8"))
        except (OSError, UnicodeError) as exc:
            raise StudioError(f"Cannot checkpoint context text {relative!r}: {exc}") from exc
        files[relative] = digest.hexdigest()
    return {"record_sha256": hashlib.sha256(encoded).hexdigest(), "files": files}


def save_context_checkpoint(path, data, expected_bytes):
    """Replace only the chosen record, refusing an observed concurrent record change."""
    staged = None
    try:
        with tempfile.NamedTemporaryFile(mode="w", encoding="utf-8", newline="\n", prefix=".context-", suffix=".json", dir=path.parent, delete=False) as handle:
            staged = Path(handle.name)
            json.dump(data, handle, ensure_ascii=False, indent=2, allow_nan=False)
            handle.write("\n")
        if path.read_bytes() != expected_bytes:
            raise StudioError("Record changed while checkpointing; review the updated handoff and run the command again")
        os.replace(staged, path)
    finally:
        if staged is not None:
            staged.unlink(missing_ok=True)


def project_context(root, target=None, checkpoint=False):
    if checkpoint and target is None:
        raise StudioError("--checkpoint requires an explicit target after reviewing its handoff; no project is selected automatically")
    if target is not None:
        qualifier, separator, identifier = target.partition(":")
        if not separator:
            qualifier, identifier = None, target
        if (qualifier is not None and qualifier not in ("game", "opportunity")) or not slug_ok(identifier):
            raise StudioError("Invalid context target: use an exact id, game:ID or opportunity:ID; paths and partial matches are not accepted")
    root = root.resolve()
    if not root.is_dir():
        raise StudioError(f"Repository root does not exist: {root}")
    try:
        records = context_records(root, (qualifier, identifier) if target is not None and qualifier is not None else None)
    except StudioError as exc:
        if target is not None and qualifier is None:
            raise StudioError(f"{exc}\nCannot confidently resolve a bare id while records are invalid; use game:{identifier} or opportunity:{identifier}.") from exc
        raise
    if target is None:
        lines = ["Project context index (recorded stage/status; nothing selected or resumed):"]
        for record in records[:CONTEXT_MAX_INDEX]:
            data = record["data"]
            state = data["stage"] if record["kind"] == "game" else data["status"]
            lines.append(f"- {record['kind']}:{record['id']} | {compact_text(data['title'], 120)} | {state}")
        if not records:
            lines.append("No game or opportunity records found.")
        if len(records) > CONTEXT_MAX_INDEX:
            lines.append(f"... {len(records) - CONTEXT_MAX_INDEX} additional records; inspect games/ and research/opportunities/ or use an exact target.")
        lines.append("Open one: python scripts/studio.py context game:ID or opportunity:ID")
        lines.append("This index does not verify handoff freshness. Parked/rejected records retain their status.")
        print_context(lines)
        return 0
    matches = [record for record in records if record["id"] == identifier and (qualifier is None or record["kind"] == qualifier)]
    if not matches:
        raise StudioError(f"Unknown context target {target!r}; run context without a target to see recorded ids")
    if len(matches) > 1:
        raise StudioError(f"Ambiguous context target {target!r}; use game:{identifier} or opportunity:{identifier}")
    record = matches[0]
    data, path, kind = record["data"], record["path"], record["kind"]
    expected_bytes = path.read_bytes()
    # Do not overwrite a record changed between discovery and this read.
    if read_json(path) != data:
        raise StudioError("Record changed during context discovery; retry after reviewing the new record")
    sources = context_sources(root, data, path, kind)
    if checkpoint and not sources:
        raise StudioError("No handoff is configured; add context.entrypoint to this record before checkpointing")
    excerpt = ""
    if sources:
        with sources[0][1].open(encoding="utf-8-sig", newline=None) as handle:
            excerpt = handle.read(CONTEXT_EXCERPT_CHARS + 1)
    if checkpoint:
        if "context" not in data:
            data["context"] = {"entrypoint": sources[0][0], "read_first": []}
        data["context"]["checkpoint"] = {
            "schema_version": 1, "captured_at": datetime.now(timezone.utc).isoformat(timespec="seconds"),
            **context_fingerprint(data, sources),
        }
        save_context_checkpoint(path, data, expected_bytes)
    context = data.get("context", {})
    saved = context.get("checkpoint")
    qualified = f"{kind}:{record['id']}"
    state = data["stage"] if kind == "game" else data["status"]
    lines = [f"{qualified} | {compact_text(data['title'], 180)}", f"Recorded {'stage' if kind == 'game' else 'status'}: {state}",
             f"Record: {path.relative_to(root).as_posix()}"]
    if saved:
        actual = context_fingerprint(data, sources)
        changed = (["record metadata"] if actual["record_sha256"] != saved["record_sha256"] else [])
        changed.extend(relative for relative in sorted(actual["files"].keys() | saved["files"].keys()) if actual["files"].get(relative) != saved["files"].get(relative))
        if changed:
            lines.append(f"Freshness: STALE - content changed since checkpoint {saved['captured_at']}.")
            lines.append("Changed: " + compact_text("; ".join(changed), 1200))
        else:
            lines.append(f"Freshness: content unchanged since checkpoint {saved['captured_at']}.")
        lines.append("A checkpoint compares the owning record and listed sources; it does not establish completeness, correctness or gameplay quality.")
    else:
        lines.append("Freshness: unverified - no content checkpoint. Review the record and handoff before relying on them.")
    if checkpoint:
        lines.append("Checkpoint saved in the same record. Stage/status and other record fields were preserved.")
    next_action = data.get("next_action") or data.get("next_test")
    if not nonempty(next_action):
        match = re.search(r"(?im)^Next(?: concrete action)?\s*:\s*(.+)$", excerpt)
        next_action = match[1] if match else (f"Read the next action/resume section in {sources[0][0]}" if sources else "No handoff or next action is recorded")
    lines.append("Next: " + compact_text(next_action, 500))
    owner = data.get("owner_selected_foundations", {})
    if isinstance(owner, dict):
        for key in ("camera", "equipment", "equipment_growth", "story_seed", "power_and_rewards", "open_design"):
            if nonempty(owner.get(key)):
                lines.append(f"Owner {key.replace('_', ' ')}: {compact_text(owner[key], 240)}")
    if sources:
        lines.extend(["", "Handoff first; reference files only as needed (supporting content is not concatenated):"])
        lines.extend(f"{index}. {relative}" for index, (relative, _) in enumerate(sources, 1))
        lines.extend(["", "Handoff excerpt:", excerpt[:CONTEXT_EXCERPT_CHARS].rstrip()])
        if len(excerpt) > CONTEXT_EXCERPT_CHARS:
            lines.append("[Handoff excerpt shortened; open the entrypoint for the rest.]")
        lines.append("Open supporting files only as needed. This command does not read config.local.json or launch tools.")
    else:
        lines.append("No handoff configured. Add context.entrypoint to this record to provide a resumable reading order.")
    print_context(lines)
    return 0


def doctor(root):
    print(f"Required Python: {sys.version.split()[0]} ({sys.executable})")
    git = shutil.which("git")
    print(f"Required Git: {git or 'MISSING - install Git and add it to PATH'}")
    tools = {}
    config = root / "config.local.json"
    if config.exists():
        data = read_json(config)
        if not isinstance(data, dict) or not isinstance(data.get("tools", {}), dict):
            raise StudioError(f"{config}: expected an object with optional tools object mapping names to executable paths")
        tools = data.get("tools", {})
        if not all(nonempty(k) and nonempty(v) for k, v in tools.items()):
            raise StudioError(f"{config}: tools must map non-empty names to executable path strings")
    defaults = {"unreal": "UnrealEditor", "godot": "godot", "unity": "Unity", "node": "node", "blender": "blender"}
    for name in sorted(defaults.keys() | tools.keys()):
        configured = tools.get(name)
        if configured:
            location = Path(configured).expanduser()
            if not location.is_absolute():
                location = root / location
            executable = str(location) if location.is_file() else None
        else:
            executable = shutil.which(defaults[name])
        print(f"Optional {name}: {executable or ('NOT FOUND (check configured path)' if configured else 'not detected')}")
    print("Optional tools are reported only; no engine is downloaded or launched. Presence is not a build test.")
    return int(git is None or sys.version_info < (3, 11))


def fetch_json(url):
    request = Request(url, headers={"User-Agent": "Game-Studio-research/1.0 (public Steam snapshot)", "Accept": "application/json"})
    try:
        with urlopen(request, timeout=30) as response:
            return json.load(response)
    except (OSError, ValueError, HTTPError, URLError) as exc:
        raise StudioError(f"Steam request failed for {url}: {exc}") from exc


def snapshot_app(appid):
    price_url = f"https://store.steampowered.com/api/appdetails?appids={appid}&cc=us&l=english"
    review_url = f"https://store.steampowered.com/appreviews/{appid}?json=1&language=all&purchase_type=steam&filter=all&num_per_page=0&review_type=all"
    details = fetch_json(price_url)
    reviews = fetch_json(review_url)
    record = details.get(str(appid)) if isinstance(details, dict) else None
    if not isinstance(record, dict) or record.get("success") is not True or not isinstance(record.get("data"), dict):
        raise StudioError(f"Steam app {appid}: appdetails did not return a successful app record ({price_url})")
    data = record["data"]
    if not nonempty(data.get("name")):
        raise StudioError(f"Steam app {appid}: appdetails missing game name ({price_url})")
    if data.get("is_free") is True:
        price = {"currency": "USD", "list": 0.0, "current": 0.0, "is_free": True}
    else:
        raw = data.get("price_overview")
        if not isinstance(raw, dict) or raw.get("currency") != "USD" or not number(raw.get("initial")) or not number(raw.get("final")):
            raise StudioError(f"Steam app {appid}: USD price unavailable (unreleased, unavailable in US, or incomplete response); no price invented ({price_url})")
        price = {"currency": "USD", "list": raw["initial"] / 100, "current": raw["final"] / 100, "is_free": False}
    summary = reviews.get("query_summary") if isinstance(reviews, dict) else None
    if not isinstance(reviews, dict) or reviews.get("success") != 1 or not isinstance(summary, dict):
        raise StudioError(f"Steam app {appid}: review summary unavailable ({review_url})")
    counts = {}
    for key in ("total_positive", "total_negative", "total_reviews"):
        value = summary.get(key)
        if type(value) is not int or value < 0:
            raise StudioError(f"Steam app {appid}: invalid or missing review count {key} ({review_url})")
        counts[key] = value
    if counts["total_positive"] + counts["total_negative"] != counts["total_reviews"]:
        raise StudioError(f"Steam app {appid}: inconsistent review totals ({review_url})")
    return {"app_id": appid, "name": data["name"], "source_urls": {"details": price_url, "reviews": review_url}, "price_usd": price, "reviews": counts}


def steam_snapshot(apps, out):
    if any(type(app) is not int or app <= 0 for app in apps):
        raise StudioError("Steam app IDs must be positive integers")
    if out.exists() or out.is_symlink():
        raise StudioError(f"Refusing to overwrite snapshot: {out}; choose a new dated output file")
    records = []
    for app in dict.fromkeys(apps):
        records.append(snapshot_app(app))
    result = {"schema_version": 1, "status": "complete", "captured_at": datetime.now(timezone.utc).isoformat(), "review_filter": {"language": "all", "purchase_type": "steam", "review_type": "all", "filter": "all"}, "note": "Observed Steam prices and review counts, not sales or revenue estimates.", "apps": records}
    out.parent.mkdir(parents=True, exist_ok=True)
    # Write completely first, then link atomically without replacing an existing file.
    descriptor, temporary = tempfile.mkstemp(prefix=".steam-snapshot-", suffix=".tmp", dir=out.parent)
    staged = Path(temporary)
    try:
        with os.fdopen(descriptor, "w", encoding="utf-8", newline="\n") as handle:
            json.dump(result, handle, ensure_ascii=False, indent=2)
            handle.write("\n")
        os.link(staged, out)
    except FileExistsError as exc:
        raise StudioError(f"Refusing to overwrite snapshot: {out}") from exc
    finally:
        staged.unlink(missing_ok=True)
    return result


def parser():
    result = argparse.ArgumentParser(description="Manage small game projects and evidence without third-party Python packages. Validation is structural, not an engine build or fun test.")
    commands = result.add_subparsers(dest="command", required=True)
    def rooted(name, help_text):
        command = commands.add_parser(name, help=help_text, description=help_text)
        command.add_argument("--root", type=Path, default=ROOT, help="Repository root (defaults to this script's parent-parent)")
        return command
    command = rooted("new-game", "Create an honest game planning scaffold atomically; refuse overwrites. Does not select a concept or create a working engine project.")
    command.add_argument("slug", help="Lowercase letters/digits separated by hyphens; no paths")
    command.add_argument("--title", required=True, help="Human-readable title")
    command.add_argument("--engine", choices=ENGINES, required=True, help="Planning metadata; no engine install")
    rooted("status", "Discover game.json manifests and show each game's stage and resume file.")
    command = rooted("context", "Show a bounded game/opportunity index or an explicit target's handoff and content freshness; no project is resumed automatically.")
    command.add_argument("target", nargs="?", help="Exact id, game:ID or opportunity:ID; qualify ids shared by both record kinds")
    command.add_argument("--checkpoint", action="store_true", help="After reviewing an explicit target's handoff, record its content hashes in that same record")
    rooted("validate", "Check repository layout, JSON schemas, native agent TOML, skill frontmatter and local references.")
    command = commands.add_parser("score", help="Validate and score one opportunity; scores are not sales forecasts.")
    command.add_argument("path", type=Path, help="Path to an opportunity JSON file")
    rooted("doctor", "Report required Python/Git and optional configured/PATH tools without launching engines.")
    command = commands.add_parser("steam-snapshot", help="Fetch USD prices and all-language Steam-purchase reviews; never estimate sales.")
    command.add_argument("--apps", type=int, nargs="+", required=True, help="Positive Steam app IDs; duplicates are fetched once")
    command.add_argument("--out", type=Path, required=True, help="New JSON file; an existing file is never overwritten")
    return result


def main(argv=None):
    for stream in (sys.stdout, sys.stderr):
        if hasattr(stream, "reconfigure"):
            stream.reconfigure(encoding="utf-8")
    args = parser().parse_args(argv)
    try:
        if args.command == "new-game":
            print(f"Created planning scaffold: {new_game(args.root, args.slug, args.title, args.engine)}")
            print("Engine project is not implemented. Start with BRIEF.md and STATUS.md.")
        elif args.command == "status":
            return status(args.root)
        elif args.command == "context":
            return project_context(args.root, args.target, args.checkpoint)
        elif args.command == "validate":
            errors = validate(args.root.resolve())
            if errors:
                print(f"Validation failed: {len(errors)} issue(s). See studio/IMPLEMENTATION-CONTRACT.md.", file=sys.stderr)
                for error in errors:
                    print(f"- {error}", file=sys.stderr)
                return 1
            print("Validation passed: structure, records, native metadata and relative links. No engine build or gameplay claim.")
        elif args.command == "score":
            data = read_json(args.path)
            errors = check_record(data, args.path, "candidate")
            if errors:
                raise StudioError("\n".join(errors))
            score = sum(data["scores"][key] * weight for key, weight in WEIGHTS.items())
            print(f"{data['title']}: {score:.2f}/5 ({score * 20:.1f}/100)")
            print(f"Trend: {data['trend_stage']}; confidence: {data['confidence']}; status: {data['status']}")
            print("Decision aid only; not a sales forecast or evidence that fun has been validated.")
        elif args.command == "doctor":
            return doctor(args.root.resolve())
        else:
            result = steam_snapshot(args.apps, args.out)
            print(f"Snapshot complete: {len(result['apps'])} app(s) -> {args.out}")
        return 0
    except (StudioError, OSError, UnicodeError) as exc:
        print(f"ERROR: {exc}", file=sys.stderr)
        if args.command == "steam-snapshot":
            print("Snapshot status: FAILED. No partial research data should be used.", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
