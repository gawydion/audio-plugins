#include "rbm_core.h"

#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static float clampf (float x, float lo, float hi)
{
    return x < lo ? lo : (x > hi ? hi : x);
}

static void onepole_clear (RbmOnePole* f)
{
    f->x1 = 0.0f;
    f->y1 = 0.0f;
}

static void onepole_lpf (RbmOnePole* f, float fs, float hz)
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

static void onepole_hpf (RbmOnePole* f, float fs, float hz)
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

static float onepole_proc (RbmOnePole* f, float x)
{
    const float y = f->b0 * x + f->b1 * f->x1 - f->a1 * f->y1;
    f->x1 = x;
    f->y1 = y;
    return y;
}

/* Silicon diode pair, slight +/− asymmetry like KD521 / 1N914. */
static float diode_clip (float x, float thresh)
{
    const float t = thresh > 1.0e-6f ? thresh : 1.0e-6f;
    const float s = x / t;
    const float k = s >= 0.0f ? 1.00f : 0.92f;
    return t * tanhf (s * k);
}

void rbm_init (RbmCore* s)
{
    memset (s, 0, sizeof (*s));
    s->sampleRate = 44100.0f;
}

void rbm_reset (RbmCore* s)
{
    onepole_clear (&s->pickupLpf);
    onepole_clear (&s->inHpf);
    onepole_clear (&s->st1Lpf);
    onepole_clear (&s->st1Hpf);
    onepole_clear (&s->st2Lpf);
    onepole_clear (&s->st2Hpf);
    onepole_clear (&s->toneLp);
    onepole_clear (&s->toneHp);
    onepole_clear (&s->recLpf);
    onepole_clear (&s->outHpf);
}

void rbm_prepare (RbmCore* s, float sampleRate)
{
    if (sampleRate < 8000.0f)
        sampleRate = 8000.0f;

    s->sampleRate = sampleRate;
    const float fs = sampleRate;

    /* Guitar + 39 k input load. */
    onepole_lpf (&s->pickupLpf, fs, 7200.0f);

    /* C1 100 nF into ~39 k  →  ~41 Hz. */
    onepole_hpf (&s->inHpf, fs, 41.0f);

    /* 470 pF feedback across 12 k collector ≈ 28 kHz. */
    onepole_lpf (&s->st1Lpf, fs, 28000.0f);
    onepole_lpf (&s->st2Lpf, fs, 28000.0f);

    /* 47 nF clip cap: diodes see mids/highs, bass mostly bypasses. */
    onepole_hpf (&s->st1Hpf, fs, 220.0f);
    onepole_hpf (&s->st2Hpf, fs, 220.0f);

    /* Tone stack: 20 k + 3.9 nF  ≈ 2040 Hz LPF
                   22 k + 10 nF   ≈  723 Hz HPF */
    onepole_lpf (&s->toneLp, fs, 2040.0f);
    onepole_hpf (&s->toneHp, fs,  723.0f);

    /* Q4 collector 10 k + 470 pF. */
    onepole_lpf (&s->recLpf, fs, 24000.0f);

    /* C13 100 nF into the 100 k volume pot ≈ 16 Hz. */
    onepole_hpf (&s->outHpf, fs, 16.0f);

    rbm_reset (s);
}

float rbm_process (RbmCore* s, float x,
                   float sustain, float tone, float volume, float input)
{
    sustain = clampf (sustain, 0.0f, 1.0f);
    tone    = clampf (tone,    0.0f, 1.0f);
    volume  = clampf (volume,  0.0f, 1.0f);
    input   = clampf (input,   0.0f, 1.0f);

    /* Pickup / guitar volume. Mild audio taper, unity-ish at the
       default 70% so a DI guitar actually hits the clippers. */
    x *= input * (0.40f + 0.90f * input);
    x  = onepole_proc (&s->pickupLpf, x);
    x  = onepole_proc (&s->inHpf, x);

    /* Q1 — CE input amp. Rc 12 k / Re 390 ≈ 29, 470 k shunt FB
       knocks that down. High ceiling: this stage feeds the pot,
       it is not the main clipper. */
    x = diode_clip (x * 12.0f, 2.4f);

    /* Sustain 100 k between Q1 collector and Q2 base.
       Minimum is not zero — the real pot still leaks. Linear,
       not squared, or noon never reaches the diodes. */
    const float sus = 0.22f + 0.78f * sustain;

    /* Open-loop of a Russian clip stage: Rc 12 k / (Re 390 + re)
       ≈ 28. NYC uses 100–150 Ω emitters (~2× that). Diodes (KD521
       / 1N914) clamp the collector at ~0.6 V. */
    const float stageGain = 28.0f;

    /* ---- Q2 + D1/D2 ----
       47 nF clip cap: bass mostly misses the diodes (smooth/fat),
       mids and highs take the silicon. Small leftover-bass mix so
       it stays a muff, not an overdrive. */
    {
        const float drive   = x * sus * stageGain;
        const float hi      = onepole_proc (&s->st1Hpf, drive);
        const float clipped = diode_clip (hi, 0.60f);
        const float lo      = drive - hi;
        x = onepole_proc (&s->st1Lpf, clipped + 0.22f * lo);
    }

    /* ---- Q3 + D3/D4 ----
       Same stage again. Second smash is why a Muff sustains. */
    {
        const float drive   = x * stageGain;
        const float hi      = onepole_proc (&s->st2Hpf, drive);
        const float clipped = diode_clip (hi, 0.56f);
        const float lo      = drive - hi;
        x = onepole_proc (&s->st2Lpf, clipped + 0.18f * lo);
    }

    /* Passive BMP tone stack. CCW = bass, noon = mid scoop, CW =
       treble. Insertion loss is huge (~20 dB at noon) — Q4 exists
       to pay that back. */
    {
        const float lp = onepole_proc (&s->toneLp, x);
        const float hp = onepole_proc (&s->toneHp, x);
        x = (1.0f - tone) * lp + tone * hp;
        x *= 8.5f;
    }

    /* Q4 recovery. Rc 10 k / Re 2 k ≈ 5. Soft clip at the rail. */
    x = onepole_proc (&s->recLpf, diode_clip (x * 5.0f, 1.55f));
    x = onepole_proc (&s->outHpf, x);

    /* Volume 100 k, audio-ish taper, then a −12 dB pad.
       Same fuzz, just not slamming the next plugin. */
    x *= volume * (0.35f + 0.95f * volume) * 0.2512f;
    return x;
}
