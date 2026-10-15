#pragma once

/* Model T style high-headroom amp — audio inner loop.
 *
 * Behavioral model of the early-70s Rev A: Bassman-derived dual
 * Bright / Normal preamp, passive tone stack, long-tail phase
 * inverter with presence in the feedback loop, quad-6550
 * ultralinear power stage, solid-state supply (stiff, little sag).
 *
 * Jump is the fifth-hole center jack: one input feeds both channels.
 * Link outputs are not modelled.
 *
 * Write DSP here. JUCE only wraps AU / VST3 / the editor.
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    float b0, b1, a1;
    float x1, y1;
} MtOnePole;

typedef struct
{
    float b0, b1, b2, a1, a2;
    float x1, x2, y1, y2;
} MtBiquad;

typedef struct
{
    float sampleRate;

    MtOnePole inHpf;
    MtOnePole pickupLpf;

    MtOnePole brightMiller;
    MtOnePole normalMiller;
    MtOnePole brightCap;       /* HF across the Bright volume pot */

    MtOnePole mixHpf;
    MtOnePole stackBass;
    MtOnePole stackTreble;
    MtOnePole midDip;

    MtOnePole presenceHp;      /* NFB shelf: more HF = less feedback */
    MtOnePole piLpf;

    MtBiquad cabRes;
    MtOnePole cabLpf;
    MtOnePole outHpf;

    float sag;
} MtCore;

void mt_init    (MtCore* s);
void mt_prepare (MtCore* s, float sampleRate);
void mt_reset   (MtCore* s);
float mt_process (MtCore* s, float x,
                  float bright, float normal,
                  float bass, float treble, float presence,
                  float master, int jump);

#ifdef __cplusplus
}
#endif
