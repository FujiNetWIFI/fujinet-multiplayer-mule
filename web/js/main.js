// M.U.L.E. Tournament web test client.
//
// Timing follows the Atari 800 original frame for frame (docs/TIMING.md): the
// client runs a fixed 60 Hz (jiffy) simulation, walks the colonist with the
// measured per-tick steps and terrain delays, and predicts the grant cursor,
// auction lines and time bars between polls from the server's jiffy clock.
import { Api, listTables } from './api.js';
import { Gfx, W, H } from './gfx.js';
import { decode, hexdump } from './bin.js';

const GOODS = ['food', 'energy', 'smithore', 'crystite'];
const SPECIES = ['mechtron', 'gollumer', 'packer', 'bonzoid', 'spheroid', 'flapper', 'leggite', 'humanoid'];
const PHASE = { lobby: 0, grant: 1, landauction: 2, develop: 3, production: 4, declare: 5, trading: 6, summary: 7, gameover: 8 };
const STAGE = { show: 0, declare: 1, lead: 2, active: 3, done: 4 };
const MAP_X = 16, MAP_Y = 16, PS = 32, COLS = 9, ROWS = 5, STORE = 22;
const TOWN_X = 64, TOWN_Y = 24;

// Measured on the original (NTSC jiffies, canvas pixels = Atari hi-res pixels / scanlines).
const T = {
  MOVE: 6,           // the colonist moves once every 6 jiffies
  MAP_DX: 4, MAP_DY: 2,   // map step: 2 colour clocks / 2 scanlines
  TOWN_DX: 8, TOWN_DY: 4, // town step: 4 colour clocks / 4 scanlines
  MOUNTAIN: 21,      // jiffies per step while crossing a mountain
  RIVER: 14,         // jiffies per step while crossing the river
  TOWN_EXIT: 25,     // screen change town <-> map
  SUMMARY_ANIM: 410, LANDING: 726, GRANT_INTRO: 186,
  PRODUCTION: 1421, GOODS_STATUS: 561,
};

const $ = (id) => document.getElementById(id);
const gfx = new Gfx($('c'));
let api = null;
let keys = new Set();
let pressed = new Set();
let leaveAsk = false; // ESC in a game: asking whether to leave
let debugTimer = 0;
let jiffy = 0; // local simulation frame counter

// Local development-turn state (the walk is simulated here; actions go to the server).
const walk = { view: 'town', x: 0, y: 0, frame: 0, next: 0, sample: -1, landMode: false, inside: '', turnKey: '', freeze: 0 };

// ---------------------------------------------------------------- the alert
// The one sound: two rising notes (the retro clients' SND_ALERT) whenever
// the game wants your hand on the stick. It rings once per occasion (key).
// Browsers start audio only after a key or click, so the first one opens it.

let audio = null, alerted = '';

function unlockAudio() {
  try {
    if (!audio) audio = new AudioContext();
    if (audio.state === 'suspended') audio.resume();
  } catch { audio = null; }
}

function alertOnce(v, key) {
  if (v.me < 0 || key === alerted) return;
  alerted = key;
  if (!audio || audio.state !== 'running') return;
  const t0 = audio.currentTime;
  [[1041, 0, 8], [1457, 8, 14]].forEach(([hz, at, n]) => {   // frames at 60 Hz
    const o = audio.createOscillator(), g = audio.createGain();
    o.type = 'square';
    o.frequency.value = hz;
    g.gain.value = 0.12;
    o.connect(g).connect(audio.destination);
    o.start(t0 + at / 60);
    o.stop(t0 + (at + n) / 60);
  });
}

// The time bar's tick, the original's (its VBI, $5E98): with u units of the
// bar left (PTU on a turn, ticks in an auction) and u under 30, a quiet
// tick every u jiffies, high and low by turns (AUDF $0A and $0F: ~2900 and
// ~2000 Hz), so it hurries as the bar runs out. The retro clients' bar_tick.
let tickNext = 0, tickLow = 0, tickSeen = 0;

function barTick(u) {
  const now = performance.now();
  if (now - tickSeen > 100) tickNext = 0;            // the bar was away: start over
  tickSeen = now;
  u = Math.floor(u);
  if (u <= 0 || u >= 30) { tickNext = 0; tickLow = 0; return; }
  if (now < tickNext) return;
  tickNext = now + u * 1000 / 60;
  const hz = tickLow ? 1998 : 2906;
  tickLow ^= 1;
  if (!audio || audio.state !== 'running') return;
  const t0 = audio.currentTime, o = audio.createOscillator(), g = audio.createGain();
  o.type = 'square';
  o.frequency.value = hz;
  g.gain.value = 0.03;
  o.connect(g).connect(audio.destination);
  o.start(t0);
  o.stop(t0 + 2 / 60);
}

function log(s) {
  const el = $('log');
  el.textContent = s + '\n' + el.textContent.slice(0, 4000);
}

// ---------------------------------------------------------------- setup

