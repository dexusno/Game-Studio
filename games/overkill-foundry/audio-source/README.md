# Original mechanical sound palette

`build_mechanical.py` creates 23 small, original PCM cues for Overkill Foundry:
gun reports, metal impacts, breech/forge/claw movements, Shield/rig hits, robot
movement/breakup, service/result indicators, quiet controls and a factory room
loop. It uses deterministic inharmonic resonances, filtered pseudorandom noise,
frequency sweeps and short reflections. There are no sampled recordings,
third-party patches, other-game assets or external generation services.

This is a first implemented sound palette. Its mechanical character and mix
still require listening in actual gameplay and owner feedback. Generated PCM
and technical measurements do not establish satisfying sound design.

## Reproduce

Run from the repository root with Python 3, standard library only:

```powershell
python games/overkill-foundry/audio-source/build_mechanical.py
python games/overkill-foundry/audio-source/build_mechanical.py --check
```

The small 48 kHz / mono / 16-bit sources and compact measurements are in
`assets/audio/mechanical-v001`. `--check` regenerates every sample in memory,
compares WAV bytes and the audit, and validates headroom, non-silence, DC and
endpoints. The six-second room loop uses periodic components; its seam jump is
checked against ordinary adjacent sample changes. No whole-game loudness or
hardware playback claim follows from these source checks.

Execute `unreal/Tools/import_audio.py` through Unreal's Python commandlet to
import sources into ignored `/Game/FoundryAudio` SoundWave derivatives. The
importer checks audited source hashes and duration, sets the explicit loop
property and records `Saved/AudioImport/audio-import.json`. Packaging must cook
this directory because the adapter loads named assets dynamically.

## Runtime contract

`FFoundryAudio` consumes committed events once, alongside the visual adapter.
It never edits state, calculates rewards/damage, reads future intents or draws
from any core RNG stream. The 18-damage light/heavy split is only an initial
sound-presentation threshold. Tiny event-ID pitch variation is cosmetic.
One strongest impact and one breakup per committed event batch avoid an
unreadable burst of pellet/robot sounds. Fire and action voices have separate
limits and brief release tails; these are audio limits, never gameplay limits.

Original WAV levels already reserve headroom. Runtime starts at 0.55 master
gain, with additional cue trims. The room sits below useful action transients.
`SetVolume(0)` immediately mutes current sounds without later replaying them.
`StopAll` clears voices on restart or leaving the game. No speech, licensed
music or stereo position is needed for essential game information; visuals
remain authoritative. Precision good/miss cues are prepared for the later
actual timing interaction, not evidence that it exists.

An isolated `-FoundryAudioProbe` launch without `-nosound` plays every cue and
records the game's own master submix through Unreal's AudioMixer API. It writes
`Saved/AudioCapture/foundry-audio-<timestamp>.wav` and exits after export time.
This captures only this game's mix, not system audio or another application.
The diagnostic temporarily sets `au.NeverDisableSubmixes=1` and restores its
previous value on exit: Unreal otherwise omits silent buffers from a recording,
shortening the captured clock. Ordinary game rendering keeps its normal setting.
The resulting PCM must be checked for actual signal, clipping and cue coverage;
constructing a sound component is not sufficient playback evidence. Normal
event dispatch and mute/reset still need an integrated gameplay check.

On 20 September the actual UE5.8.2 import passed all 23 source identities and
durations. `AudioProbe-20260920-162046.log` exported 32.704 seconds of stereo
48kHz PCM. The independent `verify_mix.py` check found signal in all 23 logged
cue windows, peak -23.308dBFS and zero clipped samples. The module hash was
unchanged before/after the probe. Compact identities and results are in
`assets/audio/mechanical-v001/unreal-mix-evidence.json`; the raw mix remains
ignored in `unreal/Saved/AudioCapture`. This sequential cue diagnostic does not
establish simultaneous-combat mix quality, physical speakers or human taste.

## Origin and rights

Created 20 September 2026 by Game Studio / Codex, with original Python synthesis
source in this directory. The asset manifest records each cue and the generator.
No third-party sound or recording credit is required by these inputs. The
studio's distribution licence remains unset, as for its other original assets.
