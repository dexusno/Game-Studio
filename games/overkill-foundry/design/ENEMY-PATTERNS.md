# Robot pattern generation — bounded variation

**16 September 2026 · proposed generator contract**

Companion to [campaign progression](CAMPAIGN-PROGRESSION.md). The owner's primary procedural rule gates **which robots and formations** can attack by city and progress. Move-sequence variation sits inside those gates; it never unlocks a stronger robot early or adds an unannounced effect to a familiar enemy.

## Authored identity, seeded variation

Keep the [roster](ENEMY-ROSTER.md)'s printed cycle as each robot's default/reference pattern. Do not replace all 42 entries with unconstrained random attacks. Each additional variant needs an explicit legal sequence and the same counterplay review as a new formation.

| Pattern family | What may vary | What stays fixed |
| --- | --- | --- |
| Teaching/reference | Formation selection between fights | The revised single-Mite/Ram worked encounter and its Charge → Blast cycle are the current exact fixtures. The older two-Mite version is historical. |
| Cycle offset | An explicitly allowed starting position | No starting on a blast that requires an earlier Charge; no skipped first-exposure preparation move. |
| Action packet | Order of named light/debuff actions within a bounded packet | Same actions/counts per packet; locked opener, maximum debuff frequency and required recovery. |
| Charge chain | Legal light actions before entering the chain | Charge commits its described next heavy action. No random cancellation, surprise heavier blast or another blast without Charge. |
| Conditional support | Existing branch based on living allies, slots or spent supply | Finite queues, maximum live helpers, no resurrection unless expressly defined, no immediate summon attack. |
| Boss phases | Only explicitly designed phase-local variants | Existing thresholds, one-time transitions, warning windows and committed-intent timing. |

A first concrete candidate is **Cable Binder (C1-R04)**. Its default packet is Spray (4 × 2), Foul (6 plus Recipe Fouling 1), Punch (12). The proposed alternate packet is Spray, Punch, Foul. A seed selects one of these two packets at each packet boundary. Both contain 26 printed direct damage, three actions and one Fouling application before modifiers. Equal printed totals do not prove equal player HP loss: the timing changes defence/crafting choices and still needs a costed test. Spray always opens a packet, so Punch and Foul cannot immediately repeat across boundaries. No new stats or effects are introduced.

Allow this candidate only after Cable Binder has been introduced with its default packet. Other roster entries retain their default sequences until a similarly explicit variant exists. New games already vary their formations and routes without requiring every individual robot to have a random move table.

## Selection and committed intent

At fight entry, choose and save the opening intent before the player's first collection. At each subsequent round boundary, resolve due transitions and select the next legal intent before collection/planning. Forecast the complete enemy phase left to right, including known ally buffs, multi-hits, status ticks and conditional branches.

The shown action does not reroll during crafting, loading, target selection, Fire, shopping or closing the interface. Player actions can change its resulting damage, remove its source or trigger a documented state transition; update the forecast truthfully. An advertised prerequisite or condition belongs in the preview. This is not permission to substitute an unrelated random action.

For support branches, freeze the selected action at intent selection. If an Assembly Rig has committed to Attack because its helper slots were full, killing a helper leaves that Attack committed; the new slot is considered at the next selection. If Deploy was selected, resolve only the available slots and remaining supply under its printed limits; it cannot turn into a surprise Attack. Any different behaviour requires a named, previewed exception.

Keep existing explicit exceptions: Conveyor Sovereign's displayed crush can be postponed by a Brake Drone kill; Continuity Engine's phase timing follows its roster rule; a killed actor cannot perform its intent. Those are deterministic player-visible consequences, not new RNG rolls. Charge obligations and earned Overbalance recovery have priority over random selection.

Once a pattern is selected, record its state, any promised next action, finite supplies and the random draw position. Fight restart restores the original pre-start input checkpoint and deterministically reruns start effects once, not a new move bag; see the [timing/save contract](TIMING-AND-PERSISTENCE.md#exact-fight-entry-checkpoint). If identical player choices lead to different intents after Continue, the generator violates the selected restart rule.

## Whole-formation safeguards

Evaluate robots together. A light attack from one body can become lethal when a preceding body grants Drive, a second suppresses Shield and a third retaliates during preparation. Per-robot weights alone cannot bound that interaction.

- In introduction, use the listed fixed openers only. Do not synchronise newly assembled heavy attacks.
- Preserve any authored preparation/recovery windows. A sequence cannot repeatedly roll away an opening its counterplay depends on. Continuous attackers such as mites and Cable Binder may retain target-removal and defensive counterplay without gaining a new no-attack action.
- Never create repeated suppression beyond the effect's printed duration/stack limits. Do not extend an expiring penalty through an accidental unlimited streak.
- Preserve roster summon limits and next-round activation. Pattern choice cannot replenish a spent queue or grant another loot allocation.
- Never add a global shot cap or automatically end the player turn to contain a strong build. Only End Turn passes play to the robots. Installed Shield protects automatically against enemy attacks without an activation event; apply normal depletion/reset and explicit exceptions.
- Keep all quantities whole numbers. Do not derive random fractional damage or secretly scale stats with current player power.

Formation-specific safe bounds should come from actual recipe/haul/HP studies. Until those exist, the allowlist is the bound: **unreviewed combinations are ineligible**, even if their average damage resembles an approved fight.

## Reproducibility and evidence

Use separate deterministic streams keyed by campaign seed, mercenary, city ID, route node, encounter ID and robot instance. Persist the generator version and resolved content; do not rely on a language runtime's undocumented RNG stability across releases. Route draws, move draws and loot draws cannot consume each other's sequence.

The accompanying schedule study checks deterministic selection and the Cable Binder packet contract as design examples. It does not implement intent rendering, damage resolution, boss transitions or the save system. Those require future gameplay tests. Source inspiration is documented in the [STS2 progression study](research/2026-09-16-sts2-progression.md): Hunter Killer uses constrained weighted moves, Nibbit illustrates fixed-cycle offsets, and Ovicopter illustrates state-conditioned support. Our exact seed and save contract comes from the owner's restart rule, not a verified STS2 RNG implementation.
