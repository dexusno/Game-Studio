'use strict';

const test = require('node:test');
const assert = require('node:assert/strict');
const { Game, CONFIG } = require('../src/core.js');
const DT = CONFIG.fixedStep;

function ticks(game, count, input = {}) {
  for (let i = 0; i < count; i++) game.step(DT, typeof input === 'function' ? input(game, i) : input);
}
function fixture(options = {}) {
  const game = new Game({ seed: 10, ...options });
  game.spawns = [];
  game._entryQueue = [];
  game._progress = () => {};
  game.phase = 'combat';
  game.drainEvents();
  return game;
}
function enemy(game, x, y, options = {}) {
  const item = { id: game._nextId++, type: 'spitter', x, y, px: x, py: y,
    r: 0.3, hp: 4, maxHp: 4, phase: 'tell', aim: { x: -1, y: 0 },
    tell: 1000, tellDuration: 2000, locked: true, flash: 0,
    pattern: 'pin', recovery: 0, nextContactAt: 0, escort: false, _attacks: 0, ...options };
  game.enemies.push(item);
  return item;
}
function allLoose(game) {
  for (const piece of game.pieces) Object.assign(piece, {
    state: 'loose', x: 2, y: 2, px: 2, py: 2, vx: 0, vy: 0,
    returnArmed: false, returnSpent: false, _looseStep: -1
  });
}
function oneHeld(game) { allLoose(game); game.pieces[0].state = 'held'; return game.pieces[0]; }
function shot(game, x, y, vx = -7, vy = 0) {
  const item = { id: game._nextId++, x, y, px: x, py: y, r: 0.15,
    vx, vy, life: 4, enemyId: 9999, _dead: false };
  game.shots.push(item);
  return item;
}
function conserved(game) {
  assert.equal(game.pieces.length, 6);
  assert.deepEqual(game.pieces.map(p => p.id).sort(), [0, 1, 2, 3, 4, 5]);
  assert.ok(game.pieces.every(p => ['held', 'outbound', 'ejected', 'loose', 'returning'].includes(p.state)));
  assert.ok(game.pieces.every(p => Number.isFinite(p.x) && Number.isFinite(p.y)));
  assert.equal(game.heldCount, game.pieces.filter(p => p.state === 'held').length);
}

test('movement is normalized, bounded, independent of aim, and ignores invalid dt', () => {
  const game = fixture();
  game.step(0.1, { moveX: 1, moveY: 1, aimX: 0, aimY: -1 });
  assert.ok(Math.abs(Math.hypot(game.player.x - 6, game.player.y - 7) - 0.5) < 1e-7);
  assert.deepEqual(game.player.aim, { x: 0, y: -1 });
  ticks(game, 1000, { moveX: -1, moveY: -1 });
  assert.equal(game.player.x, 0.5);
  assert.equal(game.player.y, 0.5);
  assert.deepEqual(game.player.aim, { x: 0, y: -1 });
  const before = game.snapshot();
  for (const dt of [NaN, Infinity, -1, 0]) game.step(dt, { moveX: 1 });
  assert.deepEqual(game.snapshot(), before);
});

test('full and partial fans use the same seven-degree spacing', () => {
  for (const count of [1, 2, 3, 6]) {
    const game = fixture();
    allLoose(game);
    game.pieces.slice(0, count).forEach(p => { p.state = 'held'; });
    game.step(DT, { firePressed: true, aimX: 1, aimY: 0 });
    const flying = game.pieces.filter(p => p.state === 'outbound');
    assert.equal(flying.length, count);
    flying.forEach((p, i) => {
      assert.ok(Math.abs(p.angle * 180 / Math.PI - (i - (count - 1) / 2) * 7) < 1e-8);
    });
    assert.equal(game.stats.partialVolleys, count < 6 ? 1 : 0);
    conserved(game);
  }
});

test('landing cannot attach in the launch update, and a held fire input does not repeat', () => {
  const game = fixture();
  game.player.x = 23.5;
  game.step(DT, { firePressed: true, aimX: 1 });
  assert.equal(game.heldCount, 0);
  assert.ok(game.pieces.every(p => p.state === 'loose'));
  ticks(game, 20, { firePressed: true, aimX: 1 });
  assert.equal(game.heldCount, 6);
  assert.equal(game.stats.volleys, 1);
  game.step(DT, {});
  game.step(DT, { firePressed: true, aimX: 1 });
  assert.equal(game.stats.volleys, 2);
});

