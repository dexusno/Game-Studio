'use strict';
// Device-free lifecycle tests. These validate scheduling and safety, not hearing.
const test = require('node:test');
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const vm = require('node:vm');
const code = fs.readFileSync(path.join(__dirname, '../src/audio.js'), 'utf8');

function fixture(options = {}) {
  const contexts = [];
  const intervals = new Map();
  const listeners = new Map();
  let nextTimer = 0;
  class Param {
    constructor(value = 0) { this.value = value; this.calls = []; }
    record(method, value, time) {
      assert.ok(Number.isFinite(value), `${method}: finite value`);
      assert.ok(Number.isFinite(time) && time >= 0, `${method}: valid time`);
      if (method === 'exponential') assert.ok(value > 0, 'exponential target must be positive');
      this.calls.push({ method, value, time });
      this.value = value;
    }
    setValueAtTime(v, t) { this.record('set', v, t); }
    linearRampToValueAtTime(v, t) { this.record('linear', v, t); }
    exponentialRampToValueAtTime(v, t) { this.record('exponential', v, t); }
    cancelScheduledValues(t) { assert.ok(Number.isFinite(t)); }
    cancelAndHoldAtTime(t) { assert.ok(Number.isFinite(t)); }
  }
  class Node {
    constructor(c) { this.context = c; this.disconnected = false; this.connections = []; c.nodes.push(this); }
    connect(node) { assert.ok(node); this.connections.push(node); return node; }
    disconnect() { this.disconnected = true; this.connections = []; }
  }
  class Source extends Node {
    constructor(c) { super(c); this.frequency = new Param(440); this.started = null; this.stopped = Infinity; c.sources.push(this); }
    start(t, offset = 0) {
      assert.equal(this.started, null, 'source must be started only once');
      assert.ok(Number.isFinite(t) && t >= 0);
      assert.ok(Number.isFinite(offset) && offset >= 0);
      this.started = t;
      if (this.buffer) assert.ok(offset < this.buffer.duration, 'noise starts within buffer');
    }
    stop(t) {
      assert.notEqual(this.started, null, 'Web Audio forbids stop before start');
      assert.ok(Number.isFinite(t) && t >= 0);
      this.stopped = t;
    }
  }
  class Context {
    constructor() {
      if (options.constructorFails) throw new Error('audio unavailable');
      this.currentTime = 0;
      this.sampleRate = 48000;
      this.state = 'suspended';
      this.nodes = [];
      this.sources = [];
      this.destination = new Node(this);
      contexts.push(this);
    }
    createGain() { const n = new Node(this); n.gain = new Param(1); return n; }
    createDynamicsCompressor() {
      if (options.graphFails) throw new Error('graph unavailable');
      const n = new Node(this);
      for (const p of ['threshold', 'knee', 'ratio', 'attack', 'release']) n[p] = new Param();
      return n;
    }
    createWaveShaper() { return new Node(this); }
    createBiquadFilter() { const n = new Node(this); n.frequency = new Param(); n.Q = new Param(); return n; }
    createOscillator() {
      if (options.sourceFails) throw new Error('source unavailable');
      return new Source(this);
    }
    createBufferSource() { return new Source(this); }
    createBuffer(channels, length, sampleRate) {
      assert.equal(channels, 1);
      const data = new Float32Array(length);
      return { duration: length / sampleRate, getChannelData: () => data };
    }
    resume() {
      if (options.resumeFails) return Promise.reject(new Error('autoplay blocked'));
      this.state = 'running';
      if (this.onstatechange) this.onstatechange();
      return Promise.resolve();
    }
    close() { this.state = 'closed'; return Promise.resolve(); }
    advance(seconds) {
      this.currentTime += seconds;
      for (const s of this.sources) {
        if (!s.ended && s.stopped <= this.currentTime) {
          s.ended = true;
          if (s.onended) s.onended();
        }
      }
    }
    get connectedSources() {
      return this.sources.filter(s => !s.disconnected && s.stopped > this.currentTime).length;
    }
  }
  const document = {
    hidden: false, visibilityState: 'visible',
    addEventListener: (type, fn) => listeners.set(type, fn),
    removeEventListener: type => listeners.delete(type)
  };
  const sandbox = {
    document, navigator: { userActivation: { isActive: true } },
    setInterval: fn => { intervals.set(++nextTimer, fn); return nextTimer; },
    clearInterval: id => intervals.delete(id),
    module: { exports: {} }
  };
  if (!options.missingAPI) sandbox.AudioContext = Context;
  vm.runInNewContext(code, sandbox, { filename: 'audio.js' });
  const sound = new sandbox.SixfoldAudio.Sound();
  return {
    sound, contexts, sandbox, intervals, listeners,
    hide(value) { document.hidden = value; document.visibilityState = value ? 'hidden' : 'visible'; listeners.get('visibilitychange')(); },
    tick() { for (const fn of [...intervals.values()]) fn(); }
  };
}

