/* SPDX-License-Identifier: GPL-3.0-only */
/* X0X TB-303 bass engine - see bass303.h for what is ported and what is not.
 *
 * Reference: schwung-303 src/dsp/open303/rosic_Open303.{h,cpp} (getSample, triggerNote,
 * slideToNote, calculateEnvModScalerAndOffset), rosic_TeeBeeFilter.h (TB_303 mode),
 * rosic_AnalogEnvelope / DecayEnvelope / LeakyIntegrator / OnePoleFilter / BiquadFilter,
 * rosic_MipMappedWaveTable.cpp (fillWithSaw303 / fillWithSquare303), src/dsp/plugin.cpp
 * (pot ranges) and src/dsp/drive.h (Soft / RAT). */
#include "bass303.h"
#include "fastmath.h"

#define FSO (BASS303_SR * (float)BASS303_OS)    /* oversampled rate */

/* ---- parameters ------------------------------------------------------------------------ */

static const char *const wave_names[] = { "Saw", "Sqr" };
static const char *const drv_names[] = { "Off", "Soft", "RAT" };

/* defaults are schwung-303's (every knob at 0.5, Devilfish values stock), except Volume,
 * lowered from 0.8 so the nominal peak is ~0.5 */
static const x0x_param_t params[BASS303_NPARAMS] = {
    { "Cutoff", 127, 64, 0 },        /* 314 .. 2394 Hz, exponential */
    { "Reso", 127, 64, 0 },          /* 0 .. 100 % */
    { "EnvMod", 127, 64, 0 },        /* 0 .. 100 % */
    { "Decay", 127, 64, 0 },         /* 200 .. 2000 ms, exponential (stock 303 range) */
    { "Accent", 127, 64, 0 },        /* 0 .. 100 % */
    { "Wave", 1, 0, wave_names },
    { "Tune", 127, 64, 0 },          /* A4 = 440 + (pot - 64) * 0.625 Hz: 400 .. 479.4 */
    { "Volume", 127, 96, 0 },        /* -60 .. 0 dB; 96 = -14.6 dB: peaks ~0.5 (schwung-303: 102) */
    { "Drive", 127, 0, 0 },          /* 0 = bypass */
    { "DrvTyp", 2, 1, drv_names },
    { "Slide", 127, 21, 0 },         /* 2 .. 360 ms (Devilfish); 21 = 61 ms, stock is 60 */
    { "AccDec", 127, 7, 0 },         /* 30 .. 3000 ms (Devilfish); 7 = 194 ms, stock is 200 */
};

/* ---- drive scaling (X0X; not in drive.h) ------------------------------------------------
 * The drive always sees the voice at one level, DRV_REF, and Volume is applied after it, so
 * Volume no longer sets how hard the drive is hit. A makeup gain per Drive pot keeps the
 * loudness level with Drive 0 (K-weighted, ITU-R BS.1770, measured on an acid line: cutoff 45,
 * reso 95, envmod 85, decay 45, accent 90, saw), and a deliberate rise on top of it, linear in
 * dB, reaches +3 dB (Soft) / +4.5 dB (RAT) at full Drive. Drive 0 is still a bypass. */
#define DRV_REF 0.18620871f              /* the level the tables were measured at (Volume ~96) */
#define DRV_RISE_SOFT 3.0f
#define DRV_RISE_RAT 4.5f
#define RAT_XF 8                         /* RAT: Drive 1..7 crossfade the dry voice into it */
#define RAT_TONE_HZ 936.48098f           /* the stock tone: 1 / (2 pi (0.5 * 100k + 1.5k) 3.3n) */

static const float makeup_soft[128] = {
    1.0000f, 1.0262f, 1.0275f, 1.0289f, 1.0303f, 1.0319f, 1.0335f, 1.0354f,
    1.0372f, 1.0393f, 1.0414f, 1.0438f, 1.0462f, 1.0491f, 1.0520f, 1.0548f,
    1.0577f, 1.0613f, 1.0649f, 1.0685f, 1.0721f, 1.0766f, 1.0810f, 1.0854f,
    1.0899f, 1.0957f, 1.1015f, 1.1074f, 1.1133f, 1.1192f, 1.1252f, 1.1312f,
    1.1372f, 1.1452f, 1.1533f, 1.1614f, 1.1696f, 1.1778f, 1.1861f, 1.1945f,
    1.2029f, 1.2135f, 1.2243f, 1.2351f, 1.2460f, 1.2570f, 1.2681f, 1.2793f,
    1.2906f, 1.3044f, 1.3183f, 1.3323f, 1.3465f, 1.3608f, 1.3753f, 1.3899f,
    1.4047f, 1.4222f, 1.4400f, 1.4579f, 1.4761f, 1.4945f, 1.5132f, 1.5320f,
    1.5511f, 1.5733f, 1.5958f, 1.6186f, 1.6418f, 1.6652f, 1.6891f, 1.7132f,
    1.7377f, 1.7657f, 1.7941f, 1.8230f, 1.8524f, 1.8822f, 1.9125f, 1.9433f,
    1.9746f, 2.0098f, 2.0456f, 2.0821f, 2.1192f, 2.1570f, 2.1954f, 2.2345f,
    2.2744f, 2.3186f, 2.3637f, 2.4096f, 2.4565f, 2.5042f, 2.5529f, 2.6026f,
    2.6532f, 2.7087f, 2.7653f, 2.8232f, 2.8822f, 2.9425f, 3.0041f, 3.0669f,
    3.1311f, 3.2007f, 3.2720f, 3.3447f, 3.4192f, 3.4952f, 3.5730f, 3.6525f,
    3.7337f, 3.8212f, 3.9106f, 4.0022f, 4.0959f, 4.1917f, 4.2899f, 4.3903f,
    4.4931f, 4.6024f, 4.7144f, 4.8291f, 4.9467f, 5.0670f, 5.1903f, 5.3166f,
};
static const float makeup_rat[128] = {
    1.0000f, 1.6276f, 1.6276f, 1.6276f, 1.6276f, 1.6276f, 1.6276f, 1.6276f,
    1.6276f, 1.6289f, 1.6303f, 1.6320f, 1.6336f, 1.6143f, 1.5953f, 1.5764f,
    1.5578f, 1.5273f, 1.4975f, 1.4682f, 1.4395f, 1.4072f, 1.3756f, 1.3448f,
    1.3146f, 1.2824f, 1.2510f, 1.2204f, 1.1905f, 1.1614f, 1.1329f, 1.1052f,
    1.0781f, 1.0539f, 1.0303f, 1.0072f, 0.9846f, 0.9625f, 0.9409f, 0.9198f,
    0.8992f, 0.8854f, 0.8718f, 0.8584f, 0.8453f, 0.8323f, 0.8195f, 0.8070f,
    0.7946f, 0.7892f, 0.7838f, 0.7785f, 0.7732f, 0.7680f, 0.7628f, 0.7576f,
    0.7524f, 0.7521f, 0.7519f, 0.7516f, 0.7513f, 0.7510f, 0.7507f, 0.7504f,
    0.7501f, 0.7414f, 0.7328f, 0.7242f, 0.7158f, 0.7075f, 0.6992f, 0.6911f,
    0.6830f, 0.6695f, 0.6562f, 0.6431f, 0.6304f, 0.6178f, 0.6056f, 0.5935f,
    0.5818f, 0.5713f, 0.5610f, 0.5508f, 0.5409f, 0.5311f, 0.5216f, 0.5122f,
    0.5029f, 0.4950f, 0.4871f, 0.4794f, 0.4718f, 0.4644f, 0.4570f, 0.4498f,
    0.4427f, 0.4367f, 0.4307f, 0.4249f, 0.4191f, 0.4134f, 0.4078f, 0.4022f,
    0.3968f, 0.3927f, 0.3886f, 0.3845f, 0.3805f, 0.3766f, 0.3727f, 0.3688f,
    0.3649f, 0.3619f, 0.3590f, 0.3560f, 0.3531f, 0.3501f, 0.3473f, 0.3444f,
    0.3416f, 0.3385f, 0.3354f, 0.3324f, 0.3294f, 0.3264f, 0.3235f, 0.3206f,
};

