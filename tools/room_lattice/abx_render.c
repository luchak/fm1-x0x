/* SPDX-License-Identifier: GPL-3.0-only */
/* ROOM's wet output only, for ABX listening and for lattice_metric.py: a hit every 2 s for 8 s,
 * stereo 16-bit WAV, at a fixed gain (the same for any build, so A and B are not normalised).
 *   abx_render OUT.wav DECAY TONE clicks|snare
 * clicks: a 2 ms noise click (the worst case: bright, short); snare: a 50 ms decaying noise burst.
 * Build it against the tree whose fxbus.c is to be heard (README.md). */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define X0X_DRUM_TABLES_DEFINE              /* no drum909.c in this link: the tables are defined here */
#include "dsp/fxbus.c"

#define FS 44100
#define LEN (FS * 8 / 256 * 256)

static fxbus_t F;
static int16_t dly[88200];
static fx_rv_buf_t rv;
static float in[LEN], ol[LEN], orr[LEN];
static unsigned rs = 1u;
static float rnd(void) { rs = rs * 1664525u + 1013904223u; return (float)(int)(rs >> 9) / 4194304.0f - 1.0f; }
static void w32(FILE *f, unsigned v) { fwrite(&v, 4, 1, f); }
static void w16(FILE *f, unsigned v) { unsigned short s = (unsigned short)v; fwrite(&s, 2, 1, f); }

int main(int argc, char **argv)
{
    if (argc != 5) {
        fprintf(stderr, "usage: abx_render OUT.wav DECAY TONE clicks|snare\n");
        return 2;
    }
    fxbus_init(&F, dly, 88200, &rv);
    fxbus_set(&F, FX_DIST, 0);
    fxbus_set(&F, FX_COMP, 0);
    fxbus_set(&F, FX_VOLUME, 127);
    fxbus_set(&F, FX_RV_DECAY, atoi(argv[2]));
    fxbus_set(&F, FX_RV_TONE, atoi(argv[3]));
    fxbus_set_rv_type(&F, FX_RV_ROOM);
    const int clicks = !strcmp(argv[4], "clicks");
    for (int t = 0; t < LEN; t++) {
        const int ph = t % (FS * 2);
        in[t] = clicks ? (ph < 88 ? 0.9f * rnd() * (1.0f - ph / 88.0f) : 0.0f)
                       : (ph < 2205 ? 0.85f * rnd() * expf(-ph / 600.0f) : 0.0f);
    }
    static const float z[256];
    for (int i = 0; i < LEN; i += 256)
        fxbus_process_st(&F, z, z, in + i, z, ol + i, orr + i, 256);
    FILE *f = fopen(argv[1], "wb");
    if (!f) {
        perror(argv[1]);
        return 1;
    }
    fwrite("RIFF", 1, 4, f); w32(f, 36 + LEN * 4); fwrite("WAVEfmt ", 1, 8, f); w32(f, 16); w16(f, 1); w16(f, 2);
    w32(f, FS); w32(f, FS * 4); w16(f, 4); w16(f, 16); fwrite("data", 1, 4, f); w32(f, LEN * 4);
    for (int t = 0; t < LEN; t++) {
        const float a = fmaxf(-1.0f, fminf(1.0f, ol[t] * 0.5f)), b = fmaxf(-1.0f, fminf(1.0f, orr[t] * 0.5f));
        w16(f, (unsigned)(int)lrintf(a * 32767.0f));
        w16(f, (unsigned)(int)lrintf(b * 32767.0f));
    }
    fclose(f);
    return 0;
}