async function running(options) {
  const f = fixture(options);
  f.sound.setScene({ active: true, paused: false });
  assert.equal(await f.sound.unlock(), true);
  return f;
}

test('lazy creation, real-gesture guard, no buffered pre-unlock actions', async () => {
  const f = fixture();
  f.sound.setScene({ active: true });
  assert.equal(f.sound.event({ type: 'launch' }), false);
  assert.equal(f.contexts.length, 0);
  f.sandbox.navigator.userActivation.isActive = false;
  assert.equal(await f.sound.unlock(), false);
  assert.equal(f.contexts.length, 0);
  f.sandbox.navigator.userActivation.isActive = true;
  assert.equal(await f.sound.unlock(), true);
  assert.equal(f.sound.voices.filter(v => v.key === 'launch').length, 0);
  assert.equal(f.intervals.size, 1);
  await f.sound.unlock();
  assert.equal(f.contexts.length, 1);
  assert.equal(f.intervals.size, 1);
  f.sound.dispose();
});

test('missing, failing, and blocked AudioContexts leave gameplay usable', async () => {
  for (const options of [{ missingAPI: true }, { constructorFails: true }, { graphFails: true }, { resumeFails: true }]) {
    const f = fixture(options);
    f.sound.setScene({ active: true });
    assert.equal(await f.sound.unlock(), false);
    assert.equal(f.sound.event({ type: 'hull' }), false);
    assert.equal(f.intervals.size, 0);
    f.sound.setVolumes({ sfx: 0 });
    f.sound.dispose();
    if (f.contexts.length) assert.equal(f.contexts[0].state, 'closed');
  }
});

test('all core cues schedule valid nodes; catches preserve six ordered pitches', async () => {
  const f = await running();
  const c = f.contexts[0];
  f.sound.setVolumes({ music: 0 });
  c.advance(.04);
  const pitches = [];
  for (let held = 1; held <= 6; held++) {
    assert.equal(f.sound.event({ type: 'collect', held }), true);
    const v = f.sound.voices.findLast(v => v.key === 'collect');
    pitches.push(v.sources[0].frequency.calls[0].value);
  }
  assert.equal(new Set(pitches).size, 6);
  assert.ok(pitches.every((f, i) => i === 0 || f > pitches[i - 1]));
  assert.ok(f.sound.voices.filter(v => v.key === 'collect').at(-1).when - c.currentTime < .10);
  c.advance(1);
  for (const type of ['launch', 'empty', 'rejected', 'land', 'recallStart', 'block', 'hull', 'enemyHit', 'enemyDie', 'shoot', 'warning', 'stageClear', 'win', 'lose']) {
    assert.equal(f.sound.event({ type, returnHit: true }), true, type);
    c.advance(2);
  }
  assert.equal(f.sound.event({ type: 'invented' }), false);
  f.sound.dispose();
});

test('charge lasts through the tell, then matching enemy release cancels its tail', async () => {
  const f = await running();
  assert.equal(f.sound.event({ type: 'warning', kind: 'charge', duration: .9, enemyId: 42 }), true);
  const v = f.sound.voices.find(v => v.key === 'charge');
  assert.ok(v.end > .9 && v.end < .92);
  f.contexts[0].advance(.3);
  assert.equal(f.sound.event({ type: 'shoot', enemyId: 42 }), true);
  assert.equal(v.stopping, true);
  f.contexts[0].advance(.012);
  assert.ok(v.sources.every(s => s.disconnected));
  f.sound.dispose();
});

