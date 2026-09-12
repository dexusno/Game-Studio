# Magnet Sweep — revised extraction loop

2026-09-12. Design contract for root integration after Klaus rejected the playable demo. The owner explicitly requests implementation of challenge, risk, rewards, real attraction, improved presentation, music, upgrades, levels and loot. Earlier unrestricted-cargo/no-risk/no-economy rules are superseded. No development-time ceiling applies. This document owns interaction and risk rules; progression numbers and content are coordinated with the progression designer. These are hypotheses awaiting a new playable build and owner observations.

## Promise and complete loop

**Pull valuable scrap out of a dangerous jumble, protect a valuable load, and turn four fuel charges into a better magnet.**

1. A contract opens with its required banked value, optional gold target, four visibly full furnace fuel charges, and an enticing rare salvage object. All remain readable during play.
2. Hold LMB or toggle Space to activate the magnet. Pieces visibly rotate, slide, accelerate and gather onto its actual position. Release a held field or toggle it off to stop attracting; carried scrap remains attached. Hold Shift or toggle Q to narrow the field for separating valuable scrap from an adjacent hot cell or avoiding an unwanted heavy bundle.
3. Choose a load. Cheap iron is easy and bulky; copper and alloy offer more value per kilogram. Physically linked pieces move as a bundle with a shared mass/value preview. A valuable bundle may fit an empty magnet but overload the current haul.
4. Bank a stable current load using one fuel charge, or use RMB **Drop haul** to release the entire unbanked load recoverably and rebuild it. Overcapacity or an attached hot cell starts the instability fuse. Its warning forecasts one lost fuel charge and the exact valuable piece at risk. Switching the field off alone does not make an overloaded load safe.
5. Click the furnace to pour. The physical stream melts, an ingot forms, coins and XP count into their destinations, and the order meter visibly advances. A pour consumes one of four charges. A completed order pays its promised reward and unlocks the option to finish safely or use remaining charges to pursue gold/rare loot.
6. Spend earned credits on a visible rig improvement, take another contract with a changed arrangement, and try a different extraction decision. Quitting retains the real state; starting a new job does not silently refund losses or duplicate a completed pour.

Every successful pour uses one fuel charge. Every expired instability fuse also uses one charge for an emergency quench. The job fails if all four charges are used below the required banked value. If the quota was already earned, exhausting fuel ends the attempt without revoking that success. Previously banked credits and XP remain owned; there is no negative wallet, integrity meter, passive countdown or field-energy meter. The contract preview must also detect when surviving salvage cannot reach the quota and offer an explicit failed-job finish/retry rather than leave an impossible board active indefinitely.

## Controls and immediate response

| Input | Immediate response | Meaningful decision |
| --- | --- | --- |
| Pointer movement | Actual magnet moves toward a visible target; carried pieces follow with controlled motion. | Choose approach and return route. |
| Hold LMB or toggle Space | Radial field attracts every eligible nearby piece, including marked hot cells. | Broad gathering is fast but can take unwanted mass or danger; toggling avoids continuous mouse grip. |
| Hold Shift or toggle Q | Field visibly narrows while attraction is active. | Trade coverage for control near a valuable seam. |
| Release the held field or toggle Space off | Field switches off; free pieces settle at their current positions and cargo remains attached. | Stop drawing in more danger without undoing the successful part of a pull; attached instability persists. |
| Press RMB: Drop haul | Release **all** unbanked pieces, including hot cells, onto the tray recoverably and switch the field off. No fuel is spent. | Save fuel and rare salvage from a discharge, at the cost of rebuilding the chosen batch. It does not sort the valuable pieces for the player. |
| Click furnace while carrying salvage | Accept one pour once, credit the actual load, consume one charge, play payoff. | Lock in progress now or try for a more valuable batch. |

No ring-drag capsule or invisible direct-neighbor selection survives this redesign. A chain means those visible pieces are physically connected and will be pulled together; highlight the entire bundle, its total kilograms and credits. An empty field pressing near a ring performs the same attraction as pressing elsewhere. Do not introduce a separate chain-cutting upgrade without implementing and explaining that action.

Incoming load forecasts use actual eligible pieces and bundle mass, not decorative approximate highlights. A marked hot cell has a unique silhouette and red glow before attachment. Ordinary scrap stays desirable because it earns cash; finite pours make its opportunity cost visible without labeling a normal pickup a mistake.

