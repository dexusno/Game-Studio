"""Compile P00's complete Cinderwall boundary; this does not execute game rules.

Standard library only. Run without flags to regenerate the two deterministic JSON
artifacts. --check checks them without writing. --release additionally requires
per-operation implementation/test evidence from runtime-support.json. Source
eligibility never shrinks to match that support file.
"""

from __future__ import annotations

import argparse
import copy
import hashlib
import itertools
import json
import re
import sys
from pathlib import Path


GAME = Path(__file__).resolve().parents[1]
CONTENT = GAME / "content"
MATERIALS = ("Iron", "Copper", "Carbon", "Glass", "Circuit")
RARITIES = ("Common", "Uncommon", "Rare", "Legendary")
RECIPE_ID = re.compile(r"^(SH|MA|IV|AD|NO)\d{3}$")
ROBOT_ID = re.compile(r"^(C[123]-[ROB]\d{2})\b")
CHARACTERS = {"SH": "Shared", "MA": "Mara", "IV": "Ivo", "AD": "Ada", "NO": "Noor"}
ARTIFACTS = ("catalogue.snapshot.json", "cinderwall.manifest.json")
CORE_OPERATIONS = (
    "core.action-validation", "core.integer-damage", "core.event-order",
    "core.deterministic-streams", "core.collect-and-precision", "core.recipe-copies",
    "core.physical-parts", "core.installation-history", "core.shield-payments",
    "core.cooldown-clocks", "core.load-unload-fire", "core.end-turn",
    "core.fight-outcome", "core.core-and-part-sales", "core.shop-transactions",
    "core.reward-claims", "core.memory-exchange", "core.collection-discovery",
    "core.profile-and-unlocks", "core.fight-entry-restart", "core.choice-persistence",
    "core.city-completion", "core.nested-choice-transactions",
)


class InvalidContent(ValueError):
    pass


def require(condition, message):
    if not condition:
        raise InvalidContent(message)


def encoded(value):
    return (json.dumps(value, ensure_ascii=False, sort_keys=True, indent=2) + "\n").encode("utf-8")


def digest(data):
    return hashlib.sha256(data).hexdigest()


def read_json(relative):
    return json.loads((GAME / relative).read_text(encoding="utf-8"))


def source_ref(relative, line=None, text=None, pointer=None):
    ref = {"path": relative}
    if line is not None:
        ref["line"] = line
    if text is not None:
        ref["exact_text"] = text
    if pointer is not None:
        ref["json_pointer"] = pointer
    return ref


def cells(line):
    return [cell.strip() for cell in line.strip().strip("|").split("|")]


def parse_recipes():
    relative = "design/RECIPE-CATALOGUE.md"
    result = {}
    rarity = None
    for n, line in enumerate((GAME / relative).read_text(encoding="utf-8").splitlines(), 1):
        if line in {"### " + r for r in ("Base",) + RARITIES}:
            rarity = line[4:]
        if not re.match(r"^\| (SH|MA|IV|AD|NO)\d{3} \|", line):
            continue
        c = cells(line)
        require(len(c) == 7, f"Malformed recipe at {relative}:{n}")
        rid, name, output, kind, cost_text, cooldown, effect = c
        pairs = re.findall(r"(\d+) (Iron|Copper|Carbon|Glass|Circuit)", cost_text)
        require(" + ".join(f"{a} {b}" for a, b in pairs) == cost_text, f"Unparsed costs: {rid}")
        require(len({m for _, m in pairs}) == len(pairs), f"Repeated cost: {rid}")
        require(rid not in result and rarity is not None, f"Duplicate or unclassified recipe: {rid}")
        require(cooldown == "None" or cooldown.isdigit(), f"Unparsed cooldown: {rid}")
        result[rid] = {
            "id": rid, "name": name, "mercenary": CHARACTERS[rid[:2]],
            "rarity": rarity, "kind": kind, "output_text": output,
            "cost_text": cost_text, "materials": {m: int(a) for a, m in pairs},
            "cooldown_text": cooldown, "cooldown": None if cooldown == "None" else int(cooldown),
            "effect": effect, "source": source_ref(relative, n, line),
        }
    require(len(result) == 606, f"Expected current 606-row source contract, got {len(result)}")
    return result


def parse_upgrades():
    result = {}
    for filename in ("shared-upgrades.json", "character-mayor-upgrades.json"):
        relative = "design/data/" + filename
        for i, raw in enumerate(read_json(relative)["upgrades"]):
            require(raw["id"] not in result, f"Duplicate upgrade {raw['id']}")
            result[raw["id"]] = {**raw, "source": source_ref(relative, pointer=f"/upgrades/{i}")}
    require(len(result) == 288, f"Expected current 288-upgrade source contract, got {len(result)}")
    return result


