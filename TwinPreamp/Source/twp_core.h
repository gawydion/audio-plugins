#pragma once

/* Twin Preamp — AB763 Vibrato-channel style preamp + spring reverb.
 *
 * Behavioral model of:
 *   - 12AX7 input / second stage
 *   - Classic Fender TMB tone stack (100k slope, 250pF / 0.1uF / mid)
 *   - Spring send (high-pass + mild drive) → algorithmic spring
 *   - Mix back + light recovery
 *
 * High-headroom clean Fender platform with iconic spring drip.
 * No power amp or cab.
 */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct
{
    float b0, b1, a1;
    float x1, y1;
} TwpOnePole;

typedef struct
{
    float sampleRate;

    /* Preamp filters */
    TwpOnePole inHpf;
    TwpOnePole toneBass;
    TwpOnePole toneMid;
    TwpOnePole toneTreble;
    TwpOnePole postHpf;

    /* Spring */
    TwpOnePole springHpf;
    float springDelays[4];   /* simple multi-tap spring */
    int   springIdx[4];
    int   springLen[4];
    float springMod;
    float springPhase;
} TwpCore;

void twp_init     (TwpCore* s);
void twp_prepare  (TwpCore* s, float sampleRate);
void twp_reset    (TwpCore* s);
float twp_process (TwpCore* s, float x,
                   float volume, float bass, float mid, float treble, float reverb);

#ifdef __cplusplus
}
#endif
