# Organic fire palette

**Rejected by the owner after listening: “Still sounds artificial.”** The reviewed preview SHA-256 is `e5b268c190eb10fe9bb22c477d5b7914b78073fbec207b85d7d0566c46639129`. These eight cues are preserved as a rejected baseline and must not be automatically imported as accepted audio. The report records `acceptance.status = rejected_by_owner`. [Diagnosis and separate minimal-edit comparison](candidate-natural/REVIEW.md) follow. The integration specifications below describe that rejected candidate; they are not an acceptance decision.

Eight mono PCM16/48 kHz cues replace the thin caster's metallic/force-field tell and release. The materials are edited real breath, match ignition and fire-staff sound sources. No pitched oscillator, bell, metal hit, notification sample or musical component is added. Those source choices establish the construction, not a subjective listening verdict.

`organic-fire-preview.wav` is a 12.2-second offline audition: two complete gathers/volleys, then an interrupted gather and silence. It uses the agreed 1.35-second gather, first throw at attack time 0.03 and second at 0.47. The exact event timeline and file identities are in `audio-report.json`. The preview has no room, distance attenuation, shield audio or game mastering. Do not import it as a gameplay cue.

## Engine contract

Import only `S_Caster*.wav` under `/Game/Audio/OrganicFire`. Use the Effects sound class/group, with the existing master/SFX gain and mute behavior. Set loop flags only on `S_CasterFurnaceLoop` and `S_CasterFlightLoop`. Use short, decoded, spatialized sound waves; all are mono. Import gain and pitch remain 1. Runtime event gain below is applied once. Reimport must preserve/restore the looping and concurrency properties explicitly.

| Cue | Seconds | Runtime gain | Global / owner cap | Priority | Trigger and stop |
| --- | ---: | ---: | ---: | ---: | --- |
| S_CasterIgnite | 0.64 | 0.70 | 3 / 1 | 78 | Once on entering caster gather. Attach to the chest; retain a component so an early interruption can fade it out. |
| S_CasterFurnaceLoop | 1.60 loop | 0.68 × charge envelope | 3 / 1 | 80 | Chest-attached throughout gather and the brief second-shot regather. Start with 120 ms fade-in; never restart each tick. |
| S_CasterReleaseA/B/C | 1.12 / 1.08 / 1.20 | 0.82 | 4 / 2, shared by all variants | 88 | Once per successfully spawned fireball at its actual emitting location. Alternate variants with no immediate repeat. Use pitch 0.97–1.03 at most. |
| S_CasterFlightLoop | 1.10 loop | 0.52 | 4 / 1 per projectile | 45 | Attach to the fireball, with 50 ms fade-in. Its position supplies the movement. Stop at resolved collision, projectile destruction, invalidation, room teardown, or lifespan expiry. |
| S_CasterImpactA/B | 1.16 / 1.24 | 0.80 | 4 / 1, shared by both variants | 85 | One contact burst at the resolved collision position. Alternate variants. Ordinary harmless expiry can be silent; never emit a full impact on every teardown path. |

Use one shared concurrency group for all release variants and one for both impact variants, otherwise the caps multiply by the number of files. Prefer stopping the farthest/lowest-priority flight voice before a cast tell or contact burst. Apply the owner cap through an owner-limited group or explicit per-owner component management. A caster's two release tails may overlap; it must not keep a second copy of its charge loop.

For a normalized first-gather progress `u`, use a smooth gain envelope from 0.22 at the end of the 120 ms onset to 0.45 midway and 1.0 near commitment. Hold the loop through the first throw, briefly drop it to 0.22 after release, then grow to 1.0 for the second throw 0.44 seconds later. Fade out over 35 ms after the second successful spawn. No fixed WAV crescendo decides when a fireball exists.

Fade Ignite and FurnaceLoop out over 60 ms on stun, death, lost target, cancelled attack, encounter teardown, or another exit from the relevant casting state. A stopped charge must stay stopped until a genuinely new gather begins. If simulation pauses, pause these components rather than replaying or releasing them. Mute must also silence attached/looping components, and unmute must not trigger missed release events. Destroy owned components on EndPlay. Clear pointers after completion. Flight should stop immediately or within 35 ms at impact so its tail cannot outlive the projectile indefinitely.

Suggested starting attenuation, in Unreal centimetres: Ignite/Furnace/Release have a 300 cm inner radius and 1900 cm falloff; Impact has 250 cm + 1600 cm; Flight has 100 cm + 850 cm. Use the project's established natural-distance attenuation shape, spatialization and conservative occlusion. At the caster's usual 10–11 m range, verify the held charge still reads behind the shield and water ambience. These attenuation distances are integration proposals, not observed mix settings.

Remove the old `S_EnemyTell` and `S_EnemyFire` playback for this caster branch. Preserve unrelated monsters/player cues. Root owns that runtime change, the separate importer and the shared manifest. The new release is naturally more broadband; raising all enemy sounds would not fix the old source choice.

## Evidence and remaining review

- Script: `python scripts/prepare_organic_fire_audio.py`; deterministic check: add `--check`. ffmpeg must be on PATH. Rendering reads only the retained sources; it does not download assets or alter Unreal content, shared configuration or the primary manifest.
- Eight source hashes verified before rendering. All eight final cues are mono PCM16/48 kHz; no full-scale samples. Actual measured true peaks range from −16.95 to −8.09 dBFS. One-shots begin/end at digital zero.
- Furnace/Flight loop boundary jumps are respectively −64.29 and −55.50 dBFS, smaller than each loop's 99th-percentile interior sample difference. Their quietest 20 ms RMS windows are −29.16 and −34.45 dBFS: there is no fade-to-silence at the loop seam. This is a sample-continuity check, not proof that a loop cannot be recognized by ear.
- The uncompressed preview peaks at −5.12 dBFS. Its cancelled charge is exactly silent from 10.67 seconds onward. A selected overlap stress mix of two furnace voices, two different releases, four aligned flight voices and one impact peaks at −2.51 dBFS. This does not cover all possible phase offsets or the player/environment mix.
- Spectrograms of all eight delivered cues and the preview were inspected in `technical-spectrum-review.jpg`: each uses broadband, changing energy; the preview includes clear silence after cancellation. This is visual signal inspection and does not establish absence of perceived harshness or a convincing monster voice.
- **Listening remains unverified.** Attempting audio emission in this session returned: “audio content omitted because you do not support audio input.” No claim of hearing the sources, the produced sounds, loop seams or the game mix is made. A playable preview is supplied for listening in a capable environment.

Root's remaining integration checks: actual engine decode and shared concurrency assignment; first/second release synchronization; prompt charge cancellation for every exit; no persistent flight/charge after destruction or pause/reset; master/SFX mute; rendered in-game sound capture; and an actual perceptual review of organic force, harshness and balance with shield/ambient sounds. If listening reports a muddy low end, first reduce the release breath layer or raise its high-pass cutoff rather than adding a bright pitched cue.

## Provenance

[Primary rights evidence](rights/cc0-source-evidence.md) and `sources.json` cover every retained source, including download/member identities and exact source cuts. `manifest-rows.csv` contains proposed rows for the integration owner; it does not mutate `assets/manifest.csv`. Only the eight S_Caster WAVs are runtime imports. The optional source/preview credits are LEGIT Audio, mikeask and qubodup, with Game Studio audio editing. CC0 does not require attribution. The owner's Suno downloads, prior sources and prior cues are preserved.