int bass303_nparams(void) { return BASS303_NPARAMS; }

const x0x_param_t *bass303_param(int i)
{
    return (i >= 0 && i < BASS303_NPARAMS) ? &params[i] : 0;
}

int bass303_get(const bass303_t *b, int i)
{
    return (i >= 0 && i < BASS303_NPARAMS) ? b->pot[i] : 0;
}

/* exp(-1/(tau_ms * fs / 1000)): one-pole / RC coefficient */
static float rc_coeff(float tau_ms, float fs)
{
    return tau_ms > 0.0f ? fm_expf(-1000.0f / (tau_ms * fs)) : 0.0f;
}

/* pw[m] = c^m, m = 0..BASS303_CTRL */
static void powers(float *pw, float c)
{
    int m;
    pw[0] = 1.0f;
    for (m = 1; m <= BASS303_CTRL; m++)
        pw[m] = pw[m - 1] * c;
}

/* the main envelope's multiplier changed: its powers, and the closed-form gains of the two RC
 * followers it drives, g[m] = (1-a) sum_{n=1..m} a^(m-n) c^n (so r_m = a^m r_0 + g[m] e_0) */
static void env_tables(bass303_t *b)
{
    int m;
    powers(b->env_pw, b->env_c);
    b->g1[0] = b->g2[0] = 0.0f;
    for (m = 1; m <= BASS303_CTRL; m++) {
        b->g1[m] = b->rc1_c * b->g1[m - 1] + (1.0f - b->rc1_c) * b->env_pw[m];
        b->g2[m] = b->rc2_c * b->g2[m - 1] + (1.0f - b->rc2_c) * b->env_pw[m];
    }
}

/* Open303::calculateEnvModScalerAndOffset, the measured mapping */
static void calc_envmod(bass303_t *b)
{
    float e = (float)b->pot[BASS303_ENVMOD] * (1.0f / 127.0f);
    float c = fm_log2f(b->cutoff * (1.0f / 313.8152786f)) * (1.0f / 2.931683907f); /* log2(c1/c0) */
    float slo = 3.773996325f * e + 0.736965594f;
    float shi = 4.194548788f * e + 0.864344901f;
    b->env_scaler = (1.0f - c) * slo + c * shi;
    b->env_offset = 0.048292931f * c + 0.294391201f;
}

/* drive.h set_lowpass / set_low_shelf (TDF-II coefficients, a0-normalised) */
static void bq_lowpass(bass303_bq_t *q, float fc, float fs)
{
    float w = FM_TWO_PI * fc / fs, cs = fm_cosf(w), sn = fm_sinf(w);
    float alpha = sn * 0.707106781f, a0 = 1.0f + alpha;     /* sn / (2 q), q = 1/sqrt2 */
    q->b0 = 0.5f * (1.0f - cs) / a0;
    q->b1 = (1.0f - cs) / a0;
    q->b2 = q->b0;
    q->a1 = -2.0f * cs / a0;
    q->a2 = (1.0f - alpha) / a0;
}

static void bq_low_shelf(bass303_bq_t *q, float fc, float gain_db, float fs)
{
    float A = fm_db2lin(0.5f * gain_db);                       /* 10^(g/40) */
    float w = FM_TWO_PI * fc / fs, cs = fm_cosf(w), sn = fm_sinf(w);
    float alpha = sn * 0.707106781f;                           /* S = 1: sn/2 * sqrt(2) */
    float s2 = 2.0f * fm_sqrtf(A) * alpha;
    float a0 = (A + 1.0f) + (A - 1.0f) * cs + s2;
    q->b0 = A * ((A + 1.0f) - (A - 1.0f) * cs + s2) / a0;
    q->b1 = 2.0f * A * ((A - 1.0f) - (A + 1.0f) * cs) / a0;
    q->b2 = A * ((A + 1.0f) - (A - 1.0f) * cs - s2) / a0;
    q->a1 = -2.0f * ((A - 1.0f) + (A + 1.0f) * cs) / a0;
    q->a2 = ((A + 1.0f) + (A - 1.0f) * cs - s2) / a0;
}