def parse_robots():
    relative = "design/ENEMY-ROSTER.md"
    result = {}
    for n, line in enumerate((GAME / relative).read_text(encoding="utf-8").splitlines(), 1):
        if not line.startswith("| C"):
            continue
        c = cells(line)
        match = ROBOT_ID.match(c[0])
        if not match:
            continue
        rid = match[1]
        require(len(c) == 5 and rid not in result, f"Malformed/duplicate robot {rid}")
        name = re.sub(r"\*\*", "", c[0][len(rid):]).strip()
        result[rid] = {
            "id": rid, "name": name, "tier": int(rid[1]),
            "category": {"R": "Regular", "O": "Officer", "B": "Boss"}[rid[3]],
            "hp_text": c[1], "effect": c[2], "counterplay": c[3],
            "inspiration": c[4], "source": source_ref(relative, n, line),
        }
    require(len(result) == 42, f"Expected current 42-robot source contract, got {len(result)}")
    return result


def parse_statuses():
    relative = "design/ROBOT-EFFECTS.md"
    aliases = {"Burn": "thermal-runaway", "Corrosion": "acid-etch", "Mark": "target-paint", "Weaken": "drive-fault"}
    result = {}
    for n, line in enumerate((GAME / relative).read_text(encoding="utf-8").splitlines(), 1):
        if not line.startswith("| "):
            continue
        c = cells(line)
        if re.fullmatch(r"FX\d{2}", c[0]):
            name = c[1].split(":", 1)[0].replace("**", "").removesuffix(" N")
            result[c[0]] = {"id": c[0], "name": name, "aliases": [], "effect": c[1], "timing": c[2], "purpose": c[3], "source": source_ref(relative, n, line)}
        elif c[0] in aliases:
            name = c[1].removesuffix(" N")
            sid = aliases[c[0]]
            result[sid] = {"id": sid, "name": name, "aliases": [c[0]], "effect": c[2], "source": source_ref(relative, n, line)}
    require(len(result) == 24, "Status table changed: expected 20 FX and four canonical aliases")
    return result


def allowed_upgrade(u, mercenary="Mara", city=1):
    return u["mercenary"] in ("Shared", mercenary) and u["min_city"] <= city <= u["max_city"]


def matching_statuses(text, statuses):
    return sorted(sid for sid, row in statuses.items() if any(
        re.search(r"(?<![\w])" + re.escape(term) + r"(?![\w])", text)
        for term in [row["name"]] + row["aliases"]
    ))


