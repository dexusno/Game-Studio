"""Fit Klaus's fireball.wav to the existing private playable sound candidate."""
from pathlib import Path
import hashlib
import json
import shutil
import wave
import numpy as np

GAME = Path(__file__).resolve().parents[1]
STUDIO = GAME.parents[1]
SOURCE = GAME / 'assets/audio/fireball.wav'
PREVIOUS = STUDIO / '.local/organic-fire-review/audio-preview-v1'
OUTPUT = STUDIO / '.local/organic-fire-review/audio-owner-fireball-v1'


def sha(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def main():
    original_hash = sha(SOURCE)
    with wave.open(str(SOURCE), 'rb') as source:
        rate, channels = source.getframerate(), source.getnchannels()
        if source.getsampwidth() != 2 or rate != 48000 or channels != 2:
            raise ValueError('Expected the supplied stereo PCM16 48kHz source.')
        samples = np.frombuffer(source.readframes(source.getnframes()), '<i2')
    mono = samples.reshape(-1, channels).mean(axis=1) / 32768.0
    gain = min(1.0, 10 ** (-5 / 20) / max(abs(mono)))
    mono *= gain
    fade = min(round(.003 * rate), len(mono) // 2)
    mono[:fade] *= np.linspace(0, 1, fade)
    mono[-fade:] *= np.linspace(1, 0, fade)
    pcm = np.round(mono * 32767).astype('<i2').tobytes()
    previous = json.loads((PREVIOUS / 'audition-report.json').read_text(encoding='utf-8'))
    OUTPUT.mkdir(parents=True, exist_ok=True)
    files = []
    for entry in previous['files']:
        name = entry['name']
        path = OUTPUT / (name + '.wav')
        if name.startswith('S_CasterRelease'):
            with wave.open(str(path), 'wb') as sound:
                sound.setnchannels(1); sound.setsampwidth(2); sound.setframerate(rate)
                sound.writeframes(pcm)
            files.append(dict(name=name, path=path.name, sha256=sha(path), looping=False,
                duration_seconds=len(mono)/rate, source='assets/audio/fireball.wav',
                source_sha256=original_hash, alias_of=None if name.endswith('A') else 'S_CasterReleaseA'))
        else:
            prior = PREVIOUS / (name + '.wav')
            if sha(prior) != entry['sha256']:
                raise ValueError('Previous candidate changed: ' + name)
            shutil.copyfile(prior, path)
            files.append(entry)
    report = dict(complete=True, acceptance='private-unaccepted-audition',
        source='Klaus supplied ElevenLabs fireball.wav on 2026-09-11',
        owner_prompt='the sound of a fireball shot from a monster in a AAA game',
        source_sha256=original_hash, source_count=4, files=files,
        source_license='Owner-reported ElevenLabs Free plan; private noncommercial playtest only.',
        processing=dict(duration_seconds=len(mono)/rate, trim_seconds=0,
            stereo_to_mono='0.5 left + 0.5 right', constant_gain_db=float(20*np.log10(gain)),
            endpoint_fade_seconds=.003, pitch_changed=False),
        note='Release A/B/C use the same owner clip. Five remaining cues preserve the preceding Stable Audio3 candidate byte-for-byte. No generated variations or new AI requests.',
        agent_listened=False, owner_accepted_integrated_mix=False)
    (OUTPUT / 'audition-report.json').write_text(json.dumps(report, indent=2)+'\n', encoding='utf-8')
    if sha(SOURCE) != original_hash:
        raise ValueError('Owner original changed during preparation.')
    print(json.dumps(dict(output=str(OUTPUT), source_sha256=original_hash,
        release_duration_seconds=len(mono)/rate, files=len(files))))


if __name__ == '__main__':
    main()
