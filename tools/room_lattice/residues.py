#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""The lattice's shrink per trip round ROOM's network for a set of line delays: each trip multiplies the
early tail's part at harmonic m of 2756 Hz by |phi(m)| = |mean over the lines of exp(2 pi i m d / 16)|;
this prints the worst over m = 1 .. 15 (1: it never fades). Also the largest common factor of any two.

  residues.py D0 D1 ... D7        (the delays, in samples: ring + damper sample + reads back)
"""
import sys
from itertools import combinations
from math import gcd

import numpy as np

d = [int(a) for a in sys.argv[1:]] or [1123, 1282, 1461, 1570, 1751, 1906, 2089, 2242]
m = np.arange(1, 16)[:, None]
phi = np.abs(np.exp(2j * np.pi * m * np.array(d)[None, :] / 16).mean(1))
print("delays", d, "mod 16", [x % 16 for x in d])
print(f"worst |phi| per trip {phi.max():.2f} (at harmonic {int(phi.argmax()) + 1} of 2756 Hz)")
print("largest common factor of two delays", max(gcd(a, b) for a, b in combinations(d, 2)))