static inline float bq_run(bass303_bq_t *q, float x)
{
    float y = q->b0 * x + q->z1;
    q->z1 = q->b1 * x - q->a1 * y + q->z2;
    q->z2 = q->b2 * x - q->a2 * y;
    return y;
}

/* the RAT's Drive taper, 0..1 over pots 8..127 (below 8 the dry crossfade does the work) */
static float rat_t(int pot)
{
    return fm_clampf((float)(pot - RAT_XF) * (1.0f / (127.0f - RAT_XF)), 0.0f, 1.0f);
}

/* RAT distortion resistance, 0..1 (drive.h's dist). drive.h maps the pot to it linearly, so the
 * op-amp's gain at 1 kHz, 1 + dist * 100k / |Z1| (|Z1| ~ 81 ohm), is already +21 dB at the first
 * step and most of the knob is used up in its first tenth. Here the knob is linear (to the power
 * 1.3) in that gain in dB, up to drive.h's maximum (dist = 1, ~61.8 dB) */
static float rat_dist(int pot)
{
    float t = rat_t(pot);
    if (t <= 0.0f)
        return 0.0f;
    return fm_minf((fm_db2lin(61.8f * fm_powf(t, 1.3f)) - 1.0f) * 81e-5f, 1.0f);
}

/* the RAT's tone one-pole (drive.h: fixed at the stock 936 Hz) opens to 12 kHz at the bottom of
 * the taper and closes, log-interpolated, to the stock tone by its middle: so low Drive brightens
 * the voice, as the distortion would, instead of dulling it before the distortion arrives */
static void rat_tone(bass303_t *b)
{
    float u = fm_minf(2.0f * rat_t(b->pot[BASS303_DRIVE]), 1.0f);
    float fc = RAT_TONE_HZ * fm_exp2f((1.0f - u) * 3.6796409f);   /* log2(12000 / RAT_TONE_HZ) */
    b->r_tone_b1 = fm_expf(-FM_TWO_PI / BASS303_SR * fc);
}

/* the gains after the drive: makeup with its rise, and the RAT's dry crossfade */
static void drv_gains(bass303_t *b)
{
    int p = b->pot[BASS303_DRIVE], rat = b->drv_type == BASS303_DRV_RAT;
    float rise = (rat ? DRV_RISE_RAT : DRV_RISE_SOFT) * (float)p * (1.0f / 127.0f);
    b->drv_makeup = (rat ? makeup_rat : makeup_soft)[p] * fm_db2lin(rise);
    b->drv_xf = rat && p < RAT_XF ? (float)p * (1.0f / RAT_XF) : 1.0f;
}

static int drive_on(const bass303_t *b) { return b->drv_type != BASS303_DRV_OFF && b->drv_amt > 0.0f; }

/* RAT op-amp: drive.h rat::State::update_op_amp_coeffs + the correction one-pole cutoff */
static void rat_coeffs(bass303_t *b)
{
    const float R1 = 100000.0f, C1 = 1e-10f, Z1_B0 = 2.72149e-7f, Z1_B1 = 0.0027354f;
    const float Z1_A0 = 6.27638e-9f, Z1_A1 = 0.0000069f;
    const float t = 1.0f / BASS303_SR, s0 = t * 0.5f, s1 = t * t * 0.25f, s2 = t * t * t * 0.125f;
    float dist = fm_maxf(rat_dist(b->pot[BASS303_DRIVE]), 0.0001f);
    float z2_b0 = fm_maxf(dist * R1, 1.0f), z2_a0 = z2_b0 * C1;
    float a0 = Z1_B0 * z2_a0, a1 = Z1_B0 + Z1_B1 * z2_a0, a2 = Z1_B1 + z2_a0;
    float num[4], den[4], nz[4], dz[4], inv;
    int i;
    num[0] = a0; num[1] = a1 + Z1_A0 * z2_b0; num[2] = Z1_A1 * z2_b0 + Z1_B1 + z2_a0; num[3] = 1.0f;
    den[0] = a0; den[1] = a1; den[2] = a2; den[3] = 1.0f;
    for (i = 0; i < 2; i++) {                                  /* bilinear transform */
        const float *x = i ? den : num;
        float *o = i ? dz : nz;
        float x0 = x[0], x1 = x[1] * s0, x2 = x[2] * s1, x3 = x[3] * s2;
        o[0] = x0 + x1 + x2 + x3;
        o[1] = -3.0f * x0 - x1 + x2 + 3.0f * x3;
        o[2] = 3.0f * x0 - x1 - x2 + 3.0f * x3;
        o[3] = -x0 + x1 - x2 + x3;
    }
    inv = 1.0f / dz[0];
    for (i = 0; i < 4; i++) {
        b->r_b[i] = nz[i] * inv;
        b->r_a[i] = dz[i] * inv;
    }
    /* correction cutoff = MAX_GAIN_AT_1HZ / (dist * (MAX_DIST_GAIN - 1) + 1) */
    b->r_corr_b1 = fm_expf(-FM_TWO_PI / BASS303_SR * 1119360.558f / (dist * 2306.231003f + 1.0f));
}

