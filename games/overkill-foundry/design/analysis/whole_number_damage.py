"""Executable design arithmetic; not gameplay code or a combat simulation.

Run from any directory. All damage operations use integers. The catalogue audit
counts percentage effects and checks for decimal damage literals in recipe text.
"""

import json
from pathlib import Path
import re


def whole_share(amount: int, numerator: int, denominator: int) -> int:
    """Return a whole share, discarding the remainder without storing it."""
    if any(type(value) is not int for value in (amount, numerator, denominator)):
        raise TypeError("Amounts and ratio terms must be integers")
    if amount < 0 or numerator < 0 or denominator <= 0:
        raise ValueError("Amounts must be nonnegative; denominator must be positive")
    return amount * numerator // denominator


def boost_damage(damage: int, *eligible_percent_bonuses: int) -> int:
    """Combine eligible increases before calculating one whole damage bonus."""
    if any(type(value) is not int or value < 0 for value in eligible_percent_bonuses):
        raise ValueError("Eligible percentage increases must be nonnegative integers")
    return damage + whole_share(damage, sum(eligible_percent_bonuses), 100)


def reduce_damage(damage: int, percent: int) -> int:
    """Calculate remaining damage for one explicitly specified reduction."""
    if type(percent) is not int or not 0 <= percent <= 100:
        raise ValueError("A reduction must be an integer from 0 to 100")
    return whole_share(damage, 100 - percent, 100)


def spread_hit_amounts(damage: int, *eligible_share_percents: int) -> list[int]:
    """Keep each eligible spread part's hit separate, in supplied example order.

    Actual game ordering, dead-target and zero-damage trigger rules are outside
    this arithmetic model.
    """
    whole_share(damage, 0, 100)
    return [whole_share(damage, percent, 100) for percent in eligible_share_percents]


def spread_contribution_total(damage: int, *eligible_share_percents: int) -> int:
    """Return a preview total only; it must not replace the separate hits."""
    return sum(spread_hit_amounts(damage, *eligible_share_percents))


def main() -> None:
    catalogue = Path(__file__).resolve().parents[1] / "RECIPE-CATALOGUE.md"
    rows = []
    for line in catalogue.read_text(encoding="utf-8").splitlines():
        if re.match(r"^\| (?:SH|MA|IV|AD|NO)\d{3}\s*\|", line):
            cells = [cell.strip() for cell in line.strip("|").split("|")]
            if len(cells) != 7:
                raise ValueError(f"Unexpected catalogue columns: {cells[0]}")
            rows.append(cells)
    assert len(rows) == 606
    ratio_rows = [row[0] for row in rows if "%" in row[6]]
    assert len(ratio_rows) == 23
    assert not any(re.search(r"\d+\.\d+\s*(?:%|damage)", row[6]) for row in rows)

    examples = {
        "risky_packing_9_damage_plus_60_percent": boost_damage(9, 60),
        "combined_20_and_30_percent_on_10_damage": boost_damage(10, 20, 30),
        "combined_20_and_20_percent_on_3_damage": boost_damage(3, 20, 20),
        "layered_charge_21_damage_plus_16_percent": boost_damage(21, 16),
        "wide_burst_half_of_24_damage": whole_share(24, 50, 100),
        "half_of_9_damage": whole_share(9, 1, 2),
        "enemy_9_damage_reduced_by_50_percent": reduce_damage(9, 50),
        "half_of_5_shield_removed": whole_share(5, 1, 2),
        "40_percent_of_1_damage": whole_share(1, 40, 100),
        "overlapping_50_and_40_percent_of_20_damage": spread_contribution_total(20, 50, 40),
        "overlapping_50_and_40_percent_of_9_damage": spread_contribution_total(9, 50, 40),
    }
    assert list(examples.values()) == [14, 15, 4, 24, 12, 4, 4, 2, 0, 18, 7]
    assert spread_contribution_total(20, 50, 40) == spread_contribution_total(20, 40, 50)
    assert spread_contribution_total(20) == 0
    hit_examples = {
        "20_damage_with_50_and_40_percent_parts": spread_hit_amounts(20, 50, 40),
        "9_damage_with_50_and_40_percent_parts": spread_hit_amounts(9, 50, 40),
    }
    assert list(hit_examples.values()) == [[10, 8], [4, 3]]
    assert spread_hit_amounts(20, 40, 50) == [8, 10]
    assert spread_hit_amounts(20) == []

    # Include every whole damage amount from 0 through 200 and percentages
    # through 200, covering zero damage, odd splits, large bonuses and reductions.
    checked_pairs = 0
    for damage in range(201):
        for percent in range(201):
            share = whole_share(damage, percent, 100)
            boosted = boost_damage(damage, percent)
            assert type(share) is int and type(boosted) is int
            assert share * 100 <= damage * percent < (share + 1) * 100
            assert boosted == damage + share
            if percent <= 100:
                remaining = reduce_damage(damage, percent)
                assert type(remaining) is int and 0 <= remaining <= damage
            checked_pairs += 1
    assert boost_damage(3, 20, 20) > 3 + whole_share(3, 20, 100) * 2

    print(json.dumps({
        "scope": "Design arithmetic and catalogue audit only; no runtime or balance test",
        "recipe_rows": len(rows),
        "percentage_effect_rows": ratio_rows,
        "checked_damage_percentage_pairs": checked_pairs,
        "examples": examples,
        "separate_spread_hit_examples": hit_examples,
        "result": "All computed damage is whole; no fractional remainder is carried",
    }, indent=2))


if __name__ == "__main__":
    main()
