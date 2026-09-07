# Sixfold Recoil — full proposed game design, v0.2

**Revised:** 2026-09-07, Europe/Oslo. **Stable opportunity ID:** `scrapstorm`. **Stage:** design exploration; still shortlisted. No game has been selected, built or playtested.

This is the current complete design for Klaus's 17 requested topics. It supersedes the [archived v0.1 brief](v0.1.md), incorporates the [integrated review](../2026-09-07-scrapstorm-review/README.md), [gameplay review](../2026-09-07-scrapstorm-review/gameplay-review.md), [commercial review](../2026-09-07-scrapstorm-review/commercial-review.md) and [coordinator audit](../2026-09-07-scrapstorm-review/coordinator-evidence/audit.md), and carries forward Klaus's correction: **there is no hard development-time or owner-time cap.** Earlier day/hour figures in preserved history are superseded planning hypotheses, not current ceilings or estimates of AI delivery.

The recommendation is a compact **arcade mastery game with light run builds**. The desired product has three arena courses, four ordinary enemy roles, a Foreman finale, eight upgrade definitions with three choices per run, and six fixed challenges. Its first experiment is much smaller: one arena, six pieces, two enemies and a finite clear. That experiment isolates the core; it does not set the shipping inventory. USD 4.99 remains the price hypothesis, with no demonstrated case for a higher price yet.

**Current rules baseline:** harmless global recall, full movement, and recall taking priority over throw while held. Damaging recall and one alternative input rule are precisely specified comparisons below. Keeping them separate lets a build test what actually helps. All numeric rules are starting tuning values; all imagined player actions are illustrations. Clearer prose has not raised fun confidence.

## 1. Elevator pitch / hook

**Your armour is your ammunition. Aim it. Throw it. Get it back.**

Sixfold Recoil is a top-down action game about Pip, a tiny magnetic robot whose six pieces of scrap are both shield and weapon. Aim a volley into hostile machinery, dodge while exposed, and pull the same objects home. Learn when a small recovered volley is enough and when to keep your protection.

The intended pleasure is **clack → impact → exposed movement → six satisfying catches**. Better aim, shorter recovery routes and well-timed attacks should make a familiar rhythm increasingly rewarding. A surprising tactical choice on every volley is unnecessary; wanted practice and improvement matter.

## 2. Genre and platforms

- **Genre:** single-player arcade arena action, with temporary upgrades and bounded encounters. “Light roguelite builds” describes a secondary feature; a huge randomized build catalogue is not the promise.
- **Proposed first release:** premium, offline Windows PC game on Steam. Keyboard/mouse and gamepad are both product requirements. No engine is selected by this design task; deterministic 2D movement and swept collision are sufficient requirements for choosing one later.
- **Session structure:** choose a course, clear five encounters, finish the Foreman, then retry or choose another goal. Active duration emerges from kills, movement and skill. There is no six-minute script, survival countdown or promised minimum playtime.
- **Other targets:** Linux, Steam Deck verification and consoles remain possible later increments with their own measured value/cost. Their support is not claimed.
- **Authorization:** this revision authorizes no implementation, concept selection, purchase, outreach or publication. A subsequent playable task can use this complete specification directly.

## 3. Target audience and comparable titles

The primary player likes independent movement and aiming, readable danger, fast retries and learning a small action vocabulary. Arcade record chasers are a closer fit than players mainly seeking hundreds of synergies. Arena-roguelite players are an adjacent audience. People interested in miniatures may like the identity; their hobby interest does not establish demand for combat.

These are the review's **September 7, 2026 Oslo-date observations, US region, USD regular / then-advertised price**. The preserved [commercial snapshot](../2026-09-07-scrapstorm-review/commercial-evidence/steam-snapshot.json), [Rocket Fist snapshot](../2026-09-07-scrapstorm-review/commercial-evidence/steam-rocket-fist.json) and [gameplay snapshot](../2026-09-07-scrapstorm-review/gameplay-evidence/steam-comparators.json) govern the numbers. They are dated evidence, not a promise of today's checkout price or a hands-on evaluation.

