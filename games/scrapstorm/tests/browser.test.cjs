/* Independent packaged-browser checks. Uses an installed Playwright and Chrome.
 * Set SIXFOLD_PLAYWRIGHT to its module directory and SIXFOLD_CHROMIUM to a browser.
 * No desktop input or audible playback. Fixture checks are labelled explicitly.
 */
'use strict';
const assert = require('node:assert/strict');
const fs = require('node:fs');
const path = require('node:path');
const http = require('node:http');
const crypto = require('node:crypto');
const { pathToFileURL } = require('node:url');
const { chromium } = require(process.env.SIXFOLD_PLAYWRIGHT || 'playwright');

const gameDir = path.resolve(__dirname, '..');
const artifactPath = path.join(gameDir, 'build', 'Sixfold-Recoil-Beta.html');
const artifact = fs.readFileSync(artifactPath);
const build = JSON.parse(fs.readFileSync(path.join(gameDir, 'build', 'build-info.json'), 'utf8'));
const KEY = 'sixfold-recoil-beta-v1';
const report = {
  date: new Date().toISOString(), build,
  artifactSha256: crypto.createHash('sha256').update(artifact).digest('hex'),
  environment: { platform: process.platform, arch: process.arch, node: process.version, headless: true, muted: true },
  checks: [],
  limits: ['No human fun assessment.', 'No physical gamepad, speakers, or foreground fullscreen/display verification.', 'Synthetic focus and controller events have limited hardware coverage.']
};
let browser, server, base;
const snap = page => page.evaluate(() => window.__sixfold.snapshot());
const until = (page, predicate, arg, timeout = 5000) => page.waitForFunction(predicate, arg, { timeout });
const settle = page => page.evaluate(() => new Promise(resolve => requestAnimationFrame(() => requestAnimationFrame(resolve))));
async function check(name, fn) {
  if (process.env.SIXFOLD_TEST_FILTER && !new RegExp(process.env.SIXFOLD_TEST_FILTER, 'i').test(name)) return;
  const started = Date.now();
  const context = await browser.newContext({ viewport: { width: 1280, height: 720 }, acceptDownloads: true });
  const page = await context.newPage(), errors = [];
  page.on('pageerror', error => errors.push(error.message));
  page.on('console', message => { if (message.type() === 'error') errors.push(message.text()); });
  try {
    const evidence = await fn(page, context);
    assert.deepEqual(errors, [], 'browser runtime or console errors');
    if (await page.evaluate(() => Boolean(window.__sixfold)).catch(() => false)) assert.deepEqual((await snap(page)).faults, []);
    report.checks.push({ name, status: 'passed', milliseconds: Date.now() - started, evidence });
    console.log(`PASS ${name}`);
  } catch (error) {
    const state = await snap(page).catch(() => null);
    report.checks.push({ name, status: 'failed', milliseconds: Date.now() - started, error: error.stack, browserErrors: errors,
      state: state ? { view: state.view, build: state.build.hash, time: state.game.time, recalling: state.game.recalling, heldCount: state.game.heldCount, stats: state.game.stats } : null });
    console.log(`FAIL ${name}: ${error.message}`);
  } finally { await context.close(); }
}
async function load(page, qa = false) {
  await page.goto(base + (qa ? '?qa=1' : ''), { waitUntil: 'load' });
  await until(page, () => Boolean(window.__sixfold));
  assert.equal((await snap(page)).build.hash, build.hash, 'loaded artifact identity');
}
async function start(page) {
  await page.locator('#start').click();
  await until(page, () => window.__sixfold.snapshot().game.time > .12);
  assert.equal((await snap(page)).view, 'play');
}
async function point(page, x, y) {
  return page.evaluate(({ x, y }) => {
    const r = window.__sixfold.renderer;
    return { x: r.ox + x * r.scale, y: r.oy + y * r.scale };
  }, { x, y });
}
async function aim(page, x, y) { const p = await point(page, x, y); await page.mouse.move(p.x, p.y); }
async function clickThrow(page, x = 12, y = 7) { await aim(page, x, y); await page.mouse.click((await point(page, x, y)).x, (await point(page, x, y)).y); }

