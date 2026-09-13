# Magnet Sweep — a rig worth building

Integrated recommendation, 13 September 2026, Europe/Oslo. This describes a proposed successor mode. The playable package remains Extraction E1 / 0.5.0; the expedition system below has not been implemented or playtested. No development deadline governs this design. Existing careers and the current owner session remain separate.

## Decision: keep the magnet, change the game around it

The old permanent shopping ladder and loose-metal tray cannot support the requested depth. More entries would still lead toward the same fully purchased rig solving the same problem. Attraction, swinging weight, metal collisions, selective release and banked rewards are useful foundations.

**Build a salvage machine during an expedition, use its interacting tools to recover increasingly difficult machinery, and extract a secured machine core to win.** Each run presents different physical opportunities and equipment choices. The ultimate tool is the powerful rig assembled for that expedition, not one final item replacing every other build.

An upgrade must answer the owner's question: **“What can I now accomplish that my previous rig could not?”** A larger radius or faster animation by itself does not pass. A support may amplify an action when the combination creates a new trajectory, target, preserved object, mechanism or useful chain. Its description must show that consequence.

Keep one readable worksite at a time, a compact workshop between sites and two equipped tool buttons. No lore campaign, open world, combat roster, ingredient tree or extra progression currencies are needed. Depth comes from metal, supports, current and momentum interacting with the tool rules.

## What playing a run would mean

At departure, choose a free starter tool and spend 12 run credits on a support or another tool. Attraction, release, inspection, unloading and basic physical manipulation are universal. Four starters are initially available: extraction, launching, electrical discharge and winching. Directional repulsion and remote fields expand starter choices through discovery. No permanent power grind is needed before the game becomes interesting.

Each of the first three sites has one marked physical objective, optional valuable salvage and a depot reward. Candidate objectives are an 8 kg gearbox, a 12 kg pump and a 16 kg drive assembly. These are encounter specifications, not existing content. They combine welded ballast, blocked routes, live branches, unstable supports and telegraphed machinery. Before choosing the next site, see its relevant conditions and named optional reward.

The fourth site ends in freeing and delivering a 20 kg core. It fits an empty baseline 24 kg rig; a capacity purchase is not a mandatory key. The challenge is its securing mechanisms and transport route. Progress belongs to actual clamps, supports and moved obstructions visible from entry.

Between sites, buy from four saved offers, freely refit and consider half-price resale. **The recommendation is fresh run equipment with permanent discoveries, unlocked options, records and archived winning rigs.** An archived rig can be inspected and used in practice. This remains the working progression choice: Klaus has not selected between it and a persistent workshop/rig alternative. No old save is converted.

Replay means trying the attractive purchase declined last time, discovering another interaction, preserving a device previously smashed or completing an optional risky recovery. Winning must feel worthwhile before collection completion or challenge modifiers are added.

## Six ways to play

| Rig family | Player decisions | New accomplishment and tradeoff |
|---|---|---|
| Surgical recovery | Aim at a connection, choose release order, cut and pluck a selected piece | Free value from unwanted ballast and preserve usable machinery. A careless load-bearing or live cut has a physical consequence. |
| Junk artillery | Collect expendable metal, weld a projectile, choose mass and aim | Break casings, move braces or reach around cover through a rebound. Ammunition and delicate salvage can be damaged. |
| Circuit builder | Place conductors, choose branches/grounds, then pulse | Actuate remote latches or make a finite source operate several mechanisms. Poor routing can energize something worth preserving. |
| Heavy rigger | Position anchors and real ballast, tension a line, sequence a tow | Move an intact awkward assembly while maintaining support. Setup occupies space and can obstruct another route. |
| Vector operator | Aim a shove, rotate a group and use placement/impact | Clear a corridor, wedge machinery or redirect material without collecting everything. A wrong angle makes the next approach harder. |
| Field architect | Place bounded source/receiver fields and choose transfer routes | Recover across separated or hazardous regions and split a real load between destinations. Reach, endpoints and energy constrain the operation. |

