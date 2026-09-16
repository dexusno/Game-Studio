"""Design-only route study; no combat, economy, game saves or gameplay runtime.

Run from any directory: python campaign_progression.py --seeds 1000
Models the twelve-city registry, layered access graphs and gated offer sets.
Uses SHA-256-keyed streams for repeatable examples, not a production RNG API.
"""

import argparse
import hashlib
import itertools
import json
import random
import re
from pathlib import Path


VERSION = "paper-route-1"
# Each tuple is an authored complete formation. No arbitrary robot attachments.
FORMATIONS = {
    "mites": ("C1-R01", "C1-R01"),
    "ram": ("C1-R02",), "cask": ("C1-R05",),
    "knuckle": ("C1-R08",), "binder": ("C1-R04",),
    "nest": ("C1-R07",), "sentinel": ("C1-R03",),
    "worked": ("C1-R01", "C1-R01", "C1-R02"),
    "emitter_mite": ("C1-R06", "C1-R01"),
    "binder_mite": ("C1-R04", "C1-R01"),
    "cargo": ("C2-R01",), "latch": ("C2-R05",),
    "courier": ("C2-R08",), "brood": ("C2-R07",),
    "pylon": ("C2-R06",),
    "screen_cargo": ("C2-R02", "C2-R01"),
    "loom_cargo": ("C2-R04", "C2-R01"),
    "booster_cargo": ("C2-R03", "C2-R01"),
    "litany": ("C3-R01",), "vault": ("C3-R05",),
    "surveyor": ("C3-R07",), "assembly": ("C3-R04",),
    "siphon": ("C3-R08",), "auditor": ("C3-R06",),
    "aegis_gunner": ("C3-R02", "C3-R03"),
}
OPENING = {
    1: ("mites", "ram", "cask"),
    2: ("cargo", "latch", "courier"),
    3: ("litany", "vault", "surveyor"),
}
# (mercenary, tier, city, added middle, added late, preferred officer, boss)
CITIES = (
    ("Mara", 1, "Cinderwall", ("knuckle", "binder"), ("worked", "nest"), 1, 1),
    ("Mara", 2, "Coilbridge", ("screen_cargo", "brood"), ("loom_cargo", "booster_cargo"), 1, 1),
    ("Mara", 3, "Glassward", ("aegis_gunner",), ("assembly", "siphon"), 3, 2),
    ("Ivo", 1, "Brinegate", ("knuckle", "nest"), ("sentinel", "emitter_mite"), 3, 2),
    ("Ivo", 2, "Sablecross", ("screen_cargo", "pylon"), ("brood", "loom_cargo"), 2, 2),
    ("Ivo", 3, "Veilcourt", ("aegis_gunner",), ("siphon", "assembly"), 3, 1),
    ("Ada", 1, "Rivetford", ("nest", "binder"), ("knuckle", "binder_mite"), 2, 3),
    ("Ada", 2, "Latchhaven", ("brood", "booster_cargo"), ("screen_cargo", "loom_cargo"), 1, 3),
    ("Ada", 3, "Relaykeep", ("assembly",), ("aegis_gunner", "siphon"), 1, 1),
    ("Noor", 1, "Copperwake", ("nest", "knuckle"), ("binder", "sentinel"), 3, 2),
    ("Noor", 2, "Stormrail", ("brood", "screen_cargo"), ("booster_cargo", "loom_cargo"), 1, 3),
    ("Noor", 3, "Prismhold", ("assembly",), ("aegis_gunner", "auditor"), 3, 2),
)


def rng(*key):
    payload = json.dumps((VERSION, *key), separators=(",", ":")).encode()
    return random.Random(int.from_bytes(hashlib.sha256(payload).digest(), "big"))


def widths(character):
    """Layer widths for encounters 1..12. Junctions cost no extra step."""
    return {
        "Mara": (1, 1, 1, 3, 3, 3, 1, 3, 3, 3, 1, 1),
        "Ivo": (1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 1, 1),
        "Ada": (1, 1, 3, 3, 3, 3, 3, 3, 3, 3, 1, 1),
        "Noor": (1, 1, 3, 3, 1, 2, 2, 2, 1, 3, 1, 1),
    }[character]


def destinations(character, step, previous_lane):
    """Reachable destination lanes for the next numbered encounter."""
    w = widths(character)
    current = w[step - 1]
    previous = w[step - 2] if step > 1 else 1
    if current == 1:
        return (0,)
    if previous == 1 or (character == "Ada" and step in (3, 5, 7, 9)):
        return tuple(range(current))
    if character == "Ivo":
        return tuple(i for i in range(current) if abs(i - previous_lane) <= 1)
    return (previous_lane,)


def eligible(city, step):
    _, tier, _, middle, late, _, _ = city
    intro = 3 if tier == 1 else 2
    result = OPENING[tier]
    if step > intro:
        result += middle
    if 7 <= step <= 10:
        result += late
    return result


def category_schedule(city, seed):
    character, tier, name, _, _, preferred, _ = city
    stream = rng(seed, character, name, "categories")
    choices = [p for p in itertools.combinations(range(4, 11), 3)
               if all(b - a > 1 for a, b in zip(p, p[1:]))]
    officer_steps = stream.choice(choices)
    remaining = [i for i in (1, 2, 3) if i != preferred]
    stream.shuffle(remaining)
    identities = [preferred, *remaining]
    if tier == 3:
        # Citadel Breacher always occupies the final opportunity, at >= 8.
        identities = [i for i in identities if i != 2] + [2]
    officers = dict(zip(officer_steps, identities))
    intro = 3 if tier == 1 else 2
    mysteries = set(stream.sample(range(intro + 1, 12), 3))
    return officers, mysteries


