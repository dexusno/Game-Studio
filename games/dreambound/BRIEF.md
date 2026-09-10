# Cyborg between worlds — first beta
Updated 2026-09-08. Klaus authorized building the beta. Internal directory name dreambound is not the final title.

## Current owner refinement — September 10
Klaus asks for a higher art bar informed by AAA/high-end indie references, detailed organic enemies (only the player is established as a cyborg), and more reusable props, fauna and habitat-specific variation. Keep the preferred player arms/weapon. The next increment adds wet spring and dry ruin dressing to the existing connected procedural layout and new biological enemy art; broader realms remain future scope and creature lore is not settled.

Movement needs physical weight. Shift is hold-to-sprint with stamina depletion, delayed recovery and an on-screen meter. Dash has a separate button and a brief forceful collision-swept evasive burst. Left Alt is the current implementation choice. Ground-speed-driven gait, landing and acceleration response should communicate a body moving through the world while preserving mouse control and the existing combat loop. Two owner-supplied Suno downloads are being tested for charge/release sound.

## Latest owner refinement — segmented shield and procedural art

Klaus's latest playtest found the idle shield obstructive, charge too quick/weak, sound poor and melee too short/boring. He selected a collapsed default weapon form with animated expansion for guarding. The current corrective increment folds the physical pieces, slows full commitment to 1.8s with a distinct full-volley payoff, improves melee reach/combos and replaces the audio palette. A secondary weapon is only a fallback to consider if this still fails to carry combat. He asked whether tools/packs would help: his subsequent correction requires an art-creation pipeline across biomes; a ruins kit is insufficient. His latest research request covers current developer experience, commercial leaders and latest open models, including cloud GPUs/APIs. The resulting proposal compares Meshy 7, Rodin Gen-2.5, Tripo P2.0 Preview and TRELLIS.2 with Blender finishing; no purchase or quality winner is established. These changes are hypotheses for play, not evidence of fun or anchor-B quality.

The next combat foundation is the [segmented shield](SHIELD-DESIGN.md): holding LMB lights individual pieces sequentially, release launches the lit pieces, and the remainder stays available for defense. Spent defensive pieces and projectiles destroyed by qualifying enemies each start their own regeneration; intact deployed pieces can be recalled and visibly reattach. The new segmented experiment implements six identities with independent regeneration; current build evidence is in STATUS.md. Count, durability and timings remain provisional tuning. Q recalls; F is the separate heavy action for this playtest.

Rewards must state their input or automatic trigger, visibly demonstrate their effects, and retain those instructions after acquisition and ranking up. Klaus could not tell how to activate the study's rewards or see their special effects. The art target remains B, with a substantial observed gap. The same finished modules must work across different generated layouts with readable encounters; a static beauty scene cannot satisfy the procedural requirement. Premium tools and Unreal-specific libraries are now explicitly part of the research, with purchases still requiring a concrete approved choice.

## Owner playtest correction — 2026-09-08
Klaus rejected beta2's gun-like weapon, weak feedback, shallow/confusing combat, plain enemies and graphics far below anchor B. Its existing pulse/guard implementation and eight-room content pass are historical implementation choices, not an accepted gameplay or art foundation. Movement was usable but felt generic. [Evidence and next experiment](RETROSPECTIVE.md).

The shield itself must be the physical weapon, with meaningful attack/defense commitment and launch/recall. Free choice remains important: retaining it for close attacks, guarding and counters must be useful, and throwing must not force a single repetitive all-at-once sequence. Broad combinable upgrades remain the direction.

The corrective increment is a connected set of three procedural courts with a properly authored and animated hero shield, contrasting threats, clear instruction through play and two earned upgrades that change decisions. Demonstrate actual movement, block/strike/launch/recall/catch, powerful synchronized sound/contact reactions and coherent scenery before expanding rooms or the item count. The exact controls, timing, shield topology and two attachment examples are proposals to test. This is a correction to the next implementation step, not a reduction of the eventual game to a small arena.

Antagonists, their motives, reasons for fighting, world-specific enemy identities and boss/recurrence logic remain unresolved. Klaus explicitly deferred that story work. Existing narrative sketches are not approved canon.

## Play question
After earning an attachment, does Klaus change positioning, target selection or defense and want to try another build? If rewards merely increase damage or the same safe loop wins every encounter, revise before expanding the campaign.

## Corrective study boundary
The current source assembles three seeded courts, three escalating encounters and two earned choices. Partial throws retain protection; Q recalls survivors, F bashes/rushes, and spent or destroyed pieces independently rebuild. Optional safe practice and retained ability instructions follow rewards. One biome and a small modular kit remain the scope; this does not establish the intended campaign or art fidelity. Full ordinary completion time and enjoyment remain unmeasured.

## Previous route scope — retained campaign intent
Windows first-person Unreal 5.8.2. One modular weapon/shield with freely chosen attack and guard, timed defense, a close impact/special, dash and jump. One substantial medieval dream realm assembled from authored encounter spaces, an optional disclosed attachment pursuit, several early reward choices, a boss, and a compact technological arrival where earned equipment remains usable. At least two obtainable contrasting builds; no developer menu substituting for acquisition.
A complete beta journey is provisionally 15–25 minutes, subject to actual playtesting; this is not the campaign duration. Baseline actions must be viable. Preserve earned attachment patterns and a boss milestone on failure and restart; resume the current seeded journey. New expeditions vary layout/encounters/offers. Same-seed restart is available for practice.

## Rejected beta2 implementation choices — historical
LMB aimed pulse; RMB held guard with a short timed deflection window; Q deliberate impact or installed special; Shift dash; Space jump; E interact; 1/2/3 choose rewards; Tab inspect build; Escape pause. Mouse sensitivity adjustable in pause.
First reward families: Mirror Facet, Ram Edge, Echo Chamber, Frost Core, Ember Core, Storm Core, Stormfracture, Split Prism and a boss capacitor. Implementation must explain slot/stack behavior; early choices offer a usable route, optional challenge previews a desired reward. Earned powerful combinations are allowed; finite secondary-effect chains prevent duplicate state consumption.
Threats: a closing melee sentinel, a projectile caster, a mobile pressure enemy, and a guardian with distinct aimed, close and ground attacks. Mixed encounters and cover challenge static guarding.

## Presentation
Selected style B: sculpted painterly 3D, substantial beveled forms, tactile ceramic/metal/stone, broad painted color variation and expressive lighting. Original modeled weapon and modular courtyard art, visible physical attachment changes, restrained effects and sound. Keep enemy tells visible in first person. Compare actual rendered gameplay with the concept direction; concepts do not establish achievable fidelity.
Weapon self-awareness remains a proposal. A restrained optional awakening hint can be tested without settling its origin, adding constant chatter or changing player control.

## Scope and resources
Owner Klaus; integration owner current task. Initial internal implementation checkpoint: approximately two agent workdays, an adjustable planning estimate rather than a delivery promise or quality cut. Owner playtest request: roughly 30 minutes once the representative build passes technical review. Cash allocation zero; purchases require Klaus's approval. Installed tools and vetted free software may be used. No publication or external accounts required.
No full campaign, multiple complete biomes, dragons, multiplayer, live AI dialogue or exhaustive catalogue. These exclusions do not replace the intended larger game.

## Acceptance
Technical requirements remain actual Windows launch, input/focus, earned rewards, failure/retry, persistence and eventual route completion. The next result must also visibly and audibly establish the physical shield fantasy and approach anchor B in an actual authored space. Independent QA distinguishes moving play from staged calls. Klaus's beta2 assessment is negative; the corrective study must earn its own owner assessment. Technical passes cannot establish improved fun.