void bass303_set(bass303_t *b, int i, int v)
{
    float n;
    if (i < 0 || i >= BASS303_NPARAMS)
        return;
    if (v < 0)
        v = 0;
    if (v > params[i].max)
        v = params[i].max;
    b->pot[i] = (uint8_t)v;
    n = (float)v * (1.0f / 127.0f);
    switch (i) {
    case BASS303_CUTOFF:
        b->cutoff = 314.0f * fm_exp2f(n * 2.930586688f);       /* log2(2394/314) */
        calc_envmod(b);
        break;
    case BASS303_ENVMOD:
        calc_envmod(b);
        break;
    case BASS303_RESO:                                         /* TeeBeeFilter::setResonance */
        b->reso = (1.0f - fm_expf(-3.0f * n)) * (1.0f / 0.950212932f);
        break;
    case BASS303_DECAY:                                        /* applied at the next note */
        b->decay_c = rc_coeff(200.0f * fm_exp2f(n * 3.321928095f), BASS303_SR);
        break;
    case BASS303_ACCDEC:
        b->accdec_c = rc_coeff(30.0f + 2970.0f * n, BASS303_SR);
        break;
    case BASS303_ACCENT:
        b->accent = n;
        break;
    case BASS303_WAVE:
        b->wave = v;
        break;
    case BASS303_TUNE:
        b->tuning = 440.0f + (float)(v - 64) * 0.625f;
        break;
    case BASS303_VOLUME:
        b->amp_scaler = fm_db2lin(-60.0f + 60.0f * n);
        break;
    case BASS303_SLIDE:                                        /* slew tau = 0.2 * slide time */
        b->slew_c = rc_coeff(0.2f * (2.0f + 358.0f * n), BASS303_SR);
        powers(b->slew_pw, b->slew_c);
        break;
    case BASS303_DRIVE:
        b->drv_amt = n;
        b->s_gain = fm_db2lin(-2.0f + 32.0f * n);             /* drive.h: 24 n dB (at the Volume's level) */
        b->s_inv = 1.0f / b->s_gain;
        rat_coeffs(b);
        rat_tone(b);
        drv_gains(b);
        break;
    case BASS303_DRVTYPE:
        if (v != b->drv_type) {                                /* drive.h set_model: fresh state */
            b->s_pre.z1 = b->s_pre.z2 = b->s_post.z1 = b->s_post.z2 = 0.0f;
            b->up_lp.z1 = b->up_lp.z2 = b->down_lp.z1 = b->down_lp.z2 = 0.0f;
            b->r_z[0] = b->r_z[1] = b->r_z[2] = b->r_corr_z = b->r_tone_z = 0.0f;
            b->dcb_x1 = b->dcb_y1 = 0.0f;
        }
        b->drv_type = v;
        drv_gains(b);
        break;
    }
}

/* clear everything that only matters while sound is running (Open303::triggerNote after idle) */
static void clear_audio_state(bass303_t *b)
{
    int i;
    b->dc.x1 = b->dc.x2 = b->dc.y1 = b->dc.y2 = 0.0f;
    b->hp1_x1 = b->hp1_y1 = 0.0f;
    b->f_y1 = b->f_y2 = b->f_y3 = b->f_y4 = b->fb_x1 = b->fb_y1 = 0.0f;
#if BASS303_OS >= 2
    for (i = 0; i < 6; i++)
        b->hb1[i].x1 = b->hb1[i].y1 = 0.0f;
#endif
#if BASS303_OS == 4
    for (i = 0; i < 3; i++)
        b->hb2[i].x1 = b->hb2[i].y1 = 0.0f;
#endif
    b->ap_x1 = b->ap_y1 = b->hp2_x1 = b->hp2_y1 = 0.0f;
    b->nt.x1 = b->nt.x2 = b->nt.y1 = b->nt.y2 = 0.0f;
    b->s_pre.z1 = b->s_pre.z2 = b->s_post.z1 = b->s_post.z2 = 0.0f;
    b->up_lp.z1 = b->up_lp.z2 = b->down_lp.z1 = b->down_lp.z2 = 0.0f;
    for (i = 0; i < 3; i++)
        b->r_z[i] = 0.0f;
    b->r_corr_z = b->r_tone_z = 0.0f;
    b->dcb_x1 = b->dcb_y1 = 0.0f;
    b->amp_y = 0.0f;
}

