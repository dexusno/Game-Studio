# Magnet Sweep — proposed complete redesign plan

Prepared for Klaus, 13 September 2026. **Sequence remains a draft for owner revision. Five premise options and a native Slay the Spire 2 combat study are delivered; no premise or detailed combat rules are selected. Art production and redesign implementation have not started.**

Latest steering: Klaus proposes round-based combat in which the hanging magnet supplies a smart forge weapon, combining scrap into attacks or defence against enemies that fight back. The [play study and proposed combat direction](SCRAP-COMBAT-DIRECTION.md) recommends developing one complete mid-game round, intermediate stage goals, useful rewards and loss/reset rules together before visual production. This brings progression design forward from step 7; its later playable implementation still follows owner-set goals. The original nine-step sequence below remains a reference for review, not automatic authorization.

The aim is a game whose purpose, objects, actions and consequences make sense while you play, with an expressive swinging magnet and a visual style you want to spend time in. This is a redesign of the whole experience. Existing features earn their place by serving the new design.

Further owner steering: assemble physical ammunition by stacking components, spend it when fired, and consider retaining unused pieces between turns but refreshing stock each fight. Single-target and multiple-target effects, poison, piercing and scrap-built armour should compete for resources and energy. Loading should visibly build anticipation as each component contributes before the shot. The [completed 595-entry strategy study](BUILD-STRATEGY-STUDY.md) translates reference-game synergies into this direction, including semi-random early rewards and run-long rig capabilities. Its detailed rules and example values are proposals for step 2, not approved implementation.

## How we will work

You review and revise this plan first. You then set a goal for the step you want to pursue. We complete that goal, give you something concrete to judge, and revise it until you are satisfied. We begin another step only when you set its goal. A step can be split into smaller goals if useful. No development timetable is imposed.

Already directed by you: a **2.5D side view**, metal that can stack vertically, a magnet hanging from a chain or similar suspension and swinging as it moves, important physical behavior, and at least **five overall visual styles with five variants each** to choose from. Readable objects, visible cause and effect, plain instructions, useful progression and meaningful wins/losses apply to every step.

Still to choose: who you are, what you are working toward, the world/tone, exact controls, level objectives, loss and retry consequences, progression/reset rules, visual style and content scope. A dystopian survivor, factory operator or construction project is not selected merely because it was mentioned earlier. Neither the current four-site structure nor its equipment catalogue is automatically retained.

## 1. Decide who we are and why the work matters

Current deliverable: [five premise options](PREMISE-OPTIONS.md), awaiting Klaus's choice or revisions. Preparing the choices does not settle the premise.

Establish the game's simple premise: the player's role, their situation, why using this magnet helps, and the larger result they want to achieve. Connect ordinary recovery work to a visible end goal. Keep the backstory compact enough to communicate through the opening, environment and progress rather than a lore manual.

**What you receive:** a few short premise options, each explaining who you are, what a typical job accomplishes and what changes when the game is completed. We develop your preferred direction.

**What you judge:** “Do I understand why I am doing this, and do I care about reaching the end?”

## 2. Decide how a level works, including how we actually lose

For the proposed combat direction, use a complete encounter and one detailed round: enemy intentions, available scrap, powered loads, attack/shield choices, forge preview, material consumption and replenishment, then enemy response. Connect encounter rewards to a stage guardian and the eventual final mission. Settle the strategic role of the physical magnet before multiplying materials and status effects. The study's examples and three-stage outline are proposals, not selected rules.

Describe one typical mid-game level from arrival to outcome. Give it a concrete objective, a reason to take an optional risk, an understandable success condition and a real failure condition. Decide what failure costs, what is retained, and what restarting a level or expedition means. The actual losing rules must survive the retry/save flow; unlimited undo must not quietly erase the agreed stakes.

Explain how careful play differs from a risky move and how a player can recognise danger early enough to make a choice. Avoid surprise punishment caused by unexplained rules or unstable physics. Consider what happens if the objective becomes unreachable so the player gets a clear resolution rather than a dead end. Exact penalties and resources are choices for this goal, not decisions made by this plan.

**What you receive:** a short illustrated level walkthrough, a safe/risky decision example, and a plain explanation of win, loss, retreat and retry where applicable.

