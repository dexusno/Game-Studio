"""Generate lifecycle input oracles from authored manifest/resale data, not runtime output.

The separate fixed reference_values.json table preserves the already source-derived
turn-one damage/Shield oracles used by recipe author tests. Those scalar references
are not claimed as independent QA or complete recipe effect coverage.
"""
import argparse
import hashlib
import json
from pathlib import Path

HERE = Path(__file__).resolve().parent
GAME = HERE.parents[2]
MATERIALS = ["Iron", "Copper", "Carbon", "Glass", "Circuit"]
SPREAD = {"SH005", "SH051", "SH082", "SH110", "MA041", "MA111"}
CHOICE_MAGNETS = {"SH080", "SH106", "SH108", "SH119", "SH121", "SH122"}
SHIELD_PRODUCERS = {1: "UGS-028", 2: "SH045", 3: "SH033", 4: "SH026", 5: "MA004",
                    6: "SH026", 7: "SH062", 8: "SH034", 10: "SH065", 12: "SH096",
                    14: "MA099", 15: "SH092", 18: "SH115", 20: "MA113", 24: "MA116"}
PRODUCER_HANDLERS = {
    "SH076": "Engine::catalogueUtility -> sacrifice/fixedShield/plain/receive",
    "SH045": "Engine::afterAmmo -> shieldPart",
    "SH033": "Engine::catalogueUtility -> shieldPart",
    "SH026": "Engine::effects -> shieldPart/schedule; Engine::endTurn delivery",
    "MA004": "Engine::effects -> shieldPart",
    "SH062": "Engine::afterShotHooks -> shieldPart",
    "SH034": "Engine::catalogueUtility -> schedule; Engine::endTurn delivery",
    "SH065": "Engine::catalogueUtility -> sacrifice/shieldPart",
    "SH096": "Engine::catalogueInstalled -> schedule; Engine::endTurn delivery",
    "MA099": "Engine::endTurnHooks -> specialLater; Engine::extendedDelivery",
    "SH092": "Engine::endTurnHooks -> schedule; Engine::endTurn delivery",
    "SH115": "Engine::catalogueUtility -> schedule; Engine::endTurn delivery",
    "MA113": "Engine::catalogueActivate -> payment/shieldPart",
    "MA116": "Engine::catalogueUtility -> payment/shieldPart",
    "MY1-03": "Engine::fightStartPayload -> plain/receive/shieldPart",
    "MY1-19": "Engine::endTurnUpgrade -> plain/copyLater; Engine::extendedDelivery -> receive",
    "MY1-22": "Engine::fightStartPayload -> packChoice; Engine::answerUpgrade -> plain/receive/shieldPart",
    "MY1-M1": "Engine::paidUpgradeReward after actual MA010 Heat payment -> shieldPart",
    "UGS-028": "Engine::endTurnUpgrade -> shieldPart; no intervening player action",
    "UGS-053": "Engine::turnStartPayload -> shieldPart (turn two)",
    "SH126": "Engine::catalogueActivate -> collectionEffects -> plain/receive (Perfect)",
    "MA119": "Engine::catalogueUtility -> plain/copyLater; Engine::extendedDelivery -> receive",
}


def encode(value):
    return (json.dumps(value, indent=2, sort_keys=True) + "\n").encode()


def vector(materials):
    return "{" + ",".join(str(materials.get(name, 0)) for name in MATERIALS) + "}"


def plain_origins(kind, value):
    sources = ["SH076"] if kind == "Ammo" else [SHIELD_PRODUCERS[value]]
    extras = ({4: ["MY1-22"], 5: ["MY1-19"], 6: ["MY1-03"], 10: ["SH126", "MA119"]}
              if kind == "Ammo" else {4: ["MY1-22"], 6: ["MY1-03"], 8: ["MY1-M1"],
                                      10: ["UGS-053", "SH126", "MA119"]})
    return sources + extras.get(value, [])