These are different actions and spatial problems. A family earns its place only when built encounters support its decisions and players use them. Every mandatory site has a basic route; optional opportunities may strongly favor one family. Two active slots allow hybrids without class locks.

## Equipment and extensibility

Fit **two active tools and four passive sockets**. A keystone occupies two passive sockets, competing with a pair of useful cheap interactions. There is no additional equipment power meter. The shop previews which effects replacing an item would disable.

| Price tier | Credits | What it buys |
|---|---:|---|
| Focused support | 4–6 | Rebound Plate gives a physical reflection; Ground Clip terminates an electrical branch at chosen ground. |
| Specialist | 8–12 | Another active operation, welding selected scrap into a slug, a conductive tether or second anchor. |
| Advanced support | 14–18 | A broader interaction such as an induction bridge, shear gate or Gantry's paired transport. Gantry costs 14 and occupies two sockets; compare its handling benefit against cheaper supports. |
| Keystone | 22–26 | A major transformation such as a real multi-branch electrical operation. Two passive sockets; never required to win. |

Price and offer frequency are separate. A situational 5-credit part may be the better purchase than a 24-credit keystone. Normal duplicates do not stack. No automatic three-rank ladders: another rank would need a new consequence.

The [mechanics catalogue](BUILD-SYSTEM-MECHANICS.md) defines **30 distinct starting modules**, including six active tools and five keystones, with triggers, targets, costs, risks and required world opportunities. Walking Gantry is now a 14-credit advanced specialist: its controlled frame recovery used 20 battery versus Hook + Ratchet's 22, both preserving 320 appraisal. That small advantage did not justify the former 24-credit ultimate classification. Thirty is a seed catalogue, not a ceiling or a claim of finished content. The [economy proposal](BUILD-SYSTEM-ECONOMY.md) and [machine-readable scenarios](build-economy.json) supply the priced choices; the historical 24-credit receipts remain valid.

A module composes **when it happens → what it affects → what it does → what it consumes → what explains the result**. Shared primitives include severing/joining real pieces, impulses, impacts, finite charge, tethers, support constraints and bounded transfer. New combinations of those primitives can become data-defined modules. A new world behavior, such as a functioning portable machine, requires implementation; naming it in data does not create it.

This gives the catalogue room to grow without promising literally infinite authored upgrades. Every addition still needs a clear explanation and a useful physical opportunity. The mechanics paper includes a concrete Cold Seam definition and the boundary between reusable data and new code.

## Combinations change the operation

**Extraction + discharge:** a narrow terminal gap cannot accept a bulky welded bundle. Extract its conductor, place it across the gap and pulse the resulting path to release a remote latch. Extraction alone cannot energize it; the electrical tool alone lacks a conducting path. Both ingredients are useful independently.

**Welding + winching:** loose small pieces cannot be attached as one stable counterweight. Join their conserved mass, attach the resulting body to a hook and support an intact machine. The same slug could instead be ammunition. Its purpose depends on the rest of the rig and the player's plan.

The equipped Rail button explicitly shows **Weld: 4 battery**, then **Launch: 8 battery** as two separate commitments. Welding never fires or arms a shot automatically. Press the other equipped tool button to use the retained paid slug with Winch; pressing Rail again deliberately launches it. RMB always drops the haul recoverably, including a welded slug, and reacquiring it costs another real operation. Before committing a weld, changing aim changes no material or battery.

**Launching + rebound + discharge:** aim expendable conductive metal around cover so it bridges separated contacts, then energize it. The rebound must change the actual trajectory and the scrap must physically span the gap. An automatic nearest-target zap is insufficient.

**Precise release + Intact Recovery + winching:** preserve a functional assembly at its final release and transport it with its useful state intact. Keeping the device working enables an operation that shattering or smelting removes. Its function and finite energy must actually exist; this requires the new functional-assembly primitive.

