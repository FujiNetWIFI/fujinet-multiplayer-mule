#!/usr/bin/env python3
"""sndcheck.py -- the beeps in a MAME -wavwrite recording, in frames.

    sndcheck.py FILE.wav [FPS]

The Studio II has one fixed tone, gated by Q a frame at a time, so a cue is
a rhythm: this prints each run of tone as "start  on  [off before the
next]" in frames (FPS, default 60), from s2sim.py's S2SIM_WAV=FILE.
"""
import array
import sys
import wave


def main():
    if len(sys.argv) < 2:
        sys.exit(__doc__)
    fps = float(sys.argv[2]) if len(sys.argv) > 2 else 60.0
    w = wave.open(sys.argv[1])
    rate, ch, width = w.getframerate(), w.getnchannels(), w.getsampwidth()
    if width != 2:
        sys.exit("sndcheck: 16-bit samples only")
    data = array.array("h", w.readframes(w.getnframes()))
    data = data[::ch]
    # tone or not, a millisecond at a time (the tone is ~300 Hz: a window of
    # 4 ms always holds a swing); runs measured in ms, shown in frames
    win = max(1, rate // 1000)
    n = len(data) // win
    on = []
    for k in range(n):
        seg = data[max(0, (k - 2) * win):(k + 2) * win]
        on.append(max(seg) - min(seg) > 2000)
    runs, k = [], 0
    while k < n:
        if on[k]:
            j = k
            while j < n and on[j]:
                j += 1
            runs.append((k, j - k))
            k = j
        else:
            k += 1
    fr = 1000.0 / fps
    runs = [(round(s / fr), round(l / fr)) for s, l in runs]
    for i, (s, l) in enumerate(runs):
        gap = runs[i + 1][0] - (s + l) if i + 1 < len(runs) else None
        print("%6d %3d%s" % (s, l, "" if gap is None else "  %d" % gap))


if __name__ == "__main__":
    main()