async function init() {
  const ok = await gfx.load();
  $('noassets').hidden = ok;
  await fillTables();
  const sel = $('table');
  const q = new URLSearchParams(location.search);
  if (q.get('table')) sel.value = q.get('table');
  $('name').value = q.get('player') || localStorage.getItem('mule.name') || '';
  $('join').addEventListener('submit', (e) => {
    e.preventDefault();
    const name = $('name').value.trim().toLowerCase();
    if (!name) return;
    localStorage.setItem('mule.name', name);
    start(sel.value, name);
  });
  if (q.get('table') && $('name').value) start(sel.value, $('name').value);
  addEventListener('keydown', (e) => {
    unlockAudio();
    if (e.target.tagName === 'INPUT') return;
    if (['ArrowUp', 'ArrowDown', 'ArrowLeft', 'ArrowRight', ' '].includes(e.key)) e.preventDefault();
    if (!keys.has(e.key)) pressed.add(e.key);
    keys.add(e.key);
  });
  addEventListener('keyup', (e) => keys.delete(e.key));
  addEventListener('blur', () => keys.clear());
  $('c').addEventListener('click', () => { unlockAudio(); pressed.add(leaveAsk ? 'click' : ' '); });
  $('join').addEventListener('submit', unlockAudio);
  requestAnimationFrame(loop);
}

async function fillTables(selected) {
  const sel = $('table');
  sel.replaceChildren();
  for (const t of await listTables()) {
    const o = document.createElement('option');
    o.value = t.t;
    o.textContent = `${t.t}: ${t.n} (${t.p}/${t.m} ${t.ph})`;
    sel.append(o);
  }
  if (selected) sel.value = selected;
}

function start(table, name) {
  if (api) api.stopped = true;
  leaveAsk = false;
  api = new Api(table, name);
  api.onState(onState);
  $('status').textContent = `${name} @ ${table}`;
  history.replaceState(null, '', `?table=${table}&player=${name}`);
  api.run();
  clearInterval(debugTimer);
  debugTimer = setInterval(binDebug, 1000);
}

// leave: off the table (in a game the computer takes over our seat), back
// to the join form. No request may carry our name afterwards: the server
// would give the seat back (that is how a dropped player rejoins), so the
// poll in flight is let finish first.
async function leave() {
  const a = api;
  leaveAsk = false;
  if (!a) return;
  a.stopped = true;
  api = null;
  clearInterval(debugTimer);
  while (a.busy) await new Promise((r) => setTimeout(r, 20));
  try {
    await fetch(a.url('/leave'), { method: 'POST' });
  } catch (e) {
    console.warn(e);
  }
  log(`left ${a.table}`);
  $('status').textContent = '';
  $('help').textContent = '';
  history.replaceState(null, '', location.pathname);
  await fillTables(a.table);
}

let lastPhase = -1, lastRound = 0, lastEvent = '', lastColony = '';
function onState(v) {
  if (v.err) log('! ' + v.err);
  if (v.p !== lastPhase || v.r !== lastRound) {
    lastPhase = v.p; lastRound = v.r;
    log(`round ${v.r}: ${v.ph}`);
  }
  if (v.tu && v.tu.et && v.tu.et !== lastEvent) { lastEvent = v.tu.et; log(`${v.pl[v.tu.s].n}: ${v.tu.et}`); }
  if (v.co && v.co.tx && v.co.tx !== lastColony) { lastColony = v.co.tx; log('colony: ' + v.co.tx); }
  $('help').textContent = helpLine(v);
}

function helpText(v) {
  switch (v.p) {
    case PHASE.lobby: return 'left/right: species   space: ready   esc: leave the table\nEmpty seats are filled by computer players.';
    case PHASE.grant: return 'space/click: claim the plot under the cursor';
    case PHASE.landauction: return 'hold up to bid (releasing lets your bid drift down)\nspace: skip the for-sale display';
    case PHASE.develop: return 'arrows: walk (diagonals are fastest; mountains and the river slow you down)\n' +
      'space: start turn / act.  Town: corral = buy/return M.U.L.E., shops = outfit, pub = gamble & end turn,\n' +
      'assay = analyse sample, land office = next space on your plot lists it for sale.\n' +
      'Map: space on your plot installs; without a M.U.L.E. it takes an assay sample;\nspace on the wampus catches it.  E: end turn';
    case PHASE.declare: return 'up: seller   down: buyer';
    case PHASE.trading: return 'hold up/down to move your line   C: collude';
    default: return 'space: continue';
  }
}

function helpLine(v) {
  return helpText(v) + (v.p === PHASE.lobby ? '' : '\nesc: leave the game (asks first; the computer takes over your seat)');
}

// ---------------------------------------------------------------- loop (fixed 60 Hz)

let acc = 0, lastT = performance.now();
function loop(now) {
  acc += Math.min(250, now - lastT);
  lastT = now;
  const v = api && api.state;
  if (v && pressed.has('Escape')) {
    if (leaveAsk || v.p === PHASE.lobby) leave();
    else leaveAsk = true;
    pressed.clear();
  } else if (leaveAsk && pressed.size) {
    leaveAsk = false; // any other key: carry on
    pressed.clear();
  }
  let steps = 0;
  while (acc >= 1000 / 60) {
    acc -= 1000 / 60;
    jiffy++;
    if (v && !leaveAsk) simulate(v);
    steps++;
  }
  if (leaveAsk) {
    // the game stands still: its screens see no keys while we ask
    const k = keys, p = pressed;
    keys = new Set();
    pressed = new Set();
    render(v);
    keys = k;
    pressed = p;
    gfx.rect(0, 176, W, 24, 0x00);
    gfx.textCenter('LEAVE THE GAME? ESC AGAIN: YES', 184, 0x0e);
  } else {
    render(v);
  }
  pressed.clear();
  requestAnimationFrame(loop);
}