def inapplicable(kind, rarity=None, ephemeral=False):
    if ephemeral:
        return ["Reserve storage, player removal, positive sale and copying: UGS-028 creates this installed value inside atomic End Turn; normal reset deletes it, or a terminal phase forbids actions and campaign cleanup clears it.",
                "Load/Fire: Shield is not a bullet part.",
                "Paid output: this upgrade grant has no recipe-copy payment; origin is Granted."]
    if kind in {"Modifier", "Spread", "Magnet"}:
        values = ["SH071/SH102/SH118 copying: these sources accept Ammo and/or Shield, not this kind; all three invalid selections are executed.",
                  "Shield installation/removal/depletion/reinstallation: this is not a Shield part; Install rejects."]
        if kind == "Spread":
            values += ["Planning Activate: this physical part is consumed only by Fire; Activate rejects."]
        elif kind == "Magnet":
            values += ["Bullet loading/firing: this part fits during preparation; Load rejects.",
                       "Arbitrary later-round storage: explicit Magnet expiry overrides ordinary reserve storage; the next collection removes an unfitted expired part."]
        else:
            values += ["Bullet loading/firing of the Modifier itself: planning Activate consumes it; Load rejects."]
        return values
    values = ["Shield installation/removal/depletion: Ammo is consumed by Fire." if kind == "Ammo"
              else "Bullet load/Fire, UGS-015 committed Ammo copy and SH072 fired-Ammo return: this type is Shield."]
    if rarity is not None:
        if rarity != "Common":
            values.append("SH071/SH102 positive copying: the source is not Common; the ineligible rarity rejects.")
        elif kind == "Shield":
            values.append("SH071 positive copying: Shield is not Ammo; the ineligible kind rejects.")
        if rarity not in {"Base", "Common", "Uncommon", "Rare"}:
            values.append("SH118 positive copying: the source is above Rare; the ineligible rarity rejects.")
        if kind == "Ammo" and rarity not in {"Base", "Common"}:
            values.append("SH072 positive return: this source exceeds Common; the selected fired part rejects atomically.")
    return values


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--check", action="store_true")
    args = parser.parse_args()
    manifest_path = GAME / "content/cinderwall.manifest.json"
    snapshot_path = GAME / "content/catalogue.snapshot.json"
    manifest = json.loads(manifest_path.read_text(encoding="utf-8"))
    snapshot = json.loads(snapshot_path.read_text(encoding="utf-8"))
    values = json.loads((HERE / "reference_values.json").read_text(encoding="utf-8"))
    cases, plain_cases, inventory, origin_ids = [], [], {}, {"recipe:MA010"}
    for key, part in manifest["parts"].items():
        selected = True
        record = {"kind": part["kind"], "canonical_recipe": part["canonical_recipe"],
                  "origins": part["origins"], "resale": part["resale"],
                  "selected": selected,
                  "claim_boundary": "Physical-type materialization, applicable copying, immutable resale, placement and lifetime only. Full recipe/upgrade effect obligations remain separate."}
        if part["ordinary_generated"] and key != "warm-rivet":
            origin_ids.update(part["origins"])
            value = part["resale"]["original_fixed_value"]
            producers = plain_origins(part["kind"], value)
            plain_cases.append("{" + ",".join([json.dumps(key), "Kind::" + part["kind"], str(value)]) + "},")
            record["test_entry"] = "core/tests/parts/extra_lifecycle.inl::plainSemantics"
            record["producer_paths"] = {source: PRODUCER_HANDLERS[source] for source in producers}
            record["producer_scope"] = "These named paths execute; other manifest origins are not certified by this type fixture. SH076 receives controlled fixed printed Shield inputs, not a claim that every value is naturally obtainable."
            record["inapplicable"] = inapplicable(part["kind"], ephemeral=key == "plain-shield-1")
            if key != "plain-shield-1":
                record["copy_eligibility_by_producer"] = {}
                for producer in producers:
                    rarity = snapshot["recipes"][producer]["rarity"] if producer in snapshot["recipes"] else "Base"
                    record["copy_eligibility_by_producer"][producer] = {
                        "physical_rarity": rarity, "SH071": rarity == "Common" and part["kind"] == "Ammo",
                        "SH102": rarity == "Common", "SH118": rarity in {"Base", "Common", "Uncommon", "Rare"},
                        "SH072": part["kind"] == "Ammo" and rarity in {"Base", "Common"}}
                record["inapplicable"].append("SH071/SH102/SH118 paths marked false in copy_eligibility_by_producer reject in executed actions. SH072 rejects above-Common Ammo; for Shield it is inapplicable because the source cannot enter a fired bullet.")
                if part["kind"] == "Ammo":
                    record["inapplicable"].append("UGS-015 paid-Ammo-recipe production copy: these tested generated sources are Utility conversion/delivery or grants, not a paid Ammo recipe's output; no positive trigger is claimed.")
            record["contract"] = ["real authored grant or paid SH076 conversion", "immutable original fixed value and canonical ordinary payload",
                                  "source rarity/creator versus behavior reference", "eligible copies and inapplicable ephemeral windows",
                                  "placement/storage/consumption/reset", "per-part current-price resale"]
        else:
            source = part["canonical_recipe"]
            recipe = snapshot["recipes"][source]
            runtime_kind = "Spread" if source in SPREAD else part["kind"]
            record["runtime_kind"] = runtime_kind
            record["test_entry"] = "core/tests/part_lifecycle_tests.cpp::materializeAndUse/printedCopies/committedCopy/returnDelivery/saleAndCleanup" if runtime_kind in {"Ammo", "Shield"} else "core/tests/parts/extra_lifecycle.inl::planningLifecycle/" + {"Modifier": "modifierSemantics", "Spread": "spreadSemantics", "Magnet": "magnetSemantics"}[runtime_kind] + "; core/tests/part_lifecycle_tests.cpp::saleAndCleanup"
            record["producer_paths"] = {source: "Rules::apply(Craft) -> Engine::craft/recipePart/receive"}
            if source not in CHOICE_MAGNETS:
                record["producer_paths"]["free " + source] = "Rules::grantPart -> Engine::recipePart/receive; no Use/cooldown/payment"
            record["inapplicable"] = inapplicable(runtime_kind, recipe["rarity"])
            if source == "SH066":
                record["test_entry"] += "/sweepSemantics"
                record["settled_contract"] = "TIMING-AND-PERSISTENCE section3 explicitly makes this automatic: 3 * min(3, availableShield // 3), ordered after enemies and before retention/reset; T05/T06 prohibit frozen reads or spending incomplete groups."
                record["inapplicable"].append("Player-selected Shield payment: this conversion is explicitly automatic. No selector is required or invented.")
            if source in CHOICE_MAGNETS:
                record["inapplicable"].append("Free no-choice grant: the current factory has no selector argument. This typed Magnet is proved through paid Craft with explicit materials; no free-grant path is claimed.")
            record["copy_eligibility"] = {"SH071": runtime_kind == "Ammo" and recipe["rarity"] == "Common",
                                          "SH102": runtime_kind in {"Ammo", "Shield"} and recipe["rarity"] == "Common",
                                          "SH118": runtime_kind in {"Ammo", "Shield"} and recipe["rarity"] in {"Base", "Common", "Uncommon", "Rare"}}
            basis = (snapshot["resale"]["bases"]["warm-rivet"]["materials"] if key == "warm-rivet"
                     else part["resale"]["normal_one_part_materials"])
            cases.append("{" + ",".join([json.dumps(key), json.dumps(source),
                "Kind::" + ("Spread" if source in SPREAD else part["kind"]), "Rarity::" + recipe["rarity"], vector(recipe["materials"]),
                vector(basis), str(2 if key == "warm-rivet" else 1), str(values.get(part["kind"], {}).get(source, 0))]) + "},")
            record["source"] = recipe["source"]
            record["contract"] = ["paid materialization and grant distinction", "per-instance provenance and ready-copy eligibility",
                                  "saved reserve and loaded/installed place transitions", "canonical resale with real transaction receipts",
                                  "printed-only and committed copies where eligible", "consumption and campaign result cleanup"]
        inventory["part:" + key] = record
    assert len(inventory) == 207 and len(cases) == 178 and len(plain_cases) == 29
    inventory = {"manifest_sha256": hashlib.sha256(manifest_path.read_bytes()).hexdigest(),
                 "snapshot_sha256": hashlib.sha256(snapshot_path.read_bytes()).hexdigest(),
                 "required": 207, "selected_for_execution": 207, "explicit_residual": 0,
                 "notice": "Selection is a test plan, not a passing support declaration. Named native execution determines evidence.",
                 "obligations": inventory}
    origin_cases = []
    for origin in sorted(origin_ids):
        kind, source = origin.split(":", 1)
        rarity = snapshot["recipes"][source]["rarity"] if kind == "recipe" else "Base"
        cost = snapshot["recipes"][source]["materials"] if kind == "recipe" else {}
        origin_cases.append("{" + json.dumps(source) + ",Rarity::" + rarity + "," + vector(cost) + "},")
    outputs = {"cases.inc": ("// Authored manifest/resale inputs; see generate_cases.py.\n" + "\n".join(cases) + "\n").encode(),
               "plain-cases.inc": ("// Declared fixed generated values, not runtime output enumeration.\n" + "\n".join(plain_cases) + "\n").encode(),
               "origin-rarity.inc": ("// Authored recipe rarity and material cost; generic upgrade grants use Base ordinary behavior.\n" + "\n".join(origin_cases) + "\n").encode(),
               "contracts.json": encode(inventory)}
    for name, data in outputs.items():
        path = HERE / name
        if args.check:
            assert path.read_bytes() == data, name + " differs from authored inputs"
        else:
            path.write_bytes(data)
    print("Part lifecycle plan: 207 selected output types; SH066 uses the settled automatic timing contract.")


if __name__ == "__main__":
    main()
