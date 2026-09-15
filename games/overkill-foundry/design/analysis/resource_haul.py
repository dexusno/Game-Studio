"""Reproduce the printed-cost audit; this is not a combat/balance simulation.

Run from any directory with Python 3; writes JSON to stdout only.
Ignores effect grants, extra effect costs, cooling/reuse, shots and combat outcomes.
Each starter recipe may appear at most once in the subset affordability probe.
"""

import hashlib
import itertools
import json
import re
import statistics
from collections import Counter
from pathlib import Path


MATERIALS = ("Iron", "Copper", "Carbon", "Glass", "Circuit")
RARITIES = ("Base", "Common", "Uncommon", "Rare", "Legendary")
CHARACTERS = {"MA": "Mara", "IV": "Ivo", "AD": "Ada", "NO": "Noor"}
CATALOGUE = Path(__file__).resolve().parents[1] / "RECIPE-CATALOGUE.md"


def read_recipes():
    rows, source_lines = [], []
    rarity = None
    for line in CATALOGUE.read_text(encoding="utf-8").splitlines():
        if line in {"### " + value for value in RARITIES}:
            rarity = line[4:]
        if not re.match(r"^\| (SH|MA|IV|AD|NO)\d{3} \|", line):
            continue
        cells = [cell.strip() for cell in line.strip("|").split("|")]
        assert len(cells) == 7, cells
        pairs = re.findall(r"(\d+) (Iron|Copper|Carbon|Glass|Circuit)", cells[4])
        assert " + ".join(n + " " + m for n, m in pairs) == cells[4], cells[0]
        assert len({m for _, m in pairs}) == len(pairs), cells[0]
        costs = {m: int(n) for n, m in pairs}
        assert rarity in RARITIES
        rows.append({
            "id": cells[0], "name": cells[1], "kind": cells[3], "rarity": rarity,
            "cost": tuple(costs.get(m, 0) for m in MATERIALS),
            "total": sum(costs.values()), "cooldown": cells[5], "effect": cells[6],
        })
        source_lines.append(line)
    assert len(rows) == len({r["id"] for r in rows}) == 606
    digest = hashlib.sha256("\n".join(source_lines).encode("utf-8")).hexdigest()
    return rows, digest


def summarize(rows):
    amounts = [r["total"] for r in rows]
    costs = [sum(r["cost"][i] for r in rows) for i in range(5)]
    return {
        "recipes": len(rows), "mean_units": round(statistics.mean(amounts), 3),
        "median_units": statistics.median(amounts), "min_units": min(amounts),
        "max_units": max(amounts), "cost_histogram": dict(sorted(Counter(amounts).items())),
        "material_units": dict(zip(MATERIALS, costs)),
        "material_share_percent": {m: round(100 * n / sum(costs), 2) for m, n in zip(MATERIALS, costs)},
        "requires_circuit": sum(r["cost"][4] > 0 for r in rows),
    }


def cost_sum(rows):
    return tuple(sum(r["cost"][i] for r in rows) for i in range(5))


def affordable(cost, supply):
    return all(c <= s for c, s in zip(cost, supply))


def starter_subsets(rows):
    subsets = []
    for flags in itertools.product((False, True), repeat=len(rows)):
        selected = [r for r, flag in zip(rows, flags) if flag]
        # Having these kinds does not prove their effects are usable or sufficient.
        kinds = {r["kind"] for r in selected}
        subsets.append((cost_sum(selected), len(selected), "Ammo" in kinds and "Shield" in kinds))
    return subsets


def main():
    rows, digest = read_recipes()
    by_id = {r["id"]: r for r in rows}
    shared = [by_id[f"SH{n:03}"] for n in range(1, 9)]
    kits = {prefix: shared + [by_id[f"{prefix}{n:03}"] for n in range(1, 5)] for prefix in CHARACTERS}
    assert all(len(kit) == 12 for kit in kits.values())
    # Explicit hypothetical supply mappings for sensitivity analysis, not owner decisions.
    # Each adds two units of one chosen material to the fixed basket.
    fixed_baskets = {6: (1, 1, 1, 1, 0), 8: (2, 2, 1, 1, 0),
                     10: (3, 2, 1, 1, 1), 12: (4, 3, 1, 1, 1)}
    probe = {}
    for prefix, kit in kits.items():
        subsets = starter_subsets(kit)
        eligible = [r for r in rows if r["id"].startswith(("SH", prefix))]
        low_tier = [r for r in eligible if r["rarity"] in ("Base", "Common")]
        character_result = {}
        for total, fixed in fixed_baskets.items():
            assert sum(fixed) + 2 == total
            choices = {}
            for i, material in enumerate(MATERIALS):
                supply = tuple(n + (2 if j == i else 0) for j, n in enumerate(fixed))
                feasible = [(cost, count, both) for cost, count, both in subsets if affordable(cost, supply)]
                choices[material] = {
                    "supply": dict(zip(MATERIALS, supply)),
                    "max_distinct_recipe_uses_cost_only": max(count for _, count, _ in feasible),
                    "attack_and_shield_subsets_cost_only": sum(both for _, _, both in feasible),
                    "max_printed_spend_cost_only": max(sum(cost) for cost, _, _ in feasible),
                    "individually_affordable_starter_recipes": sum(affordable(r["cost"], supply) for r in kit),
                    "individually_affordable_base_common_recipes": sum(affordable(r["cost"], supply) for r in low_tier),
                    "base_common_recipe_denominator": len(low_tier),
                }
            character_result[total] = choices
        probe[CHARACTERS[prefix]] = character_result
    bundles = {
        "basic_attack_and_defence": ("SH001", "SH002", "SH003"),
        "strong_shared_attack_and_defence": ("SH001", "SH002", "SH003", "SH004", "SH006"),
        "shared_attack_defence_and_haul_investment": ("SH001", "SH002", "SH003", "SH004", "SH008"),
        "shared_attack_defence_and_cooling": ("SH001", "SH002", "SH003", "SH004", "SH007"),
        "mara_attack_defence_and_heat": ("SH001", "SH002", "SH003", "MA001", "MA002"),
        "ivo_attack_defence_and_mark": ("SH001", "SH002", "SH003", "IV001", "IV003"),
        "ada_attack_defence_and_repair": ("SH001", "SH002", "SH003", "AD001", "AD004"),
        "noor_attack_defence_and_charge": ("SH001", "SH002", "SH003", "NO001", "NO002"),
    }
    result = {
        "catalogue_recipe_rows_sha256": digest,
        "scope": "Printed costs only, equal weight per recipe. No inference of reward rates, effect legality, win rate or fun.",
        "overall": summarize(rows),
        "by_rarity": {tier: summarize([r for r in rows if r["rarity"] == tier]) for tier in RARITIES},
        "by_kind": {kind: summarize([r for r in rows if r["kind"] == kind]) for kind in sorted({r["kind"] for r in rows})},
        "eligible_pool_demand": {CHARACTERS[p]: summarize([r for r in rows if r["id"].startswith(("SH", p))]) for p in CHARACTERS},
        "starter_costs": {CHARACTERS[p]: summarize(kit) for p, kit in kits.items()},
        "hypothetical_supply_probe": probe,
        "example_bundles": {name: {"ids": ids, "cost": dict(zip(MATERIALS, cost_sum([by_id[i] for i in ids])))} for name, ids in bundles.items()},
    }
    print(json.dumps(result, indent=2))


if __name__ == "__main__":
    main()
