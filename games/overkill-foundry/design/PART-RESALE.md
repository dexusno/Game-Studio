# Part selling prices — beta references

Owner-selected policy, 20 September 2026. A generated part can use a normal one-part material value even if it has no craftable recipe. Equivalent parts share a reference where possible. These references never charge materials, add recipes, alter delivery amounts or change part effects. Unanchored numerical costs below are beta tuning, not tested balance.

**Example:** Plain Needle and Plain Slug both add 6 damage. Both use the Slug's normal 1-Iron cost. If Iron costs 10 credits in the shop, either sells for 5 credits, even if the Needle was granted by an effect. The example does not select an Iron price.

**Calculation:** value the listed one-part materials at current shop prices, divide by two, and round down to whole credits for each sold part. Discounts, actual batch/generator costs and copied-part origins do not change its reference. Two parts worth 2.5 credits each sell for 2 each, totalling 4, not 5.

The existing eligibility rules still apply: this table does not make used/installed parts or pending deliveries saleable. A copied part retains its reference. Damage, Shield spending and later bonuses do not reprice a part.

## Standard one-part values

Pure generated Ammo uses one Iron per started six points of its fixed damage. Pure generated Shield uses one Iron per started six points of its original grant, plus one Copper for a positive grant. The 6-damage/6-Shield values match SH001/SH002 exactly; the step sizes elsewhere are whole-material beta choices. Zero-value family outputs have zero material value. A family never replaces the own-recipe basis of an ordinary crafted source part with secondary effects.

| Reference | Normal one-part materials | Rationale |
| --- | --- | --- |
| plain-ammo-4 | 1 Iron | Exact Plain Slug equivalent at 6; other values use the beta plain-ammo family, one Iron per started six damage. |
| plain-ammo-6 | 1 Iron | Exact Plain Slug equivalent at 6; other values use the beta plain-ammo family, one Iron per started six damage. |
| plain-ammo-8 | 2 Iron | Exact Plain Slug equivalent at 6; other values use the beta plain-ammo family, one Iron per started six damage. |
| plain-ammo-10 | 2 Iron | Exact Plain Slug equivalent at 6; other values use the beta plain-ammo family, one Iron per started six damage. |
| plain-shield-3 | 1 Iron + 1 Copper | Exact Basic Shield Plate equivalent at 6; other values use the beta plain-shield family, one Iron per started six Shield plus one Copper. |
| plain-shield-4 | 1 Iron + 1 Copper | Exact Basic Shield Plate equivalent at 6; other values use the beta plain-shield family, one Iron per started six Shield plus one Copper. |
| plain-shield-5 | 1 Iron + 1 Copper | Exact Basic Shield Plate equivalent at 6; other values use the beta plain-shield family, one Iron per started six Shield plus one Copper. |
| plain-shield-6 | 1 Iron + 1 Copper | Exact Basic Shield Plate equivalent at 6; other values use the beta plain-shield family, one Iron per started six Shield plus one Copper. |
| plain-shield-8 | 2 Iron + 1 Copper | Exact Basic Shield Plate equivalent at 6; other values use the beta plain-shield family, one Iron per started six Shield plus one Copper. |
| plain-shield-9 | 2 Iron + 1 Copper | Exact Basic Shield Plate equivalent at 6; other values use the beta plain-shield family, one Iron per started six Shield plus one Copper. |
| plain-shield-10 | 2 Iron + 1 Copper | Exact Basic Shield Plate equivalent at 6; other values use the beta plain-shield family, one Iron per started six Shield plus one Copper. |
| plain-shield-12 | 2 Iron + 1 Copper | Exact Basic Shield Plate equivalent at 6; other values use the beta plain-shield family, one Iron per started six Shield plus one Copper. |
| plain-shield-14 | 3 Iron + 1 Copper | Exact Basic Shield Plate equivalent at 6; other values use the beta plain-shield family, one Iron per started six Shield plus one Copper. |
| plain-shield-15 | 3 Iron + 1 Copper | Exact Basic Shield Plate equivalent at 6; other values use the beta plain-shield family, one Iron per started six Shield plus one Copper. |
| plain-shield-20 | 4 Iron + 1 Copper | Exact Basic Shield Plate equivalent at 6; other values use the beta plain-shield family, one Iron per started six Shield plus one Copper. |
| warm-rivet | 1 Iron + 1 Carbon | 4 damage plus 1 Heat. Uses Heated Rivet ingredients as a beta reference; retains its own weaker effect and batch cost. |
| split-acid-needle | 1 Iron + 1 Carbon + 1 Glass | 6 damage plus Corrosion 2. Needle Acid Capsule ingredient family with a Glass increment for higher unconditional damage; whole-material one-part beta reference, not half the batch cost. |
| copper-pin | 1 Iron + 1 Copper | 3 damage plus Mark 2. One Iron below the stronger Denting Slug as a beta reference; not half the Copper Pins batch cost. |
| pulse-tip | 2 Iron + 1 Copper | 10 damage plus 1 Charge. Conducting Pin ingredients with one additional Iron for the stronger damage. |