test('pause and hidden-tab silence cut loops and scheduled catches; nothing replays', async () => {
  const f = await running();
  const c = f.contexts[0];
  f.sound.setScene({ recalling: true });
  for (let held = 1; held <= 6; held++) f.sound.event({ type: 'collect', held });
  f.sound.setScene({ paused: true });
  c.advance(.012);
  assert.equal(c.connectedSources, 0);
  assert.equal(f.intervals.size, 0);
  assert.equal(f.sound.event({ type: 'hull' }), false);
  f.sound.setScene({ paused: false });
  assert.equal(f.sound.voices.filter(v => v.key === 'collect').length, 0);
  assert.equal(f.intervals.size, 1);
  f.hide(true);
  c.advance(.012);
  assert.equal(c.connectedSources, 0);
  assert.equal(f.intervals.size, 0);
  assert.equal(f.sound.event({ type: 'win' }), false);
  f.hide(false);
  assert.equal(f.intervals.size, 1);
  f.sound.dispose();
});

test('music and effects mute independently, including the recall hum', async () => {
  const f = await running();
  const c = f.contexts[0];
  f.sound.setScene({ recalling: true });
  f.sound.setVolumes({ sfx: 0 });
  c.advance(.03);
  assert.ok(f.sound.voices.every(v => v.bus === 'music'));
  assert.equal(f.sound.event({ type: 'block' }), false);
  f.sound.setVolumes({ sfx: 1, music: 0 });
  c.advance(.03);
  assert.ok(f.sound.voices.every(v => v.bus === 'sfx'));
  assert.equal(f.intervals.size, 0);
  assert.equal(f.sound.event({ type: 'launch' }), true);
  f.sound.setVolumes({ sfx: -1, music: Infinity });
  c.advance(.03);
  assert.equal(c.connectedSources, 0);
  assert.equal(f.sound.volumes.sfx, 0);
  assert.equal(f.sound.volumes.music, 0);
  f.sound.dispose();
});

test('thousands of events stay bounded and preserve hull priority', async () => {
  const f = await running();
  const c = f.contexts[0];
  f.sound.setScene({ recalling: true, danger: 1 });
  const types = ['collect', 'enemyHit', 'land', 'enemyDie', 'launch', 'shoot', 'block'];
  let maximum = 0;
  for (let i = 0; i < 3000; i++) {
    f.sound.event({ type: types[i % types.length], held: i % 6 + 1 });
    maximum = Math.max(maximum, c.connectedSources);
    assert.ok(c.connectedSources <= 32, `${c.connectedSources} source nodes`);
    assert.ok(f.sound.voices.length <= 18);
    if (i % 7 === 0) { c.advance(.005); f.tick(); }
  }
  assert.ok(maximum >= 20, 'stress fixture reached meaningful overlap');
  assert.equal(f.sound.event({ type: 'hull' }), true);
  assert.ok(c.connectedSources <= 32);
  const curve = f.sound._limiter.curve;
  assert.ok([...curve].every(x => Number.isFinite(x) && Math.abs(x) < .80));
  f.sound.dispose();
});

test('terminal cue survives inactive results; pause and disposal silence it', async () => {
  const f = await running();
  const c = f.contexts[0];
  f.sound.setScene({ active: false, recalling: false });
  assert.equal(f.sound.event({ type: 'launch' }), false);
  assert.equal(f.sound.event({ type: 'win' }), true);
  f.sound.setScene({ active: false, paused: false });
  assert.ok(f.sound.voices.find(v => v.key === 'win' && !v.stopping));
  f.sound.setScene({ paused: true });
  c.advance(.012);
  assert.equal(c.connectedSources, 0);
  f.sound.dispose();
  f.sound.dispose();
  assert.equal(c.state, 'closed');
  assert.equal(f.listeners.size, 0);
  assert.equal(f.intervals.size, 0);
  assert.equal(await f.sound.unlock(), false);
});

