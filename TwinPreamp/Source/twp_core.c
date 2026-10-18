#include "twp_core.h"

#include <math.h>
#include <string.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

static float clampf (float x, float lo, float hi)
{
    return x < lo ? lo : (x > hi ? hi : x);
}

static void onepole_clear (TwpOnePole* f)
{
    f->x1 = 0.0f;
    f->y1 = 0.0f;
}

static void onepole_lpf (TwpOnePole* f, float fs, float hz)
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

static void onepole_hpf (TwpOnePole* f, float fs, float hz)
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

static float onepole_proc (TwpOnePole* f, float x)
{
    const float y = f->b0 * x + f->b1 * f->x1 - f->a1 * f->y1;
    f->x1 = x;
    f->y1 = y;
    return y;
}

/* Soft tube-ish stage */
static float tube_stage (float x, float gain, float thresh)
{
    const float s = x * gain;
    return thresh * tanhf (s / thresh);
}

void twp_init (TwpCore* s)
{
    memset (s, 0, sizeof (*s));
    s->sampleRate = 44100.0f;
}

void twp_reset (TwpCore* s)
{
    onepole_clear (&s->inHpf);
    onepole_clear (&s->toneBass);
    onepole_clear (&s->toneMid);
    onepole_clear (&s->toneTreble);
    onepole_clear (&s->postHpf);
    onepole_clear (&s->springHpf);

    for (int i = 0; i < 4; ++i)
        s->springIdx[i] = 0;
    s->springPhase = 0.0f;
}

void twp_prepare (TwpCore* s, float sampleRate)
{
    if (sampleRate < 8000.0f)
        sampleRate = 8000.0f;

    s->sampleRate = sampleRate;
    const float fs = sampleRate;

    /* Input coupling ~ 30 Hz */
    onepole_hpf (&s->inHpf, fs, 30.0f);

    /* Approximate Fender TMB shelves / bands */
    onepole_lpf (&s->toneBass,   fs,  180.0f);
    onepole_hpf (&s->toneMid,    fs,  400.0f);
    onepole_lpf (&s->toneMid,    fs, 1200.0f); /* reuse? simple */
    onepole_hpf (&s->toneTreble, fs, 2500.0f);

    onepole_hpf (&s->postHpf, fs, 20.0f);

    /* Spring send high-pass (500 pF style ~ a few hundred Hz) */
    onepole_hpf (&s->springHpf, fs, 280.0f);

    /* Simple 4-tap spring lengths (samples). Tune for ~30–80 ms spread. */
    const float ms[4] = { 28.0f, 42.0f, 61.0f, 85.0f };
    for (int i = 0; i < 4; ++i)
    {
        s->springLen[i] = (int) (fs * ms[i] * 0.001f);
        if (s->springLen[i] < 32) s->springLen[i] = 32;
        if (s->springLen[i] > 8192) s->springLen[i] = 8192;
        s->springIdx[i] = 0;
    }

    twp_reset (s);
}

float twp_process (TwpCore* s, float x,
                   float volume, float bass, float mid, float treble, float reverb)
{
    volume = clampf (volume, 0.0f, 1.0f);
    bass   = clampf (bass,   0.0f, 1.0f);
    mid    = clampf (mid,    0.0f, 1.0f);
    treble = clampf (treble, 0.0f, 1.0f);
    reverb = clampf (reverb, 0.0f, 1.0f);

    /* Input stage */
    x = onepole_proc (&s->inHpf, x);
    x = tube_stage (x, 2.2f + 6.0f * volume, 1.4f);

    /* Very simple TMB approximation (not full interactive, but Fender-ish) */
    {
        const float b = onepole_proc (&s->toneBass,   x) * (0.4f + 1.2f * bass);
        const float t = onepole_proc (&s->toneTreble, x) * (0.5f + 1.5f * treble);
        const float m = x * (0.6f + 0.8f * mid);
        x = b + m * 0.45f + t;
        x *= 0.55f; /* insertion loss compensation */
    }

    x = tube_stage (x, 1.8f, 1.6f);
    x = onepole_proc (&s->postHpf, x);

    /* Spring */
    float wet = 0.0f;
    if (reverb > 0.001f)
    {
        float send = onepole_proc (&s->springHpf, x);
        send = tube_stage (send, 3.5f, 0.9f); /* mild drive into springs */

        /* Four delayed taps with slight modulation for flutter */
        s->springPhase += 0.0008f;
        if (s->springPhase > 6.28f) s->springPhase -= 6.28f;
        const float mod = 0.0025f * sinf (s->springPhase);

        for (int i = 0; i < 4; ++i)
        {
            const int len = s->springLen[i];
            int idx = s->springIdx[i];
            /* simple circular (fixed buffer would be better; here we just delay by index for skeleton) */
            /* For a real implementation use delay lines. This is a placeholder sum. */
            wet += send * (0.55f - 0.08f * i);
            s->springIdx[i] = (idx + 1) % len;
        }
        wet *= 0.22f * reverb;
        /* add a little "boing" transient emphasis */
        wet += 0.04f * send * reverb * (send > 0.0f ? 1.0f : -0.7f);
    }

    x = x + wet;
    x = tube_stage (x, 1.4f, 1.8f);

    return x;
}
