"""Check beta-balance-v1 against authored data; no combat engine or win-rate model.

Run with --write to refresh the compact, deterministic JSON evidence beside this
script. Source pins deliberately require a reviewed profile update after content
changes. All currency arithmetic uses integers; means/probabilities are analysis.
"""

import argparse
import hashlib
import json
import re
from pathlib import Path

from resource_haul import CHARACTERS, MATERIALS, affordable, cost_sum, read_recipes, starter_subsets, summarize


DESIGN = Path(__file__).resolve().parents[1]
PROFILE = DESIGN / "data/beta-balance-v1.json"


def digest(text):
    return hashlib.sha256(text.encode("utf-8")).hexdigest()


def load_sources():
    rows, recipe_hash = read_recipes()
    enemy_lines = [line for line in (DESIGN / "ENEMY-ROSTER.md").read_text(encoding="utf-8").splitlines()
                   if re.match(r"\| C[123]-[ROB]\d\d ", line)]
    upgrades, hashes = [], {"recipe_rows": recipe_hash, "enemy_rows": digest("\n".join(enemy_lines))}
    for filename in ("shared-upgrades.json", "character-mayor-upgrades.json"):
        source = (DESIGN / "data" / filename).read_text(encoding="utf-8")
        data = json.loads(source)
        upgrades.extend(data["upgrades"])
        hashes[filename] = digest(source)
    return rows, enemy_lines, upgrades, hashes


