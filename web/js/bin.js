// Decoder for the ?bin=1 state (see clients/mule_proto.h). Used by the debug
// panel to check the binary protocol against the JSON one.

const u8 = (n = 0) => ({ t: 'u8', n });
const i8 = (n = 0) => ({ t: 'i8', n });
const u16 = (n = 0) => ({ t: 'u16', n });
const i16 = (n = 0) => ({ t: 'i16', n });
const u32 = () => ({ t: 'u32', n: 0 });
const str = (n) => ({ t: 'str', n });
const arr = (s, n) => ({ t: 'arr', s, n });

const Header = { magic: u8(), version: u8(), msgType: u8(), phase: u8(), round: u8(), mySeat: i8(), err: u8(), size: u8(),
  t: u16(), elapsed: u16(), countdown: i16(), result: i16() };
const Player = { name: str(9), species: u8(), color: u8(), flags: u8(), money: u16(), goods: u8(4), score: u16(), rank: u8(),
  prod: u8(4), spoil: u8(4) };
const Map_ = { terrain: u8(45), owner: i8(45), mule: i8(45), crystite: i8(45), prod: u8(45) };
const Store = { stock: u8(4), price: u16(4), buy: u16(4), sell: u16(4), mules: u8(), mulePrice: u16() };
const Grant = { cursor: i8(), claimed: i8(4), step: u8(), next: u16() };
const Auction = { kind: u8(), good: i8(), plot: i8(), seller: i8(), flags: u8(), winner: i8(), colluders: u8(), stage: u8(),
  step: u8(), traded: u8(), start: u16(), ask: u16(), lo: u16(), hi: u16(), storeBuy: u16(), storeSell: u16(), price: u16(),
  lastTrade: u16(), stageLeft: u16(), ticks: u8(), tickLeft: u8(), role: u8(4), pos: u16(4), target: u16(4), units: i8(4),
  push: i8(4) };
const Wampus = { plot: u8(), mount: u8(), from: u16(), to: u16() };
const Turn = { seat: i8(), flags: u8(), carrying: i8(), event: u8(), length: u16(), elapsed: u16(), left: u16(), endIn: u16(),
  ptu: u8(), nWampus: u8(), wampus: arr(Wampus, 12) };
const Colony = { event: u8(), plot: i8(), rating: u8(), winner: i8(), failed: u8(), pad: u8(), total: u32() };
export const State = { h: arr(Header, 1), players: arr(Player, 4), map: arr(Map_, 1), store: arr(Store, 1), grant: arr(Grant, 1),
  auction: arr(Auction, 1), turn: arr(Turn, 1), colony: arr(Colony, 1) };

export function decode(bytes, schema = State, le = true) {
  const dv = new DataView(bytes.buffer, bytes.byteOffset, bytes.byteLength);
  let o = 0;
  function one(f) {
    switch (f.t) {
      case 'u8': return dv.getUint8(o++);
      case 'i8': return dv.getInt8(o++);
      case 'u16': { const v = dv.getUint16(o, le); o += 2; return v; }
      case 'i16': { const v = dv.getInt16(o, le); o += 2; return v; }
      case 'u32': { const v = dv.getUint32(o, le); o += 4; return v; }
    }
  }
  function field(f) {
    if (f.t === 'str') {
      const s = bytes.subarray(o, o + f.n);
      o += f.n;
      return new TextDecoder().decode(s.subarray(0, s.indexOf(0) < 0 ? f.n : s.indexOf(0)));
    }
    if (f.t === 'arr') {
      const out = [];
      for (let i = 0; i < f.n; i++) out.push(struct(f.s));
      return f.n === 1 ? out[0] : out;
    }
    if (f.n) return Array.from({ length: f.n }, () => one(f));
    return one(f);
  }
  function struct(s) {
    const r = {};
    for (const [k, f] of Object.entries(s)) r[k] = field(f);
    return r;
  }
  const r = struct(schema);
  r.size = o;
  return r;
}

export function hexdump(bytes, max = 256) {
  let s = '';
  for (let i = 0; i < Math.min(bytes.length, max); i += 16) {
    const row = Array.from(bytes.subarray(i, i + 16));
    s += i.toString(16).padStart(4, '0') + '  ' + row.map((b) => b.toString(16).padStart(2, '0')).join(' ') + '\n';
  }
  if (bytes.length > max) s += `... ${bytes.length} bytes\n`;
  return s;
}