def compile_all():
    authored = read_json("content/cinderwall-authored.json")
    annotations = read_json("content/generated-output-annotations.json")
    beta = read_json("design/data/beta-balance-v1.json")
    resale = read_json("design/data/part-resale-bases.json")
    recipes, upgrades, robots, statuses = parse_recipes(), parse_upgrades(), parse_robots(), parse_statuses()
    inputs = sorted(set(authored["source_contracts"] + [
        "design/RECIPE-CATALOGUE.md", "design/ENEMY-ROSTER.md", "design/ROBOT-EFFECTS.md",
        "design/data/beta-balance-v1.json", "design/data/part-resale-bases.json",
        "design/data/shared-upgrades.json", "design/data/character-mayor-upgrades.json",
        "content/cinderwall-authored.json", "content/generated-output-annotations.json",
        "tools/compile_content.py",
    ]))
    source_hashes = {path: digest((GAME / path).read_bytes()) for path in inputs}
    catalogue = {
        "schema_version": 1,
        "source_sha256": source_hashes,
        "recipes": recipes, "upgrades": upgrades, "robots": robots, "statuses": statuses,
        "beta": beta, "resale": resale,
        "rule_documents": {path: (GAME / path).read_text(encoding="utf-8") for path in authored["source_contracts"]},
        "notice": "Exact source records and contracts; no runtime effects or balance evidence are implied.",
    }
    er = {i: r for i, r in recipes.items() if r["mercenary"] in ("Shared", "Mara")}
    eu = {i: u for i, u in upgrades.items() if allowed_upgrade(u)}
    ordinary = {i: u for i, u in eu.items() if "Mayor" not in u["acquisition"]}
    mayor = {i: u for i, u in eu.items() if "Mayor" in u["acquisition"]}
    recipe_pools = {}
    for route, weights in beta["recipe_offer_weights_by_city"]["1"].items():
        recipe_pools[route] = {
            "rarity_weights": dict(zip(RARITIES, weights)),
            "ids": sorted(i for i, r in er.items() if r["rarity"] in RARITIES and weights[RARITIES.index(r["rarity"])] > 0),
        }
    upgrade_pools = {}
    for route, weights in beta["upgrade_offer_weights_by_city"]["1"].items():
        upgrade_pools[route] = {
            "rarity_weights": dict(zip(RARITIES, weights)),
            "ids": sorted(i for i, u in ordinary.items() if route in u["acquisition"] and weights[RARITIES.index(u["rarity"])] > 0),
        }
    tech = next(m for m in authored["mysteries"] if m["category"] == "Tech support")["upgrade_offer"]
    upgrade_pools["Mystery"] = {
        "rarity_weights": dict(zip(tech["rarities"], tech["rarity_weights"])),
        "ids": sorted(i for i, u in ordinary.items() if "Mystery" in u["acquisition"] and u["rarity"] in tech["rarities"]),
    }
    mayor_pools = {lane: {
        "rarity_weights": dict(zip(("Rare", "Legendary"), beta["mayor_rare_legendary_weights_by_city"]["1"])),
        "ids": sorted(i for i, u in mayor.items() if u["mayor_lane"] == lane),
    } for lane in ("infrastructure", "tactical", "mercenary")}
    formations = copy.deepcopy(authored["formations"])
    registry_line = next(line for line in (GAME / "design/CAMPAIGN-PROGRESSION.md").read_text(encoding="utf-8").splitlines() if line.startswith("| **Mara 1: Cinderwall**"))
    for formation in formations:
        fragment = formation.get("source_fragment")
        require(fragment is None or fragment in registry_line, f"Formation no longer matches Cinderwall registry: {formation['id']}")
    require({b["robot_id"] for f in formations if f["category"] == "Officer" for b in f["bodies"]} == {rid for rid, r in robots.items() if r["tier"] == 1 and r["category"] == "Officer"}, "Cinderwall must include its tier's three Officers")
    route = authored["route"]
    position_gates = {}
    for position in range(1, 13):
        bands = {"opening"}
        if position >= 4:
            bands.add("middle")
        if position in route["pressure_positions"]:
            bands.add("late")
        position_gates[str(position)] = {
            "Regular": sorted(f["id"] for f in formations if f["band"] in bands) if position != 12 else [],
            "Officer": sorted(f["id"] for f in formations if f["category"] == "Officer") if position in route["officer_schedule"]["positions"] else [],
            "Mystery": sorted(m["id"] for m in authored["mysteries"]) if position in route["mystery_schedule"]["positions"] else [],
            "Boss": [f["id"] for f in formations if f["category"] == "Boss"] if position == 12 else [],
        }
    enabled_robots = {b["robot_id"] for f in formations for b in f["bodies"]}
    pending = list(enabled_robots)
    while pending:
        for summon in authored["robot_annotations"][pending.pop()]["summons"]:
            if summon["robot_id"] not in enabled_robots:
                enabled_robots.add(summon["robot_id"])
                pending.append(summon["robot_id"])
    require(enabled_robots == set(authored["robot_annotations"]), "Robot annotations must exactly cover roster closure")
    parts, outputs, dependencies = {}, {}, {}

    def part(pid, value):
        if pid not in parts:
            parts[pid] = {"id": pid, **value, "origins": [], "names": []}
        return pid

    def plain(kind, amount):
        require(type(amount) is int and amount > 0, "Generated fixed amounts must be positive whole numbers")
        lower = kind.lower()
        return part(f"plain-{lower}-{amount}", {
            "kind": kind, "canonical_recipe": "SH001" if kind == "Ammo" else "SH002",
            "fixed_damage" if kind == "Ammo" else "fixed_shield": amount,
            "resale": {"family": f"plain-{lower}", "original_fixed_value": amount},
            "ordinary_generated": True,
        })

    def own_part(rid):
        r = er[rid]
        return part("recipe-" + rid, {
            "kind": r["kind"], "canonical_recipe": rid,
            "resale": {"recipe": rid, "normal_one_part_materials": r["materials"]},
            "ordinary_generated": False,
        })

    def add_output(origin, pid, record):
        outputs.setdefault(origin, []).append({"part_id": pid, **record})
        if origin not in parts[pid]["origins"]:
            parts[pid]["origins"].append(origin)
        if record.get("name") and record["name"] not in parts[pid]["names"]:
            parts[pid]["names"].append(record["name"])
        dependencies.setdefault(origin, set()).add("part:" + pid)

    for rid, r in er.items():
        origin = "recipe:" + rid
        mappings = resale["recipe_outputs"].get(rid)
        if mappings:
            for mapping in mappings:
                basis = mapping["basis"]
                if basis.startswith("recipe:"):
                    require(basis[7:] in er, f"Out-of-scope ordinary source part: {basis}")
                    pids = [own_part(basis[7:])]
                elif basis.startswith("family:"):
                    kind = "Ammo" if basis == "family:plain-ammo" else "Shield"
                    pids = [plain(kind, value) for value in range(mapping["minimum"], mapping["maximum"] + 1)]
                else:
                    require(basis in resale["bases"], f"Unknown resale basis {basis}")
                    base = resale["bases"][basis]
                    if basis.startswith("plain-"):
                        amount = base.get("fixed_damage", base.get("fixed_shield"))
                        pids = [plain(base["kind"], amount)]
                    else:
                        pids = [part(basis, {"kind": base["kind"], "canonical_recipe": rid, "resale": {"basis": basis, "record": base}, "ordinary_generated": True})]
                for pid in pids:
                    add_output(origin, pid, {"source_mapping": mapping, "name": mapping["name"], "quantity": mapping["quantity"], "clock": mapping["when"], "choice_rule": "The exact source condition/alternative applies; listed alternatives are not simultaneous grants."})
        elif r["kind"] != "Utility":
            match = re.match(r"1 (.+)$", r["output_text"])
            require(match is not None, f"Unmapped non-single part output {rid}: {r['output_text']}")
            add_output(origin, own_part(rid), {"quantity": 1, "clock": "recipe Use into reserve, unless the explicit source says otherwise", "name": match[1]})

    for group, prefix, eligible in (("recipe_plain_grants", "recipe", er), ("upgrade_plain_grants", "upgrade", eu)):
        for identifier, annotation in annotations[group].items():
            require(identifier in eligible, f"Out-of-scope grant annotation: {identifier}")
            for kind, key in (("Ammo", "ammo"), ("Shield", "shield")):
                for amount in annotation.get(key, []):
                    add_output(prefix + ":" + identifier, plain(kind, amount), {
                        "quantity_and_clock": "Exactly as the source effect; values can be alternatives or conditional.",
                        "annotation": annotation, "source_effect": eligible[identifier]["effect"],
                    })
    for uid, rids in annotations["upgrade_named_grants"].items():
        require(uid in eu, f"Out-of-scope named grant: {uid}")
        for rid in rids:
            pid = own_part(rid)
            add_output("upgrade:" + uid, pid, {"canonical_recipe": rid, "quantity_and_clock": eu[uid]["effect"]})

    copy_selectors = {}
    for identifier, selector in annotations["part_copy_selectors"].items():
        origin = ("recipe:" if identifier in er else "upgrade:") + identifier
        require(identifier in er or identifier in eu, f"Unknown copy source {identifier}")
        # The broad graph includes every physical type that could pass this
        # selector. The executor evaluates exact instance provenance/rarity.
        selected = []
        for pid, record in parts.items():
            origin_rarities = {er[record["canonical_recipe"]]["rarity"]}
            origin_rarities.update(er[o[7:]]["rarity"] for o in record["origins"] if o.startswith("recipe:"))
            if record["kind"] in selector["kinds"] and origin_rarities.intersection(selector["rarities"]):
                selected.append(pid)
        copy_selectors[origin] = {**selector, "candidate_part_ids": sorted(selected), "instance_eligibility": "Read the original part's stored provenance, not the copying recipe's rarity. This type graph is a conservative dependency closure."}
        dependencies.setdefault(origin, set()).update("part:" + pid for pid in selected)

    nested = {}
    for uid, selector in annotations["nested_offer_selectors"].items():
        require(uid in eu, f"Out-of-scope nested offer: {uid}")
        pool = er if selector["kind"] == "recipe" else ordinary
        selected = [i for i, row in pool.items() if row["rarity"] in selector["rarities"]
                    and (selector["mercenary"] != "Shared" or row["mercenary"] == "Shared")
                    and ("recipe_kinds" not in selector or row["kind"] in selector["recipe_kinds"])]
        require(selected, f"Empty nested selector {uid}")
        nested[uid] = {**selector, "candidate_ids": sorted(selected)}
        dependencies.setdefault("upgrade:" + uid, set()).update(selector["kind"] + ":" + i for i in selected)

    nodes = {}

    def node(key, data, deps=()):
        nodes[key] = {**data, "requires": sorted(set(deps) | dependencies.get(key, set()))}

    for rid, r in er.items():
        refs = ["status:" + sid for sid in matching_statuses(r["effect"], statuses)]
        node("recipe:" + rid, {"kind": "recipe", "source_id": rid, "source": r["source"], "operation": "recipe:" + rid}, refs)
    for uid, u in eu.items():
        refs = ["status:" + sid for sid in matching_statuses(u["effect"], statuses)]
        node("upgrade:" + uid, {"kind": "upgrade", "source_id": uid, "source": u["source"], "operation": "upgrade:" + uid}, refs)
    for pid, p in parts.items():
        # An original physical part can have future output hooks. Referencing
        # its recipe carries that complete dependency, including copies.
        deps = ["recipe:" + p["canonical_recipe"]] if not p["ordinary_generated"] else []
        node("part:" + pid, {"kind": "part", "operation": "part:" + pid}, deps)
    for rid in sorted(enabled_robots):
        a = authored["robot_annotations"][rid]
        refs = ["status:" + i for i in a["status_ids"]]
        refs += ["robot:" + s["robot_id"] for s in a["summons"]]
        node("robot:" + rid, {"kind": "robot", "source_id": rid, "source": robots[rid]["source"], "operation": "robot:" + rid}, refs)
    enabled_statuses = sorted({dep[7:] for n in nodes.values() for dep in n["requires"] if dep.startswith("status:")})
    for sid in enabled_statuses:
        require(sid in statuses, f"Unknown status {sid}")
        node("status:" + sid, {"kind": "status", "source_id": sid, "source": statuses[sid]["source"], "operation": "status:" + sid})
    for f in formations:
        node("formation:" + f["id"], {"kind": "formation", "operation": "formation:" + f["id"]}, ["robot:" + b["robot_id"] for b in f["bodies"]])
    for mystery in authored["mysteries"]:
        if mystery["category"] == "Regular combat":
            deps = ["formation:" + f["id"] for f in formations if f["category"] == "Regular"]
        elif mystery["category"] == "Merchant":
            deps = ["recipe:" + i for i in er if er[i]["rarity"] in ("Uncommon", "Rare")]
            deps += ["upgrade:" + i for i in upgrade_pools["Shop"]["ids"] if eu[i]["rarity"] in ("Uncommon", "Rare")]
        else:
            deps = ["upgrade:" + i for i in upgrade_pools["Mystery"]["ids"]]
        node("mystery:" + mystery["id"], {"kind": "mystery", "operation": "mystery:" + mystery["id"]}, deps)
    node("ability:hot-barrel", {"kind": "ability", "operation": "ability:hot-barrel", "tuning": beta["player"]["Mara"]})
    node("route:" + route["id"], {"kind": "route", "operation": "route:" + route["id"]},
         ["formation:" + f["id"] for f in formations] + ["mystery:" + m["id"] for m in authored["mysteries"]])
    roots = {"ability:hot-barrel", "route:" + route["id"]}
    starters = sorted(i for i, r in er.items() if r["rarity"] == "Base")
    roots.update("recipe:" + i for i in starters)
    for pool in recipe_pools.values():
        roots.update("recipe:" + i for i in pool["ids"])
    for pool in list(upgrade_pools.values()) + list(mayor_pools.values()):
        roots.update("upgrade:" + i for i in pool["ids"])
    reachable = closure(nodes, roots)
    require(reachable == set(nodes), "Compiled a node that cannot be reached from an offer, starter or route")
    operations = sorted(set(CORE_OPERATIONS) | {n["operation"] for n in nodes.values()})
    manifest = {
        "schema_version": 1, "content_version": authored["content_version"], "baseline_revision": "dcb72a6662559da466e6cb4029b648e210f52c76",
        "source_sha256": source_hashes, "catalogue_snapshot_sha256": digest(encoded(catalogue)),
        "scope": authored["scope"], "balance_profile": beta["profile_id"],
        "eligibility_notice": "This is the final eligible-content boundary. Inclusion is not implementation, balance, visual approval or fun evidence. Runtime support is separate and never changes these pools.",
        "player": beta["player"]["Mara"], "campaign": beta["campaign"], "gathering": beta["gathering"], "shop": beta["shop"],
        "starter_recipe_ids": starters, "eligible_recipe_ids": sorted(er), "eligible_upgrade_ids": sorted(eu),
        "ordinary_upgrade_ids": sorted(ordinary), "mayor_upgrade_ids": sorted(mayor),
        "robot_ids": sorted(enabled_robots), "status_ids": enabled_statuses,
        "recipe_pools": recipe_pools, "upgrade_pools": upgrade_pools, "mayor_pools": mayor_pools,
        "offer_rules": {
            "base": beta["offer_rule"],
            "upgrade_ownership": "Exclude ever-acquired IDs, including expired equipment; source, city and mercenary filters precede rarity. Validate literal payment/selection requirements before showing an offer.",
            "mayor": "Exactly one shared infrastructure, one shared tactical and one Mara candidate. Select one gift. No arbitrary cross-lane draw or normal-pool Mayor leakage.",
            "recipe_memory": "Owned IDs remain eligible. Each accepted exact copy takes one slot and has independent cooldown/use state. Record all seen offers in Collection.",
            "exhaustion": authored["authored_tuning"]["ordinary_upgrade_exhaustion"],
        },
        "route": route, "position_gates": position_gates, "formations": formations,
        "robot_annotations": authored["robot_annotations"], "mysteries": authored["mysteries"],
        "rivet_mite_placement": beta["enemy"]["rivet_mite_placement"],
        "core_values": {i: beta["enemy"]["core_credit_values_by_roster_id"][i] for i in sorted(enabled_robots)},
        "parts": parts, "outputs": outputs, "copy_selectors": copy_selectors, "nested_offer_selectors": nested,
        "part_identity_rules": annotations["identity_rules"], "authored_tuning": authored["authored_tuning"],
        "dependency_roots": sorted(roots), "dependencies": nodes, "required_runtime_operations": operations,
        "excluded_source_ids": {
            "recipes": sorted(set(recipes) - set(er)), "upgrades": sorted(set(upgrades) - set(eu)),
            "robots": sorted(set(robots) - enabled_robots),
            "reason": "Other mercenaries, later city tiers, or alternate roster/boss entries outside the existing Cinderwall registry. Preserved in the full source snapshot; not deleted or permanently locked.",
        },
    }
    manifest["counts"] = {
        "source_recipes": len(recipes), "source_upgrades": len(upgrades), "source_robots": len(robots),
        "eligible_recipes": len(er), "starters": len(starters), "ordinary_upgrades": len(ordinary),
        "mayor_gifts": len(mayor), "robots": len(enabled_robots), "formations": len(formations),
        "mysteries": len(authored["mysteries"]), "statuses": len(enabled_statuses), "physical_part_types": len(parts),
        "runtime_operations_required": len(operations),
    }
    validate_manifest(manifest, catalogue)
    return catalogue, manifest


