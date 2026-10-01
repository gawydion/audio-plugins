#pragma once

/* Green Russian / Sovtek Big Muff — audio inner loop.
 *
 * Circuit reference (Tonepad / PedalPCB / Kit Rae V7C):
 *   Q1–Q4 NPN  KT3102E / 2N5088 / BC549C
 *   D1–D4      KD521 / 1N914 silicon
 *   Emitters   390 Ω (lower gain than NYC 100–150 Ω)
 *   Clip coup. 47 nF  (fat, smooth bottom vs 1 µF NYC)
 *   Feedback   470 pF on each gain stage
 *   Tone       20 k + 3.9 nF  ||  22 k + 10 nF, 100 k lin pot
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
} RbmOnePole;

typedef struct
{
    float sampleRate;

    RbmOnePole pickupLpf;
    RbmOnePole inHpf;
    RbmOnePole st1Lpf;     /* 470 pF Miller / feedback */
    RbmOnePole st1Hpf;     /* 47 nF interstage         */
    RbmOnePole st2Lpf;
    RbmOnePole st2Hpf;
    RbmOnePole toneLp;     /* 20 k + 3.9 nF            */
    RbmOnePole toneHp;     /* 22 k + 10 nF             */
    RbmOnePole recLpf;
    RbmOnePole outHpf;
} RbmCore;

void rbm_init     (RbmCore* s);
void rbm_prepare  (RbmCore* s, float sampleRate);
void rbm_reset    (RbmCore* s);
float rbm_process (RbmCore* s, float x, float sustain, float tone, float volume, float input);

#ifdef __cplusplus
}
#endif
