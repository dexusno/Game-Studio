(function (root) {
  'use strict';
  const TAU = Math.PI * 2;
  const C = { ink: '#172b2d', floor: '#314945', rim: '#557064', aqua: '#90ead4', gold: '#ffe1a0', coral: '#ff826a', paper: '#f4f0d7' };
  const clamp = (v, a, b) => Math.max(a, Math.min(b, v));
  function rounded(ctx, x, y, w, h, r) { ctx.beginPath(); ctx.roundRect(x, y, w, h, r); }
  function circle(ctx, x, y, r, fill, stroke, width = .04) {
    ctx.beginPath(); ctx.arc(x, y, r, 0, TAU);
    if (fill) { ctx.fillStyle = fill; ctx.fill(); }
    if (stroke) { ctx.strokeStyle = stroke; ctx.lineWidth = width; ctx.stroke(); }
  }
  class Renderer {
    constructor(canvas) {
      this.canvas = canvas; this.ctx = canvas.getContext('2d', { alpha: false });
      this.particles = []; this.rings = []; this.trails = new Map(); this.shake = 0;
      this.width = 0; this.height = 0; this.scale = 1; this.ox = 0; this.oy = 0;
      this.resize();
    }
    resize() {
      this.width = window.innerWidth; this.height = window.innerHeight;
      this.dpr = Math.min(window.devicePixelRatio || 1, 2);
      this.canvas.width = Math.round(this.width * this.dpr);
      this.canvas.height = Math.round(this.height * this.dpr);
      const top = this.width < 650 ? 112 : 96;
      const bottom = this.height < 740 ? 110 : 120;
      this.scale = Math.max(8, Math.min((this.width - 70) / 24, (this.height - top - bottom) / 14));
      this.ox = (this.width - 24 * this.scale) / 2;
      this.oy = top + (this.height - top - bottom - 14 * this.scale) / 2;
    }
    worldPoint(clientX, clientY) { return { x: (clientX - this.ox) / this.scale, y: (clientY - this.oy) / this.scale }; }
    reset() { this.particles.length = 0; this.rings.length = 0; this.trails.clear(); this.shake = 0; }
    event(e) {
      const x = Number.isFinite(e.x) ? e.x : 12, y = Number.isFinite(e.y) ? e.y : 7;
      let count = 0, color = C.gold, speed = 2;
      if (e.type === 'launch') { count = 10; speed = 3; this.shake = Math.max(this.shake, .07); this.rings.push({ x, y, age: 0, life: .26, color: C.gold, r: .45 }); }
      if (e.type === 'collect') { count = 3; color = C.aqua; speed = .9; }
      if (e.type === 'enemyHit') { count = 7; color = e.returnHit ? C.aqua : C.gold; speed = 2.4; }
      if (e.type === 'enemyDie') { count = 18; color = '#b8bfa0'; speed = 3.2; this.shake = Math.max(this.shake, .045); }
      if (e.type === 'block') { count = 13; color = C.gold; speed = 3.2; this.shake = .11; this.rings.push({ x, y, age: 0, life: .3, color: C.gold, r: .55 }); }
      if (e.type === 'hull') { count = 18; color = C.coral; speed = 3.6; this.shake = .18; this.rings.push({ x, y, age: 0, life: .45, color: C.coral, r: .55 }); }
      if (e.type === 'win') { count = 35; color = C.aqua; speed = 4; this.rings.push({ x, y, age: 0, life: .7, color: C.aqua, r: .8 }); }
      for (let i = 0; i < count; i++) {
        const a = (i / Math.max(count, 1)) * TAU + Math.random() * .35, v = speed * (.35 + Math.random() * .65);
        this.particles.push({ x, y, vx: Math.cos(a) * v, vy: Math.sin(a) * v, age: 0, life: .22 + Math.random() * .42, size: .025 + Math.random() * .055, angle: a, color });
      }
      if (this.particles.length > 260) this.particles.splice(0, this.particles.length - 260);
    }
    render(game, options) {
      const ctx = this.ctx, { home = false, elapsed = 0, dt = 0, shake = true, freeze = false } = options;
      if (this.width !== window.innerWidth || this.height !== window.innerHeight) this.resize();
      ctx.setTransform(this.dpr, 0, 0, this.dpr, 0, 0);
      const bg = ctx.createLinearGradient(0, 0, this.width, this.height);
      bg.addColorStop(0, '#172b30'); bg.addColorStop(.6, '#14262b'); bg.addColorStop(1, '#102026');
      ctx.fillStyle = bg; ctx.fillRect(0, 0, this.width, this.height);
      this.drawOuter(ctx);
      ctx.save();
      this.shake *= Math.exp(-dt * 15);
      const shakeX = shake && !freeze ? Math.sin(elapsed * 121) * this.shake * this.scale : 0;
      const shakeY = shake && !freeze ? Math.cos(elapsed * 143) * this.shake * this.scale * .7 : 0;
      ctx.translate(this.ox + shakeX, this.oy + shakeY); ctx.scale(this.scale, this.scale);
      if (home) ctx.globalAlpha = .63;
      this.drawTray(ctx, elapsed);
      ctx.save(); rounded(ctx, .05, .05, 23.9, 13.9, .3); ctx.clip();
      if (home) this.drawHome(ctx, elapsed);
      else if (game) {
        this.drawSpawns(ctx, game.spawns || [], elapsed);
        this.drawTells(ctx, game.enemies || []);
        if (game.state === 'playing') this.drawAim(ctx, game);
        this.drawPieces(ctx, game, elapsed, freeze);
        for (const e of game.enemies) this.drawEnemy(ctx, e, game.player, elapsed);
        this.drawShots(ctx, game.shots, elapsed);
        this.drawPip(ctx, game.player.x, game.player.y, game.player.aim, elapsed, game.heldCount, game.player.grace, game.player.flash, game.state === 'lost');
        this.drawHeld(ctx, game, elapsed);
        this.drawEffects(ctx, freeze ? 0 : dt);
        if (game.phase === 'transition') {
          ctx.fillStyle = '#d7f2d7'; ctx.textAlign = 'center'; ctx.font = '700 .25px Segoe UI, sans-serif';
          ctx.fillText('ALL PIECES ACCOUNTED FOR', 12, 6); ctx.font = '.2px Segoe UI, sans-serif';
          ctx.fillStyle = '#adc5aa'; ctx.fillText('The next gate is opening', 12, 6.45);
        }
      }
      ctx.restore(); ctx.restore();
      if (!home && game) this.drawBossBar(ctx, game);
      if (home) {
        const veil = ctx.createLinearGradient(0, 0, this.width * .73, 0);
        veil.addColorStop(0, '#14262bfa'); veil.addColorStop(.60, '#14262bee'); veil.addColorStop(1, '#14262b00');
        ctx.fillStyle = veil; ctx.fillRect(0, 0, this.width, this.height);
      }
    }
    drawOuter(ctx) {
      ctx.strokeStyle = '#82a58a0b'; ctx.lineWidth = 1;
      for (let x = 13; x < this.width; x += 29) { ctx.beginPath(); ctx.moveTo(x, 0); ctx.lineTo(x, this.height); ctx.stroke(); }
      for (let y = 9; y < this.height; y += 29) { ctx.beginPath(); ctx.moveTo(0, y); ctx.lineTo(this.width, y); ctx.stroke(); }
      ctx.fillStyle = '#061b2266';
      rounded(ctx, this.ox - 9, this.oy - 5, this.scale * 24 + 18, this.scale * 14 + 19, 17); ctx.fill();
    }
    drawTray(ctx) {
      ctx.save();
      rounded(ctx, -.19, -.19, 24.38, 14.38, .45); ctx.fillStyle = '#5e7564'; ctx.fill();
      rounded(ctx, -.1, -.1, 24.2, 14.2, .39); ctx.fillStyle = '#202f2d'; ctx.fill();
      const floor = ctx.createLinearGradient(0, 0, 24, 14);
      floor.addColorStop(0, '#3c5348'); floor.addColorStop(.5, '#344b42'); floor.addColorStop(1, '#2b413d');
      rounded(ctx, .02, .02, 23.96, 13.96, .29); ctx.fillStyle = floor; ctx.fill();
      ctx.strokeStyle = '#bed0a30b'; ctx.lineWidth = .018;
      for (let x = 1; x < 24; x++) { ctx.beginPath(); ctx.moveTo(x, .25); ctx.lineTo(x, 13.75); ctx.stroke(); }
      for (let y = 1; y < 14; y++) { ctx.beginPath(); ctx.moveTo(.25, y); ctx.lineTo(23.75, y); ctx.stroke(); }
      for (let i = 0; i < 155; i++) {
        const x = .6 + ((i * 71.73) % 22.7), y = .6 + ((i * 27.19) % 12.7);
        ctx.strokeStyle = i % 3 ? '#a2b5a009' : '#132f2420'; ctx.lineWidth = .02;
        ctx.beginPath(); ctx.moveTo(x, y); ctx.lineTo(x + .12 + (i % 5) * .08, y + .05); ctx.stroke();
      }
      ctx.strokeStyle = '#bed0a326'; ctx.lineWidth = .025;
      rounded(ctx, .36, .36, 23.28, 13.28, .1); ctx.stroke();
      ctx.setLineDash([.3, .3]); ctx.strokeStyle = '#bed0a316';
      rounded(ctx, 1.1, 1.1, 21.8, 11.8, .25); ctx.stroke(); ctx.setLineDash([]);
      for (const [x, y] of [[.18, .18], [23.82, .18], [.18, 13.82], [23.82, 13.82]]) {
        circle(ctx, x, y, .11, '#9aab8c', '#112a25', .025);
        ctx.strokeStyle = '#40524a'; ctx.lineWidth = .035; ctx.beginPath(); ctx.moveTo(x - .055, y - .045); ctx.lineTo(x + .055, y + .045); ctx.stroke();
      }
      ctx.fillStyle = '#aabd9a24'; ctx.font = '700 .22px Segoe UI, sans-serif'; ctx.textAlign = 'left';
      ctx.fillText('SORTING BAY / 01', .8, .85); ctx.textAlign = 'right'; ctx.fillText('KEEP ALL SIX', 23.2, 13.3);
      for (const [x,y,a] of [[0,7,0],[24,7,Math.PI],[12,0,Math.PI/2],[12,14,-Math.PI/2]]) {
        ctx.save(); ctx.translate(x,y); ctx.rotate(a); ctx.fillStyle = '#a5ac7144';
        for (let i = -2; i < 3; i++) { ctx.beginPath(); ctx.moveTo(.07,i*.22-.06);ctx.lineTo(.3,i*.22+.02);ctx.lineTo(.3,i*.22+.11);ctx.lineTo(.07,i*.22+.03);ctx.fill(); }
        ctx.restore();
      }
      ctx.restore();
    }
    drawHome(ctx, t) {
      ctx.save(); ctx.globalAlpha = 1; ctx.translate(17.4, 6.8); ctx.scale(2.65, 2.65);
      circle(ctx, 0, .3, 1.8, '#172f2944'); circle(ctx, 0, 0, 1.6, null, '#9fe7bc13', .012);
      this.drawPip(ctx, 0, 0, { x: -.84, y: .45 }, t, 6, 0, 0, false);
      for (let i=0;i<6;i++){ const a=t*.32+i*TAU/6; this.drawScrap(ctx,Math.cos(a)*1.2,Math.sin(a)*1.05,i,a, C.aqua, true); }
      ctx.restore();
      const enemy = {x:22,y:3,r:.55,type:'spitter',hp:2,maxHp:2,aim:{x:-1,y:.2},phase:'recover',flash:0};
      ctx.globalAlpha=.55; this.drawEnemy(ctx,enemy,{x:17,y:7},t);ctx.globalAlpha=1;
    }
    drawPip(ctx, x, y, aim, t, held, grace, flash, dead) {
      ctx.save(); ctx.translate(x, y);
      const facing = Math.atan2(aim?.y ?? 0, aim?.x ?? 1);
      ctx.fillStyle='#10272366'; ctx.beginPath();ctx.ellipse(.04,.19,.57,.35,0,0,TAU);ctx.fill();
      if (grace > 0) circle(ctx,0,0,.66,null,`rgba(255,198,135,${.35+.25*Math.sin(t*30)})`,.035);
      ctx.rotate(dead ? -.5 : facing);
      const bob = dead ? 0 : Math.sin(t*7)*.013;
      for (const y0 of [-.42,.3]) {
        rounded(ctx,-.3,y0,.58,.18,.06);ctx.fillStyle='#132d30';ctx.fill();ctx.strokeStyle='#718579';ctx.lineWidth=.025;ctx.stroke();
        for(let i=0;i<5;i++){ctx.fillStyle='#4d655d';ctx.fillRect(-.23+i*.103,y0+.025,.042,.12);}
      }
      rounded(ctx,-.43,-.34+bob,.85,.68,.2);ctx.fillStyle=flash>0?'#fff4d2':'#e7c77e';ctx.fill();ctx.lineWidth=.035;ctx.strokeStyle='#182f30';ctx.stroke();
      rounded(ctx,-.31,-.26+bob,.63,.50,.14);ctx.fillStyle=dead?'#797c64':'#f4df9f';ctx.fill();
      rounded(ctx,-.14,-.205+bob,.42,.41,.1);ctx.fillStyle='#173c42';ctx.fill();ctx.strokeStyle='#bbaa68';ctx.lineWidth=.025;ctx.stroke();
      if (!dead) { ctx.fillStyle=held?C.aqua:C.coral; ctx.fillRect(.065,-.125+bob,.07,.075);ctx.fillRect(.065,.05+bob,.07,.075); }
      else { ctx.strokeStyle='#b09875';ctx.lineWidth=.03;ctx.beginPath();ctx.moveTo(.04,-.1);ctx.lineTo(.14,0);ctx.moveTo(.04,0);ctx.lineTo(.14,-.1);ctx.stroke(); }
      rounded(ctx,.34,-.17,.18,.34,.04);ctx.fillStyle='#667f77';ctx.fill();ctx.fillStyle=held?C.aqua:'#e18870';ctx.fillRect(.465,-.125,.07,.25);
      circle(ctx,-.28,-.15,.042,'#516f66');circle(ctx,-.28,.15,.042,'#516f66');
      ctx.strokeStyle='#567c6d';ctx.lineWidth=.04;ctx.beginPath();ctx.moveTo(-.30,-.3);ctx.lineTo(-.42,-.55);ctx.stroke();circle(ctx,-.42,-.55,.068,dead?'#839b84':C.aqua);
      ctx.restore();
    }
    drawHeld(ctx, game, t) {
      const held = game.pieces.filter(p=>p.state==='held');
      if (held.length) circle(ctx,game.player.x,game.player.y,.94,null,'#9ce7c718',.016);
      held.forEach(p=>{const a=t*.52+p.id*TAU/6;this.drawScrap(ctx,game.player.x+Math.cos(a)*.94,game.player.y+Math.sin(a)*.94,p.id,a,C.aqua,true);});
    }
    drawScrap(ctx,x,y,id,a,color,held) {
      ctx.save();ctx.translate(x,y);ctx.rotate(a);
      circle(ctx,.04,.10,.13,'#09282255');ctx.lineJoin='round';ctx.lineCap='round';
      if(id%3===0){circle(ctx,0,0,.145,color,'#172d32',.028);circle(ctx,0,0,.052,'#345a50');}
      else if(id%3===1){ctx.strokeStyle='#152f32';ctx.lineWidth=.115;ctx.beginPath();ctx.moveTo(-.145,0);ctx.lineTo(.13,0);ctx.stroke();ctx.strokeStyle=color;ctx.lineWidth=.065;ctx.stroke();circle(ctx,-.115,0,.082,color,'#173136',.023);}
      else {ctx.strokeStyle='#16353b';ctx.lineWidth=.09;rounded(ctx,-.13,-.06,.26,.12,.04);ctx.stroke();ctx.strokeStyle=color;ctx.lineWidth=.035;ctx.stroke();}
      if(held){ctx.fillStyle='#eff7d9';ctx.fillRect(-.07,-.08,.045,.026);}
      ctx.restore();
    }
    drawPieces(ctx,game,t,freeze) {
      for (const p of game.pieces) {
        if (p.state==='held') {this.trails.delete(p.id);continue;}
        let trail=this.trails.get(p.id)||[];
        if(!freeze){trail.push({x:p.x,y:p.y});if(trail.length>7)trail.shift();this.trails.set(p.id,trail);}
        const returning=p.state==='returning',flying=p.state==='outbound'||p.state==='ejected';
        const color=returning?C.aqua:flying?C.gold:'#c1e6c0';
        if((returning||flying)&&trail.length>1){ctx.beginPath();ctx.moveTo(trail[0].x,trail[0].y);for(const pos of trail)ctx.lineTo(pos.x,pos.y);ctx.strokeStyle=returning?'#9afbdd65':'#ffdda865';ctx.lineWidth=returning?.05:.065;ctx.stroke();}
        if(p.state==='loose'){circle(ctx,p.x,p.y,.24,null,'#a5deb96b',.021);circle(ctx,p.x,p.y,.33,null,'#85c99712',.016);}
        if(returning && p.returnArmed && !p.returnSpent){circle(ctx,p.x,p.y,.23,null,'#d8ffcf88',.025);}
        this.drawScrap(ctx,p.x,p.y,p.id,Number.isFinite(p.angle)?p.angle:t*5+p.id,color,false);
      }
    }
    drawAim(ctx,game) {
      const p=game.player,a=Math.atan2(p.aim.y,p.aim.x),n=game.heldCount;
      if(game.recalling){circle(ctx,p.x,p.y,1.21,null,'#94e4c42d',.018);return;}
      const ray=3.6;
      if(n>0){ctx.save();ctx.translate(p.x,p.y);ctx.rotate(a);ctx.beginPath();ctx.moveTo(.7,0);ctx.arc(0,0,ray,-.305,.305);ctx.closePath();ctx.fillStyle='#bcf2cf05';ctx.fill();
        for(let i=0;i<n;i++){const angle=(i-(n-1)/2)*35/5*Math.PI/180;ctx.beginPath();ctx.moveTo(Math.cos(angle)*1.15,Math.sin(angle)*1.15);ctx.lineTo(Math.cos(angle)*ray,Math.sin(angle)*ray);ctx.strokeStyle='#b7e7c120';ctx.lineWidth=.012;ctx.stroke();}ctx.restore();}
      const tx=p.x+Math.cos(a)*1.46,ty=p.y+Math.sin(a)*1.46;
      ctx.save();ctx.translate(tx,ty);ctx.rotate(a);ctx.strokeStyle=n?'#dcf8d9aa':'#e5937899';ctx.lineWidth=.033;ctx.beginPath();ctx.moveTo(-.055,-.095);ctx.lineTo(.07,0);ctx.lineTo(-.055,.095);ctx.stroke();ctx.restore();
    }
    drawSpawns(ctx,spawns,t) {
      for(const s of spawns){const progress=1-clamp(s.time/s.duration,0,1);circle(ctx,s.x,s.y,s.type==='foreman'?1.45:.72,'#ff997116','#f0b07766',.025);ctx.strokeStyle='#ffd093';ctx.lineWidth=.048;ctx.beginPath();ctx.arc(s.x,s.y,s.type==='foreman'?1.48:.75,-Math.PI/2,-Math.PI/2+TAU*progress);ctx.stroke();
        ctx.fillStyle='#ffe3a8';ctx.font='700 .34px Segoe UI,sans-serif';ctx.textAlign='center';ctx.fillText('!',s.x,s.y+.12);
      }
    }
    drawTells(ctx,enemies) {
      for(const e of enemies){if(e.phase!=='tell')continue;const angle=Math.atan2(e.aim.y,e.aim.x);const spread=e.pattern==='fork'?[-18,18]:[0];
        for(const offset of spread){const a=angle+offset*Math.PI/180;
          ctx.save();ctx.strokeStyle=e.locked?'#ff937481':'#eaaa6a40';ctx.lineWidth=e.locked?.11:.028;if(!e.locked)ctx.setLineDash([.18,.19]);ctx.beginPath();ctx.moveTo(e.x+Math.cos(a)*e.r,e.y+Math.sin(a)*e.r);ctx.lineTo(e.x+Math.cos(a)*30,e.y+Math.sin(a)*30);ctx.stroke();ctx.restore();
        }
        const progress=1-clamp(e.tell/e.tellDuration,0,1);ctx.strokeStyle=e.locked?C.coral:'#ffca88';ctx.lineWidth=.045;ctx.beginPath();ctx.arc(e.x,e.y,e.r+.16,-Math.PI/2,-Math.PI/2+TAU*progress);ctx.stroke();
      }
    }
    drawEnemy(ctx,e,p,t) {
      ctx.save();ctx.translate(e.x,e.y);
      ctx.fillStyle='#0d24255c';ctx.beginPath();ctx.ellipse(.08,.20,e.r*1.08,e.r*.65,0,0,TAU);ctx.fill();
      const a=e.type==='biter'?Math.atan2(p.y-e.y,p.x-e.x):Math.atan2(e.aim.y,e.aim.x);
      ctx.rotate(a);const hit=e.flash>0;const tell=e.phase==='tell';
      if(e.type==='biter'){
        const bite=.055*Math.sin(t*10+e.id);rounded(ctx,-.4,-.3,.55,.6,.12);ctx.fillStyle=hit?C.paper:'#b77b65';ctx.fill();ctx.strokeStyle='#223731';ctx.lineWidth=.04;ctx.stroke();
        for(const side of [-1,1]){ctx.save();ctx.translate(.1,side*(.23+bite));ctx.rotate(side*.15);rounded(ctx,-.2,-.09,.5,.18,.04);ctx.fillStyle=hit?C.paper:'#ec9f77';ctx.fill();ctx.strokeStyle='#283e36';ctx.lineWidth=.025;ctx.stroke();for(let i=0;i<3;i++){ctx.fillStyle='#e8d4a6';ctx.fillRect(-.08+i*.105,-side*.1-.04,.05,.08);}ctx.restore();}
        circle(ctx,-.2,-.13,.055,'#382c24');circle(ctx,-.2,.13,.055,'#382c24');ctx.fillStyle='#ffe3b0';ctx.fillRect(-.19,-.15,.027,.035);ctx.fillRect(-.19,.11,.027,.035);
      } else if(e.type==='spitter') {
        circle(ctx,0,0,.48,hit?C.paper:'#849787','#243b34',.035);rounded(ctx,-.27,-.4,.42,.8,.075);ctx.fillStyle=hit?C.paper:'#a9ac87';ctx.fill();
        rounded(ctx,.03,-.2,.63,.4,.045);ctx.fillStyle='#3b5750';ctx.fill();ctx.strokeStyle='#223c35';ctx.lineWidth=.035;ctx.stroke();ctx.fillStyle=tell?'#ffba80':'#9f7861';ctx.fillRect(.48,-.17,.15,.34);circle(ctx,-.1,-.25,.06,tell?C.coral:'#425e54');circle(ctx,-.1,.25,.06,tell?C.coral:'#425e54');circle(ctx,.65,0,.11,'#293b36');
      } else {
        rounded(ctx,-1,-.88,1.82,1.76,.2);ctx.fillStyle=hit?C.paper:'#a0755f';ctx.fill();ctx.strokeStyle='#233831';ctx.lineWidth=.06;ctx.stroke();
        for(const y of [-1.07,.8]){rounded(ctx,-.8,y,1.2,.26,.04);ctx.fillStyle='#30463f';ctx.fill();for(let i=0;i<6;i++){ctx.fillStyle='#74856c';ctx.fillRect(-.71+i*.19,y+.055,.09,.15);}}
        rounded(ctx,-.74,-.65,1.17,1.3,.12);ctx.fillStyle=hit?C.paper:'#d0aa79';ctx.fill();rounded(ctx,-.4,-.48,.69,.96,.08);ctx.fillStyle='#2e4540';ctx.fill();
        ctx.fillStyle=tell?C.coral:C.gold;ctx.fillRect(-.18,-.31,.15,.2);ctx.fillRect(-.18,.11,.15,.2);
        for(const y of [-.54,.54]){rounded(ctx,.45,y-.12,.75,.24,.045);ctx.fillStyle='#78937c';ctx.fill();ctx.fillStyle=tell?C.coral:'#c59d72';ctx.fillRect(1.04,y-.12,.13,.24);}
        for(const [x,y]of[[-.86,-.7],[-.86,.7],[.68,-.7],[.68,.7]])circle(ctx,x,y,.065,'#ded3a0','#485447',.015);
      }
      ctx.restore();
      if(e.type!=='foreman'&&e.hp<e.maxHp){const w=e.r*1.6;ctx.fillStyle='#18372b';rounded(ctx,e.x-w/2,e.y-e.r-.3,w,.085,.03);ctx.fill();ctx.fillStyle='#f4c08a';rounded(ctx,e.x-w/2,e.y-e.r-.3,w*(e.hp/e.maxHp),.085,.025);ctx.fill();}
    }
    drawShots(ctx,shots,t) {
      for(const s of shots){const a=Math.atan2(s.vy,s.vx);ctx.save();ctx.translate(s.x,s.y);ctx.rotate(a);ctx.strokeStyle='#ff8e655e';ctx.lineWidth=.09;ctx.beginPath();ctx.moveTo(-.52,0);ctx.lineTo(0,0);ctx.stroke();circle(ctx,0,0,s.r+.04,'#542d2b');circle(ctx,0,0,s.r,'#ff896f');circle(ctx,.02,0,s.r*.46,'#fff0b5');ctx.restore();}
    }
    drawEffects(ctx,dt) {
      for(let i=this.particles.length-1;i>=0;i--){const p=this.particles[i];p.age+=dt;if(p.age>p.life){this.particles.splice(i,1);continue;}p.x+=p.vx*dt;p.y+=p.vy*dt;p.vx*=Math.exp(-dt*5);p.vy*=Math.exp(-dt*5);ctx.save();ctx.globalAlpha=1-p.age/p.life;ctx.translate(p.x,p.y);ctx.rotate(p.angle+p.age*3);ctx.fillStyle=p.color;ctx.fillRect(-p.size,-p.size,p.size*2,p.size*1.4);ctx.restore();}
      for(let i=this.rings.length-1;i>=0;i--){const r=this.rings[i];r.age+=dt;if(r.age>=r.life){this.rings.splice(i,1);continue;}ctx.save();ctx.globalAlpha=(1-r.age/r.life)*.6;circle(ctx,r.x,r.y,r.r+r.age*2.3,null,r.color,.032);ctx.restore();}
    }
    drawBossBar(ctx,game) {
      const boss=game.enemies.find(e=>e.type==='foreman');if(!boss)return;
      const w=Math.min(250,this.width*.25),x=(this.width-w)/2,y=this.oy+15;
      ctx.fillStyle='#16322cdf';rounded(ctx,x-12,y-8,w+24,37,6);ctx.fill();
      ctx.font='700 8px Segoe UI,sans-serif';ctx.textAlign='center';ctx.fillStyle='#f1d3a2';ctx.fillText('THE FOREMAN',this.width/2,y+3);
      ctx.fillStyle='#5e5141';rounded(ctx,x,y+10,w,5,2);ctx.fill();ctx.fillStyle='#e8ae7b';rounded(ctx,x,y+10,w*clamp(boss.hp/boss.maxHp,0,1),5,2);ctx.fill();
    }
  }
  root.SixfoldView = { Renderer };
})(globalThis);
