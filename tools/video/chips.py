#!/usr/bin/env python3
"""The intro tune (chip.py's SONG) for more sound chips.

usage: chips.py CHIP OUT.wav      CHIP: sn76489 coco lynx astrocade msx tia

The song is turned into note events once (events()); each chip renders them
with its own pitches, waveforms and volume steps:

  sn76489    PCjr / Tandy 1000 / Coleco Adam: 3 square voices (10-bit
             dividers of 3.579545 MHz / 32) and white noise, 2 dB steps
  coco       the CoCo's 6-bit DAC: four voices mixed in software, 7.8 kHz,
             6-bit samples (the gritty sound of CoCo music)
  lynx       Mikey: 4 channels, each a timer clocking a shift register (a
             square, or a poly buzz for the bass), 8-bit volumes, stereo
  astrocade  the Bally custom chip: 3 tones divided from the master
             oscillator, its built-in vibrato on them all, noise for drums
  msx        one AY-3-8910 PSG at 1.79 MHz: lead, bass, arpeggio, with the
             drums' noise mixed in
  tia        the 2600's two voices: the tune on one (5-bit dividers of the
             31.4 kHz clock, so the song moves to the key it plays best in),
             the bass and the snare's noise sharing the other
"""
import os
import sys

import numpy as np
from scipy.io import wavfile
from scipy.signal import butter, lfilter, resample_poly

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from chip import SONG, BAR, EIGHTH, FRAME, RATE, parse, midi, chord_notes, square  # noqa: E402

TOTAL = len(SONG) * BAR


def hz(m):
    return 440.0 * 2 ** ((m - 69) / 12.0)


def events():
    """lead [(t, dur, midi, last)], bass [(t, dur, midi, last)],
    arp [(t, dur, (m1, m2, m3), soft)], drums [(t, kind)] kind: kick snare hat hat2"""
    lead, bass, arp, drums = [], [], [], []
    for bi, (sec, (chord, mel)) in enumerate(SONG):
        t0 = bi * BAR
        halves = chord.split("/")
        last = bi == len(SONG) - 1
        t = t0
        for n, l in parse(mel):
            d = l * EIGHTH
            if n:
                lead.append((t, d if not last else 2.8, midi(n), last, l))
            t += d
        for hi, ch in enumerate(halves):
            root = chord_notes(ch, 2)[0]
            span = BAR / len(halves)
            for k in range(int(span / EIGHTH)):
                if (sec == "intro" and bi < 2 and k % 2) or (last and k):
                    continue
                bass.append((t0 + hi * span + k * EIGHTH, EIGHTH * (0.85 if not last else 6),
                             root + (12 if k % 2 else 0), last))
        if sec != "outro" or bi == len(SONG) - 2:
            for hi, ch in enumerate(halves):
                span = BAR / len(halves)
                arp.append((t0 + hi * span, span, tuple(chord_notes(ch, 4)), sec in ("intro", "break")))
        if (sec == "intro" and bi < 2) or last:
            continue
        for k in range(8):
            st = t0 + k * EIGHTH
            drums.append((st, "snare" if k in (2, 6) else ("hat2" if k % 2 else "hat")))
            if k in (0, 4) and not (sec == "break" and bi % 8 < 2):
                drums.append((st, "kick"))
    return lead, bass, arp, drums


def env60(n, sr, decay, sustain=0.0, gate=None, release=0.05, steps=None, db=None):
    """A volume envelope stepped at 60 Hz; quantised to `steps` linear steps
    or to `db`-dB attenuation steps (15 and off)."""
    t = np.floor(np.arange(n) / sr / FRAME) * FRAME
    v = sustain + (1 - sustain) * np.exp(-t / max(decay, 1e-6))
    if gate is not None:
        v = np.where(t > gate, v * np.exp(-(t - gate) / max(release, 1e-6) * 3), v)
    v = np.clip(v, 0, 1)
    if steps:
        v = np.round(v * steps) / steps
    if db:
        att = np.clip(np.round(-20 * np.log10(np.maximum(v, 1e-6)) / db), 0, 15)
        v = np.where(att >= 15, 0.0, 10 ** (-att * db / 20))
    return v