(async () => {
  server = http.createServer((request, response) => {
    if (request.url.startsWith('/favicon.ico')) { response.writeHead(204); response.end(); return; }
    response.writeHead(200, { 'Content-Type': 'text/html; charset=utf-8', 'Cache-Control': 'no-store' }); response.end(artifact);
  });
  await new Promise(resolve => server.listen(0, '127.0.0.1', resolve));
  base = `http://127.0.0.1:${server.address().port}/Sixfold-Recoil-Beta.html`;
  const executablePath = process.env.SIXFOLD_CHROMIUM || process.env.SIXFOLD_CHROME;
  browser = await chromium.launch({ headless: true, ...(executablePath ? { executablePath } : {}), args: ['--mute-audio'] });
  report.environment.browser = browser.version();

  await check('Offline file launch, keyboard menu, movement, throw, recall and input priority', async (page, context) => {
    const external = [];
    page.on('request', request => { if (/^https?:/.test(request.url())) external.push(request.url()); });
    await context.setOffline(true);
    await page.goto(pathToFileURL(artifactPath).href, { waitUntil: 'load' });
    await until(page, () => Boolean(window.__sixfold));
    assert.equal((await snap(page)).build.hash, build.hash);
    assert.equal((await snap(page)).view, 'home');
    await page.locator('#start').focus(); await page.keyboard.press('Enter');
    await until(page, () => window.__sixfold.snapshot().game.time > .12);
    const initial = (await snap(page)).game.player.x;
    await page.keyboard.down('d'); await page.waitForTimeout(300); await page.keyboard.up('d');
    assert.ok((await snap(page)).game.player.x > initial + .7, 'WASD moves');
    await page.keyboard.down('ArrowLeft'); await page.waitForTimeout(300); await page.keyboard.up('ArrowLeft');
    assert.ok(Math.abs((await snap(page)).game.player.x - initial) < .5, 'arrow control reverses movement');
    await aim(page, 12, 7); await page.mouse.down(); await page.waitForTimeout(400); await page.mouse.up();
    let s = await snap(page); assert.equal(s.game.stats.volleys, 1, 'held left mouse is one throw'); assert.equal(s.game.stats.piecesLaunched, 6);
    await until(page, () => window.__sixfold.snapshot().game.pieces.every(p => p.state === 'loose'));
    await page.mouse.down({ button: 'right' });
    await until(page, () => window.__sixfold.snapshot().game.pieces.some(p => p.state === 'returning'));
    await until(page, () => window.__sixfold.snapshot().game.heldCount === 6);
    await page.mouse.click((await point(page, 12, 7)).x, (await point(page, 12, 7)).y);
    await until(page, () => window.__sixfold.snapshot().game.stats.rejectedThrows > 0);
    assert.equal((await snap(page)).game.stats.volleys, 1, 'default recall prevents throw');
    await page.mouse.up({ button: 'right' }); await settle(page);
    await clickThrow(page); await until(page, () => window.__sixfold.snapshot().game.stats.volleys === 2);
    assert.deepEqual(external, [], 'standalone game requires no network requests');
    s = await snap(page); return { networkRequests: external.length, volleys: s.game.stats.volleys, collects: s.game.stats.collects, buildHash: s.build.hash };
  });

  await check('Pause, focus loss, fresh controls, resume, settings and exit to menu', async page => {
    await load(page, true); await start(page);
    await page.keyboard.down('d'); await page.waitForTimeout(150); await page.keyboard.press('Escape');
    await until(page, () => window.__sixfold.snapshot().view === 'pause');
    const stopped = (await snap(page)).game;
    await page.waitForTimeout(350);
    assert.equal((await snap(page)).game.time, stopped.time);
    await page.keyboard.press('Escape'); await page.waitForTimeout(200);
    assert.equal((await snap(page)).game.player.x, stopped.player.x, 'held movement does not restart on resume');
    await page.evaluate(() => window.dispatchEvent(new KeyboardEvent('keydown', { key: 'd', code: 'KeyD', repeat: true, bubbles: true })));
    await page.waitForTimeout(150);
    assert.equal((await snap(page)).game.player.x, stopped.player.x, 'OS key repeat must not bypass fresh-input requirement');
    await page.keyboard.up('d'); await page.keyboard.down('d'); await page.waitForTimeout(120); await page.keyboard.up('d');
    assert.ok((await snap(page)).game.player.x > stopped.player.x + .25);
    await clickThrow(page); await page.waitForTimeout(550); await page.mouse.down({ button: 'right' });
    await until(page, () => window.__sixfold.snapshot().game.recalling);
    await page.evaluate(() => window.dispatchEvent(new Event('blur')));
    await until(page, () => window.__sixfold.snapshot().view === 'pause');
    assert.match(await page.locator('#pause-reason').innerText(), /FOCUS/);
    const focusTime = (await snap(page)).game.time; await page.waitForTimeout(250); assert.equal((await snap(page)).game.time, focusTime);
    await page.locator('#resume').click(); await page.waitForTimeout(120);
    assert.equal((await snap(page)).game.recalling, false, 'held recall is cleared after focus change');
    await page.mouse.up({ button: 'right' });
    await page.keyboard.press('Escape'); await page.locator('#pause-settings').click();
    const settingsTime = (await snap(page)).game.time; await page.waitForTimeout(150); assert.equal((await snap(page)).game.time, settingsTime);
    await page.locator('#settings-back').click(); assert.equal((await snap(page)).view, 'pause');
    await page.locator('#quit').click(); assert.equal((await snap(page)).view, 'home');
    await start(page); const fresh = (await snap(page)).game;
    assert.equal(fresh.player.hull, 3); assert.equal(fresh.heldCount, 6); assert.equal(fresh.stats.volleys, 0);
    return { focusEvent: 'synthetic window blur', pauseFreezesSimulation: true, freshMenuStart: true };
  });

  await check('Left-held mouse recall and throw-interrupt chord require fresh recall afterwards', async page => {
    await load(page);
    await page.locator('#home-settings').click(); await page.locator('#interrupt').check(); await page.locator('#settings-back').click();
    await start(page); await aim(page, 12, 7);
    await page.mouse.down({ button: 'left' });
    await until(page, () => window.__sixfold.snapshot().game.stats.volleys === 1);
    await page.waitForTimeout(550); await page.mouse.down({ button: 'right' });
    await until(page, () => window.__sixfold.snapshot().game.recalling);
    await until(page, () => window.__sixfold.snapshot().game.heldCount === 6);
    assert.equal((await snap(page)).game.stats.volleys, 1, 'held left mouse does not repeat throws');
    await page.mouse.up({ button: 'left' }); await settle(page); await page.mouse.down({ button: 'left' });
    await until(page, () => window.__sixfold.snapshot().game.stats.volleys === 2);
    assert.equal((await snap(page)).game.recalling, false, 'fresh left edge interrupts held recall');
    await page.mouse.up({ button: 'left' }); await page.waitForTimeout(600);
    assert.equal((await snap(page)).game.recalling, false, 'held right button cannot automatically rearm recall');
    await page.mouse.up({ button: 'right' }); await settle(page); await page.mouse.down({ button: 'right' });
    await until(page, () => window.__sixfold.snapshot().game.recalling);
    await page.mouse.up({ button: 'right' });
    return { leftThenRight: 'recalls', rightThenFreshLeft: 'interrupt throws', freshRecallRequired: true };
  });

  await check('Settings persist normally and next-attempt input options are isolated', async page => {
    await load(page);
    await page.locator('input[name="recall"][value="cutting"]').check();
    await page.locator('#home-settings').click();
    await page.locator('#shake').uncheck(); await page.locator('#interrupt').check();
    await page.locator('#effects-volume').fill('23'); await page.locator('#music-volume').fill('11');
    await page.locator('#settings-back').click(); await page.reload();
    let s = await snap(page);
    assert.deepEqual(s.settings, { music: .11, sfx: .23, shake: false, interruptRecall: true, returnDamage: true });
    await start(page); assert.equal((await snap(page)).activeOptions.interruptRecall, true);
    await page.keyboard.press('Escape'); await page.locator('#pause-settings').click(); await page.locator('#interrupt').uncheck();
    s = await snap(page); assert.equal(s.settings.interruptRecall, false); assert.equal(s.activeOptions.interruptRecall, true);
    await page.locator('#settings-back').click(); await page.locator('#restart').click();
    await until(page, () => window.__sixfold.snapshot().game.time > .1);
    s = await snap(page); assert.equal(s.activeOptions.interruptRecall, false); assert.equal(s.activeOptions.returnDamage, true);
    return { persistedSettings: s.settings, nextAttemptApplied: true };
  });

  await check('Corrupt or unavailable local storage still permits play', async (page, context) => {
    await load(page); await page.evaluate(key => localStorage.setItem(key, '{bad json'), KEY); await page.reload();
    let s = await snap(page); assert.equal(s.view, 'home'); assert.equal(s.settings.returnDamage, false);
    await start(page); assert.equal((await snap(page)).view, 'play');
    await context.addInitScript(() => {
      Object.defineProperty(Storage.prototype, 'getItem', { value() { throw new DOMException('Disabled for test', 'SecurityError'); } });
      Object.defineProperty(Storage.prototype, 'setItem', { value() { throw new DOMException('Disabled for test', 'SecurityError'); } });
    });
    await page.reload(); await start(page); assert.equal((await snap(page)).storageAvailable, false);
    await page.keyboard.press('Escape'); await page.locator('#pause-settings').click();
    assert.match(await page.locator('#storage-note').innerText(), /Saving is unavailable/);
    return { corruptJSON: 'playable defaults', deniedStorage: 'playable with explicit save status' };
  });

  await check('Unavailable Web Audio degrades to playable silent mode', async (page, context) => {
    await context.addInitScript(() => {
      Object.defineProperty(window, 'AudioContext', { value: undefined });
      Object.defineProperty(window, 'webkitAudioContext', { value: undefined });
    });
    await load(page); await start(page); await clickThrow(page);
    await until(page, () => window.__sixfold.snapshot().game.stats.volleys === 1);
    await page.keyboard.press('Escape'); await page.locator('#pause-settings').click();
    await page.locator('#effects-volume').fill('50'); await page.locator('#settings-back').click(); await page.locator('#resume').click();
    assert.equal((await snap(page)).view, 'play');
    return { audioAPI: 'deliberately unavailable', gameplayAndSettingsRemainUsable: true };
  });

  await check('Natural no-input defeat, result export, retry and recall-mode comparison', async page => {
    await load(page); await start(page);
    await until(page, () => window.__sixfold.snapshot().view === 'results', undefined, 35000);
    let s = await snap(page);
    assert.equal(s.game.state, 'lost'); assert.equal(s.game.player.hull, 0); assert.equal(s.game.stats.hullHits, 3);
    assert.ok(s.game.stats.blocks >= 6); assert.match(await page.locator('#result-title').innerText(), /Loose screws/);
    const downloadPromise = page.waitForEvent('download'); await page.locator('#save-run').click();
    const download = await downloadPromise; const exported = JSON.parse(fs.readFileSync(await download.path(), 'utf8'));
    assert.equal(exported.outcome, 'lost'); assert.equal(exported.build.hash, build.hash); assert.equal(exported.options.returnDamage, false);
    const lossSeconds = s.game.time;
    await page.locator('#retry').click(); await until(page, () => window.__sixfold.snapshot().game.time > .1);
    s = await snap(page); assert.equal(s.game.player.hull, 3); assert.equal(s.game.heldCount, 6); assert.equal(s.game.stats.hullHits, 0);
    await page.keyboard.press('Escape'); await page.locator('#quit').click();
    await page.locator('input[name="recall"][value="cutting"]').check(); await start(page);
    assert.equal((await snap(page)).activeOptions.returnDamage, true);
    return { lossSeconds, blocks: exported.stats.blocks, hullHits: exported.stats.hullHits, exportedBuild: exported.build.hash, freshRetry: true };
  });

  await check('Terminal victory fixture reaches results, stores best and switches mode', async (page, context) => {
    // Deliberately constructed terminal state: tests the browser results/persistence flow,
    // not the ability of a human or input bot to beat the natural level.
    await context.addInitScript(() => {
      window.__qaFinish = null;
      const descriptor = Object.getOwnPropertyDescriptor(window, 'SixfoldCore');
      if (descriptor) throw new Error('Unexpected core before scripts');
      Object.defineProperty(window, 'SixfoldCore', {
        configurable: true,
        set(api) {
          delete window.SixfoldCore; window.SixfoldCore = api;
          const original = api.Game.prototype.step;
          api.Game.prototype.step = function (dt, input) {
            original.call(this, dt, input);
            if (window.__qaFinish && this.state === 'playing') {
              const fixture = window.__qaFinish; window.__qaFinish = null;
              this.time = fixture.time; this.stage = 3; this.stageName = 'Foreman';
              this.player.hull = fixture.hull; this._finish('won');
            }
          };
        }
      });
    });
    await load(page); await start(page);
    await page.evaluate(() => { window.__qaFinish = { time: 123.5, hull: 2 }; });
    await until(page, () => window.__sixfold.snapshot().view === 'results');
    assert.match(await page.locator('#result-title').innerText(), /Line cleared/);
    assert.match(await page.locator('#best-note').innerText(), /best clear/);
    assert.equal((await snap(page)).game.heldCount, 6);
    const persisted = await page.evaluate(key => JSON.parse(localStorage.getItem(key)), KEY);
    assert.equal(Object.values(persisted.records)[0], 123.5);
    await page.locator('#switch-mode').click(); await until(page, () => window.__sixfold.snapshot().game.time > .1);
    assert.equal((await snap(page)).activeOptions.returnDamage, true);
    await page.evaluate(() => { window.__qaFinish = { time: 110, hull: 3 }; });
    await until(page, () => window.__sixfold.snapshot().view === 'results');
    const second = await page.evaluate(key => JSON.parse(localStorage.getItem(key)), KEY);
    assert.equal(Object.keys(second.records).length, 2, 'recall modes have separate records');
    await page.locator('#results-menu').click(); await page.reload();
    assert.equal((await snap(page)).settings.returnDamage, true);
    return { kind: 'constructed terminal fixture', harmlessBest: 123.5, cuttingBest: 110, separateRecords: true };
  });

  await check('Resize, display geometry and browser fullscreen request', async page => {
    await load(page, true);
    const sizes = [{ width: 1920, height: 1080 }, { width: 1280, height: 720 }, { width: 1024, height: 768 }, { width: 800, height: 600 }];
    const geometry = [];
    for (const size of sizes) {
      await page.setViewportSize(size); await settle(page);
      const data = await page.evaluate(() => {
        const r = window.__sixfold.renderer, b = document.querySelector('#start').getBoundingClientRect();
        return { width: innerWidth, height: innerHeight, arena: [r.ox, r.oy, r.ox + r.scale * 24, r.oy + r.scale * 14], start: [b.left, b.top, b.right, b.bottom], scrollWidth: document.documentElement.scrollWidth };
      });
      assert.ok(data.start[0] >= 0 && data.start[1] >= 0 && data.start[2] <= size.width && data.start[3] <= size.height, 'start button stays visible');
      assert.ok(data.arena[0] >= 0 && data.arena[1] >= 0 && data.arena[2] <= size.width && data.arena[3] <= size.height, 'arena stays on screen');
      assert.ok(data.scrollWidth <= size.width, 'no horizontal overflow'); geometry.push(data);
    }
    await page.locator('#fullscreen').click(); await settle(page);
    const fullscreen = await page.evaluate(() => Boolean(document.fullscreenElement));
    if (fullscreen) await page.evaluate(() => document.exitFullscreen());
    await start(page); await page.setViewportSize({ width: 1280, height: 720 }); await settle(page);
    await aim(page, 6, 1); await page.waitForTimeout(120);
    assert.ok((await snap(page)).game.player.aim.y < -.99, 'mouse aim stays aligned after resize');
    return { geometry, headlessFullscreenAccepted: fullscreen, foregroundDisplayNotTested: true };
  });

  await check('Simulated standard controller movement, edge-trigger throw, pause and disconnect', async (page, context) => {
    await context.addInitScript(() => {
      window.__pad = { connected: true, mapping: 'standard', axes: [0, 0, 0, 0], buttons: Array.from({ length: 17 }, () => ({ value: 0, pressed: false })) };
      Object.defineProperty(navigator, 'getGamepads', { value: () => window.__pad ? [window.__pad] : [] });
    });
    await load(page, true); await start(page); await settle(page);
    const initial = (await snap(page)).game.player.x;
    await page.evaluate(() => { window.__pad.axes = [1, 0, 1, 0]; }); await page.waitForTimeout(250);
    assert.ok((await snap(page)).game.player.x > initial + .6);
    await page.evaluate(() => { window.__pad.axes = [0, 0, 1, 0]; window.__pad.buttons[7] = { value: 1, pressed: true }; });
    await page.waitForTimeout(350); assert.equal((await snap(page)).game.stats.volleys, 1);
    await page.evaluate(() => { window.__pad.buttons[9] = { value: 1, pressed: true }; });
    await until(page, () => window.__sixfold.snapshot().view === 'pause');
    await page.evaluate(() => { window.__pad.buttons[9] = { value: 0, pressed: false }; }); await settle(page);
    await page.evaluate(() => { window.__pad.buttons[9] = { value: 1, pressed: true }; });
    await until(page, () => window.__sixfold.snapshot().view === 'play'); await page.waitForTimeout(150);
    assert.equal((await snap(page)).game.stats.volleys, 1, 'held trigger does not fire after resume');
    await page.evaluate(() => { window.__pad.buttons[9] = { value: 0, pressed: false }; window.dispatchEvent(new Event('gamepaddisconnected')); window.__pad = null; });
    await until(page, () => window.__sixfold.snapshot().view === 'pause');
    assert.match(await page.locator('#pause-reason').innerText(), /CONTROLLER DISCONNECTED/);
    return { device: 'synthetic standard navigator.getGamepads', movement: true, triggerEdge: true, heldResumeSafety: true, disconnectPause: true };
  });

  await check('Natural complete level through real wrapper using a scripted standard controller', async (page, context) => {
    // The engineer supplied the waypoint strategy. This independent browser traversal
    // supplies only normal controller values; it never writes game state or calls step.
    await context.addInitScript(() => {
      const memory = { point: 0, fired: false }, observed = new Set();
      window.__pilotEvidence = { stages: [], forkTellSeen: false, maxEnemies: 0, maxEscorts: 0 };
      const points = [[6, 3], [18, 3], [20, 10.5], [6, 11], [3, 7]];
      Object.defineProperty(navigator, 'getGamepads', { value: () => {
        const pad = { connected: true, mapping: 'standard', axes: [0, 0, 0, 0], buttons: Array.from({ length: 17 }, () => ({ value: 0, pressed: false })) };
        const state = window.__sixfold?.snapshot();
        if (!state || state.view !== 'play' || state.game.state !== 'playing') return [pad];
        const g = state.game, p = g.player;
        observed.add(g.stageName); window.__pilotEvidence.stages = [...observed];
        window.__pilotEvidence.forkTellSeen ||= g.enemies.some(e => e.type === 'foreman' && e.phase === 'tell' && e.pattern === 'fork');
        window.__pilotEvidence.maxEnemies = Math.max(window.__pilotEvidence.maxEnemies, g.enemies.length);
        window.__pilotEvidence.maxEscorts = Math.max(window.__pilotEvidence.maxEscorts, g.enemies.filter(e => e.escort).length);
        const targets = [...g.enemies].sort((a, b) => Math.hypot(a.x - p.x, a.y - p.y) - Math.hypot(b.x - p.x, b.y - p.y)), target = targets[0];
        let waypoint = points[memory.point % points.length];
        if (Math.hypot(waypoint[0] - p.x, waypoint[1] - p.y) < .7) waypoint = points[++memory.point % points.length];
        let mx = waypoint[0] - p.x, my = waypoint[1] - p.y, magnitude = Math.hypot(mx, my) || 1;
        mx /= magnitude; my /= magnitude;
        for (const enemy of targets) {
          const dx = p.x - enemy.x, dy = p.y - enemy.y, distance = Math.hypot(dx, dy) || .01, safety = enemy.type === 'foreman' ? 2.8 : 2.1;
          if (distance < safety) { mx += dx / distance * (safety - distance) * 2; my += dy / distance * (safety - distance) * 2; }
        }
        for (const shot of g.shots) {
          const dx = p.x - shot.x, dy = p.y - shot.y, time = Math.max(0, Math.min(.45, (dx * shot.vx + dy * shot.vy) / 49));
          const near = Math.hypot(dx - shot.vx * time, dy - shot.vy * time);
          if (near < 1.2 && time > 0) {
            const sign = (dx * -shot.vy + dy * shot.vx) >= 0 ? 1 : -1;
            mx += -shot.vy / 7 * sign * (1.2 - near) * 1.8; my += shot.vx / 7 * sign * (1.2 - near) * 1.8;
          }
        }
        magnitude = Math.max(1, Math.hypot(mx, my));
        const ax = target ? target.x - p.x : 1, ay = target ? target.y - p.y : 0, aimLength = Math.hypot(ax, ay) || 1;
        const fire = Boolean(target && g.heldCount >= 2 && !memory.fired && aimLength < 6.2);
        memory.fired = fire; pad.axes = [mx / magnitude, my / magnitude, ax / aimLength, ay / aimLength];
        pad.buttons[7] = { value: fire ? 1 : 0, pressed: fire }; pad.buttons[6] = { value: fire ? 0 : 1, pressed: !fire };
        return [pad];
      } });
    });
    await load(page); await start(page);
    await until(page, () => window.__sixfold.snapshot().view === 'results', undefined, 90000);
    const s = await snap(page), traversal = await page.evaluate(() => window.__pilotEvidence);
    assert.equal(s.game.state, 'won', `scripted controller did not clear: ${JSON.stringify(s.game.stats)}`);
    assert.equal(s.game.stats.stagesCleared, 3); assert.equal(s.game.heldCount, 6);
    assert.deepEqual(traversal.stages, ['Opening', 'Crossfire', 'Pressure', 'Foreman']);
    assert.ok(traversal.forkTellSeen, 'Foreman fork pattern exercised'); assert.ok(traversal.maxEnemies <= 6); assert.ok(traversal.maxEscorts <= 2);
    assert.equal(s.game.enemies.length, 0); assert.equal(s.game.shots.length, 0); assert.equal(s.game.spawns.length, 0);
    assert.match(await page.locator('#result-title').innerText(), /Line cleared/);
    return { kind: 'read-only-state-driven virtual standard controller, real-time browser loop', activeSeconds: s.game.time, hull: s.game.player.hull, stats: s.game.stats, ...traversal };
  });
})().catch(error => { report.fatal = error.stack; }).finally(async () => {
  if (browser) await browser.close();
  if (server) await new Promise(resolve => server.close(resolve));
  console.log(JSON.stringify(report, null, 2));
  if (report.fatal || report.checks.some(c => c.status === 'failed')) process.exitCode = 1;
});
