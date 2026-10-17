/* SPDX-License-Identifier: GPL-3.0-only */
/* The reverb's ROOM type (fxbus.c), held to a plain float FDN of the same structure written out
 * here (its lines and diffusers float, its coefficients fxbus's own), so what is checked is the
 * storage (8-bit block-float rings, int16 diffusers: about -40 dB from float), and the reads' and
 * writes' timing (a slip there is 0 dB). That runs in calls of irregular sizes (as the engine's,
 * split at sequencer events), with each reverb knob at its ends and middle. Also: its tail
 * reaches exact silence and it goes idle; RvLvl 0 is silent; L and R are uncorrelated and as loud;
 * a full-scale send at the longest decay stays finite; switching type cuts the tail cleanly (either
 * type after a switch is that type fresh); turning RvDec and RvTone while ROOM plays sets its
 * coefficients as for a fresh ROOM; mono is the stereo left; and the host time per sample against
 * CLASSIC (information). */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#define X0X_DRUM_TABLES_DEFINE              /* no drum909.c in this link: the tables are defined here */
#include "../../firmware/src/dsp/fxbus.h"

#define N 256
#define FS 44100
static int fails;
static void check(int ok, const char *what)
{
    printf("  %s %s\n", ok ? "ok  " : "FAIL", what);
    fails += !ok;
}
static unsigned rs = 12345u;
static float rnd(void) { rs = rs * 1664525u + 1013904223u; return (float)(int)(rs >> 9) / 4194304.0f - 1.0f; }

static fxbus_t F, G;
static int16_t dl1[88200], dl2[88200];
static fx_rv_buf_t rv1, rv2;
static const float zero[N];

/* ---- the reference: fxbus.c's ROOM with float lines and diffusers ---- */
/* each line's delay: its ring, and one sample more where its damper delays (FX_RM_DLY) */
static const int RL[8] = {FX_RM_L0 + FX_RM_DLY(0), FX_RM_L1 + FX_RM_DLY(1), FX_RM_L2 + FX_RM_DLY(2), FX_RM_L3 + FX_RM_DLY(3),
                          FX_RM_L4 + FX_RM_DLY(4), FX_RM_L5 + FX_RM_DLY(5), FX_RM_L6 + FX_RM_DLY(6), FX_RM_L7 + FX_RM_DLY(7)};
static const float RIN[8] = {1, 1, -1, -1, 1, -1, -1, 1};
static const int DL[4] = {FX_RM_D0, FX_RM_D1, FX_RM_D2, FX_RM_D3};
static const float DG[4] = {FX_RM_G0, FX_RM_G1, FX_RM_G2, FX_RM_G3};
typedef struct {
    float line[8][FX_RM_L7 + 1], dif[4][FX_RM_DIF_TOTAL], lp[8], tl, tr;
    int pos[8], dpos[4];
    d9_biquad_t hp;
} ref_t;
static ref_t R;
static void ref_run(ref_t *r, const fxbus_t *f, const float *in, float *ol, float *orr, int n)
{
    const fx_room_t *rm = &f->rv_buf->room;
    const float lvl = f->rv_level * f->rm_out * 0.35355339f * rm->tap_k;
    for (int i = 0; i < n; i++) {
        float x = d9_biquad_tick(&r->hp, in[i]), s[8];
        for (int k = 0; k < 4; k++) {
            float bo = r->dif[k][r->dpos[k]], v = x + DG[k] * bo;
            r->dif[k][r->dpos[k]] = v;
            x = bo - DG[k] * v;
            r->dpos[k] = (r->dpos[k] + 1) % DL[k];
        }
        for (int j = 0; j < 8; j++) {
            float v = r->line[j][r->pos[j]];
            r->lp[j] = rm->a[j] * r->lp[j] + rm->c[j] * v;   /* (c has the loop gain and the mix's 1/sqrt(8)) */
            s[j] = r->lp[j];
        }
        for (int h = 1; h < 8; h <<= 1)
            for (int j = 0; j < 8; j += h << 1)
                for (int k = j; k < j + h; k++) {
                    float u = s[k], w = s[k + h];
                    s[k] = u + w;
                    s[k + h] = u - w;
                }
        /* the outputs: the mix's rows 1 (L: + - + - + - + -) and 6 (R: + + - - - - + +), before the input,
         * the lines' mean damper undone (fxbus's tap_a, tap_k) */
        ol[i] = (s[1] - rm->tap_a * r->tl) * lvl;
        orr[i] = (s[6] - rm->tap_a * r->tr) * lvl;
        r->tl = s[1];
        r->tr = s[6];
        for (int j = 0; j < 8; j++) {
            r->line[j][r->pos[j]] = s[j] + x * 0.35355339f * RIN[j];
            r->pos[j] = (r->pos[j] + 1) % RL[j];
        }
    }
}

