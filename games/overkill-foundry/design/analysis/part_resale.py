"""Check and render beta resale references; not a shop or combat implementation."""

import hashlib
import json
from pathlib import Path
import re


DESIGN = Path(__file__).resolve().parents[1]
DATA = DESIGN / "data/part-resale-bases.json"


def read_catalogue():
    rows = {}
    for line in (DESIGN / "RECIPE-CATALOGUE.md").read_text(encoding="utf-8").splitlines():
        if re.match(r"^\| (SH|MA|IV|AD|NO)\d{3} \|", line):
            cells = [c.strip() for c in line.strip("|").split("|")]
            rows[cells[0]] = {"name": cells[1], "kind": cells[3], "cost": {
                material: int(n) for n, material in
                re.findall(r"(\d+) (Iron|Copper|Carbon|Glass|Circuit)", cells[4])}}
    assert len(rows) == 606
    return rows


def materials_for(reference, registry, recipes, value=None):
    if reference.startswith("recipe:"):
        recipe = recipes[reference.split(":", 1)[1]]
        assert recipe["kind"] != "Utility", "Utility costs cannot price a generated part"
        return dict(recipe["cost"])
    if reference in registry["bases"]:
        return dict(registry["bases"][reference]["materials"])
    assert reference in {"family:plain-ammo", "family:plain-shield"}
    assert type(value) is int and value >= 0
    if value == 0:
        return {}
    result = {"Iron": (value + 5) // 6}
    if reference == "family:plain-shield":
        result["Copper"] = 1
    return result


def sale_price(reference, prices, registry, recipes, value=None):
    assert all(type(prices[m]) is int and prices[m] >= 0 for m in registry["materials"])
    ingredients = materials_for(reference, registry, recipes, value)
    return sum(n * prices[m] for m, n in ingredients.items()) // 2


def validate(registry, recipes):
    materials = set(registry["materials"])
    assert registry["new_craftable_recipes"] is False
    names = {}
    for key, base in registry["bases"].items():
        assert base["anchor_recipe"] in recipes
        assert base["materials"] and set(base["materials"]) <= materials
        assert all(type(n) is int and n > 0 for n in base["materials"].values())
        for parameter, family in (("fixed_damage", "plain-ammo"), ("fixed_shield", "plain-shield")):
            if parameter in base:
                assert base["materials"] == materials_for("family:" + family, registry, recipes, base[parameter])
    for rid, outputs in registry["recipe_outputs"].items():
        assert rid in recipes and outputs
        for output in outputs:
            assert type(output["quantity"]) is int and output["quantity"] > 0
            ref = output["basis"]
            if ref.startswith("recipe:"):
                assert ref == "recipe:" + rid
            if ref.startswith("family:"):
                assert output["value_parameter"] == "fixed_damage"
                for value in range(output["minimum"], output["maximum"] + 1):
                    assert set(materials_for(ref, registry, recipes, value)) <= materials
            else:
                assert set(materials_for(ref, registry, recipes)) <= materials
            if output["name"] in names:
                assert names[output["name"]] == ref, "Same output has conflicting prices"
            names[output["name"]] = ref
    assert materials_for("plain-ammo-6", registry, recipes) == recipes["SH001"]["cost"]
    assert materials_for("plain-shield-6", registry, recipes) == recipes["SH002"]["cost"]
    prices = dict.fromkeys(materials, 10)  # Illustration, not selected shop prices.
    assert sale_price("plain-ammo-6", prices, registry, recipes) == 5
    assert sale_price("recipe:SH001", prices, registry, recipes) == 5
    # Odd-price sale floors per part, not after pooling a batch.
    prices["Iron"] = 5
    each = sale_price("plain-ammo-4", prices, registry, recipes)
    assert each == 2 and 2 * each == 4
    # Different shop prices change the sale value without changing the reference.
    prices["Iron"] = 12
    assert sale_price("plain-ammo-6", prices, registry, recipes) == 6
    assert [materials_for("family:plain-ammo", registry, recipes, n)["Iron"]
            for n in (1, 6, 7, 12, 13, 14)] == [1, 1, 2, 2, 3, 3]
    for family in ("plain-ammo", "plain-shield"):
        assert materials_for("family:" + family, registry, recipes, 0) == {}
    return {"recipe_cases": len(registry["recipe_outputs"]),
            "standard_references": len(registry["bases"]),
            "output_descriptions": sum(map(len, registry["recipe_outputs"].values())),
            "checks": "Recipe/reference coverage, integer quantities, alias consistency, exact ordinary-part anchors, per-part floor, current prices and variable-output boundaries passed.",
            "limits": "No runtime sales, combat, income-loop search, campaign simulation or balance test."}


def material_text(materials):
    return " + ".join(f"{n} {m}" for m, n in materials.items()) or "0 materials"


def main():
    registry = json.loads(DATA.read_text(encoding="utf-8"))
    recipes = read_catalogue()
    report = validate(registry, recipes)
    report["registry_sha256"] = hashlib.sha256(DATA.read_bytes()).hexdigest()
    (DESIGN / "analysis/part_resale.json").write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8")
    lines = ["# Part selling prices — beta references", "",
             "Owner-selected policy, 20 September 2026. A generated part can use a normal one-part material value even if it has no craftable recipe. Equivalent parts share a reference where possible. These references never charge materials, add recipes, alter delivery amounts or change part effects. Unanchored numerical costs below are beta tuning, not tested balance.", "",
             "**Example:** Plain Needle and Plain Slug both add 6 damage. Both use the Slug's normal 1-Iron cost. If Iron costs 10 credits in the shop, either sells for 5 credits, even if the Needle was granted by an effect. The example does not select an Iron price.", "",
             "**Calculation:** value the listed one-part materials at current shop prices, divide by two, and round down to whole credits for each sold part. Discounts, actual batch/generator costs and copied-part origins do not change its reference. Two parts worth 2.5 credits each sell for 2 each, totalling 4, not 5.", "",
             "The existing eligibility rules still apply: this table does not make used/installed parts or pending deliveries saleable. A copied part retains its reference. Damage, Shield spending and later bonuses do not reprice a part.", "",
             "## Standard one-part values", "",
             "Pure generated Ammo uses one Iron per started six points of its fixed damage. Pure generated Shield uses one Iron per started six points of its original grant, plus one Copper for a positive grant. The 6-damage/6-Shield values match SH001/SH002 exactly; the step sizes elsewhere are whole-material beta choices. Zero-value family outputs have zero material value. A family never replaces the own-recipe basis of an ordinary crafted source part with secondary effects.", "",
             "| Reference | Normal one-part materials | Rationale |", "| --- | --- | --- |"]
    for key, ref in registry["bases"].items():
        lines.append(f"| {key} | {material_text(ref['materials'])} | {ref['basis_note']} |")
    lines += ["", "Recast Plate Slug uses its fixed converted damage, capped at 14: 1–6 damage uses 1 Iron, 7–12 uses 2 Iron, and 13–14 uses 3 Iron. Its source Shield part and the Utility's actual ingredients are not added to its price.", "",
              "## All 27 recipe mappings", "",
              "Each entry names an output type and its one-part value. Conditional alternatives and scheduled outputs remain governed by the recipe; this table does not grant them or combine their schedules. A crafted source part keeps its full own-recipe reference; its future rewards are separately priced ordinary parts.", "",
              "| Recipe | Outputs and one-part materials |", "| --- | --- |"]
    for rid, outputs in registry["recipe_outputs"].items():
        entries = []
        for output in outputs:
            ref = output["basis"]
            cost = "1–3 Iron by fixed damage" if ref.startswith("family:") else material_text(materials_for(ref, registry, recipes))
            entries.append(f"{output['name']}: **{cost}** ({output['when']})")
        lines.append(f"| {rid} — {recipes[rid]['name']} | " + "<br>".join(entries) + " |")
    lines += ["", "## Evidence and remaining work", "",
              report["checks"], "",
              "The [machine-readable references](data/part-resale-bases.json) include quantities, aliases, conditions and beta rationale. Run `python games/overkill-foundry/design/analysis/part_resale.py` to check and render this record. The recipe audit separately verifies exact coverage of its 27 resale cases.", "",
              "Shop resource prices and numerical resale balance still need testing. Repeated grants, discounted batches and selling versus combat use may change campaign income substantially; test those paths in the planned shared-core simulator. " + report["limits"], ""]
    (DESIGN / "PART-RESALE.md").write_text("\n".join(lines), encoding="utf-8")
    print(json.dumps(report, indent=2))


if __name__ == "__main__":
    main()