test('recall cannot reverse outbound flight and preserves all IDs through catches', () => {
  const game = fixture();
  game.step(DT, { firePressed: true, aimX: 1 });
  ticks(game, 40, { recallHeld: true });
  assert.ok(game.pieces.every(p => p.state === 'outbound'));
  ticks(game, 200, { recallHeld: true });
  assert.equal(game.heldCount, 6);
  assert.equal(game.stats.collects, 6);
  assert.ok(game.pieces.every(p => p.cycle === 1 && !p.returnArmed));
  assert.deepEqual(game.drainEvents().filter(e => e.type === 'collect').map(e => e.held), [1, 2, 3, 4, 5, 6]);
  conserved(game);
});

test('default simultaneous recall/throw is rejected without buffering', () => {
  const game = fixture();
  game.step(DT, { recallHeld: true, firePressed: true });
  assert.equal(game.heldCount, 6);
  assert.equal(game.stats.rejectedThrows, 1);
  assert.equal(game.recalling, true);
  ticks(game, 5, { recallHeld: false, firePressed: true });
  assert.equal(game.stats.volleys, 0);
  game.step(DT, {});
  game.step(DT, { firePressed: true });
  assert.equal(game.stats.volleys, 1);
});

test('alternate throw suspends held recall until release and fresh press', () => {
  const game = fixture({ interruptRecall: true });
  game.step(DT, { recallHeld: true, firePressed: true });
  assert.equal(game.stats.volleys, 1);
  assert.equal(game.recalling, false);
  ticks(game, 100, { recallHeld: true });
  assert.ok(game.pieces.every(p => p.state === 'loose'));
  assert.equal(game.recalling, false);
  game.step(DT, { recallHeld: false });
  game.step(DT, { recallHeld: true });
  assert.ok(game.pieces.every(p => p.state === 'returning'));
  game.step(DT, { recallHeld: true, firePressed: true });
  assert.equal(game.stats.emptyThrows, 1);
  assert.equal(game.recalling, true, 'an empty throw must not suspend recall');
});

test('pause freezes simulation and requires fresh action edges after resume', () => {
  const game = fixture({ interruptRecall: true });
  game.step(DT, { firePressed: true });
  ticks(game, 70, { recallHeld: true });
  game.setPaused(true);
  const before = game.snapshot();
  ticks(game, 600, { moveX: 1, firePressed: true, recallHeld: true });
  assert.deepEqual(game.snapshot(), before);
  const copy = game.snapshot();
  copy.pieces[0].x = 999;
  assert.notEqual(game.pieces[0].x, 999);
  game.setPaused(false);
  game.step(DT, { recallHeld: true, firePressed: true });
  assert.equal(game.recalling, false);
  assert.equal(game.stats.volleys, 1);
  game.step(DT, {});
  game.step(DT, { recallHeld: true });
  assert.equal(game.recalling, true);
  conserved(game);
});

test('outbound sweep hits the first live obstruction and does not tunnel', () => {
  const game = fixture();
  oneHeld(game);
  const front = enemy(game, 7.1, 7, { r: 0.1, hp: 1 });
  const behind = enemy(game, 7.6, 7, { r: 0.1, hp: 3 });
  game.step(0.1, { firePressed: true, aimX: 1 });
  assert.equal(front.hp, 0);
  assert.equal(behind.hp, 3);
  assert.equal(game.stats.outboundDamage, 1);
  assert.equal(game.pieces[0].state, 'loose');
  assert.ok(Math.abs(game.pieces[0].x - 6.9) < 1e-8);
});

test('harmless/cutting comparison differs only in return speed and eligible damage', () => {
  for (const cutting of [false, true]) {
    const game = fixture({ returnDamage: cutting });
    allLoose(game);
    const piece = game.pieces[0];
    Object.assign(piece, { x: 10, y: 7, cycle: 1, returnArmed: cutting });
    const first = enemy(game, 8.5, 7, { r: 0.2 });
    const second = enemy(game, 7.6, 7, { r: 0.2 });
    ticks(game, 90, { recallHeld: true });
    assert.equal(first.hp, cutting ? 3 : 4);
    assert.equal(second.hp, 4, 'a return has only one damage allowance');
    assert.equal(game.stats.returnHits, cutting ? 1 : 0);
    assert.equal(piece.state, 'held');
    assert.equal(piece.returnArmed, false);
  }
});

