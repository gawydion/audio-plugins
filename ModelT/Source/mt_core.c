#include "mt_core.h"

#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static float clampf (float x, float lo, float hi)
{
    return x < lo ? lo : (x > hi ? hi : x);
}

static void onepole_clear (MtOnePole* f)
{
    f->x1 = 0.0f;
    f->y1 = 0.0f;
}

static void onepole_lpf (MtOnePole* f, float fs, float hz)
{
    const float ny = 0.49f * fs;
    if (hz > ny) hz = ny;
    if (hz < 1.0f) hz = 1.0f;
    const float g = tanf ((float) M_PI * hz / fs);
    const float d = 1.0f / (1.0f + g);
    f->b0 = g * d;
    f->b1 = f->b0;
    f->a1 = (g - 1.0f) * d;
}

static void onepole_hpf (MtOnePole* f, float fs, float hz)
{
    const float ny = 0.49f * fs;
    if (hz > ny) hz = ny;
    if (hz < 0.1f) hz = 0.1f;
    const float g = tanf ((float) M_PI * hz / fs);
    const float d = 1.0f / (1.0f + g);
    f->b0 = d;
    f->b1 = -d;
    f->a1 = (g - 1.0f) * d;
}

static float onepole_proc (MtOnePole* f, float x)
{
    const float y = f->b0 * x + f->b1 * f->x1 - f->a1 * f->y1;
    f->x1 = x;
    f->y1 = y;
    return y;
}

static void biquad_clear (MtBiquad* f)
{
    f->x1 = f->x2 = 0.0f;
    f->y1 = f->y2 = 0.0f;
}

/* Peaking EQ. gainDb is the peak height. */
static void biquad_peak (MtBiquad* f, float fs, float hz, float q, float gainDb)
{
    const float ny = 0.45f * fs;
    if (hz > ny) hz = ny;
    if (hz < 20.0f) hz = 20.0f;
    if (q < 0.2f) q = 0.2f;

    const float A  = powf (10.0f, gainDb / 40.0f);
    const float w0 = 2.0f * (float) M_PI * hz / fs;
    const float cw = cosf (w0);
    const float sw = sinf (w0);
    const float alpha = sw / (2.0f * q);

    const float b0 = 1.0f + alpha * A;
    const float b1 = -2.0f * cw;
    const float b2 = 1.0f - alpha * A;
    const float a0 = 1.0f + alpha / A;
    const float a1 = -2.0f * cw;
    const float a2 = 1.0f - alpha / A;
    const float inv = 1.0f / a0;

    f->b0 = b0 * inv;
    f->b1 = b1 * inv;
    f->b2 = b2 * inv;
    f->a1 = a1 * inv;
    f->a2 = a2 * inv;
}

static float biquad_proc (MtBiquad* f, float x)
{
    const float y = f->b0 * x + f->b1 * f->x1 + f->b2 * f->x2
                  - f->a1 * f->y1 - f->a2 * f->y2;
    f->x2 = f->x1;
    f->x1 = x;
    f->y2 = f->y1;
    f->y1 = y;
    return y;
}

/* 12AX7-ish soft clip. Asymmetric, high ceiling so the preamp
   stays mostly linear until both volumes are up. */
static float triode (float x, float drive, float ceil)
{
    const float s = x * drive;
    const float k = s >= 0.0f ? 1.00f : 0.86f;
    return ceil * tanhf (s * k / ceil);
}

/* Ultralinear 6550: softer than pentode, mild asymmetry,
   thick rather than fizzy when it finally clips. */
static float power_amp (float x, float thresh)
{
    const float t = thresh > 0.15f ? thresh : 0.15f;
    const float s = x / t;
    const float k = s >= 0.0f ? 0.92f : 0.78f;
    return t * tanhf (s * k);
}

void mt_init (MtCore* s)
{
    memset (s, 0, sizeof (*s));
    s->sampleRate = 44100.0f;
}

void mt_reset (MtCore* s)
{
    onepole_clear (&s->inHpf);
    onepole_clear (&s->pickupLpf);
    onepole_clear (&s->brightMiller);
    onepole_clear (&s->normalMiller);
    onepole_clear (&s->brightCap);
    onepole_clear (&s->mixHpf);
    onepole_clear (&s->stackBass);
    onepole_clear (&s->stackTreble);
    onepole_clear (&s->midDip);
    onepole_clear (&s->presenceHp);
    onepole_clear (&s->piLpf);
    biquad_clear (&s->cabRes);
    onepole_clear (&s->cabLpf);
    onepole_clear (&s->outHpf);
    s->sag = 0.0f;
}

