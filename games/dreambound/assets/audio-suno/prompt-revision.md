# Suno prompt revision — proposed audition

September 10, 2026. Klaus says the sound prompting needs reconsideration after playing the results. This is a prompt audit, not a listening review. His specific objection is still pending; the worker has not heard the outputs.

The revised **Release and Charge source** prompts below have now each been generated once in Sounds / One-Shot, with blank BPM and Any key, in the verified existing Chrome account. The four audition alternatives have enabled Play buttons: Release 0:10 and 0:08; Charge 0:13 and 0:10. Exact IDs, links and generation evidence are in [revision2-generation-report.json](revision2-generation-report.json). The Chrome tab is retained for listening. Heavy impact and Guard remain proposals. No unlock, download, purchase, share, worker playback or runtime change occurred during that generation task. Subsequently, Klaus supplied the first revised motor MP3 and the original full-release MP3 for a bounded editing experiment. Two edited WAVs are now ready in `reverie-suno-v1`; see [INTEGRATION.md](INTEGRATION.md). No subjective acceptance is claimed, and the other candidates remain unevaluated.

## What the first prompts got wrong

The four prompts in `generation-report.json` are about 400 characters each and try to direct the source, fictional materials, several sound layers, emotional intensity, mix, timing and exclusions at once. That is too much competing direction for this audition. We should establish recognizable source actions before combining them into the game's finished cues.

| Existing batch | Prompt problem | Revision |
|---|---|---|
| Full release | Six plates, a crack, pneumatic thrust, crystalline splinters, sub bass and a one-second decay describe an entire composite effect. “Huge” and “tactile” give little concrete acoustic direction. | Request one familiar mechanical firing action. Add any crystal accent later only if it helps. |
| Charge motor | Magnetic hum, bronze gears, glass resonance and flutter compete for attention. The prompt tries to describe both a machine and an abstract energy bed. | Start with a motor under load; the game can supply the rising pitch. |
| Heavy impact | Metal, ceramic, stone, crystal grit and bending resonance overlap strongly with the release and guard descriptions. | Make the impact's defining quality a blunt collision with stone. |
| Guard/parry | Strike, ricochet, magical resonance and scattering sparks blur the central contact. It repeats the other batches' bronze/ceramic/glass vocabulary. | Make the defining quality a ringing defensive clang. |

Remove the long negative noun lists from this test. They add clutter without establishing the desired action. This is not a claim that Suno ignores or reverses negation; that has not been tested. Likewise, the UI lengths of 3–11 seconds for the original One-Shot outputs do not establish what they sound like. They do show that our subsecond/one-second instructions did not determine the total output length.

Suno's current guide recommends concise, specific language and familiar sound words. It documents two alternatives per generation, One Shot and Loop types, plus optional BPM/key controls. It recommends One Shot for effects and cautions that loops can include music. It suggests duration wording as an iteration technique, without guaranteeing exact timing. [Official Suno Sounds guide, edited September 9, 2026](https://help.suno.com/en/articles/10625537).

## Four distinct candidate prompts

Use **Sounds → One-Shot**, leave BPM blank and key unspecified/Any. Start with these as a small audition batch; do not regenerate all gameplay cues. Even the motor is initially a source recording to inspect and edit, rather than an assumed finished loop.

| Candidate | Exact prompt | Intended distinction |
|---|---|---|
| Release | `A spring-loaded metal launcher firing once, with a sharp latch snap and forceful rush of air. Dry, close recording.` | Mechanical release and outgoing motion. |
| Heavy impact | `A heavy metal plate striking a stone block once. Dense, blunt impact with a short decay. Close, dry recording.` | Weight and contact, without a prominent ringing tail. |
| Guard | `Steel striking a thick bronze shield once. A sharp resonant clang that rings out and fades. Close recording.` | Protective contact with a clear metallic ring. |
| Charge source | `An electric servo motor spinning steadily under load. A low mechanical whirr, close and dry.` | Sustained machinery, distinct from the three transients. |

These intentionally probe physical sound foundations. They do not settle whether Klaus wants the finished shield to sound realistic, magical or more exaggerated. His specific reaction should steer the next change. If a candidate is wrong, change one defining phrase or source at a time; do not restore the full adjective stack.

## Accept or reject on the audio

- **Recognition:** a listener should hear the requested action without needing its title. Compare the alternatives at a consistent listening level so louder is not mistaken for better.
- **Distinction:** release should convey outward motion, heavy impact a blunt hit, guard a ringing contact, and charge a sustained mechanism. Reject candidates that collapse into the same generic blast or musical accent.
- **Editable one-shots:** require at least one clean, complete attack and its decay that can be isolated. Extra leading/trailing silence can be trimmed. A longer file is acceptable if the useful event is intact; reject a swell, repeated sequence or background layer that cannot be separated from it. Do not claim Suno will generate the final 0.5–1.2 second gameplay length.
- **Editable motor:** require at least 2.25 seconds of reasonably steady material for a 2 second loop with a 250 ms overlap. Check that looping and pitch changes preserve a machine-like sound. An ordinary sample seam must be edited and checked; the generation mode alone proves nothing about it.
- **Unwanted content:** reject audible speech, singing, recognizable melody, backing beats, or a sustained piercing tone that dominates the useful action. These are audition criteria, not claims about the first results.
- **Integration:** after a candidate passes listening, obtain its permitted official download, record its source region and processing, and check onset, clipping, tails and overlaps at actual runtime gains. Preserve the existing event gates and voice limits. If listening is unavailable, label only the measured signal checks as verified and leave subjective acceptance open.

Return the chosen IDs and one concrete reason for each selection before expanding the palette. No current output is declared accepted by this document.