test('landing overlap and recall toggles cannot produce return damage', () => {
  const game = fixture({ returnDamage: true });
  allLoose(game);
  const piece = game.pieces[0];
  Object.assign(piece, { x: 9, y: 7, cycle: 1, returnArmed: true });
  const target = enemy(game, 9, 7, { r: 0.5 });
  ticks(game, 18, (_, i) => ({ recallHeld: i % 2 === 0 }));
  assert.equal(target.hp, 4);
  assert.equal(game.stats.returnHits, 0);
  ticks(game, 100, { recallHeld: true });
  assert.equal(target.hp, 4, 'pulling out of overlap is an exit, not a new crossing');
  assert.equal(piece.state, 'held');
});

test('contact-excluded enemy can be hit only after active separation and a new crossing', () => {
  const game = fixture({ returnDamage: true });
  allLoose(game);
  const piece = game.pieces[0];
  Object.assign(piece, { x: 9, y: 7, cycle: 1, returnArmed: true });
  const target = enemy(game, 9, 7, { r: 0.5 });
  ticks(game, 16, { recallHeld: true });
  assert.ok(piece.x < 8.05);
  assert.equal(target.hp, 4);
  game.player.x = 12;
  ticks(game, 70, { recallHeld: true });
  assert.equal(target.hp, 3);
  assert.equal(game.stats.returnHits, 1);
  assert.equal(piece.state, 'held');
});

test('a near-face outbound hit does not automatically hit the same target on return', () => {
  const game = fixture({ returnDamage: true });
  oneHeld(game);
  const target = enemy(game, 7.8, 7, { r: 0.55, hp: 2 });
  game.step(DT, { firePressed: true, aimX: 1 });
  ticks(game, 160, { recallHeld: true });
  assert.equal(target.hp, 1);
  assert.equal(game.stats.outboundDamage, 1);
  assert.equal(game.stats.returnHits, 0);
});

test('return crossing sweep includes enemy motion, and toggling cannot refresh a spent hit', () => {
  const game = fixture({ returnDamage: true });
  allLoose(game);
  const piece = game.pieces[0];
  Object.assign(piece, { state: 'returning', x: 10, y: 7, cycle: 1,
    returnArmed: true, returnTravel: 1 });
  const target = enemy(game, 9.97, 7.45, { r: 0.3 });
  piece._contacts[target.id] = { eligible: true, separatedAt: 0 };
  game._updateEnemies = () => {
    target.px = target.x; target.py = target.y;
    target.y = 7;
  };
  game.step(DT, { recallHeld: true });
  assert.equal(target.hp, 3);
  game.step(DT, { recallHeld: false });
  ticks(game, 15, (_, i) => ({ recallHeld: i % 2 === 0 }));
  assert.equal(target.hp, 3);
  assert.equal(piece.returnSpent, true);
});

test('blocking chooses a stable piece, disarms it, and enforces wall-truncated ejection', () => {
  const game = fixture({ returnDamage: true });
  game.player.x = 0.5;
  enemy(game, 1.1, 7, { type: 'biter', r: 0.45, hp: 20 });
  game.step(DT, { recallHeld: true });
  const ejected = game.pieces[0];
  assert.equal(game.stats.blocks, 1);
  assert.equal(ejected.state, 'ejected');
  assert.equal(ejected.returnArmed, false);
  assert.equal(ejected.x, 0.1);
  assert.equal(game.player.hull, 3);
  ticks(game, 29, { recallHeld: true });
  assert.equal(ejected.state, 'ejected');
  game.step(DT, { recallHeld: true });
  assert.equal(ejected.state, 'loose');
  game.step(DT, { recallHeld: true });
  assert.equal(ejected.state, 'held');
  assert.equal(game.stats.returnHits, 0);
  conserved(game);
});

test('simultaneous hostile hits consume shots but only one block crosses fresh grace', () => {
  const game = fixture();
  for (let i = 0; i < 6; i++) shot(game, 6, 7);
  game.step(DT, {});
  assert.equal(game.stats.blocks, 1);
  assert.equal(game.player.hull, 3);
  assert.equal(game.heldCount, 5);
  assert.equal(game.shots.length, 0);
  assert.ok(game.player.grace > 0.24);
});

