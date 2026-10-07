#!/usr/bin/env python3
"""The intro tune (chip.py's SONG) arranged for a Mockingboard: two
AY-3-8910s on the Apple's 1.023 MHz clock, one for each stereo side.

usage: mockingboard.py OUT.wav

  left chip    A lead        B bass, bouncing octaves   C kick (tone) and snare (noise)
  right chip   A counter-line: a third below the lead in the A sections, the
               lead an octave down half a beat late in the B sections
               B arpeggio    C hats (noise)

Tone periods are the chip's 12-bit counts (f = 1023000 / 16 / period),
volumes its 16 logarithmic steps, envelopes stepped at 60 Hz. A little
crossfeed keeps headphones comfortable.
"""
import os
import sys

import numpy as np
from scipy.io import wavfile
from scipy.signal import butter, lfilter

sys.path.insert(0, os.path.dirname(os.path.abspath(__file__)))
from chip import (SONG, BAR, EIGHTH, FRAME, RATE, NOTES, parse, midi, chord_notes,  # noqa: E402
                  square, frames_env, ay_level, Track)

CLOCK = 1023000.0
CROSSFEED = 0.15
SCALE = [0, 2, 4, 5, 7, 9, 11]          # C major / A minor


def hz(m):
    """The nearest the AY can make: a 12-bit period of CLOCK / 16."""
    period = max(1, min(4095, round(CLOCK / 16 / (440.0 * 2 ** ((m - 69) / 12.0)))))
    return CLOCK / 16 / period


def third_below(m):
    """The diatonic third below a note (C major; G# is taken as G)."""
    pc, octv = m % 12, m // 12
    if pc not in SCALE:
        pc -= 1
    i = SCALE.index(pc) - 2
    if i < 0:
        i += 7
        octv -= 1
    return octv * 12 + SCALE[i]


def noise_bits(rng, period_hz, n):
    """The AY's noise: random bits held for one noise period each."""
    hold = max(1, int(RATE / period_hz))
    bits = rng.integers(0, 2, n // hold + 2).astype(float) * 2 - 1
    return np.repeat(bits, hold)[:n]


def tone(t0, track, m, dur, gain, decay, sustain, release, gate, vib=True):
    cnt = int(dur * RATE)
    tt = np.arange(cnt) / RATE
    f = hz(m) * ((1 + 0.006 * np.sin(2 * np.pi * 6 * tt) * (tt > 0.15)) if vib else 1)
    env = frames_env(dur, decay=decay, sustain=sustain, release=release, gate=gate)
    track.add(t0, square(f, cnt) * env, gain)


def render():
    total = len(SONG) * BAR
    lead, bass, drums = Track(total), Track(total), Track(total)        # left chip
    counter, arp, hats = Track(total), Track(total), Track(total)       # right chip
    rng = np.random.default_rng(1977)
    for bi, (sec, (chord, mel)) in enumerate(SONG):
        t0 = bi * BAR
        halves = chord.split("/")
        last = bi == len(SONG) - 1
        # --- lead (left A) and counter-line (right A)
        t = t0
        for n, l in parse(mel):
            d = l * EIGHTH
            if n:
                m = midi(n)
                dur = d if not last else 2.8
                rel, gate = (0.04, dur - 0.03) if not last else (0.8, dur - 1.2)
                tone(t, lead, m, dur, 0.30, 0.3 if l > 1 else 0.12, 0.55, rel, gate)
                if sec in ("A", "outro"):
                    tone(t, counter, third_below(m), dur, 0.17, 0.3 if l > 1 else 0.12, 0.5, rel, gate)
                elif sec == "B":
                    tone(t + EIGHTH, counter, m - 12, min(dur, 3 * EIGHTH), 0.16, 0.25, 0.45, 0.04,
                         min(dur, 3 * EIGHTH) - 0.03)
                elif sec == "break":
                    tone(t + 2 * EIGHTH, counter, m, dur, 0.10, 0.3, 0.4, 0.04, dur - 0.03)
            t += d
        # --- bass (left B): octave bounce on eighths
        for hi, ch in enumerate(halves):
            root = chord_notes(ch, 2)[0]
            span = BAR / len(halves)
            for k in range(int(span / EIGHTH)):
                if (sec == "intro" and bi < 2 and k % 2) or (last and k):
                    continue
                m = root + (12 if k % 2 else 0)
                dur = EIGHTH * (0.85 if not last else 6)
                tone(t0 + hi * span + k * EIGHTH, bass, m, dur, 0.26, 0.08, 0.5,
                     0.02 if not last else 0.8, dur - (0.02 if not last else 0.8), vib=False)
        # --- arpeggio (right B): chord tones every 2 video frames
        if sec != "outro" or bi == len(SONG) - 2:
            for hi, ch in enumerate(halves):
                span = BAR / len(halves)
                notes = chord_notes(ch, 4)
                cnt = int(span * RATE)
                idx = (np.arange(cnt) / RATE / (2 * FRAME)).astype(int) % 3
                f = np.array([hz(m) for m in notes])[idx]
                beat = (np.arange(cnt) / RATE) % (EIGHTH * 2)
                env = ay_level(0.35 + 0.65 * np.exp(-beat / 0.18))
                arp.add(t0 + hi * span, square(f, cnt) * env, 0.09 if sec in ("intro", "break") else 0.075)
        # --- drums: kick and snare left, hats right
        if (sec == "intro" and bi < 2) or last:
            continue
        for k in range(8):
            st = t0 + k * EIGHTH
            if k in (2, 6):
                dur = 0.16
                cnt = int(dur * RATE)
                env = frames_env(dur, decay=0.05, sustain=0.0, release=0.01)
                drums.add(st, noise_bits(rng, 6000, cnt) * env, 0.22)
            else:
                dur = 0.035
                cnt = int(dur * RATE)
                env = frames_env(dur, decay=0.012, sustain=0.0, release=0.005)
                hats.add(st, noise_bits(rng, 24000, cnt) * env, 0.12 if k % 2 else 0.16)
            if k in (0, 4) and not (sec == "break" and bi % 8 < 2):
                dur = 0.12
                cnt = int(dur * RATE)
                tt = np.arange(cnt) / RATE
                f = 140 * np.exp(-tt / 0.03) + 45
                env = frames_env(dur, decay=0.04, sustain=0.0, release=0.01)
                drums.add(st, square(f, cnt) * env, 0.35)
    left = lead.buf + bass.buf + drums.buf
    right = 1.4 * (counter.buf + arp.buf + hats.buf)       # the lighter side, evened up
    left, right = left + CROSSFEED * right, right + CROSSFEED * left
    out = []
    b, a = butter(1, 30 / (RATE / 2), "high")
    b2, a2 = butter(2, 12000 / (RATE / 2))          # the card's output filtering
    for ch in (left, right):
        ch = lfilter(b, a, lfilter(b2, a2, ch))
        out.append(ch[:int((total + 1.0) * RATE)])
    st = np.stack(out, axis=1)
    st *= 0.89 / np.max(np.abs(st))
    fade = int(0.05 * RATE)
    st[-fade:] *= np.linspace(1, 0, fade)[:, None]
    return st


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else "music-mockingboard.wav"
    st = render()
    wavfile.write(out, RATE, (st * 32767).astype(np.int16))
    print("%s: %.1f s stereo, %d bars" % (out, len(st) / RATE, len(SONG)))


if __name__ == "__main__":
    main()