// simulate runs one jiffy of local game logic (walking, input).
function simulate(v) {
  if (v.p === PHASE.develop) developStep(v);
}

function render(v) {
  gfx.clear(0x00);
  if (!v) {
    gfx.textCenter('M.U.L.E.', 80, 0x0e, true);
    gfx.textCenter('TOURNAMENT SERVER', 100, 0x0c);
    gfx.textCenter('ENTER A NAME AND JOIN', 120, 0x08);
    return;
  }
  try {
    switch (v.p) {
      case PHASE.lobby: return lobby(v);
      case PHASE.grant: return grant(v);
      case PHASE.landauction: case PHASE.declare: case PHASE.trading: return auction(v);
      case PHASE.develop: return develop(v);
      case PHASE.production: return production(v);
      case PHASE.summary: return summary(v, v.r === 0 ? 'STATUS SUMMARY #0' : 'STATUS SUMMARY #' + v.r);
      case PHASE.gameover: return summary(v, 'FINAL STATUS');
    }
  } catch (e) { console.error(e); }
}

function act(path) {
  return api.act(path).then((r) => { if (r.ec) log('! ' + r.err); return r; });
}

// ---------------------------------------------------------------- lobby

function lobby(v) {
  gfx.textCenter('M.U.L.E.', 8, 0x0e, true);
  gfx.textCenter('TOURNAMENT  12 ROUNDS', 24, 0x1a);
  v.pl.forEach((p, i) => {
    const x = 24 + i * 72, y = 48;
    gfx.species(SPECIES[p.ai ? 0 : p.sp], Math.floor(jiffy / 8), x + 20, y, gfx.playerColor(i));
    gfx.text(p.ai ? 'CPU' : p.n.slice(0, 8), x, y + 28, 0x0c);
    gfx.text(p.ai ? 'MECHTRON' : SPECIES[p.sp], x, y + 38, 0x08);
    if (!p.ai) gfx.text(p.rd ? 'READY' : 'WAITING', x, y + 48, p.rd ? 0xc6 : 0x34);
  });
  if (v.cd > 0) {
    gfx.textCenter('STARTING IN ' + Math.ceil((v.cd - api.since()) / 60), 140, 0x0e);
    alertOnce(v, 'lobby');
  } else {
    gfx.textCenter('WAITING FOR PLANETEERS', 140, 0x08);
    if (alerted === 'lobby') alerted = '';
  }
  gfx.textCenter('LEFT/RIGHT: SPECIES  SPACE: READY', 180, 0x08);
  if (v.me < 0) return;
  const me = v.pl[v.me];
  if (pressed.has('ArrowLeft')) act(`/setup/species/${(me.sp + 7) % 8}`);
  if (pressed.has('ArrowRight')) act(`/setup/species/${(me.sp + 1) % 8}`);
  if (pressed.has(' ') || pressed.has('Enter')) act('/ready');
}

// ---------------------------------------------------------------- map

function plotXY(p) { return [MAP_X + (p % COLS) * PS, MAP_Y + Math.floor(p / COLS) * PS]; }

function hash(n) { n = (n * 2654435761) >>> 0; return n ^ (n >>> 13); }

function mountainPos(p, m) {
  const [x, y] = plotXY(p);
  const h = hash(p * 7 + m);
  return [x + 2 + (h % 14), y + 4 + m * 9];
}

function riverX(y) { return plotXY(4)[0] + 12 + Math.round(Math.sin(y / 9) * 3); }

function drawMap(v, opts = {}) {
  gfx.rect(0, 0, W, H, 0x08);
  const m = v.map;
  for (let y = MAP_Y; y < MAP_Y + ROWS * PS; y += 2) {
    const rx = riverX(y);
    for (let i = 0; i < 4; i++) if (hash(y * 5 + i) % 3) gfx.rect(rx + i * 2, y, 1, 1, 0xf4);
  }
  for (let p = 0; p < 45; p++) {
    const [x, y] = plotXY(p);
    if (p === STORE) {
      gfx.rect(x, y, PS, PS, 0x00);
      for (let i = 0; i < 3; i++) gfx.rect(x + 4 + i * 9, y + 4, 6, 4, 0x0c);
      gfx.rect(x + 11, y + 14, 10, 18, 0x0c);
      continue;
    }
    for (let k = 0; k < m.mt[p]; k++) {
      const [mx, my] = mountainPos(p, k);
      gfx.mountain((p + k) % 4, mx, my);
    }
    if (m.ow[p] >= 0 && !(opts.hide === p)) {
      const icon = m.mu[p] >= 0 ? GOODS[m.mu[p]] : 'house';
      gfx.plot(x, y, gfx.playerColor(m.ow[p]), icon);
      if (m.fs[p]) gfx.text('$', x + 2, y + 2, 0x0e);
    }
    if (m.cr[p] >= 0 && !opts.noCrystite) gfx.text(String(m.cr[p]), x + 2, y + 24, 0x86);
    if (opts.prod && opts.prod(p) && m.mu[p] >= 0 && m.ow[p] >= 0) {
      gfx.rect(x + 2, y + 12, 10, 9, 0x00);
      gfx.text(String(m.pr[p]), x + 3, y + 13, m.ls[p] ? 0x34 : 0x0e);
    }
  }
}