Show a concrete combined preview: affected pieces, direction, total energy and foreseeable danger. Celebrate a successful chain through sequential sound and motion, with brief causal feedback such as “Counterweight joined — support held — pump recovered intact.” Floating numbers cannot substitute for physical impact.

A manual action has one identity. Reactions retain original material, source and target identities. A source depletes once; splitting/welding conserves mass and value; banking pays once. Generated effects may trigger compatible children but cannot impersonate fresh manual input or repeatedly consume the same edge. Preserve big finite chains. The mechanics paper specifies ordering, costs and save semantics.

## Affordable choices and useful rewards

Guaranteed pre-finale income is **48 credits**: 12 initially plus 10/12/14 from site completion. Two visible refining thresholds per site each pay 2 credits once, giving **60** maximum ordinary income. Proposed thresholds are 120/240, 180/360 and 240/480 output. These need actual salvage and battery route witnesses; budget arithmetic cannot establish physical reachability.

Better recovery changes buying power now. A player with 4 credits who earns Site 1's clear reward has 14; crossing both refining thresholds leaves 18. That buys a 10-credit second tool and 5-credit support together. Show the payment when smelting and connect the reward to the purchase. Ordinary mixed scrap does not cause an undisclosed purity failure.

At each depot, generate and save four distinct offers:

1. An unowned support compatible with an installed capability and a real opportunity on the previewed next site.
2. An independently useful alternative, often another active, with an affordable legal fitting plan and no missing prerequisite.
3. A weighted wildcard from another family; explicit missing prerequisites prevent it counting toward the guarantee.
4. A compatible specialist/keystone to aspire to, without guaranteeing the whole desired combination.

The first two must each be affordable at arrival and support different decisions. Matching colours or technical equipability are insufficient. Reject stock without a qualifying pair; do not disguise a content gap with battery purchases. Measuring how frequently useful legal stock can be generated remains implementation work.

There are **no rerolls** in this experiment. Reload/retry does not change stock. A non-stacking 4-credit precharge starts the next site at 120 instead of 100 battery, but is not a substitute for module choice. Resale is half the actual paid price, rounded down; free/found items return zero.

The initial demolition route exposed a poor late shop: 21 credits remained because offers were weak fits. That stock is rejected. Rebound Plate, Vector Emitter, Anchor Winch and Salvage Cyclone instead offer a corner shot, a new second operation or a costly transformation. The revised routes and verifier report carry the final arithmetic; passing them does not certify victory or random-shop quality.

After both refining thresholds, leave ordinary residue unless it serves the operation or a personal record. Direct exceptional performance toward **one previewed optional recovery with a named reward**, such as a found module or blueprint. It need not appear at every site, and found modules cannot be sold for newly minted cash. Whether the bounded cash rewards remain exciting is a playtest question; another currency would not answer it.

## Risk and recovery

Provisional values make the design testable; they are not settled balance. A site begins with 100 battery. Moving secured cargo, planning, releasing and unloading are free. Ordinary attraction costs 6 per deliberate valid activation. Invalid/empty input costs nothing; secondary costs are previewed. There is no idle recharge or furnace-pour tax.

Use 24 kg safe capacity and 36 kg hard limit. An unsafe committed haul starts a visible **3-second fuse**. RMB releases before expiry without additional cost; the already paid operation and disrupted setup are the loss. Reacquiring material costs another action. Expiry trips the tool, spends **12 battery** (clamped at zero) and spills the haul recoverably. No arbitrary most-valuable-piece deletion. A fresh action is required to resume, and one incident charges once.

Crushers, swinging machinery and live branches damage the actual struck fragile salvage. Show the contact area and foreseeable danger. The unique objective remains recoverable, although its preservation grade can fall. Mandatory layouts need recovery paths from ordinary spills; a dropped core cannot leave the reachable play area.

