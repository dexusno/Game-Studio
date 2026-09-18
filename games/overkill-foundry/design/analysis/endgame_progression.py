"""Validate endgame design data and reproduce a cost-only starter sensitivity probe.

Not a combat engine, achievement evaluator or Steam integration.
"""
from __future__ import annotations

import argparse
from collections import Counter
import json
from pathlib import Path

from resource_haul import MATERIALS, CHARACTERS, read_recipes, starter_subsets, affordable

DESIGN = Path(__file__).resolve().parents[1]


def read_json(name: str) -> dict:
    return json.loads((DESIGN / name).read_text(encoding="utf-8-sig"))


def maximum_unlocked(best: dict[str, int], merc: str, gate: str) -> int:
    """Reference calculation for the design's profile-only tier gate."""
    eligible = all(v >= 0 for v in best.values()) if gate == "all_four_base_routes" else best[merc] >= 0
    return min(10, best[merc] + 1) if eligible else 0


def record_route_clear(best: dict[str, int], merc: str, tier: int, gate: str) -> dict[str, int]:
    if not 0 <= tier <= maximum_unlocked(best, merc, gate):
        raise ValueError("Tier is not unlocked for this mercenary")
    return {**best, merc: max(best[merc], tier)}


def validate(achievements: dict, progression: dict) -> dict:
    rows = achievements["achievements"]
    ids = [r["api_name"] for r in rows]
    assert len(rows) == len(set(ids)) == 40
    assert len({r["name"] for r in rows}) == 40
    required = ["api_name", "name", "description", "event", "condition", "commit_scope", "hidden", "status"]
    for r in rows:
        assert all(k in r for k in required), r
        assert r["status"] == "proposal" and r["api_name"].startswith("OF_")
        assert r["condition"].get("type") and isinstance(r["hidden"], bool)
        assert r["commit_scope"] in ["encounter", "city", "campaign", "profile", "account"]
    meta = achievements["launch_manifests"]["launch_achievements_v1"]["api_names"]
    assert len(meta) == len(set(meta)) == 39 and set(meta) == set(ids) - {"OF_ALL_ACHIEVEMENTS"}
    recipe_rows, recipe_hash = read_recipes()
    upgrade_ids = {r["id"] for f in ["data/shared-upgrades.json", "data/character-mayor-upgrades.json"] for r in read_json(f)["upgrades"]}
    for manifest, expected in [("launch_recipes_v1", {r["id"] for r in recipe_rows}), ("launch_upgrades_v1", upgrade_ids)]:
        ref = achievements["launch_manifests"][manifest]
        assert len(ref["ids"]) == len(set(ref["ids"])) == ref["draft_count"]
        assert set(ref["ids"]) == expected
    tiers = progression["tiers"]
    assert [r["tier"] for r in tiers] == list(range(11))
    assert progression["maximum_tier"] == 10
    assert progression["entry_gate"]["mode"] in ["all_four_base_routes", "per_mercenary_base_route"]
    for previous, current in zip(tiers, tiers[1:]):
        assert current["ordinary_haul_reduction"] >= previous["ordinary_haul_reduction"]
        assert 0 < current["base_mercenary_hp_percent"] <= previous["base_mercenary_hp_percent"]
        assert current["enemy_base_hp_percent"] >= previous["enemy_base_hp_percent"]
        assert current["enemy_attack_hit_bonus"] >= previous["enemy_attack_hit_bonus"]
        fields = ["ordinary_haul_reduction", "base_mercenary_hp_percent", "enemy_base_hp_percent", "enemy_attack_hit_bonus"]
        assert any(current[k] != previous[k] for k in fields)
    mercs = progression["mercenaries"]
    best = dict.fromkeys(mercs, -1)
    gate = progression["entry_gate"]["mode"]
    assert all(maximum_unlocked(best, m, gate) == 0 for m in mercs)
    best = record_route_clear(best, "Mara", 0, gate)
    assert maximum_unlocked(best, "Ivo", gate) == 0
    assert maximum_unlocked(best, "Mara", "all_four_base_routes") == 0
    assert maximum_unlocked(best, "Mara", "per_mercenary_base_route") == 1
    for merc in mercs[1:]:
        best = record_route_clear(best, merc, 0, gate)
    assert all(maximum_unlocked(best, m, gate) == 1 for m in mercs)
    try:
        record_route_clear(best, "Mara", 2, gate)
    except ValueError:
        pass
    else:
        raise AssertionError("Tier skip was accepted")
    best = record_route_clear(best, "Mara", 1, gate)
    assert maximum_unlocked(best, "Mara", gate) == 2 and maximum_unlocked(best, "Noor", gate) == 1
    assert record_route_clear(best, "Mara", 0, gate) == best
    assert record_route_clear(best, "Mara", 1, gate) == best
    threshold = progression["core_unlock"]["all_mercenary_clear_tier"]
    assert 1 <= threshold <= progression["maximum_tier"]
    before_core = dict.fromkeys(mercs, threshold)
    before_core["Noor"] = threshold - 1
    assert not all(v >= threshold for v in before_core.values())
    after_core = record_route_clear(before_core, "Noor", threshold, gate)
    assert all(v >= threshold for v in after_core.values())
    assert maximum_unlocked(dict.fromkeys(mercs, 10), "Mara", gate) == 10
    assert maximum_unlocked(dict.fromkeys(mercs, -1), "Mara", gate) == 0
    return {"achievements": len(rows), "groups": dict(Counter(r["group"] for r in rows)),
            "tiers_above_base": 10, "entry_gate": gate, "core_unlock_tier_all_mercenaries": threshold,
            "minimum_route_wins_to_reveal": len(mercs) * (threshold + 1),
            "recipe_rows": len(recipe_rows), "recipe_sha256": recipe_hash,
            "upgrade_ids": len(upgrade_ids), "reference_gate_cases": "passed"}