function header(s) { gfx.textCenter(s, 2, 0x0c); }

function wrap(s, n = 40) {
  const out = [];
  let line = '';
  for (const w of String(s).toUpperCase().split(' ')) {
    if (line && (line + ' ' + w).length > n) { out.push(line); line = w; } else line = line ? line + ' ' + w : w;
  }
  if (line) out.push(line);
  return out;
}

function statusLine(v, s) {
  gfx.rect(0, H - 24, W, 24, 0x00);
  if (s) gfx.text(s.slice(0, 40), 0, H - 22, 0x1a);
  if (v.me >= 0) {
    const p = v.pl[v.me];
    gfx.text(`$${p.m} F${p.g[0]} E${p.g[1]} S${p.g[2]} C${p.g[3]}`.slice(0, 40), 0, H - 10, gfx.playerColor(v.me));
  }
}

// A vertical time bar at the right edge, 1 pixel per unit (as the original's),
// ticking as it runs out.
function timeBar(units, color = 0x84, tick = true) {
  barTick(tick ? units : 0);
  const h = Math.max(0, Math.min(110, Math.round(units)));
  gfx.rect(W - 8, 176 - h, 4, h, color);
}

// ---------------------------------------------------------------- land grant

function grantable(v, p) { return p !== STORE && v.map.ow[p] < 0; }

// predictCursor advances the server's cursor by the steps due since the last poll.
function predictCursor(v) {
  const g = v.gr;
  if (g.end) return 45;
  let cur = g.cur, since = api.since();
  if (since < g.nx) return cur;
  let steps = 1 + Math.floor((since - g.nx) / g.ctu);
  while (steps-- > 0 && cur < 45) {
    cur++;
    while (cur < 45 && !grantable(v, cur)) cur++;
  }
  return cur;
}

function grant(v) {
  drawMap(v);
  header('LAND GRANT #' + v.r);
  const cur = predictCursor(v);
  const mine = v.me >= 0 ? v.gr.cl[v.me] : -1;
  let msg = mine >= 0 ? 'PLOT CLAIMED.' : 'PRESS YOUR BUTTON TO SELECT A PLOT.';
  if (cur < 0) msg = v.r === 1 && api.elapsed() < T.LANDING ? 'THE SHIP WILL BE BACK IN 12 MONTHS.' : 'LAND GRANT';
  if (cur >= 0 && cur < 45 && !v.gr.end) {
    const [x, y] = plotXY(cur);
    // The cursor box blinks off for a frame as it steps.
    gfx.frame(x, y, PS, PS, 0x0e, 2);
  }
  statusLine(v, msg);
  if (mine < 0 && cur >= 0 && cur < 45) alertOnce(v, 'grant' + v.r);
  if (v.me >= 0 && mine < 0 && cur >= 0 && cur < 45 && pressed.has(' ')) act('/claim/' + cur);
}

// ---------------------------------------------------------------- development

const TOWN_BUILDINGS = () => (gfx.m ? gfx.m.town.buildings : {
  food: [1, 1, 4, 4], energy: [7, 1, 4, 4], smithore: [13, 1, 4, 4], crystite: [19, 1, 4, 4],
  assay: [1, 9, 4, 7], land: [6, 9, 4, 6], pub: [11, 9, 4, 5], corral: [17, 9, 6, 9] });

function turnKey(v) { return `${v.r}:${v.tu.s}`; }

// terrainDelay returns the jiffies until the next step at the colonist's feet.
function terrainDelay(v, x, y) {
  const fx = x + 4, fy = y + 8, fw = 8, fh = 3;
  for (let p = 0; p < 45; p++) {
    for (let k = 0; k < v.map.mt[p]; k++) {
      const [mx, my] = mountainPos(p, k);
      if (fx < mx + 16 && fx + fw > mx && fy < my + 6 && fy + fh > my) return T.MOUNTAIN;
    }
  }
  const rx = riverX(fy);
  if (fx < rx + 8 && fx + fw > rx) return T.RIVER;
  return T.MOVE;
}

function myTurnActive(v) {
  const t = v.tu;
  return t && v.me === t.s && t.st && !t.en && !t.ai;
}

// developStep moves the colonist one jiffy (called at 60 Hz).
function developStep(v) {
  const t = v.tu;
  if (!t) return;
  if (walk.turnKey !== turnKey(v)) {
    // Each turn starts in the middle of town. [measured]
    Object.assign(walk, { view: 'town', x: TOWN_X + 88, y: TOWN_Y + 39, sample: -1, landMode: false,
      inside: '', turnKey: turnKey(v), next: 0, freeze: 0 });
  }
  if (!myTurnActive(v)) return;
  if (walk.freeze > 0) { walk.freeze--; return; }
  let dx = 0, dy = 0;
  if (keys.has('ArrowLeft')) dx -= 1;
  if (keys.has('ArrowRight')) dx += 1;
  if (keys.has('ArrowUp')) dy -= 1;
  if (keys.has('ArrowDown')) dy += 1;
  if (!dx && !dy) return;
  // Steps happen on the game's 6-jiffy movement tick, delayed on rough terrain.
  if (jiffy < walk.next || (walk.next === 0 && jiffy % T.MOVE)) return;
  if (walk.view === 'town') {
    walk.x += dx * T.TOWN_DX;
    walk.y += dy * T.TOWN_DY;
    walk.next = jiffy + T.MOVE;
  } else {
    walk.x += dx * T.MAP_DX;
    walk.y += dy * T.MAP_DY;
    walk.next = jiffy + terrainDelay(v, walk.x, walk.y);
  }
  walk.frame++;
}