void bass303_init(bass303_t *b)
{
    uint8_t *p = (uint8_t *)b;
    unsigned k;
    int i;
    float w, sn, cs, alpha, scale, x, F, O, ln_cosh_p, ln_cosh_m;

    for (k = 0; k < sizeof(*b); k++)
        p[k] = 0;

    /* fixed (Open303 constructor + setSampleRate) */
    b->rc1_c = rc_coeff(3.0f, BASS303_SR);           /* normalAttack */
    b->rc2_c = rc_coeff(15.0f, BASS303_SR);
    powers(b->rc1_pw, b->rc1_c);
    powers(b->rc2_pw, b->rc2_c);
    powers(b->ampd_pw, rc_coeff(1230.0f, BASS303_SR));         /* amp: y += (1 - c)(0 - y) */
    powers(b->ampn_pw, rc_coeff(1.0f, BASS303_SR));            /* release, normal: 1 ms */
    powers(b->ampa_pw, rc_coeff(50.0f, BASS303_SR));           /* release, accented: 50 ms */

    w = FM_TWO_PI * 200.0f / BASS303_SR;             /* de-clicker: LOWPASS12, q = sqrt(.5) */
    sn = fm_sinf(w);
    cs = fm_cosf(w);
    alpha = sn * 0.707106781f;
    scale = 1.0f / (1.0f + alpha);
    b->dc_a1 = 2.0f * cs * scale;
    b->dc_a2 = (alpha - 1.0f) * scale;
    b->dc_b1 = (1.0f - cs) * scale;
    b->dc_b0 = 0.5f * b->dc_b1;

    x = fm_expf(-FM_TWO_PI * 44.486f / FSO);         /* OnePoleFilter HIGHPASS (dspguide) */
    b->hp1_b0 = 0.5f * (1.0f + x);
    b->hp1_a1 = x;
    x = fm_expf(-FM_TWO_PI * 150.0f / FSO);          /* feedback highpass (stock 150 Hz) */
    b->fbhp_b0 = 0.5f * (1.0f + x);
    b->fbhp_a1 = x;
    x = fm_expf(-FM_TWO_PI * 44.486f / BASS303_SR);  /* the same two at the base rate (lite) */
    b->hp1l_b0 = 0.5f * (1.0f + x);
    b->hp1l_a1 = x;
    x = fm_expf(-FM_TWO_PI * 150.0f / BASS303_SR);
    b->fbhpl_b0 = 0.5f * (1.0f + x);
    b->fbhpl_a1 = x;
    x = fm_expf(-FM_TWO_PI * 24.167f / BASS303_SR);
    b->hp2_b0 = 0.5f * (1.0f + x);
    b->hp2_a1 = x;
    x = fm_tanf(FM_PI * 14.008f / BASS303_SR);       /* OnePoleFilter ALLPASS (DAFX) */
    b->ap_b0 = (x - 1.0f) / (x + 1.0f);
    b->ap_a1 = -b->ap_b0;

    w = FM_TWO_PI * 7.5164f / BASS303_SR;            /* BANDREJECT, 4.7 octaves */
    sn = fm_sinf(w);
    x = fm_sinf(0.5f * w);
    cs = 1.0f - 2.0f * x * x;                        /* cos w without the cancellation */
    x = 0.5f * FM_LN2 * 4.7f * w / sn;               /* alpha = sn * sinh(x) */
    alpha = sn * 0.5f * (fm_expf(x) - fm_expf(-x));
    scale = 1.0f / (1.0f + alpha);
    b->nt_a1 = 2.0f * cs * scale;
    b->nt_a2 = (alpha - 1.0f) * scale;
    b->nt_b0 = scale;
    b->nt_b1 = -2.0f * cs * scale;

    /* 303 square = -tanh(F * (2p - 1) + O), F = dB2amp(36.9), O = 4.37 (fillWithSquare303,
     * after its 180 degree circular shift). Mean over a cycle, for the DC the mip-map drops:
     * -(ln cosh(F + O) - ln cosh(F - O)) / 2F; ln cosh(y) = y - ln 2 + ln(1 + e^-2y), y > 0. */
    F = fm_db2lin(36.9f);
    O = 4.37f;
    ln_cosh_p = (F + O) + fm_logf(1.0f + fm_expf(-2.0f * (F + O)));
    ln_cosh_m = (F - O) + fm_logf(1.0f + fm_expf(-2.0f * (F - O)));
    b->sq_dc = -(ln_cosh_p - ln_cosh_m) / (2.0f * F);
    b->sq_h = fm_tanhf(F + O) + fm_tanhf(F - O);    /* height of the hard edge at p = 0 */

    /* drive.h prepare() */
    bq_low_shelf(&b->s_pre, 400.0f, 6.0f, BASS303_SR);
    bq_low_shelf(&b->s_post, 400.0f, -6.0f, BASS303_SR);
    bq_lowpass(&b->up_lp, 19000.0f, 2.0f * BASS303_SR);
    bq_lowpass(&b->down_lp, 19000.0f, 2.0f * BASS303_SR);
    /* (the RAT's tone: rat_tone, with Drive) */

    b->drv_type = -1;
    for (i = 0; i < BASS303_NPARAMS; i++)
        bass303_set(b, i, params[i].def);

    b->osc_freq = b->slew_y = 440.0f;
    b->env_c = b->decay_c;
    env_tables(b);
    b->env_y = 0.0f;
    b->idle = 1;
    b->snap = 1;
}

/* ---- notes ----------------------------------------------------------------------------- */

static void set_accent_state(bass303_t *b, int accent)
{
    if (accent) {
        b->accent_gain = b->accent;
        b->env_c = b->accdec_c;          /* setMainEnvDecay(accentDecay) */
    } else {
        b->accent_gain = 0.0f;
        b->env_c = b->decay_c;
    }
    b->amp_acc = accent != 0;            /* release: accentAmpRelease 50 ms, normal 1 ms */
    env_tables(b);
}

void bass303_note_on(bass303_t *b, int note, int accent, int slide)
{
    float f;
    if (note < 0)
        note = 0;
    if (note > 127)
        note = 127;
    f = b->tuning * fm_exp2f((float)(note - 69) * (1.0f / 12.0f));
    set_accent_state(b, accent);
    b->osc_freq = f;
    if (!(slide && b->gate)) {           /* triggerNote; else slideToNote (no retrigger) */
        b->slew_y = f;                   /* pitchSlewLimiter.setState */
        b->env_y = 1.0f / b->env_c;      /* mainEnv.trigger: the next sample is 1 */
        b->amp_trig = 1;                 /* ampEnv.noteOn: attack time 0 */
        b->trig = 1;
    }
    if (b->idle)
        b->snap = 1;                     /* no interpolation from the stale control values */
    b->gate = 1;
    b->idle = 0;
    b->quiet = 0;
    b->started = 1;
}

void bass303_note_off(bass303_t *b) { b->gate = 0; }
void bass303_set_lite(bass303_t *b, int on) { b->lite = on != 0; }
void bass303_all_off(bass303_t *b) { b->gate = 0; }

/* ---- render ---------------------------------------------------------------------------- */

/* polyBLEP residual for a unit (2-high) step at phase 0, t in [0, 1), dt = increment. The
 * divides happen only on the two samples per period next to the step. */
static inline float blep(float t, float dt)
{
    float x;
    if (t < dt) {
        x = t / dt;
        return x + x - x * x - 1.0f;
    }
    if (t > 1.0f - dt) {
        x = (t - 1.0f) / dt;
        return x * x + x + x + 1.0f;
    }
    return 0.0f;
}

#if BASS303_OS >= 2
static inline float hb(bass303_ap_t *s, float a, float x)
{
    float y = a * (x - s->y1) + s->x1;
    s->x1 = x;
    s->y1 = y;
    return y;
}
#endif

