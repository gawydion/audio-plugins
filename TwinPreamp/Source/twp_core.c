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

    for (int i = 0; i < TWP_SPRING_TAPS; ++i)
    {
        memset (s->springBuf[i], 0, sizeof (s->springBuf[i]));
        s->springWrite[i] = 0;
    }
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
    onepole_lpf (&s->toneMid,    fs, 1200.0f);
    onepole_hpf (&s->toneTreble, fs, 2500.0f);

    onepole_hpf (&s->postHpf, fs, 20.0f);

    /* Spring send high-pass (500 pF style ~ a few hundred Hz) */
    onepole_hpf (&s->springHpf, fs, 280.0f);

    /* Multi-tap spring lengths (samples). ~30–85 ms spread for drip. */
    const float ms[TWP_SPRING_TAPS] = { 32.0f, 48.0f, 67.0f, 91.0f };
    for (int i = 0; i < TWP_SPRING_TAPS; ++i)
    {
        s->springLen[i] = (int) (fs * ms[i] * 0.001f);
        if (s->springLen[i] < 64) s->springLen[i] = 64;
        if (s->springLen[i] > TWP_SPRING_MAX - 1) s->springLen[i] = TWP_SPRING_MAX - 1;
        s->springWrite[i] = 0;
    }

    twp_reset (s);
}

/* read_delay: write is the next write position (sample about to be overwritten is delay == len) */
static float read_delay (const float* buf, int len, int write, float delaySamples)
{
    float d = delaySamples;
    if (d < 1.0f) d = 1.0f;
    if (d > (float) len) d = (float) len;

    int id = (int) d;
    float frac = d - (float) id;

    int i0 = write - id;
    while (i0 < 0) i0 += len;
    i0 %= len;
    int i1 = i0 - 1;
    if (i1 < 0) i1 += len;

    return buf[i0] * (1.0f - frac) + buf[i1] * frac;
}

float twp_process (TwpCore* s, float x,
                   float volume, float bass, float mid, float treble, float reverb)
{
    volume = clampf (volume, 0.0f, 1.0f);
    bass   = clampf (bass,   0.0f, 1.0f);
    mid    = clampf (mid,    0.0f, 1.0f);
    treble = clampf (treble, 0.0f, 1.0f);
    reverb = clampf (reverb, 0.0f, 1.0f);

    /* Input stage — higher base gain for usable level */
    x = onepole_proc (&s->inHpf, x);
    x = tube_stage (x, 3.5f + 8.0f * volume, 1.6f);

    /* Simple TMB approximation */
    {
        const float b = onepole_proc (&s->toneBass,   x) * (0.5f + 1.4f * bass);
        const float t = onepole_proc (&s->toneTreble, x) * (0.6f + 1.6f * treble);
        const float m = x * (0.7f + 0.9f * mid);
        x = b + m * 0.5f + t;
        x *= 0.7f; /* milder insertion-loss compensation */
    }

    x = tube_stage (x, 2.2f, 1.8f);
    x = onepole_proc (&s->postHpf, x);

    /* Spring reverb */
    float wet = 0.0f;
    if (reverb > 0.001f)
    {
        float send = onepole_proc (&s->springHpf, x);
        send = tube_stage (send, 2.8f, 1.0f);

        /* Slow LFO for mild flutter */
        s->springPhase += 0.00035f;
        if (s->springPhase > 6.2831853f) s->springPhase -= 6.2831853f;
        const float mod = 3.5f * sinf (s->springPhase); /* ~±3.5 samples */

        const float weights[TWP_SPRING_TAPS] = { 0.55f, 0.40f, 0.28f, 0.18f };

        for (int i = 0; i < TWP_SPRING_TAPS; ++i)
        {
            float* buf = s->springBuf[i];
            const int len = s->springLen[i];
            int wr = s->springWrite[i];

            /* Read delayed sample first (wr is next write position) */
            const float dly = (float) len + mod * (0.4f + 0.15f * (float) i);
            wet += read_delay (buf, len, wr, dly) * weights[i];

            /* Then write current send */
            buf[wr] = send;

            wr = (wr + 1) % len;
            s->springWrite[i] = wr;
        }

        wet *= 0.65f * reverb;
    }

    x = x + wet;
    x = tube_stage (x, 1.6f, 2.0f);

    /* Makeup so default settings are usable in a DAW (was far too quiet) */
    x *= 5.5f;

    return x;
}