function develop(v) {
  const t = v.tu;
  if (!t) return drawMap(v);
  const mine = v.me === t.s;
  const who = v.pl[t.s];
  if (!mine || !t.st || t.en || t.ai) {
    drawMap(v);
    header('DEVELOPMENT #' + v.r);
    let msg = `${who.n.toUpperCase()}'S TURN`;
    if (t.ai) msg = 'THE COMPUTER IS TAKING HIS TURN';
    if (mine && !t.st) {
      msg = 'PRESS YOUR STICK BUTTON TO START';
      alertOnce(v, 'turn' + turnKey(v));
    }
    if (t.en && !t.ai) msg = 'TIME HAS RUN OUT';
    if (t.en && (t.lg || []).includes('pub')) msg = 'GAMBLING AT THE PUB';
    statusLine(v, msg);
    if (t.ai) timeBar((t.ei - api.since()) / (t.ptu || 28));
    else if (t.st && !t.en) timeBar((t.lf - api.since()) / t.ptu);
    if (mine && !t.st && pressed.has(' ')) act('/turn/begin');
    return;
  }
  const left = Math.max(0, t.lf - api.since());
  if (walk.view === 'map') mapTurn(v, t, left); else townTurn(v, t, left);
  timeBar(left / t.ptu);
  if (t.et && t.el + api.since() < 400) {
    const lines = wrap(t.et);
    gfx.rect(0, 12, W, lines.length * 10 + 4, 0x00);
    lines.forEach((l, i) => gfx.text(l, 0, 14 + i * 10, 0x1a));
  }
  if (pressed.has('e') || pressed.has('E')) act('/turn/end');
}

function carryIcon(t) { return t.hm ? (t.cy >= 0 ? GOODS[t.cy] : 'mule') : ''; }

function mapTurn(v, t, left) {
  walk.x = Math.max(14, Math.min(298, walk.x));
  walk.y = Math.max(14, Math.min(166, walk.y));
  drawMap(v);
  header('DEVELOPMENT #' + v.r);
  const rel = t.el + api.since();
  let wampus = null;
  for (const w of t.wp || []) {
    if (!t.ct && !t.hm && rel >= w.f && rel < w.t) {
      const [mx, my] = mountainPos(w.p, w.m);
      wampus = [mx + 8, my + 2];
      if (Math.floor(jiffy / 8) % 2) gfx.rect(mx + 6, my, 4, 4, 0x0e);
    }
  }
  const cx = walk.x + 8, cy = walk.y + 5;
  const plot = Math.floor((cx - MAP_X) / PS) + Math.floor((cy - MAP_Y) / PS) * COLS;
  const inMap = cx >= MAP_X && cx < MAP_X + COLS * PS && cy >= MAP_Y && cy < MAP_Y + ROWS * PS;
  if (inMap && plot === STORE) {
    // Enter the town on the side we came from.
    const [sx, sy] = plotXY(STORE);
    walk.view = 'town';
    walk.freeze = T.TOWN_EXIT;
    if (cx < sx + 8) { walk.x = TOWN_X + 4; walk.y = TOWN_Y + 39; }
    else if (cx > sx + PS - 8) { walk.x = TOWN_X + 172; walk.y = TOWN_Y + 39; }
    else if (cy < sy + 16) { walk.x = TOWN_X + 88; walk.y = TOWN_Y + 4; }
    else { walk.x = TOWN_X + 88; walk.y = TOWN_Y + 118; }
    walk.inside = '';
    return;
  }
  if (t.hm) gfx.mule(walk.x - 14, walk.y + 2, gfx.playerColor(v.me));
  gfx.species(SPECIES[v.pl[v.me].sp], walk.frame, walk.x, walk.y, gfx.playerColor(v.me), true);
  statusLine(v, timeLine(t));
  if (!pressed.has(' ') || !inMap) return;
  if (wampus && Math.abs(wampus[0] - cx) < 6 && Math.abs(wampus[1] - cy) < 6) {
    act('/turn/wampus').then((r) => { if (!r.ec) log(`caught the wampus! +$${r.res}`); });
  } else if (walk.landMode && v.map.ow[plot] === v.me) {
    walk.landMode = false;
    act((v.map.fs[plot] ? '/turn/unsell/' : '/turn/sell/') + plot);
  } else if (t.hm) {
    act('/turn/install/' + plot).then((r) => { if (r.res === 1) log('your M.U.L.E. ran away!'); });
  } else if (walk.sample < 0) {
    walk.sample = plot;
    log(`took an assay sample from plot ${plot}`);
  }
}

function timeLine(t) {
  const carry = carryIcon(t);
  return `${carry.toUpperCase()}${walk.sample >= 0 ? ' SAMPLE' : ''}`;
}