def closure(nodes, roots):
    reached = set()
    queue = list(roots)
    while queue:
        identifier = queue.pop()
        require(identifier in nodes, f"Unknown dependency: {identifier}")
        if identifier not in reached:
            reached.add(identifier)
            queue.extend(nodes[identifier]["requires"])
    return reached


def validate_manifest(m, c):
    require(m["balance_profile"] == "beta-balance-v1.1", "Incorrect beta input version")
    require(m["scope"]["mercenary"] == "Mara" and m["scope"]["city_tier"] == 1, "Incorrect MVP boundary")
    require(m["scope"]["city_victory"]["profile_unlock"] == "Ivo", "Mara city clear must preserve Ivo's unlock")
    expected_recipes = {i for i, r in c["recipes"].items() if r["mercenary"] in ("Shared", "Mara")}
    expected_upgrades = {i for i, u in c["upgrades"].items() if allowed_upgrade(u)}
    require(set(m["eligible_recipe_ids"]) == expected_recipes, "Eligible recipe pool was silently trimmed or broadened")
    require(set(m["eligible_upgrade_ids"]) == expected_upgrades, "Eligible upgrade pool was silently trimmed or broadened")
    require(set(m["starter_recipe_ids"]) == {f"SH{n:03}" for n in range(1, 9)} | {f"MA{n:03}" for n in range(1, 5)}, "Starter set changed")
    require(m["player"]["start_hp"] == 80 and m["campaign"]["start_credits"] == 100, "Selected initial tuning changed without a new profile")
    require(sum(m["gathering"]["foundation_by_city"]["1"]) + m["gathering"]["steered_units_of_one_selected_material"] == 10, "Expected ten-material steered haul")
    for source, pool in m["recipe_pools"].items():
        expected = {i for i in expected_recipes if pool["rarity_weights"].get(c["recipes"][i]["rarity"], 0) > 0}
        require(set(pool["ids"]) == expected, f"Incomplete or broadened {source} recipe offer pool")
        require(len(pool["ids"]) == len(set(pool["ids"])), f"Duplicate recipe IDs in {source}")
        for rid in pool["ids"]:
            r = c["recipes"][rid]
            require(rid in expected_recipes and pool["rarity_weights"].get(r["rarity"], 0) > 0, f"Illegal {source} recipe {rid}")
    require(not any(c["recipes"][i]["rarity"] == "Legendary" for i in m["recipe_pools"]["Regular"]["ids"]), "Regular Legendary leakage")
    for source, pool in m["upgrade_pools"].items():
        expected = {i for i in expected_upgrades if source in c["upgrades"][i]["acquisition"] and pool["rarity_weights"].get(c["upgrades"][i]["rarity"], 0) > 0}
        require(set(pool["ids"]) == expected, f"Incomplete or broadened {source} upgrade offer pool")
        for uid in pool["ids"]:
            u = c["upgrades"][uid]
            require(uid in expected_upgrades and source in u["acquisition"] and "Mayor" not in u["acquisition"], f"Illegal {source} upgrade {uid}")
    for lane, pool in m["mayor_pools"].items():
        expected = {i for i in expected_upgrades if "Mayor" in c["upgrades"][i]["acquisition"] and c["upgrades"][i]["mayor_lane"] == lane}
        require(set(pool["ids"]) == expected, f"Incomplete or broadened {lane} Mayor pool")
    require({c["upgrades"][i]["mercenary"] for i in m["mayor_pools"]["mercenary"]["ids"]} == {"Mara"}, "Incorrect mercenary Mayor lane")
    require(all(pool["ids"] for pool in m["mayor_pools"].values()), "Empty Mayor lane")
    require(set(m["dependencies"]) == closure(m["dependencies"], m["dependency_roots"]), "Unreachable dependency node")
    for f in m["formations"]:
        mites = [b for b in f["bodies"] if b["robot_id"] == "C1-R01"]
        require(len(mites) <= 1, f"Multiple live Mites in {f['id']}")
        require(not mites or any(b["hp"] > mites[0]["hp"] for b in f["bodies"]), f"Mite lacks stronger initial companion in {f['id']}")
        require(all(b["robot_id"] in m["robot_ids"] for b in f["bodies"]), f"Unknown formation body {f['id']}")
    require(m["rivet_mite_placement"]["max_alive_per_encounter"] == 1, "Single-Mite limit missing")
    for rid, annotation in m["robot_annotations"].items():
        for summon in annotation["summons"]:
            require(summon["max_alive"] == 1 and summon["robot_id"] == "C1-R01", f"Illegal Cinderwall summon {rid}")
    require(m["robot_annotations"]["C1-R07"]["summons"][0]["lifetime_count"] == 4, "Coil Nest supply changed")
    require(m["robot_annotations"]["C1-O03"]["summons"][0]["lifetime_count"] == 1, "Split Chassis death-spawn changed")
    require(sum(m["core_values"]["C1-O03"]) + sum(m["core_values"]["C1-R01"]) == 40, "Split Chassis core allocation changed")
    require(len(m["position_gates"]) == 12, "Twelve encounter fixture incomplete")
    for pos, gate in m["position_gates"].items():
        n = int(pos)
        if n == 12:
            require(gate == {"Regular": [], "Officer": [], "Mystery": [], "Boss": ["C1-F-GATEBREAKER"]}, "Boss must be sole encounter")
        else:
            require(len(gate["Regular"]) >= 3 and not gate["Boss"], f"No complete ordinary choice at {n}")
        if n <= 3:
            require(not gate["Officer"] and not gate["Mystery"], "Opening gate leaked special encounter")
        if n == 11:
            require(not gate["Officer"] and "C1-F-NEST" not in gate["Regular"], "Approach gate leaked pressure/Officer content")
    officer_schedules = [p for p in itertools.combinations(range(4, 11), 3) if all(b - a > 1 for a, b in zip(p, p[1:]))]
    mystery_schedules = list(itertools.combinations(range(4, 12), 3))
    for officers, mysteries in itertools.product(officer_schedules, mystery_schedules):
        for position in range(1, 12):
            categories = ["Regular"]
            if position in officers:
                categories.append("Officer")
            if position in mysteries:
                categories.append("Mystery")
            categories = ["Regular"] * (3 - len(categories)) + categories
            require(categories in m["route"]["legal_ordinary_offer_sets"], "Schedule cannot produce three legal offers")
            require(all(m["position_gates"][str(position)][category] for category in categories), "Scheduled category has no legal content")
    require(sum(x["weight"] for x in m["mysteries"]) == 100, "Mystery weights must total 100")
    require({x["category"]: x["weight"] for x in m["mysteries"]} == m["campaign"]["mystery_outcome_weights"], "Mystery category weights changed")
    for source, outputs in m["outputs"].items():
        require(source in m["dependencies"], f"Unknown output source {source}")
        for output in outputs:
            require(output["part_id"] in m["parts"], f"Unknown generated part {output['part_id']}")
    for p in m["parts"].values():
        require(p["canonical_recipe"] in expected_recipes and p["resale"], f"Unpriced or unknown part {p['id']}")
        require(p["kind"] != "Utility", "A Utility activation became an inventory part")
    expected_ops = set(CORE_OPERATIONS) | {node["operation"] for node in m["dependencies"].values()}
    require(set(m["required_runtime_operations"]) == expected_ops, "Runtime gate does not cover full closure")
    return {"exhaustive_category_schedules": len(officer_schedules) * len(mystery_schedules), "offer_states_checked": len(officer_schedules) * len(mystery_schedules) * 11}