class Out:
    def __init__(self, sr, chans=1):
        self.sr = sr
        self.buf = np.zeros((int((TOTAL + 4) * sr), chans))

    def add(self, t, sig, gain, pan=None):
        i = int(round(t * self.sr))
        sig = sig[:max(0, len(self.buf) - i)]
        if self.buf.shape[1] == 1:
            self.buf[i:i + len(sig), 0] += sig * gain
        else:
            l, r = (1.0, 1.0) if pan is None else pan
            self.buf[i:i + len(sig), 0] += sig * gain * l
            self.buf[i:i + len(sig), 1] += sig * gain * r


def stepped(seq, rate, n, sr, phase0=0.0):
    """A clocked waveform: step through seq at `rate` steps a second."""
    rate = np.broadcast_to(np.asarray(rate, dtype=float), (n,))
    ph = phase0 + np.cumsum(rate / sr)
    return seq[ph.astype(np.int64) % len(seq)] * 2 - 1


SQ = np.array([1.0, 0.0])


def finish(buf, sr, lowpass=None):
    """Down to 48 kHz, filtered, normalised, cut at the song's end + 1 s."""
    if sr != RATE:
        g = np.gcd(int(sr), RATE)
        buf = resample_poly(buf, RATE // g, int(sr) // g, axis=0)
    b, a = butter(1, 30 / (RATE / 2), "high")
    buf = lfilter(b, a, buf, axis=0)
    if lowpass:
        b, a = butter(2, lowpass / (RATE / 2))
        buf = lfilter(b, a, buf, axis=0)
    buf = buf[:int((TOTAL + 1.0) * RATE)]
    buf *= 0.89 / np.max(np.abs(buf))
    fade = int(0.05 * RATE)
    buf[-fade:] *= np.linspace(1, 0, fade)[:, None]
    return buf


# ---------------------------------------------------------------------------
# SN76489 (PCjr, Tandy 1000, Coleco Adam)
# ---------------------------------------------------------------------------
def sn76489():
    sr = RATE * 4
    clk = 3579545 / 32.0
    q = lambda m: clk / max(1, min(1023, round(clk / hz(m))))     # noqa: E731
    o = Out(sr)
    lead, bass, arp, drums = events()
    rng = np.random.default_rng(1981)
    for t, d, m, last, l in lead:
        n = int(d * sr)
        e = env60(n, sr, 0.3 if l > 1 else 0.12, 0.55, d - (0.03 if not last else 1.2),
                  0.04 if not last else 0.8, db=2)
        o.add(t, stepped(SQ, 2 * q(m), n, sr) * e, 0.30)
    for t, d, m, last in bass:
        n = int(d * sr)
        e = env60(n, sr, 0.08, 0.5, d - (0.02 if not last else 0.8), 0.02 if not last else 0.8, db=2)
        o.add(t, stepped(SQ, 2 * q(m), n, sr) * e, 0.26)
    for t, d, ns, soft in arp:
        n = int(d * sr)
        idx = (np.arange(n) / sr / (2 * FRAME)).astype(int) % 3
        f = np.array([q(m) for m in ns])[idx]
        beat = np.floor((np.arange(n) / sr) % (EIGHTH * 2) / FRAME) * FRAME
        e = 0.35 + 0.65 * np.exp(-beat / 0.18)
        att = np.clip(np.round(-20 * np.log10(e) / 2), 0, 15)
        o.add(t, stepped(SQ, 2 * f, n, sr) * 10 ** (-att * 2 / 20), 0.07 if soft else 0.055)
    for t, kind in drums:
        if kind == "kick":
            n = int(0.12 * sr)
            tt = np.arange(n) / sr
            f = 140 * np.exp(-tt / 0.03) + 45
            o.add(t, stepped(SQ, 2 * f, n, sr) * env60(n, sr, 0.04, db=2), 0.33)
        else:
            dur = 0.16 if kind == "snare" else 0.035
            rate = clk / 16 if kind == "snare" else clk / 4         # the noise shift rates
            n = int(dur * sr)
            bits = rng.integers(0, 2, int(dur * rate) + 2).astype(float)
            e = env60(n, sr, 0.05 if kind == "snare" else 0.012, db=2)
            o.add(t, stepped(bits, rate, n, sr) * e, {"snare": 0.22, "hat": 0.13, "hat2": 0.09}[kind])
    return finish(o.buf, sr, 10000)


# ---------------------------------------------------------------------------
# The CoCo's 6-bit DAC: a 4-voice software synth
# ---------------------------------------------------------------------------
def coco():
    sr = 7860                                   # the mixer loop's rate
    o = Out(sr)
    lead, bass, arp, drums = events()
    rng = np.random.default_rng(1980)
    for t, d, m, last, l in lead:
        n = int(d * sr)
        e = env60(n, sr, 0.3 if l > 1 else 0.12, 0.6, d - (0.03 if not last else 1.2),
                  0.04 if not last else 0.8, steps=8)
        o.add(t, stepped(SQ, 2 * hz(m), n, sr) * e, 0.30)
    for t, d, m, last in bass:
        n = int(d * sr)
        e = env60(n, sr, 0.1, 0.6, d - (0.02 if not last else 0.8), 0.02 if not last else 0.8, steps=8)
        # a saw for the bass: it sits under the squares on a DAC
        ph = (np.cumsum(np.full(n, hz(m) / sr))) % 1.0
        o.add(t, (2 * ph - 1) * e, 0.30)
    for t, d, ns, soft in arp:
        n = int(d * sr)
        idx = (np.arange(n) / sr / (2 * FRAME)).astype(int) % 3
        f = np.array([hz(m) for m in ns])[idx]
        beat = (np.arange(n) / sr) % (EIGHTH * 2)
        o.add(t, stepped(SQ, 2 * f, n, sr) * (0.35 + 0.65 * np.exp(-beat / 0.18)), 0.07 if soft else 0.06)
    for t, kind in drums:
        dur = {"kick": 0.12, "snare": 0.15, "hat": 0.03, "hat2": 0.03}[kind]
        n = int(dur * sr)
        if kind == "kick":
            tt = np.arange(n) / sr
            sig = np.sin(2 * np.pi * np.cumsum(120 * np.exp(-tt / 0.03) + 40) / sr)
            o.add(t, sig * env60(n, sr, 0.05), 0.40)
        else:
            sig = rng.uniform(-1, 1, n)
            o.add(t, sig * env60(n, sr, 0.05 if kind == "snare" else 0.01),
                  {"snare": 0.22, "hat": 0.12, "hat2": 0.08}[kind])
    # the DAC: 6 bits, held between samples
    b = o.buf / np.max(np.abs(o.buf))
    b = np.round((b + 1) / 2 * 63) / 63 * 2 - 1
    b = np.repeat(b, 6, axis=0)[:, :]                       # zero-order hold up to 47.16 kHz
    return finish(b, sr * 6, 9000)


# ---------------------------------------------------------------------------
# Mikey (Lynx): 4 timers clocking shift registers, stereo
# ---------------------------------------------------------------------------
def lfsr_seq(taps, length=4095):
    r, out = 1, []
    for _ in range(length):
        out.append(r & 1)
        b = 0
        for tp in taps:
            b ^= (r >> tp) & 1
        r = ((r << 1) | b) & 0xFFF
        if r == 0:
            r = 1
    return np.array(out, dtype=float)


def lynx():
    sr = RATE * 4
    mk = lambda m: 1e6 / 2 / max(1, round(1e6 / 2 / hz(m)))      # noqa: E731  (1 MHz timer)
    buzz = lfsr_seq((0, 5, 7), 31)                                 # a short poly: the bass's grit
    o = Out(sr, 2)
    lead, bass, arp, drums = events()
    rng = np.random.default_rng(1989)
    for t, d, m, last, l in lead:
        n = int(d * sr)
        e = env60(n, sr, 0.3 if l > 1 else 0.12, 0.55, d - (0.03 if not last else 1.2),
                  0.04 if not last else 0.8, steps=127)
        o.add(t, stepped(SQ, 2 * mk(m), n, sr) * e, 0.30, (0.9, 1.0))
    for t, d, m, last in bass:
        n = int(d * sr)
        e = env60(n, sr, 0.08, 0.5, d - (0.02 if not last else 0.8), 0.02 if not last else 0.8, steps=127)
        o.add(t, stepped(buzz, len(buzz) * hz(m), n, sr) * e, 0.22, (1.0, 1.0))
    for t, d, ns, soft in arp:
        n = int(d * sr)
        idx = (np.arange(n) / sr / (2 * FRAME)).astype(int) % 3
        f = np.array([mk(m) for m in ns])[idx]
        beat = (np.arange(n) / sr) % (EIGHTH * 2)
        o.add(t, stepped(SQ, 2 * f, n, sr) * (0.35 + 0.65 * np.exp(-beat / 0.18)),
              0.075 if soft else 0.06, (0.55, 1.0))
    for t, kind in drums:
        if kind == "kick":
            n = int(0.12 * sr)
            tt = np.arange(n) / sr
            o.add(t, stepped(SQ, 2 * (140 * np.exp(-tt / 0.03) + 45), n, sr) * env60(n, sr, 0.04), 0.34)
        else:
            dur = 0.16 if kind == "snare" else 0.035
            n = int(dur * sr)
            rate = 12000 if kind == "snare" else 40000
            bits = rng.integers(0, 2, int(dur * rate) + 2).astype(float)
            o.add(t, stepped(bits, rate, n, sr) * env60(n, sr, 0.05 if kind == "snare" else 0.012),
                  {"snare": 0.22, "hat": 0.13, "hat2": 0.09}[kind], (1.0, 0.6))
    return finish(o.buf, sr, 12000)


# ---------------------------------------------------------------------------
# The Bally Astrocade's custom sound chip
# ---------------------------------------------------------------------------
def astrocade():
    sr = RATE * 4
    MO = 33                                     # the master oscillator's divider (as the client sets it)
    base = 1789773 / 2 / MO
    q = lambda m: base / max(1, min(256, round(base / hz(m))))     # noqa: E731
    o = Out(sr)
    lead, bass, arp, drums = events()
    rng = np.random.default_rng(1978)
    tt_all = np.arange(int((TOTAL + 4) * sr)) / sr
    vib = 1 + 0.008 * np.sin(2 * np.pi * 7.5 * tt_all)          # the chip's vibrato, on every tone

    def v(t, n):
        i = int(round(t * sr))
        return vib[i:i + n]
    for t, d, m, last, l in lead:
        n = int(d * sr)
        e = env60(n, sr, 0.3 if l > 1 else 0.12, 0.55, d - (0.03 if not last else 1.2),
                  0.04 if not last else 0.8, steps=15)
        o.add(t, stepped(SQ, 2 * q(m) * v(t, n), n, sr) * e, 0.30)
    for t, d, m, last in bass:
        n = int(d * sr)
        e = env60(n, sr, 0.08, 0.5, d - (0.02 if not last else 0.8), 0.02 if not last else 0.8, steps=15)
        o.add(t, stepped(SQ, 2 * q(m) * v(t, n), n, sr) * e, 0.26)
    for t, d, ns, soft in arp:
        n = int(d * sr)
        idx = (np.arange(n) / sr / (2 * FRAME)).astype(int) % 3
        f = np.array([q(m) for m in ns])[idx]
        beat = (np.arange(n) / sr) % (EIGHTH * 2)
        e = np.round((0.35 + 0.65 * np.exp(-beat / 0.18)) * 15) / 15
        o.add(t, stepped(SQ, 2 * f * v(t, n), n, sr) * e, 0.07 if soft else 0.055)
    for t, kind in drums:
        if kind == "kick":
            n = int(0.12 * sr)
            tt = np.arange(n) / sr
            o.add(t, stepped(SQ, 2 * (140 * np.exp(-tt / 0.03) + 45), n, sr) * env60(n, sr, 0.04), 0.34)
        else:
            dur = 0.16 if kind == "snare" else 0.035
            n = int(dur * sr)
            rate = 9000 if kind == "snare" else 30000
            bits = rng.integers(0, 2, int(dur * rate) + 2).astype(float)
            o.add(t, stepped(bits, rate, n, sr) * env60(n, sr, 0.05 if kind == "snare" else 0.012, steps=15),
                  {"snare": 0.22, "hat": 0.13, "hat2": 0.09}[kind])
    return finish(o.buf, sr, 9000)


# ---------------------------------------------------------------------------
# One AY-3-8910 (MSX): chip.py's voices, with the noise on channel C
# ---------------------------------------------------------------------------
def msx():
    import chip
    mix = chip.render()
    return mix[:, None] if mix.ndim == 1 else mix


# ---------------------------------------------------------------------------
# The 2600's TIA: two voices, 5-bit dividers of 31.4 kHz
# ---------------------------------------------------------------------------
TIA_CLK = 31440.0
TIA_DIV = {4: 2, 12: 6, 6: 31}          # AUDC: pure tone /2, /6, and the /31 bass


def tia_note(m, audcs=(4, 12)):
    """The nearest TIA pitch to MIDI note m: (frequency, cents off)."""
    best = None
    for c in audcs:
        for n in range(32):
            f = TIA_CLK / TIA_DIV[c] / (n + 1)
            err = abs(1200 * np.log2(f / hz(m)))
            if best is None or err < best[1]:
                best = (f, err)
    return best


def tia_transpose(lead):
    """The transposition (semitones) whose melody the TIA plays most in tune."""
    best = None
    for tr in range(-7, 8):
        err = sum(tia_note(m + tr)[1] for _, _, m, _, _ in lead)
        if best is None or err < best[1]:
            best = (tr, err)
    return best[0]


def tia():
    sr = RATE * 4
    o = Out(sr)
    lead, bass, arp, drums = events()
    tr = tia_transpose(lead)
    rng = np.random.default_rng(1977)
    v15 = lambda e: np.round(e * 15) / 15                       # noqa: E731
    for t, d, m, last, l in lead:                                 # voice 0: the tune
        n = int(d * sr)
        f, _ = tia_note(m + tr)
        e = v15(env60(n, sr, 0.3 if l > 1 else 0.12, 0.6, d - (0.03 if not last else 1.2),
                      0.04 if not last else 0.8))
        o.add(t, stepped(SQ, 2 * f, n, sr) * e, 0.32)
    snare = {round(t, 3) for t, k in drums if k == "snare"}
    for t, d, m, last in bass:                                    # voice 1: bass, but drums win
        if round(t, 3) in snare:
            continue
        n = int(d * sr)
        f, _ = tia_note(m + tr - 12, (6, 12))
        e = v15(env60(n, sr, 0.08, 0.5, d - (0.02 if not last else 0.8), 0.02 if not last else 0.8))
        o.add(t, stepped(SQ, 2 * f, n, sr) * e, 0.30)
    for t, kind in drums:
        if kind != "snare":
            continue
        n = int(0.14 * sr)
        rate = TIA_CLK / 4                                        # AUDC 8: the 9-bit poly
        bits = rng.integers(0, 2, int(0.14 * rate) + 2).astype(float)
        o.add(t, stepped(bits, rate, n, sr) * v15(env60(n, sr, 0.05)), 0.26)
    print("tia: transposed %+d" % tr)
    return finish(o.buf, sr, 8000)


CHIPS = {"sn76489": sn76489, "coco": coco, "lynx": lynx, "astrocade": astrocade, "msx": msx, "tia": tia}


def main():
    name, out = sys.argv[1], sys.argv[2]
    b = CHIPS[name]()
    if b.shape[1] == 1:
        b = np.repeat(b, 2, axis=1)
    wavfile.write(out, RATE, (b * 32767).astype(np.int16))
    print("%s: %s, %.1f s" % (out, name, len(b) / RATE))


if __name__ == "__main__":
    main()
