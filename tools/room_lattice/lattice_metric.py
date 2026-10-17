#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""How strongly ROOM's early tail repeats every 44100 / 16 = 2756 Hz (the lines' 16-sample lattice), from
abx_render's WAVs: the correlation of the log spectrum (2048-sample window, tilt removed with a ~700 Hz
moving average) with itself 2756 Hz along, minus its mean correlation at neighbouring lags. White noise
gives 0. Averaged over the 4 hits and both channels, at 60 .. 300 ms after each hit.

  lattice_metric.py A.wav [B.wav ...]
"""
import os
import sys
import wave

import numpy as np

WIN = 2048
LAG = WIN // 16                     # bins in 2756 Hz
TIMES = (0.06, 0.1, 0.15, 0.2, 0.3)


def load(path):
    w = wave.open(path)
    return np.frombuffer(w.readframes(w.getnframes()), np.int16).reshape(-1, 2).astype(float)


def spec(seg):
    s = np.log(np.abs(np.fft.rfft(seg[:WIN] * np.hanning(WIN))) + 1e-6)
    return (s - np.convolve(s, np.ones(33) / 33, mode="same"))[40:-40]


def excess(s):
    c = lambda lag: np.corrcoef(s[:-lag], s[lag:])[0, 1]
    near = list(range(LAG - 24, LAG - 6)) + list(range(LAG + 6, LAG + 24))
    return c(LAG) - np.mean([c(lag) for lag in near])


def main():
    w = max(len(os.path.basename(p)) for p in sys.argv[1:])
    print(f"{'ms after the hit':{w}s} " + " ".join(f"{int(t * 1000):5d}" for t in TIMES))
    for path in sys.argv[1:]:
        x = load(path)
        vals = [np.mean([excess(spec(x[int((h * 2 + t) * 44100):, ch])) for h in range(4) for ch in (0, 1)])
                for t in TIMES]
        print(f"{os.path.basename(path):{w}s} " + " ".join(f"{v:5.2f}" for v in vals))


if __name__ == "__main__":
    main()
