---
name: game-ui-ux
description: Design, critique or implement distinctive game UI and understandable player flows, including HUDs, tutorials, equipment screens and risk/reward feedback. Use for game interface and usability work, not general website design or an unrequested gameplay redesign.
---

# Game UI/UX

Make the interface feel native to the game and understandable during play. Treat visual appeal, comprehension, interaction and emotional payoff as connected design problems. Neither decoration nor removing all text is a substitute for designing the experience.

## Establish the player's situation

Read the current assignment, owner feedback and relevant game records. Inspect the actual screens and implementation needed for this task; avoid loading the whole design archive. Respect the requested mode: critique, design, prototype, implementation or specialist setup. Setup alone authorizes no game changes. Owner rejection takes precedence over an earlier agent's positive review.

Resolve the interface's player-facing contract from the available evidence:

- Who am I, why am I doing this, and what does the larger goal change?
- What am I trying to accomplish now, and what action is available next?
- What will that action affect, cost or risk before I commit?
- How will I recognise success, failure and progress, and what carries forward?

Separate established fiction and rules from proposals and missing decisions. If the game has no meaningful loss, purpose or progression, report that to the game designer/producer; labels and animation cannot supply missing systems. Do not silently choose a backstory, invent a penalty, rebalance rewards or expand mechanics during a UI assignment. Continue any authorized interface work that does not depend on that choice. Mystery can be intentional; distinguish it from missing information needed for a fair decision.

## Learn from games, then make an original solution

For a substantial redesign, use [references/pattern-library.md](references/pattern-library.md) to choose useful comparisons. Match camera, input, decision tempo and information complexity; inspect a contrasting approach when it helps expose a tradeoff. A small fix can use established project evidence without a new research exercise.

Identify the specific problem each reference solves, the observed interaction or visual treatment, why it might transfer, and where it would fail in this game. Prefer shipped game captures, developer talks and first-party material. Record URL/version or timestamp, access date and what was actually inspected. A talk abstract is not a watched talk; a remembered example is a lead, not evidence. Learn structures and techniques without copying proprietary assets or a recognizable branded interface. There is no quota of games and no claim of personal experience playing countless titles.

## Shape attention and the journey

Start from the player's task sequence, not a grid of panels. For each element ask which decision it supports and when it is needed. Keep the play area dominant during action. Use persistent HUD for information that must remain available; contextual prompts for an immediate action; deliberate inspect/menu views for comparisons and detail; and a short debrief for consequences and next steps. These are options, not mandatory screens.

Promote information as its urgency changes. A danger warning must survive reward messages and identify the threatened resource or object, consequence and available response. Normal state should be calmer. Anchor a cue to its subject when this reduces searching, but prevent world labels from obscuring targets or piling up. Avoid distant places the player must read simultaneously under pressure.

Reduce clutter by removing duplication, sequencing explanation and improving interaction. Do not shrink the font, hide essential rules behind hover, replace every label with an unfamiliar icon or distribute a single decision across many menus. Preserve useful depth: equipment comparisons still need effects, cost, compatibility, capacity and the consequence of replacing something. Put the decisive facts together; reveal technical detail on request.

Teach through a short action, visible response and confirmation of the player's real progress. Use concrete verbs and current input glyphs. Point at the actual destination or target. Advance a tutorial on the demonstrated action, not an unrelated timer or click through paragraphs. Provide recoverable instruction when interrupted or returning later. Readability improvements do not authorize spoiling a puzzle's solution or removing its challenge.

## Give the interface a visual identity

Derive the visual language from the game's player fantasy, art direction and tone. Specify only the useful system: hierarchy of display and reading type, spacing, silhouette, palette, surfaces, icon grammar, focus/selection treatment, transition behavior and sound cues. Coordinate with art and audio owners for assets; don't make every game an industrial console or reuse a web-dashboard template.

Make one focal action or decision visually lead each state. Use deliberate composition and negative space, not equally weighted cards everywhere. Reserve ornamental fonts for places they remain legible; use readable body type and distinguish numbers and symbols clearly. Icons need consistent meaning, states and accessible names; pair unfamiliar symbols with concise labels until learned. Texture, borders and animation must reinforce hierarchy, not compete with it.

Use motion and sound to connect action to result: response to input, a resource change at its source, a reward visibly reaching its destination, a clear transition after completion. Avoid gratuitous shakes, long unskippable reward sequences, constant blinking and sound as the sole carrier of essential information. Tune feedback to repeated use as well as its first impression.

Use ImageGen for useful concept/style explorations or raster assets, Blender/3D tools for appropriate spatial elements, and engine-native layout and text for actual interaction. Inspect inputs before editing and track asset provenance. Generated text-heavy screen art is a concept, not a functioning UI. Don't invoke every tool just because it is available.

## Specify behavior, accessibility and integration

For changed controls, identify the actual state source and event, available/selected/blocked/committed states, input, visible response and resulting transition. Name a blocked action's reason and remedy. Differentiate selection, preview and purchase/commit; display the real cost and result. Distinguish carried, banked, earned, equipped and unlocked when those are different states in the game. Failure feedback must accurately explain what caused the loss, what was lost or retained, and what a retry resets.

Keep UI and world input from firing together. Define focus capture, cancel/back, pause behavior, restoration after closing a panel and interruption during feedback. Respect existing bindings, save semantics and the player's context unless the assignment changes them. In an implementation handoff give the engineer concrete assets, state bindings, layout/scaling rules and acceptance behavior, not only a mood board.

Check final rendered text at the target resolution and viewing distance; font point size alone is not evidence. Reflow at UI scaling and longer localized strings rather than squeezing. Use redundant shape/text as well as color for essential states. Provide predictable focus and access to essential explanations for supported input methods, not mouse hover alone. Include contrast against actual moving backgrounds, focus visibility, reduced motion and subtitle/caption needs where relevant. Read the linked Xbox guidelines when needing specific criteria; do not claim platform certification from a partial check.

## Verify the experience and hand it back

Scale verification to the changed flow. Inspect representative real states at gameplay size: quiet play, an important action/selection, pressure or blocked action, and its outcome as applicable. Check relevant resolution/scaling, overlap, longest meaningful labels, input focus and interruptions. A screenshot can establish layout observations, not responsiveness, navigation or comprehension in motion.

For substantial work, use an independent uncoached interaction pass when available. Give the tester a player goal rather than instructions that teach the route. Ask them to identify the objective, next action and consequence, carry out the task, and explain the result. Record wrong turns, hesitations, misread symbols and unnecessary reading, as well as what worked. Label scripted checks, agent heuristic review and human observations separately; none proves enjoyment or sales. If only static review is available, report the remaining interaction checks without presenting them as passed.

Return the concrete artifact or prioritized findings, what changed for the player, inspected evidence, unresolved decisions and limitations. For critique, connect each issue to a location/state, likely player error and proposed remedy; distinguish cosmetic preferences from comprehension, control and consequence failures. Give the integration owner the concise status/provenance update. Stop at the assigned deliverable; don't launch an unsolicited redesign or add systems to demonstrate the role.