| Comparable | Regular / advertised USD | Design implication and limit |
|---|---:|---|
| [Akane](https://store.steampowered.com/app/884260/) | 4.99 / 4.99 | One arena can support a paid mastery game. Its advertised five enemy types and equipment goals strengthen the case for distinct threats and goals here. It does not prove our loop or value. |
| [Rocket Fist](https://store.steampowered.com/app/413500/) | 4.99 / 4.99 | Robot throw/exposure/recovery is already a recognizable mechanic. Its social modes and larger solo adventure differ; 29 Steam-purchase reviews in the review snapshot show limited visible response, not known sales or failure. |
| [Boomerang Battles](https://store.steampowered.com/app/2337660/) | 5.99 / 5.99 | Returning weapons and an advertised 100 rooms do not guarantee attention; the snapshot has six qualifying reviews. Useful variety must earn play, and discovery remains separate. |
| [Brotato](https://store.steampowered.com/app/1942280/) | 4.99 / 2.99 promotion | Large build inventory at a low price is strong competition. Manual aiming already exists there; our distinction is spending and recovering the same physical protection. |
| [SNKRX](https://store.steampowered.com/app/915310/) | 2.99 / 2.99 | A simple-looking control scheme can accompany extensive builds. Our proposed eight cards should be sold as modifiers of technique, not equivalent breadth. |
| [Devil Daggers](https://store.steampowered.com/app/422970/) | 9.99 / 9.99 | A single arena can command more when movement, threats and learning support it. Its advertised 13 enemies and replay tools do not establish a $9.99 case for our untested product. |
| [CROSSBOW: Bloodnight](https://store.steampowered.com/app/1329540/) | 2.99 / 0.74 promotion | The review's targeted player reports raise danger readability and input friction. They motivate tests here; they are attributed experiences, not reproduced current defects. |

The purposive review sample covers twelve distinct priced games and includes weak responses. Overlapping reviewer samples are not independent votes, review counts are not paid units, and sale purchases do not establish regular-price acceptance. [Titan Souls](https://store.steampowered.com/app/297130/) and [Boomerang X](https://store.steampowered.com/app/1170060/) remain retrieval/movement references above the owner's price ceiling, not pricing targets.

## 4. Detailed core gameplay loop

1. **Read the next danger.** Notice approaching jaws, a locked shooting lane, held scrap and landing positions. A full ring offers protection but does not hurt enemies.
2. **Make an angle.** Group targets, choose a useful wall angle or get around a divider. Stay close enough for a quick catch if that route is safe.
3. **Commit at a useful moment.** Keep pieces through a threat, or throw to remove it and evade while empty. The same move can be comfortable and repeatable when its execution is satisfying.
4. **Launch what is held.** One press launches every attached piece. Aim and fan coverage determine hits; a six-piece volley does not guarantee six useful hits.
5. **Move and recover.** Loose pieces collect nearby or return globally while recall is held. Flying pieces must finish their outbound flight first. Harmless baseline returns are recovery, not another automatic attack.
6. **Use an opening.** Release recall and fire the pieces already home, or wait for more protection and coverage. Pieces that hit nearby can arrive before distant misses; evenly landed pieces may arrive together. Partial fire is an available technique, not a mandatory skill check or guaranteed window.
7. **Clear and change the problem.** Finite packs lead to a safe upgrade choice or the next encounter. Later threats and layouts ask for different routes; the completed build gets two encounters to matter.

Timing, aim, grouping, distance and execution are the proposed sources of mastery. If recovery is only unwelcome reload work, neither more cards nor more encounters solves that by itself.

## 5. Implementable mechanics and tuning rules

### Space, movement and inputs

Use a fixed simulation step and continuous/swept collision for moving pieces and hostile shots. World units are relative to Pip's **one-unit diameter**. The first test arena is a 24 × 14 rectangle. Pip moves at 5 units/second with normalized diagonals, without acceleration lag or a dodge button. Movement remains available during all attacks and recall. Aim is independent; a released gamepad aim stick preserves its last nonzero direction.

Keyboard/mouse: WASD move, mouse aim, LMB throw, RMB hold recall, Escape pause. Gamepad: left stick move, right stick aim, right trigger throw, left trigger recall, Menu pause. A throw uses the button's rising edge, never repeats simply because it stays held, and is never buffered behind a menu or recall. For analog triggers, start with press at 0.55 and release below 0.35 to avoid threshold chatter; allow remapping.

**Default arbitration: recall wins.** Determine recall's held state before processing that frame's throw edge. If recall is held, a throw press does nothing to the pieces and queues nothing. This includes simultaneous presses. The fan preview changes to a recall symbol; an attempted throw gives a quiet blocked-action tick and a short “Release recall to throw” prompt near Pip. Releasing recall restores the fan; a new throw press is needed. Empty throws have a different hollow click and a brief empty ring pulse. Prompts fade with learning and do not interrupt play.

**Focused input comparison, only if unwanted ignored throws recur:** a throw edge with at least one held piece ends recall and launches those pieces immediately. Simultaneous inputs give throw priority in this variant. Recall remains suspended until its input is released and freshly pressed; continuing to hold the trigger cannot immediately resume it. An empty throw gives the empty cue and leaves recall running. Pause/resume resets the latch and requires fresh action presses. Change no other mechanic in this comparison. Adopt it if it reduces failed intended actions without replacing them with unwanted volleys. Do not add an ammunition selector, attack mode wheel or second attack button.

### Six conserved pieces

Pip starts each normal attempt with six distinct piece IDs and three hull pips. At every simulation step:

`held + outbound + ejected + loose + returning = 6`

The pieces are uniform combat objects despite different cosmetic shapes. They never disappear, become enemy loot, or require a kill/currency to retrieve. All damaging effects below use ordinary block/hull rules. No enemy steals ammo, pierces protection or deals unavoidable damage merely because Pip is empty.

| State/action | Starting rule | Boundary and feedback |
|---|---|---|
| Held | Each attached piece is one available shot and one ordinary block. The orbit is decorative; its gaps do not determine defence. | Visible count 0–6 and six small state marks. Orbiting scrap does no damage. |
| Launch | Spawn held pieces just beyond Pip's front at 0.65 units from its centre, clear of solid geometry. Speed 14 units/s, total outbound path budget 7 units, damage 1 per eligible hit. Piece collision radius 0.10. | Blocked launch origins clamp to the near side of a wall; they cannot shoot through it. Launch recoil is visual, with no loss of movement control. |
| Fan | Full baseline fan is 35°. For `n` held pieces, offsets are `(i − (n−1)/2) × fan/5`, for `i=0…n−1`. | One piece is centred; two use ±3.5° in the base fan. Fewer pieces do not spread to the full six-piece width. Fan cards change `fan`, not this rule. |
| Outbound → loose | Normally land at first live enemy contact, path-budget exhaustion or solid boundary. Apply one outbound damage, then stop attacking. Friendly pieces do not collide with one another. | A near enemy can intercept a shot intended for one behind it. Lance and Bank are the only stated exceptions. Recall cannot reverse an outbound piece early. |
| Loose → held | Auto-collect within 0.8 units of Pip's centre, with a clear path for ordinary nearby pickup. | No pickup through a divider. Ejected/outbound pieces are ineligible. Quiet outlined scrap is distinct from harmless visual debris. |
| Loose → returning | Holding recall attracts every loose piece anywhere in the arena at 10 units/s toward Pip's current position. | Full player movement; no radius limit, reload cooldown or slowdown. Returning pieces pass over low arena dividers, visibly lifted by magnetic traces, so geometry cannot strand them. |
| Returning → held | Attach at 0.65 units from Pip's centre. Only attachment restores protection. | A distinct catch sound and state-mark change. Neither the magnetic trail nor a passing piece shields Pip. |
| Returning → loose | Releasing recall settles unfinished returns in place, clamped to a reachable floor point if over a divider. | This settling never causes enemy damage or counts as a crossing. It preserves the piece's launch cycle and spent return-hit flag. |

Actions use the pieces held at the start of the simulation step; movement and collisions then resolve. A catch during that step is available to the next step's input. A newly landed loose piece cannot collect until the following step, preventing a same-update launch/land/attach chain. This is state ordering, not an added reload cooldown.

Out-of-bounds safeguards restore the **same ID**, harmlessly, to the nearest reachable floor point; they never spawn a replacement copy. Encounter cleanup collects existing IDs into the ring. Cosmetic debris is a separate noninteractive system.

### Blocks, hull and recovery

A damaging contact/shot while Pip has any held piece ejects one piece, consumes the hostile projectile if applicable, and deals no hull damage. Eject opposite the contact side, up to 2 units at 8 units/s. The ejected state lasts **at least 0.25 seconds**, even if a boundary truncates travel; it cannot be collected or recalled during that outward state. Then it becomes loose and obeys normal recall/pickup. Ejection disarms any previous return attack: blocking never creates return damage.

A block grants 0.25 seconds of global guard grace and pushes a contacting Biter 0.6 units away where space allows. It does not displace a braced/dashing Rammer or Foreman or redirect their marked path. Grace starts at the block and overlaps ejection; it is not extended by harmless overlap. During grace, hostile projectiles that contact Pip are consumed without another hit. A Biter's contact attack has its own 0.8-second interval, preventing frame-by-frame bites. Other enemy bodies use that ordinary contact interval unless executing their specified single-hit attack.

With zero held pieces, an eligible hit removes one of three hull pips and gives one second of clearly shown damage grace. Hull persists between encounters; there is no baseline healing or permanent stat progression. Zero hull ends the attempt. Resolve collision events in time-of-impact order, with stable piece/enemy IDs breaking ties; no more than one player damage event can pass a fresh grace boundary at the same instant. Death wins an exactly simultaneous lethal boss/player hit; a dead player cannot receive a clear.

These ejection values complete an unspecified v0.1 rule; they are **not a demonstrated repair for safe shielding**. A weak pack may allow indefinite defensive waiting, which cannot clear it. The concern is whether waiting plus close-range attacks also removes skill and desirable risk from real mixed encounters. The stress tests below decide that.

### Return damage: exact optional rule and comparison

The proposed baseline **H** has harmless returns. The early comparison **D** gives every actually launched piece one optional **1-damage return hit** in that piece's launch cycle, with return speed multiplied by 0.75 (7.5 units/s before other modifiers). This is the same effect as the later Return Teeth card; the core experiment has no upgrade offers. D is a serious baseline candidate, not an observed improvement.

A cycle begins only when that piece launches from the held state. It can have outbound hits and **at most one eligible return hit in total**, even across releases and re-presses of recall. A return hit does not stop its journey. Attachment ends its armed cycle; ejection disarms it; only another actual launch can arm a new one.

**A new return crossing is required:**

1. At the start or resumption of returning, mark every enemy touching or overlapping the piece as contact-excluded, using the sum of their collision radii plus a 0.05-unit separation margin.
2. Such an enemy becomes eligible only after the piece has been fully outside that expanded boundary for a complete simulation step while actively returning. Recall toggling, landing, divider clamping and stationary loose contact cannot create this eligibility.
3. Damage requires a subsequent outside-to-inside swept crossing during actual return motion. The piece must have moved at least 0.05 units since separation; include the enemy's motion in the relative sweep. Pulling away from a just-hit enemy is an exit, not a hit. An enemy may cross a moving return path, but walking onto loose scrap is harmless.
4. The first eligible enemy in that motion step takes 1 damage and atomically spends the piece's return-hit allowance. Remaining crossings are harmless. Ties use time of impact, then stable enemy ID. Releasing/repressing recall never resets the allowance or grants a damage event.

An enemy already hit outbound **may** be hit on return, but only after real separation and a new crossing. Immediate fire/recall against the same overlapping target cannot supply an automatic two-hit kill. A piece that landed on an enemy's near face and is pulled straight back will normally do no return damage to it. The player needs another route, an intervening moving enemy, a miss beyond a target, or piercing flight to create a second hit.

H is simpler, faster and tests whether launch/recovery itself earns practice. D might make moving around a landing point an attack decision, but also adds damage and changes exposure duration. Compare routes, kill rate, pressure, failed inputs and wanted replay. If D is preferred only because it is stronger, do a focused challenge/speed control before attributing that preference to route play. If D earns the core position, make it available from the start, remove Return Teeth from the pool without inventing a filler replacement, and retune enemy/boss/challenge pacing before expanding content. Sections 8 and the validation plan specify that branch.

### Enemy pressure: four product roles, two in the first experiment

Health, speed and timing below are initial values shared across courses. Enemies are recognizable by silhouette, tell and behavior, not colour alone. All can be avoided at base movement speed with no held scrap in the intended encounter configurations; that requirement must be checked in motion.

| Enemy | Concrete behavior | Decision it adds |
|---|---|---|
| **Biter** — 2 hull, radius 0.45 | Approaches at 2.5 units/s with local separation, contacts at most once per 0.8 s, ordinary damage and knockback. It navigates around dividers. | Group for efficient hits; choose a short recovery without being crowded. |
| **Spitter** — 2 hull, radius 0.55 | Moves at 2 units/s toward a line of sight at roughly 6 units, then stops. A 0.8-s tell tracks for its first 0.4 s and visibly locks for its last 0.4 s. Fires one radius-0.15 projectile at 7 units/s, then has 1.4 s recovery before its next tell. A nonlethal hit does not cancel a locked shot; killing it does. | Leave a firing lane, hold protection through it, or remove the shooter while dodging empty. |
| **Rammer** — 4 hull, radius 0.55 | Approaches at 2 units/s. Within 6 units it braces for 0.9 s, locking its straight path after 0.45 s, then dashes up to 5 units at 8 units/s. Wall impact ends the dash; recover for 0.8 s. One contact hit per dash; grace still applies. | Bait and sidestep a commitment, exploit recovery, and change direction instead of following one perimeter route. It does not home during the dash. |
| **Sweeper** — 3 hull, radius 0.60 | Moves at 1.5 units/s to a clear lane. Marks a 1.4-unit-wide strip through Pip's position for 1.0 s; it is fixed from the start. Sends one broad, blockable pulse along it at 6 units/s, then recovers for 2 s. The pulse stops at a divider/boundary and expires within 4 s. | Move across a marked lane or attack the source before release; reconsider a distant landing zone. No persistent floor damage or ammo destruction. |

Ordinary outbound-hit knockback is 0.3 units where space permits. Enemies resist displacement during locked tells and dashes, so impact feedback cannot redirect a committed warning; they remain damageable. Outside those states, Scatter can increase ordinary knockback as specified in §8. The Foreman's smaller displacement cap is separate.

Start shooter tells at least 0.35 seconds apart; at most two ranged enemies may be attacking/charging and at most one Sweeper strip may be live or warning. Queue later tells in deterministic spawn order. These caps limit visual congestion, not guarantee a safe route: reject authored combinations that close every escape corridor during the locked tells. Body contact is ordinary damage, never an invisible armour bypass. Enemies reveal their first attack only after entering the arena.

## 6. Distinguishing features and what earns its place

| Feature | Intended value | Evidence needed |
|---|---|---|
| Six visible objects shared by shield and ammunition | A throw has an immediate readable cost; recovery feels physical. | Players explain blocks/exposure and want to perform the action again. |
| Recovery in motion | Distance, misses and the route home affect the next opening. | Players enjoy improving aim/routes or cadence; retrieval is not consistently described as housekeeping. |
| Optional partial volleys | Use a brief opening without another selection interface. | Intentional use in near-hit/far-miss arrangements; clustered returns need not provide it. Remove prominence from the pitch if it is rarely useful. |
| Courses and fixed challenges | Learn geometry and threats under comparable conditions. | Different layouts or challenges change execution and motivate another clear. |
| Small behavior-changing builds | Narrow piercing, wide spacing, bank shots or heavy close play change technique. | Retained cards cause noticeable useful differences, not just more power or a larger combination count. |
| Expressive toy machinery | Recognizable combat and a sympathetic robot. | People identify active aiming, danger and recovery immediately at gameplay scale. |

No feature is claimed to be unprecedented. Return attacks are deliberately absent from the baseline store promise until the comparison resolves them.

## 7. Structure and recommended complete product

**Recommended baseline package:** one robot; three selectable arena courses; four ordinary enemy types; one Foreman with course-aware positioning; five encounters per course run; eight cards with three picks; Standard and Overdrive presets; six fixed single-encounter challenges; practice, records, settings, reliable saving, audio and a verified Windows package. These are proposed product commitments to evaluate after the core, not features implemented now or quotas to fill with weak content.

### Three courses with different geometry

Each course uses the same 24 × 14 footprint, six pieces and control rules. All three Standard courses are accessible from the start; Open Tray is marked as the introduction. Pip starts near the left side with full hull and scrap. Completing any Standard course resolves the story and unlocks Overdrive across courses; there is no power reward.

| Course | Geometry and encounter identity | Useful difference |
|---|---|---|
| **Open Tray** | Clear rectangle. Entry pairs move from a single approach to adjacent/opposite sides. Outer walls support Bank Rims. | Learn spacing, direct aiming and changes of direction without obstruction. This is the core-test arena. |
| **Split Bench** | Two low rectangular baffles, each 1.5 × 4 units, centred near (8, 5) and (16, 9). Rounded collision corners; all corridors retain at least 2.5 units of clear width. | Break line of sight, choose which side to attack from, and use banks. Enemies must route around baffles; outbound/hostile shots stop on them. Magnetic returns pass over them. No hidden off-screen enemies. |
| **Crossbelt** | Two horizontal conveyor strips, 2 units wide, centred at y=4 and y=10; the upper moves right and the lower left at 1.2 units/s. They add the same lateral velocity to player and ground enemies. | Decide when to cross or use a belt during aim/recovery. Belt arrows are always visible. They do no damage and do not move loose, outbound or returning scrap; fixed pieces are shown resting magnetically on rails. Walls clamp movement without crushing. |

Crossbelt changes effective enemy/player ground motion equally, while return speed remains measured in world space. Avoidability and catches must be rechecked there, especially with Heavy Clips. If belts mainly add unwanted steering, revise or replace that course; do not retain it for an arena count.

### Five encounters per run

There are **twelve ordinary encounter templates**: the following four roles authored once for each course, with course-specific entries, cover and belt approaches. Each template contains two finite packs. Seeded mirroring changes approach direction; it cannot change health, invent new packs or silently vary the rules. The Foreman is a fifth encounter with bounded escorts.

| Encounter | Standard starting roster, first pack → second pack | Purpose and transition |
|---|---|---|
| **Opening** | 2 Biters → 2 Biters + 1 Spitter | Re-establish throw/exposure against a simple threat, then add a shot. Clear gives upgrade pick 1. |
| **Crossfire** | 2 Biters + 1 Spitter → 2 Biters + 1 Spitter | Separate approach directions and firing time. Clear gives pick 2. |
| **Interception** | 1 Rammer + 2 Biters → 1 Rammer + 1 Spitter + 2 Biters | Bait a locked charge, then handle it with ranged pressure. Clear gives pick 3. |
| **Pressure** | 1 Sweeper + 2 Biters → 1 Rammer + 1 Spitter + 2 Biters | Cross a marked lane and recover under mixed threats; use the full build. No further pick. |
| **Foreman** | Foreman + at most four Biter escorts over the fight, no more than two escorts alive | Finish using familiar tells and commitment windows. Immediate win on surviving its defeat. |

Each entry has a 0.9-second visible/audible warning. Newly entering enemies begin at least 3 units from Pip; choose the next authored safe entry if necessary. No damage can occur from outside the arena. Start the second pack's warning once at least one first-pack enemy has died and at most two remain, subject to a six-ordinary-enemy live cap. Clearing a pack triggers the next warning immediately; no extra spawn timer protects a nominal duration. The first two-enemy teaching pack is therefore not followed by an automatic simultaneous second pack before a kill.

An ordinary encounter clears only when **all assigned enemies across both packs** have been defeated, including queued entries. Then stop hostile effects, collect all existing pieces automatically, preserve hull and show the upgrade/continue transition. A player can proceed immediately; no mandatory victory animation or cleanup delays the next attempt.

### Mastery beyond a first clear

Overdrive keeps the same enemy health, piece count and tells' visual vocabulary. Start with one additional Biter in the second pack of each ordinary encounter, a live cap of seven, Spitter recovery reduced from 1.4 to 1.0 seconds, and Rammer recovery from 0.8 to 0.65 seconds. Keep the original warning/aim-lock durations and ranged concurrency caps. Use a second authored entry arrangement per course that invites direction changes. Foreman uses those Rammer recovery values with the same health and escort total. Test fair escape while empty; difficulty must come from execution and combined pressure, not health multiplication.

Six fixed challenges are available from the start and labelled by skill; none is a daily task or service. Each is one finite encounter with a fixed layout, roster, seed and loadout, and instant same-challenge retry:

| Challenge | Fixed H loadout and starting roster | Practice goal |
|---|---|---|
| **Close Quarters** | Open Tray; Scatter Fork, Close Coil, Launch Treads; 2 Rammers + 4 Biters in two split packs | Control spacing with short volleys and quick nearby catches. |
| **Needle Lane** | Split Bench; Lance Rails, Long Cast, Launch Treads; 3 Spitters + 4 Biters in two capped packs | Line up targets without being trapped behind cover. |
| **Bank Job** | Split Bench; Bank Rims, Scatter Fork, Long Cast; 2 Spitters + 2 Rammers in two packs | Use banks or change position; banking is useful, not a compulsory hidden kill condition. |
| **Heavy Traffic** | Crossbelt; Heavy Clips, Close Coil, Launch Treads; 2 Rammers + 2 Sweepers + 2 Biters in two packs | Time slow heavy shots and belt crossings; respect the single-Sweeper-lane cap. |
| **Crossing Lines** | Open Tray; Return Teeth, Lance Rails, Long Cast; 2 Spitters + 4 Biters in two split packs | Try real return crossings and compare direct versus two-leg attacks. If D becomes core, remove Return Teeth from this fixed loadout; the same two remaining cards plus core D reproduce its effect. |
| **Foreman's Overtime** | Open Tray; Lance Rails, Bank Rims, Close Coil; Foreman and its four capped Biter escorts, Overdrive recovery | Practice the finish directly, without replaying an easy opening. |

Every challenge offers a clear medal, a clean-hull-clear medal and a personal best time under identical conditions. No medals require taking damage, waiting, farming blocks or killing infinitely spawned enemies. Practice also allows replaying any encountered ordinary template with a chosen legal loadout; practice results do not overwrite course/challenge records. There is no endless-mode, online leaderboard or campaign-length promise.

## 8. Progression, upgrades and reliable offers

### Three early choices

After encounters 1, 2 and 3, pause combat and offer two distinct cards; choose one. Each is nonstacking. All definitions are eligible from the first run. There is no currency, shop, reroll, permanent damage bonus or grind to unlock the enjoyable controls. Two final encounters use the finished build.

| Card ID / name | Concrete starting effect | Intended technique / tradeoff |
|---|---|---|
| `lance` **Lance Rails** | Fan 12°. Each outbound piece can hit up to two distinct enemies, at most once each; land on its second hit or normal range/boundary. Incompatible with Scatter Fork. | Line targets up; narrow coverage makes lateral approaches harder. Piercing does not refresh a return hit. |
| `scatter` **Scatter Fork** | Fan 55°. Outside locked tells/dashes, outbound-hit knockback rises from 0.3 to 0.9 units where the target can move; damage unchanged. Incompatible with Lance Rails. | Make room across a wide approach; landing multiple hits on one target usually needs a closer shot. Bosses accept at most 0.15 units of hit knockback under either rule. |
| `teeth` **Return Teeth** | Activate exactly the D rule in §5 for launched pieces: one new-crossing return hit, 1 damage, return speed ×0.75. | Make a second attack route at the cost of longer exposure. It cannot turn a block or contact reversal into damage. Removed entirely if D becomes core. |
| `coil` **Close Coil** | Return speed ×2 while a returning piece is within 4 units of Pip's current centre; outside that radius it uses its other applicable speed. | Stay near landings for a fast catch; leaving a safe position just for speed is a choice. No effect on outbound/ejection time. |
| `treads` **Launch Treads** | An actual nonempty throw grants +20% player movement speed for 0.6 s. It neither stacks nor refreshes while active; another throw after expiry can grant it again. No invulnerability. | Use exposure to cross a gap. Tiny volleys may maintain frequent boosts, which the mixed-pressure test must assess. |
| `bank` **Bank Rims** | One specular bounce per outbound piece against an outer wall or baffle. The original path budget still applies; no speed/damage increase. At an exact corner reverse both blocked components and spend the one bounce. | Aim an indirect shot and accept a different landing position. On an enemy hit, normal landing/piercing rules still apply. |
| `heavy` **Heavy Clips** | Outbound damage 2 instead of 1; outbound and return speed each ×0.75. Return damage stays 1 if present. Range/path budget unchanged. | One outbound hit kills a Biter/Spitter, but slower flight and recovery demand timing. Its extra power is a balance risk to compare, not proof of another play style. |
| `long` **Long Cast** | Outbound path budget 10.5 instead of 7 units, all other base values unchanged. | Reach a distant shooter or longer bank route; misses land farther away and take longer to recover. |

Only Lance/Scatter are incompatible. Multiply applicable return-speed factors once: `10 × return-rule factor × Heavy factor × Close Coil factor`. Return-rule factor is 0.75 for either H+Return Teeth **or** D, never both. For example H+Teeth+Heavy is 5.625 units/s far away and 11.25 nearby with Coil. Close Coil does not speed block ejection. Launch Treads changes Pip's own motion, then Crossbelt adds conveyor velocity; it does not accelerate pieces.

Bank + Lance can cross up to two enemies and bounce once within the same fixed path budget; wall contact does not reset the hit list. Heavy + Lance changes outbound damage only. The same cycle/contact rules govern all cards. This prevents return toggles, banks, piercing and collision separation from multiplying hit allowances.

**Grounding Guard is cut:** a block-centred safety card overlaps the problem of cheap defensive waiting, while Bank and Heavy offer more distinct attack techniques to test. Spring Guard's passive block damage and Reserve Clip's guaranteed protection after throwing remain rejected. Eight is the current useful hypothesis, not a quota; replace or remove cards that players experience as redundant.

### Offer algorithm

Use a seeded shuffle and stable card IDs so a same-route retry reproduces offers. Before each choice:

1. Build `legal` from pool cards not owned and individually compatible with **all** owned cards. Previously declined cards stay eligible.
2. Split `legal` into never-shown cards and previously shown/declined cards. Shuffle each group using the offer seed, independently of combat effects and player input timing.
3. Fill two slots from never-shown first, then fill any remaining slot from declined cards. Mark both displayed cards as shown. Do not duplicate a card or offer an incompatible/owned one.
4. Require at least two legal cards at every reachable choice. Validate the pool and number of picks as a unit before using a configuration; malformed configurations fail the content check, rather than shipping a one-card offer.

The full H pool is `{lance, scatter, teeth, coil, treads, bank, heavy, long}` with **three picks**. If D is promoted, remove `teeth`: seven cards, still three picks. These have respectively 50 and 30 legal unordered final triples; those are arithmetic configurations, **not distinct play styles or hours of replay**.

The explicit reduced experiment uses `{lance, scatter, coil, treads}` with **two picks after encounters 1 and 2**. It is a smaller content comparison, not an invisible cut to the advertised complete product. In the troublesome branch “show Lance + Coil, choose Lance,” Scatter is excluded; the next offer is Treads + the declined Coil. Reoffering the declined card fixes the old four-card edge case. Do not run three picks with this four-card/conflict configuration.

### Persistent goals and comparable records

Save settings, first completions, course/preset medals, challenge medals, last route seed and local records. Hull and upgrades reset on a fresh attempt. For each clear store active time, hull remaining, hull hits taken, blocks, loadout and route identity; no points are awarded for damage received or stalling. Active time sums each encounter's first entry warning through its clear/death, excluding pause. Safe teaching, menus, upgrade deliberation and transition presentation are excluded.

“Retry same route” repeats the course, preset, pattern/offer seed and rule version, with fresh hull/pieces and new choices allowed. “New route” changes the seed. Best-time comparisons include course, preset, seed, chosen loadout, assistance flags and rules version; a different combination is labelled a different setup, not an unqualified improvement over the old one. Fixed challenges provide the simpler like-for-like benchmark. Retire old records to a visible archive when rules change, rather than comparing changed physics silently.

Mastery goals are clean clears, faster execution under the same conditions, all three courses, Overdrive, and distinct short challenges. Each needs wanted replay evidence; merely exposing more checkboxes is insufficient.

## 9. Narrative and setting

Pip is a homemade magnetic salvage robot. A malfunctioning sorting line labels Pip and its six useful keepsakes as waste. The robot fights through a workbench station and shuts down the Foreman controlling it.

Open Tray, Split Bench and Crossbelt are alternative routes through that line, each ending at its supervisor. Any first Standard clear supplies the complete small story: the disposal lamps go out, the six objects settle into a proud little formation, and Pip's work lamp comes back on. Further courses and Overdrive are optional mastery, not withheld story chapters.

Use a few skippable wordless beats: power-on, the mistaken disposal label, a surprised tilt when protection leaves, and the shutdown payoff. Worn enamel, washers, clips, clamps and labels establish scale. There is no dialogue tree, voiced cast, inventory fiction or crafting system. The robot's sympathy should make a dangerous escape memorable without delaying retries.

## 10. Art and audio direction

### Appearance in motion

Top-down 2D, with chunky original toy-machine silhouettes, restrained material texture and gentle contact shadows. Pip is bright and rounded against a quiet warm-grey surface. Biters have jaws, Spitters a visible barrel, Rammers a bracing wedge, and Sweepers a broad rolling head. The Foreman reads as a larger supervisor built from the same visual vocabulary.

Use shape, motion, outline and brightness together: held pieces orbit, outbound pieces have short directional trails, loose pieces have a quiet floor outline, and returning pieces have lifted magnetic traces. Dangerous shots use a different silhouette and motion pattern. A return-hit-capable piece gains a small toothed/charged accent only while its allowance is armed; spending it removes that accent. Do not render harmless recall as an enemy-damaging slash. Hull loss, a blocked hit and an ignored input must be distinguishable without reading text.

Keep the arena visible at gameplay scale. Baffles cannot hide Pip or a shot tell; belt markings stay below threat contrast. Leave a readable 0.8–1.0-second warning even when several impacts occur. Optional short hit-stop may emphasize impact later, but it cannot silently advance enemies or consume input and must be evaluated for repeated-fire comfort.

[GNOG](https://store.steampowered.com/app/290510/GNOG/), [Wilmot's Warehouse](https://store.steampowered.com/app/839870/Wilmots_Warehouse/) and [WALL-E](https://www.pixar.com/wall-e) remain the original brief's toy-material, graphic-readability and wordless-character references. These are direction references from the September 7 source work, not assets to reproduce or evidence that their audiences want this action game. Pip needs its own silhouette and character design.

### Sound and feel

The signature is a substantial mechanical launch followed by individual dry catches that resolve into a six-note magnetic chord. Metal, wood and plastic colours can vary subtly; combat physics do not. Recall adds an electrical rise that never masks a locked-shot warning. A block is a sharp deflection; hull damage is lower and harsher. Enemy tells have distinct rhythms, so a Rammer brace cannot be mistaken for a Spitter charge.

Start the paid direction with one rhythmic electronic combat bed, a pressure layer and a short Foreman motif, plus concise win/lose cues. About fourteen event families cover launch, empty/blocked input, outbound/return hits, landings, catches, recall, blocks, hull loss, tells, enemy breakage and transitions. This is a sound-design starting inventory, not a limit on useful variation. Repeated scrap hits should compress/dampen under important danger cues. Separate music/effects volume, reduced flash and adjustable shake are required.

Original/code-authored, generated or licensed assets can all be evaluated at gameplay scale. Record origin, tool, license, modification and credit in the eventual game's `assets/manifest.csv`; a style reference is not permission to reuse its assets. No assets are created or purchased in this task.

## 11. Sample encounter: Crossfire

This illustrates the **H baseline**, Open Tray, with Bank Rims selected after Opening. It deliberately does not require Return Teeth or Lance to describe a satisfying exchange.

Pip enters with six pieces and the hull left from Opening. Two Biters approach from the left; the first Spitter enters near the upper edge after its warning. You move toward the lower-right to keep an exit and separate the shooter's line from the Biters. Its tell locks. You can keep the ring to insure against a mistake or throw now and dodge empty.

You aim across the approaching Biters and throw. Two pieces connect with the nearer Biter and destroy its two hull; other rays miss or strike elsewhere and continue to their range limit, with Bank Rims allowing one wall bounce. This does not guarantee the second Biter dies. You step across the locked shot and hold recall. The nearby hit pieces return before the longer banked misses.

Two pieces are home with a useful angle on the surviving Biter. You **release recall, then make a fresh throw press**. If both narrow partial-shot rays connect, its two hull are enough to finish it; a single hit leaves it alive. The other pieces stay loose until you hold recall again. The movement and distinct catch sounds should make the opportunity readable, but the sample does not prove the window is comfortable.

The second pack's warning can begin once the first has lost an enemy and has at most two survivors. You change direction as its entries appear. A direct shot at the upper Spitter is now obstructed by a Biter; you can move to a clearer angle or use a **reachable** bank within the seven-unit total path. The preview shows that path limit. You use cover from the remaining held pieces through a shot tell, dodge, then finish the Spitter with a deliberate aimed volley.

After all six assigned enemies are defeated, hostile effects stop, the same six pieces collect automatically and pick 2 appears. There is no cleaning-up task and no required encounter duration.

For a separate Return Teeth/D fixture, arrange a missed piece beyond a pursuer, then move sideways before recall so its path crosses that enemy. Also try a piece still touching the enemy it hit: pulling it straight away must do no second damage. These are comparative fixtures, not unannounced powers in the H encounter above.

## 12. UI/UX and dependable handling

Keep attention in the arena. The minimal HUD shows three hull pips, held count with six state marks near Pip/reticle, course and encounter `1/5`, pending/current pack information, and up to three chosen card icons. A clear-progress indicator counts the finite assigned enemies; it is not a survival clock. Do not suggest an encounter is complete while an assigned pack is still queued. A quiet optional active timer supports practice.

The reticle previews the current fan and, with Bank Rims, one reachable bounce and its remaining path budget. During recall it shows the recall state and the release-to-throw cue from §5. Return-hit accents appear only when the rule is actually enabled. Enemy tells show where danger will go once locked; never use a tracking line that secretly continues following Pip after lock.

Upgrade screens freeze combat and show the rule, a tiny trajectory illustration and the useful tradeoff. For example, Bank Rims: “Bounce once off walls. Same travel limit; a new way around an enemy.” Full numeric rules are available while paused. There are no shop resources to learn. Reoffered declined cards are ordinary legal choices, with no punishment text.

First-run teaching is a short safe throw/recall interaction followed by the two-Biter pack. Prompts introduce protection, recall and an empty/blocked attempt when relevant; they do not pause every few seconds. The introduction is skippable and not repeated on retries. Show gamepad and remapped prompts accurately. After every menu, require fresh action presses so selecting a card cannot launch the ring on resume.

Pause on focus loss, freeze all combat/timers, release held actions, and resume only through an explicit input. Test input capture, analog thresholds, window/resolution changes, controller connection changes and keyboard-only/gamepad-only menu operation. Readability targets include 1280×720 and 1920×1080 with UI scaling and clear threat margins in other aspect ratios. These are requirements, not verified platform support.

Save settings/records with a versioned atomic write and recoverable previous copy. A corrupt save should restore usable defaults and preserve a valid backup when possible, with a clear notice; it must not prevent starting a game. There is no mandatory account or telemetry. Mid-run suspend is not in the current package; reevaluate it if measured sessions or interruptions make restart costly. Regular pause remains available throughout.

Results show clear/defeat, active time, hull hits, finishing hull, loadout and comparable route identity. Primary actions are **Retry same route**, **New route**, **Courses/challenges** and **Quit**. After a first clear, highlight Overdrive and a relevant fixed challenge without forcing either. Show separate records, not an ambiguous global score that rewards a luckier offer seed.

## 13. Win, loss, finale and endgame

**Ordinary win:** every enemy assigned to both packs is defeated. **Run win:** the Foreman dies while Pip remains alive. Escorts power down and hostile projectiles stop immediately; surviving escorts are not a compulsory cleanup wave. **Loss:** zero hull. A brief break-apart effect can be skipped into immediate retry. No progress currency is lost and no grind strengthens the next attempt.

The Foreman begins at **24 hull**, radius 1.25, with no invulnerability phase. All eligible piece hits deal their normal damage; all its attacks can be blocked or avoided under the same rules as ordinary enemies. Four perfect six-damage outbound volleys would exhaust that health. This is an arithmetic bound under ideal hits, not a measured boss length, and Return Teeth/Heavy/Lance interactions can change it.

Use a deterministic learned-threat cycle:

1. **Pin:** the familiar Spitter shot, with a 0.8-s tell and last 0.4 s locked, followed by 1.0 s recovery.
2. **Fork:** mark two diverging shot lanes at ±18° around the aimed centre, tell for 0.9 s with the last 0.45 s locked, fire one projectile down each at 7 units/s, then recover 1.0 s. The gap between lanes is visibly safe; no hidden centre shot.
3. **Drive:** the Rammer brace/locked-path vocabulary, dash up to 4 units at 8 units/s after a 0.9-s tell, then recover 0.8 s. Stop at walls/baffles. One contact hit per dash and no steering after lock.

Then repeat from the new position. Course-specific waypoints keep the supervisor in navigable space with at least two exit directions around its footprint. Conveyors affect its ground movement under the same rule as other enemies. Nonlethal impacts cannot secretly cancel/redirect committed attacks; boss knockback is capped at 0.15 units outside locked tells/dashes and disabled during them. No untelegraphed teleport or shield exception appears in the finale.

The first two Biter escorts enter with the boss's warning. After one dies, warn the next unused escort entry; repeat until the **four-escort total** is exhausted, respecting the two-alive cap. Do not create an infinite source of danger, score or time padding. Boss defeat cancels pending entries. Its earliest attack starts immediately after entry; the player is allowed to kill it before seeing every pattern.

Tune for a satisfying combination of learned pressure, useful attack windows and a decisive shutdown. If new players erase it before experiencing any pressure, first examine hitbox, positioning and escort interception, then a more useful attack arrangement. If it becomes repetitive or overly miss-heavy, shorten/rewrite the pattern. Health may be tuned for observed challenge, but neither a longer bar nor forced waiting is justified by a desired clock. Expert execution is allowed to make the finale short.

Any Standard course clear finishes the small fiction. The desired continuing game is fair mastery across the other courses, Overdrive, clean clears and fixed challenges. If those do not earn wanted post-clear or later-session play, the product remains unproved; finishing the story once does not establish its commercial replay claim.

## 14. Business model and price hypothesis

**Premium, paid once; proposed test price USD 4.99.** The contemplated purchase includes the complete package in §7, dependable controls, coherent art/audio and reliable offline saving/packaging. No ads, paid power, loot boxes, mandatory DLC or ongoing content service are planned.

The expanded package is recommended because geometry, threat roles and comparable challenges address the review's most concrete value gap. Those paper additions **do not establish** that buyers would pay $6.99 or $9.99. The closest $4.99 mastery reference supports testing the proposition, while cheaper build-heavy alternatives and low-response retrieval games remain counterevidence. Show the actual package and experience at $4.99 first. Keep a “would skip” response and named alternatives available in evaluation.

The owner permits **up to USD 9.99** when the genre, actual package and evidence justify it. Reconsider $6.99–9.99 only after a representative product earns sustained repeat interest and its presentation/value response supports that price against current comparables. Refresh regular versus discounted, region-specific prices then. Do not fill a content quota to reach a sticker price or quietly lower the price to excuse a failed loop.

Economics need explicit separate inputs:

| Input | Current knowledge and measurement plan |
|---|---|
| AI execution | Ordinary 2D systems are technically plausible; game-specific duration/rework is unmeasured. Record active execution and verification for actual increments, not human-team calendars or task age. |
| Subscription capacity versus extra cash | Existing paid allowance is distinct from purchased credits, overages or new tools. Record actual incremental charges when incurred; no human programmer wage is imputed to AI coding. Neither all work being free nor an imaginary salary is assumed. |
| Assets/tools/release | No money is committed by this revision. Record needed licenses, assets, store charges and promotion expenses individually; refresh applicable store terms before a release commitment. The review's fee/timing evidence is preserved in its audit. |
| Owner attention | No hour cap. Measure play, decisions, review, setup and support attention separately. Klaus need not administer every test or do routine development. There is no invented hourly valuation. |
| Calendar | Implementation, tester availability, storefront waiting/review and release timing are different clocks. No launch date or completion-day estimate is supplied by this desk revision. |
| Support | Compatibility, save recovery and actual launch defects may need follow-up. Estimate from verified targets and observed defects; support effort/cash are currently unknown, not zero. |
| Obtainable demand and proceeds | No reachable buyer count, conversion, realized receipt or refund rate is measured for this game. A few hundred buyers is worthwhile only if their proceeds justify actual relevant costs and attention. |

For a later cost scenario, define `C` as incremental cash including anticipated support and `q` as average per-paid-copy contribution after discounts, regional prices, refunds, applicable store deductions and variable costs. If `q > 0`, cash break-even is `ceil(C/q)`. Treat subscription allocation and owner time explicitly alongside it. There is no justified numeric sales or profit forecast now; review counts cannot fill the missing demand input. Actual transactions would be stronger evidence than stated intent.

## 15. Positioning, name, audience scale and discovery

**Provisional replacement title: Sixfold Recoil.** Steam already lists a different [Scrapstorm by Pixiliated](https://store.steampowered.com/app/5122440/Scrapstorm/) in the action-roguelite neighbourhood. The September 7 [bounded naming check](naming-check.md) found no exact-title game for Sixfold Recoil in the inspected Steam, itch and indexed-web results. It did find unrelated Sixfold titles and a close recall shooter named [Snapback](https://miraybuluc.itch.io/snapback), which weakened Snapback Salvage. These observations support a working-name choice, not exclusivity or legal clearance. Refresh before public branding. Keep the repository ID/path `scrapstorm` for history; no slug migration is needed.

Proposed descriptive line: **“An arcade action game where your six shield pieces are your shots. Aim, throw, dodge and pull them home.”** The first eventual clip should show independent aiming, a threatening locked shot, the ring leaving, exposed movement, and the same pieces returning. Show a recovery or escape before an upgrade screen. “Recoil” alone could imply movement powered by gun kickback, so the description and actual motion must explain the game. Materials and personality support combat recognition; they must not imply a crafting, collecting or relaxing workshop game.

The paid-action format has evidence; our reachable niche does not yet have a numeric size. The review cites a developer/partner report of more than 1.5 million Brotato copies in April 2023 and [SNKRX's developer log](https://a327ex.com/posts/snkrx_log), whose July 9, 2021 entry reports 80,781 units. These are attributed historical units for different products and discovery circumstances, not unique prospects or a forecast for this studio. The [Swarmlake developer paper](https://www.cg.tuwien.ac.at/research/publications/2018/GRIESHOFER-2018-GOS/GRIESHOFER-2018-GOS-thesis.pdf) also separates historical low-price units from gross receipts; current list price cannot reconstruct money earned. Full dates, figures and access limits remain in the review.

The [September 6 research](../2026-09-06-next-game/README.md) records Pinterest's reported 437% growth in global miniature-from-trash text searches, **May 2026 versus May 2025**, published August 25. It does not establish September acceleration, an impending peak, PC-game demand or automatic advertising. Retain upcycling inspiration only because familiar useful junk helps the game's identity after the attention fades. The imported historical research retains old planning discussion for traceability; it does not override this v0.2 or Klaus's correction.

Plausible discovery tests are a readable clip and later playable sample shown through actually reachable arcade/action channels, followed by reactions to the honest complete package. Creator coverage and Steam discovery are possibilities, not secured access. No public page, campaign, contact or paid promotion is undertaken here. The useful next commercial observation is whether people who enjoyed the real action prefer this package to alternatives at its test price, and why they would skip it. That remains weaker than obtainable paid demand until stronger evidence arrives.

## 16. Scope flexibility, feasibility and value of increments

There is **no hard development-time or owner-time ceiling**. The original 6–7 implementation days, 4–6 owner hours and one-day prototype estimate are retired from current planning. Do not substitute human-team durations, human programmer wages or a new arbitrary limit. A larger useful game is allowed when its enjoyment and plausible commercial value justify actual costs; extra inventory without better play does not.

The relevant local capability reference is Klaus's report of a couple-hour Breakout evening, together with the review coordinator's inspection of its later NEON BREACH documentation: twenty campaign stages, upgrades/pickups, endless play, code-authored presentation, saves and recorded Windows package verification. This is attributed local implementation evidence, **not a new test in this task**, and the later inventory is not all assigned to that first evening. It supports attempting concrete AI increments; it does not measure this game's feel, finishing duration or buyers. See the [audit's production correction](../2026-09-07-scrapstorm-review/coordinator-evidence/audit.md#production-economics--corrected-after-klauss-clarification).

| Work increment after separate implementation authorization | Player-value question | Evidence/cost to retain before expanding |
|---|---|---|
| Core: six pieces, two enemies, H/D, finite clear, clear sound and restart | Does throw/exposure/recovery earn understanding and another attempt? | A runnable package, input/collision probes, observed play; active AI work, extra charges and owner attention separately. |
| Representative encounter: one useful card, Rammer, final-direction attack/block/hull feedback | Does a new commitment window improve learning without reducing readability? | Compare the same encounter before/after; inspect motion/audio and record rework, not only compile success. |
| Replay increment: Split Bench plus one fixed challenge and same-setup records | Does geometry or a fair goal motivate play after a win? | Actual post-clear/later-session response, route differences, packaging and save cost. |
| Complete package: remaining cards, Sweeper, Crossbelt, Foreman, Overdrive and other challenges | Does every proposed element change technique, pressure or a wanted goal? | Retain useful additions, revise redundant ones, and revise the remaining effort/cash expectation from delivered increments. |
| Product verification and proposed price | Is this a coherent, worthwhile paid game at an acceptable actual cost? | Both inputs, target resolutions, pause/saves, clean packaged launch, representative player/value response and an explicit cost/demand case. |

These are evidence-producing increments, not day allocations or automatic approval meetings. Design authorizes their specification only; later authorized work should proceed normally within its own scope.

Protect the core commitment, full movement, recoverable finite pieces, aim/input reliability, readable danger, sound, instant retry, fair records and a decisive finish. Cut low-value background decoration, redundant cosmetic shapes, repeated staging and weak cards first. The old four-card experiment remains valid only with its two-pick algorithm in §8.

Do **not** casually cut Overdrive or fixed goals when they supply the reason to return. A course can be removed/replaced if its geometry fails to improve play, but that changes the advertised package and value test. Likewise remove an ordinary encounter if it rehearses solved play without pacing benefit; then move or reduce upgrade choices consistently instead of leaving the full-build segment missing. Record material package changes and repeat the affected value test.

Possible later additions include a demonstrably different threat, another worthwhile course, richer original presentation, replay viewing or suspend saves. Judge each by its measured benefit and actual production/support cost. Multiplayer, online services, freeform construction, material physics piles and a permanent power economy change the premise and are outside the current design. They are not repairs for a weak repeated action.

## 17. Complete round walkthrough: start to finish

An **illustrative Standard Open Tray run under H**, with harmless recall throughout. There are no timestamps: hits, pack overlap and duration must emerge from play. It uses a legal offer sequence and three picks, and does not assume a return attack or guaranteed multi-kill that the chosen build lacks.

**Start — six useful objects.** You choose Open Tray. Pip wakes, its three hull lights fill, and six pieces settle into orbit. A small fan follows the aim cursor. You complete or skip the safe throw/recall prompt. On a replay this staging is already skippable.

**Opening — the consequence of a throw.** Two Biters enter after the left warning. You move sideways so their approaches separate across your aim. A volley lands two rays on one Biter; it breaks, while the other's remaining hull tells you the whole fan was not a guaranteed crowd kill. You are now empty. Holding recall returns the near hit pieces first, while misses finish their outward travel. You recover rather than shoot blindly at the first catch.

That first kill permits the next entry warning. Two more Biters and a Spitter arrive. You wait through the Spitter's tracking part, see its line lock, step out and release recall before throwing. The complete five-enemy assignment eventually clears. There is no requirement to wait out a timer. Scrap comes home automatically and hull remains at three.

**Pick 1 — Bank Rims or Close Coil.** You choose Bank Rims. The illustration shows one reflection and the same total travel limit. You have added an angle to learn, not free damage.

**Crossfire — a smaller opening.** Biters approach across one side while the upper Spitter charges. You hit the nearer Biter, then dodge its shooter's locked lane while recalling. Two near pieces arrive before the banked misses. You release recall and make a fresh throw press into the wounded/near target; the narrow partial rays must actually connect to finish it. You hold recall again to resume the distant returns.

The next pack enters from a different pair of edges. You attempt a wall angle that would travel too far; the preview ends before the target, so you move closer instead. With some protection attached you can afford to stay through a tell, then aim around the Biter obstructing the Spitter. After both three-enemy packs are defeated, the arena becomes safe.

**Pick 2 — Lance Rails or Launch Treads.** You choose Launch Treads, keeping the baseline fan and single enemy hit per outbound piece. A short speed boost after a real throw may help cross an opening; it cannot grant invulnerability or refresh itself while active.

**Interception — change direction.** A Rammer braces among two Biters. You lure its line toward the outside, wait for lock and launch as you cross its path sideways with the Treads boost. You keep moving while empty. It rushes past along the marked line and pauses at the end, giving a useful aimed window. Your pieces still need to land and return; the boost does not pull them back faster.

The second pack adds another Rammer, a Spitter and two Biters. You mistake the shooter's tracking phase for the lock, turn back too early while empty and lose one hull pip. The hull sound differs from a block, and one second of damage grace gives room to recover. You complete the seven-enemy assignment with two hull. Nothing heals at the break.

**Pick 3 — Scatter Fork or Return Teeth.** You choose Scatter Fork. It is legal because you declined Lance. The build is **Bank Rims + Launch Treads + Scatter Fork**; returns remain harmless. The wider fan and stronger outward knockback invite close crowd-control shots, while narrow partial volleys remain available through the same fan formula.

**Pressure — use the full build.** The Sweeper marks a fixed strip while two Biters close in. You cross it during the warning, aim broadly across the jaws and use the boost from throwing to reach open space. Knockback buys room, but the three-hull Sweeper does not die from two ordinary hits. You dodge its pulse and use another aimed volley to finish it.

A Rammer, Spitter and two Biters make up the final ordinary pack. You briefly keep recalling with three pieces attached because a shot is arriving. One blocks it, ejecting into its quarter-second outward state. It cannot snap straight back on that same impact. You then release recall and throw the two still held pieces into a close Biter; if both hit, two hull are enough. A fresh recall retrieves the rest as you sidestep the Rammer. After the finite pack dies, scrap gathers and you continue directly to the finish without another card.

**Foreman — a learned finish.** The supervisor and two escorts enter. Its first aimed shot uses the familiar tracking-then-lock tell. You line up a Biter to remove close pressure but remember you have no Lance: pieces hitting it will not pierce into the boss. You step to a clear angle for the next throw.

The Fork tell shows two lanes and a gap. You move through an open route, choose a reachable wall bank for a stray piece, and recall in motion. A miss takes longer to recover, which you can see. When Drive locks its line, you sidestep and use the recovery window for a closer volley. New escorts can replace dead ones only until the four-escort total is spent.

The wide fan sometimes scatters damage; you approach carefully to concentrate more rays without colliding. Near the end, two pieces return before the others and the boss has at most two hull remaining. You release recall and throw that partial volley into its recovery window. If both rays connect, it is enough. A fast accurate player might finish much earlier; neither extra cycles nor every boss pattern is compulsory.

**Payoff — and an honest reason to return.** The Foreman powers down immediately, surviving escorts stop, lamps dim and Pip's work light returns. You clear with two hull and the three chosen cards. The results show that route/loadout's active time, one hull hit, the clear medal and the still-open clean-clear goal. Overdrive becomes available.

You can retry the same route for a cleaner clear, try Split Bench's cover, or jump directly into Bank Job to practise a weak technique. A different card selection is another possibility, not proof of a different play style. If you feel finished and do not want any of these, record that response. The menu cannot manufacture replay value.

## Concise rationale linked to the review

| Material v0.2 change | Review finding addressed and reason |
|---|---|
| Exact recall-priority inputs, visible failed-action feedback, one isolated alternative | [Gameplay review: input/state load](../2026-09-07-scrapstorm-review/gameplay-review.md). Keeps the original reliable control baseline testable and makes its possible friction visible without a selection interface. |
| New-crossing eligibility, contact exclusion and one return hit per launch cycle | [Audit: return contact risk](../2026-09-07-scrapstorm-review/coordinator-evidence/audit.md). Removes the paper ambiguity that could make every contact recall an automatic kill; it does not prove collision code or good feel. |
| Explicit ejection travel, enemy locks, finite pack release and stress-test tuning order | [Integrated review: safe repetitive play](../2026-09-07-scrapstorm-review/README.md). Makes tanking, close fire/recall and perimeter play falsifiable without assuming a comfortable rhythm is an exploit. |
| Three courses, four threat roles, three picks, fixed goals and direct finale practice | [Commercial review: complete-package/replay risk](../2026-09-07-scrapstorm-review/commercial-review.md). Adds geometry, commitment windows and fair mastery goals beyond one clear; keeps their actual value unproven. |
| Eight specified cards, Grounding Guard removed, declined-card reoffers, explicit reduced pool | [Audit: four-card failure](../2026-09-07-scrapstorm-review/coordinator-evidence/audit.md). Reliable offers survive the intended reductions; configuration counts are not play styles. |
| Learned-threat Foreman and a clock-free, harmless-return sample run | [Gameplay review: unsupported finale and Return Teeth dependence](../2026-09-07-scrapstorm-review/gameplay-review.md). The finish ends when solved, and the baseline experience stands on its own in the written example. |
| Sixfold Recoil and combat-first positioning | [Review title collision](../2026-09-07-scrapstorm-review/README.md), followed by the [new bounded name check](naming-check.md). Preserves the stable opportunity ID while avoiding the observed exact Steam collision. |
| No imposed time cap; actual increment cost/value replaces old calendar arithmetic | [Corrected coordinator audit](../2026-09-07-scrapstorm-review/coordinator-evidence/audit.md). Uses the local AI workflow, owner correction and unknown demand honestly. |

**Intentionally not adopted:** damaging recall is not promoted on a designer's preference; H remains the initial baseline and D is compared before content. Extra pressure/recovery penalties are not applied merely because passive waiting is conceivable. Overdrive is not a harmless first cut when it supplies mastery. The v0.1 four-encounter/six-card inventory is not preserved as a ceiling, and a higher price is not inferred from larger proposed counts. Global recall, full movement, deterministic shielding and the rejection of passive block damage remain intact because adding friction would not itself improve enjoyment.

## Next recommended playable test and decision rules

**No build or human test exists.** The next task to authorize is a focused playable core comparison: Open Tray; six conserved pieces; H and D toggles; Biter and Spitter; one short finite two-pack encounter; three hull; clear/defeat; explicit action/block/hull sounds; pause and immediate same-seed retry. Start with the Opening roster, then a Crossfire fixture with near hits and far misses. A few minutes is a session-planning suggestion, not a survival timer, development-day allocation or owner-time cap. There are no cards, courses or progression in this first experiment.

### First try to disprove the mechanics

| Probe | What to observe / fail condition | Focused response if it fails |
|---|---|---|
| Piece conservation and contact | Block at a wall/corner, land on enemies/boundaries, toggle recall, pause during each state, and clear with scattered pieces. Fail on lost/duplicated IDs, early ejected-piece catches, return damage from overlap or multiple eligible return hits in one cycle. | Fix the invariant/collision rule before interpreting any preference or challenge measurement. |
| Intended throw and partial windows | Both input devices; simultaneous presses; holding both; near-hit/far-miss versus clustered returns. Log held count and rejected throw edges, then ask what the player meant to do. | If repeated intended throws are ignored, compare only the throw-ends-recall alternative. If partials remain rare/unhelpful, de-emphasize them; do not add an attack-selection interface. |
| Stationary recall / corner shielding | For a short bounded observation, deliberately hold recall without moving, then insert close aimed throws. Record hull loss, block/catch cadence, clears and active clear time in single- and mixed-threat fixtures. | Safe waiting alone cannot win and is not an automatic defect. If waiting plus close throws clears mixed fixtures with negligible execution and replaces wanted mastery, first revise existing entries/Spitter timing; then compare one recovery change, e.g. 3.5-unit ejection with a 0.45-s minimum outward state. Never combine several remedies before learning which helps. |
| Perimeter and close-range cycling | Try clockwise/counterclockwise perimeter fire/recall and the closest repeatable shot distance, against adjacent and opposite entries. Compare with ordinary aim/route play. | A useful repeatable rhythm may pass. Revise only if the same low-attention policy erases skill/pressure or makes further play unwanted. Use visible entry/tell arrangements first; no permanent ammo loss, block damage or unavoidable empty-state attack. |
| Avoidability and readable failure | Evade the worst authored mixed tell while empty at base speed; test grace/contact at boundaries and loss of focus. | Repair trapped routes, stacked warnings or unclear block/hull cues before calling increased kill rate better balance. Later repeat this for Rammer/Sweeper, each course and Overdrive. |

A tuning pass should change a named cause and repeat the affected probe. If a collision/contact bug distorted D's damage, discard preference results from that broken build. Static checks cannot establish the outcomes of these probes.

### Compare harmless and damaging recall without confusing power with routes

Use equal initial exposure, the same seed/roster/hull and no cards. Reverse H→D and D→H order between available players; record order, input device and prior experience. Start with H at 10 return speed and D at 7.5 with the defined crossing rule. Record active clear time, kills per active minute, outbound/return damage, hull loss, stationary versus repositioned return hits, intended partial shots, ignored throws and what the player wants to repeat. A faster/easier D clear is information about power, not proof of route-based enjoyment.

If D looks better, identify whether players deliberately create crossings and describe those moments. If preference might be explained by speed/power, compare **H at 7.5 speed versus D at 7.5** on the same roster to isolate damage, then use a separately labelled pressure-matched fixture: adjust only the number/overlap of existing warned enemies until clear rate and hull pressure are approximately comparable. Preserve avoidability; do not inflate health or shorten tells to force a match. Record the changed roster and failure to match if applicable. This diagnostic follows the initial comparison, not a factorial test required of everyone.

Promote D only if understandable, wanted return attacks or an improved repeatable action survive that challenge check without dominant contact/close-range abuse. If players prefer D solely as easier damage, it may still reveal a desired difficulty change, but it does not validate the return-routing claim: retune H or revise D and compare again. Keep H if its comfortable rhythm earns practice and replay. If D is promoted, remove Return Teeth from the seven-card product pool, retain its speed factor exactly once, adjust Crossing Lines as specified, version records, and retest Heavy/Coil, enemies, boss and pacing. Do not gate the chosen core action behind offers.

### Human observations stay separate

Observe three to five available target players without coaching the desired sequence. Availability is not assumed and no recruitment/contact occurs in this task. With five participants, use the review's small-sample signals below as directional evidence; for fewer people, report raw counts and uncertainty rather than pretending the same threshold is statistically reliable.

| Observation | Record separately | Direction for the next decision |
|---|---|---|
| **Understanding** | Can the player explain why a hit blocked versus damaged hull and how the same pieces attack/recover? | Aim for 4/5 explaining correctly. If not, revise controls/feedback and re-observe before making a fun conclusion. |
| **Enjoyment and mastery** | Specific moments they want again, actions that feel like work, deliberate changes of aim/timing/routes, and improving execution. | Seek 3/5 varying a relevant choice or showing wanted improvement in a comfortable rhythm. Tactical novelty is diagnostic, not a quota that overrules observed enjoyment. |
| **Defeat retry** | Among players who lose and are free to stop, who starts another attempt without being directed, and why? | Seek at least three voluntary repeat attempts across the five sessions, but always split defeat retries from post-clear replays and state each denominator. Curiosity/observer compliance is not durable replay evidence. |
| **Post-clear replay** | Among players who clear, who independently chooses another clear or fixed goal, and why? | If nobody clears, this evidence is missing. Aim for at least two of at least three clearers wanting another meaningful attempt before calling replay promising; zero or one prompts a specific goal/content revision. These are decision aids, not retention estimates. |
| **Later-session interest** | At a separately available later session, does the player choose to return when stopping is acceptable? State interval, invitations, attendance and who actually plays. | No response/unavailability is missing evidence. If willing returners quickly feel finished, revise the product-depth hypothesis. Do not call same-sitting retries later retention. |

After the core passes, use the representative encounter/course/challenge increment to test post-clear and later-session value before multiplying inventory. Show a truthful package card and $4.99 hypothesis only after players have a representative experience. Record a price objection separately from “I do not want this game,” alternative products, presentation comprehension and stated intent. No five-person count forecasts paid sales.

**Proceed recommendation:** controls/invariants are sound, players understand the consequences, enjoy improving and voluntarily repeat, and a concrete next content increment has a credible benefit at its measured AI/cash/attention cost. Core success recommends the next increment, not full production selection or a higher price.

**Revise recommendation:** feedback/input confusion, power-confounded D preference, ineffective cards, a solved low-skill policy, poor post-clear motivation or an unreadable threat has a concrete change worth testing. More useful content is permitted if the action works; repeat the affected comparison rather than restarting the broad market funnel.

**Hold/redesign recommendation:** after focused fixes, play remains unwanted or no specific next change has a credible route to enjoyment; or an honest package's plausible obtainable proceeds fail to justify actual costs/attention. Lack of measured fun/demand remains uncertainty, not an invented failure result. Needing more development time or useful content alone is not a hold condition.

## Provenance and document verification

The source v0.1 is preserved byte-for-byte as [v0.1.md](v0.1.md). The dated review and linked supporting snapshots are copied without changing their conclusions. The earlier research run is included to keep the original source-link chain usable; its other concepts and superseded budget discussion are historical context, not new portfolio records or current v0.2 instructions. The original absolute source locations remain unchanged.

New rules, product inventory, walkthrough and recommendations are studio design hypotheses dated September 7, 2026. The new public research is the bounded [naming check](naming-check.md); comparison prices and historical disclosures retain the current review's source dates, filters and limitations. No competitor was played, no game was built, and no human enjoyment, price acceptance, sales or game-specific delivery duration was observed.

**Document/spec checks, 2026-09-07:** `python scripts/studio.py validate` passed. All 17 numbered topics are present; 17 Markdown files and 65 local links (including the linked heading) resolved, and nine supporting JSON files parsed. Source hashes confirmed all 25 original files unchanged and all 24 imported archival/evidence files byte-identical; only this checkout's current opportunity and new design material were revised. The opportunity retained its stable ID, shortlisted status, low confidence, fun score of 2 and both prior decision entries.

Exhaustive standard-library Python enumeration of the stated unseen-first/declined-fallback algorithm found **zero impossible offers** over 15,840 complete H offer/selection sequences, 3,600 D sequences and 24 reduced-pool sequences. Their legal final configuration counts are 50, 30 and 5. The Lance/Coil reduced-pool regression correctly reoffers Coil alongside Treads; a third pick with that four-card conflicting pool is explicitly invalid. The walkthrough's Bank → Treads → Scatter selections are legal, and its recall is harmless throughout.

`git diff --check` passed; explicit `git diff --no-index --check` checks also covered the three newly authored/updated files because they began untracked in this checkout. Manual specification review reconciled five-encounter pacing, three picks, fixed challenge loadouts, damage/health examples, return-speed stacking, state ordering and knockback during locked attacks. Historical v0.1/review prose remains unchanged and is not mistaken for current rules. These checks validate documents, records and arithmetic; **none is a collision simulation, playable build or human fun test**. External-link checking used the current review's dated verified sources plus the new naming work; every historical external link was not re-fetched.