void mt_prepare (MtCore* s, float sampleRate)
{
    if (sampleRate < 8000.0f)
        sampleRate = 8000.0f;

    s->sampleRate = sampleRate;
    const float fs = sampleRate;

    /* Guitar into 1M / 68k grid stoppers. */
    onepole_hpf (&s->inHpf, fs, 18.0f);
    onepole_lpf (&s->pickupLpf, fs, 8500.0f);

    /* Miller on the first triodes. Bright is open, Normal is darker. */
    onepole_lpf (&s->brightMiller, fs, 9000.0f);
    onepole_lpf (&s->normalMiller, fs, 5200.0f);

    /* Bright cap across the volume pot, ~470 pF into the pot. */
    onepole_hpf (&s->brightCap, fs, 2800.0f);

    /* Coupling into the mixer triode. */
    onepole_hpf (&s->mixHpf, fs, 28.0f);

    /* Bassman-style stack, approximated:
       bass shelf ~80 Hz, treble shelf ~3 kHz, fixed mid dip ~600 Hz. */
    onepole_lpf (&s->stackBass, fs, 180.0f);
    onepole_hpf (&s->stackTreble, fs, 2200.0f);
    onepole_lpf (&s->midDip, fs, 700.0f);

    /* Presence is a high-pass in the NFB loop. */
    onepole_hpf (&s->presenceHp, fs, 1800.0f);
    onepole_lpf (&s->piLpf, fs, 12000.0f);

    /* Closed-back 4x12: speaker resonance and cone rolloff. */
    biquad_peak (&s->cabRes, fs, 92.0f, 0.85f, 4.5f);
    onepole_lpf (&s->cabLpf, fs, 4200.0f);
    onepole_hpf (&s->outHpf, fs, 32.0f);

    mt_reset (s);
}

float mt_process (MtCore* s, float x,
                  float bright, float normal,
                  float bass, float treble, float presence,
                  float master, int jump)
{
    bright   = clampf (bright,   0.0f, 1.0f);
    normal   = clampf (normal,   0.0f, 1.0f);
    bass     = clampf (bass,     0.0f, 1.0f);
    treble   = clampf (treble,   0.0f, 1.0f);
    presence = clampf (presence, 0.0f, 1.0f);
    master   = clampf (master,   0.0f, 1.0f);

    x = onepole_proc (&s->inHpf, x);
    x = onepole_proc (&s->pickupLpf, x);

    /* ---- Bright channel ----
       Audio taper on the volume. Bright cap passes highs when the
       pot is down, same idea as a Bassman / Plexi. */
    float b = triode (x, 7.5f, 1.35f);
    b = onepole_proc (&s->brightMiller, b);
    const float bVol = bright * (0.25f + 0.75f * bright);
    const float brightHf = onepole_proc (&s->brightCap, b);
    b = b * bVol + brightHf * (1.0f - bVol) * 0.35f;

    /* ---- Normal channel ----
       Darker first triode. Only fed when the fifth hole is used
       (jump), or when the player has plugged Normal alone — we
       treat jump-off as Bright only, which is the common single-jack
       use. Dial Normal to blend once Jump is on. */
    float n = 0.0f;
    if (jump)
    {
        n = triode (x, 6.2f, 1.35f);
        n = onepole_proc (&s->normalMiller, n);
        const float nVol = normal * (0.25f + 0.75f * normal);
        n *= nVol;
    }

    /* Mixer triode. Stays linear until both channels are hot. */
    float m = onepole_proc (&s->mixHpf, b + n);
    m = triode (m, 3.4f, 1.6f);

    /* Passive stack. Noon is a mild scoop, not a flat line. */
    const float low  = onepole_proc (&s->stackBass, m);
    const float high = onepole_proc (&s->stackTreble, m);
    const float mid  = onepole_proc (&s->midDip, m);
    const float body = m * 0.55f + low * (0.15f + 0.85f * bass) * 0.55f;
    const float top  = high * (0.20f + 0.80f * treble);
    m = body + top - mid * 0.18f;

    /* Phase inverter + presence. Presence lifts HF by reducing
       the feedback on the top end. */
    const float pi = onepole_proc (&s->piLpf, m);
    const float hf = onepole_proc (&s->presenceHp, pi);
    const float driven = pi + hf * presence * 0.85f;

    /* Master is the headroom control. Low Master sits well under
       the 6550 clip; high Master is the thick power-tube grind.
       Ultralinear + stiff SS supply: high threshold, shallow sag. */
    const float drive = 0.35f + master * master * 4.6f;
    float p = driven * drive;

    const float env = fabsf (p);
    const float atk = env > s->sag ? 0.0025f : 0.00035f;
    s->sag += (env - s->sag) * atk;
    const float thresh = 1.15f - s->sag * 0.12f;

    p = power_amp (p, thresh);

    /* Cab and the fixed output pad (~-12 dB), same idea as the
       fuzz plugins so a guitar track does not slam the next plug. */
    p = biquad_proc (&s->cabRes, p);
    p = onepole_proc (&s->cabLpf, p);
    p = onepole_proc (&s->outHpf, p);

    return p * 0.2512f;
}