test('hull grace cannot be extended by harmless shot overlap', () => {
  const game = fixture();
  allLoose(game);
  for (let i = 0; i < 132; i++) {
    shot(game, 6, 7);
    game.step(DT, {});
  }
  assert.equal(game.stats.hullHits, 2);
  assert.equal(game.player.hull, 1);
  assert.equal(game.shots.length, 0);
});

test('time-of-impact gives exact lethal ties to death, but earlier boss defeat cancels danger', () => {
  for (const earlier of [false, true]) {
    const game = fixture();
    game.stage = 3;
    game.player.hull = 1;
    allLoose(game);
    const boss = enemy(game, 9, 7, { type: 'foreman', r: 0.4, hp: 1 });
    Object.assign(game.pieces[0], { state: 'outbound', x: earlier ? 8.47 : 8.4, y: 7,
      vx: 14, vy: 0, range: 7, cycle: 1 });
    shot(game, 6.7, 7);
    game.step(DT, {});
    assert.equal(game.state, earlier ? 'won' : 'lost');
    assert.equal(game.player.hull, earlier ? 1 : 0);
    if (earlier) assert.equal(boss.hp, 0);
    assert.equal(game.spawns.length, 0);
    assert.equal(game.shots.length, 0);
  }
});

test('spawn warnings stay safe at entry, including a player occupying the warned position', () => {
  const game = new Game({ seed: 10 });
  const blocked = game.spawns[0];
  game.player.x = blocked.x; game.player.y = blocked.y;
  const old = { x: blocked.x, y: blocked.y };
  ticks(game, 108);
  assert.ok(game.spawns.includes(blocked));
  assert.ok(blocked.time > 0.89);
  assert.notDeepEqual({ x: blocked.x, y: blocked.y }, old);
  assert.ok(Math.hypot(blocked.x - game.player.x, blocked.y - game.player.y) >= 3);
  assert.ok(game.enemies.every(e => Math.hypot(e.x - game.player.x, e.y - game.player.y) >= 3));
});

test('shooter charge starts are staggered and a locked nonlethal hit does not cancel fire', () => {
  const game = fixture();
  allLoose(game);
  const first = enemy(game, 10, 5.5, { phase: 'approach', locked: false, tell: 0 });
  enemy(game, 10, 8.5, { phase: 'approach', locked: false, tell: 0 });
  ticks(game, 53);
  assert.equal(first.locked, true);
  const aim = { ...first.aim };
  const piece = game.pieces[0];
  Object.assign(piece, { state: 'outbound', x: first.x, y: first.y, vx: 14, vy: 0, range: 7 });
  game.player.y = 3;
  ticks(game, 70);
  const events = game.drainEvents();
  const charges = events.filter(e => e.type === 'warning' && e.kind === 'charge');
  assert.ok(charges.length >= 2);
  assert.ok(charges[1].time - charges[0].time >= 0.35 - 1e-7);
  assert.deepEqual(first.aim, aim);
  assert.ok(events.some(e => e.type === 'shoot' && e.enemyId === first.id));
});

test('waiting cannot advance the opening: pressure can cause a real loss and same-seed reset', () => {
  const game = new Game({ seed: 42 });
  const initial = game.snapshot();
  ticks(game, 2400);
  assert.equal(game.state, 'lost');
  assert.equal(game.stage, 0);
  assert.equal(game.stats.kills, 0);
  assert.equal(game.stats.blocks, 6);
  assert.equal(game.stats.hullHits, 3);
  game.reset();
  assert.deepEqual(game.snapshot(), initial);
});

