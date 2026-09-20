"""Independent source/manifest checks; no combat or release-readiness claim."""
import json
from pathlib import Path

GAME = Path(__file__).resolve().parents[1]


def read(path):
    return json.loads((GAME / path).read_text(encoding="utf-8"))


manifest = read("content/cinderwall.manifest.json")
snapshot = read("content/catalogue.snapshot.json")
beta = read("design/data/beta-balance-v1.json")
checks = 0


def check(condition, label):
    global checks
    assert condition, label
    checks += 1
    print("PASS", label)


expected_recipes = {f"SH{i:03}" for i in range(1, 127)} | {f"MA{i:03}" for i in range(1, 121)}
check(set(manifest["eligible_recipe_ids"]) == expected_recipes, "all 246 Shared/Mara recipes remain eligible")
check(set(manifest["starter_recipe_ids"]) == {f"SH{i:03}" for i in range(1, 9)} | {f"MA{i:03}" for i in range(1, 5)}, "8 Shared + 4 Mara starters")
rows = (GAME / "design/RECIPE-CATALOGUE.md").read_text(encoding="utf-8").splitlines()
check(all(rows[r["source"]["line"] - 1] == r["source"]["exact_text"] for r in snapshot["recipes"].values()), "all 606 preserved recipe rows match actual source text and lines")
upgrades = {
    u["id"]: u
    for file in ("shared-upgrades.json", "character-mayor-upgrades.json")
    for u in read("design/data/" + file)["upgrades"]
    if u["mercenary"] in {"Shared", "Mara"} and u["min_city"] <= 1 <= u["max_city"]
}
check(set(manifest["eligible_upgrade_ids"]) == set(upgrades), "upgrade eligibility derives from source city and mercenary fields")
check(set(manifest["ordinary_upgrade_ids"]) == {i for i, u in upgrades.items() if "Mayor" not in u["acquisition"]}, "ordinary pool excludes Mayor gifts")
check(set(manifest["mayor_upgrade_ids"]) == {i for i, u in upgrades.items() if "Mayor" in u["acquisition"]}, "all eligible Mayor gifts retained")
rarities = ("Common", "Uncommon", "Rare", "Legendary")
for source, weights in beta["recipe_offer_weights_by_city"]["1"].items():
    allowed = {rarity for rarity, weight in zip(rarities, weights) if weight}
    expected = {i for i in expected_recipes if snapshot["recipes"][i]["rarity"] in allowed}
    check(set(manifest["recipe_pools"][source]["ids"]) == expected, source + " recipe pool matches beta source weights")
for source, weights in beta["upgrade_offer_weights_by_city"]["1"].items():
    allowed = {rarity for rarity, weight in zip(rarities, weights) if weight}
    expected = {i for i, u in upgrades.items() if source in u["acquisition"] and u["rarity"] in allowed}
    check(set(manifest["upgrade_pools"][source]["ids"]) == expected, source + " upgrade pool matches actual acquisition/rarity fields")
expected_robots = {"C1-R01", "C1-R02", "C1-R04", "C1-R05", "C1-R07", "C1-R08", "C1-O01", "C1-O02", "C1-O03", "C1-B01"}
check(set(manifest["robot_ids"]) == expected_robots, "Cinderwall registry and three Officers plus sole Gatebreaker boss")
opening = {"C1-F-MITE-RAM", "C1-F-RAM", "C1-F-CASK"}
middle = opening | {"C1-F-PRESS", "C1-F-BINDER"}
late = middle | {"C1-F-NEST"}
for position in range(1, 13):
    gate = manifest["position_gates"][str(position)]
    expected = opening if position <= 3 else middle if position <= 6 or position == 11 else late if position <= 10 else set()
    check(set(gate["Regular"]) == expected, f"position {position} Regular gate matches progression band")
    check(bool(gate["Officer"]) == (4 <= position <= 10) and bool(gate["Mystery"]) == (4 <= position <= 11), f"position {position} special-category gates")
check(manifest["position_gates"]["12"]["Boss"] == ["C1-F-GATEBREAKER"], "boss-only final encounter")
formations = manifest["formations"]
check(all(sum(b["robot_id"] == "C1-R01" for b in f["bodies"]) <= 1 for f in formations), "at most one initial Mite in every formation")
check(all(any(b["hp"] > 7 for b in f["bodies"] if b["robot_id"] != "C1-R01") for f in formations if any(b["robot_id"] == "C1-R01" for b in f["bodies"])), "initial Mites have stronger company")
for rid, total in (("C1-R07", 4), ("C1-O03", 1)):
    summon = manifest["robot_annotations"][rid]["summons"]
    check(len(summon) == 1 and summon[0]["robot_id"] == "C1-R01" and summon[0]["max_alive"] == 1 and summon[0]["lifetime_count"] == total, rid + " summon identity/live/lifetime limits")
check(sum(manifest["core_values"]["C1-O03"]) == 35 and sum(manifest["core_values"]["C1-R01"]) == 5, "chassis/death-spawn retain 35+5 core allocation")
check({o["part_id"] for o in manifest["outputs"]["recipe:SH026"]} == {"plain-shield-4", "plain-shield-6"}, "Folding Brace resolves to distinct 4/6 Shield output types")
for copier, generator, generated in (("SH102", "MA016", "plain-shield-5"), ("SH118", "SH096", "plain-shield-12")):
    check("recipe-" + generator in manifest["copy_selectors"]["recipe:" + copier]["candidate_part_ids"], copier + " includes eligible source generator part")
    check("recipe:" + generator in manifest["dependencies"]["part:recipe-" + generator]["requires"] and "part:" + generated in manifest["dependencies"]["recipe:" + generator]["requires"], copier + " copied source retains future-output closure")
check(all(p["resale"] and p["canonical_recipe"] in expected_recipes and p["kind"] != "Utility" for p in manifest["parts"].values()), "all physical outputs retain resale source; no Utility inventory object")
check({x["category"]: x["weight"] for x in manifest["mysteries"]} == {"Regular combat": 40, "Merchant": 30, "Tech support": 30}, "authored Mystery categories preserve 40/30/30 beta mix")
check(manifest["scope"]["city_victory"]["profile_unlock"] == "Ivo" and not manifest["scope"]["city_victory"]["full_three_city_campaign_clear"], "city completion preserves Ivo unlock without claiming full campaign clear")
print(f"{checks} independent content checks passed; structure only, no runtime support claim.")