function townTurn(v, t) {
  gfx.rect(0, 0, W, H, 0x08);
  header('DEVELOPMENT #' + v.r);
  gfx.town(TOWN_X, TOWN_Y);
  const b = TOWN_BUILDINGS();
  const [cx0, cy0] = [TOWN_X + b.corral[0] * 8, TOWN_Y + b.corral[1] * 8];
  for (let i = 0; i < v.st.mu; i++) gfx.mule(cx0 + 4 + (i % 2) * 22, cy0 + 4 + Math.floor(i / 2) * 7, gfx.col(0x00));
  gfx.text('$' + v.st.mp, cx0 + 4, cy0 + 56, 0x00);
  const cx = walk.x + 8, cy = walk.y + 11;
  if (cx < TOWN_X || cx > TOWN_X + 192 || cy < TOWN_Y || cy > TOWN_Y + 144) {
    // Leave town next to the store plot on the side we walked out of. [measured: 25 jiffies]
    const [sx, sy] = plotXY(STORE);
    walk.view = 'map';
    walk.freeze = T.TOWN_EXIT;
    if (cx < TOWN_X) { walk.x = sx - 18; walk.y = sy + 9; }
    else if (cx > TOWN_X + 192) { walk.x = sx + PS + 2; walk.y = sy + 9; }
    else if (cy < TOWN_Y) { walk.x = sx + 8; walk.y = sy - 12; }
    else { walk.x = sx + 8; walk.y = sy + PS + 2; }
    return;
  }
  let inside = '';
  for (const [name, r] of Object.entries(b)) {
    const x = TOWN_X + r[0] * 8, y = TOWN_Y + r[1] * 8;
    if (cx >= x && cx < x + r[2] * 8 && cy >= y && cy < y + r[3] * 8) inside = name;
  }
  if (inside && inside !== walk.inside) enterBuilding(v, t, inside);
  walk.inside = inside;
  if (t.hm) gfx.mule(walk.x - 14, walk.y + 14, gfx.playerColor(v.me));
  gfx.species(SPECIES[v.pl[v.me].sp], walk.frame, walk.x, walk.y, gfx.playerColor(v.me));
  gfx.rect(TOWN_X, TOWN_Y + 144, 192, 10, 0x08);
  gfx.text(`YOUR MONEY EQUALS $${v.pl[v.me].m}`, TOWN_X - 8, TOWN_Y + 146, 0x00);
  statusLine(v, timeLine(t));
}

function enterBuilding(v, t, name) {
  if (GOODS.includes(name)) {
    if (t.hm) act('/turn/outfit/' + name).then((r) => { if (!r.ec) log('outfitted for ' + name); });
    else log('buy a M.U.L.E. at the corral first');
    return;
  }
  switch (name) {
    case 'corral':
      if (t.hm) act('/turn/return').then((r) => { if (!r.ec) log('M.U.L.E. returned'); });
      else act('/turn/buymule').then((r) => { if (!r.ec) log('bought a M.U.L.E.'); });
      break;
    case 'pub':
      act('/turn/pub').then((r) => { if (!r.ec) log(`won $${r.res} gambling!`); });
      break;
    case 'assay':
      if (walk.sample >= 0) {
        const p = walk.sample;
        walk.sample = -1;
        act('/turn/assay/' + p).then((r) => { if (!r.ec) log(`${['no', 'low', 'medium', 'high', 'very high'][r.res]} crystite found in plot ${p}.`); });
      } else log('take a sample from a plot first (space on the map without a M.U.L.E.)');
      break;
    case 'land':
      walk.landMode = true;
      log('land office: press space on one of your plots to list it for sale');
      break;
  }
}

// ---------------------------------------------------------------- production

function production(v) {
  // Plots report one after another over the production display. [measured 1421 jiffies]
  const e = api.elapsed();
  const shown = Math.floor(e / (T.PRODUCTION / 48));
  drawMap(v, { prod: (p) => p <= shown });
  header('PRODUCTION #' + v.r);
  statusLine(v, v.co && v.co.tx ? '' : 'PRODUCTION');
  if (v.co && v.co.tx) {
    const lines = wrap(v.co.tx);
    gfx.rect(0, H - 24 - lines.length * 10, W, lines.length * 10, 0x00);
    lines.forEach((l, i) => gfx.text(l, 0, H - 22 - lines.length * 10 + i * 10, 0x1a));
  }
  if (v.co && v.co.pt >= 0 && v.co.pt < 45) {
    const [x, y] = plotXY(v.co.pt);
    if (Math.floor(jiffy / 15) % 2) gfx.frame(x, y, PS, PS, 0x34, 2);
  }
}

// ---------------------------------------------------------------- auctions

// Line y on screen: the store's buy line (or the land start price) is scanline
// 137 and each price step is 2 scanlines, as on the original.
function lineY(a, price) {
  const base = a.land ? a.sp : a.sb;
  return 109 - Math.round((price - base) / a.stp) * 2;
}

// at: the price my line was released at, walked to until the server's
// answer (which carries on to it) has come back.
const localPush = { dir: 0, at: -1, atUntil: 0 };

// predictLine moves a line toward its target at one step per LineStep jiffies.
function predictLine(a, l, seat, v) {
  let target = l.t;
  if (seat === v.me && a.sg === STAGE.active) {
    if (localPush.dir > 0) target = a.hi;
    else if (localPush.dir < 0) target = a.lo;
    else if (localPush.at >= 0 && jiffy < localPush.atUntil) target = localPush.at;
  }
  const steps = Math.floor(api.since() / a.lj);
  let pos = l.p;
  for (let i = 0; i < steps && pos !== target; i++) pos += Math.sign(target - pos) * a.stp;
  return pos;
}