def starter_probe(progression: dict) -> list[dict]:
    rows, _ = read_recipes()
    by_id = {r["id"]: r for r in rows}
    shared = [by_id[f"SH{n:03}"] for n in range(1, 9)]
    fixture = progression["beta_fixture"]
    results = []
    for prefix, merc in CHARACTERS.items():
        subsets = starter_subsets(shared + [by_id[f"{prefix}{n:03}"] for n in range(1, 5)])
        for reduction in range(4):
            baseline = list(fixture["base_haul"])
            for material in fixture["deduction_order"][:reduction]:
                baseline[MATERIALS.index(material)] -= 1
            assert all(v >= 1 for v in baseline)
            choices = {}
            for i, material in enumerate(MATERIALS):
                supply = tuple(n + (fixture["steered_units"] if j == i else 0) for j, n in enumerate(baseline))
                feasible = [s for s in subsets if affordable(s[0], supply)]
                choices[material] = {"supply": dict(zip(MATERIALS, supply)),
                                     "maximum_distinct_uses": max(s[1] for s in feasible),
                                     "ammo_and_shield_subsets": sum(s[2] for s in feasible)}
            results.append({"mercenary": merc, "ordinary_haul": sum(baseline) + fixture["steered_units"],
                            "steering": choices})
    return results


def render_achievements(data: dict, report: dict) -> str:
    lines = ["# Achievement roster", "", "18 September 2026 · **40 proposed Steam achievements** · Original names and gameplay conditions", "",
             "See [Lockdown and the finale](ENDGAME-PROGRESSION.md), [Steam integration and exact event semantics](STEAM-ACHIEVEMENTS.md), "
             "and [source observations](research/2026-09-18-endgame-and-achievements.md). "
             "The [structured definitions](data/achievements.json) contain stable proposed API names, typed conditions, events and launch ID sets. "
             "This is a generated design document; no Steam achievements have been configured or unlocked.", "",
             "**Qualification:** use one profile wherever stated. Ordinary legitimate campaigns and the selected same-seed Continue behavior qualify. "
             "Combat feats commit at the fight's terminal result; explicitly victorious feats require survival. "
             "Collection discovery records visible offers, including declined offers. Debug-injected state does not qualify. "
             "Achievements never grant stronger recipes or modify another profile's gameplay progression.", "",
             "**Inspiration:** adapt the challenge roles described in the [STS2 achievement reference](https://slaythespire.wiki.gg/wiki/Slay_the_Spire_2:Achievements). "
             "Its wiki lists unfinished content, so this is not a parity claim with released Steam features. "
             "Our conditions use material crafting, installed Shield, saved parts and the four mercenaries' existing mechanics.", ""]
    for group in dict.fromkeys(r["group"] for r in data["achievements"]):
        lines += [f"## {group}", "", "| API name | Achievement | Player-facing requirement | Hidden |", "| --- | --- | --- | --- |"]
        for r in data["achievements"]:
            if r["group"] == group:
                lines.append(f"| `{r['api_name']}` | **{r['name']}** | {r['description']} | {'Yes' if r['hidden'] else 'No'} |")
        lines.append("")
    lines += ["## Collection and final-goal boundaries", "",
              "The draft launch collections contain 606 recipes and 288 upgrades. Their explicit ID sets must match shipping content before publication; "
              "unreleased/debug content is excluded and later additions do not silently move the target. "
              "The two full-collection goals are optional, potentially long-term achievements. Neither is required for CROWN-0. "
              "The last achievement requires the other 39 launch achievements; it is separate from the hidden finale victory achievement.", "",
              "**The Last Order** is the proposed special narrative achievement: defeat CROWN-0 after all four Tier 5 signatures. "
              "**Fourfold Shutdown** rewards repeating that finale with each mercenary. **No City Left Behind** covers all-four Tier 10 mastery. "
              "A distinct icon/crest can make the narrative reward special; Steam rarity is not an authored tier in this manifest.", "",
              "## Validation", "", f"The design check validated {report['achievements']} distinct API names, the 39-item nonrecursive meta set, "
              "the exact draft collection IDs, tier progression boundaries and the four-mercenary finale gate. "
              "It also produced a starter-cost sensitivity probe. None of these checks exercises a runtime achievement evaluator or Steam API.", ""]
    return "\n".join(lines).rstrip() + "\n"


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--write", action="store_true")
    args = parser.parse_args()
    achievements = read_json("data/achievements.json")
    progression = read_json("data/lockdown-tiers.json")
    report = validate(achievements, progression)
    report["starter_cost_probe"] = starter_probe(progression)
    report["scope"] = "Design arithmetic and printed-cost feasibility only. No effect grants, reuse, upgrades, enemy combat, healing economy, runtime achievement evaluation or Steam calls."
    if args.write:
        (DESIGN / "ACHIEVEMENTS.md").write_text(render_achievements(achievements, report), encoding="utf-8")
        (DESIGN / "analysis/endgame_progression.json").write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({k: v for k, v in report.items() if k != "starter_cost_probe"}, indent=2))
    print("\nCost-only starter probe: range across the five steering choices")
    for r in report["starter_cost_probe"]:
        counts = [v["ammo_and_shield_subsets"] for v in r["steering"].values()]
        print(f"{r['mercenary']:4} / {r['ordinary_haul']:2} materials: {min(counts)}-{max(counts)} Ammo+Shield subsets")


if __name__ == "__main__":
    main()
