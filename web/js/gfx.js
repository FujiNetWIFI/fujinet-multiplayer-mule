// Drawing helpers using the assets extracted from the Atari disk (web/assets).
// Falls back to plain shapes when the assets have not been generated.

export const W = 320, H = 208;

// Fallback palette entries (Atari NTSC) for when no manifest exists.
const FALLBACK = { 0x00: '#000000', 0x04: '#4f4f4f', 0x08: '#7a7a7a', 0x0c: '#b0b0b0', 0x0e: '#ffffff', 0x84: '#4a4ae0',
  0x54: '#c040a0', 0xc4: '#40a040', 0x34: '#d06030', 0xf4: '#a0a040', 0x24: '#806020', 0xb4: '#207860', 0x44: '#b04040',
  0xd4: '#408030', 0xc6: '#50b050', 0x1a: '#e0c040', 0x06: '#909090', 0x86: '#6060ff', 0x96: '#314f9e' };

export class Gfx {
  constructor(canvas) {
    this.c = canvas;
    this.g = canvas.getContext('2d');
    this.g.imageSmoothingEnabled = false;
    this.m = null;
    this.img = {};
    this.tints = new Map();
  }

  async load() {
    try {
      const r = await fetch('assets/manifest.json');
      if (!r.ok) throw new Error('no manifest');
      this.m = await r.json();
      const names = ['font', 'mapchars', 'town', 'mountains', 'species'];
      await Promise.all(names.map((n) => new Promise((ok, bad) => {
        const im = new Image();
        im.onload = () => { this.img[n] = im; ok(); };
        im.onerror = bad;
        im.src = 'assets/' + this.m[n].file;
      })));
      return true;
    } catch (e) {
      this.m = null;
      return false;
    }
  }

  col(atari) {
    if (typeof atari === 'string') return atari;
    if (this.m) return this.m.palette[atari & 0xff];
    return FALLBACK[atari] || '#888';
  }

  playerColor(seat) {
    const pc = this.m ? this.m.colors.players : [0x84, 0x54, 0xc4, 0x34];
    return this.col(pc[seat] ?? 0x0e);
  }

  clear(atari = 0x00) {
    this.g.fillStyle = this.col(atari);
    this.g.fillRect(0, 0, W, H);
  }

  rect(x, y, w, h, atari) {
    this.g.fillStyle = this.col(atari);
    this.g.fillRect(x, y, w, h);
  }

  frame(x, y, w, h, atari, t = 1) {
    this.rect(x, y, w, t, atari); this.rect(x, y + h - t, w, t, atari);
    this.rect(x, y, t, h, atari); this.rect(x + w - t, y, t, h, atari);
  }

  // tinted copy of a white-on-transparent sheet
  tinted(name, color) {
    const key = name + color;
    let t = this.tints.get(key);
    if (!t) {
      const im = this.img[name];
      t = document.createElement('canvas');
      t.width = im.width; t.height = im.height;
      const g = t.getContext('2d');
      g.drawImage(im, 0, 0);
      g.globalCompositeOperation = 'source-in';
      g.fillStyle = color;
      g.fillRect(0, 0, t.width, t.height);
      this.tints.set(key, t);
    }
    return t;
  }

  // Text in the Atari font. scale 1 = mode 2 (8x8), wide = mode 6 (16x8).
  text(s, x, y, atari = 0x0c, wide = false) {
    s = String(s).toUpperCase();
    const cw = wide ? 16 : 8;
    if (!this.img.font) {
      this.g.fillStyle = this.col(atari);
      this.g.font = '8px monospace';
      this.g.textBaseline = 'top';
      for (let i = 0; i < s.length; i++) this.g.fillText(s[i], x + i * cw, y);
      return;
    }
    const sheet = this.tinted('font', this.col(atari));
    for (let i = 0; i < s.length; i++) {
      let code = s.charCodeAt(i);
      if (code < 32 || code > 95) code = 32;
      code -= 32;
      this.g.drawImage(sheet, (code % 16) * 8, Math.floor(code / 16) * 8, 8, 8, x + i * cw, y, cw, 8);
    }
  }

  textCenter(s, y, atari, wide = false) {
    const cw = wide ? 16 : 8;
    this.text(s, Math.floor((W - String(s).length * cw) / 2), y, atari, wide);
  }

  // Map character (mode 6: 8x8 glyph drawn 16x8).
  mapChar(code, x, y, color) {
    if (!this.img.mapchars) return;
    const sheet = this.tinted('mapchars', color);
    this.g.drawImage(sheet, (code % 16) * 8, Math.floor(code / 16) * 8, 8, 8, x, y, 16, 8);
  }

  // A 32x32 plot frame with an icon, as the Atari map draws it.
  plot(x, y, color, icon) {
    const mc = this.m && this.m.mapchars;
    if (!mc) {
      this.frame(x + 1, y + 1, 30, 30, color, 2);
      if (icon) { this.g.fillStyle = color; this.g.font = '8px monospace'; this.g.fillText(icon[0].toUpperCase(), x + 12, y + 12); }
      return;
    }
    for (let r = 0; r < 4; r++) this.mapChar(mc.left[r], x, y + r * 8, color);
    this.mapChar(mc.rightTop, x + 16, y, color);
    this.mapChar(mc.rightBottom, x + 16, y + 24, color);
    if (icon && mc.icons[icon]) {
      this.mapChar(mc.icons[icon][0], x + 16, y + 8, color);
      this.mapChar(mc.icons[icon][1], x + 16, y + 16, color);
    }
  }

  mountain(kind, x, y) {
    if (!this.img.mountains) {
      this.g.fillStyle = this.col(0x04);
      this.g.beginPath(); this.g.moveTo(x, y + 6); this.g.lineTo(x + 8, y); this.g.lineTo(x + 16, y + 6); this.g.fill();
      return;
    }
    this.g.drawImage(this.img.mountains, kind * 8, 0, 8, 6, x, y, 16, 6);
  }

  // Species sprite: 8x22 player graphics drawn double width.
  species(name, frame, x, y, color, half = false) {
    const sp = this.m && this.m.species;
    if (!sp || !sp.frames[name]) {
      this.rect(x + 4, y + (half ? 0 : 4), 8, half ? 11 : 18, color);
      return;
    }
    const list = sp.frames[name];
    const f = list[((frame % list.length) + list.length) % list.length];
    const sheet = this.tinted('species', color);
    this.g.drawImage(sheet, f * 8, 0, 8, 22, x, y, 16, half ? 11 : 22);
  }

  // The M.U.L.E. (drawn from a small bitmap; the original is multi-player art).
  mule(x, y, color) {
    const rows = ['0000000000001100', '0000000000011110', '0111111111111100', '1111111111111000',
      '0111111111110000', '0110000001100000', '0110000001100000', '0110000001100000'];
    this.g.fillStyle = color;
    rows.forEach((r, j) => { for (let i = 0; i < 16; i++) if (r[i] === '1') this.g.fillRect(x + i, y + j, 1, 1); });
  }

  town(x, y) {
    if (this.img.town) {
      this.g.drawImage(this.img.town, x, y);
      return;
    }
    this.rect(x, y, 192, 144, 0x00);
  }
}