Recast Plate Slug uses its fixed converted damage, capped at 14: 1–6 damage uses 1 Iron, 7–12 uses 2 Iron, and 13–14 uses 3 Iron. Its source Shield part and the Utility's actual ingredients are not added to its price.

## All 27 recipe mappings

Each entry names an output type and its one-part value. Conditional alternatives and scheduled outputs remain governed by the recipe; this table does not grant them or combine their schedules. A crafted source part keeps its full own-recipe reference; its future rewards are separately priced ordinary parts.

| Recipe | Outputs and one-part materials |
| --- | --- |
| SH026 — Folding Brace | Regular 4-Shield part: **1 Iron + 1 Copper** (on recipe Use)<br>Regular 6-Shield part: **1 Iron + 1 Copper** (next-round delivery) |
| SH057 — Spare Metal Brace | Regular 8-Shield part: **2 Iron + 1 Copper** (on recipe Use)<br>Regular 5-Shield part: **1 Iron + 1 Copper** (conditional next-round delivery) |
| SH076 — Plate Recasting | Recast Plate Slug: **1–3 Iron by fixed damage** (conversion) |
| SH092 — Hinged Wall | Hinged Wall Brace: **2 Iron + 1 Copper + 1 Glass** (crafted source part)<br>Regular 15-Shield part: **3 Iron + 1 Copper** (next-round delivery) |
| SH113 — Last Wall | Last Wall Plate: **3 Iron + 1 Copper + 1 Glass** (crafted source part)<br>Regular 20-Shield part: **4 Iron + 1 Copper** (next-round delivery; cooling is a delivery effect, not a part attribute) |
| SH126 — Salvage Foundry | Auto-Forge Die: **1 Iron + 1 Copper + 1 Carbon + 1 Circuit** (crafted source part)<br>Salvage Slug: **2 Iron** (Perfect-grab reward)<br>Salvage Shield Plate: **2 Iron + 1 Copper** (Perfect-grab reward) |
| MA038 — Rivet Bundle | Warm Rivet: **1 Iron + 1 Carbon** (on recipe Use) |
| MA048 — Holdfast | Locking Plate: **2 Iron + 2 Copper** (crafted source part)<br>Regular 12-Shield part: **2 Iron + 1 Copper** (next round if Heat paid; alternative to 4)<br>Regular 4-Shield part: **1 Iron + 1 Copper** (next round if Heat not paid; alternative to 12) |
| MA099 — Settled Slag | Slag Foundation: **3 Iron + 1 Carbon + 1 Circuit** (crafted source part)<br>Regular 14-Shield part: **3 Iron + 1 Copper** (conditional next-round delivery) |
| MA107 — Warm Repair Bench | Ready Plate: **2 Iron + 1 Copper** (next-turn delivery) |
| MA119 — Spare Foundry | Siege Slug: **2 Iron** (next-turn delivery)<br>Siege Plate: **2 Iron + 1 Copper** (next-turn delivery) |
| IV019 — Feint Plate | Folded Plate: **2 Iron + 1 Copper** (crafted source part)<br>Regular 3-Shield part: **1 Iron + 1 Copper** (next-round delivery) |
| IV057 — Stored Wall | Packed Wall: **2 Iron + 1 Copper + 1 Glass** (crafted source part)<br>Regular 4-Shield part: **1 Iron + 1 Copper** (next-round delivery) |
| IV061 — Hold the Gap | Gap Plate: **2 Iron + 1 Glass** (crafted source part)<br>Regular 5-Shield part: **1 Iron + 1 Copper** (conditional next-round delivery) |
| IV088 — Paired Needles | Split Acid Needle: **1 Iron + 1 Carbon + 1 Glass** (on recipe Use) |
| IV090 — Stockroom Wall | Stockroom Plate: **2 Iron + 1 Copper + 1 Circuit** (crafted source part)<br>Regular 14-Shield part: **3 Iron + 1 Copper** (conditional next-round delivery) |
| IV118 — Spare Needle Bench | Plain Needle: **1 Iron** (conditional next-turn reward, at most once per round) |
| AD017 — Spare Bolts | Small Rivet: **1 Iron** (on recipe Use) |
| AD074 — Hold the Panel | Locking Guard: **2 Iron + 1 Copper + 1 Circuit** (crafted source part)<br>Regular 8-Shield part: **2 Iron + 1 Copper** (conditional next-round delivery) |
| AD083 — Work Order | Mixed Order: **2 Iron + 1 Copper + 1 Circuit** (crafted source part)<br>Small Rivet: **1 Iron** (next-turn delivery)<br>Guard Tab: **2 Iron + 1 Copper** (next-turn delivery) |
| AD092 — Shared Scaffold | Scaffold Guard: **3 Iron + 1 Copper + 1 Circuit** (crafted source part)<br>Regular 12-Shield part: **2 Iron + 1 Copper** (one conditional delivery in each of the next two rounds) |
| AD117 — Night Shift | Overnight Order: **2 Iron + 2 Copper + 1 Carbon + 1 Circuit** (crafted source part)<br>Heavy Rivet: **2 Iron** (next-turn delivery)<br>Guard Tab: **2 Iron + 1 Copper** (next-turn delivery) |
| NO031 — Copper Pins | Copper Pin: **1 Iron + 1 Copper** (on recipe Use) |
| NO063 — Twin Field | Small Field Ring: **1 Iron + 1 Copper** (on recipe Use) |
| NO087 — Field Lock | Field Lock: **2 Iron + 2 Copper** (crafted source part)<br>Regular 12-Shield part: **2 Iron + 1 Copper** (conditional next-round delivery) |
| NO107 — Shield Press | Stored Field Plate: **2 Iron + 1 Copper** (next-turn delivery) |
| NO120 — Pocket Generator | Pulse Tip: **2 Iron + 1 Copper** (next-turn delivery)<br>Charged Plate: **2 Iron + 1 Copper** (next-turn delivery) |

## Evidence and remaining work

Recipe/reference coverage, integer quantities, alias consistency, exact ordinary-part anchors, per-part floor, current prices and variable-output boundaries passed.

The [machine-readable references](data/part-resale-bases.json) include quantities, aliases, conditions and beta rationale. Run `python games/overkill-foundry/design/analysis/part_resale.py` to check and render this record. The recipe audit separately verifies exact coverage of its 27 resale cases.

Shop resource prices and numerical resale balance still need testing. Repeated grants, discounted batches and selling versus combat use may change campaign income substantially; test those paths in the planned shared-core simulator. No runtime sales, combat, income-loop search, campaign simulation or balance test.
