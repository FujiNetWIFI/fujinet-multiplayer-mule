// Server API: adaptive polling with ?hash=, action calls, and a jiffy clock
// (1/60 s, the original game's frame rate) synced to the server.

export class Api {
  constructor(table, player) {
    this.table = table;
    this.player = player;
    this.state = null;
    this.hash = '';
    this.recvAt = 0; // performance.now() when state.t was received
    this.listeners = [];
    this.busy = false;
    this.stopped = false;
    this.offset = null; // smoothed (server jiffy - local jiffy)
  }

  url(path, extra = '') {
    return `${path}?table=${encodeURIComponent(this.table)}&player=${encodeURIComponent(this.player)}${extra}`;
  }

  onState(fn) { this.listeners.push(fn); }

  localJiffy() { return performance.now() * 60 / 1000; }

  sync(t) {
    // Smooth the clock offset so jitter in response times doesn't make animations jump.
    const off = t - this.localJiffy();
    if (this.offset === null || Math.abs(off - this.offset) > 30) this.offset = off;
    else this.offset += (off - this.offset) * 0.1;
  }

  accept(v, hash) {
    if (v.same) {
      if (this.state) {
        this.state.t = v.t;
        this.state.pe = v.pe;
        this.recvAt = performance.now();
      }
      this.sync(v.t);
      return;
    }
    this.state = v;
    this.fullT = v.t;
    this.recvAt = performance.now();
    this.sync(v.t);
    if (hash) this.hash = hash;
    for (const fn of this.listeners) fn(v);
  }

  async poll() {
    if (this.busy) return;
    this.busy = true;
    try {
      const r = await fetch(this.url('/state', `&hash=${this.hash || 0}`));
      this.accept(await r.json(), r.headers.get('X-Mule-Hash'));
    } catch (e) {
      console.warn(e);
    } finally {
      this.busy = false;
    }
  }

  // Poll at the interval the server suggests for the current phase.
  run() {
    const loop = async () => {
      if (this.stopped) return;
      await this.poll();
      const ms = (this.state && this.state.poll) || 250;
      setTimeout(loop, ms);
    };
    loop();
  }

  async act(path) {
    const r = await fetch(this.url(path), { method: 'POST' });
    const v = await r.json();
    this.hash = '';
    this.accept(v);
    return v;
  }

  async raw(path, extra) {
    const r = await fetch(this.url(path, extra));
    return new Uint8Array(await r.arrayBuffer());
  }

  // Server clock now, in jiffies.
  now() {
    if (!this.state) return 0;
    if (this.offset === null) return this.state.t;
    return this.localJiffy() + this.offset;
  }

  // Jiffies since the last state was received.
  since() {
    if (!this.state) return 0;
    return Math.max(0, this.now() - this.state.t);
  }

  // Jiffies since the last full state was built (an unchanged reply moves
  // the clock on but carries no countdowns).
  sinceFull() {
    if (!this.state) return 0;
    return Math.max(0, this.now() - this.fullT);
  }

  // Phase-elapsed jiffies, extrapolated.
  elapsed() {
    if (!this.state) return 0;
    return this.state.pe + this.since();
  }
}

export async function listTables() {
  const r = await fetch('/tables');
  return r.json();
}
