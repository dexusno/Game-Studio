/* Sixfold Recoil: original procedural sound, authored for this game with Codex.
 * No samples, external music, downloads, or runtime dependencies.
 *
 * Integration: unlock() ONLY in a pointer/key/gamepad activation handler; call
 * setScene once per frame and event() for each drained core event. Volumes are
 * 0..1 (defaults music .30, sfx .75); danger is 0..1. Result scenes may set
 * active:false, paused:false so win/lose can finish. Pause/hidden kills all
 * voices, with an 8 ms de-click fade. Stale events are never queued.
 *
 * Palette: launch = low dry clack; collect.held = D E F# A B D tuned catches;
 * block = bright metal; hull = low double knock; warning.kind:'charge' =
 * rising coil (duration/tellDuration); other warning = quiet entry doublet;
 * shoot = short descending zap. Harmless/cutting return hits share the same
 * restrained impact family. Music is an original eight-step electrical pulse,
 * not an imported song. Recall hum follows recalling, on the effects slider.
 *
 * Limits: 32 source nodes / 18 grouped voices including ambience; each event
 * also has a cap and cooldown below. Important danger cues steal older, lower
 * priority voices; quiet impacts are dropped first. No reverberation or sample
 * loops. Oscillator hum fades at either end, so there is no file loop seam.
 * Mix: sfx bus .85, music bus .60, output .85, before user levels. A compressor
 * plus bounded safety curve retains headroom even under event floods.
 */