static void setup(fxbus_t *f, int16_t *dl, fx_rv_buf_t *rv, int type, int dec, int tone)
{
    fxbus_init(f, dl, 88200, rv);
    fxbus_set(f, FX_DIST, 0);
    fxbus_set(f, FX_COMP, 0);
    fxbus_set(f, FX_VOLUME, 127);
    fxbus_set(f, FX_RV_DECAY, dec);
    fxbus_set(f, FX_RV_TONE, tone);
    fxbus_set_rv_type(f, type);
}

/* the send, samples [t0, t0 + n): noise bursts (snare-like, 50 ms at RMS ~0.5) every half second
 * before sample `on`, then silence */
static void send(float *x, int t0, int n, int on)
{
    for (int i = 0; i < n; i++) {
        int t = t0 + i;
        x[i] = (t < on && t % (FS / 2) < FS / 20) ? 0.85f * rnd() : 0.0f;
    }
}

/* call sizes: every phase against the lines' 16-sample blocks, and whole blocks */
static const int SZ[] = {256, 13, 256, 1, 37, 256, 200, 3, 256, 64};
#define NSZ ((int)(sizeof SZ / sizeof SZ[0]))

int main(void)
{
    /* Decay, Tone, LowCut, Level (-1: the default) */
    static const int SET[][4] = {{127, 15, -1, -1}, {127, 115, -1, -1}, {73, 57, -1, -1}, {0, 0, -1, -1},
                                 {64, 64, 0, -1}, {64, 64, 64, -1}, {64, 64, 127, -1}, {64, 64, -1, 64},
                                 {64, 64, -1, 127}};
    float x[N], l[N], r[N], rl[N], rr[N];
    printf("reverb (ROOM):\n");
    for (unsigned c = 0; c < sizeof SET / sizeof SET[0]; c++) {
        double e = 0, d = 0, el = 0, er = 0, elr = 0;
        int last = -1, idle_at = -1;
        const int on = 520 * N;                              /* 3 s of bursts, then the tail: 14 s in all */
        setup(&F, dl1, &rv1, FX_RV_ROOM, SET[c][0], SET[c][1]);
        if (SET[c][2] >= 0)
            fxbus_set(&F, FX_RV_HPF, SET[c][2]);
        if (SET[c][3] >= 0)
            fxbus_set(&F, FX_RV_LEVEL, SET[c][3]);
        memset(&R, 0, sizeof R);
        R.hp = F.rv_hp;                                      /* the same high-pass, fresh */
        rs = 777u;
        for (int t0 = 0, k = c; t0 < 2400 * N; k++) {
            const int n = SZ[k % NSZ];
            send(x, t0, n, on);
            fxbus_process_st(&F, zero, zero, x, zero, l, r, n);
            ref_run(&R, &F, x, rl, rr, n);
            for (int i = 0; i < n; i++) {
                float vl = rl[i] * F.volume, vr = rr[i] * F.volume;
                e += (double)vl * vl + (double)vr * vr;
                d += (double)(l[i] - vl) * (l[i] - vl) + (double)(r[i] - vr) * (r[i] - vr);
                el += (double)l[i] * l[i];
                er += (double)r[i] * r[i];
                elr += (double)l[i] * r[i];
                if (l[i] != 0.0f || r[i] != 0.0f)
                    last = t0 + i;
            }
            t0 += n;
            if (idle_at < 0 && t0 >= on && F.rm_quiet >= FX_RM_QUIET)
                idle_at = t0;
        }
        char what[160];
        snprintf(what, sizeof what, "Decay %3d Tone %3d LowCut %3d Level %3d: against the float reference %.1f dB (< -35)",
                 SET[c][0], SET[c][1], fxbus_get(&F, FX_RV_HPF), fxbus_get(&F, FX_RV_LEVEL), 10 * log10(d / e));
        check(10 * log10(d / e) < -35.0, what);
        snprintf(what, sizeof what, "  silent from %.2f s after the last burst, idle at %.2f s",
                 (last - on) / (double)FS, (idle_at - on) / (double)FS);
        check(last >= 0 && last < 2300 * N && idle_at > 0, what);
        snprintf(what, sizeof what, "  L and R: correlation %.2f (|r| < 0.5), R/L %+.1f dB (within 3)",
                 elr / sqrt(el * er), 10 * log10(er / el));
        check(fabs(elr / sqrt(el * er)) < 0.5 && fabs(10 * log10(er / el)) < 3.0, what);
    }

    {   /* RvLvl 0: nothing out, exactly */
        int silent = 1;
        setup(&F, dl1, &rv1, FX_RV_ROOM, 100, 70);
        fxbus_set(&F, FX_RV_LEVEL, 0);
        rs = 4u;
        for (int b = 0; b < 400; b++) {
            send(x, b * N, N, 300 * N);
            fxbus_process_st(&F, zero, zero, x, zero, l, r, N);
            for (int i = 0; i < N; i++)
                silent &= l[i] == 0.0f && r[i] == 0.0f;
        }
        check(silent, "RvLvl 0: silent");
    }

    {   /* full-scale sine, 60 Hz .. 1 kHz, at the hottest send (part level 127 = x1.61), longest decay */
        double ph = 0, peak = 0;
        int finite = 1;
        setup(&F, dl1, &rv1, FX_RV_ROOM, 127, 115);
        for (int b = 0; b < 1200; b++) {
            for (int i = 0; i < N; i++) {
                double fr = 60.0 * pow(1000.0 / 60.0, (b * N + i) / (1200.0 * N));
                ph += 2 * M_PI * fr / FS;
                x[i] = (float)(1.61 * sin(ph));
            }
            fxbus_process_st(&F, zero, zero, x, zero, l, r, N);
            for (int i = 0; i < N; i++) {
                finite &= isfinite(l[i]) && isfinite(r[i]);
                peak = fmax(peak, fmax(fabs(l[i]), fabs(r[i])));
            }
        }
        char what[120];
        snprintf(what, sizeof what, "a full-scale sweep at the hottest send stays finite (output peak %.2f)", peak);
        check(finite && peak < 100.0, what);
    }

    {   /* ROOM, then back to CLASSIC: from the switch on, a fresh CLASSIC's output exactly */
        int same = 1;
        setup(&F, dl1, &rv1, FX_RV_ROOM, 120, 90);
        setup(&G, dl2, &rv2, FX_RV_CLASSIC, 120, 90);
        rs = 99u;
        for (int b = 0; b < 200; b++) {
            send(x, b * N, N, 200 * N);
            fxbus_process_st(&F, zero, zero, x, zero, l, r, N);
        }
        fxbus_set_rv_type(&F, FX_RV_CLASSIC);
        F.rv_hp = G.rv_hp;                                   /* (the high-pass's own state is not the tail's) */
        F.dl_hp = G.dl_hp;
        for (int b = 0; b < 400; b++) {
            send(x, b * N, N, 300 * N);
            fxbus_process_st(&F, zero, zero, x, zero, l, r, N);
            fxbus_process_st(&G, zero, zero, x, zero, rl, rr, N);
            same &= !memcmp(l, rl, sizeof l) && !memcmp(r, rr, sizeof r);
        }
        check(same, "ROOM -> CLASSIC: the tail is cut, then CLASSIC exactly as fresh");
    }

    {   /* CLASSIC, then ROOM: from the switch on, a fresh ROOM's output exactly */
        int same = 1;
        setup(&F, dl1, &rv1, FX_RV_CLASSIC, 120, 90);
        setup(&G, dl2, &rv2, FX_RV_ROOM, 120, 90);
        rs = 98u;
        for (int b = 0; b < 200; b++) {
            send(x, b * N, N, 200 * N);
            fxbus_process_st(&F, zero, zero, x, zero, l, r, N);
        }
        fxbus_set_rv_type(&F, FX_RV_ROOM);
        F.rv_hp = G.rv_hp;
        F.dl_hp = G.dl_hp;
        for (int b = 0; b < 400; b++) {
            send(x, b * N, N, 300 * N);
            fxbus_process_st(&F, zero, zero, x, zero, l, r, N);
            fxbus_process_st(&G, zero, zero, x, zero, rl, rr, N);
            same &= !memcmp(l, rl, sizeof l) && !memcmp(r, rr, sizeof r);
        }
        check(same, "CLASSIC -> ROOM: the tail is cut, then ROOM exactly as fresh");
    }

    {   /* RvDec, then RvTone, turned while ROOM plays: each time, the coefficients of a fresh ROOM at the
         * new setting (one knob at a time: each must recompute them itself) */
        static const int TURN[2][2] = {{30, 70}, {30, 110}};
        setup(&F, dl1, &rv1, FX_RV_ROOM, 100, 70);
        rs = 6u;
        for (int b = 0; b < 50; b++) {
            send(x, b * N, N, 50 * N);
            fxbus_process_st(&F, zero, zero, x, zero, l, r, N);
        }
        for (int k = 0; k < 2; k++) {
            fxbus_set(&F, k ? FX_RV_TONE : FX_RV_DECAY, TURN[k][k]);
            setup(&G, dl2, &rv2, FX_RV_ROOM, TURN[k][0], TURN[k][1]);
            const fx_room_t *a = &F.rv_buf->room, *g = &G.rv_buf->room;
            check(!memcmp(a->a, g->a, sizeof a->a) && !memcmp(a->c, g->c, sizeof a->c) && a->tap_a == g->tap_a &&
                      a->tap_k == g->tap_k && F.rm_out == G.rm_out,
                  k ? "RvTone turned while ROOM plays: its coefficients follow"
                    : "RvDec turned while ROOM plays: its coefficients follow");
        }
    }

    {   /* mono == the stereo left */
        int same = 1;
        float m[N];
        setup(&F, dl1, &rv1, FX_RV_ROOM, 100, 70);
        setup(&G, dl2, &rv2, FX_RV_ROOM, 100, 70);
        rs = 5u;
        for (int b = 0; b < 400; b++) {
            send(x, b * N, N, 300 * N);
            fxbus_process_st(&F, zero, zero, x, zero, l, r, N);
            fxbus_process(&G, zero, x, zero, m, N);
            same &= !memcmp(l, m, sizeof l);
        }
        check(same, "mono: the stereo left, bit for bit");
    }

    {   /* host time per sample, busy (information) */
        for (int t = 0; t < 2; t++) {
            struct timespec a, b;
            setup(&F, dl1, &rv1, t ? FX_RV_ROOM : FX_RV_CLASSIC, 127, 115);
            rs = 3u;
            clock_gettime(CLOCK_MONOTONIC, &a);
            for (int k = 0; k < 4000; k++) {
                send(x, (k % 100) * N, N, 100 * N);
                fxbus_process_st(&F, zero, zero, x, zero, l, r, N);
            }
            clock_gettime(CLOCK_MONOTONIC, &b);
            printf("  info %s: %.1f ns per sample (the whole fxbus, host)\n", t ? "ROOM   " : "CLASSIC",
                   ((b.tv_sec - a.tv_sec) * 1e9 + (b.tv_nsec - a.tv_nsec)) / (4000.0 * N));
        }
    }
    printf("reverb: %s\n", fails ? "FAIL" : "ok");
    return fails != 0;
}