## Settled starting tuning

| Rule | Initial value |
| --- | --- |
| Furnace charges per contract | 4; an empty click consumes none |
| Starter stable capacity | 24 kg; +8 kg per capacity upgrade tier, three tiers |
| Absolute attached-mass limit | 150% of current stable capacity; a whole bundle above this limit remains on the tray |
| Field radius | 110 world units; +35 per coil tier, three tiers |
| Attraction strength | Base 100%; +25% per coil tier |
| Shift precision radius | 50% of current field radius; available immediately |
| Instability fuse | 3 seconds; +1 second per stabilizer tier, three tiers |
| Expired fuse | 1 fuel charge consumed; exact highest-value ordinary cargo piece destroyed; remaining ordinary pieces recoverable |
| Starter iron | 2 kg / 4 credits |
| Starter copper | 3 kg / 12 credits |
| Starter alloy | 4 kg / 24 credits |

Engine integration confirmed that continuous attraction, moving piece positions and RMB control are feasible. The exact motion curve and magnet speed need native tuning. Root selected a spring-follow target with a finite speed near 1000 world units/second on the 1000-unit-wide tray; do not teleport the magnet and cargo with the cursor. This is a motion hypothesis, not an observed successful feel. The furnace refuses an unstable load, closing the teleport-to-bank loophole. Drop haul deliberately gives no automatic value sorting: careful extraction preserves a chosen batch, while a rescue requires rebuilding it. Avoid a second severe-overload timing band in this iteration.

## Exact risk and recovery

- Instability starts when **actual attached mass exceeds capacity, or a hot cell is attached**. The same fuse handles both conditions. It continues while overloaded with LMB released. Pause/focus loss freezes simulation and requires fresh input on resume.
- Show a red arc and a concise forecast: `UNSTABLE — 30/24 kg — 2.1s — lose 1 fuel + alloy core (48 cr)`. The highest-value ordinary cargo piece is the at-risk piece; equal values resolve by stable piece ID. With only a hot cell attached, forecast just one lost fuel charge. The warning, loss and resulting save must agree.
- RMB **Drop haul** releases every attached piece, whether the current load is stable or unstable, and switches the field off. All dropped pieces, including hot cells, remain recoverable. It spends no fuel, destroys nothing and leaves no automatically selected valuable load behind. The label must make the whole-haul consequence explicit before use.
- The drop forms a readable nearby spill. Do not immediately recapture it or scatter it off the tray. Pieces must be separated enough that precision can recover a wanted item without inevitably taking the hot cell again. Test this at tray edges: individually clamping a radial spill can stack pieces into an unfair mixed pile.
- If the fuse expires, consume **one existing fuel charge**, visibly destroy the forecast highest-value ordinary cargo piece and consume attached hot cells. Remaining ordinary cargo spills onto the tray recoverably. Show the fuel used and exact lost name/value. Banked money, XP, upgrades and already banked collection items are untouched. This closes intentionally discharging a zero-value cell to remove a hazard for no cost.
- The furnace refuses an unstable load and shows `UNSTABLE — DROP HAUL FIRST`; refusal consumes no fuel and awards no credit. A stable accepted load credits exactly once and its animation creates no hidden extra deadline. A whole-load preview says how much the quota, wallet and XP will gain and shows the charge being spent.
- If no valuable cargo is attached when a hot cell trips, consume the cell and one fuel charge; do not invent a wallet debit or valuable-piece loss. The opening job should retain a realistically achievable recovery route after one failed risky move.

This creates a specific dilemma: with 12 kg already attached, an 18 kg linked prize bundle would produce 30/24 kg. Banking the first load spends a charge but preserves its value. Dropping it allows a different chosen batch but requires collecting again. Attempting to keep the complete 30 kg load starts a predictable fuse; that unstable load cannot be poured. A timely Drop haul saves the fuel and valuable pieces, while failure costs one fuel and the best item. The decision is to assemble a valuable stable batch, not repeatedly gather everything and automatically retain its best pieces.

## Readability, challenge, pacing and appeal

