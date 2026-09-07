(function () {
  'use strict';
  const $ = id => document.getElementById(id);
  const QA = new URLSearchParams(location.search).get('qa') === '1';
  const KEY = 'sixfold-recoil-beta-v1';
  const defaults = { music: .30, sfx: .75, shake: true, interruptRecall: false, returnDamage: false };
  const clamp = (v, a, b) => Math.max(a, Math.min(b, v));
  const build = window.SIXFOLD_BUILD || { version: '0.1.0-beta', hash: 'development' };
  let storageAvailable = !QA, saved = { settings: { ...defaults }, records: {} };
  if (!QA) {
    try {
      const raw = localStorage.getItem(KEY), parsed = raw ? JSON.parse(raw) : null;
      if (parsed && parsed.schema === 1 && parsed.settings && typeof parsed.records === 'object' && parsed.records) {
        const s = parsed.settings;
        saved.settings = { music: Number.isFinite(s.music) ? clamp(s.music, 0, 1) : defaults.music, sfx: Number.isFinite(s.sfx) ? clamp(s.sfx, 0, 1) : defaults.sfx, shake: s.shake !== false, interruptRecall: s.interruptRecall === true, returnDamage: s.returnDamage === true };
        for (const [key, value] of Object.entries(parsed.records)) if (Number.isFinite(value) && value > 0) saved.records[key] = value;
      }
    } catch (_) { storageAvailable = false; }
  }
  const settings = saved.settings;
  const sound = new SixfoldAudio.Sound();
  const renderer = new SixfoldView.Renderer($('game'));
  let game = new SixfoldCore.Game({ seed: 604216, returnDamage: settings.returnDamage, interruptRecall: settings.interruptRecall });
  let activeOptions = {}, view = 'home', settingsOrigin = 'home', hasStarted = false, ending = 0;
  let resultSaved = false, lastResult = null, toastTime = 0, accumulated = 0, previousTime = performance.now(), elapsed = 0;
  let mouse = { x: 0, y: 0, known: false }, recallHeld = false, fireQueued = false;
  let inputKind = 'mouse', lastAim = { x: 1, y: 0 }, keys = new Set(), padPrevious = [], padSeen = false;
  let padFireDown = false, padRecallDown = false, blockPadFire = true, blockPadRecall = true, navDirection = 0, navNext = 0;
  const hints = new Set(), faults = [], frameStats = { frames: 0, steps: 0, maxFrameDelta: 0 };
  const screens = { home: $('home'), pause: $('pause-screen'), settings: $('settings-screen'), results: $('results-screen') };
  const setText = (el, value) => { if (el.textContent !== String(value)) el.textContent = value; };
  const formatTime = value => { const seconds = Math.max(0, Math.floor(value)); return `${Math.floor(seconds / 60)}:${String(seconds % 60).padStart(2, '0')}`; };
  function save() {
    if (QA) return;
    try { localStorage.setItem(KEY, JSON.stringify({ schema: 1, settings, records: saved.records })); storageAvailable = true; }
    catch (_) { storageAvailable = false; }
    updateStorageNote();
  }
  function updateStorageNote() { setText($('storage-note'), storageAvailable ? 'Settings and records stay on this device.' : QA ? 'QA session: settings and records are not saved.' : 'Saving is unavailable here. You can still play normally.'); }
  function unlock() {
    try { const result = sound.unlock(); if (result?.catch) result.catch(() => {}); } catch (_) { /* Sound is optional. */ }
  }
  function volumes() { sound.setVolumes(QA ? { music: 0, sfx: 0 } : { music: settings.music, sfx: settings.sfx }); }
  function clearControls() {
    keys.clear(); recallHeld = false; fireQueued = false; accumulated = 0;
    blockPadFire = true; blockPadRecall = true; padFireDown = false; padRecallDown = false;
    if (game?.clearInput) game.clearInput();
  }
  function showToast(text, duration = 2.6) { setText($('toast'), text); $('toast').hidden = false; toastTime = duration; }
  function focusSoon(id) { requestAnimationFrame(() => { if (!$(id).closest('[hidden]')) $(id).focus({ preventScroll: true }); }); }
  function setView(next) {
    view = next;
    for (const [name, panel] of Object.entries(screens)) panel.hidden = name !== view;
    const combat = hasStarted && view !== 'home';
    $('stage-hud').hidden = !combat; $('clock').hidden = !combat; $('combat-hud').hidden = !combat; $('pause-button').hidden = view !== 'play';
    document.body.classList.toggle('in-play', view === 'play');
    if (view !== 'play') { $('toast').hidden = true; toastTime = 0; }
    game.setPaused(view === 'pause' || (view === 'settings' && settingsOrigin === 'pause'));
    clearControls();
    sound.setScene({ active: view === 'play' && game.state === 'playing', paused: view === 'pause' || view === 'settings' || document.hidden, recalling: false, danger: 0 });
  }
  function start(mode = settings.returnDamage) {
    unlock(); settings.returnDamage = !!mode; syncSettings(); save();
    activeOptions = { seed: 604216, returnDamage: !!mode, interruptRecall: settings.interruptRecall };
    game = new SixfoldCore.Game(activeOptions); hasStarted = true; resultSaved = false; lastResult = null; ending = 0;
    renderer.reset(); setView('play'); $('game').focus({ preventScroll: true }); previousTime = performance.now();
    lastAim = { x: 1, y: 0 };
    if (!hints.has('start')) { showToast('WASD to move · Click to throw · Hold right mouse to recall', 4.5); hints.add('start'); }
  }
  function pause(reason = 'TAKE A BREATHER') {
    if (view !== 'play' || game.state !== 'playing') return;
    setText($('pause-reason'), reason); setView('pause'); focusSoon('resume');
  }
  function resume() {
    if (view !== 'pause') return;
    unlock(); setView('play'); $('game').focus({ preventScroll: true }); previousTime = performance.now();
  }
  function mainMenu() {
    hasStarted = false; ending = 0; renderer.reset(); setView('home'); syncSettings(); focusSoon('start');
  }
  function openSettings() { settingsOrigin = view === 'home' ? 'home' : 'pause'; setView('settings'); focusSoon('effects-volume'); }
  function closeSettings() { save(); setView(settingsOrigin); focusSoon(settingsOrigin === 'home' ? 'home-settings' : 'resume'); }
  function syncSettings() {
    $('effects-volume').value = Math.round(settings.sfx * 100); $('music-volume').value = Math.round(settings.music * 100);
    setText($('effects-value'), `${Math.round(settings.sfx * 100)}%`); setText($('music-value'), `${Math.round(settings.music * 100)}%`);
    $('shake').checked = settings.shake; $('interrupt').checked = settings.interruptRecall;
    document.querySelector(`input[name="recall"][value="${settings.returnDamage ? 'cutting' : 'recover'}"]`).checked = true;
    volumes(); updateStorageNote();
  }
  function finish() {
    if (resultSaved) return;
    resultSaved = true;
    const won = game.state === 'won', mode = activeOptions.returnDamage ? 'Cut through' : 'Recover';
    const recordKey = `${build.version}|${activeOptions.seed}|${activeOptions.returnDamage ? 'cutting' : 'recover'}|${activeOptions.interruptRecall ? 'interrupt' : 'priority'}`;
    const previous = saved.records[recordKey], best = won && (!previous || game.time < previous);
    if (best) { saved.records[recordKey] = game.time; save(); }
    lastResult = { build, completedOn: new Date().toISOString(), outcome: game.state, activeSeconds: Math.round(game.time * 100) / 100, stage: game.stageName, hull: game.player.hull, options: { ...activeOptions }, stats: { ...game.stats }, technicalNote: 'Locally recorded actions; not a fun or preference assessment.' };
    setText($('result-eyebrow'), won ? 'OPEN TRAY · COMPLETE' : `OPEN TRAY · ${String(game.stageName).toUpperCase()}`);
    setText($('result-title'), won ? 'Line cleared.' : 'Loose screws.');
    setText($('result-message'), won ? 'Six pieces, all accounted for. Nice work, Pip.' : 'Pip is down. Your next try starts with all six.');
    setText($('result-time'), formatTime(game.time)); setText($('result-hull'), `${game.player.hull} / 3`); setText($('result-mode'), mode);
    setText($('best-note'), best ? 'Your best clear with these settings.' : previous ? `Best clear with these settings: ${formatTime(previous)}` : 'Find a rhythm. Make the next throw count.');
    setText($('switch-mode'), activeOptions.returnDamage ? 'Try harmless recall' : 'Try cutting recall');
    setView('results'); focusSoon('retry');
  }
  async function fullscreen() {
    try { if (document.fullscreenElement) await document.exitFullscreen(); else await document.documentElement.requestFullscreen(); }
    catch (_) { showToast('Fullscreen is unavailable in this browser view.'); }
  }
  function saveRun() {
    if (!lastResult) return;
    const blob = new Blob([JSON.stringify(lastResult, null, 2) + '\n'], { type: 'application/json' });
    const href = URL.createObjectURL(blob), anchor = document.createElement('a');
    anchor.href = href; anchor.download = `Sixfold-Recoil-run-${Date.now()}.json`; document.body.appendChild(anchor); anchor.click(); anchor.remove();
    setTimeout(() => URL.revokeObjectURL(href), 1000);
  }
  $('start').addEventListener('click', () => start()); $('resume').addEventListener('click', resume);
  $('pause-button').addEventListener('click', () => pause()); $('restart').addEventListener('click', () => start(activeOptions.returnDamage));
  $('quit').addEventListener('click', mainMenu); $('results-menu').addEventListener('click', mainMenu);
  $('home-settings').addEventListener('click', openSettings); $('pause-settings').addEventListener('click', openSettings); $('settings-back').addEventListener('click', closeSettings);
  $('retry').addEventListener('click', () => start(activeOptions.returnDamage)); $('switch-mode').addEventListener('click', () => start(!activeOptions.returnDamage));
  $('fullscreen').addEventListener('click', fullscreen); $('save-run').addEventListener('click', saveRun);
  for (const radio of document.querySelectorAll('input[name="recall"]')) radio.addEventListener('change', () => { settings.returnDamage = radio.value === 'cutting'; save(); });
  for (const [id, key] of [['effects-volume', 'sfx'], ['music-volume', 'music']]) $(id).addEventListener('input', () => { unlock(); settings[key] = Number($(id).value) / 100; syncSettings(); save(); });
  $('shake').addEventListener('change', () => { settings.shake = $('shake').checked; save(); });
  $('interrupt').addEventListener('change', () => { settings.interruptRecall = $('interrupt').checked; save(); });

  window.addEventListener('pointermove', e => {
    mouse = { x: e.clientX, y: e.clientY, known: true };
    if (e.movementX || e.movementY) inputKind = 'mouse';
    if (!(e.buttons & 2)) recallHeld = false;
  });
  // Mouse events preserve each button's edge during a chord. Pointer events
  // only emit pointerdown for the first pressed mouse button.
  $('game').addEventListener('mousedown', e => {
    if (view !== 'play' || game.state !== 'playing') return;
    unlock(); e.preventDefault(); mouse = { x: e.clientX, y: e.clientY, known: true }; inputKind = 'mouse';
    if (e.button === 0) fireQueued = true; if (e.button === 2) recallHeld = true;
  });
  window.addEventListener('mouseup', e => { if (e.button === 2) recallHeld = false; });
  $('game').addEventListener('contextmenu', e => e.preventDefault());
  window.addEventListener('keydown', e => {
    if (e.code === 'Escape' && !e.repeat) { e.preventDefault(); if (view === 'play') pause(); else if (view === 'pause') resume(); else if (view === 'settings') closeSettings(); return; }
    if (e.code === 'KeyF' && !e.repeat && (view === 'home' || view === 'play')) { e.preventDefault(); fullscreen(); return; }
    if (view !== 'play') return;
    if (['KeyW','KeyA','KeyS','KeyD','ArrowUp','ArrowDown','ArrowLeft','ArrowRight','Space'].includes(e.code)) { e.preventDefault(); if (!e.repeat) keys.add(e.code); }
  });
  window.addEventListener('keyup', e => keys.delete(e.code));
  window.addEventListener('blur', () => { clearControls(); if (view === 'play') pause('PAUSED WHEN FOCUS CHANGED'); else sound.setScene({ active: false, paused: true, recalling: false, danger: 0 }); });
  document.addEventListener('visibilitychange', () => { if (document.hidden) { clearControls(); if (view === 'play') pause('PAUSED WHILE YOU WERE AWAY'); sound.setScene({ active: false, paused: true, recalling: false, danger: 0 }); } });
  window.addEventListener('resize', () => renderer.resize());
  window.addEventListener('gamepaddisconnected', () => { if (padSeen && view === 'play' && inputKind === 'pad') pause('CONTROLLER DISCONNECTED'); padSeen = false; padPrevious = []; });
  window.addEventListener('error', e => { faults.push(String(e.message)); });
  window.addEventListener('unhandledrejection', e => { faults.push(String(e.reason)); });

  function focusable() { return [...document.querySelectorAll('.screen:not([hidden]) button, .screen:not([hidden]) input')].filter(el => !el.disabled && el.getClientRects().length); }
  function menuPad(pad, buttons) {
    const direction = buttons[13] || pad.axes[1] > .55 ? 1 : buttons[12] || pad.axes[1] < -.55 ? -1 : 0;
    if (direction && (direction !== navDirection || elapsed >= navNext)) {
      const items = focusable(), index = items.indexOf(document.activeElement);
      if (items.length) items[(index + direction + items.length) % items.length].focus({ preventScroll: false });
      navNext = elapsed + (direction !== navDirection ? .42 : .18);
    }
    navDirection = direction;
    if (buttons[0] && !padPrevious[0]) {
      const focused = document.activeElement;
      if (focused?.matches('button,input[type=checkbox],input[type=radio]') && focused.getClientRects().length) focused.click();
      else if (view === 'home') $('start').click();
    }
    if (buttons[1] && !padPrevious[1]) { if (view === 'settings') closeSettings(); else if (view === 'pause') resume(); else if (view === 'results') mainMenu(); }
    if (document.activeElement?.type === 'range') {
      const directionX = buttons[15] && !padPrevious[15] ? 1 : buttons[14] && !padPrevious[14] ? -1 : 0;
      if (directionX) { const el = document.activeElement; el.value = clamp(Number(el.value) + directionX * 5, 0, 100); el.dispatchEvent(new Event('input', { bubbles: true })); }
    }
  }
  function pollPad() {
    let pad = null;
    try { pad = [...(navigator.getGamepads?.() || [])].find(p => p && p.connected && p.mapping === 'standard'); } catch (_) { /* Keyboard/mouse still work. */ }
    const output = { moveX: 0, moveY: 0, aimX: 0, aimY: 0, recall: false };
    $('pad-status').hidden = !pad;
    if (!pad) { padSeen = false; return output; }
    padSeen = true;
    const buttons = pad.buttons.map(b => b.pressed || b.value > .55);
    if (buttons[9] && !padPrevious[9]) { if (view === 'play') pause(); else if (view === 'pause') resume(); }
    if (view !== 'play') { menuPad(pad, buttons); padPrevious = buttons; return output; }
    const axes = pad.axes;
    const mx = axes[0] || 0, my = axes[1] || 0, ax = axes[2] || 0, ay = axes[3] || 0;
    if (Math.hypot(mx, my) > .18) { output.moveX = mx; output.moveY = my; inputKind = 'pad'; }
    if (Math.hypot(ax, ay) > .22) { output.aimX = ax; output.aimY = ay; lastAim = { x: ax, y: ay }; inputKind = 'pad'; }
    const fv = pad.buttons[7]?.value || 0, rv = pad.buttons[6]?.value || 0;
    if (fv < .35) { padFireDown = false; blockPadFire = false; }
    if (rv < .35) { padRecallDown = false; blockPadRecall = false; }
    if (fv > .55 && !padFireDown && !blockPadFire) { fireQueued = true; inputKind = 'pad'; }
    if (fv > .55) padFireDown = true;
    if (rv > .55 && !blockPadRecall) { padRecallDown = true; inputKind = 'pad'; }
    output.recall = padRecallDown && !blockPadRecall;
    padPrevious = buttons; return output;
  }
  function processEvents() {
    for (const e of game.drainEvents()) {
      renderer.event(e); sound.event(e);
      if (e.type === 'rejected') showToast('Release recall, then click to throw.', 2);
      if (e.type === 'empty' && !hints.has('empty')) { showToast('Empty. Hold right mouse to pull your scrap home.'); hints.add('empty'); }
      if (e.type === 'hull' && !hints.has('hull')) { showToast('No pieces attached = no protection.'); hints.add('hull'); }
      if (e.type === 'stageClear') { showToast('Bay cleared. All six pieces recovered.', 1.25); }
      if (e.type === 'win' || e.type === 'lose') ending = .72;
    }
  }
  function updateHUD() {
    setText($('clock'), formatTime(game.time)); setText($('stage-label'), String(game.stageName).toUpperCase());
    [...$('stage-steps').children].forEach((el, i) => { el.className = i < game.stage ? 'done' : i === game.stage ? 'active' : ''; });
    setText($('held-count'), game.heldCount);
    setText($('protection-label'), game.heldCount === 0 ? 'EXPOSED · KEEP MOVING' : game.recalling ? 'RECALLING' : 'PROTECTION READY');
    document.body.classList.toggle('exposed', game.heldCount === 0 && hasStarted);
    [...$('piece-marks').children].forEach((el, i) => { el.className = game.pieces[i]?.state || ''; });
    [...$('hull-pips').children].forEach((el, i) => { el.className = i < game.player.hull ? '' : 'empty'; });
    $('hull-pips').setAttribute('aria-label', `Hull ${game.player.hull} of 3`);
  }
  function frame(now) {
    const rawDelta = Math.max(0, (now - previousTime) / 1000), dt = Math.min(rawDelta, .075);
    previousTime = now; elapsed += dt; frameStats.frames++; frameStats.maxFrameDelta = Math.max(frameStats.maxFrameDelta, rawDelta);
    const pad = pollPad();
    if (view === 'play' && game.state === 'playing') {
      const moveX = (keys.has('KeyD') || keys.has('ArrowRight') ? 1 : 0) - (keys.has('KeyA') || keys.has('ArrowLeft') ? 1 : 0) + pad.moveX;
      const moveY = (keys.has('KeyS') || keys.has('ArrowDown') ? 1 : 0) - (keys.has('KeyW') || keys.has('ArrowUp') ? 1 : 0) + pad.moveY;
      if (inputKind === 'mouse' && mouse.known) { const point = renderer.worldPoint(mouse.x, mouse.y); if (Math.hypot(point.x - game.player.x, point.y - game.player.y) > .03) lastAim = { x: point.x - game.player.x, y: point.y - game.player.y }; }
      accumulated += dt;
      const STEP = 1 / 120;
      while (accumulated + 1e-9 >= STEP && game.state === 'playing') {
        game.step(STEP, { moveX, moveY, aimX: lastAim.x, aimY: lastAim.y, firePressed: fireQueued, recallHeld: recallHeld || pad.recall });
        fireQueued = false; accumulated -= STEP; frameStats.steps++;
      }
      processEvents();
    } else accumulated = 0;
    if (view === 'play' && game.state !== 'playing') { ending -= dt; if (ending <= 0) finish(); }
    if (toastTime > 0 && view === 'play') { toastTime -= dt; if (toastTime <= 0) $('toast').hidden = true; }
    updateHUD();
    const danger = clamp(game.enemies.length / 7 + (game.player.hull === 1 ? .2 : 0), 0, 1);
    sound.setScene({ active: view === 'play' && game.state === 'playing', paused: view === 'pause' || view === 'settings' || document.hidden, recalling: view === 'play' && game.recalling, danger });
    renderer.render(game, { home: view === 'home' || (view === 'settings' && settingsOrigin === 'home'), elapsed, dt, shake: settings.shake, freeze: view === 'pause' || view === 'settings' });
    requestAnimationFrame(frame);
  }
  window.__sixfold = {
    snapshot: () => ({ build, view, qa: QA, settings: { ...settings }, activeOptions: { ...activeOptions }, game: game.snapshot(), faults: [...faults], frameStats: { ...frameStats }, storageAvailable }),
    get renderer() { return renderer; },
    ...(QA ? { start, pause, resume } : {})
  };
  if (QA) Object.defineProperty(window.__sixfold, 'game', { get: () => game });
  syncSettings(); setView('home'); setText($('build-label'), `BETA ${build.version.replace('-beta', '')} · LOCAL PLAYTEST`);
  requestAnimationFrame(frame);
})();
