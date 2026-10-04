#!/usr/bin/env python3
"""An original 8-bit tune for the intro video, in the style of the
Intellivision's AY-3-8914: three square-wave tone channels and one noise
channel, volumes in the chip's 16 logarithmic steps, envelopes updated at
60 Hz like a game's sound driver.

usage: chip.py OUT.wav

120 BPM, 4/4; one bar is 2 s, so 120 bars run 4:00. Sections:
intro 4 | A 16 | B 16 | A 16 | break 8 | B 16 | A 16 | break 8 | B 8 | A 10 | outro 2
"""
import sys

import numpy as np
from scipy.io import wavfile

RATE = 48000
BPM = 120
EIGHTH = 60.0 / BPM / 2
BAR = EIGHTH * 8
FRAME = 1.0 / 60

NOTES = {"C": 0, "C#": 1, "D": 2, "D#": 3, "E": 4, "F": 5, "F#": 6, "G": 7, "G#": 8, "A": 9, "A#": 10, "B": 11}
CHORDS = {"C": "C E G", "Am": "A C E", "F": "F A C", "G": "G B D", "Dm": "D F A", "E": "E G# B"}


def midi(n):
    name, octv = n[:-1], int(n[-1])
    return 12 * (octv + 1) + NOTES[name]


def hz(m):
    # the real chip divides a 3.58 MHz / 16 clock; snap to its period grid
    clock = 3579545 / 32.0
    period = max(1, round(clock / (440.0 * 2 ** ((m - 69) / 12.0))))
    return clock / period


def ay_level(v):
    """0..1 -> the nearest of the AY's 16 volume steps (3 dB apart)."""
    v = np.clip(v, 0, 1)
    step = np.round(15 + 2 * np.log2(np.maximum(v, 1e-6)))
    step = np.clip(step, 0, 15)
    return np.where(step <= 0, 0.0, 2 ** ((step - 15) / 2))


def frames_env(dur, attack=0.0, decay=0.25, sustain=0.6, release=0.05, gate=None):
    """A volume envelope stepped at 60 Hz, sample-rate output."""
    n = int(dur * RATE)
    t = np.floor(np.arange(n) / RATE / FRAME) * FRAME          # 60 Hz steps
    gate = dur - release if gate is None else gate
    env = np.where(t < attack, t / max(attack, 1e-6),
                   sustain + (1 - sustain) * np.exp(-(t - attack) / max(decay, 1e-6)))
    env = np.where(t > gate, env * np.exp(-(t - gate) / max(release, 1e-6) * 3), env)
    return ay_level(env)


def square(freq, n, phase0=0.0):
    """Band-limited square (polyBLEP) at a fixed or per-sample frequency."""
    f = np.broadcast_to(np.asarray(freq, dtype=float), (n,))
    dt = f / RATE
    ph = (phase0 + np.cumsum(dt)) % 1.0
    y = np.where(ph < 0.5, 1.0, -1.0)

    def blep(t, d):
        out = np.zeros_like(t)
        a = t < d
        x = t[a] / d[a]
        out[a] = x + x - x * x - 1
        b = t > 1 - d
        x = (t[b] - 1) / d[b]
        out[b] = x * x + x + x + 1
        return out
    y += blep(ph, dt)
    y -= blep((ph + 0.5) % 1.0, dt)
    return y


class Track:
    def __init__(self, seconds):
        self.buf = np.zeros(int(seconds * RATE) + 4 * RATE)

    def add(self, t, sig, gain):
        i = int(round(t * RATE))
        self.buf[i:i + len(sig)] += sig * gain


def parse(seq):
    """'E5:2 r:1 G5:1' -> [(note or None, eighths)]"""
    out = []
    for tok in seq.split():
        n, _, l = tok.partition(":")
        out.append((None if n == "r" else n, float(l or 1)))
    return out