// A controller is executed against ordinary simulation, with no health or collision overrides.
// It supplies reproducible completion evidence, not evidence of human enjoyment or difficulty.
function pilot(game, memory) {
  const p = game.player;
  const targets = [...game.enemies].sort((a, b) => Math.hypot(a.x - p.x, a.y - p.y) - Math.hypot(b.x - p.x, b.y - p.y));
  const target = targets[0];
  const points = [[6, 3], [18, 3], [20, 10.5], [6, 11], [3, 7]];
  let waypoint = points[memory.point % points.length];
  if (Math.hypot(waypoint[0] - p.x, waypoint[1] - p.y) < 0.7) waypoint = points[++memory.point % points.length];
  let mx = waypoint[0] - p.x, my = waypoint[1] - p.y;
  const magnitude = Math.hypot(mx, my) || 1;
  mx /= magnitude; my /= magnitude;
  for (const e of targets) {
    const dx = p.x - e.x, dy = p.y - e.y, d = Math.hypot(dx, dy) || 0.01;
    const danger = e.type === 'foreman' ? 2.8 : 2.1;
    if (d < danger) { mx += dx / d * (danger - d) * 2; my += dy / d * (danger - d) * 2; }
  }
  for (const s of game.shots) {
    const dx = p.x - s.x, dy = p.y - s.y;
    const time = Math.max(0, Math.min(0.45, (dx * s.vx + dy * s.vy) / 49));
    const nx = dx - s.vx * time, ny = dy - s.vy * time;
    const near = Math.hypot(nx, ny);
    if (near < 1.2 && time > 0) {
      const sign = (dx * -s.vy + dy * s.vx) >= 0 ? 1 : -1;
      mx += -s.vy / 7 * sign * (1.2 - near) * 1.8;
      my += s.vx / 7 * sign * (1.2 - near) * 1.8;
    }
  }
  let fire = false;
  if (target && game.heldCount >= 2 && !memory.fired
    && Math.hypot(target.x - p.x, target.y - p.y) < 6.2) fire = true;
  memory.fired = fire;
  return { moveX: mx, moveY: my, aimX: target ? target.x - p.x : 1,
    aimY: target ? target.y - p.y : 0, firePressed: fire, recallHeld: !fire };
}

test('actual finite rosters and boss complete under a reproducible movement/aim controller', () => {
  for (const returnDamage of [false, true]) {
    const game = new Game({ seed: 42, returnDamage });
    const memory = { point: 0, fired: false };
    let maxEscorts = 0, escortEntries = 0, sawFork = false;
    for (let frame = 0; frame < 120 * 240 && game.state === 'playing'; frame++) {
      game.step(DT, pilot(game, memory));
      maxEscorts = Math.max(maxEscorts, game.enemies.filter(e => e.escort).length);
      for (const event of game.drainEvents()) {
        if (event.type === 'warning' && event.kind === 'entry' && game.stage === 3 && event.enemyType === 'biter') escortEntries++;
        if (event.type === 'shoot' && event.pattern === 'fork') sawFork = true;
      }
      assert.ok(game.enemies.length <= 6);
      conserved(game);
    }
    assert.equal(game.state, 'won', JSON.stringify({ cutting: returnDamage, stage: game.stage,
      time: game.time, hull: game.player.hull, stats: game.stats }));
    assert.equal(game.stats.stagesCleared, 3);
    assert.ok(game.stats.kills >= 17 && game.stats.kills <= 21);
    assert.ok(maxEscorts <= 2);
    assert.ok(escortEntries <= 4);
    assert.equal(game.heldCount, 6);
    assert.equal(game.enemies.length, 0);
    assert.equal(game.shots.length, 0);
    assert.equal(game.spawns.length, 0);
    assert.ok(sawFork, 'the controller exercises the learned fork pattern');
    console.log(`completion ${returnDamage ? 'cutting' : 'harmless'}: ${game.time.toFixed(2)}s, hull ${game.player.hull}, kills ${game.stats.kills}, blocks ${game.stats.blocks}, return hits ${game.stats.returnHits}`);
  }
});

test('same-seed input replay is deterministic and conservation survives mixed input/focus resets', () => {
  const first = new Game({ seed: 801, returnDamage: true, interruptRecall: true });
  const second = new Game({ seed: 801, returnDamage: true, interruptRecall: true });
  for (let frame = 0; frame < 1800; frame++) {
    const input = { moveX: Math.sin(frame * 0.018), moveY: Math.cos(frame * 0.021),
      aimX: Math.cos(frame * 0.023), aimY: Math.sin(frame * 0.023),
      firePressed: frame % 73 === 0, recallHeld: frame % 191 > 23 };
    if (frame % 317 === 0) { first.clearInput(); second.clearInput(); }
    first.step(DT, input); second.step(DT, input);
    conserved(first); conserved(second);
  }
  assert.deepEqual(first.snapshot(), second.snapshot());
});