Zero battery is not instant defeat: a secured objective can still be delivered. If further paid manipulation is necessary and no accessible finite source remains, retreat or retry. Retreat ends the expedition and forfeits the unfinished site objective, unbanked haul and provisional discoveries. Permanent discoveries/records from dispatched sites remain. Run credits have no permanent cash conversion.

**Retry site** restores the whole site-entry checkpoint: world, battery, wallet, installed equipment, stock/random state, claims and provisional discoveries. Rewards since that checkpoint roll back too. It cannot refresh energy while preserving profits. The archive distinguishes a recovery with retries from a first-attempt record without withholding core content. Permanent discoveries commit only on dispatch.

Unpaused world time advances hazards, projectiles and the cargo fuse. Planning/pause freezes them together, including on focus loss. It allows inspection and ghost previews, not free world movement. Resume preserves exact elapsed state and paid costs. There is no overall countdown forcing rushed reading.

## A physical final operation

The core has three readable physical constraints:

- Pull the preload collar along its guide to slacken the upper support.
- Move real ballast into a catch to counterbalance the second latch.
- Insert an iron brace before the sweeping arm pins the third mechanism, then use the safe phase.

E confirms a state actually achieved through physical manipulation; it is not three paid hold bars. A press lane separates the released core from its receiver. The baseline uses normal attraction, placement and timing with a substantial battery margin. The mechanics paper owns exact geometry and complete fallback requirements.

Different rigs can cut a release path, break expendable bracing, energize a latch, counterbalance/tow, reposition a stopper or transfer across the dangerous region. Upgrades open preservation, access and geometry outcomes missing from baseline play. If all approaches become the same motions with different effects, change the encounter. If a rare module is required, change the encounter.

Connections need visible mechanical meanings: weld, hinge, support or wire. The old unexplained “links” label is insufficient. The objective and receiver are visible before the player is asked to collect anything.

## Tutorial and presentation

Teach a physical goal first, then a starter ability that changes a real result. Let the player see a welded bundle's full mass and use the capability to accomplish something different. Lessons respond to actual actions and show what the banked reward can buy.

Proposed controls: baseline attraction on LMB/Space, two active slots on Q/F, contextual service/unload on E, whole-haul drop on RMB and a labelled planning pause. Prompts must distinguish **smelt scrap** from **deliver objective**. The furnace leaves mission objects intact and explains their separate receiver. Releasing LMB or switching the field off retains secured cargo. RMB drops the whole haul recoverably onto the worksite. E at the furnace banks scrap. Teach those different results visibly; no context silently changes RMB into selective retention.

Attraction needs acceleration, rotation, heavy settling and differentiated impacts. Electrical effects follow their actual path; a taut winch visibly bears weight; a slug changes the struck support. Sound follows causal events, with calming music underneath and independent level controls. No new art/audio was made or perceptually verified in this design increment.

The energy proposal uses previewed multi-group captures, not single-piece clicking. Its maximum physical pull window is 1.2 seconds, ending earlier when settled. **Compare it with paid continuous attraction in the playable experiment.** Field feel is more important than defending the first accounting proposal. Both variants need real targets, visible cost and finite energy.

## Research decisions

Primary descriptions/developer notes were inspected on 12–13 September 2026. These are design references, not playtests or commercial evidence. Specialist papers retain dates and limitations.