# --------------------------------------------------------------------------
# The tune. Each bar: (chord, lead) with lead in eighths (8 per bar). A bar
# with two chords is written "F/G" (two beats each).
# --------------------------------------------------------------------------
A = [
    ("C", "E5:2 G5:2 C6:3 B5:1"),
    ("Am", "A5:2 E5:2 C5:2 E5:2"),
    ("F", "F5:2 A5:2 C6:2 A5:1 G5:1"),
    ("G", "G5:3 F5:1 E5:2 D5:2"),
    ("C", "E5:2 G5:2 C6:3 D6:1"),
    ("Am", "E6:2 D6:2 C6:2 A5:2"),
    ("F/G", "F5:1 G5:1 A5:2 B5:1 C6:1 D6:2"),
    ("C", "C6:6 r:2"),
    ("C", "G5:1 r:1 G5:1 A5:1 G5:2 E5:2"),
    ("Am", "A5:1 r:1 A5:1 B5:1 C6:2 A5:2"),
    ("F", "C6:2 A5:2 F5:2 A5:2"),
    ("G", "B5:2 D6:2 G6:3 F6:1"),
    ("C", "E6:2 G5:2 C6:3 D6:1"),
    ("Am", "E6:2 D6:2 C6:2 A5:2"),
    ("F/G", "A5:1 B5:1 C6:2 D6:2 B5:2"),
    ("C", "C6:4 G5:2 C5:2"),
]
B = [
    ("Am", "A4:1 C5:1 E5:1 A5:3 G5:2"),
    ("F", "F5:2 E5:1 C5:1 A4:4"),
    ("C", "G4:1 C5:1 E5:1 G5:3 E5:2"),
    ("G", "D5:2 B4:2 G4:2 B4:2"),
    ("Am", "A4:1 C5:1 E5:1 A5:2 B5:1 C6:2"),
    ("F", "A5:2 F5:2 C6:2 A5:2"),
    ("Dm", "D6:2 C6:1 A5:1 F5:2 D5:2"),
    ("E", "E5:2 G#5:2 B5:2 E6:2"),
    ("Am", "A5:1 C6:1 E6:1 A6:3 G6:2"),
    ("F", "F6:2 E6:1 C6:1 A5:4"),
    ("C", "G5:1 C6:1 E6:1 G6:3 E6:2"),
    ("G", "D6:2 B5:2 G5:2 B5:2"),
    ("Am", "C6:2 B5:2 A5:2 E5:2"),
    ("F", "F5:2 A5:2 C6:2 F6:2"),
    ("Dm", "F6:2 E6:1 D6:1 A5:2 F5:2"),
    ("G", "G5:2 F5:2 D5:2 B4:2"),
]
BREAK = [("Am", "r:8"), ("F", "r:8"), ("C", "r:8"), ("G", "r:8"),
         ("Am", "A5:4 E6:4"), ("F", "F6:4 C6:4"), ("C", "G6:4 E6:4"), ("G", "D6:2 B5:2 G5:2 D6:2")]
INTRO = [("C", "r:8"), ("C", "r:8"), ("F", "r:8"), ("G", "r:4 G5:1 A5:1 B5:1 D6:1")]
OUTRO = [("C", "C6:2 G5:1 E5:1 C5:2 G5:2"), ("C", "C6:8")]

SONG = ([("intro", b) for b in INTRO] + [("A", b) for b in A] + [("B", b) for b in B] +
        [("A", b) for b in A] + [("break", b) for b in BREAK] + [("B", b) for b in B] +
        [("A", b) for b in A] + [("break", b) for b in BREAK] + [("B", b) for b in B[8:]] +
        [("A", b) for b in A[:8] + A[14:]] + [("outro", b) for b in OUTRO])


def chord_notes(name, octv):
    return [midi(n + str(octv)) for n in CHORDS[name].split()]