/* Halfband decimators: polyphase allpass pairs (de Soras' hiir structure; coefficients from
 * its closed-form elliptic design, response checked numerically). 2x -> 1x: 6 coefficients,
 * transition 0.04 (passband flat to 18.5 kHz, -74.5 dB from 25.6 kHz at the 2x rate). 4x -> 2x:
 * 3 coefficients, transition 0.137 (flat to 19.9 kHz, -62 dB over the band that folds below
 * 20 kHz). 8 / 4 coefficients (-99 / -81 dB) measured the same alias floor in
 * tests/host/run_bass303.sh: what reaches the decimator is already low-passed by the ladder.
 * Paths: even coefficients take the newer sample, odd ones the older. */
#if BASS303_OS >= 2
static inline float decim_hb1(bass303_ap_t *s, float x_old, float x_new)
{
    float a = x_new, c = x_old;
    a = hb(&s[0], 0.068204076f, a);
    c = hb(&s[1], 0.240270358f, c);
    a = hb(&s[2], 0.448676236f, a);
    c = hb(&s[3], 0.641122367f, c);
    a = hb(&s[4], 0.799997564f, a);
    c = hb(&s[5], 0.934482236f, c);
    return 0.5f * (a + c);
}
#endif
#if BASS303_OS == 4
static inline float decim_hb2(bass303_ap_t *s, float x_old, float x_new)
{
    float a = x_new, c = x_old;
    a = hb(&s[0], 0.104996100f, a);
    c = hb(&s[1], 0.375357314f, c);
    a = hb(&s[2], 0.754467339f, a);
    return 0.5f * (a + c);
}
#endif

/* While idle, advance what Open303 keeps running so the next note starts where the
 * reference would: oscillator phase, slew, the main envelope and its RC followers
 * (closed forms of N steps of each recursion). */
static void idle_advance(bass303_t *b, int n)
{
    float N = (float)n, cn, an, ph;
    if (!b->started)
        return;                                       /* Open303 does not run before note 1 */
    ph = b->phase + N * b->slew_y * (1.0f / BASS303_SR);    /* N base samples */
    b->phase = ph - fm_floorf(ph);
    an = fm_powf(b->slew_c, N);
    b->slew_y = b->osc_freq + an * (b->slew_y - b->osc_freq);
    cn = fm_powf(b->env_c, N);
    an = fm_powf(b->rc1_c, N);
    /* r[n] = a r[n-1] + (1-a) u[n], u[n] = E c^n:  r_N = a^N r0 + (1-a) E c (c^N - a^N)/(c - a) */
    b->rc1_y = an * b->rc1_y + (1.0f - b->rc1_c) * b->env_y * b->env_c * (cn - an) / (b->env_c - b->rc1_c);
    an = fm_powf(b->rc2_c, N);
    if (b->accent_gain > 0.0f)
        b->rc2_y = an * b->rc2_y + (1.0f - b->rc2_c) * b->env_y * b->env_c * (cn - an) / (b->env_c - b->rc2_c);
    else
        b->rc2_y *= an;
    b->env_y = fm_flush(b->env_y * cn);
    b->rc1_y = fm_flush(b->rc1_y);
    b->rc2_y = fm_flush(b->rc2_y);
}

static void drive_block(bass303_t *b, float *out, int n)
{
    int i;
    float y, v, u0, u1, yh;
    /* Volume, the makeup and the crossfade, after the drive: out = vol (dry (1 - xf) + wet makeup xf) */
    const float vol = b->amp_scaler * (1.0f / DRV_REF);
    const float wg = vol * b->drv_xf * b->drv_makeup, dg = vol * (1.0f - b->drv_xf);
    if (b->drv_type == BASS303_DRV_SOFT) {
        const float g = b->s_gain, ig = b->s_inv;
        for (i = 0; i < n; i++) {
            v = bq_run(&b->s_pre, out[i]) * g;
            u0 = 2.0f * bq_run(&b->up_lp, v);
            u1 = 2.0f * bq_run(&b->up_lp, 0.0f);
            bq_run(&b->down_lp, fm_tanhf(u0 + 0.15f) - 0.14888503f);
            y = bq_run(&b->down_lp, fm_tanhf(u1 + 0.15f) - 0.14888503f);
            y = bq_run(&b->s_post, y * ig);
            yh = y - b->dcb_x1 + 0.9996f * b->dcb_y1;
            b->dcb_x1 = y;
            b->dcb_y1 = yh;
            out[i] = wg * yh;
        }
    } else {                                           /* RAT */
        const float *rb = b->r_b, *ra = b->r_a;
        const float cb1 = b->r_corr_b1, tb1 = b->r_tone_b1;
        float x, x2;
        for (i = 0; i < n; i++) {
            x = out[i];
            y = x * rb[0] + b->r_z[0];                 /* op-amp, 3rd-order TDF-II */
            b->r_z[0] = x * rb[1] - y * ra[1] + b->r_z[1];
            b->r_z[1] = x * rb[2] - y * ra[2] + b->r_z[2];
            b->r_z[2] = x * rb[3] - y * ra[3];
            b->r_corr_z = y * (1.0f - cb1) + b->r_corr_z * cb1;
            v = b->r_corr_z * 1.877f;
            u0 = 2.0f * bq_run(&b->up_lp, v);
            u1 = 2.0f * bq_run(&b->up_lp, 0.0f);
            {   /* x / (1 + x^4)^(1/4) = x s t, s = y^(-1/2), t = s^(-1/2) = y^(1/4): no divide */
                float s0, s1;
                x2 = u0 * u0;
                s0 = fm_rsqrtf(1.0f + x2 * x2);
                u0 *= s0 * fm_rsqrtf(s0);
                x2 = u1 * u1;
                s1 = fm_rsqrtf(1.0f + x2 * x2);
                u1 *= s1 * fm_rsqrtf(s1);
            }
            bq_run(&b->down_lp, u0);
            y = bq_run(&b->down_lp, u1) * 0.3204805f;
            b->r_tone_z = y * (1.0f - tb1) + b->r_tone_z * tb1;
            y = b->r_tone_z;
            yh = y - b->dcb_x1 + 0.9996f * b->dcb_y1;
            b->dcb_x1 = y;
            b->dcb_y1 = yh;
            out[i] = dg * x + wg * yh;
        }
    }
}

