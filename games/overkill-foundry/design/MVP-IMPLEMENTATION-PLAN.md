# MVP readiness and implementation order

Review date: 20 September 2026. Planning only. Source baseline: `1e78ddd`, plus the owner's request to identify glaring holes before starting an MVP. This plan orders implementation; it does not replace the mechanic decisions or select a smaller final game. Current progress remains in [STATUS](../STATUS.md).

## Readiness verdict

**There is enough settled design to plan and build the common foundations. The complete MVP boundary is not yet ready to call settled.** The original 111 recipe clarifications are closed. The general timing/persistence contract and [first beta tuning profile](BETA-TUNING.md) are now specified. The STS2-inspired progression/content framework and draft rosters already exist. The MVP-scope choice remains pending; translating the relevant existing pools into executable content is implementation work. The numerical review exposed repeatable starter-part farming against non-scaling Mites; an overtime-pressure question is pending, without changing the current rules. No game implementation or runtime balance claim follows from this planning work.

| Finding | Why it matters | Treatment before dependent work |
| --- | --- | --- |
| First-MVP size has never been selected | A three-fight experiment, one complete city and four full campaigns require different content and acceptance criteria | Owner question pending. Recommend one complete City 1 with Mara. Common rules work is useful for every option. |
| Resolved: Shield protects during Fire | Owner clarifies that firing automatically activates Shield when installed parts provide strength | Ordinary recoil depletes that available Shield before HP. No manual activation, refill or repeated first-installation effect; zero-shot enemy-phase protection and explicit exceptions remain. This is no longer a readiness gap. |
| Resolved at design level: effect order | [Timing contract](TIMING-AND-PERSISTENCE.md) fixes action/reaction order, stable ties, end-phase effects and delayed deliveries | Implement the shared queue and verify its worked traces before enabling complex effects. Preserve explicit clocks and special abilities. |
| Resolved at design level: transitions and persistence | [Save contract](TIMING-AND-PERSISTENCE.md#5-fight-completion-and-between-fight-flow) fixes pre-start rollback, old-stock clearing, staged supplies and once-only choices/results | Implement atomic commits and crash/reload checks; no runtime correctness is claimed by the written contract. |
| Existing content framework; implementation still required | The campaign registry already assigns opening/middle/late robot formations, Officers and bosses across twelve cities. Recipe starters/shared-character rarity pools and filtered upgrade/Mayor pools already exist | Derive the build manifest from those sources for the chosen demo boundary and implement its effects. Individual Mystery outcomes still need authoring; initial offer weights and numerical inputs now come from beta-balance-v1. |
| Whole-city economy and HP attrition have not been simulated | Paper recipe costs and one worked fight do not establish shop affordability, healing supply, repeated generator sales or a beatable boss | Configurable beta values plus complete-city runs and human tests. This is balance work, not a reason to reopen the base rules. |
| No fresh Unreal project or playable proof exists | Concept boards and the old camera study establish a direction, not input quality, runtime performance or graphics approval | Verify the new toolchain, build the shared core, then an actual interactive build. Human visual approval remains a delivery gate. |

The owner resolved recoil protection on 20 September: Shield automatically activates when firing if installed parts provide strength. The first-MVP scope and newly exposed farming/overtime question remain pending. The other open rows are concrete work packages. They do not require a new questionnaire before every implementation step.

## Content framework already available

The [campaign progression plan](CAMPAIGN-PROGRESSION.md) already provides mercenary → city → progress band → district → formation → legal pattern gates and a twelve-city placement registry. The beta fixture uses easier opening Regular fights, then development/pressure pools, optional Officer/Mystery opportunities, an approach fight and one boss. The twelve-encounter count, opportunity schedule and exact placements remain draft tuning within the owner's selected framework.

For example, Mara's proposed Cinderwall opening pool is the Mite pair, Breach Ram and Pressure Cask; middle/late gates add named formations, with Redline Pursuer as the preferred first Officer and Gatebreaker Prime as the initial boss. That is already a concrete content allocation. The existing 606-recipe catalogue supplies accepted starter sets and shared/character rarity pools; the 288-upgrade catalogue and Mayor rules supply eligibility and tiered offers. No new permanent recipe-unlock gate or replacement roster is needed.

What remains is implementing the relevant effects/data, writing individual Mystery outcomes and testing the now-specified beta values across whole cities. An incomplete runtime does not mean the design framework is missing.

## Proposed playable boundary

Pending the scope answer, the recommended first MVP is **Mara's complete first city**: profile/New Game, Mayor choice, gathering and Precision, crafting and removable parts, repeated shots and End Turn, gated encounter choices, rewards and recipe exchange, shops, Mystery visits, boss victory, death and same-seed Continue. Use Mara's city identity and route from the existing progression proposal. The existing 12-encounter route is a possible beta fixture within the selected 10–20 range, not a newly approved count.

Build configuration starts from the existing 12-recipe starter set, city-specific formation registry, shared/mercenary recipe rarity pools and upgrade/Mayor eligibility rules. Compile the relevant existing IDs into the chosen demo boundary, covering route rewards, shops, Officers, the boss and complete Mystery outcomes. A supported-content manifest is an implementation deliverable, not a request to redesign the pools or select another arbitrary “30 recipes” limit. Verify that the resulting rewards support at least two plausible builds. Include at least one healing path, one cooling interaction, saved-part play, Heat/Hot Barrel, a delayed Shield grant and a deliberate upgrade exception so the MVP tests the actual game.

The old three-fight/roughly-30-recipe proposal was never selected. If chosen now, use it as the shorter playable boundary and still keep the same foundations. If full campaigns are chosen, expand the enabled content and campaign gates before acceptance; do not silently label one city as that MVP.

The full design remains four mercenaries with their own three cities and progression trees, the complete eligible recipe and upgrade pools, later difficulty tiers, the ultimate challenge and Steam achievements. A smaller first build is an integration milestone, not deletion of those commitments. No calendar or owner-time cap is imposed.

**Playtest question:** do visible enemy intentions and changing supplies make the player change what they craft, save, install and fire, and does a reward change a later decision? A repeated best sequence regardless of enemies/rewards, compulsory tedious cooling/selling loops, or precision play that is only tolerated for its reward would challenge the design. Automated wins cannot answer whether the interaction feels good.

## Sources and rule precedence

Owner decisions override draft defaults. A general default does not remove a named character, recipe or upgrade exception. Older proposals remain history when a later explicit decision resolves that same issue. A completed source audit is not an exhaustive runtime interaction test.

| Area | Implementation input and its limit |
| --- | --- |
| Rules and chronology | [Redesign plan](REDESIGN-PLAN.md), [decisions](../DECISIONS.md) and current [brief](../BRIEF.md). Historical readiness sections are superseded by this review. |
| Recipes | [606-row catalogue](RECIPE-CATALOGUE.md) and [completed audit](RECIPE-RULE-AUDIT.md), reviewed recipe snapshot `f8a7751`. No recipe-row changes in this review; numbers still need beta testing. |
| Part prices | [Part resale](PART-RESALE.md), its [reference data](data/part-resale-bases.json) and [analysis](analysis/part_resale.py). Fixed source identity and original generated value determine the reference; subsequent bonuses/depletion do not reprice a part. |
| Mercenaries | [Abilities](MERCENARY-ABILITIES.md). Preserve Hot Barrel, Charged Barrel, Find the Seam and Bolt. Quench Recovery/Residual Current are unselected alternatives, not automatic additions. |
| Enemies and routes | [Roster](ENEMY-ROSTER.md), [effects](ROBOT-EFFECTS.md), [patterns](ENEMY-PATTERNS.md), [progression](CAMPAIGN-PROGRESSION.md). Draft values and optional effect aliases require one implementation identity; aliases must not stack as two mechanics. |
| Upgrades | [Catalogue](UPGRADE-CATALOGUE.md), [system](UPGRADE-SYSTEM.md) and [Mayors](MAYORS.md). Individual effects/values remain proposals. The 288-item pool requires typed implementation and interaction coverage. |
| Presentation | [Camera flow](COMBAT-CAMERA-FLOW.md) and the supplied style anchor. 16B preparation, rear claw, visible intentions and 16C action remain the direction. Reference art is not a game asset/build. |
| Validation | [Worked encounter](ENCOUNTER-DRONES-AND-SIEGE.md) and [balance plan](BALANCE-RESEARCH.md). The paper route is an arithmetic fixture; the existing Python studies are not a combat simulator. |
| Later progression | [Endgame](ENDGAME-PROGRESSION.md), [achievements](ACHIEVEMENTS.md) and [Steam integration](STEAM-ACHIEVEMENTS.md). Preserve these requirements in the data/event design; their proposed numbers and gates are not silently approved. |

## Shared implementation foundation

Proposed structure: an engine-independent C++ rules library used by both a graphics-free runner and the Unreal game adapter. Confirm the installed compiler/Unreal versions and a working build in package P01 before fixing build-system details. Do not adapt Magnet Sweep's gameplay code or create an independent Python combat implementation.

The core owns state, legal actions, costs, random draws, effects, outcomes and immutable event records. Both clients submit the same actions and receive the same results. The UI, claw animation and AI policy do not calculate a second version of damage or rewards. Preview evaluates a copied state without spending live resources, consuming random draws or recording achievements. Inputs from Precision are recorded as deterministic collection results, so rendering/physics cannot change a replay's combat outcome.

Keep stable IDs for recipes, owned recipe copies, physical parts, upgrades, robots, encounters, offers and reward transactions. Store original source/resale reference separately from mutable part strength, age, remaining Shield and installation history. Version content, rules, saves and RNG streams. A changed content version must not silently reinterpret an old checkpoint.

A policy interface can later support Jev or another decision maker. Legal action generation, exact arithmetic and rule execution remain local code. Remote AI is optional and is not on the MVP's critical path; no TypeSafe integration is designed or implemented by this review.

## Contracts to finish before feature implementation

**Timing and persistence — completed specification:** [TIMING-AND-PERSISTENCE.md](TIMING-AND-PERSISTENCE.md) now fixes atomic payments, explicit trigger precedence, stable effect order, recoil/death interception, pre-reset Shield spending/retention, delayed deliveries and the four-turn escape trace. Its persistence matrix and phase transitions cover rewards, shops, Mayor/Mystery choices, profile discovery, the original pre-start fight checkpoint, atomic save receipts and crash-recovery limits. The 18 worked traces are expected results for future core tests, not executed gameplay tests. This resolves the general timing/save design gap under the owner's instruction to work out straightforward consequences.

**Collection/economy values — specified for initial tests.** [beta-balance-v1](BETA-TUNING.md) sets 80 HP for each mercenary, ten-material steered hauls, 100 starting Credits, Precision reward amounts, meter values, finite shop quantities/prices, body-specific core values and source-specific loot weights. Use its [versioned data](data/beta-balance-v1.json), preserving explicit bonus/upgrade exceptions and the timing contract. Precision control windows/commit-cancel interaction still need implementation and human testing. The known starter resale loop has a pending enemy-pressure proposal; do not hide it behind an unapproved action/storage/sale cap.

**One enabled-content manifest — engineering work from the existing framework.** Derive every possible MVP starter, offer, grant, robot, status, Mystery and upgrade from the existing catalogue/eligibility and city registry for the chosen boundary, including nested generated outputs and summoned robots. Map each to supported effect operations and test fixtures. Detect a reference to unsupported content at validation/build time and exclude it from offers until implemented. Preserve full catalogue source data; staging implementation is not a permanent recipe-unlock system. A player should never receive a descriptive but nonfunctional reward.

## Dependency-ordered work packages

These IDs describe build order, not a competing progress tracker. A package may begin only when its listed prerequisites supply the stated contracts. P00 resolves the remaining scope choice, derives the enabled-content manifest from existing pools, adopts beta-balance-v1, and applies the completed timing/persistence contract. Those event/save decisions do not need another owner review by default. If an answer remains pending, common contract preparation may proceed but the affected content/acceptance rules remain blocked.

| ID | Package | Depends on | Deliverable and evidence required to finish |
| --- | --- | --- | --- |
| P00 | Scope, contracts and existing-pool manifest | — | Record the scope answer; apply completed timing/save and Shield/recoil contracts and beta-balance-v1; derive exact enabled content. Track the pending farming-pressure choice separately. No implementation from assumed answers. |
| P01 | Fresh core and Unreal build setup | P00 | New project/library, headless executable and minimal Unreal host. Record reproducible commands and tool versions; launch both locally. No old game code. |
| P02 | State, IDs and typed content | P01 | Integer state, source identities, effect operations, manifest validator and versioned serialization. Round-trip representative state; reject duplicate IDs, broken references and unsupported effects. |
| P03 | Deterministic actions and events | P02 | Legal-action API, atomic costs, queued triggers, separate RNG streams, dry-run preview, logs and entry snapshots. Same version/seed/actions reproduce the state/event trace; preview is side-effect free. |
| P04 | First complete graphics-free fight | P03 | Start, craft, reserve/install, Load/unload, target, Fire, End Turn, enemy actions, win/death and restart. Reproduce the approved drones/siege arithmetic, including 20 damage → 17 after Armor and 20 Shield absorbing 18. |
| P05 | Part, recipe and character effects | P04 | Implement every operation needed by the selected recipe pool, Hot Barrel, Heat and relevant statuses. Verify the edge-case matrix below, generated-part prices and explicit exceptions. |
| P06 | Robot patterns and formations | P04 | Committed intent, fixed/seeded pattern templates, roster effects, escape, summoning and boss phases needed by the selected city. Verify gates, action eligibility, preview updates and bounded summons. |
| P07 | Collection and material economy | P05 | Deterministic steering/Precision result model, once-per-round/fight rules, timed gathering bonuses and configurable material values. Prove quantities and bonus consumption; no extra haul from another shot. |
| P08 | Permanent upgrades and Mayors | P05, P06, P07 | Typed exceptions, scoped trigger ordering, ownership/stacking, acquisition/removal rules and complete eligible Mayor/Officer/shop offers. Test explicit retention, grant, cost and death-prevention cases in the enabled pool. |
| P09 | City route, loot and Mystery outcomes | P06, P07, P08 | Gated encounter generation, counting, one boss option, rewards, memory exchange, full Mystery transactions and city result. Demonstrate a complete route with meaningful choices and no unavailable offers. |
| P10 | Shop and between-fight transactions | P05, P08, P09 | Buying resources/recipes/upgrades, core/eligible-part sales, finite stock, exactly-once restock and supplies for the next fight. Check affordability, fixed sale references and duplicate/reload handling. |
| P11 | Profile, saves and Continue integration | P03, P09, P10 | Collection, character unlocks, New Game replacement, between-fight resume, checkpoint restart and death. Crash/reload checks at purchase, reward acceptance, city clear and fight entry; no duplicate rewards or lost new supplies. |
| P12 | Complete-city simulation and reports | P10, P11 | Run recorded seeds with several simple policies; output wins, HP/resource curves, purchases, recipe/upgrade choices, turns, stalls and exact failure traces. Hundreds of fast runs only after the core fixtures pass. |
| P13 | Unreal action adapter and debug controls | P04 | Bind visible state and controls to the existing core, including action rejection and logs. Same scripted input sequence produces the headless result; no combat calculations in presentation code. |
| P14 | Player UI and campaign flows | P05, P07, P09, P10, P11, P13 | Profile/mercenary selection, intents, Recipe/Shop, readable part assembly, target/load/fire, End Turn, rewards, tutorial, death and city completion. Show exact costs/remaining Shield and explain disabled actions. |
| P15 | Claw, rig, cameras, sound and readable art | P07, P13 | Actual 16B/16C transitions, rear claw with Precision input, distinctive rig/city/robots, clear hit/Shield feedback and brisk repeat firing. Log asset rights and verify 20+ parts without a logical cap. |
| P16 | Integrated MVP candidate | P08, P12, P14, P15 | Package the selected playable boundary with its entire enabled manifest. Complete a route through the graphical build; cross-check its recorded decisions in the headless core. Finish runtime content coverage and fix integration failures. |
| P17 | Human tests and balance iteration | P16 | Observe comprehension, meaningful choices, repetition, Precision enjoyment and controller/mouse behavior as applicable. Klaus verifies graphics and approves visuals; compare observed strategies with simulation results and tune/retest changed cases. |
| P18 | Verified demo delivery | P17 | Reproducible Windows package, independent launch/play/save checks, resolution/input-focus and performance checks, documented build ID and known limitations. Close the selected MVP acceptance criteria with actual evidence. |

P13 is intentionally available after P04 even though listed later: it can run alongside P05–P12 once the core action interface is stable. P06 can proceed alongside P05; P14 and P15 can proceed in parallel after their prerequisites. If agents are assigned later, use separate core/content, Unreal/UI and art/audio paths; one integration owner maintains STATUS and the contracts. Parallel activity does not waive dependencies.

```mermaid
flowchart LR
    A[Scope and contracts] --> B[Shared core and content schema]
    B --> C[Deterministic playable fight]
    C --> D[Recipes, enemies and gathering]
    D --> E[Upgrades, city, shop and saves]
    E --> F[Hundreds of headless runs]
    C --> G[Unreal adapter]
    E --> H[Full player flows]
    G --> H
    G --> I[Claw, cameras, art and sound]
    D --> I
    F --> J[Integrated demo]
    H --> J
    I --> J
    J --> K[Human playtest and visual approval]
    K --> L[Verified Windows package]
```

## High-risk verification matrix

These are behavioral checks for the implementation, not new balance rules or tests of document wording.

| Area | Minimum representative evidence |
| --- | --- |
| Turn and cooldown | Zero-shot End Turn; multiple shots without another haul/tick; cooldown 1 blocks the next round; explicit cooling permits a legal reuse; independent copies; no-cooldown use limits and named exceptions. |
| Parts and Shield | Auto-installed Utility grants; remove/save/reinstall; first-install costs/values counted once; earlier-install history survives removal; drain 6 then 4 by 5 leaves 1 then 4; no refill; next-round delivery versus retention; pre-reset conversion. |
| Damage and characters | Whole-number combined percentages; independent spread hits in placement order; explicit payload targets; lost later hits on dead targets; zero base damage alongside the enabled character's explicit exception (Hot Barrel for Mara; Charged Barrel when Noor is enabled). |
| Costs and death | Full cost + 1 for own HP costs; failed atomic payment; ordinary recoil drains installed Shield before HP, including during Fire; lethal recoil interrupts the shot; simultaneous last-robot/player death; an enabled explicit rescue exception. |
| Enemies and RNG | Previewed intent stays committed; effects update forecasts lawfully; summoned entities cannot act early; escape warning/actions/departure; no loot for fled enemies; same seed/choices repeat after Continue. |
| Economy and persistence | Fixed price after a discount/copy/bonus; selling eligible reserves with a loaded bullet; no sale of loaded parts; saved shops do not refill; fight-end clearing preserves later purchases; no duplicate rewards; Collection discoveries survive rollback/death. |
| Content interactions | Every enabled effect has coverage plus targeted pairings at high-risk boundaries: cooling + extra uses, grant + refund, Shield spend + retention, death + reward, copied part + original-sale reference. Never claim exhaustive combinatorial coverage. |
| Simulation quality | Compare several policies and common seeds, include poor decisions, report distribution and failures rather than only average win rate. Detect infinite/profit loops and emit a trace; a runner timeout is not a gameplay action cap or forced End Turn. |
| Presentation | At least 20 applied parts on each assembly plus larger builds; clear intents and recoil forecast; accurate preview; focus/resolution changes; accessible readable text; quick repeated shots; actual human visual approval. |

## Expansion after the first accepted boundary

Use the same implementation packages to enable Ivo, Ada/Bolt and Noor, each with their own route/city data and character tests, then the remaining cities and catalogue breadth. If the owner chooses the full-campaign MVP, this expansion moves inside P16's acceptance boundary. Test progression gates per mercenary rather than reusing one identical route graph.

Add Lockdown tiers and the final reveal after ordinary full campaigns, balance reporting and persistent per-character progression work. Resolve their still-proposed entry/reveal thresholds before implementation. Implement local achievement predicates against the shared events and stable reward receipts, then the Steam adapter and platform validation when a real app/build context exists. They cannot infer completion from an uncommitted fight attempt. Keep the optional Jev experiment after legal actions, observations, policy baselines and reports are working; automated feedback comes from recorded outcomes, not an assumed model explanation.

## Current next step

Finish P00 using the pending scope answer and the settled Shield/recoil rule. The immediate follow-up deliverable is a concrete enabled-content manifest derived from the existing framework, using the completed beta-balance-v1 fixtures. The farming/overtime answer remains separate. Do not ask the owner to establish progression/content pools again. The [18 timing/save traces](TIMING-AND-PERSISTENCE.md#7-worked-traces-and-future-runtime-checks) are specified and will become shared-core checks during implementation. Then request/receive the owner's next implementation goal under the existing step-by-step workflow. This planning request does not authorize starting the engine project automatically. No balance, gameplay or graphics approval is claimed by completing this document.