def validate_release(manifest, support_path):
    support = json.loads(support_path.read_text(encoding="utf-8"))
    bindings = support.get("bindings", {})
    missing = sorted(set(manifest["required_runtime_operations"]) - set(bindings))
    require(not missing, f"Unsupported release operations: {len(missing)}; first: {', '.join(missing[:8])}")
    expected_hash = digest(encoded(manifest))
    require(support.get("manifest_sha256") == expected_hash, "Runtime support does not pin this manifest SHA-256")
    require(bool(support.get("verified_build")), "Runtime support has no verified shared-core build")
    for operation in manifest["required_runtime_operations"]:
        b = bindings[operation]
        require(bool(b.get("handler")) and bool(b.get("tests")), f"No handler/fixture IDs for {operation}")
        for field in ("implementation", "evidence"):
            record = b.get(field, {})
            path = (GAME / record.get("path", "")).resolve()
            require(path.is_relative_to(GAME.resolve()) and path.is_file(), f"Missing game-local {field} for {operation}")
            require(record.get("sha256") == digest(path.read_bytes()), f"Stale {field} for {operation}")
        evidence = json.loads((GAME / b["evidence"]["path"]).read_text(encoding="utf-8"))
        require(evidence.get("verified_build") == support["verified_build"], f"Evidence/build mismatch: {operation}")
        require(evidence.get("manifest_sha256") == expected_hash, f"Evidence/content mismatch: {operation}")
        passed = {t["id"] for t in evidence.get("tests", []) if t.get("result") == "passed"}
        require(set(b["tests"]).issubset(passed), f"Unpassed fixtures: {operation}")
    return len(bindings)