// predictTicks returns the time bar length with sub-tick progress.
function predictTicks(a) {
  if (a.sg !== STAGE.active && a.sg !== STAGE.declare) return a.tk;
  const rate = a.sg === STAGE.declare || a.idl ? 6 : 25;
  const since = api.since();
  if (since < a.tl) return a.tk;
  return Math.max(0, a.tk - 1 - Math.floor((since - a.tl) / rate));
}

function auction(v) {
  const a = v.au;
  if (!a) return;
  if (a.land && a.sg === STAGE.show) {
    // The plot for sale blinks on the map until everyone presses (or 255 BTU pass).
    drawMap(v, { hide: Math.floor(jiffy / 20) % 2 ? a.pt : -1 });
    header(`LAND AUCTION #${v.r}`);
    if (Math.floor(jiffy / 20) % 2) { const [x, y] = plotXY(a.pt); gfx.frame(x, y, PS, PS, 0x0e, 2); }
    statusLine(v, a.sl >= 0 ? `${v.pl[a.sl].n} IS SELLING THIS PLOT` : 'THE COLONY IS SELLING THIS PLOT');
    if (pressed.has(' ')) act('/ready/1');
    return;
  }
  if (!a.land && a.sg === STAGE.show) return goodsStatus(v, a);
  gfx.clear(a.col ? 0x46 : 0x00);
  header(a.land ? `LAND AUCTION #${v.r}` : `AUCTION #${v.r}  ${GOODS[a.gd].toUpperCase()}`);
  if (!a.land) {
    gfx.rect(16, lineY(a, a.ss), 280, 1, 0x86);
    gfx.text('STORE SELLS $' + a.ss, 170, lineY(a, a.ss) - 9, 0x86);
    gfx.rect(16, lineY(a, a.sb), 280, 1, 0x1a);
    gfx.text('STORE BUYS $' + a.sb, 170, lineY(a, a.sb) + 2, 0x1a);
    gfx.text(`STORE HAS ${v.st.s[a.gd]} UNITS`, 16, 12, 0x08);
  } else {
    gfx.text(`PLOT ${a.pt % 9 + 1},${Math.floor(a.pt / 9) + 1} ${a.sl >= 0 ? 'SOLD BY ' + v.pl[a.sl].n : 'COLONY'}`.toUpperCase(), 16, 12, 0x08);
    gfx.rect(16, lineY(a, a.sp), 280, 1, 0x1a);
    gfx.text('$' + a.sp, 260, lineY(a, a.sp) + 2, 0x1a);
    if (a.sl >= 0) { gfx.rect(16, lineY(a, a.ak), 280, 1, 0x86); gfx.text('ASK $' + a.ak, 250, lineY(a, a.ak) - 9, 0x86); }
  }
  a.ln.forEach((l, i) => {
    const x = 32 + i * 64;
    const p = v.pl[i];
    const col = gfx.playerColor(i);
    if (a.sg === STAGE.declare || a.sg === STAGE.lead) {
      const role = l.r === 1 ? 'BUY' : l.r === 2 ? 'SELL' : '';
      const y = l.r === 2 ? 30 : 130;
      gfx.species(SPECIES[p.sp], 0, x, y, col);
      gfx.text(role, x, y + 24, col);
    } else if (l.r) {
      const pos = predictLine(a, l, i, v);
      const y = lineY(a, pos);
      gfx.rect(x - 8, y, 32, 1, col);
      const moving = pos !== l.t;
      gfx.species(SPECIES[p.sp], moving ? Math.floor(jiffy / 8) : 0, x, l.r === 2 ? y - 22 : y + 1, col);
      gfx.text('$' + pos, x + 16, l.r === 2 ? y - 9 : y + 2, col);
    }
    gfx.text(p.n.slice(0, 7), x - 8, 172, col);
    gfx.text(a.land ? '$' + p.m : `${p.g[a.gd] ?? 0}${l.u ? (l.u > 0 ? '+' : '') + l.u : ''}`, x - 8, 182, col);
  });
  timeBar(predictTicks(a), 0x0e, a.sg === STAGE.active || a.sg === STAGE.declare);
  let msg = '';
  if (a.sg === STAGE.declare) msg = 'PUSH STICK TO DECLARE AS BUYER OR SELLER';
  else if (a.sg === STAGE.lead) msg = 'GET READY...';
  if (a.sg === STAGE.declare || a.sg === STAGE.lead) alertOnce(v, `auction${v.r}:${v.p}:${a.gd}:${a.pt}:${a.sg}`);
  else if (a.dn) msg = a.land ? (a.w >= 0 ? `${v.pl[a.w].n} BOUGHT IT FOR $${a.pc}` : 'NO SALE') : 'AUCTION IS OVER';
  else if (a.cr >= 0) msg = 'SELLER AT CRITICAL LEVEL!';
  else if (a.col) msg = 'COLLUSION!';
  gfx.rect(0, H - 14, W, 14, 0x00);
  gfx.text(msg.toUpperCase().slice(0, 40), 0, H - 10, 0x1a);
  if (v.me < 0) return;
  if (a.sg === STAGE.declare) {
    if (pressed.has('ArrowUp')) act('/declare/s');
    if (pressed.has('ArrowDown')) act('/declare/b');
    return;
  }
  const dir = keys.has('ArrowUp') ? 1 : keys.has('ArrowDown') ? -1 : 0;
  if (a.sg === STAGE.active && dir !== localPush.dir) {
    if (dir === 0) {
      // Released: the line stays where I saw it stop.
      const at = predictLine(a, a.ln[v.me], v.me, v);
      localPush.dir = 0;
      localPush.at = at;
      localPush.atUntil = jiffy + 60;
      act(`/auction/push/hold/${a.tr}/${at}`);
    } else {
      localPush.dir = dir;
      act('/auction/push/' + dir);
    }
  }
  if (pressed.has('c') || pressed.has('C')) act('/collude');
}

