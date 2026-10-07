#!/usr/bin/env python3
"""The intro tune (chip.py's SONG) voiced for the Atari's POKEY.

usage: pokey.py OUT.wav

POKEY makes each voice by dividing a clock and passing the result through a
polynomial counter ("distortion"), with 16 linear volume steps:

  lead, arpeggio  pure tone (AUDC $A0) from the 64 kHz clock with an 8-bit
                  divider, f = 63921 / (2 (AUDF + 1)): the high notes come
                  out a little sharp or flat, as they do on the machine
  bass            distortion C ($C0, the 4-bit poly) on two joined channels
                  at 1.79 MHz: Atari's buzzy bass, in tune
  drums           the 17-bit poly as noise (snare, hats) and a falling pure
                  tone (kick)

Envelopes step at 60 Hz, as a VBI sound driver would.
"""
import sys

import numpy as np
from scipy.io import wavfile
from scipy.signal import butter, lfilter, resample_poly

sys.path.insert(0, __import__("os").path.dirname(__import__("os").path.abspath(__file__)))
from chip import SONG, BAR, EIGHTH, FRAME, RATE, parse, midi, chord_notes  # noqa: E402

OVER = 4                     # synthesise at 4x and filter down (POKEY is not band-limited)
SR = RATE * OVER
CLK64 = 63921.0
CLK179 = 1789790.0


def hz(m):
    return 440.0 * 2 ** ((m - 69) / 12.0)


def pure64(m):
    """An 8-bit divider on the 64 kHz clock: the frequency POKEY can make."""
    n = int(round(CLK64 / (2 * hz(m)) - 1))
    n = max(0, min(255, n))
    return CLK64 / (2 * (n + 1))


POLY4 = None


def poly4():
    global POLY4
    if POLY4 is None:
        r, out = 0xF, []
        for _ in range(15):
            out.append(r & 1)
            b = ((r >> 0) ^ (r >> 1)) & 1
            r = (r >> 1) | (b << 3)
        POLY4 = np.array(out, dtype=float)
    return POLY4


def clocked(seq, rate, n, phase0=0.0):
    """Step through seq at `rate` steps a second: a POKEY channel's output."""
    rate = np.broadcast_to(np.asarray(rate, dtype=float), (n,))
    ph = phase0 + np.cumsum(rate / SR)
    return seq[ph.astype(np.int64) % len(seq)] * 2 - 1


SQUARE = np.array([1.0, 0.0])


def vol_env(dur, decay, sustain, release, gate):
    """0..15 volume, stepped every 1/60 s."""
    n = int(dur * SR)
    t = np.floor(np.arange(n) / SR / FRAME) * FRAME
    v = sustain + (1 - sustain) * np.exp(-t / max(decay, 1e-6))
    v = np.where(t > gate, v * np.exp(-(t - gate) / max(release, 1e-6) * 3), v)
    return np.round(np.clip(v, 0, 1) * 15) / 15


class Track:
    def __init__(self, seconds):
        self.buf = np.zeros(int(seconds * SR) + 4 * SR)

    def add(self, t, sig, gain):
        i = int(round(t * SR))
        self.buf[i:i + len(sig)] += sig * gain


