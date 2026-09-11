# Stable Audio 3 public-demo audition

**Unaccepted private audition. Nothing here is approved for runtime import or release.** Three anonymous requests to Stability AI's [official public demo](https://huggingface.co/spaces/stabilityai/stable-audio-3) succeeded on 2026-09-11. No login, credentials, purchase, paid API, gated-weight access, contact-sharing acceptance or Suno operation was used. The three-generation limit is exhausted; no additional requests were made.

| Raw server file | Seconds | Seed | Intended source event |
| --- | ---: | ---: | --- |
| internal_ignition-raw.wav | 3 | 46711 | Low flame ignition in a confined cavity, turbulent burning and small crackles |
| erupting_flare-raw.wav | 2 | 46712 | Pressurized flame eruption with an air punch and rough fire tail |
| fireball_whoosh-raw.wav | 2 | 46713 | One fast moving fireball pass with a turbulent air/fire tail |

All requests explicitly selected **small-sfx**, eight sampling steps, CFG 1.0 and the pingpong sampler. Each request record retains the exact prompt, duration, seed, settings, timestamps, live event identifier, terminal response and output hash. These are actual model-generated sounds, unlike the rejected local layered Foley draft.

The [model card](https://huggingface.co/stabilityai/stable-audio-3-small-sfx) identifies a 0.6B sound-effects model. The Space's public source selects that model for `small-sfx`; the client cannot independently hash the weights currently loaded on the remote GPU. Records therefore distinguish the observed model metadata revision from the unreported loaded-weight revision. Observed Space revision: `a6d4a3300fb7d96b2f28ac409d868cba65eb6093`; model metadata revision: `ae12755283df9d62ca39a9b050a39a0b607b8c20`.

## API and reproduction evidence

The Space reported `private=false`, `gated=false`, runtime `RUNNING`, ZeroGPU `zero-a10g`. Its [public Gradio API information](https://stabilityai-stable-audio-3.hf.space/gradio_api/info) advertised `/infer` with public visibility and the named parameters used. `request_audition.py` uses that documented API: POST named JSON to `/gradio_api/call/v2/infer`, then read the returned event through `/gradio_api/call/infer/{event_id}`. It uses a fresh HTTP session with environment/netrc credential loading disabled.

The script refuses to repeat a recorded generation. `--resume` reconnects only to an already-known unfinished event; it never submits another generation. The three successful request records provide exact reproducible settings, but those requests must not be repeated without new authorization. Sampling results may vary across server/library/weight changes despite a fixed seed.

## Files and verification

The three WAVs retain the exact downloaded response bytes. There is **no local pitch change, EQ, layering, normalization, compression or conversion**. The demo's [published implementation](https://huggingface.co/spaces/stabilityai/stable-audio-3/blob/main/app.py) normalizes generated output peaks and saves PCM16. The received files are stereo PCM16 at 44.1 kHz, with one maximum-magnitude sample each; that is consistent with the server's documented peak normalization. No clipping or sound-quality judgment follows from that observation alone.

ffprobe verified the format/duration and ffmpeg decoded every file without error. Hashes and measured levels are in `audition-summary.json`. These measurements confirm usable files, not organic character, prompt adherence, lack of unwanted sounds or suitability in the game.

**Agent listening remains unavailable.** The prior audio tool attempt reported unsupported audio input. None of these three files has been heard by this worker, and there is no owner acceptance yet. The owner must hear them before an integration decision. The raw WAVs are ignored by Git so a broad asset commit cannot accidentally publish unaccepted material.

## License evidence and scope

The model repository's [LICENSE.md](https://huggingface.co/stabilityai/stable-audio-3-small-sfx/blob/main/LICENSE.md) was publicly readable without accepting the model gate. Its exact text is retained as `model-license-evidence.txt`, SHA-256 `d6f6b1a4dce5c852bd6d7d9482d002baf0ccdb71e662250b73be9eec8764ee8d`. It is the Stability AI Community License dated July 5, 2024, rather than CC0.

The license permits evaluation/testing, addresses model-use revenue limits and commercial registration, contains an integrated-end-user exception, and separates output ownership from model/derivative distribution. The current [Stability licensing page](https://stability.ai/license) also discusses output ownership and Community eligibility. This worker has not registered the owner or decided the commercial terms of consuming this hosted demo's outputs. **Commercial release clearance remains false.** Root should resolve the applicable output terms if a candidate is worth using; a model's availability below a revenue threshold alone is not an asset-release clearance.

No model weights or software are shipped in this folder. The source request script, prompts and audit records can be reviewed independently of the private audio files. The rejected CC0-based palette, its acceptance record, the game importer/runtime and shared records remain unchanged by this audition.