(function (root, factory) {
  const api = factory(root);
  if (typeof module === 'object' && module.exports) module.exports = api;
  root.SixfoldAudio = api;
})(typeof globalThis !== 'undefined' ? globalThis : this, function (root) {
  'use strict';

  const CATCHES = [293.6648, 329.6276, 369.9944, 440, 493.8833, 587.3295];
  // [priority, concurrent groups, retrigger interval in seconds]
  const RULES = {
    launch: [4, 2, .045], empty: [1, 1, .13], rejected: [1, 1, .15],
    land: [0, 3, .025], collect: [3, 6, 0], recallStart: [2, 1, .18],
    block: [5, 2, .04], hull: [6, 1, .2], enemyHit: [1, 3, .025],
    enemyDie: [2, 2, .05], shoot: [4, 3, .025],
    warning: [2, 2, .10], charge: [4, 3, .035],
    stageClear: [5, 1, .5], win: [7, 1, .5], lose: [7, 1, .5],
    recallBed: [0, 1, 0], music: [0, 4, 0]
  };
  const MAX_SOURCES = 32;
  const MAX_VOICES = 18;
  const finite = (n, fallback) => typeof n === 'number' && Number.isFinite(n) ? n : fallback;
  const clamp = (n, lo, hi) => Math.max(lo, Math.min(hi, n));
  const tone = (f, to, a, d, extra) => Object.assign({ f, to, a, d, wave: 'sine' }, extra);
  const noise = (f, a, d, extra) => Object.assign({ f, a, d, wave: 'noise' }, extra);

  class Sound {
    constructor() {
      this.ctx = null;
      this.volumes = { music: .30, sfx: .75 };
      this.scene = { active: false, paused: false, recalling: false, danger: 0 };
      this.voices = [];
      this.disposed = false;
      this._seed = 0x61f01d;
      this._last = Object.create(null);
      this._targets = new WeakMap();
      this._timer = null;
      this._nextBeat = 0;
      this._beat = 0;
      this._nextCatch = 0;
      this._doc = root.document;
      this._visibility = () => this._safeSync();
      if (this._doc && this._doc.addEventListener) {
        this._doc.addEventListener('visibilitychange', this._visibility);
      }
    }

    async unlock() {
      if (this.disposed) return false;
      // The browser still enforces autoplay. This guard also prevents accidental
      // creation by a timer on browsers which expose transient user activation.
      const activation = root.navigator && root.navigator.userActivation;
      if (activation && !activation.isActive && (!this.ctx || this.ctx.state !== 'running')) return false;
      try {
        if (!this.ctx || !this._graphReady || this.ctx.state === 'closed') {
          this._releaseContext();
          const Context = root.AudioContext || root.webkitAudioContext;
          if (typeof Context !== 'function') return false;
          this.ctx = new Context({ latencyHint: 'interactive' });
          this._buildGraph();
        }
        if (this.ctx.state !== 'running') await this.ctx.resume();
        if (this.disposed || !this.ctx) return false;
        this._sync();
        return this.ctx.state === 'running';
      } catch (_) {
        // Resume rejection is recoverable on a later real gesture. A failed
        // graph is disposed, so an unsupported audio device cannot break play.
        if (!this._graphReady) this._releaseContext();
        return false;
      }
    }

    setVolumes(values) {
      if (!values || this.disposed) return;
      for (const bus of ['music', 'sfx']) {
        this.volumes[bus] = clamp(finite(values[bus], this.volumes[bus]), 0, 1);
      }
      this._safeSync();
    }

    setScene(values) {
      if (!values || this.disposed) return;
      for (const key of ['active', 'paused', 'recalling']) {
        if (typeof values[key] === 'boolean') this.scene[key] = values[key];
      }
      this.scene.danger = clamp(finite(values.danger, this.scene.danger), 0, 1);
      this._safeSync();
    }

    event(event) {
      if (!event || typeof event.type !== 'string' || this.disposed) return false;
      const terminal = event.type === 'win' || event.type === 'lose';
      if (!this._audible() || !this.volumes.sfx || (!this.scene.active && !terminal)) return false;
      let key = event.type;
      if (key === 'warning' && event.kind === 'charge') key = 'charge';
      const rule = RULES[key];
      if (!rule || key === 'music' || key === 'recallBed' || key === 'charge' && event.type !== 'warning') return false;
      const now = this.ctx.currentTime;
      if (now - (this._last[key] ?? -Infinity) < rule[2]) return false;
      try {
        this._prune();
        if (terminal) this._stopWhere(v => !v.terminal);
        if (key === 'shoot' && event.enemyId != null) {
          this._stopWhere(v => v.key === 'charge' && v.enemyId === event.enemyId);
        }
        let when = now + .003;
        if (key === 'collect') {
          // Six same-frame catches keep their pitches with at most 90 ms spread.
          when = Math.max(when, Math.min(this._nextCatch, now + .093));
          this._nextCatch = when + .018;
        }
        const specs = this._palette(key, event);
        const voice = this._voice(key, specs, 'sfx', when);
        if (!voice) return false;
        voice.terminal = terminal;
        voice.enemyId = event.enemyId;
        this._last[key] = now;
        return true;
      } catch (_) {
        this._releaseContext();
        return false;
      }
    }

    dispose() {
      if (this.disposed) return;
      this.disposed = true;
      if (this._doc && this._doc.removeEventListener) {
        this._doc.removeEventListener('visibilitychange', this._visibility);
      }
      this._releaseContext();
    }

    _random() {
      let x = this._seed;
      x ^= x << 13; x ^= x >>> 17; x ^= x << 5;
      this._seed = x >>> 0;
      return this._seed / 4294967296;
    }

    _buildGraph() {
      const c = this.ctx;
      this._output = c.createGain();
      this._output.gain.value = 0;
      this._sfx = c.createGain();
      this._music = c.createGain();
      this._sfx.gain.value = 0;
      this._music.gain.value = 0;
      this._compressor = c.createDynamicsCompressor();
      for (const [key, value] of Object.entries({ threshold: -12, knee: 9, ratio: 6, attack: .003, release: .16 })) {
        this._compressor[key].value = value;
      }
      this._limiter = c.createWaveShaper();
      const curve = new Float32Array(2049);
      for (let i = 0; i < curve.length; i++) {
        const x = i * 2 / (curve.length - 1) - 1;
        const a = Math.abs(x);
        curve[i] = Math.sign(x) * (a <= .60 ? a : .60 + .20 * Math.tanh((a - .60) / .20));
      }
      this._limiter.curve = curve;
      this._limiter.oversample = 'none';
      this._sfx.connect(this._compressor);
      this._music.connect(this._compressor);
      this._compressor.connect(this._limiter);
      this._limiter.connect(this._output);
      this._output.connect(c.destination);
      this._noise = c.createBuffer(1, Math.ceil(c.sampleRate * .65), c.sampleRate);
      const data = this._noise.getChannelData(0);
      let mean = 0;
      for (let i = 0; i < data.length; i++) { data[i] = this._random() * 2 - 1; mean += data[i]; }
      mean /= data.length;
      for (let i = 0; i < data.length; i++) data[i] -= mean;
      c.onstatechange = () => this._safeSync();
      this._nextBeat = c.currentTime + .04;
      this._targets = new WeakMap();
      this._graphReady = true;
    }

    _audible() {
      return !this.disposed && this.ctx && this._graphReady && this.ctx.state === 'running' &&
        !this.scene.paused && !(this._doc && (this._doc.hidden || this._doc.visibilityState === 'hidden'));
    }

    _ramp(param, value, duration = .025) {
      if (this._targets.get(param) === value) return;
      const now = this.ctx.currentTime;
      const held = param.value;
      if (typeof param.cancelAndHoldAtTime === 'function') param.cancelAndHoldAtTime(now);
      else param.cancelScheduledValues(now);
      // Anchor a new ramp even when there were no future automation events;
      // otherwise it can start at the old ramp's endpoint and jump immediately.
      param.setValueAtTime(held, now);
      param.linearRampToValueAtTime(value, now + duration);
      this._targets.set(param, value);
    }

    _safeSync() {
      if (!this.ctx) return;
      try { this._sync(); } catch (_) { this._releaseContext(); }
    }

    _sync() {
      this._prune();
      if (!this._audible()) {
        this._ramp(this._output.gain, 0, .008);
        this._stopWhere(() => true);
        this._stopTimer();
        this._last = Object.create(null);
        this._nextCatch = 0;
        return;
      }
      this._ramp(this._output.gain, .85, .012);
      this._ramp(this._sfx.gain, .85 * this.volumes.sfx);
      this._ramp(this._music.gain, .60 * this.volumes.music);
      if (!this.scene.active) this._stopWhere(v => !v.terminal);
      if (!this.volumes.sfx) this._stopWhere(v => v.bus === 'sfx');
      if (!this.scene.active || !this.volumes.music) {
        this._stopWhere(v => v.bus === 'music');
        this._stopTimer();
      } else if (this._timer == null) {
        this._nextBeat = this.ctx.currentTime + .04;
        this._tick();
        this._timer = root.setInterval(() => {
          try { this._tick(); } catch (_) { this._releaseContext(); }
        }, 100);
        if (this._timer && typeof this._timer.unref === 'function') this._timer.unref();
      }
      const recall = this.scene.active && this.scene.recalling && this.volumes.sfx > 0;
      if (!recall) this._stopWhere(v => v.key === 'recallBed');
      else if (!this.voices.some(v => v.key === 'recallBed' && !v.stopping)) {
        this._voice('recallBed', [
          tone(146.8324, null, .022, Infinity, { attack: .07 }),
          tone(220.4, null, .015, Infinity, { attack: .09 })
        ], 'sfx', this.ctx.currentTime + .003);
      }
    }

    _tick() {
      if (!this._audible() || !this.scene.active || !this.volumes.music) return;
      this._prune();
      const now = this.ctx.currentTime;
      // A stalled frame starts from now rather than replaying missed beats.
      if (this._nextBeat < now) this._nextBeat = now + .025;
      while (this._nextBeat < now + .15) {
        const step = this._beat++ % 8;
        const roots = [73.4162, 0, 146.8324, 110, 73.4162, 0, 110, 146.8324];
        const specs = [tone(1760, 1500, .011 + .006 * this.scene.danger, .027)];
        if (roots[step]) specs.push(tone(roots[step], null, step % 4 === 0 ? .18 : .065, .24, { attack: .015 }));
        this._voice('music', specs, 'music', this._nextBeat);
        this._nextBeat += .30;
      }
    }

    _stopTimer() {
      if (this._timer != null) root.clearInterval(this._timer);
      this._timer = null;
    }

    _palette(key, e) {
      const variation = .985 + this._random() * .03;
      switch (key) {
        case 'launch': return [tone(210 * variation, 66, .34, .16, { wave: 'triangle', cutoff: 2100 }), noise(1800, .23, .065)];
        case 'empty': return [tone(134, 95, .115, .09), noise(750, .045, .018)];
        case 'rejected': return [tone(230, 215, .075, .038)];
        case 'land': return [noise(1150 * variation, .08, .037)];
        case 'collect': {
          const held = clamp(Math.round(finite(e.held, finite(e.heldCount, finite(e.count, 1)))), 1, 6);
          const f = CATCHES[held - 1];
          return [tone(f, null, held === 6 ? .22 : .19, held === 6 ? .34 : .23), tone(f * 2.003, null, .045, .11)];
        }
        case 'recallStart': return [tone(146.8, 293.6, .085, .16, { wave: 'triangle', cutoff: 1200, attack: .012 })];
        case 'block': return [tone(610 * variation, 430, .27, .15, { wave: 'triangle', cutoff: 2800 }), noise(2900, .19, .07)];
        case 'hull': return [tone(102, 48, .37, .29, { wave: 'triangle', cutoff: 950 }), tone(151, 68, .19, .19, { delay: .055 }), noise(650, .15, .17)];
        case 'enemyHit': return [tone((e.returnHit ? 540 : 410) * variation, 190, .095, .075), noise(1450, .08, .045)];
        case 'enemyDie': return [tone(245 * variation, 74, .14, .19), noise(1750, .12, .13)];
        case 'shoot': return [tone(520, 165, .20, .14, { wave: 'triangle', cutoff: 2000 }), noise(2200, .15, .085)];
        case 'charge': {
          const d = clamp(finite(e.duration, finite(e.tellDuration, .65)), .18, 1.5);
          return [tone(260, 680, .14, d, { wave: 'triangle', cutoff: 1600, rise: true })];
        }
        case 'warning': return [tone(440, null, .085, .07), tone(440, null, .07, .07, { delay: .14 })];
        case 'stageClear': return [tone(293.6648, null, .13, .29), tone(440, null, .11, .33, { delay: .13 })];
        case 'win': return [293.6648, 369.9944, 440, 587.3295].map((f, i) => tone(f, null, .18, .43, { delay: i * .13, attack: .009 }));
        case 'lose': return [tone(220, 110, .18, .63, { wave: 'triangle', cutoff: 950, attack: .012 }), tone(110, 73.4162, .14, .52, { delay: .24 })];
        default: return [];
      }
    }

    _voice(key, specs, bus, when) {
      if (!specs.length) return null;
      const [priority, cap] = RULES[key];
      const same = this.voices.filter(v => v.key === key && !v.stopping);
      if (same.length >= cap) {
        if (priority < 2) return null;
        this._kill(same[0], true);
      }
      let count = this.voices.reduce((sum, v) => sum + v.sources.length, 0);
      while (this.voices.length >= MAX_VOICES || count + specs.length > MAX_SOURCES) {
        const victim = this.voices.filter(v => v.priority <= priority)
          .sort((a, b) => a.priority - b.priority || a.when - b.when)[0];
        if (!victim) return null;
        count -= victim.sources.length;
        this._kill(victim, true);
      }
      const c = this.ctx;
      const voice = { key, priority, bus, when, end: when, nodes: [], sources: [], terminal: false, stopping: false };
      voice.gain = c.createGain();
      voice.gain.gain.value = 1;
      voice.gain.connect(bus === 'music' ? this._music : this._sfx);
      voice.nodes.push(voice.gain);
      this.voices.push(voice);
      for (const spec of specs) {
        const t = when + (spec.delay || 0);
        const looping = spec.d === Infinity;
        const duration = spec.d;
        const source = spec.wave === 'noise' ? c.createBufferSource() : c.createOscillator();
        voice.sources.push(source);
        voice.nodes.push(source);
        const envelope = c.createGain();
        envelope.gain.value = 0;
        voice.nodes.push(envelope);
        if (spec.wave === 'noise') source.buffer = this._noise;
        else {
          source.type = spec.wave;
          source.frequency.setValueAtTime(spec.f, t);
          if (spec.to) source.frequency.exponentialRampToValueAtTime(spec.to, t + duration * .87);
        }
        if (spec.cutoff || spec.wave === 'noise') {
          const filter = c.createBiquadFilter();
          filter.type = 'lowpass';
          filter.frequency.value = spec.cutoff || spec.f;
          filter.Q.value = .55;
          voice.nodes.push(filter);
          source.connect(filter);
          filter.connect(envelope);
        } else source.connect(envelope);
        envelope.connect(voice.gain);
        const attack = spec.attack || .003;
        envelope.gain.setValueAtTime(0, t);
        envelope.gain.linearRampToValueAtTime(spec.a * (spec.rise ? .25 : 1), t + attack);
        if (!looping) {
          if (spec.rise) envelope.gain.linearRampToValueAtTime(spec.a, t + duration * .80);
          envelope.gain.exponentialRampToValueAtTime(.0001, t + duration - .008);
          envelope.gain.linearRampToValueAtTime(0, t + duration);
        }
        voice.end = Math.max(voice.end, t + duration + .004);
        // Keep completion bookkeeping on the callback, not a wall-clock timer.
        source.onended = () => {
          source._sixfoldEnded = true;
          if (voice.sources.every(s => s._sixfoldEnded)) this._disconnect(voice);
        };
        if (spec.wave === 'noise') source.start(t, this._random() * Math.max(0, .65 - duration - .01));
        else source.start(t);
        if (!looping) source.stop(t + duration + .003);
      }
      return voice;
    }

    _prune() {
      if (!this.ctx) return;
      for (const voice of [...this.voices]) {
        if (voice.end <= this.ctx.currentTime) this._disconnect(voice);
      }
    }

    _stopWhere(predicate) {
      for (const voice of [...this.voices]) if (!voice.stopping && predicate(voice)) this._kill(voice, false);
    }

    _kill(voice, immediate) {
      if (!this.ctx) return;
      const now = this.ctx.currentTime;
      voice.stopping = true;
      if (!immediate) {
        voice.gain.gain.cancelScheduledValues(now);
        voice.gain.gain.setValueAtTime(voice.gain.gain.value, now);
        voice.gain.gain.linearRampToValueAtTime(0, now + .008);
      } else voice.gain.gain.value = 0;
      for (const source of voice.sources) {
        try { source.stop(now + (immediate ? 0 : .009)); } catch (_) { /* already ended */ }
      }
      voice.end = Math.min(voice.end, now + .01);
      if (immediate) this._disconnect(voice);
    }

    _disconnect(voice) {
      for (const node of voice.nodes) {
        try { node.disconnect(); } catch (_) { /* already disconnected */ }
      }
      const i = this.voices.indexOf(voice);
      if (i >= 0) this.voices.splice(i, 1);
    }

    _releaseContext() {
      this._stopTimer();
      const c = this.ctx;
      if (c) {
        for (const voice of [...this.voices]) this._kill(voice, true);
        c.onstatechange = null;
        for (const node of [this._sfx, this._music, this._compressor, this._limiter, this._output]) {
          try { if (node) node.disconnect(); } catch (_) { /* partial graph */ }
        }
        try {
          const closing = c.close();
          if (closing && closing.catch) closing.catch(() => {});
        } catch (_) { /* audio remains optional */ }
      }
      this.ctx = null;
      this._graphReady = false;
      this._output = null;
      this._noise = null;
      this.voices = [];
      this._last = Object.create(null);
      this._nextCatch = 0;
    }
  }

  return { Sound };
});