**What you judge:** “Would beating this feel earned? Could I lose, understand why, and want another attempt?”

## 3. Choose the graphics through 25 comparable examples

Create **at least five distinctly different style families, each with five variants**. The following are proposed families and directions to explore; you may change them before setting this goal.

| Overall style | Five variants within that style |
| --- | --- |
| Pixel art | Chunky retro; detailed modern pixels; restrained industrial palette; vivid arcade palette; atmospheric pixel noir |
| Realistic 3D | Clean modern workshop; worn working scrapyard; heavy weathered industry; cold abandoned facility; warm restored machinery |
| Cartoon | Bold cel shading; rounded playful forms; comic-book outlines; retro animation; flat graphic cutouts |
| Hand-drawn / line art | Fine pen and ink; rough pencil; bold brush outlines; colored line drawing; watercolor with ink contours |
| Stylized 3D | Miniature diorama; clay-like materials; carved wooden-toy treatment; sculpted painterly surfaces; simplified graphic materials |

All 25 examples use the **same representative side-view composition**: hanging magnet, stacked recognisable scrap, one visibly connected mechanism and a clear collection destination, suited to the premise selected in step 1. Show comparable framing and gameplay-scale object sizes. Variants should change shape treatment, materials, outlines, shading or atmosphere meaningfully, rather than merely changing one tint. Keep the comparison about style; do not invent a different game or backstory for every picture.

**What you receive:** 25 labelled art anchors plus comparison sheets, with larger individual images available for inspection. First choose a family, then refine a variant. Record the selected palette, shapes, material treatment, lighting and interface direction. These are concept references; their achievable appearance must later be checked in the playable game.

**What you judge:** “Which world would I enjoy looking at, and can I clearly recognise its objects and actions?”

## 4. Make the hanging magnet feel good in a small playable scene

Build an isolated side-view physics experiment using the chosen visual direction on a representative magnet and a small set of scrap. Decide how you move its suspension point, raise/lower it and control the field. The chain or suspension should visibly support the magnet; acceleration, stopping and carried weight should produce understandable movement.

Test pieces pulling toward the magnet, colliding, attaching, hanging, falling, sliding and stacking. Check whether pulling one piece disturbs a pile in useful, readable ways. Keep interaction on a clear, reachable side-view plane while using depth for visual appeal. Tune control and damping so skillful handling is possible and ordinary movement does not become a struggle against wobble. Add essential pull, contact and release sounds now.

Use just enough temporary UI to operate and judge this experiment. It is not yet the full game or tutorial. Reuse existing source/assets only where they support the new behavior; preserve the old prototype and saves separately.

**What you receive:** a playable magnet-and-scrap scene with representative art, direct controls and observable physical responses.

**What you judge:** “Is moving, pulling, lifting and dropping enjoyable before we add a whole level around it?”

## 5. Build one level whose machinery explains itself

Turn the level from step 2 into a coherent side-view space using the physical interaction from step 4. Give objects distinct, recognisable forms and a visible job. Any gate, counterweight, support, cable or moving obstacle must visibly connect to what it affects. Show the obstacle preventing progress, the action changing it and the resulting new opportunity.

For example, a weight platform might descend and visibly pull a cable that raises a gate. That illustrates the required cause-and-effect clarity; it is not a selected level mechanic. Reject a pad that only changes an invisible completion flag without explaining what physically changed.

Arrange the scene so stacking, height, reach and swinging matter to the decisions. Introduce only the mechanisms needed for this representative level. Give the current objective and immediate danger clear visual priority; the UI/UX specialist works with the level designer and artist here, rather than arriving after the layout is finished. Include the agreed winning and losing outcomes in the playable level.

**What you receive:** one playable level with recognisable pieces, visible mechanical relationships, clear destinations and a real risk/reward decision.

**What you judge:** “Can I work out what is happening and why my action helps, without someone explaining the board to me?”

## 6. Refine the interface and turn the first play into a tutorial

Develop the small interface already used in steps 4–5 into a consistent player experience. Keep the scene visually dominant. Show the immediate objective, relevant resources and available action; bring forward detail when the player selects an object, faces danger or makes a purchase. Remove repeated instructions and panels that compete with the action.

