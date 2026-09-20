"""Independent rarity oracle from the human catalogue, including suffixed headings."""
import hashlib
import json
import re
from pathlib import Path

game = Path(__file__).resolve().parents[2]
source = (game / "design/RECIPE-CATALOGUE.md").read_text(encoding="utf-8")
snapshot = json.loads((game / "content/catalogue.snapshot.json").read_text(encoding="utf-8"))
manifest_bytes = (game / "content/cinderwall.manifest.json").read_bytes()
manifest = json.loads(manifest_bytes)
expected = {}
rarity = None
for line in source.splitlines():
    heading = re.match(r"^###\s+(Base|Common|Uncommon|Rare|Legendary)\b", line)
    if heading:
        rarity = heading[1]
    row = re.match(r"^\| ((?:SH|MA|IV|AD|NO)\d{3}) \|", line)
    if row:
        assert rarity is not None, row[1]
        assert row[1] not in expected, row[1]
        expected[row[1]] = rarity

assert len(expected) == 606
for rid, value in expected.items():
    actual = snapshot["recipes"][rid]["rarity"]
    assert actual == value, f"{rid}: heading says {value}, snapshot says {actual}"
for n in range(121, 124):
    assert expected[f"SH{n}"] == "Rare"
for n in range(124, 127):
    assert expected[f"SH{n}"] == "Legendary"
regular = {
    rid for rid, value in expected.items()
    if rid.startswith(("SH", "MA")) and value in {"Common", "Uncommon", "Rare"}
}
assert regular == set(manifest["recipe_pools"]["Regular"]["ids"])
assert len(regular) == 207
print("PASS 606 rarity metadata entries against actual source headings, including suffixed headings")
print("PASS SH121-123 Rare, SH124-126 Legendary, and exact 207-ID Regular offer pool")
print("manifest_sha256=" + hashlib.sha256(manifest_bytes).hexdigest())