| Reference | Transfer | Do not import |
|---|---|---|
| [Slay the Spire](https://store.steampowered.com/app/646570/Slay_the_Spire/) | Select interacting parts during attempts; routes and rewards shape a build. | A card interface or enormous content count merely to imitate its structure. |
| [Balatro](https://www.playbalatro.com/faq) | Separate winning from collection; limited equipment slots make parts compete. | Score escalation disconnected from useful consequences. |
| [SULFUR](https://store.steampowered.com/app/2124120/SULFUR/) | Layer tools and modifications that change delivery/consequences with visible tradeoffs. | An FPS or loss of the owner's old career. “sulfpher” is interpreted as SULFUR. |
| [Hollow Knight](https://www.teamcherry.com.au/blog/revealing-the-power-of-the-charms) | Collecting options differs from equipping a limited subset. | A large exploration world or lore. The charm source is prerelease, not current balance proof. |
| [Inscryption: Kaycee's Mod](https://steamcommunity.com/games/1092790/announcements/detail/3132822729058776859) | Repeat attempts reveal combinations; developer patches illustrate limiting particular self-copy loops while retaining others. | Mystery narrative or suppressing all secondary effects. Detailed patch sources are in the critique. |
| [Hades](https://www.supergiantgames.com/blog/hades-superstar-update-patch-notes/) | Base action, transformation and reactive support create recognizable builds; manual triggers need clear semantics. | Combat enemies or boon labels without corresponding play. Historical notes supply examples. |
| [Noita](https://noitagame.com/) | Shared world properties make composed effects surprising but explainable. | A complete particle simulator or unreadable lethal chaos. |
| [Dice A Million](https://store.steampowered.com/app/3430340/Dice_A_Million/) | Give strong combinations a finish line; combine active objects and passive rules. | Large numbers as the only evidence of power. |
| [Nova Drift](https://www.novadrift.io/) | Builds change actions/chained outcomes; [developer discussion](https://blog.novadrift.io/wild-metamorphosis/) connects variable offers with adaptation. | Theoretical combination counts as proof of viable builds. [Patch history](https://blog.novadrift.io/patch-notes/) also motivates explicit effect ownership. |

The three experts exchanged and challenged proposals. [Mechanics](BUILD-SYSTEM-MECHANICS.md), [economy](BUILD-SYSTEM-ECONOMY.md) and the independent [gameplay critique](BUILD-SYSTEM-CRITIQUE.md) preserve evidence and dissent. Critique changed overload loss, shop guarantees and the core interaction.

## Production sequence and evidence

This is ordered by questions to resolve, not a timetable.

1. Implement an isolated expedition model and mixed worksite, preserving the old career. Establish battery, objective, furnace/receiver distinction and complete save/retry transactions.
2. Build extraction, launch/weld, conduction and tethers on shared pieces. Demonstrate the conductor and counterweight combinations, including mistakes and conservation. Compare continuous and bounded attraction.
3. Integrate an initial shop, an earned late pivot and the physical core operation. Use real purchase paths, not debug-granted final rigs, to test whether rewards change the next approach.
4. Implement repulsion and remote fields and prove all six approaches through different inputs, geometry and tradeoffs. Add modules as their corresponding world opportunities exist.
5. Independently play for comprehension, impact, meaningful risk, readable combinations and voluntary replay. Expand sites/collection after the interaction is worth repeating.

The mechanics experiment is a subset, not a redefinition of the full goal. A player should explain what a purchase enabled, predict a combination, recognize its reward and name another rig they want to try.

The [economy verifier](../scripts/VerifyBuildEconomy.py) and [report](../evidence/build-economy-verification.json) pass **30 catalogue entries, six routes and 24 shops**, ending with **1 / 16 / 2 / 0 / 3 / 2 credits**. Checks cover prices, offers, cash, resale, slots, once-only payments, equipped capability prerequisites and an affordable current-tool support alongside another compatible option. Six deliberately invalid scenarios were [rejected](../evidence/build-economy-invalid-scenarios.json), including forced-pivot stock, duplicate payouts/precharge and missing required tools. Independent final critique found no remaining blocking paper inconsistency after control and stock repairs. They do not simulate mechanisms, prove refining thresholds attainable, validate generated shops or measure fun. Extraction E1's 21 passing tests concern the previous implementation only.

**Conclusion:** this magnet concept can support the requested design if its physical problems and run structure change with the upgrades. The static tray cannot. The recommendation is concrete enough to implement and challenge; enjoyment and broad build viability remain unproven until the new playable experiment exists.