Use familiar words and highlight the actual object or destination being discussed. Teach one action at a time through doing it, seeing its effect and confirming success. Let players revisit help. A tutorial should not require someone to understand the game's invented vocabulary before they can begin. If a practice section protects the player from loss, make that boundary explicit and ensure the normal level's stakes are real.

Carry the same language and visual identity through start, pause, inspection, success, failure and retry. Make essential guidance available with the supported controls, not only by hovering. Check text size, contrast and screen scaling in the actual game. Support feedback with restrained sound and music suited to the chosen world.

**What you receive:** an interactive introduction and a consistent, uncluttered interface across the representative level and its outcomes.

**What you judge:** “Can I start without a manual, understand a mistake and know what to do next?”

## 7. Make rewards and upgrades change what we can accomplish

Connect salvage and level rewards to the larger purpose from step 1. Show what the player earned, what it contributes toward and why a purchase matters. Develop a small set of contrasting upgrades that enable different actions or solutions and can be tried in the representative level. Show their effect on the rig or its behavior wherever possible.

Do not use the rejected radius-only improvement as the model for meaningful progression. Establish the basic equipment, an intermediate setup and the intended end-state payoff before expanding the catalogue. Decide what lasts between attempts, how loss interacts with ownership and what makes another playthrough worthwhile. Avoid adding currencies, tiers or loot solely to fill a screen.

**What you receive:** a playable earn → choose → upgrade → use loop, with visible progress toward the end goal and understandable consequences for losses.

**What you judge:** “Do I want this reward, and can I use it to accomplish something I could not do before?”

## 8. Review a complete concept demo before full development

Combine the accepted pieces into a short, representative demo: an approachable start, a meaningful recovery challenge, an optional risk, actual success and failure, a reward choice and a chance to use the resulting upgrade. Include representative mid-game difficulty; do not make the whole demo an easy lesson with no stakes. Communicate the larger goal and how this recovery contributes to it.

Use the selected graphics, sounds and interface together. This demo is where we check whether the whole experience makes sense and feels rewarding. Fix issues across mechanics, art and guidance rather than assuming a confusing result only needs another tutorial paragraph. The exact demo length and number of scenes are set in its goal.

**What you receive:** a packaged concept demo you can play without me narrating the solution. Independent QA checks technical behavior; you judge clarity, feel, challenge and appeal.

**What you judge:** “Is this the game I want us to finish? Does the whole experience work, and do I want to play again?”

We revise this demo until you are satisfied before expanding into the full game. Passing automated tests alone does not satisfy this step.

## 9. Build out the approved game and deliver its ending

After the concept demo is accepted and you set further goals, create the remaining levels and progression around its proven visual and interaction language. Add variation that changes decisions while keeping objects and rules recognisable. Let the final challenge use what the player has learned and deliver the visible outcome promised by the premise.

Polish pacing, sound, music, controls and presentation as the game grows. Check ordinary play, risky play, failure/retry, upgrades, save/resume, input focus, display sizes and performance in the actual packaged build. Test for unwinnable states, ambiguous instructions and rewards that can be duplicated by restarting. Preserve existing owner saves; any redesigned save behavior needs an explicit migration/preservation plan.

**What you receive:** the complete redesigned playable game, built through your subsequent goals and revisions, with honest verification and any remaining limitations stated.

**What you judge:** “Does the game stay understandable and rewarding from the first pull to the ending?”

## Specialist responsibilities

The game designer connects purpose, choices, stakes and progression. The UI/UX designer works from the start on comprehension, interaction flow and visual hierarchy. The art director creates and carries the selected visual language into usable assets. The engineer implements and verifies the physics and game behavior; the audio designer connects sound to action and tone. Gameplay critique challenges clarity, fairness, reward and replay appeal. The producer coordinates the current goal, and QA independently checks the resulting build. Use these roles only where the authorized goal benefits from them.

**Next action: choose or revise the five premise options for step 1.** The remaining sequence can still be revised. No art generation, mechanics implementation or new level production is authorized by the premise goal. Store preparation, publication, marketing and purchases remain outside the current scope.