static const float recip[5] = { 0.0f, 1.0f, 0.5f, 0.333333343f, 0.25f };

static void run(bass303_t *b, float *out, int n)
{
    int i, j, m;
    /* hot state in locals: out[] may alias the struct's floats, which would force reloads */
    float phase = b->phase, inc = b->c_inc, f_b0 = b->c_b0, k = b->c_k, g2 = b->c_g2, a = b->c_a;
    float hp1x = b->hp1_x1, hp1y = b->hp1_y1;
    float y1 = b->f_y1, y2 = b->f_y2, y3 = b->f_y3, y4 = b->f_y4, fbx = b->fb_x1, fby = b->fb_y1;
    const float osc_freq = b->osc_freq, acc = b->accent_gain;
    const float scaler = b->env_scaler, offset = b->env_offset, cutoff = b->cutoff, r = b->reso;
    const float *amp_pw = b->gate ? b->ampd_pw : b->amp_acc ? b->ampa_pw : b->ampn_pw;
    const float amp_boost = b->gate ? 0.45f + 4.0f * acc : 0.0f;
    /* the overload guard's lite mode: no oversampling; a change of rate jumps to the new rate's
     * coefficients (snap) rather than gliding between the two rates' values */
    const int lite = b->lite;
    const int nos = lite ? 1 : BASS303_OS;
    const float fso = lite ? BASS303_SR : FSO;
    const float hp1_b0 = lite ? b->hp1l_b0 : b->hp1_b0, hp1_a1 = lite ? b->hp1l_a1 : b->hp1_a1;
    const float fb_b0 = lite ? b->fbhpl_b0 : b->fbhp_b0, fb_a1 = lite ? b->fbhpl_a1 : b->fbhp_a1;
    const float sq_dc = b->sq_dc, sq_h2 = 0.5f * b->sq_h;
    const float out_gain = drive_on(b) ? DRV_REF : b->amp_scaler;   /* driven: Volume after it */
    const int square = b->wave;
    float os[BASS303_OS];

    for (i = 0; i < n; i += m) {
        float e0, fc, fx, n_b0, n_k, n_g2, n_inc, n_a, rm, d_inc, d_b0, d_k, d_g2, d_a;
        m = n - i < BASS303_CTRL ? n - i : BASS303_CTRL;

        /* ---- control step: Open303's per-sample recursions advanced m samples in closed
         * form (exact), then the cutoff and amp they produce at the chunk's last sample */
        e0 = b->env_y;                                         /* main envelope: y *= c */
        b->env_y = e0 * b->env_pw[m];
        b->rc1_y = b->rc1_pw[m] * b->rc1_y + b->g1[m] * e0;    /* RC followers of it */
        b->rc2_y = b->rc2_pw[m] * b->rc2_y + (acc > 0.0f ? b->g2[m] * e0 : 0.0f);
        b->slew_y = osc_freq + b->slew_pw[m] * (b->slew_y - osc_freq);
        if (b->amp_trig) {                                     /* attack 0: 1, then decay */
            b->amp_y = amp_pw[m - 1];
            b->amp_trig = 0;
        } else {
            b->amp_y *= amp_pw[m];
        }
        if (lite != b->lite_cur) {
            b->lite_cur = lite;
            b->snap = 1;
        }
        n_inc = b->slew_y * (1.0f / fso);
        n_a = b->amp_y + amp_boost * b->env_y;
        fc = cutoff * fm_exp2f(scaler * (b->rc1_y - offset) + acc * b->rc2_y);   /* n1 = n2 = 1 */
        fc = fm_clampf(fc, 200.0f, 20000.0f);
        /* TeeBeeFilter::calculateCoefficientsApprox4, TB_303 branch only (the a1 polynomial
         * it computes first is overwritten there, so it is skipped) */
        fx = fc * (0.707106781f / fso);
        n_b0 = (0.00045522346f + 6.1922189f * fx) / (1.0f + 12.358354f * fx + 4.4156345f * (fx * fx));
        n_k = fx * (fx * (fx * (fx * (fx * (fx + 7198.6997f) - 5837.7917f) - 476.47308f) + 614.95611f) + 213.87126f) + 16.998792f;
        n_g2 = 2.0f * ((n_k * (1.0f / 17.0f) - 1.0f) * r + 1.0f) * (1.0f + r);
        n_k *= r;
        if (b->snap) {                                         /* first chunk after init / idle */
            inc = n_inc; f_b0 = n_b0; k = n_k; g2 = n_g2; a = n_a;
            d_inc = d_b0 = d_k = d_g2 = d_a = 0.0f;
            b->snap = 0;
        } else {                                               /* per-sample linear interpolation */
            rm = recip[m];
            d_inc = (n_inc - inc) * rm;
            d_b0 = (n_b0 - f_b0) * rm;
            d_k = (n_k - k) * rm;
            d_g2 = (n_g2 - g2) * rm;
            d_a = (n_a - a) * rm;
            if (b->trig) {               /* a trigger jumps pitch and amp in Open303: no ramp */
                inc = n_inc;
                a = n_a;
                d_inc = d_a = 0.0f;
            }
        }
        b->trig = 0;

        for (j = 0; j < m; j++) {
            float s, t, ym, amp;
            inc += d_inc;
            f_b0 += d_b0;
            k += d_k;
            g2 += d_g2;
            a += d_a;

            /* amp: de-clicked (200 Hz Butterworth), per sample */
            ym = b->dc_b0 * (a + b->dc.x2) + b->dc_b1 * b->dc.x1 + b->dc_a1 * b->dc.y1 + b->dc_a2 * b->dc.y2;
            b->dc.x2 = b->dc.x1;
            b->dc.x1 = a;
            b->dc.y2 = b->dc.y1;
            b->dc.y1 = ym;
            amp = ym * out_gain;

            /* oversampled: oscillator -> highpass -> TeeBeeFilter */
            {
                int q;
                for (q = 0; q < nos; q++) {
                    if (!square) {
                        t = phase + 0.5f;                       /* Saw303: rises -1..1, jump at p = .5 */
                        if (t >= 1.0f)
                            t -= 1.0f;
                        s = -(t + t - 1.0f - blep(t, inc));     /* Open303 inverts the osc */
                    } else {
                        /* Square303 (shaped saw, minus its mean, polyBLEP on the hard rising edge
                         * at p = 0), times 0.5 as BlendOscillator scales it, inverted */
                        s = 0.5f * (fm_tanhf(69.98419960f * (phase + phase - 1.0f) + 4.37f)
                                    + sq_dc - sq_h2 * blep(phase, inc));
                    }
                    phase += inc;
                    if (phase >= 1.0f)
                        phase -= 1.0f;

                    ym = hp1_b0 * (s - hp1x) + hp1_a1 * hp1y;
                    hp1x = s;
                    hp1y = ym;

                    t = k * y4;                                 /* feedback through the highpass */
                    fby = fb_b0 * (t - fbx) + fb_a1 * fby;
                    fbx = t;
                    t = ym - fby;
                    y1 += 2.0f * f_b0 * (t - y1 + y2);
                    y2 += f_b0 * (y1 - 2.0f * y2 + y3);
                    y3 += f_b0 * (y2 - 2.0f * y3 + y4);
                    y4 += f_b0 * (y3 - 2.0f * y4);
                    os[q] = g2 * y4;
                }
            }
#if BASS303_OS == 1
            s = os[0];
#elif BASS303_OS == 2
            s = lite ? os[0] : decim_hb1(b->hb1, os[0], os[1]);
#else
            s = lite ? os[0] : decim_hb1(b->hb1, decim_hb2(b->hb2, os[0], os[1]), decim_hb2(b->hb2, os[2], os[3]));
#endif

            /* allpass 14 Hz, highpass 24 Hz, notch 7.5 Hz at the base rate */
            t = b->ap_b0 * (s - b->ap_y1) + b->ap_x1;
            b->ap_x1 = s;
            b->ap_y1 = t;
            s = b->hp2_b0 * (t - b->hp2_x1) + b->hp2_a1 * b->hp2_y1;
            b->hp2_x1 = t;
            b->hp2_y1 = s;
            t = b->nt_b0 * (s + b->nt.x2) + b->nt_b1 * b->nt.x1 + b->nt_a1 * b->nt.y1 + b->nt_a2 * b->nt.y2;
            b->nt.x2 = b->nt.x1;
            b->nt.x1 = s;
            b->nt.y2 = b->nt.y1;
            b->nt.y1 = t;
            out[i + j] = t * amp;
        }
    }

    b->phase = phase;
    b->c_inc = inc;
    b->c_b0 = f_b0;
    b->c_k = k;
    b->c_g2 = g2;
    b->c_a = a;
    b->env_y = fm_flush(b->env_y);
    b->rc1_y = fm_flush(b->rc1_y);
    b->rc2_y = fm_flush(b->rc2_y);
    b->amp_y = fm_flush(b->amp_y);
    b->hp1_x1 = hp1x;
    b->hp1_y1 = hp1y;
    b->f_y1 = y1;
    b->f_y2 = y2;
    b->f_y3 = y3;
    b->f_y4 = y4;
    b->fb_x1 = fbx;
    b->fb_y1 = fby;
}