def render():
    total = len(SONG) * BAR
    lead, bass, arp, drum = Track(total), Track(total), Track(total), Track(total)
    rng = np.random.default_rng(1979)
    noise = (rng.integers(0, 2, 1 << 17)).astype(float)       # the 17-bit poly's bits
    for bi, (sec, (chord, mel)) in enumerate(SONG):
        t0 = bi * BAR
        halves = chord.split("/")
        last = bi == len(SONG) - 1
        # --- lead: pure tone on the 64 kHz clock, a touch of vibrato by
        # nudging AUDF the way drivers do (one divider step)
        t = t0
        for nt, l in parse(mel):
            d = l * EIGHTH
            if nt:
                dur = d if not last else 2.8
                cnt = int(dur * SR)
                f0 = pure64(midi(nt))
                tt = np.arange(cnt) / SR
                wob = np.where((tt > 0.18) & ((tt * 12).astype(int) % 2 == 1), 0.996, 1.0)
                env = vol_env(dur, 0.3 if l > 1 else 0.12, 0.6, 0.04 if not last else 0.8,
                              dur - (0.03 if not last else 1.2))
                lead.add(t, clocked(SQUARE, 2 * f0 * wob, cnt) * env, 0.30)
            t += d
        # --- bass: distortion C, 16-bit divider (in tune), octave bounce
        for hi, ch in enumerate(halves):
            root = chord_notes(ch, 2)[0]
            span = BAR / len(halves)
            for k in range(int(span / EIGHTH)):
                if (sec == "intro" and bi < 2 and k % 2) or (last and k):
                    continue
                m = root + (12 if k % 2 else 0)
                st = t0 + hi * span + k * EIGHTH
                dur = EIGHTH * (0.85 if not last else 6)
                cnt = int(dur * SR)
                n16 = round(CLK179 / (15 * hz(m)) - 7)
                rate = CLK179 / (n16 + 7)
                env = vol_env(dur, 0.09, 0.55, 0.02 if not last else 0.8, dur - 0.02 if not last else 1.0)
                bass.add(st, clocked(poly4(), rate, cnt) * env, 0.24)
        # --- arpeggio: chord tones every 2 frames, pure tone, quieter
        if sec != "outro" or bi == len(SONG) - 2:
            for hi, ch in enumerate(halves):
                span = BAR / len(halves)
                notes = chord_notes(ch, 4)
                cnt = int(span * SR)
                idx = (np.arange(cnt) / SR / (2 * FRAME)).astype(int) % 3
                f = np.array([pure64(m) for m in notes])[idx]
                beat = (np.arange(cnt) / SR) % (EIGHTH * 2)
                env = np.round((0.35 + 0.65 * np.exp(-np.floor(beat / FRAME) * FRAME / 0.18)) * 15) / 15
                gain = 0.075 if sec in ("intro", "break") else 0.06
                arp.add(t0 + hi * span, clocked(SQUARE, 2 * f, cnt) * env, gain)
        # --- drums
        if (sec == "intro" and bi < 2) or last:
            continue
        for k in range(8):
            st = t0 + k * EIGHTH
            if k in (2, 6):                       # snare: poly noise at ~9 kHz
                dur = 0.16
                cnt = int(dur * SR)
                env = vol_env(dur, 0.05, 0.0, 0.01, dur)
                drum.add(st, clocked(noise, 9000, cnt, rng.integers(0, 1 << 17)) * env, 0.24)
            else:                                 # hat: poly noise at ~32 kHz
                dur = 0.035
                cnt = int(dur * SR)
                env = vol_env(dur, 0.012, 0.0, 0.005, dur)
                drum.add(st, clocked(noise, 31960, cnt, rng.integers(0, 1 << 17)) * env,
                         0.09 if k % 2 else 0.13)
            if k in (0, 4) and not (sec == "break" and bi % 8 < 2):
                dur = 0.12                        # kick: a falling pure tone
                cnt = int(dur * SR)
                tt = np.arange(cnt) / SR
                f = 150 * np.exp(-tt / 0.03) + 45
                env = vol_env(dur, 0.04, 0.0, 0.01, dur)
                drum.add(st, clocked(SQUARE, 2 * f, cnt) * env, 0.34)
    mix = lead.buf + bass.buf + arp.buf + drum.buf
    mix = resample_poly(mix, 1, OVER)            # anti-alias down to 48 kHz
    b, a = butter(1, 30 / (RATE / 2), "high")    # DC block (POKEY is unipolar)
    mix = lfilter(b, a, mix)
    mix = mix[:int((total + 1.0) * RATE)]
    mix *= 0.89 / np.max(np.abs(mix))
    fade = int(0.05 * RATE)
    mix[-fade:] *= np.linspace(1, 0, fade)
    return mix


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else "music-atari.wav"
    mix = render()
    st = np.stack([mix, mix], axis=1)
    wavfile.write(out, RATE, (st * 32767).astype(np.int16))
    print("%s: %.1f s, %d bars" % (out, len(mix) / RATE, len(SONG)))


if __name__ == "__main__":
    main()