- **Readability:** order progress and projected pour value are the main HUD goal; four fuel icons sit beside the furnace. Cargo kg/capacity stays beside the magnet. Instability temporarily takes visual priority. A modest level/XP bar and next affordable upgrade explain lasting progress without covering the tray.
- **Challenge:** field footprint, mixed value/mass, connected bundles and hot-cell position must change useful approaches. Each job needs a conservative feasible route and a more valuable hazardous opportunity. Do not scatter identical pieces and call the resulting layout a new challenge.
- **Pacing:** the first useful gathering occurs immediately, and the first pour gives unmistakable money/XP feedback. Allow looking and planning with no countdown. Avoid compulsory last-fleck cleanup and repeated empty transit. First contract completion must enable an affordable meaningful upgrade.
- **Appeal:** field activation needs a rising electrical hum, nearby metal tremble, acceleration into the magnet, separate impact ticks and an audible mass change. Drop haul has a clear release pulse and landing sounds; loss is distinct from a recoverable drop and successful collection. The furnace has a coherent pour/melt/ingot sequence with bass, metallic detail, warmth and reward audio. Soothing music sits beneath these events, with independent music/SFX volume controls. These are experience requirements for the presentation owners, not proof that the current effects satisfy them.

## Acceptance behaviors and short playtest

1. From a fresh job, a player can identify the required goal, remaining pours and next reward without reading a design document.
2. A visible piece moves from its current position toward the actual magnet before becoming cargo. Releasing the field interrupts free-piece attraction rather than rewinding it; attracting a bundle forecasts and acquires the connected group consistently.
3. Broad and precision attraction produce observably different results in a copper/hot-cell seam. A correctly positioned precision pull can take the copper while leaving the cell.
4. A 30 kg load on the starter rig remains unstable after field deactivation. Drop haul releases every attached piece recoverably, turns attraction off and spends no fuel. Deliberately ignoring the warning consumes one fuel, destroys the forecast piece and scatters the rest; recovering survivors never recreates the destroyed piece. A hot-cell-only expiry still consumes one fuel.
5. Four poor-value pours, or fewer pours with wasted quench charges, can visibly fail a contract. A conservative valid route reaches the base goal; a selective approach can improve its outcome. The same route should not automatically achieve every optional target with every rig. One opening-job failure must not make recovery depend on perfect play.
6. A melt awards the advertised credits and XP exactly once, displays the forged result, and can unlock/purchase an upgrade whose effect is visible on the next pull. Save/reload preserves owned progression and consumed fuel.

Ask after actual play: **“Show me a moment when you chose to bank, drop your haul or change how you collected. What made that choice worth making, and what would you try differently next time?”** Also record whether rescue felt like a useful correction or tedious recollection, and whether the player voluntarily starts another contract after the first upgrade. Directed QA completion, designer preference and screenshots do not establish fun.

## Peer critique, exclusions and next iteration

The loop and progression designers agreed on four furnace charges instead of a field-on battery timer. The gameplay critic challenged stacked meters, load danger disappearing on release, opaque loss, unchanged old ring rules and cursor teleport making every emergency safe. Root resolved these with one persistent fuse, exact highest-value loss, physical bundles, precision controls and no integrity meter. Subsequent review found two structural loopholes: intentional hot-cell-only failure removed a hazard for free, and selective emergency release automatically kept the most valuable load. Root accepted one fuel charge lost on every expired fuse and replaced selective release with whole-haul recoverable Drop haul. No new resource was introduced.

The critic's R1 native observations showed narrow attraction isolating a core and 9 kg linked bundles, while a broad pocket pull took a core, washer and both cells. These observations support a spatial role for precision; they do not prove that the new drop behavior is enjoyable or that automatic sorting dominated actual play. The remaining acceptance concern is tedious forced recollection after incidental contact, especially edge-clamped mixed spills. Verify readable, controllable recovery and meaningful batch selection in the revised build. Motion speed, quota balance, attraction feel and voluntary replay remain playtest hypotheses.

Exclude lore, crafting trees, many currencies, durability repair, daily tasks, online services, separate energy/health systems and a large loot catalogue. The smallest next iteration is one complete contract containing a safe opening, a mixed-value seam, a dangerous connected bundle, four fuel charges, visible fail/success, one banked rare item and the first purchased rig improvement, implemented in the actual scene with representative attraction/audio. Expand the contract set only after this complete loop works; this is an integration order, not a stop short of the owner's authorized implementation goal.