def analyze(profile):
    rows, enemy_lines, upgrades, hashes = load_sources()
    assert hashes == profile["source_sha256"], "Content changed: review and version the tuning profile."
    by_id = {row["id"]: row for row in rows}
    upgrade_by_id = {row["id"]: row for row in upgrades}
    enemy_ids = {re.match(r"\| (C[123]-[ROB]\d\d)", line)[1] for line in enemy_lines}
    assert len(rows) == 606 and len(upgrade_by_id) == len(upgrades) == 288
    assert len(enemy_ids) == 42
    assert set(profile["enemy"]["core_credit_values_by_roster_id"]) == enemy_ids
    assert profile["materials"] == list(MATERIALS)
    shop = profile["shop"]
    prices = shop["material_unit_prices"]
    assert len(prices) == 5 and all(isinstance(n, int) and n > 0 for n in prices)
    assert all(n > 0 for values in profile["enemy"]["core_credit_values_by_roster_id"].values() for n in values)
    assert shop["upgrade_price_overrides"]["UGS-085"] > 100
    assert set(shop["upgrade_price_overrides"]).issubset(upgrade_by_id)
    recovery = by_id[shop["recovery_access"]["guaranteed_recipe"]]
    assert recovery["kind"] == "Utility" and recovery["rarity"] == "Uncommon"
    assert shop["recovery_access"]["price"] == shop["recipe_prices"][recovery["rarity"]]
    assert profile["campaign"]["start_credits"] >= shop["recovery_access"]["price"]
    rarities = profile["recipe_rarity_order"]
    assert rarities == ["Common", "Uncommon", "Rare", "Legendary"]
    for table_name in ("recipe_offer_weights_by_city", "upgrade_offer_weights_by_city"):
        for city, sources in profile[table_name].items():
            for source, weights in sources.items():
                assert len(weights) == 4 and sum(weights) == 100 and min(weights) >= 0
                if source == "Regular":
                    assert weights[3] == 0, "Regular fights cannot drop Legendary recipes."
                for prefix, name in CHARACTERS.items():
                    if table_name.startswith("recipe"):
                        eligible = [r for r in rows if r["id"].startswith(("SH", prefix))]
                    else:
                        eligible = [u for u in upgrades if u["mercenary"] in ("Shared", name)
                                    and source in u["acquisition"] and u["min_city"] <= int(city) <= u["max_city"]]
                    available_rarities = {r["rarity"] for r in eligible}
                    assert sum(weight for rarity, weight in zip(rarities, weights) if rarity in available_rarities) > 0
                    assert len({r["id"] for r in eligible if r["rarity"] in rarities
                                and weights[rarities.index(r["rarity"])] > 0}) >= 3
    for weights in profile["mayor_rare_legendary_weights_by_city"].values():
        assert sum(weights) == 100 and min(weights) >= 0
    assert sum(profile["campaign"]["mystery_outcome_weights"].values()) == 100

    foundation = profile["gathering"]["foundation_by_city"]["1"]
    steer = profile["gathering"]["steered_units_of_one_selected_material"]
    assert all(values == foundation for values in profile["gathering"]["foundation_by_city"].values())
    assert sum(foundation) + steer == 10 and all(n >= 1 for n in foundation)
    assert all(len(values) == 5 and min(values) > 0 for values in shop["material_stock_by_city"].values())
    shared = [by_id[f"SH{n:03}"] for n in range(1, 9)]
    kits = {prefix: shared + [by_id[f"{prefix}{n:03}"] for n in range(1, 5)] for prefix in CHARACTERS}
    affordability = {}
    for prefix, kit in kits.items():
        subsets = starter_subsets(kit)
        choices = {}
        for i, material in enumerate(MATERIALS):
            supply = [n + (steer if j == i else 0) for j, n in enumerate(foundation)]
            feasible = [(count, both) for cost, count, both in subsets if affordable(cost, supply)]
            choices[material] = {
                "supply": supply, "max_distinct_uses_cost_only": max(n for n, _ in feasible),
                "affordable_attack_and_shield_subsets_cost_only": sum(both for _, both in feasible),
            }
            assert any(both for _, both in feasible)
        affordability[CHARACTERS[prefix]] = choices

    weighted_recipe_costs = {}
    for city, sources in profile["recipe_offer_weights_by_city"].items():
        weighted_recipe_costs[city] = {}
        for prefix, name in CHARACTERS.items():
            eligible = [r for r in rows if r["id"].startswith(("SH", prefix))]
            means = {rarity: sum(r["total"] for r in eligible if r["rarity"] == rarity)
                     / sum(r["rarity"] == rarity for r in eligible) for rarity in rarities}
            weighted_recipe_costs[city][name] = {
                source: round(sum(means[r] * w for r, w in zip(rarities, weights)) / 100, 3)
                for source, weights in sources.items()
            }

    # This is a repeatable cost/protection witness, not simulated combat.
    # Two ordinary 7-HP Mites keep attacking for 5 each; player never attacks.
    defend = [by_id[x] for x in ("SH002", "SH006")]
    sell = [by_id[x] for x in ("SH001", "SH003", "SH004")]
    assert all(r["cooldown"] == "None" for r in defend + sell)
    supply = [n + (steer if m == "Glass" else 0) for m, n in zip(MATERIALS, foundation)]
    used = cost_sum(defend + sell)
    assert affordable(used, supply)
    sales = {r["id"]: sum(c * p for c, p in zip(r["cost"], prices)) // 2 for r in sell}
    stall = {
        "enemy": "two C1-R01 Rivet Mites, 7 HP each; 5 damage each per enemy phase",
        "steer": "Glass", "haul": supply,
        "shield_recipes": [r["id"] for r in defend], "shield": 10, "incoming_attack_total": 10,
        "sale_recipes": [r["id"] for r in sell], "whole_credit_sale_values": sales,
        "cost_each_round": list(used), "unspent_each_round": [s-c for s, c in zip(supply, used)],
        "credits_per_round": sum(sales.values()), "credits_after_20_rounds_excluding_start": 20 * sum(sales.values()),
        "conclusion": "Positive income, zero HP loss, no declining resource or use allowance: unbounded under current draft Mite behaviour. Overtime remains a pending design choice, not implemented by this calculation."
    }
    assert stall["credits_per_round"] == 8

    # Gross purchasing envelopes, expressly not sampled routes or combat results.
    economy = {}
    for city in ("1", "2", "3"):
        normal = profile["enemy"]["reference_regular_encounter_core_value_by_city"][city]
        officer = profile["enemy"]["reference_officer_encounter_core_value_by_city"][city]
        boss = profile["enemy"]["reference_boss_encounter_core_value_by_city"][city]
        economy[city] = {
            "11_reference_regulars_and_boss_core_value": 11*normal + boss,
            "8_reference_regulars_3_officers_and_boss_core_value": 8*normal + 3*officer + boss,
            "whole_material_stock_purchase_cost": sum(c*p for c, p in zip(shop["material_stock_by_city"][city], prices)),
        }

    boss_hp = {"1": [92, 100, 84], "2": [136, 132, 154], "3": [190, 180, 166]}
    boss_envelopes = {}
    for city, hp in boss_hp.items():
        damage = profile["playtest_targets_not_observations"]["effective_damage_per_player_turn_by_city"][city]
        boss_envelopes[city] = {"total_starting_body_hp": hp, "assumed_effective_damage_per_turn": damage,
                               "ceil_hp_over_assumed_damage": [(n+damage-1)//damage for n in hp]}

    ladder = profile["lockdown"]
    assert all(len(ladder[key]) == 11 for key in ("tier", "baseline_haul", "player_base_hp_percent", "enemy_hp_percent", "enemy_positive_attack_hit_addition"))
    tier_fixtures = []
    for tier in ladder["tier"]:
        basket = foundation.copy()
        for material in ladder["foundation_removal_order"][:10-ladder["baseline_haul"][tier]]:
            basket[MATERIALS.index(material)] -= 1
        assert min(basket) >= 1 and sum(basket)+steer == ladder["baseline_haul"][tier]
        tier_fixtures.append({"tier": tier, "foundation": basket, "steer": steer,
                              "starting_hp_all_mercenaries": 80 * ladder["player_base_hp_percent"][tier] // 100})

    return {
        "profile": profile["profile_id"], "profile_sha256": digest(PROFILE.read_text(encoding="utf-8")),
        "source_sha256": hashes, "scope": "Static source, cost, pricing and arithmetic checks. No fight/run execution, bot policy, win rate, confidence interval or graphical verification.",
        "counts": {"recipes": len(rows), "upgrades": len(upgrades), "robot_entries": len(enemy_ids), "starter_steering_fixtures": 20},
        "printed_costs_by_rarity": {r: summarize([row for row in rows if row["rarity"] == r]) for r in ("Base", *rarities)},
        "starter_affordability": affordability,
        "expected_material_cost_first_reward_slot_by_character": weighted_recipe_costs,
        "reward_cost_note": "Rarity-weighted uniform eligible recipe mean for the first slot, before removal within an offer or player selection. Not an effect-value or whole-turn damage estimate.",
        "ready_part_shop_prices": {entry["reference_recipe"]: sum(c*p for c,p in zip(by_id[entry["reference_recipe"]]["cost"], prices)) + shop["ready_part_markup_credits"] for entry in shop["ready_part_offers"]},
        "reference_city_economy": economy,
        "regular_route_reference_campaign_credits_including_start": profile["campaign"]["start_credits"] + sum(e["11_reference_regulars_and_boss_core_value"] for e in economy.values()),
        "economy_note": "Illustrative reference encounters, all cores sold, no escapes/Mysteries, purchases, leftover-part sales, or upgrade income. Actual formations have individual core values; this is not a guaranteed payout or empirical average.",
        "boss_hp_throughput_envelopes": boss_envelopes,
        "boss_note": "HP divided by assumed EFFECTIVE throughput, including initial companion HP. Armor, reactions, repairs, move order, status kills, forced downtime and target switching are not modeled; actual turn counts await the shared core.",
        "starter_stall_witness": stall,
        "tier_fixtures": tier_fixtures,
        "precision_expected_extra_materials_per_fight": {
            name: sum(weights[result] * bonus for result, bonus in profile["gathering"]["precision_bonus_selected_material"].items()) / 100
            for name, weights in profile["gathering"]["precision_models_for_future_headless_tests_percent"].items()
        },
        "economic_upgrade_correction": {"id": "UGS-017", "bonus_percent": 25,
                                        "basis": "Each sold energy core; excludes part sales and other direct grants",
                                        "core_sale_examples": {str(value): value + value * 25 // 100 for value in (5, 15, 25, 40, 100)},
                                        "note": "Current beta correction to an inert draft cash-reward multiplier. No new cash-drop rule. Whole-Credit floor per core makes grouped and separate sales equivalent."}
    }


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--write", action="store_true")
    args = parser.parse_args()
    profile = json.loads(PROFILE.read_text(encoding="utf-8"))
    result = analyze(profile)
    if args.write:
        Path(__file__).with_suffix(".json").write_text(json.dumps(result, indent=2) + "\n", encoding="utf-8")
    print(json.dumps({"profile": result["profile"], "checks": "passed", **result["counts"],
                      "reference_campaign_credits": result["regular_route_reference_campaign_credits_including_start"],
                      "starter_stall_credits_per_round": result["starter_stall_witness"]["credits_per_round"],
                      "runtime_tests": "not available"}, indent=2))


if __name__ == "__main__":
    main()