def offer_set(city, seed, step, previous_lane):
    character, tier, name, _, _, _, boss = city
    if step == 12:
        return (("B", f"C{tier}-B{boss:02}", 0),)
    officers, mysteries = category_schedule(city, seed)
    stream = rng(seed, character, name, "offers", step, previous_lane)
    pool = eligible(city, step)
    # Guaranteed simpler Regular; remaining Regular cards are distinct.
    first = stream.choice(OPENING[tier])
    selected = [first]
    kinds = ["R", "O" if step in officers else "R", "M" if step in mysteries else "R"]
    for kind in kinds[1:]:
        if kind == "R":
            selected.append(stream.choice([f for f in pool if f not in selected]))
        elif kind == "O":
            selected.append(f"C{tier}-O{officers[step]:02}")
        else:
            selected.append("mystery_unresolved")
    lanes = list(destinations(character, step, previous_lane))
    stream.shuffle(lanes)
    while len(lanes) < 3:
        lanes.append(stream.choice(lanes))
    cards = list(zip(kinds, selected, lanes))
    stream.shuffle(cards)
    return tuple(cards)


def binder_packets(seed, count=30):
    stream = rng(seed, "binder", "packets")
    return [stream.choice((("spray", "foul", "punch"),
                           ("spray", "punch", "foul"))) for _ in range(count)]


def check_registry():
    design = Path(__file__).resolve().parents[1]
    roster = (design / "ENEMY-ROSTER.md").read_text(encoding="utf-8")
    ids = set(re.findall(r"C[123]-[ROB]\d{2}", roster))
    document = (design / "CAMPAIGN-PROGRESSION.md").read_text(encoding="utf-8")
    assert len(CITIES) == len({c[2] for c in CITIES}) == 12
    assert len(ids) == 42
    for city in CITIES:
        character, tier, name, middle, late, preferred, boss = city
        assert name in document
        assert f"C{tier}-B{boss:02}" in ids
        assert f"C{tier}-O{preferred:02}" in ids
        assert len(set(OPENING[tier] + middle + late)) == len(OPENING[tier] + middle + late)
        for formation in OPENING[tier] + middle + late:
            assert all(robot in ids and robot.startswith(f"C{tier}-")
                       for robot in FORMATIONS[formation])
    for character in ("Mara", "Ivo", "Ada", "Noor"):
        assert sorted(c[1] for c in CITIES if c[0] == character) == [1, 2, 3]
    signatures = {tuple((s, p, destinations(c, s, p))
                        for s in range(1, 13)
                        for p in range(widths(c)[s - 2] if s > 1 else 1))
                  for c in ("Mara", "Ivo", "Ada", "Noor")}
    assert len(signatures) == 4


def check_city(city, seed):
    character, tier, _, _, _, _, _ = city
    officers, mysteries = category_schedule(city, seed)
    assert len(officers) == len(mysteries) == 3
    assert set(officers.values()) == {1, 2, 3}
    assert all(b - a > 1 for a, b in zip(officers, list(officers)[1:]))
    assert max(officers) <= 10 and min(officers) >= 4
    assert min(mysteries) > (3 if tier == 1 else 2)
    if tier == 3:
        assert next(s for s, identity in officers.items() if identity == 2) >= 8
    reachable = {0}
    states = 0
    signature = []
    for step in range(1, 13):
        next_reachable = set()
        for previous_lane in sorted(reachable):
            cards = offer_set(city, seed, step, previous_lane)
            assert cards == offer_set(city, seed, step, previous_lane)
            states += 1
            signature.append(cards)
            if step == 12:
                assert len(cards) == 1 and cards[0][0] == "B"
            else:
                assert len(cards) == 3 and any(k == "R" for k, _, _ in cards)
                assert sum(k == "O" for k, _, _ in cards) == int(step in officers)
                assert sum(k == "M" for k, _, _ in cards) == int(step in mysteries)
                regulars = [f for k, f, _ in cards if k == "R"]
                assert len(regulars) == len(set(regulars))
                assert any(f in OPENING[tier] for f in regulars)
                assert all(f in eligible(city, step) for f in regulars)
            legal = set(destinations(character, step, previous_lane))
            assert {lane for _, _, lane in cards} == legal
            next_reachable.update(legal)
        reachable = next_reachable
    assert reachable == {0}
    return states, tuple(signature)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--seeds", type=int, default=1000)
    args = parser.parse_args()
    if args.seeds < 2:
        parser.error("use at least two seeds to check variation")
    check_registry()
    states = 0
    for city in CITIES:
        signatures = set()
        for seed in range(args.seeds):
            checked, signature = check_city(city, seed)
            states += checked
            signatures.add(signature)
        assert len(signatures) > 1
    pattern_variants = set()
    for seed in range(args.seeds):
        packets = binder_packets(seed)
        assert packets == binder_packets(seed)
        pattern_variants.update(packets)
        moves = [m for p in packets for m in p]
        assert all(p[0] == "spray" and set(p) == {"spray", "foul", "punch"} for p in packets)
        assert all(sum({"spray": 8, "foul": 6, "punch": 12}[m] for m in p) == 26 for p in packets)
        assert all(a != b for a, b in zip(moves, moves[1:]))
    assert len(pattern_variants) == 2
    print(f"PASS: 12 cities, 4 distinct graphs, {args.seeds} seeds/city, "
          f"{states:,} reachable offer states; gated pools, quotas, Regular alternatives, "
          "boss access, reproducibility and Cable Binder packet boundaries.")
    print("NOT TESTED: combat/HP balance, reward economy, teaching-history filters, "
          "district weights, anti-repeat history, alternate graph variants, engine saves or UI.")


if __name__ == "__main__":
    main()
