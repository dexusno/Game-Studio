/* Sixfold Recoil. Original simulation; no runtime dependencies or DOM access. */
(function (root, factory) {
  'use strict';
  const api = factory();
  if (typeof module === 'object' && module.exports) module.exports = api;
  if (root) root.SixfoldCore = api;
})(typeof globalThis !== 'undefined' ? globalThis : this, function () {
  'use strict';

  const CONFIG = Object.freeze({
    version: '0.1.0-beta', width: 24, height: 14, fixedStep: 1 / 120,
    maxFrame: 0.25, pieceCount: 6, playerRadius: 0.5, playerSpeed: 5,
    hull: 3, pieceRadius: 0.1, launchOffset: 0.65, fanDegrees: 35,
    throwSpeed: 14, throwRange: 7, pickupRadius: 0.8, attachRadius: 0.65,
    returnSpeed: 10, cuttingReturnSpeed: 7.5, returnMargin: 0.05,
    returnMinMotion: 0.05, ejectionSpeed: 8, ejectionRange: 2,
    ejectionDuration: 0.25, blockGrace: 0.25, hullGrace: 1,
    contactInterval: 0.8, biterSpeed: 2.5, biterRadius: 0.45,
    spitterSpeed: 2, spitterRadius: 0.55, spitterRange: 6.2,
    spitterTell: 0.8, spitterRecovery: 1.4, shotSpeed: 7, shotRadius: 0.15,
    shotLifetime: 4, shooterStagger: 0.35, rangedCap: 2, liveCap: 6,
    warningDuration: 0.9, safeSpawnDistance: 3, transitionDuration: 1.15,
    bossHull: 24, bossRadius: 1.25, bossRecovery: 1, forkDegrees: 18,
    forkTell: 0.9, escortTotal: 4, escortCap: 2
  });
  const EPS = 1e-8;
  const TAU = Math.PI * 2;
  const STAGES = ['Opening', 'Crossfire', 'Pressure', 'Foreman'];
  const TYPES = {
    biter: { r: CONFIG.biterRadius, hp: 2 },
    spitter: { r: CONFIG.spitterRadius, hp: 2 },
    foreman: { r: CONFIG.bossRadius, hp: CONFIG.bossHull }
  };
  // Explicit finite rosters. The second pack waits for a kill and reduced crowding.
  const PACKS = [
    [[['biter', 0], ['biter', 1]], [['biter', 2], ['biter', 3], ['spitter', 4]]],
    [[['biter', 5], ['biter', 6], ['spitter', 7]], [['biter', 8], ['spitter', 9]]],
    [[['biter', 9], ['biter', 10], ['spitter', 11]], [['biter', 4], ['biter', 7], ['spitter', 6]]]
  ];
  const ENTRIES = [
    [18, 4.5], [18, 9.5], [21.8, 3], [21.8, 11], [18, 2], [3.2, 2],
    [3.2, 12], [20.8, 7], [12, 2], [12, 12], [2, 7], [20, 11.5]
  ];

  const clamp = (v, lo, hi) => Math.max(lo, Math.min(hi, v));
  const lerp = (a, b, t) => a + (b - a) * t;
  const length = (x, y) => Math.hypot(x, y);
  function direction(x, y, fallbackX = 1, fallbackY = 0) {
    const d = length(x, y);
    return d > EPS && Number.isFinite(d) ? { x: x / d, y: y / d } : { x: fallbackX, y: fallbackY };
  }
  function at(body, t) { return { x: lerp(body.px, body.x, t), y: lerp(body.py, body.y, t) }; }
  function rotate(v, angle) {
    const c = Math.cos(angle), s = Math.sin(angle);
    return { x: v.x * c - v.y * s, y: v.x * s + v.y * c };
  }
  function keepInside(body) {
    body.x = clamp(body.x, body.r, CONFIG.width - body.r);
    body.y = clamp(body.y, body.r, CONFIG.height - body.r);
  }
  // Relative circle sweep returns the full contact interval, including initial overlap.
  function sweep(ax, ay, bx, by, cx, cy, dx, dy, radius) {
    const x = ax - cx, y = ay - cy;
    const vx = (bx - ax) - (dx - cx), vy = (by - ay) - (dy - cy);
    const a = vx * vx + vy * vy, c = x * x + y * y - radius * radius;
    if (a <= EPS * EPS) return c <= EPS ? { enter: 0, exit: 1 } : null;
    const b = x * vx + y * vy, disc = b * b - a * c;
    if (disc < -EPS) return null;
    const q = Math.sqrt(Math.max(0, disc));
    const enter = (-b - q) / a, exit = (-b + q) / a;
    if (exit < -EPS || enter > 1 + EPS) return null;
    return { enter: clamp(enter, 0, 1), exit: clamp(exit, 0, 1) };
  }
  function wallDistance(body, dx, dy, distance) {
    let result = distance;
    if (dx > EPS) result = Math.min(result, (CONFIG.width - body.r - body.x) / dx);
    if (dx < -EPS) result = Math.min(result, (body.r - body.x) / dx);
    if (dy > EPS) result = Math.min(result, (CONFIG.height - body.r - body.y) / dy);
    if (dy < -EPS) result = Math.min(result, (body.r - body.y) / dy);
    return Math.max(0, result);
  }
  function cleanObject(object) {
    const result = {};
    for (const key of Object.keys(object)) {
      if (key[0] !== '_') result[key] = object[key] && typeof object[key] === 'object'
        ? JSON.parse(JSON.stringify(object[key])) : object[key];
    }
    return result;
  }

  class Game {
    constructor(options = {}) { this.reset(options); }

    reset(options = {}) {
      const old = this.options || {};
      this.options = {
        returnDamage: Boolean(options.returnDamage ?? old.returnDamage ?? false),
        interruptRecall: Boolean(options.interruptRecall ?? old.interruptRecall ?? false),
        seed: (Number(options.seed ?? old.seed ?? 0x51f01d) >>> 0)
      };
      this.state = 'playing';
      this.paused = false;
      this.time = 0;
      this.stage = 0;
      this.stageName = STAGES[0];
      this.phase = 'warning';
      this.recalling = false;
      this.player = { x: 6, y: 7, px: 6, py: 7, r: CONFIG.playerRadius,
        hull: CONFIG.hull, maxHull: CONFIG.hull, grace: 0, aim: { x: 1, y: 0 }, flash: 0 };
      this.pieces = Array.from({ length: CONFIG.pieceCount }, (_, id) => ({
        id, state: 'held', x: 6, y: 7, px: 6, py: 7, vx: 0, vy: 0,
        r: CONFIG.pieceRadius, angle: id * TAU / CONFIG.pieceCount,
        cycle: 0, returnArmed: false, returnSpent: false, returnTravel: 0,
        range: 0, ejectTime: 0, _looseStep: -1, _contacts: Object.create(null)
      }));
      this.enemies = [];
      this.shots = [];
      this.spawns = [];
      this.stats = { volleys: 0, partialVolleys: 0, piecesLaunched: 0, kills: 0,
        blocks: 0, hullHits: 0, outboundDamage: 0, returnDamage: 0,
        collects: 0, rejectedThrows: 0, emptyThrows: 0, returnHits: 0,
        movingReturnHits: 0, stagesCleared: 0 };
      this._events = [];
      this._entryQueue = [];
      this._stepId = 0;
      this._nextId = 1;
      this._graceUntil = 0;
      this._lastTellAt = -Infinity;
      this._fireDown = false;
      this._requireFireRelease = false;
      this._requireRecallRelease = false;
      this._recallSuspended = false;
      this._mirrorY = (this.options.seed & 1) !== 0;
      this._bossDefeatedAt = null;
      this._beginStage(0);
      this._placeHeld();
      return this;
    }

    get heldCount() { return this.pieces.reduce((n, p) => n + (p.state === 'held'), 0); }

    setPaused(value) {
      value = Boolean(value);
      if (value !== this.paused) {
        this.paused = value;
        this.clearInput();
      }
    }

    clearInput() {
      this.recalling = false;
      this._recallSuspended = false;
      this._fireDown = false;
      this._requireFireRelease = true;
      this._requireRecallRelease = true;
      for (const piece of this.pieces) {
        if (piece.state === 'returning') this._land(piece, false);
      }
    }

    drainEvents() {
      const events = this._events;
      this._events = [];
      return events;
    }

    snapshot() {
      return { version: CONFIG.version, options: { ...this.options }, state: this.state,
        paused: this.paused, time: this.time, stage: this.stage, stageName: this.stageName,
        phase: this.phase, recalling: this.recalling, heldCount: this.heldCount,
        player: cleanObject(this.player), pieces: this.pieces.map(cleanObject),
        enemies: this.enemies.map(cleanObject), shots: this.shots.map(cleanObject),
        spawns: this.spawns.map(cleanObject), stats: { ...this.stats } };
    }

    step(dt, input = {}) {
      if (this.paused || this.state !== 'playing' || !Number.isFinite(dt) || dt <= 0) return;
      // The wrapper supplies fixed steps. Bound exceptional frame gaps and subdivide them.
      let remaining = Math.min(dt, CONFIG.maxFrame), first = true;
      while (remaining > EPS && this.state === 'playing') {
        const slice = Math.min(remaining, CONFIG.fixedStep);
        this._tick(slice, input, first);
        remaining -= slice;
        first = false;
      }
    }

    _emit(type, data = {}) { this._events.push({ type, time: this.time, ...data }); }

    _tick(dt, input, processFire = true) {
      this._stepId++;
      const startTime = this.time;
      const player = this.player;
      player.px = player.x; player.py = player.y;
      player.flash = Math.max(0, player.flash - dt);
      const aim = direction(Number(input.aimX) || 0, Number(input.aimY) || 0, player.aim.x, player.aim.y);
      player.aim = aim;
      this._safeguardPieces();
      if (this.phase !== 'transition') this._actions(input, processFire);
      else if (processFire) this._fireDown = Boolean(input.firePressed);
      let mx = Number(input.moveX) || 0, my = Number(input.moveY) || 0;
      const magnitude = length(mx, my);
      if (!Number.isFinite(magnitude)) { mx = 0; my = 0; }
      else if (magnitude > 1) { mx /= magnitude; my /= magnitude; }
      player.x += mx * CONFIG.playerSpeed * dt;
      player.y += my * CONFIG.playerSpeed * dt;
      keepInside(player);
      this._playerMoving = length(player.x - player.px, player.y - player.py) > EPS;
      if (this.phase === 'transition') {
        this._transitionLeft -= dt;
        this._placeHeld();
        if (this._transitionLeft <= EPS) this._beginStage(this.stage + 1);
        return;
      }
      this.time += dt;
      const contacts = [];
      this._updateEnemies(dt, startTime, contacts);
      this._updatePieces(dt, contacts);
      this._updateShots(dt, contacts);
      this._bodyContacts(dt, startTime, contacts);
      contacts.sort((a, b) => Math.abs(a.t - b.t) > EPS ? a.t - b.t
        : a.priority - b.priority || (a.piece?.id ?? a.shot?.id ?? a.enemy?.id ?? 0)
          - (b.piece?.id ?? b.shot?.id ?? b.enemy?.id ?? 0) || (a.enemy?.id ?? 0) - (b.enemy?.id ?? 0));
      for (const contact of contacts) {
        if (this.state !== 'playing') break;
        if (this._bossDefeatedAt !== null && contact.t > this._bossDefeatedAt + EPS) break;
        this._resolve(contact, startTime + contact.t * dt);
      }
      this.shots = this.shots.filter(s => !s._dead);
      this.enemies = this.enemies.filter(e => e.hp > 0);
      player.grace = Math.max(0, this._graceUntil - this.time);
      this._placeHeld();
      if (this.state !== 'playing') return;
      if (this._bossDefeatedAt !== null) { this._finish('won'); return; }
      this._updateSpawns(dt);
      this._progress();
    }

    _actions(input, processFire) {
      const rawRecall = Boolean(input.recallHeld), rawFire = Boolean(input.firePressed);
      if (!rawRecall) { this._requireRecallRelease = false; this._recallSuspended = false; }
      if (!rawFire) this._requireFireRelease = false;
      let recall = rawRecall && !this._requireRecallRelease && !this._recallSuspended;
      const fire = processFire && rawFire && !this._fireDown && !this._requireFireRelease;
      if (processFire) this._fireDown = rawFire;
      if (fire) {
        if (recall && !this.options.interruptRecall) {
          this.stats.rejectedThrows++;
          this._emit('rejected', { x: this.player.x, y: this.player.y, reason: 'recall' });
        } else if (!this.heldCount) {
          this.stats.emptyThrows++;
          this._emit('empty', { x: this.player.x, y: this.player.y });
        } else {
          if (recall && this.options.interruptRecall) { recall = false; this._recallSuspended = true; }
          this._launch();
        }
      }
      if (recall && !this.recalling) this._emit('recallStart', { x: this.player.x, y: this.player.y });
      this.recalling = recall;
      for (const piece of this.pieces) {
        if (!recall && piece.state === 'returning') this._land(piece, false);
        if (recall && piece.state === 'loose' && piece._looseStep < this._stepId) {
          piece.state = 'returning';
          this._startReturn(piece);
        }
      }
    }

    _launch() {
      const held = this.pieces.filter(piece => piece.state === 'held');
      this.stats.volleys++;
      if (held.length < CONFIG.pieceCount) this.stats.partialVolleys++;
      this.stats.piecesLaunched += held.length;
      held.forEach((piece, index) => {
        const angle = (index - (held.length - 1) / 2) * CONFIG.fanDegrees / 5 * Math.PI / 180;
        const dir = rotate(this.player.aim, angle);
        piece.x = this.player.x + dir.x * CONFIG.launchOffset;
        piece.y = this.player.y + dir.y * CONFIG.launchOffset;
        keepInside(piece);
        piece.px = piece.x; piece.py = piece.y;
        piece.vx = dir.x * CONFIG.throwSpeed; piece.vy = dir.y * CONFIG.throwSpeed;
        piece.angle = Math.atan2(dir.y, dir.x);
        piece.state = 'outbound'; piece.range = CONFIG.throwRange;
        piece.cycle++; piece.returnArmed = this.options.returnDamage;
        piece.returnSpent = false; piece.returnTravel = 0;
        piece._contacts = Object.create(null);
      });
      this._emit('launch', { x: this.player.x, y: this.player.y, count: held.length,
        held: 0, aim: { ...this.player.aim } });
    }

    _startReturn(piece) {
      for (const enemy of this.enemies) {
        if (enemy.hp <= 0) continue;
        const touching = length(piece.x - enemy.x, piece.y - enemy.y)
          <= piece.r + enemy.r + CONFIG.returnMargin + EPS;
        if (touching) piece._contacts[enemy.id] = { eligible: false, separatedAt: piece.returnTravel };
        else if (!piece._contacts[enemy.id])
          piece._contacts[enemy.id] = { eligible: true, separatedAt: piece.returnTravel };
      }
    }

    _updatePieces(dt, contacts) {
      for (const piece of this.pieces) {
        piece.px = piece.x; piece.py = piece.y;
        if (piece.state === 'held') continue;
        if (piece.state === 'loose') {
          if (piece._looseStep >= this._stepId) continue;
          const hit = sweep(piece.x, piece.y, piece.x, piece.y, this.player.px, this.player.py,
            this.player.x, this.player.y, CONFIG.pickupRadius);
          if (hit) contacts.push({ type: 'collect', t: hit.enter, priority: 4, piece });
          continue;
        }
        if (piece.state === 'ejected') {
          const dir = direction(piece.vx, piece.vy);
          const distance = wallDistance(piece, dir.x, dir.y, Math.min(piece.range, CONFIG.ejectionSpeed * dt));
          piece.x += dir.x * distance; piece.y += dir.y * distance;
          piece.range = Math.max(0, piece.range - distance);
          piece.ejectTime -= dt;
          if (piece.ejectTime <= EPS) this._land(piece);
          continue;
        }
        if (piece.state === 'outbound') {
          const dir = direction(piece.vx, piece.vy);
          const desired = Math.min(piece.range, CONFIG.throwSpeed * dt);
          const distance = wallDistance(piece, dir.x, dir.y, desired);
          const endT = clamp(distance / (CONFIG.throwSpeed * dt), 0, 1);
          piece.x += dir.x * distance; piece.y += dir.y * distance;
          piece.range = Math.max(0, piece.range - distance);
          this._pieceEnemySweeps(piece, endT, contacts, false);
          if (piece.range <= EPS || distance < desired - EPS || endT < 1 - EPS)
            contacts.push({ type: 'land', t: endT, priority: 3, piece });
          continue;
        }
        if (piece.state === 'returning') {
          const dir = direction(this.player.x - piece.x, this.player.y - piece.y);
          const gap = length(this.player.x - piece.x, this.player.y - piece.y);
          const speed = this.options.returnDamage ? CONFIG.cuttingReturnSpeed : CONFIG.returnSpeed;
          const distance = Math.min(speed * dt, Math.max(0, gap - CONFIG.attachRadius));
          const endT = clamp(distance / (speed * dt), 0, 1);
          piece.vx = dir.x * speed; piece.vy = dir.y * speed;
          piece.angle = Math.atan2(dir.y, dir.x);
          piece.x += dir.x * distance; piece.y += dir.y * distance;
          this._pieceEnemySweeps(piece, endT, contacts, true, distance);
          piece.returnTravel += distance;
          if (gap <= CONFIG.attachRadius + speed * dt + EPS)
            contacts.push({ type: 'collect', t: endT, priority: 4, piece });
        }
      }
    }

    _pieceEnemySweeps(piece, endT, contacts, returning, distance = 0) {
      for (const enemy of this.enemies) {
        if (enemy.hp <= 0) continue;
        const end = at(enemy, endT), radius = piece.r + enemy.r;
        let record;
        if (returning) {
          if (!piece.returnArmed || piece.returnSpent) continue;
          record = piece._contacts[enemy.id];
          if (!record) record = piece._contacts[enemy.id] = {
            eligible: length(piece.px - enemy.px, piece.py - enemy.py) > radius + CONFIG.returnMargin,
            separatedAt: piece.returnTravel
          };
        }
        const hit = sweep(piece.px, piece.py, piece.x, piece.y, enemy.px, enemy.py, end.x, end.y, radius);
        if (hit && (!returning || (record.eligible
          && length(piece.px - enemy.px, piece.py - enemy.py) > radius + EPS
          && piece.returnTravel + distance * hit.enter - record.separatedAt >= CONFIG.returnMinMotion - EPS))) {
          contacts.push({ type: 'enemyHit', t: hit.enter * endT, priority: 1, piece, enemy,
            returnHit: returning, x: lerp(piece.px, piece.x, hit.enter), y: lerp(piece.py, piece.y, hit.enter) });
        }
        if (returning && !record.eligible && endT >= 1 - EPS) {
          const expanded = sweep(piece.px, piece.py, piece.x, piece.y,
            enemy.px, enemy.py, enemy.x, enemy.y, radius + CONFIG.returnMargin);
          if (!expanded) {
            // Eligibility starts next step, after a complete active step outside the margin.
            record.eligible = true;
            record.separatedAt = piece.returnTravel + distance;
          }
        }
      }
    }

    _updateEnemies(dt, startTime, contacts) {
      for (const enemy of this.enemies) {
        enemy.px = enemy.x; enemy.py = enemy.y;
        enemy.flash = Math.max(0, enemy.flash - dt);
      }
      for (const enemy of this.enemies) {
        if (enemy.hp <= 0) continue;
        if (enemy.type === 'biter') {
          let move = direction(this.player.px - enemy.px, this.player.py - enemy.py);
          for (const other of this.enemies) {
            if (other.id === enemy.id || other.hp <= 0) continue;
            const dx = enemy.px - other.px, dy = enemy.py - other.py;
            const distance = length(dx, dy), separation = enemy.r + other.r + 0.22;
            if (distance < separation) {
              const away = direction(dx, dy, enemy.id < other.id ? -1 : 1, 0);
              const weight = (separation - distance) / separation * 1.6;
              move.x += away.x * weight; move.y += away.y * weight;
            }
          }
          move = direction(move.x, move.y);
          enemy.x += move.x * CONFIG.biterSpeed * dt;
          enemy.y += move.y * CONFIG.biterSpeed * dt;
          enemy.aim = move;
        } else if (enemy.phase === 'tell') {
          if (!enemy.locked) enemy.aim = direction(this.player.px - enemy.x, this.player.py - enemy.y);
          const before = enemy.tell;
          enemy.tell = Math.max(0, enemy.tell - dt);
          enemy.locked = enemy.tell <= enemy.tellDuration / 2 + EPS;
          if (enemy.tell <= EPS) {
            contacts.push({ type: 'shoot', t: clamp(before / dt, 0, 1), priority: 2, enemy });
            enemy.phase = 'recover';
            enemy.recovery = enemy.type === 'foreman' ? CONFIG.bossRecovery : CONFIG.spitterRecovery;
          }
        } else if (enemy.phase === 'recover') {
          enemy.recovery = Math.max(0, enemy.recovery - dt);
          enemy.locked = false;
          if (enemy.type === 'foreman') {
            const move = direction(16 - enemy.x, 7 - enemy.y, 0, 0);
            const distance = Math.min(length(16 - enemy.x, 7 - enemy.y), 0.9 * dt);
            enemy.x += move.x * distance; enemy.y += move.y * distance;
          }
          if (enemy.recovery <= EPS) enemy.phase = 'approach';
        } else {
          const distance = length(this.player.px - enemy.x, this.player.py - enemy.y);
          if (enemy.type === 'spitter' && distance > CONFIG.spitterRange) {
            const move = direction(this.player.px - enemy.x, this.player.py - enemy.y);
            const travel = Math.min(CONFIG.spitterSpeed * dt, distance - CONFIG.spitterRange);
            enemy.x += move.x * travel; enemy.y += move.y * travel;
          } else if (startTime >= this._lastTellAt + CONFIG.shooterStagger - EPS
            && this.enemies.filter(e => e.phase === 'tell').length < CONFIG.rangedCap) {
            enemy.phase = 'tell';
            enemy.pattern = enemy.type === 'foreman' && enemy._attacks % 2 ? 'fork' : 'pin';
            enemy.tellDuration = enemy.pattern === 'fork' ? CONFIG.forkTell : CONFIG.spitterTell;
            enemy.tell = enemy.tellDuration;
            enemy.locked = false;
            enemy.aim = direction(this.player.px - enemy.x, this.player.py - enemy.y);
            this._lastTellAt = startTime;
            this._emit('warning', { kind: 'charge', x: enemy.x, y: enemy.y, enemyId: enemy.id,
              duration: enemy.tellDuration, tellDuration: enemy.tellDuration, pattern: enemy.pattern });
          }
        }
        keepInside(enemy);
      }
    }

    _updateShots(dt, contacts) {
      for (const shot of this.shots) {
        shot.px = shot.x; shot.py = shot.y;
        const dir = direction(shot.vx, shot.vy);
        const desired = Math.min(CONFIG.shotSpeed * dt, shot.life * CONFIG.shotSpeed);
        const distance = wallDistance(shot, dir.x, dir.y, desired);
        const endT = clamp(distance / (CONFIG.shotSpeed * dt), 0, 1);
        shot.x += dir.x * distance; shot.y += dir.y * distance;
        shot.life -= dt;
        const playerEnd = at(this.player, endT);
        const hit = sweep(shot.px, shot.py, shot.x, shot.y, this.player.px, this.player.py,
          playerEnd.x, playerEnd.y, shot.r + this.player.r);
        if (hit) contacts.push({ type: 'playerHit', t: hit.enter * endT, priority: 0, shot,
          x: lerp(shot.px, shot.x, hit.enter), y: lerp(shot.py, shot.y, hit.enter) });
        if (shot.life <= EPS || distance < desired - EPS || endT < 1 - EPS)
          contacts.push({ type: 'expire', t: endT, priority: 3, shot });
      }
    }

    _bodyContacts(dt, startTime, contacts) {
      for (const enemy of this.enemies) {
        const hit = sweep(enemy.px, enemy.py, enemy.x, enemy.y, this.player.px, this.player.py,
          this.player.x, this.player.y, enemy.r + this.player.r);
        if (!hit) continue;
        const t = Math.max(hit.enter, (enemy.nextContactAt - startTime) / dt, 0);
        if (t <= hit.exit + EPS) contacts.push({ type: 'playerHit', t: clamp(t, 0, 1),
          priority: 0, enemy, x: lerp(enemy.px, enemy.x, t), y: lerp(enemy.py, enemy.y, t) });
      }
    }

    _resolve(contact, eventTime) {
      const { piece, enemy, shot } = contact;
      if (contact.type === 'playerHit') {
        if (shot && shot._dead || enemy && enemy.hp <= 0) return;
        if (shot) shot._dead = true;
        if (enemy) enemy.nextContactAt = eventTime + CONFIG.contactInterval;
        if (eventTime < this._graceUntil - EPS) return;
        const playerPos = at(this.player, contact.t);
        const held = this.pieces.find(p => p.state === 'held');
        const away = direction(playerPos.x - contact.x, playerPos.y - contact.y,
          -(enemy?.aim.x ?? direction(shot?.vx ?? 1, shot?.vy ?? 0).x),
          -(enemy?.aim.y ?? direction(shot?.vx ?? 1, shot?.vy ?? 0).y));
        if (held) {
          held.state = 'ejected'; held.returnArmed = false; held.returnSpent = true;
          held.x = playerPos.x + away.x * CONFIG.launchOffset;
          held.y = playerPos.y + away.y * CONFIG.launchOffset;
          keepInside(held); held.px = held.x; held.py = held.y;
          held.vx = away.x * CONFIG.ejectionSpeed; held.vy = away.y * CONFIG.ejectionSpeed;
          held.angle = Math.atan2(away.y, away.x);
          held.range = CONFIG.ejectionRange; held.ejectTime = CONFIG.ejectionDuration;
          held._contacts = Object.create(null);
          this._graceUntil = eventTime + CONFIG.blockGrace;
          this.stats.blocks++;
          if (enemy?.type === 'biter') {
            enemy.x -= away.x * 0.6; enemy.y -= away.y * 0.6; keepInside(enemy);
          }
          this._emit('block', { x: playerPos.x, y: playerPos.y, pieceId: held.id, held: this.heldCount });
        } else {
          this.player.hull--;
          this.player.flash = 0.28;
          this._graceUntil = eventTime + CONFIG.hullGrace;
          this.stats.hullHits++;
          this._emit('hull', { x: playerPos.x, y: playerPos.y, amount: 1, hull: this.player.hull });
          if (this.player.hull <= 0) this._finish('lost');
        }
      } else if (contact.type === 'enemyHit') {
        if (enemy.hp <= 0) return;
        const incoming = direction(piece.vx, piece.vy);
        if (contact.returnHit) {
          if (piece.state !== 'returning' || !piece.returnArmed || piece.returnSpent) return;
          piece.returnSpent = true;
          this.stats.returnDamage++; this.stats.returnHits++;
          if (this._playerMoving) this.stats.movingReturnHits++;
        } else {
          if (piece.state !== 'outbound') return;
          piece.x = contact.x; piece.y = contact.y;
          this.stats.outboundDamage++;
          this._land(piece);
        }
        enemy.hp--;
        enemy.flash = 0.16;
        if (!enemy.locked) {
          const knockback = enemy.type === 'foreman' ? 0.15 : 0.3;
          enemy.x += incoming.x * knockback; enemy.y += incoming.y * knockback; keepInside(enemy);
        }
        this._emit('enemyHit', { x: contact.x, y: contact.y, amount: 1,
          returnHit: contact.returnHit, enemyId: enemy.id, enemyType: enemy.type, pieceId: piece.id, hp: enemy.hp });
        if (enemy.hp <= 0) {
          this.stats.kills++; this._stageKills++;
          this._emit('enemyDie', { x: enemy.x, y: enemy.y, enemyId: enemy.id, enemyType: enemy.type });
          if (enemy.type === 'foreman') this._bossDefeatedAt = contact.t;
        }
      } else if (contact.type === 'collect') {
        if (piece.state === 'returning' || piece.state === 'loose') this._collect(piece);
      } else if (contact.type === 'land') {
        if (piece.state === 'outbound') this._land(piece);
      } else if (contact.type === 'expire') shot._dead = true;
      else if (contact.type === 'shoot' && enemy.hp > 0) this._shoot(enemy);
    }

    _shoot(enemy) {
      const angles = enemy.pattern === 'fork' ? [-CONFIG.forkDegrees, CONFIG.forkDegrees] : [0];
      for (const degrees of angles) {
        const dir = rotate(enemy.aim, degrees * Math.PI / 180);
        const shot = { id: this._nextId++, x: enemy.x + dir.x * (enemy.r + CONFIG.shotRadius + 0.03),
          y: enemy.y + dir.y * (enemy.r + CONFIG.shotRadius + 0.03), r: CONFIG.shotRadius,
          vx: dir.x * CONFIG.shotSpeed, vy: dir.y * CONFIG.shotSpeed, life: CONFIG.shotLifetime,
          enemyId: enemy.id, _dead: false };
        keepInside(shot); shot.px = shot.x; shot.py = shot.y;
        this.shots.push(shot);
      }
      enemy._attacks++;
      this._emit('shoot', { x: enemy.x, y: enemy.y, enemyId: enemy.id,
        enemyType: enemy.type, count: angles.length, pattern: enemy.pattern });
    }

    _land(piece, emit = true) {
      piece.state = 'loose'; piece.vx = 0; piece.vy = 0;
      keepInside(piece); piece._looseStep = this._stepId;
      if (emit) this._emit('land', { x: piece.x, y: piece.y, pieceId: piece.id });
    }

    _collect(piece, silent = false) {
      piece.state = 'held'; piece.returnArmed = false; piece.returnSpent = true;
      piece.vx = 0; piece.vy = 0; piece._contacts = Object.create(null);
      if (!silent) {
        this.stats.collects++;
        this._emit('collect', { x: this.player.x, y: this.player.y, pieceId: piece.id, held: this.heldCount });
      }
    }

    _placeHeld() {
      for (const piece of this.pieces) {
        if (piece.state !== 'held') continue;
        piece.angle = piece.id * TAU / CONFIG.pieceCount + this.time * 0.75;
        piece.x = this.player.x + Math.cos(piece.angle) * 0.91;
        piece.y = this.player.y + Math.sin(piece.angle) * 0.91;
      }
    }

    _safeguardPieces() {
      for (const piece of this.pieces) {
        if (piece.state === 'held') continue;
        if (!Number.isFinite(piece.x) || !Number.isFinite(piece.y)
          || piece.x < piece.r - EPS || piece.y < piece.r - EPS
          || piece.x > CONFIG.width - piece.r + EPS || piece.y > CONFIG.height - piece.r + EPS) {
          if (!Number.isFinite(piece.x)) piece.x = this.player.x;
          if (!Number.isFinite(piece.y)) piece.y = this.player.y;
          piece.returnArmed = false;
          this._land(piece, false);
        }
      }
    }

    _beginStage(stage) {
      this.stage = stage; this.stageName = STAGES[stage];
      this.phase = 'warning'; this._stageKills = 0; this._nextPack = 0;
      this._lastTellAt = -Infinity;
      if (stage < 3) this._queuePack();
      else {
        this._escortsQueued = 2;
        this._entryQueue.push({ type: 'foreman', entry: 7, escort: false },
          { type: 'biter', entry: 0, escort: true }, { type: 'biter', entry: 1, escort: true });
      }
      this._startWarnings();
    }

    _queuePack() {
      const pack = PACKS[this.stage][this._nextPack++];
      for (const [type, entry] of pack) this._entryQueue.push({ type, entry, escort: false });
    }

    _safeEntry(entry, type, ignoreId) {
      const r = TYPES[type].r;
      for (let offset = 0; offset < ENTRIES.length; offset++) {
        const index = (entry + offset) % ENTRIES.length;
        const source = ENTRIES[index];
        const point = { x: clamp(source[0], r + 0.3, CONFIG.width - r - 0.3),
          y: clamp(this._mirrorY ? CONFIG.height - source[1] : source[1], r + 0.3, CONFIG.height - r - 0.3) };
        if (length(point.x - this.player.x, point.y - this.player.y) < CONFIG.safeSpawnDistance) continue;
        if (this.spawns.some(s => s.id !== ignoreId && length(s.x - point.x, s.y - point.y) < TYPES[s.type].r + r + 0.15)) continue;
        if (this.enemies.some(e => e.hp > 0 && length(e.x - point.x, e.y - point.y) < e.r + r + 0.1)) continue;
        return { ...point, entry: index };
      }
      return null;
    }

    _startWarnings() {
      while (this._entryQueue.length && this.enemies.length + this.spawns.length < CONFIG.liveCap) {
        const pending = this._entryQueue[0];
        const point = this._safeEntry(pending.entry, pending.type);
        if (!point) break;
        this._entryQueue.shift();
        const spawn = { id: this._nextId++, type: pending.type, x: point.x, y: point.y,
          time: CONFIG.warningDuration, duration: CONFIG.warningDuration,
          escort: pending.escort, _entry: point.entry };
        this.spawns.push(spawn);
        this._emit('warning', { kind: 'entry', x: spawn.x, y: spawn.y, enemyType: spawn.type,
          duration: spawn.duration, spawnId: spawn.id });
      }
    }

    _updateSpawns(dt) {
      for (const spawn of this.spawns) {
        spawn.time -= dt;
        if (spawn.time > EPS) continue;
        if (length(spawn.x - this.player.x, spawn.y - this.player.y) < CONFIG.safeSpawnDistance) {
          const point = this._safeEntry(spawn._entry + 1, spawn.type, spawn.id);
          if (point) { spawn.x = point.x; spawn.y = point.y; spawn._entry = point.entry; }
          spawn.time = spawn.duration;
          this._emit('warning', { kind: 'entry', x: spawn.x, y: spawn.y, enemyType: spawn.type,
            duration: spawn.duration, spawnId: spawn.id });
          continue;
        }
        const template = TYPES[spawn.type];
        this.enemies.push({ id: spawn.id, type: spawn.type, x: spawn.x, y: spawn.y,
          px: spawn.x, py: spawn.y, r: template.r, hp: template.hp, maxHp: template.hp,
          phase: 'approach', aim: direction(this.player.x - spawn.x, this.player.y - spawn.y),
          tell: 0, tellDuration: CONFIG.spitterTell, locked: false, flash: 0,
          pattern: 'pin', recovery: 0, nextContactAt: this.time, escort: spawn.escort, _attacks: 0 });
        spawn._done = true;
      }
      this.spawns = this.spawns.filter(s => !s._done);
    }

    _progress() {
      if (this.stage < 3) {
        if (this._nextPack < PACKS[this.stage].length && this._stageKills > 0
          && this.enemies.length <= (this.stage === 0 ? 1 : 2)
          && !this.spawns.length && !this._entryQueue.length) this._queuePack();
        this._startWarnings();
        if (!this.enemies.length && !this.spawns.length && !this._entryQueue.length
          && this._nextPack >= PACKS[this.stage].length) {
          this.stats.stagesCleared++;
          this._emit('stageClear', { stage: this.stage, stageName: this.stageName,
            x: this.player.x, y: this.player.y });
          this.shots = [];
          for (const piece of this.pieces) this._collect(piece, true);
          this.recalling = false;
          this.phase = 'transition';
          this._transitionLeft = CONFIG.transitionDuration;
        } else this.phase = this.enemies.length ? 'combat' : 'warning';
      } else {
        const present = this.enemies.filter(e => e.escort).length
          + this.spawns.filter(s => s.escort).length + this._entryQueue.filter(s => s.escort).length;
        if (this._escortsQueued < CONFIG.escortTotal && present < CONFIG.escortCap) {
          this._entryQueue.push({ type: 'biter', entry: this._escortsQueued % 2 ? 5 : 9, escort: true });
          this._escortsQueued++;
        }
        this._startWarnings();
        this.phase = this.enemies.length ? 'combat' : 'warning';
      }
    }

    _finish(state) {
      this.state = state; this.phase = state; this.recalling = false;
      this.shots = []; this.spawns = []; this._entryQueue = [];
      if (state === 'won') {
        this.enemies = [];
        for (const piece of this.pieces) this._collect(piece, true);
        this._placeHeld();
      }
      this._emit(state === 'won' ? 'win' : 'lose', { x: this.player.x, y: this.player.y,
        hull: this.player.hull, stats: { ...this.stats } });
    }
  }

  return { Game, CONFIG };
});
