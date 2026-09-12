# Rework presentation: readable physical salvage

2026-09-12. The recommendations below began with the rejected demo captures. Root has now implemented the rework scene and inspected actual R1 and final R2 renders. The unedited [R2 board capture](../evidence/rework-board.png) shows distinct iron/copper/alloy/cell/core silhouettes, modeled bundle connections, a teal magnet, warm furnace and darker HUD panels. Native QA observed pieces moving before attachment, connected groups moving together, cargo pouring and an ingot payout. These are rendered observations, not proof of enjoyment or sales appeal. The final package cooks the engine ambient cubemap missing from R1. Hard cast shadows remain a style limitation; they are not described as fully polished.

Final risk wording now says Drop haul, releases all salvage recoverably and turns the field off. Expired warnings spend one fuel charge and the forecast most valuable piece. The historical recommendations below do not override BRIEF.md.

## What the actual capture shows

The earlier tray reads as a largely flat dark panel with small scattered props. Thin straight links look like diagram connections; their shared physical load is not evident. The magnet and scraps have weak size hierarchy. The large empty frame/HUD competes with the small working area. The furnace is a bright orange disc, and the finished metal block reads as another small prop rather than a substantial transformation.

Keep the attractive teal/copper workshop palette, but spend more of the view on the functioning machine and material. Reduce static header/footer height; reserve nearby space for the changing load, actual pending loss and four physical fuel charges. Do not explain material identity or force entirely with text.

## Shapes and materials that carry the rules

| Element | Recommended readable form |
| --- | --- |
| Iron | Cool dark-steel bolts, washers and angular offcuts. Clear bevel highlights and varied broad roughness; reliable everyday salvage. |
| Copper | Warm continuous wire curls and compact spools with obvious circular cavities. Distinguish them by silhouette as well as orange color. |
| Alloy | Larger machined hexagonal/faceted chunks, pale silver with a restrained blue tint. Broad planes and greater bulk communicate mass. |
| Hot cell | Upright/diagonal black cylindrical housing, exposed red/orange cap, warning bands and a small intermittent spark. Never just recolor a washer red. Its danger halo is visible before attraction. |
| Rare core | Distinct protected inset inside a split metal casing; a small cool light and deliberate glint attract attention without lighting the entire board. Keep its outline recognizable while carried. |
| Bundle | Several clearly linked pieces connected by thicker modeled chain/wire with visible slack. The group highlights together and has one shared mass/value readout. Lines must remain visible at gameplay resolution. |

Scale material volume consistently enough that a 4 kg chunk looks more substantial than a 2 kg washer group. Place key silhouettes far enough apart to read before force moves them. Use a rough work surface, sharper metal reflections and contact shadows; flat light rectangles do not convey material. Keep hazard, ordinary copper and the furnace distinct by shape, brightness and animation, not red/orange hue alone.

## Show the magnetic action before the number changes

Loose pieces should tremble or rotate at the edge of the active field, then slide and accelerate toward the poles before attaching. Heavy bundles lag and tighten their links; lighter pieces respond sooner. Field release before contact changes that motion. A pulse creates an outward response and a short visible recoil. These are cues to real interaction state, not a canned post-award arc.

The magnet itself needs a modest engaged vibration and pole glow, with a dense attached load that does not cover the projected field or nearby hazard. Warning can shift coil brightness, add a localized spark and animate the load strain; the main warning names the exact piece at risk. Avoid full-screen red flashes and constant camera shake. Destruction is a brief distinctive fracture/burnout; recoverable spill remains recognizable salvage on the tray.

The furnace needs depth: dark refractory rim, recessed molten surface, a few bright internal reflections and a solid acceptance opening. An accepted dump arrives as recognizable pieces, heats to orange, sinks/melts into moving liquid, then yields an ingot/payout and a visible installed mod. Four illuminated fuel cartridges/heat indicators beside it explain the contract limit. A refused unstable load leaves this whole sequence idle and explains why.

## Original audio delivered

`assets/audio/rework/` contains 19 stereo 48 kHz WAVs, including magnetic engagement/body, three metal contacts, heavy contact, rare find, vent, warning, overload, smelt, payout, upgrade, contract outcomes, UI, furnace ambience and **Copperlight Workshop**, a 96-second original instrumental loop.

The exact `/Game/MagnetSweep/Audio/rework/<cue>` import names, levels, loop flags, trigger priority and concurrency are in [PROVENANCE.md](../assets/audio/rework/PROVENANCE.md). Root merges the supplied manifest rows and owns engine mixing. Cue timing must follow actual force/contact/settlement, particularly pickup on contact and warning persisting after field-off. Music is a calm bed; it ducks for danger instead of competing with it.

## Actual render/listening acceptance

- At ordinary gameplay scale, identify iron, copper, alloy, hot cell and rare core without opening a tooltip. Identify which pieces belong to a bundle before moving it.
- While gently sweeping, see a light piece begin moving before capture and a heavier linked group resist/respond differently. A still image cannot pass this check.
- While overloaded, name the endangered item and distinguish destroyed material from recoverable spill. The warning must remain legible under the largest intended load.
- Watch one complete smelt. The owner should see material become something useful and immediately recognize the next purchased change on the rig.
- Hear repeated pickups, a large bundle, a warning corrected by vent, a real overload, a smelt and music across a loop boundary. Listen for weak body, metallic harshness, buried warning, abrupt loops and music fatigue.

Audio evidence includes original file/numerical analysis; the tool runtime explicitly rejected model audio input, so neither agent nor root has heard this palette. Native component activity is a separate technical check recorded in QA-REWORK.md. Actual R2 visuals have been inspected as described above; perceived musical comfort, impact quality and owner acceptance remain unverified.
