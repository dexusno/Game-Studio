"""Execute proposed shop routes; design evidence, not game-play evidence."""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path


def supported(module: dict, capabilities: set[str]) -> bool:
    return (set(module.get("requires_all", [])).issubset(capabilities)
            and (not module.get("requires_any")
                 or bool(set(module["requires_any"]) & capabilities)))


def check_build(fitted: set[str], catalog: dict[str, dict]) -> None:
    capabilities = {capability for ident in fitted
                    for capability in catalog[ident].get("provides", [])}
    for ident in fitted:
        assert supported(catalog[ident], capabilities), f"Inactive fitted module {ident}: missing capability"


def verify_route(route: dict, rules: dict, catalog: dict[str, dict]) -> dict:
    cash = rules["starting_cash"]
    inventory: dict[str, dict] = {}
    fitted: set[str] = set()
    earned: set[str] = set()
    offers: set[str] = set()
    trace = []
    refunds = 0
    total_spent = 0
    depot = -1
    precharged: set[int] = set()
    starter_taken = False
    at_depot = False
    logical_offer_counts = []
    for index, action in enumerate(route["actions"]):
        before = cash
        kind = action["type"]
        if kind in {"buy", "sell", "fit", "unfit", "precharge"}:
            assert at_depot, "Equipment transactions require an open depot"
        if kind == "starter":
            ident = action["module"]
            assert not starter_taken, "Only one free starter per run"
            starter_taken = True
            assert ident in rules["free_starter_choices"], "Starter has not been unlocked in this scenario"
            assert not inventory, "Starter must be the first equipment acquisition"
            assert catalog[ident]["kind"] == "active", "Starter must supply an active capability"
            inventory[ident] = {"paid": 0}
            fitted.add(ident)
        elif kind == "offers":
            assert action["depot"] == depot + 1, "Depots must advance once in order"
            depot = action["depot"]
            at_depot = True
            assert 0 <= depot <= 3, "There are four shopping visits"
            if depot:
                assert f"site_{depot}_clear" in earned, "Depot opens only after site dispatch"
            offers = set(action["modules"])
            assert len(offers) == 4, "The proposal requires four module offers"
            assert len(offers) == len(action["modules"]), "Duplicate offers hide a choice"
            assert all(ident in catalog for ident in offers), "Unknown offer"
            assert not any(ident in inventory and catalog[ident]["kind"] == "passive"
                           for ident in offers), "Stock repeats an owned unique passive"
            capabilities = {capability for ident in fitted
                            for capability in catalog[ident].get("provides", [])}
            usable = [ident for ident in offers if ident not in inventory
                      and catalog[ident]["price"] <= cash
                      and supported(catalog[ident], capabilities)]
            assert len(usable) >= 2, (f"{route['id']} depot {depot}: stock lacks two affordable "
                                      f"capability-compatible choices: {usable}, wallet {cash}")
            assert any(catalog[ident]["kind"] == "passive" for ident in usable), (
                f"{route['id']} depot {depot}: no affordable support for the installed capabilities")
            logical_offer_counts.append({"depot": depot, "candidates": sorted(usable),
                                         "physical_opportunity_verified": False})
        elif kind == "reward":
            if at_depot:
                check_build(fitted, catalog)
            at_depot = False
            ident = action["id"]
            assert ident not in earned, "Site or refining reward paid twice"
            assert ident in rules["rewards"], "Unknown reward"
            assert ident.startswith(f"site_{depot + 1}_"), "Reward belongs to the next site"
            cash += rules["rewards"][ident]
            earned.add(ident)
        elif kind == "buy":
            ident = action["module"]
            assert ident in offers, "Purchase must be offered"
            assert ident not in inventory, "Unique module already owned"
            price = catalog[ident]["price"]
            assert cash >= price, f"Cannot afford {ident}: {cash} < {price}"
            cash -= price
            total_spent += price
            inventory[ident] = {"paid": price}
            offers.remove(ident)
        elif kind == "sell":
            ident = action["module"]
            assert ident in inventory, "Cannot sell unowned equipment"
            refund = inventory.pop(ident)["paid"] // 2
            refunds += refund
            cash += refund
            fitted.discard(ident)
        elif kind == "fit":
            ident = action["module"]
            assert ident in inventory, "Cannot fit unowned equipment"
            fitted.add(ident)
        elif kind == "unfit":
            fitted.remove(action["module"])
        elif kind == "precharge":
            assert depot >= 0 and depot not in precharged, "Precharge is once per departure"
            assert action.get("depot", depot) == depot, "Service belongs to current depot"
            price = rules["precharge"]["cost"]
            assert action.get("cost", price) == price, "Incorrect service price"
            assert cash >= price, "Cannot afford precharge"
            cash -= price
            total_spent += price
            precharged.add(depot)
        else:
            raise AssertionError(f"Unknown route action {kind}")
        for slot_kind, limit in rules["slots"].items():
            used = sum(catalog[ident].get("slots", 1) for ident in fitted
                       if catalog[ident]["kind"] == slot_kind)
            assert used <= limit, f"Exceeded {slot_kind} slots: {used}/{limit}"
        assert cash >= 0
        if "cash_after" in action:
            assert cash == action["cash_after"], f"Route arithmetic: expected {action['cash_after']}, got {cash}"
        trace.append({"step": index + 1, "action": kind, "cash_before": before,
                      "cash_after": cash, "fitted": sorted(fitted)})
    assert set(route["expected_build"]) == fitted, "Final build differs from the claimed route"
    check_build(fitted, catalog)
    assert depot == 3, "Route must reach final outfitting"
    assert cash == route["expected_cash_remaining"], "Final cash differs from the claimed route"
    return {"id": route["id"], "cash_remaining": cash, "spent": total_spent,
            "resale_refunds": refunds, "build": sorted(fitted), "trace": trace,
            "logical_offer_candidates": logical_offer_counts}


def main() -> None:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("spec", type=Path)
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    data = json.loads(args.spec.read_text(encoding="utf-8"))
    catalog = {item["id"]: item for item in data["modules"]}
    assert len(catalog) == len(data["modules"]), "Duplicate module identifiers"
    for module in catalog.values():
        low, high = data["rules"]["price_tiers"][module["tier"]]
        assert low <= module["price"] <= high, "Price outside its declared tier"
        assert module["kind"] in data["rules"]["slots"]
        assert 1 <= module.get("slots", 1) <= data["rules"]["slots"][module["kind"]]
    routes = [verify_route(route, data["rules"], catalog) for route in data["routes"]]
    report = {"kind": "design-economy-evidence", "source": str(args.spec),
              "source_sha256": hashlib.sha256(args.spec.read_bytes()).hexdigest(),
              "modules_checked": len(catalog), "routes_checked": len(routes),
              "routes": routes,
              "limits": ["Checks authored offer/budget/equipment sequences only; not random offer fairness.",
                         "Does not implement physical salvage, module effects, challenge balance or fun.",
                         "Different identifiers and affordable builds do not prove distinct strategies.",
                         "Capability-compatible offers still need distinct physical opportunities in the next site."]}
    if args.output:
        args.output.write_text(json.dumps(report, indent=2) + "\n", encoding="utf-8", newline="\n")
    print(json.dumps({"modules_checked": len(catalog), "routes_checked": len(routes),
                      "remaining_cash": {route["id"]: route["cash_remaining"] for route in routes}}))


if __name__ == "__main__":
    main()