def render():
    total = len(SONG) * BAR
    lead, bass, arp, noise = Track(total), Track(total), Track(total), Track(total)
    rng = np.random.default_rng(1983)
    for bi, (sec, (chord, mel)) in enumerate(SONG):
        t0 = bi * BAR
        halves = chord.split("/")
        last = bi == len(SONG) - 1
        # --- lead (channel A): a little vibrato after the attack
        t = t0
        for n, l in parse(mel):
            d = l * EIGHTH
            if n:
                dur = d if not last else 2.8
                cnt = int(dur * RATE)
                tt = np.arange(cnt) / RATE
                vib = 1 + 0.006 * np.sin(2 * np.pi * 6 * tt) * (tt > 0.15)
                sig = square(hz(midi(n)) * vib, cnt)
                env = frames_env(dur, decay=0.3 if l > 1 else 0.12, sustain=0.55,
                                 release=0.04 if not last else 0.8, gate=dur - (0.03 if not last else 1.2))
                lead.add(t, sig * env, 0.30)
            t += d
        # --- bass (channel B): octave bounce on eighths; whole notes in intro/outro
        for hi, ch in enumerate(halves):
            root = chord_notes(ch, 2)[0]
            span = BAR / len(halves)
            steps = int(span / EIGHTH)
            for k in range(steps):
                if (sec == "intro" and bi < 2 and k % 2) or (last and k):
                    continue
                m = root + (12 if k % 2 else 0)
                st = t0 + hi * span + k * EIGHTH
                dur = EIGHTH * (0.85 if not last else 6)
                cnt = int(dur * RATE)
                env = frames_env(dur, decay=0.08, sustain=0.5, release=0.02 if not last else 0.8)
                bass.add(st, square(hz(m), cnt) * env, 0.26)
        # --- arpeggio (channel C): chord tones cycled every 2 video frames
        if sec != "outro" or bi == len(SONG) - 2:
            for hi, ch in enumerate(halves):
                span = BAR / len(halves)
                notes = chord_notes(ch, 4)
                cnt = int(span * RATE)
                idx = (np.arange(cnt) / RATE / (2 * FRAME)).astype(int) % 3
                f = np.array([hz(m) for m in notes])[idx]
                # pulse the arp on each beat so it breathes
                beat = (np.arange(cnt) / RATE) % (EIGHTH * 2)
                env = ay_level(0.35 + 0.65 * np.exp(-beat / 0.18))
                gain = 0.07 if sec in ("intro", "break") else 0.055
                arp.add(t0 + hi * span, square(f, cnt) * env, gain)
        # --- noise (drums): hats on eighths, snare on 2 and 4, kick on 1 and 3
        if sec == "intro" and bi < 2:
            continue
        if last:
            continue
        for k in range(8):
            st = t0 + k * EIGHTH
            if k in (2, 6):          # snare: long noise burst
                dur = 0.16
                cnt = int(dur * RATE)
                env = frames_env(dur, decay=0.05, sustain=0.0, release=0.01)
                noise.add(st, rng.uniform(-1, 1, cnt) * env, 0.22)
            else:                    # hat: tick of high noise
                dur = 0.035
                cnt = int(dur * RATE)
                env = frames_env(dur, decay=0.012, sustain=0.0, release=0.005)
                hiss = np.diff(rng.uniform(-1, 1, cnt + 1))
                noise.add(st, hiss * env, 0.10 if k % 2 else 0.14)
            if k in (0, 4) and not (sec == "break" and bi % 8 < 2):
                dur = 0.12
                cnt = int(dur * RATE)
                tt = np.arange(cnt) / RATE
                f = 140 * np.exp(-tt / 0.03) + 45
                env = frames_env(dur, decay=0.04, sustain=0.0, release=0.01)
                bass.add(st, square(f, cnt) * env, 0.35)
    mix = lead.buf + bass.buf + arp.buf + noise.buf
    # the RF modulator's gentle low-pass and a DC block
    from scipy.signal import butter, lfilter
    b, a = butter(2, 9000 / (RATE / 2))
    mix = lfilter(b, a, mix)
    b, a = butter(1, 30 / (RATE / 2), "high")
    mix = lfilter(b, a, mix)
    mix = mix[:int((total + 1.0) * RATE)]   # the last chord rings 1 s past the bar
    mix *= 0.89 / np.max(np.abs(mix))
    fade = int(0.05 * RATE)
    mix[-fade:] *= np.linspace(1, 0, fade)
    return mix


def main():
    out = sys.argv[1] if len(sys.argv) > 1 else "music.wav"
    mix = render()
    st = np.stack([mix, mix], axis=1)
    wavfile.write(out, RATE, (st * 32767).astype(np.int16))
    print("%s: %.1f s, %d bars" % (out, len(mix) / RATE, len(SONG)))


if __name__ == "__main__":
    main()