void bass303_render(bass303_t *b, float *out, int n)
{
    int i;
    if (b->idle) {
        for (i = 0; i < n; i++)
            out[i] = 0.0f;
        idle_advance(b, n);
        return;
    }
    run(b, out, n);

    if (drive_on(b))
        drive_block(b, out, n);

    /* states that decay toward zero while the engine still runs (the idle hold-off, where the
     * amp is exactly 0): flushed once per block, before they can reach denormals */
    b->dc.x1 = fm_flush(b->dc.x1);
    b->dc.x2 = fm_flush(b->dc.x2);
    b->dc.y1 = fm_flush(b->dc.y1);
    b->dc.y2 = fm_flush(b->dc.y2);
    b->s_pre.z1 = fm_flush(b->s_pre.z1);
    b->s_pre.z2 = fm_flush(b->s_pre.z2);
    b->s_post.z1 = fm_flush(b->s_post.z1);
    b->s_post.z2 = fm_flush(b->s_post.z2);
    b->up_lp.z1 = fm_flush(b->up_lp.z1);
    b->up_lp.z2 = fm_flush(b->up_lp.z2);
    b->down_lp.z1 = fm_flush(b->down_lp.z1);
    b->down_lp.z2 = fm_flush(b->down_lp.z2);
    b->r_z[0] = fm_flush(b->r_z[0]);
    b->r_z[1] = fm_flush(b->r_z[1]);
    b->r_z[2] = fm_flush(b->r_z[2]);
    b->r_corr_z = fm_flush(b->r_corr_z);
    b->r_tone_z = fm_flush(b->r_tone_z);
    b->dcb_x1 = fm_flush(b->dcb_x1);
    b->dcb_y1 = fm_flush(b->dcb_y1);

    /* idle: gate off, amp envelope and de-clicker below -120 dB and the drive's DC blocker
     * settled, for BASS303_IDLE_HOLD samples in a row (see bass303.h) */
    if (!b->gate && b->amp_y < 1e-6f && fm_fabsf(b->dc.y1) < 1e-6f && fm_fabsf(b->dc.y2) < 1e-6f
        && fm_fabsf(b->dcb_y1) < 1e-6f) {
        b->quiet += n;
        if (b->quiet >= BASS303_IDLE_HOLD) {
            clear_audio_state(b);
            b->idle = 1;
        }
    } else {
        b->quiet = 0;
    }
}