function goodsStatus(v, a) {
  const gd = a.gd;
  header(`STATUS #${v.r}  ${GOODS[gd].toUpperCase()}`);
  gfx.text('         UNITS  PROD  SPOIL', 16, 24, 0x08);
  const e = api.elapsed();
  v.pl.forEach((p, i) => {
    if (e < 40 + i * 20) return;
    const y = 40 + i * 20;
    gfx.species(SPECIES[p.sp], 0, 0, y - 8, gfx.playerColor(i), true);
    gfx.text(`${p.n.slice(0, 8).padEnd(8)} ${String(p.g[gd]).padStart(5)} ${String(p.pr[gd]).padStart(5)} ${String(p.sl[gd]).padStart(6)}`, 16, y, gfx.playerColor(i));
  });
  gfx.text(`STORE HAS ${v.st.s[gd]} UNITS`, 16, 140, 0x0c);
  gfx.text(`PRICE $${v.st.pr[gd]}`, 16, 152, 0x0c);
}

// ---------------------------------------------------------------- summary

function summary(v, title) {
  header(title);
  // The summary is drawn over SummaryAnim jiffies, then waits for every button.
  const e = api.elapsed();
  gfx.text('          MONEY  GOODS   LAND  TOTAL', 0, 20, 0x08);
  const land = (i) => (v.map ? v.map.ow.reduce((s, o, p) => s + (o === i ? 500 + (v.map.mu[p] >= 0 ? [25, 50, 75, 100][v.map.mu[p]] + 35 : 0) : 0), 0) : 0);
  [...v.pl.keys()].sort((a, b) => v.pl[a].rk - v.pl[b].rk).forEach((i, row) => {
    if (v.p === PHASE.summary && e < row * (T.SUMMARY_ANIM / 5)) return;
    const p = v.pl[i];
    const l = land(i), gd = p.sc - p.m - l;
    const y = 36 + row * 20;
    gfx.species(SPECIES[p.sp], 0, 0, y - 6, gfx.playerColor(i), true);
    gfx.text(`${p.n.slice(0, 8).padEnd(8)} ${String(p.m).padStart(6)} ${String(gd).padStart(6)} ${String(l).padStart(6)} ${String(p.sc).padStart(6)}`, 16, y, gfx.playerColor(i));
  });
  const tot = v.pl.reduce((s, p) => s + p.sc, 0);
  if (v.p === PHASE.gameover || e >= T.SUMMARY_ANIM * 4 / 5) gfx.text(`COLONY TOTAL ${tot}`, 16, 124, 0x0e);
  if (v.co && v.p === PHASE.gameover) {
    wrap(v.co.rtx || '', 39).forEach((l, i) => gfx.text(l, 4, 144 + i * 10, 0x1a));
    if (v.co.w >= 0) gfx.text(`${v.pl[v.co.w].n} IS THE FIRST FOUNDER`.toUpperCase(), 4, 196, 0x0e);
    return;
  }
  if (e >= T.SUMMARY_ANIM) {
    const me = v.me >= 0 ? v.pl[v.me] : null;
    gfx.text(me && me.rd ? 'WAITING FOR THE OTHER PLAYERS' : 'PRESS ALL PLAYER BUTTONS TO GO ON', 16, 190, 0x08);
    alertOnce(v, 'summary' + v.r);
    if (pressed.has(' ')) act('/ready/1');
  }
}

// ---------------------------------------------------------------- binary debug

async function binDebug() {
  if (!api || !$('debug').open || !$('binmode').checked) return;
  const b = await api.raw('/state', '&bin=1');
  const d = decode(b);
  const j = api.state;
  const checks = [
    ['size', d.size === b.length],
    ['phase', j && d.h.phase === j.p],
    ['round', j && d.h.round === j.r],
    ['money[0]', j && d.players[0].money === Math.min(65535, j.pl[0].m)],
    ['owner[]', j && !j.map || d.map.owner.every((o, i) => o === j.map.ow[i])],
  ];
  $('bininfo').textContent = `${b.length} bytes  magic=${String.fromCharCode(d.h.magic)} v${d.h.version} phase=${d.h.phase} ` +
    `round=${d.h.round} seat=${d.h.mySeat} t=${d.h.t}j\n` + checks.map(([k, ok]) => `${ok ? 'ok  ' : 'DIFF'} ${k}`).join('\n') +
    '\nplayers: ' + d.players.map((p) => `${p.name}:$${p.money}`).join(' ');
  $('hex').textContent = hexdump(b, 128);
}

// Test hook (used by the browser timing checks).
window.__mule = { walk, T, get api() { return api; }, get jiffy() { return jiffy; } };

init();