def self_test(manifest, catalogue):
    probes = []
    bad = copy.deepcopy(manifest)
    bad["eligible_recipe_ids"].remove("MA120")
    probes.append(("trimmed eligible pool", bad))
    bad = copy.deepcopy(manifest)
    bad["recipe_pools"]["Shop"]["ids"].remove("MA120")
    probes.append(("trimmed shop offer pool", bad))
    bad = copy.deepcopy(manifest)
    bad["dependencies"]["recipe:SH001"]["requires"].append("part:unregistered")
    probes.append(("unknown nested output", bad))
    bad = copy.deepcopy(manifest)
    bad["formations"][0]["bodies"].append({"robot_id": "C1-R01", "hp": 7})
    probes.append(("paired Mites", bad))
    bad = copy.deepcopy(manifest)
    bad["recipe_pools"]["Regular"]["ids"].append("SH109")
    probes.append(("Regular Legendary leakage", bad))
    bad = copy.deepcopy(manifest)
    bad["upgrade_pools"]["Shop"]["ids"].append("MY1-03")
    probes.append(("Mayor shop leakage", bad))
    bad = copy.deepcopy(manifest)
    bad["position_gates"]["12"]["Regular"].append("C1-F-RAM")
    probes.append(("ordinary boss alternative", bad))
    bad = copy.deepcopy(manifest)
    bad["parts"][next(iter(bad["parts"]))]["resale"] = {}
    probes.append(("unpriced physical output", bad))
    bad = copy.deepcopy(manifest)
    bad["required_runtime_operations"].remove("recipe:MA120")
    probes.append(("missing required runtime binding", bad))
    passed = []
    for name, fixture in probes:
        try:
            validate_manifest(fixture, catalogue)
        except InvalidContent:
            passed.append(name)
        else:
            raise InvalidContent("Validator accepted negative fixture: " + name)
    return passed


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true", help="Fail if checked-in generated data is missing/stale; do not write")
    parser.add_argument("--release", action="store_true", help="Require all runtime operations, source files and passed-fixture evidence")
    parser.add_argument("--runtime-support", type=Path, default=CONTENT / "runtime-support.json")
    parser.add_argument("--self-test", action="store_true", help="Run structural negative fixtures; no combat simulation")
    args = parser.parse_args()
    try:
        catalogue, manifest = compile_all()
        payloads = (catalogue, manifest)
        for name, payload in zip(ARTIFACTS, payloads):
            path = CONTENT / name
            data = encoded(payload)
            if args.check or args.release:
                require(path.is_file() and path.read_bytes() == data, f"Stale generated artifact: {path.relative_to(GAME)}; run compiler without --check")
            else:
                path.write_bytes(data)
        checks = validate_manifest(manifest, catalogue)
        result = {"result": "structural content checks passed", "manifest_sha256": digest(encoded(manifest)),
                  "counts": manifest["counts"], "checks": checks,
                  "runtime_support_claim": "None from these checks; use --release with verified shared-core evidence."}
        if args.self_test:
            result["negative_fixtures_rejected"] = self_test(manifest, catalogue)
        if args.release:
            result["verified_runtime_bindings"] = validate_release(manifest, args.runtime_support)
        print(json.dumps(result, ensure_ascii=False, indent=2))
        return 0
    except (InvalidContent, OSError, ValueError, KeyError) as error:
        print(f"CONTENT VALIDATION FAILED: {error}", file=sys.stderr)
        return 1


if __name__ == "__main__":
    raise SystemExit(main())