test('audio interruption is silent, and long stalls do not catch up missed beats', async () => {
  const f = await running();
  const c = f.contexts[0];
  c.state = 'interrupted';
  c.onstatechange();
  c.advance(.02);
  assert.equal(c.connectedSources, 0);
  assert.equal(f.intervals.size, 0);
  assert.equal(f.sound.event({ type: 'launch' }), false);
  assert.equal(await f.sound.unlock(), true);
  c.advance(60);
  const before = c.sources.length;
  f.tick();
  assert.ok(c.sources.length - before <= 2);
  f.sound.dispose();
});

test('failure while creating a voice releases the graph without throwing', async () => {
  const options = {};
  const f = await running(options);
  options.sourceFails = true;
  assert.equal(f.sound.event({ type: 'launch' }), false);
  assert.equal(f.sound.ctx, null);
  assert.equal(f.intervals.size, 0);
  f.sound.dispose();
});

// Optional native check uses already-installed tools only. Set module/executable
// paths in SIXFOLD_PLAYWRIGHT and SIXFOLD_CHROMIUM; never launches audible audio.
test('native offline waveform: every cue, overload headroom, click-free silence', {
  skip: !process.env.SIXFOLD_PLAYWRIGHT || !process.env.SIXFOLD_CHROMIUM ? 'Set installed browser tool paths to run native offline rendering' : false,
  timeout: 30000
}, async t => {
  const { chromium } = require(process.env.SIXFOLD_PLAYWRIGHT);
  const browser = await chromium.launch({
    executablePath: process.env.SIXFOLD_CHROMIUM, headless: true, args: ['--mute-audio']
  });
  try {
    const page = await browser.newPage();
    const result = await page.evaluate(async source => {
      const outputs = [];
      async function render(label, actions, seconds = 1.8, volumes = { music: 0, sfx: 1 }) {
        const raw = new OfflineAudioContext(1, Math.ceil(48000 * seconds), 48000);
        let pulse = null;
        // The adapter schedules on the actual offline clock; no device context
        // is ever constructed, and suspend points let later events clean up
        // already-rendered voices just as they do during a real game.
        class Context {
          constructor() { this.sampleRate = 48000; this.destination = raw.destination; this.state = 'running'; }
          get currentTime() { return raw.currentTime; }
          resume() { return Promise.resolve(); }
          close() { this.state = 'closed'; return Promise.resolve(); }
        }
        for (const method of ['createGain', 'createDynamicsCompressor', 'createWaveShaper', 'createBuffer', 'createOscillator', 'createBufferSource', 'createBiquadFilter']) {
          Context.prototype[method] = function (...args) { return raw[method](...args); };
        }
        const sandbox = {
          AudioContext: Context, navigator: { userActivation: { isActive: true } },
          document: { hidden: false, visibilityState: 'visible', addEventListener() {}, removeEventListener() {} },
          setInterval: fn => { pulse = fn; return 1; }, clearInterval: () => { pulse = null; }
        };
        const api = new Function('globalThis', source + '\nreturn globalThis.SixfoldAudio;')(sandbox);
        const sound = new api.Sound();
        sound.setVolumes(volumes);
        sound.setScene({ active: true, paused: false });
        if (!await sound.unlock()) throw new Error('Offline graph did not initialize');
        let actionError;
        const waits = actions.map(([at, action]) => raw.suspend(at).then(async () => {
          try { action(sound, sandbox, pulse); } catch (error) { actionError = error; }
          await raw.resume();
        }));
        const rendered = await raw.startRendering();
        await Promise.all(waits);
        if (actionError) throw actionError;
        const data = rendered.getChannelData(0);
        let peak = 0, sum = 0, dc = 0, maxStep = 0, last = -1, clipped = 0;
        for (let i = 0; i < data.length; i++) {
          const a = Math.abs(data[i]);
          if (!Number.isFinite(a)) throw new Error(label + ': nonfinite sample');
          peak = Math.max(peak, a); sum += data[i] * data[i]; dc += data[i];
          if (a > 1e-6) last = i;
          if (a >= .99) clipped++;
          if (i) maxStep = Math.max(maxStep, Math.abs(data[i] - data[i - 1]));
        }
        const rms = (a, b) => {
          let total = 0;
          const first = Math.ceil(a * 48000), end = Math.floor(b * 48000);
          for (let i = first; i < end; i++) total += data[i] * data[i];
          return Math.sqrt(total / (end - first));
        };
        const metrics = { label, peak, rms: Math.sqrt(sum / data.length), dc: dc / data.length, maxStep, clipped, last: last / 48000, tail: rms(seconds - .1, seconds), early: rms(0, .02) };
        if (['pause', 'hidden', 'mute'].includes(label)) metrics.afterSilence = rms(.33, .7);
        sound.dispose();
        outputs.push(metrics);
      }
      for (const type of ['launch', 'empty', 'rejected', 'land', 'collect', 'recallStart', 'block', 'hull', 'enemyHit', 'enemyDie', 'shoot', 'warning', 'stageClear', 'win', 'lose']) {
        await render(type, [[.04, s => { if (!s.event({ type, held: 6 })) throw new Error(type + ' was rejected'); }]]);
      }
      await render('charge', [[.04, s => s.event({ type: 'warning', kind: 'charge', duration: .9, enemyId: 42 })]]);
      await render('six-catches', [[.04, s => { for (let held = 1; held <= 6; held++) s.event({ type: 'collect', held }); }]]);
      await render('stress', Array.from({ length: 20 }, (_, i) => [.04 + i * .04, s => {
        for (let held = 1; held <= 6; held++) s.event({ type: 'collect', held });
        for (const type of ['launch', 'enemyHit', 'enemyDie', 'block', 'hull', 'shoot']) s.event({ type });
        s.setScene({ recalling: true, danger: 1 });
      }]).concat([[1, s => s.setScene({ active: false, recalling: false })]]), 2, { music: 1, sfx: 1 });
      for (const kind of ['pause', 'hidden', 'mute']) {
        await render(kind, [[.04, s => {
          s.setScene({ recalling: true }); s.event({ type: 'warning', kind: 'charge', duration: 1 });
        }], [.30, (s, scope) => {
          if (kind === 'pause') s.setScene({ paused: true });
          else if (kind === 'mute') s.setVolumes({ sfx: 0, music: 0 });
          else { scope.document.hidden = true; scope.document.visibilityState = 'hidden'; s.setScene({ active: true }); }
        }]], .8, { music: 1, sfx: 1 });
      }
      await render('ambience', Array.from({ length: 25 }, (_, i) => [.06 + i * .1, (s, scope, tick) => { if (tick) tick(); }])
        .concat([[2.6, s => s.setScene({ active: false })]]), 3, { music: .3, sfx: .75 });
      return { browser: navigator.userAgent, outputs };
    }, code);
    for (const out of result.outputs) {
      assert.ok(out.peak > 0 && out.peak < .8, `${out.label}: nonzero output with headroom`);
      assert.equal(out.clipped, 0, `${out.label}: clipping`);
      assert.ok(Math.abs(out.dc) < .002, `${out.label}: DC bias`);
      assert.equal(out.early, 0, `${out.label}: quiet before first trigger`);
      assert.equal(out.tail, 0, `${out.label}: finite tail`);
      if ('afterSilence' in out) {
        assert.equal(out.afterSilence, 0, `${out.label}: silence`);
        assert.ok(out.maxStep < .025, `${out.label}: shutdown discontinuity ${out.maxStep}`);
        assert.ok(out.last < .32, `${out.label}: shutdown exceeds 20 ms including render quantum/graph latency`);
      }
    }
    t.diagnostic(JSON.stringify({
      browser: result.browser, renderedCases: result.outputs.length,
      stressPeak: result.outputs.find(o => o.label === 'stress').peak,
      ambiencePeak: result.outputs.find(o => o.label === 'ambience').peak,
      silence: result.outputs.filter(o => 'afterSilence' in o).map(o => ({ kind: o.label, last: o.last, maxStep: o.maxStep }))
    }));
  } finally {
    await browser.close();
  }
});
