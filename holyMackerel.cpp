/*
 * Holy Mackerel v7.2.0 - "The Vactrol"
 * Expert Sleepers Disting NT
 *
 * Changelog:
 * v7.2.0 - Five behavioral fixes:
 *          1. Resonance: bass/volume restored at high res (static makeup gain + BP mix)
 *          2. Velocity floor raised 0.1→0.35 (reduces trigger voltage wobble)
 *          3. Dampening: no longer shortens decay! Acts as hand-on-drum:
 *             reduces brightness (85%), resonance (40%), VCA ceiling (75%)
 *          4. Material: Hard=metal (rings longer, bright), Soft=rubber (absorbs, dark)
 *          5. Hit Memory: accumulated hits slow decay up to 40% (warm vactrol)
 * v7.1.1 - Double-hit fix: Schmitt trigger detector with hysteresis,
 *          removed Smile Pass, fixed cutoff smoothing asymmetry.
 * v7.1.0 - Architecture fix: Single vactrol envelope model
 *   CRITICAL: Replaced dual-envelope (VCA + filter) with single vactrol state.
 *   In the real Buchla 292, ONE photoresistive element (Rf) controls both
 *   filter cutoff and VCA gain. The old architecture ran two independent
 *   envelopes with separate two-stage decays, each crossing its stage
 *   boundary at a different time = two audible transients ("double hit").
 *
 *   New model (based on Parker & D'Angelo, DAFX-13):
 *     - Single vactrol state with continuous level-dependent decay
 *       (higher level = faster decay = natural thwack-to-body contour)
 *     - Nonlinear transfer curves: filterGate=pow(state,exp), vcaGate=sqrt(state)
 *     - No stages, no crossfade, no double hit
 *     - Material modes shape transfer curves instead of separate decay rates
 *   Also fixed: nullptr terminators on enum string arrays (memory safety)
 *
 * v7.0.3 - Double-hit fix (superseded by v7.1.0 architectural rewrite) — the "double hit" from a single trigger.
 *   Real vactrols don't have discrete stages — phosphor decay is continuous.
 *   → Both VCA and filter envelopes now use smoothstep crossfade over ±0.08
 *     around the stage threshold, creating a smooth S-curve transition
 *   → Eliminates phantom second hit while preserving two-stage character
 *
 * v7.0.2 - Graphics crash fix (credit: Thornside PR #1):
 * - DRAW CRASH FIX: Hit animation radii grew up to 57px from center (175,38)
 *   on a 256×64 screen. Max safe Y radius is 25px. NT_drawShapeI writes
 *   directly into NT_screen[128*64] - out-of-bounds writes corrupt memory
 *   on M7, triggering hard fault. Audio DMA repeats last buffer → beeeeeep.
 *   → Derived maxYRadius from hitCenterY and screen height (not hardcoded)
 *   → Capped burstRadius, glowR, ringR, boundaryR to maxYRadius
 *   → Gate percentage text only drawn when textY <= 63
 * - COLOR CLAMP: Ray line color could reach 16 with Hit Memory ON
 *   (gate reaches 1.2), exceeding 4-bit grayscale 0-15 range
 *   → Added clampColor() wrapping all computed colors
 * - HITPHASE CAP: Prevented unbounded float growth in long sessions
 *   without triggers (animation invisible past ~25, capped at 100)
 *
 * v7.0.1 - Crash fix + sonic improvements:
 * - CRASH FIX: Filter state accumulation during rapid retriggers caused
 *   high-pitch lockup after ~8 triggers. Fixed via:
 *   → Replaced tanf() with fast_tanh() for SVF coefficient (cheaper on M7,
 *     naturally bounded, prevents extreme values near Nyquist)
 *   → Hard energy limit on filter state variables (s1, s2 clamped to ±4.0)
 *   → Filter state dampening on retrigger (s1/s2 *= 0.5)
 *   → NaN/inf protection on filter state and final output
 * - INSTANT ATTACK: Envelope now snaps to target on trigger instead of
 *   4ms slew. The "click" IS the gate opening, not a separate artifact.
 *   This fixes both the double-hit perception and the click/body separation.
 * - TRIGGER LOCKOUT: Increased from 3ms to 5ms for better double-hit rejection
 * - UI: Bar graphs raised 4px for label clearance, mode indicators dropped
 *   4px to avoid parameter bar cutoff at top of screen
 *
 * v7.0 - The best of both worlds:
 * - v5.7's proven "Smile Pass" filter (QPAS-inspired body preservation)
 * - v5.7's two-stage velocity-shaped decay (thwack + body)
 * - v5.7's sophisticated FX processor (per-effect state, scaled amount curve)
 * - v5.7's resonance range (Q 0.5-25)
 * - v5.7's correct parameter defaults and greying logic
 * - Cleaned up CV input macros from v6.0.2
 * - All 9 reported bugs from v6.0.2 fixed
 *
 * BUG FIXES (v6.0.2 regressions):
 * 1. Trigger min restored to 0 (was 1, caused immediate firing)
 * 2. Trigger default restored to Input 3 (was 2)
 * 3. Right Input default restored to 2 (was 3)
 * 4. Output defaults correct (1 and 2)
 * 5. Page names restored: "Holy Mackerel", "CV Control", "Routing"
 * 6. Greying logic restored (mono hides Right I/O, Clean hides FX Amount, etc)
 * 7. Full I/O names restored ("Left Input" not "Left In")
 * 8. FX strings: "Clean" (correct per original design)
 * 9. Resonance Q restored to 25 (was capped at 15)
 * + Gain max restored to 106 (was 200)
 * + Decay default restored to 50% (was 40%)
 * + FX Amount default restored to 0 (was 50)
 * + Stereo default restored to ON (was OFF)
 * + Threshold restored to millivolts 10-500 (was scaling10 1-50)
 * + Env Output default restored to 0 (was 15)
 * + Env output now respects Env Follower on/off switch
 *
 * ARCHITECTURE:
 * - Two-stage decay: fast "thwack" then slow "body" tail
 * - Velocity shapes stage threshold and decay speed
 * - "Smile Pass" filter: dynamic bass boost + sparkle as filter closes
 * - Per-effect FX state: tube grid blocking, screamer HP/LP, grit feedback
 * - Scaled FX amount curve: subtle → transitional → full character
 *
 * GUID: 0x486D6163 ('Hmac')
 */

#include <math.h>
#include <string.h>
#include <stdio.h>
#include <new>
#include <distingnt/api.h>

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(x) (sizeof(x) / sizeof((x)[0]))
#endif

static constexpr float PI = 3.14159265358979323846f;
static constexpr float TWO_PI = 6.28318530717958647692f;

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

static inline float clampf(float x, float lo, float hi) {
    return x < lo ? lo : (x > hi ? hi : x);
}

static inline float lerpf(float a, float b, float t) {
    return a + (b - a) * clampf(t, 0.0f, 1.0f);
}

static inline float fast_tanh(float x) {
    if (x < -3.0f) return -1.0f;
    if (x > 3.0f) return 1.0f;
    float x2 = x * x;
    return x * (27.0f + x2) / (27.0f + 9.0f * x2);
}

static inline float soft_saturate(float x, float knee) {
    float ax = fabsf(x);
    if (ax < knee) return x;
    float sign = x > 0.0f ? 1.0f : -1.0f;
    return sign * (knee + (1.0f - knee) * fast_tanh((ax - knee) / (1.0f - knee)));
}

// ============================================================================
// MATERIAL MODES
// ============================================================================

enum MaterialMode {
    MATERIAL_NATURAL = 0,
    MATERIAL_HARD = 1,
    MATERIAL_SOFT = 2
};

// Attack times in seconds
static const float kMaterialAttackTime[3] = {
    0.005f,     // Natural - 5ms (wood/organic)
    0.0005f,    // Hard - 0.5ms (metal/glass — instant, sharp transient)
    0.020f      // Soft - 20ms (rubber/felt — rounded, gentle)
};

// Decay multipliers — how long the material rings
// Hard materials RING LONGER (metal sustains), soft materials ABSORB (rubber deadens)
static const float kMaterialDecayMult[3] = {
    1.0f,       // Natural - baseline
    1.4f,       // Hard - metal/glass RINGS, longer sustain
    0.7f        // Soft - rubber/felt absorbs, shorter sustain
};

// Filter brightness (affects cutoff range)
// Hard = bright and shimmery, Soft = dark and thuddy
static const float kMaterialBrightness[3] = {
    1.0f,       // Natural - full range
    1.8f,       // Hard - bright, lots of upper harmonics
    0.35f       // Soft - dark, muted
};

// Vactrol level-dependent decay modulation
// LOW values = uniform/ringing decay (metal), HIGH = fast initial drop (thud)
static const float kMaterialVactrolMod[3] = {
    2.5f,       // Natural - balanced thwack and body
    1.2f,       // Hard - low modulation = even ring, shimmer sustain
    4.0f        // Soft - high modulation = fast thwack, quick deadening
};

// Filter transfer curve exponent (from single vactrol state)
// LOW = filter stays open (bright ringing), HIGH = filter closes fast (dark thud)
static const float kMaterialFilterExponent[3] = {
    1.8f,       // Natural - classic LPG pluck
    1.2f,       // Hard - filter stays open = bright metallic ring
    2.8f        // Soft - filter closes fast = dark, muted, felt-like
};

// ============================================================================
// FX MODES
// ============================================================================

enum FXMode {
    FX_CLEAN = 0,
    FX_TUBE = 1,
    FX_SCREAMER = 2,
    FX_GRIT = 3
};

// ============================================================================
// LPG FILTER — Buchla 292-inspired SVF
// 
// Clean SVF with resonance that preserves bass through bandpass mixing
// and makeup gain. NO level-dependent gain compensation (which caused
// the double-hit artifact in the "Smile Pass" era).
// ============================================================================

class BuchlaLPGFilter {
public:
    void setSampleRate(float sr) {
        sampleRate = sr;
        maxCutoff = sr * 0.45f;
    }
    
    void setResonance(float res) {
        resonance = clampf(res, 0.0f, 1.0f);
        
        // Q range: 0.5 (gentle) to 25 (self-oscillation territory)
        float q = 0.5f + res * res * 24.5f;
        k = 1.0f / q;
        
        // Resonance makeup gain — compensates for the SVF's LP output
        // losing broadband energy at high Q (energy concentrates at resonant peak).
        // This is STATIC per resonance setting, NOT level-dependent.
        // Safe because it doesn't change during decay (unlike the old Smile Pass).
        if (res > 0.15f) {
            float r = res - 0.15f;
            resMakeupGain = 1.0f + r * r * 4.0f;  // Aggressive makeup at high res
        } else {
            resMakeupGain = 1.0f;
        }
        
        // How much bandpass to mix in — the resonant peak itself
        // At high resonance the BP is where all the action is
        bpMixAmount = res * res * 0.5f;
    }
    
    void setBrightness(float bright) {
        brightness = clampf(bright, 0.1f, 2.0f);
    }
    
    float process(float input, float filterGate, float vcaGate) {
        // At very low resonance, blend toward bypass for clean tone
        float bypassMix = (resonance < 0.1f) ? (1.0f - resonance / 0.1f) * 0.5f : 0.0f;
        
        // Target cutoff follows filter gate
        float minCutoff = 20.0f;
        float targetCutoff = minCutoff + filterGate * brightness * (maxCutoff - minCutoff);
        targetCutoff = clampf(targetCutoff, minCutoff, maxCutoff);
        
        // Cutoff tracking — the vactrol model already produces a smooth,
        // continuous decay curve. We only need minimal smoothing to prevent
        // coefficient discontinuity at the SVF, NOT to shape the envelope.
        // 
        // CRITICAL: The old asymmetric smoother (0.4 open / 0.03 close)
        // caused the filter to STAY OPEN for ~4ms after the VCA started
        // dropping, creating a timbral plateau that the ear perceived as
        // a double-hit: first a volume drop, then a delayed brightness drop.
        //
        // Fix: use fast uniform tracking. The vactrol IS the smoother.
        float smoothCoef = 0.35f;  // Fast tracking both directions
        smoothedCutoff += (targetCutoff - smoothedCutoff) * smoothCoef;
        float cutoff = smoothedCutoff;
        
        // SVF coefficients - fast_tanh is cheaper on Cortex-M7 and naturally
        // bounded, preventing the extreme values tanf() produces near Nyquist
        // that cause filter blowup during rapid retriggering
        float w = TWO_PI * cutoff / sampleRate;
        float g = fast_tanh(w * 0.5f);
        g = clampf(g, 0.0001f, 0.9999f);
        
        // Two-pole SVF
        float hp = (input - (2.0f * k + g) * s1 - s2) / (1.0f + g * (g + 2.0f * k));
        float bp = g * hp + s1;
        float lp = g * bp + s2;
        
        // Update state with gentle saturation
        s1 = soft_saturate(g * hp + bp, 0.9f);
        s2 = soft_saturate(g * bp + lp, 0.9f);
        
        // Hard energy limit - prevents accumulation during rapid retriggers
        // that caused crash after ~8 triggers in v7.0
        s1 = clampf(s1, -4.0f, 4.0f);
        s2 = clampf(s2, -4.0f, 4.0f);
        
        // NaN protection - if anything goes sideways, reset cleanly
        if (!(s1 == s1) || !(s2 == s2)) { // NaN check
            s1 = s2 = 0.0f;
            smoothedCutoff = 20.0f;
        }
        
        // When gate is very low, gently decay filter state
        if (vcaGate < 0.01f) {
            s1 *= 0.995f;
            s2 *= 0.995f;
        }
        
        // =====================================================
        // LPG OUTPUT STAGE — Clean and authentic
        // =====================================================
        //
        // In the Buchla 292, the vactrol controls a simple LP filter
        // and VCA in tandem. There is NO bass boost, no sparkle injection,
        // no "smile pass" compensation. The bass that survives as the
        // filter closes does so naturally — because it's below cutoff.
        //
        // CRITICAL FIX (v7.2.0): The old "Smile Pass" had a dynamic bass
        // boost that INCREASED gain as the filter closed, creating a
        // second amplitude peak 3-5ms after trigger that the ear heard
        // as a double-hit. Removing it gives monotonic decay = single hit.
        
        // Base lowpass output + resonant character from bandpass
        // The BP mix adds the resonant peak back in — this is where the
        // "wild" resonance sound lives. Static mix amount per resonance
        // setting, NOT modulated by gate level (which caused double-hit).
        float filtered = lp + bp * bpMixAmount;
        
        // Apply VCA — this is the ONLY amplitude control
        float output = filtered * vcaGate;
        
        // Apply resonance makeup
        output *= resMakeupGain;
        
        // Blend toward clean bypass at very low resonance
        if (bypassMix > 0.0f) {
            float cleanPath = input * vcaGate;
            output = lerpf(output, cleanPath, bypassMix);
        }
        
        // Soft clip to prevent digital overs
        output = soft_saturate(output, 0.95f);
        
        lastBP = bp;
        return output;
    }
    
    float getBandpass() const { return lastBP; }
    void reset() { s1 = s2 = lastBP = 0.0f; smoothedCutoff = 20.0f; }
    
    // Partially dampen filter state on retrigger to prevent energy accumulation
    // from rapid repeated triggers causing high-pitch blowup
    void dampStateOnRetrigger() {
        s1 *= 0.5f;
        s2 *= 0.5f;
    }
    
private:
    float sampleRate = 48000.0f;
    float maxCutoff = 20000.0f;
    float brightness = 1.0f;
    float resonance = 0.0f;
    float k = 2.0f;
    float resMakeupGain = 1.0f;
    float bpMixAmount = 0.0f;
    float s1 = 0.0f, s2 = 0.0f;
    float smoothedCutoff = 20.0f;
    float lastBP = 0.0f;
};

// ============================================================================
// FX PROCESSOR - Per-effect state, scaled amount curve
// ============================================================================

class FXProcessor {
public:
    void setSampleRate(float sr) {
        sampleRate = sr;
        float w = TWO_PI * 720.0f / sr;
        screamerHPCoef = 1.0f - expf(-w);
        screamerLPCoef = 1.0f - expf(-w);
        float gritW = TWO_PI * 4000.0f / sr;
        gritLPCoef = 1.0f - expf(-gritW);
    }
    
    void setMode(FXMode mode) { this->mode = mode; }
    void setAmount(float amt) { amount = clampf(amt, 0.0f, 1.0f); }
    
    float process(float input, float bandpass, float gate) {
        if (mode == FX_CLEAN || amount < 0.01f) {
            return input;
        }
        
        // Scaled amount curve: <30% subtle, 30-70% transitional, 70%+ full character
        float scaledAmt;
        if (amount < 0.3f) {
            scaledAmt = amount * 0.3f;
        } else if (amount < 0.7f) {
            scaledAmt = 0.09f + (amount - 0.3f) * 1.0f;
        } else {
            scaledAmt = 0.49f + (amount - 0.7f) * 1.7f;
        }
        
        float wet = input;
        float makeupGain = 1.0f;
        
        switch (mode) {
            case FX_TUBE:
                wet = processTube(input, gate, scaledAmt, makeupGain);
                break;
            case FX_SCREAMER:
                wet = processScreamer(input, gate, scaledAmt, makeupGain);
                break;
            case FX_GRIT:
                wet = processGrit(input, bandpass, gate, scaledAmt, makeupGain);
                break;
            default:
                break;
        }
        
        wet *= makeupGain;
        return lerpf(input, wet, amount);
    }
    
    void reset() {
        tubeGridState = 0.0f;
        tubeDCPrev = 0.0f;
        tubeDCOut = 0.0f;
        screamerHP_z = 0.0f;
        screamerLP_z = 0.0f;
        gritLP_z = 0.0f;
        gritHold = 0.0f;
        gritCounter = 0.0f;
        gritFeedback = 0.0f;
    }
    
private:
    FXMode mode = FX_CLEAN;
    float amount = 0.0f;
    float sampleRate = 48000.0f;
    
    // Tube state
    float tubeGridState = 0.0f;
    float tubeDCPrev = 0.0f;
    float tubeDCOut = 0.0f;
    
    // Screamer state
    float screamerHP_z = 0.0f;
    float screamerLP_z = 0.0f;
    float screamerHPCoef = 0.1f;
    float screamerLPCoef = 0.1f;
    
    // Grit state
    float gritLP_z = 0.0f;
    float gritHold = 0.0f;
    float gritCounter = 0.0f;
    float gritFeedback = 0.0f;
    float gritLPCoef = 0.5f;
    
    // TUBE - Rich 12AX7 style saturation with grid blocking
    float processTube(float x, float gate, float amt, float& makeup) {
        float drive = 1.5f + amt * 6.0f * (0.5f + gate * 0.5f);
        x *= drive;
        
        // DC offset for asymmetric harmonics (tube character)
        float dcOffset = amt * 0.18f;
        x += dcOffset;
        
        // Asymmetric soft clipping - positive clips softer (triode character)
        float out;
        if (x > 0.0f) {
            out = x / (1.0f + x * (0.3f + amt * 0.5f));
        } else {
            out = x / (1.0f - x * (0.15f + amt * 0.25f));
        }
        
        // Second harmonic (even harmonics = tube warmth)
        float h2 = x * fabsf(x) * 0.2f * amt;
        out += h2;
        
        // Grid blocking (compression at high levels)
        if (amt > 0.4f && x > 0.5f) {
            float excess = x - 0.5f;
            tubeGridState -= fast_tanh(excess * 3.0f) * 0.0005f * amt;
        }
        tubeGridState *= 0.9998f;
        out += tubeGridState;
        
        // DC blocker
        float dcBlocked = out - tubeDCPrev + 0.995f * tubeDCOut;
        tubeDCPrev = out;
        tubeDCOut = dcBlocked;
        out = dcBlocked;
        
        makeup = 1.4f + amt * 0.4f;
        return out;
    }
    
    // SCREAMER - Aggressive Tube Screamer overdrive with bass bypass
    float processScreamer(float x, float gate, float amt, float& makeup) {
        float gain = 6.0f + amt * 50.0f;
        
        // Highpass - bass bypass
        float hp = x - screamerHP_z;
        screamerHP_z += screamerHPCoef * (x - screamerHP_z);
        
        // Mix back bass that bypasses the distortion
        float bassMix = 0.35f + (1.0f - amt) * 0.25f;
        float gained = hp * gain + x * bassMix;
        
        // Hard clip with tanh softening
        float threshold = 0.5f;
        float clipped;
        if (gained > threshold) {
            clipped = threshold + fast_tanh((gained - threshold) * 2.0f) * 0.4f;
        } else if (gained < -threshold) {
            clipped = -threshold + fast_tanh((gained + threshold) * 2.0f) * 0.4f;
        } else {
            clipped = gained;
        }
        
        // Lowpass to smooth
        screamerLP_z += screamerLPCoef * (clipped - screamerLP_z);
        float out = screamerLP_z;
        
        // Mid boost - the Screamer signature
        float midBoost = 1.0f + amt * 0.5f;
        out *= midBoost;
        
        makeup = 1.6f + amt * 0.6f;
        return out;
    }
    
    // GRIT - Fuzz + Bit Crush + Sample Rate Reduction with feedback
    float processGrit(float x, float bp, float gate, float amt, float& makeup) {
        float dry = x;
        
        float fuzzDrive = 2.0f + amt * 15.0f;
        float fuzzed = x * fuzzDrive;
        
        // Rectification for asymmetric harmonics
        float rectify = amt * 0.3f;
        fuzzed = fuzzed * (1.0f - rectify) + fabsf(fuzzed) * rectify;
        
        // Feedback for self-oscillation character
        fuzzed -= gritFeedback * amt * 0.4f;
        
        // DC bias for asymmetric clipping
        fuzzed += 0.15f * amt;
        
        // Hard asymmetric clipping
        if (fuzzed > 0.3f) {
            fuzzed = 0.3f + fast_tanh((fuzzed - 0.3f) * 3.0f) * 0.4f;
        } else if (fuzzed < -0.5f) {
            fuzzed = -0.5f + fast_tanh((fuzzed + 0.5f) * 2.0f) * 0.3f;
        }
        
        // Bit crush at higher amounts
        float crushed = fuzzed;
        if (amt > 0.3f) {
            float crushAmt = (amt - 0.3f) / 0.7f;
            float bits = 10.0f - crushAmt * 7.0f;  // 10-bit down to 3-bit
            float levels = powf(2.0f, bits);
            crushed = floorf(fuzzed * levels + 0.5f) / levels;
            
            // Sample rate reduction for lo-fi crunch
            if (amt > 0.5f) {
                float srReduce = 1.0f + (amt - 0.5f) * 12.0f;
                gritCounter += 1.0f;
                if (gritCounter >= srReduce) {
                    gritCounter -= srReduce;
                    gritHold = crushed;
                }
                crushed = gritHold;
            }
        }
        
        // Feedback for resonant character
        float fb = fast_tanh(gritFeedback * amt * 3.0f);
        crushed -= fb * 0.3f * amt;
        
        gritFeedback = crushed;
        
        // Light lowpass to tame aliasing
        gritLP_z += gritLPCoef * (crushed - gritLP_z);
        float out = gritLP_z;
        
        // Keep some dry signal for bass integrity
        float dryMix = 0.15f * (1.0f - amt * 0.5f);
        out = out * (1.0f - dryMix) + dry * dryMix;
        
        makeup = 1.8f + amt * 0.8f;
        return out;
    }
};

// ============================================================================
// DC BLOCKER
// ============================================================================

class DCBlocker {
public:
    float process(float x) {
        float y = x - xm1 + 0.997f * ym1;
        xm1 = x;
        ym1 = y;
        return y;
    }
    void reset() { xm1 = ym1 = 0.0f; }
private:
    float xm1 = 0.0f, ym1 = 0.0f;
};

// ============================================================================
// TRIGGER DETECTOR - Schmitt trigger with hysteresis + rearm guard
//
// CRITICAL FIX (v7.2.0): The old detector used a single threshold with
// only 5ms lockout. Noisy Eurorack triggers would fire twice:
//   1. Rising edge crosses threshold → trigger #1, lockout starts
//   2. Signal wobbles below threshold during pulse
//   3. After 5ms lockout: signal still high, or noise re-crosses → trigger #2
//
// Fix: Schmitt trigger (separate high/low thresholds) + must see N
// consecutive low samples before re-arming + 15ms lockout.
// ============================================================================

class TriggerDetector {
public:
    void setSampleRate(float sr) { sampleRate = sr; }
    void setThreshold(float v) { 
        thresholdHigh = clampf(v, 0.01f, 5.0f);
        // Hysteresis: must drop to 70% of threshold before re-arming
        thresholdLow = thresholdHigh * 0.7f;
    }
    
    bool process(float input) {
        if (lockoutSamples > 0) {
            lockoutSamples--;
        }
        
        // Schmitt trigger logic:
        // - To fire: input must cross thresholdHigh while armed
        // - To re-arm: input must drop below thresholdLow for minLowSamples
        bool aboveHigh = input > thresholdHigh;
        bool belowLow = input < thresholdLow;
        
        // Track consecutive samples below low threshold for re-arm
        if (belowLow) {
            lowCount++;
        } else {
            lowCount = 0;
        }
        
        // Re-arm only after signal has been convincingly low
        if (!armed && lowCount >= minLowSamples) {
            armed = true;
        }
        
        // Fire on rising edge above high threshold, if armed and not locked out
        bool trig = aboveHigh && armed && (lockoutSamples == 0);
        
        if (trig) {
            lastLevel = input;
            armed = false;  // Must re-arm before next trigger
            lowCount = 0;
            lockoutSamples = (int)(sampleRate * 0.015f);  // 15ms lockout
        }
        
        return trig;
    }
    
    float getLastLevel() const { return lastLevel; }
    void reset() { 
        armed = true; 
        lastLevel = 0.0f; 
        lockoutSamples = 0; 
        lowCount = 0; 
    }
    
private:
    float sampleRate = 48000.0f;
    float thresholdHigh = 0.1f;
    float thresholdLow = 0.07f;
    bool armed = true;
    float lastLevel = 0.0f;
    int lockoutSamples = 0;
    int lowCount = 0;
    static constexpr int minLowSamples = 16;  // ~0.33ms at 48kHz — must be low this long to re-arm
};

// ============================================================================
// LPG CHANNEL - Single vactrol model with level-dependent decay
//
// Based on Parker & D'Angelo, DAFX-13:
// One continuous curve from the vactrol's photoresistive element.
// Filter and VCA derive from nonlinear transfer functions of
// the single vactrol state, not separate envelopes.
// ============================================================================

class LPGChannel {
public:
    void setSampleRate(float sr) {
        sampleRate = sr;
        filter.setSampleRate(sr);
        fx.setSampleRate(sr);
    }
    
    void setParams(float resonance, float decayParam, float openParam, float dampening,
                   MaterialMode material, FXMode fxMode, float fxAmount, float inputGain,
                   bool hitMemory) {
        this->baseOpenCeiling = openParam;
        this->openCeiling = openParam;
        this->dampening = dampening;
        this->material = material;
        this->inputGain = inputGain;
        this->hitMemoryOn = hitMemory;
        this->baseDecayParam = decayParam;
        
        updateDecayFromParam(decayParam);
        
        // Dampening = hand on drum / towel on cymbal
        // Reduces brightness MORE aggressively (mutes upper harmonics)
        // Also slightly reduces resonance (dampened objects don't ring)
        float dampeningBrightness = 1.0f - dampening * 0.85f;  // At 100%: 15% brightness
        float dampeningResCut = 1.0f - dampening * 0.4f;       // At 100%: 60% resonance
        
        filter.setResonance(resonance * dampeningResCut);
        filter.setBrightness(kMaterialBrightness[material] * dampeningBrightness);
        
        fx.setMode(fxMode);
        fx.setAmount(fxAmount);
    }
    
    // Fast CV update - only updates targets, no expensive calculations
    void updateCV(float decayMod, float openMod) {
        openCeiling = clampf(baseOpenCeiling + openMod, 0.0f, 1.0f);
        float modDecay = clampf(baseDecayParam + decayMod, 0.0f, 1.0f);
        updateDecayFromParam(modDecay);
    }
    
    void trigger(float velocity = 1.0f) {
        float targetLevel = velocity * openCeiling;
        
        if (hitMemoryOn) {
            float previousState = vactrolState;
            targetLevel = clampf(vactrolState + targetLevel, 0.0f, 1.2f);
            
            // Warm vactrol effect: accumulated energy means the vactrol
            // stays open longer. Scale decay slowdown by how much state
            // we're building on. At full accumulation, decay slows ~40%.
            float warmth = clampf(previousState * 0.4f, 0.0f, 0.4f);
            memoryDecayScale = 1.0f + warmth;  // 1.0 = no effect, 1.4 = 40% slower
        } else {
            memoryDecayScale = 1.0f;
        }
        
        // SINGLE VACTROL MODEL — The vactrol IS the envelope.
        //
        // In the real Buchla 292, CV hits the LED, which instantly illuminates
        // the photoresistor. The "click" is the gate snapping open.
        // There is NO separate filter envelope and VCA envelope.
        // One resistance (Rf) controls everything.
        //
        // The vactrol state then decays via level-dependent continuous curve
        // (modeled in process()). The "filter closes before VCA" behavior
        // comes from nonlinear transfer functions, not separate envelopes.
        vactrolState = targetLevel;
        
        triggerVelocity = velocity;
        triggerVisual = 1.0f;
        
        // Dampen filter state on retrigger to prevent energy accumulation
        filter.dampStateOnRetrigger();
    }
    
    float process(float input) {
        // =====================================================
        // SINGLE VACTROL ENVELOPE — LEVEL-DEPENDENT DECAY
        // =====================================================
        //
        // Based on Parker & D'Angelo, DAFX-13:
        // The vactrol's photoresistive element has a continuous decay
        // where higher illumination (higher state) decays faster due to
        // greater carrier recombination rate. This naturally produces
        // the "thwack → body" contour that the old two-stage approach
        // tried to achieve with a hard boundary and crossfade.
        //
        // The key: ONE continuous curve, NO stage boundaries.
        //   High level → fast decay (the initial transient/thwack)
        //   Low level → slow decay (the lingering body/tail)
        //
        // vactrolDecayMod controls the strength of this effect:
        //   0 = pure exponential (electronic, uniform decay)
        //   2+ = strong level-dependence (struck/plucked character)
        
        if (vactrolState > 0.0f) {
            // Level-dependent speed: faster at high levels, slower at low
            // The squared term gives us a continuous curve that gracefully transitions
            // from "thwack" speed to "body" speed — no stages, no crossfade
            float speedFactor = 1.0f + vactrolState * vactrolState * vactrolDecayMod;
            
            // Velocity shapes the initial speed — harder hits decay faster initially
            // (more energy in = faster initial dissipation, like a real struck object)
            float velShape = 1.0f + (triggerVelocity - 0.5f) * 0.3f * vactrolState;
            
            // OPTIMIZATION: Combine both power operations into single expf
            // powf(coef, speed) = expf(speed * logf(coef))
            // powf(result, velShape) = expf(velShape * speed * logf(coef))
            // Combined: expf(speedFactor * velShape * logBaseDecayCoef)
            // This replaces 2x powf (~200 cycles each) with 1x expf (~60 cycles)
            float totalPower = speedFactor * velShape;
            
            // Hit memory warmth: divide totalPower to slow decay
            // (higher memoryDecayScale = slower effective decay)
            totalPower /= memoryDecayScale;
            
            float effectiveCoef = expf(totalPower * logBaseDecayCoef);
            
            vactrolState *= effectiveCoef;
            
            // Denormal clamp
            if (vactrolState < 0.0001f) vactrolState = 0.0f;
        }
        
        // =====================================================
        // NONLINEAR TRANSFER: SINGLE STATE → FILTER + VCA
        // =====================================================
        //
        // In the real Buchla 292, one Rf controls both:
        //   - Filter cutoff: ∝ 1/Rf (drops early as Rf increases)
        //   - VCA gain: Rα/(Rα + 2Rf) (stays open longer, drops late)
        //
        // We model this with transfer curves:
        //   filterGate = pow(vactrolState, filterExponent)  — drops fast
        //   vcaGate = sqrt(vactrolState) or similar         — holds open
        //
        // The filterExponent is material-dependent, encoding the "pluck"
        // character that the old dual-envelope tried to create with
        // separate decay rates. Now it comes from curve shape instead.
        
        float filterGate = powf(vactrolState, filterExponent);
        float vcaGate = sqrtf(fmaxf(vactrolState, 0.0f));
        
        // Dampening: reduce VCA ceiling (hand absorbs energy, doesn't speed it up)
        // At 100% dampening: output is 25% of normal — heavily muted but same decay shape
        float dampeningVCA = 1.0f - dampening * 0.75f;
        vcaGate *= dampeningVCA;
        
        // At very low levels, ensure clean zero-crossing
        if (vcaGate < 0.001f) vcaGate = 0.0f;
        if (filterGate < 0.001f) filterGate = 0.0f;
        
        lastGate = vcaGate;
        
        input *= inputGain;
        
        float filtered = filter.process(input, filterGate, vcaGate);
        
        float bp = filter.getBandpass();
        float processed = fx.process(filtered, bp, vcaGate);
        
        processed = dcBlocker.process(processed);
        
        // Final safety limiter
        processed = soft_saturate(processed, 0.98f);
        
        // NaN/inf protection - last line of defense against lockup
        if (!(processed == processed) || processed > 10.0f || processed < -10.0f) {
            processed = 0.0f;
            filter.reset();
            dcBlocker.reset();
        }
        
        triggerVisual *= 0.96f;
        
        return processed;
    }
    
    float getGateValue() const { return lastGate; }
    float getTriggerVisual() const { return triggerVisual; }
    
    void reset() {
        filter.reset();
        fx.reset();
        dcBlocker.reset();
        vactrolState = 0.0f;
        triggerVisual = 0.0f;
        lastGate = 0.0f;
    }
    
private:
    void updateDecayFromParam(float decayParam) {
        // Non-linear scaling for musical response
        // Maps 0-1 parameter to decay time in milliseconds
        float baseDecayMs;
        if (decayParam < 0.05f) {
            float t = decayParam / 0.05f;
            baseDecayMs = 5.0f + t * 10.0f;
        } else if (decayParam < 0.15f) {
            float t = (decayParam - 0.05f) / 0.10f;
            baseDecayMs = 15.0f + t * 25.0f;
        } else if (decayParam < 0.30f) {
            float t = (decayParam - 0.15f) / 0.15f;
            baseDecayMs = 40.0f + t * 60.0f;
        } else if (decayParam < 0.50f) {
            float t = (decayParam - 0.30f) / 0.20f;
            baseDecayMs = 100.0f + t * 100.0f;
        } else if (decayParam < 0.70f) {
            float t = (decayParam - 0.50f) / 0.20f;
            baseDecayMs = 200.0f + t * 300.0f;
        } else if (decayParam < 0.85f) {
            float t = (decayParam - 0.70f) / 0.15f;
            baseDecayMs = 500.0f + t * 1000.0f;
        } else {
            float t = (decayParam - 0.85f) / 0.15f;
            baseDecayMs = 1500.0f + t * 3500.0f;
        }
        
        float vcaDecayMs = baseDecayMs * kMaterialDecayMult[material];
        // NOTE: dampening does NOT affect decay time.
        // Dampening = hand on drum: reduces brightness + output level.
        // It's applied in setParams() (brightness) and process() (VCA ceiling).
        // A dampened drum rings just as long — you just hear less of it.
        
        // SINGLE VACTROL DECAY MODEL
        // One coefficient, one continuous curve. The level-dependent
        // speed modulation in process() creates the thwack/body contour.
        float bodySamples = vcaDecayMs * 1.5f * 0.001f * sampleRate;
        
        if (bodySamples > 0.0f) {
            baseDecayCoefficient = expf(-6.9078f / bodySamples);
            // Pre-compute log for efficient per-sample powf replacement:
            // powf(coef, speed) = expf(speed * logf(coef))
            // This saves a logf() call every sample on Cortex-M7
            logBaseDecayCoef = logf(baseDecayCoefficient);
        } else {
            baseDecayCoefficient = 0.0f;
            logBaseDecayCoef = -100.0f; // large negative → instant decay
        }
        
        // Vactrol level-dependent modulation from material
        vactrolDecayMod = kMaterialVactrolMod[material];
        
        // Filter transfer exponent from material
        filterExponent = kMaterialFilterExponent[material];
    }
    
    float sampleRate = 48000.0f;
    float openCeiling = 1.0f;
    float baseOpenCeiling = 1.0f;
    float dampening = 0.0f;
    float inputGain = 1.0f;
    float baseDecayParam = 0.5f;
    MaterialMode material = MATERIAL_NATURAL;
    bool hitMemoryOn = false;
    
    float vactrolState = 0.0f;      // Single vactrol photoresistive state (0=dark, 1=bright)
    float triggerVelocity = 1.0f;
    float memoryDecayScale = 1.0f;   // Hit memory warmth: >1 = slower decay from accumulated energy
    
    // Single vactrol decay model
    float baseDecayCoefficient = 0.999f;    // Base decay rate (body/tail speed)
    float logBaseDecayCoef = -0.001f;       // Pre-computed logf(baseDecayCoef) for Cortex-M7 optimization
    float vactrolDecayMod = 2.5f;           // Level-dependent speed modulation
    float filterExponent = 1.8f;            // Nonlinear filter transfer curve
    
    float triggerVisual = 0.0f;
    float lastGate = 0.0f;
    
    BuchlaLPGFilter filter;
    FXProcessor fx;
    DCBlocker dcBlocker;
};

// ============================================================================
// MAIN ALGORITHM
// ============================================================================

struct _holyMackerelAlgorithm : public _NT_algorithm {
    _holyMackerelAlgorithm() {}
    ~_holyMackerelAlgorithm() {}
    
    float sampleRate;
    
    LPGChannel channelL;
    LPGChannel channelR;
    TriggerDetector trigger;
    
    float hitIntensity;
    float hitPhase;
};

// ============================================================================
// PARAMETERS - All defaults restored from v5.7
// ============================================================================

enum {
    kParamResonance,
    kParamDecay,
    kParamOpen,
    kParamDampening,
    kParamMaterial,
    kParamFX,
    kParamFXAmount,
    kParamGain,
    kParamHitMemory,
    
    kParamResonanceCV,
    kParamDecayCV,
    kParamOpenCV,
    kParamDampeningCV,
    kParamFXAmountCV,
    
    kParamTriggerInput,
    kParamTriggerThreshold,
    kParamStereo,
    kParamLeftInput,
    kParamRightInput,
    kParamLeftOutput,
    kParamLeftOutputMode,
    kParamRightOutput,
    kParamRightOutputMode,
    kParamEnvFollower,
    kParamEnvOutput,
    
    kNumParams
};

static const char* const materialStrings[] = { "Natural", "Hard", "Soft", nullptr };
static const char* const fxStrings[] = { "Clean", "Tube", "Screamer", "Grit", nullptr };
static const char* const stereoStrings[] = { "Mono", "Stereo", nullptr };
static const char* const onOffStrings[] = { "Off", "On", nullptr };

static const _NT_parameter parameters[] = {
    // Page 1: Holy Mackerel
    { .name = "Resonance",   .min = 0,  .max = 100, .def = 0,   .unit = kNT_unitPercent,     .scaling = kNT_scalingNone, .enumStrings = NULL },
    { .name = "Decay",       .min = 0,  .max = 100, .def = 50,  .unit = kNT_unitPercent,     .scaling = kNT_scalingNone, .enumStrings = NULL },
    { .name = "Open",        .min = 0,  .max = 100, .def = 100, .unit = kNT_unitPercent,     .scaling = kNT_scalingNone, .enumStrings = NULL },
    { .name = "Dampening",   .min = 0,  .max = 100, .def = 0,   .unit = kNT_unitPercent,     .scaling = kNT_scalingNone, .enumStrings = NULL },
    { .name = "Material",    .min = 0,  .max = 2,   .def = 0,   .unit = kNT_unitEnum,        .scaling = kNT_scalingNone, .enumStrings = materialStrings },
    { .name = "FX",          .min = 0,  .max = 3,   .def = 0,   .unit = kNT_unitEnum,        .scaling = kNT_scalingNone, .enumStrings = fxStrings },
    { .name = "FX Amount",   .min = 0,  .max = 100, .def = 0,   .unit = kNT_unitPercent,     .scaling = kNT_scalingNone, .enumStrings = NULL },
    { .name = "Gain",        .min = 0,  .max = 106, .def = 100, .unit = kNT_unitNone,        .scaling = kNT_scalingNone, .enumStrings = NULL },
    { .name = "Hit Memory",  .min = 0,  .max = 1,   .def = 0,   .unit = kNT_unitEnum,        .scaling = kNT_scalingNone, .enumStrings = onOffStrings },
    
    // Page 2: CV Control - using macros for consistency
    NT_PARAMETER_CV_INPUT( "Resonance CV", 0, 0 )
    NT_PARAMETER_CV_INPUT( "Decay CV", 0, 0 )
    NT_PARAMETER_CV_INPUT( "Open CV", 0, 0 )
    NT_PARAMETER_CV_INPUT( "Dampening CV", 0, 0 )
    NT_PARAMETER_CV_INPUT( "FX Amt CV", 0, 0 )
    
    // Page 3: Routing
    NT_PARAMETER_CV_INPUT( "Trigger Input", 0, 3 )      // min=0 (off), default=3
    { .name = "Trig Threshold", .min = 10, .max = 500, .def = 100, .unit = kNT_unitMillivolts, .scaling = kNT_scalingNone, .enumStrings = NULL },
    { .name = "Stereo",         .min = 0,  .max = 1,   .def = 1,   .unit = kNT_unitEnum,       .scaling = kNT_scalingNone, .enumStrings = stereoStrings },
    NT_PARAMETER_AUDIO_INPUT( "Left Input", 1, 1 )
    NT_PARAMETER_AUDIO_INPUT( "Right Input", 1, 2 )
    NT_PARAMETER_AUDIO_OUTPUT_WITH_MODE( "Left Output", 1, 13 )
    NT_PARAMETER_AUDIO_OUTPUT_WITH_MODE( "Right Output", 1, 14 )
    { .name = "Env Follower",   .min = 0,  .max = 1,   .def = 0,   .unit = kNT_unitEnum,       .scaling = kNT_scalingNone, .enumStrings = onOffStrings },
    NT_PARAMETER_CV_OUTPUT( "Env Output", 0, 0 )
};

static const uint8_t page1[] = { kParamResonance, kParamDecay, kParamOpen, kParamDampening, kParamMaterial, kParamFX, kParamFXAmount, kParamGain, kParamHitMemory };
static const uint8_t page2[] = { kParamResonanceCV, kParamDecayCV, kParamOpenCV, kParamDampeningCV, kParamFXAmountCV };
static const uint8_t page3[] = { kParamTriggerInput, kParamTriggerThreshold, kParamStereo, kParamLeftInput, kParamRightInput, kParamLeftOutput, kParamLeftOutputMode, kParamRightOutput, kParamRightOutputMode, kParamEnvFollower, kParamEnvOutput };

static const _NT_parameterPage pages[] = {
    { .name = "Holy Mackerel", .numParams = ARRAY_SIZE(page1), .params = page1 },
    { .name = "CV Control",    .numParams = ARRAY_SIZE(page2), .params = page2 },
    { .name = "Routing",       .numParams = ARRAY_SIZE(page3), .params = page3 },
};

static const _NT_parameterPages parameterPages = {
    .numPages = ARRAY_SIZE(pages),
    .pages = pages,
};

// ============================================================================
// HELPERS
// ============================================================================

static inline float getGainFromParam(int g) {
    if (g <= 100) {
        return g / 100.0f;
    } else {
        // 101-106 = +1dB steps (approx)
        return 1.0f + (g - 100) / 6.0f;
    }
}

// ============================================================================
// GREYING LOGIC - Hide irrelevant parameters contextually
// ============================================================================

void updateGrayed(_holyMackerelAlgorithm* alg) {
    int idx = NT_algorithmIndex(alg);
    int off = NT_parameterOffset();
    bool mono = (alg->v[kParamStereo] == 0);
    bool clean = (alg->v[kParamFX] == FX_CLEAN);
    bool envOff = (alg->v[kParamEnvFollower] == 0);
    
    // Grey Right I/O when in mono mode
    NT_setParameterGrayedOut(idx, kParamRightInput + off, mono);
    NT_setParameterGrayedOut(idx, kParamRightOutput + off, mono);
    NT_setParameterGrayedOut(idx, kParamRightOutputMode + off, mono);
    
    // Grey FX Amount and its CV when FX is Clean
    NT_setParameterGrayedOut(idx, kParamFXAmount + off, clean);
    NT_setParameterGrayedOut(idx, kParamFXAmountCV + off, clean);
    
    // Grey Env Output when Env Follower is off
    NT_setParameterGrayedOut(idx, kParamEnvOutput + off, envOff);
}

// ============================================================================
// FACTORY FUNCTIONS
// ============================================================================

void calculateRequirements(_NT_algorithmRequirements& req, const int32_t* specifications) {
    req.numParameters = kNumParams;
    req.sram = sizeof(_holyMackerelAlgorithm);
    req.dram = 0;
    req.dtc = 0;
    req.itc = 0;
}

_NT_algorithm* construct(const _NT_algorithmMemoryPtrs& ptrs, const _NT_algorithmRequirements& req, const int32_t* specifications) {
    _holyMackerelAlgorithm* alg = new (ptrs.sram) _holyMackerelAlgorithm();
    alg->parameters = parameters;
    alg->parameterPages = &parameterPages;
    
    alg->sampleRate = (float)NT_globals.sampleRate;
    alg->channelL.setSampleRate(alg->sampleRate);
    alg->channelR.setSampleRate(alg->sampleRate);
    alg->trigger.setSampleRate(alg->sampleRate);
    alg->trigger.reset();
    alg->trigger.setThreshold(alg->v[kParamTriggerThreshold] / 1000.0f);
    
    alg->hitIntensity = 0.0f;
    alg->hitPhase = 0.0f;
    
    return alg;
}

void parameterChanged(_NT_algorithm* self, int p) {
    _holyMackerelAlgorithm* alg = (_holyMackerelAlgorithm*)self;
    
    float resonance = alg->v[kParamResonance] / 100.0f;
    float decay = alg->v[kParamDecay] / 100.0f;
    float open = alg->v[kParamOpen] / 100.0f;
    float dampening = alg->v[kParamDampening] / 100.0f;
    MaterialMode material = (MaterialMode)alg->v[kParamMaterial];
    FXMode fxMode = (FXMode)alg->v[kParamFX];
    float fxAmount = alg->v[kParamFXAmount] / 100.0f;
    float gain = getGainFromParam(alg->v[kParamGain]);
    bool hitMemory = (alg->v[kParamHitMemory] == 1);
    
    alg->channelL.setParams(resonance, decay, open, dampening, material, fxMode, fxAmount, gain, hitMemory);
    alg->channelR.setParams(resonance, decay, open, dampening, material, fxMode, fxAmount, gain, hitMemory);
    
    if (p == kParamTriggerThreshold) {
        // Threshold in millivolts → volts
        alg->trigger.setThreshold(alg->v[kParamTriggerThreshold] / 1000.0f);
    }
    
    // Update greying when relevant params change
    if (p == kParamFX || p == kParamStereo || p == kParamEnvFollower) {
        updateGrayed(alg);
    }
}

// ============================================================================
// AUDIO PROCESSING
// ============================================================================

void step(_NT_algorithm* self, float* busFrames, int numFramesBy4) {
    _holyMackerelAlgorithm* alg = (_holyMackerelAlgorithm*)self;
    int numFrames = numFramesBy4 * 4;
    
    int trigBus = alg->v[kParamTriggerInput];
    int lInBus = alg->v[kParamLeftInput] - 1;
    int rInBus = alg->v[kParamRightInput] - 1;
    int lOutBus = alg->v[kParamLeftOutput] - 1;
    int rOutBus = alg->v[kParamRightOutput] - 1;
    int envOutBus = alg->v[kParamEnvOutput];
    bool lReplace = alg->v[kParamLeftOutputMode];
    bool rReplace = alg->v[kParamRightOutputMode];
    bool stereo = (alg->v[kParamStereo] == 1);
    bool envFollowerOn = (alg->v[kParamEnvFollower] == 1);
    
    const float* trigIn = (trigBus > 0) ? busFrames + (trigBus - 1) * numFrames : nullptr;
    const float* lIn = busFrames + lInBus * numFrames;
    const float* rIn = stereo ? (busFrames + rInBus * numFrames) : lIn;
    float* lOut = busFrames + lOutBus * numFrames;
    float* rOut = stereo ? (busFrames + rOutBus * numFrames) : nullptr;
    // Env output respects the Env Follower on/off switch
    float* envOut = (envFollowerOn && envOutBus > 0) ? busFrames + (envOutBus - 1) * numFrames : nullptr;
    
    int resCVBus = alg->v[kParamResonanceCV];
    int decCVBus = alg->v[kParamDecayCV];
    int openCVBus = alg->v[kParamOpenCV];
    int dampCVBus = alg->v[kParamDampeningCV];
    int fxCVBus = alg->v[kParamFXAmountCV];
    
    const float* resCV = (resCVBus > 0) ? busFrames + (resCVBus - 1) * numFrames : nullptr;
    const float* decCV = (decCVBus > 0) ? busFrames + (decCVBus - 1) * numFrames : nullptr;
    const float* openCV = (openCVBus > 0) ? busFrames + (openCVBus - 1) * numFrames : nullptr;
    const float* dampCV = (dampCVBus > 0) ? busFrames + (dampCVBus - 1) * numFrames : nullptr;
    const float* fxCV = (fxCVBus > 0) ? busFrames + (fxCVBus - 1) * numFrames : nullptr;
    
    float baseRes = alg->v[kParamResonance] / 100.0f;
    float baseDec = alg->v[kParamDecay] / 100.0f;
    float baseOpen = alg->v[kParamOpen] / 100.0f;
    float baseDamp = alg->v[kParamDampening] / 100.0f;
    float baseFX = alg->v[kParamFXAmount] / 100.0f;
    MaterialMode material = (MaterialMode)alg->v[kParamMaterial];
    FXMode fxMode = (FXMode)alg->v[kParamFX];
    float gain = getGainFromParam(alg->v[kParamGain]);
    bool hitMemory = (alg->v[kParamHitMemory] == 1);
    
    for (int i = 0; i < numFrames; ++i) {
        if (trigIn && alg->trigger.process(trigIn[i])) {
            // Velocity: scale trigger level to 0.35-1.0 range
            // Floor at 0.35 prevents natural trigger voltage wobble from
            // creating wildly different hit intensities. Low enough for
            // false triggers to be quiet, high enough for consistency.
            float vel = clampf(alg->trigger.getLastLevel() / 5.0f, 0.35f, 1.0f);
            alg->channelL.trigger(vel);
            if (stereo) alg->channelR.trigger(vel);
            
            alg->hitIntensity = vel;
            alg->hitPhase = 0.0f;
        }
        
        if (resCV || decCV || openCV || dampCV || fxCV) {
            // Update every 32 samples (~1500Hz update rate)
            if ((i & 31) == 0) {
                float r = baseRes, d = baseDec, o = baseOpen, dp = baseDamp, f = baseFX;
                if (resCV) r = clampf(baseRes + resCV[i] * 0.1f, 0.0f, 1.0f);
                if (decCV) d = clampf(baseDec + decCV[i] * 0.1f, 0.0f, 1.0f);
                if (openCV) o = clampf(baseOpen + openCV[i] * 0.1f, 0.0f, 1.0f);
                if (dampCV) dp = clampf(baseDamp + dampCV[i] * 0.1f, 0.0f, 1.0f);
                if (fxCV) f = clampf(baseFX + fxCV[i] * 0.1f, 0.0f, 1.0f);
                
                alg->channelL.setParams(r, d, o, dp, material, fxMode, f, gain, hitMemory);
                if (stereo) alg->channelR.setParams(r, d, o, dp, material, fxMode, f, gain, hitMemory);
            }
        }
        
        float outL = alg->channelL.process(lIn[i]);
        
        if (lReplace) lOut[i] = outL;
        else lOut[i] += outL;
        
        if (stereo && rOut) {
            float outR = alg->channelR.process(rIn[i]);
            if (rReplace) rOut[i] = outR;
            else rOut[i] += outR;
        }
        
        if (envOut) {
            float gateL = alg->channelL.getGateValue();
            float gateR = stereo ? alg->channelR.getGateValue() : gateL;
            envOut[i] = ((gateL + gateR) * 0.5f) * 5.0f;
        }
    }
    
    alg->hitPhase += 0.06f;
    // Cap to prevent unbounded float growth over long sessions without triggers
    // Animation is invisible past ~25 since expf(-10) ≈ 0.00005
    if (alg->hitPhase > 100.0f) alg->hitPhase = 100.0f;
}

// ============================================================================
// UI
// ============================================================================

bool draw(_NT_algorithm* self) {
    _holyMackerelAlgorithm* alg = (_holyMackerelAlgorithm*)self;
    
    const int yOffset = 6;
    
    const int faderX = 4;
    const int faderSpacing = 18;
    const int faderTopY = 14 + yOffset;
    const int faderBottomY = 44 + yOffset;     // Was 48 - raised 4px for label clearance
    const int faderHeight = faderBottomY - faderTopY;
    const int faderWidth = 6;
    
    const char* faderLabels[] = { "RES", "DEC", "OPN", "DMP", "FX" };
    float faderValues[] = { 
        alg->v[kParamResonance] / 100.0f, 
        alg->v[kParamDecay] / 100.0f, 
        alg->v[kParamOpen] / 100.0f,
        alg->v[kParamDampening] / 100.0f, 
        alg->v[kParamFXAmount] / 100.0f 
    };
    
    for (int f = 0; f < 5; f++) {
        int x = faderX + f * faderSpacing;
        
        NT_drawShapeI(kNT_box, x, faderTopY, x + faderWidth, faderBottomY, 6);
        
        float val = faderValues[f];
        int fillHeight = (int)(val * faderHeight);
        if (fillHeight > 0) {
            NT_drawShapeI(kNT_rectangle, x + 1, faderBottomY - fillHeight, 
                          x + faderWidth - 1, faderBottomY - 1, 11);
        }
        
        int handleY = faderBottomY - fillHeight - 2;
        if (handleY >= faderTopY) {
            NT_drawShapeI(kNT_rectangle, x, handleY, x + faderWidth, handleY + 3, 15);
        }
        
        NT_drawText(x + faderWidth / 2, faderBottomY + 6, faderLabels[f], 5, kNT_textCentre, kNT_textTiny);
    }
    
    // Mode indicators - dropped 4px to avoid parameter bar cutoff
    const char* matStr[] = { "NAT", "HRD", "SFT" };
    const char* fxStr[] = { "CLN", "TUB", "SCR", "GRT" };
    NT_drawText(32, faderTopY - 4, matStr[alg->v[kParamMaterial]], 5, kNT_textCentre, kNT_textTiny);
    NT_drawText(68, faderTopY - 4, fxStr[alg->v[kParamFX]], 5, kNT_textCentre, kNT_textTiny);
    
    // Gain display
    char gainBuf[8];
    int gainVal = alg->v[kParamGain];
    if (gainVal <= 100) snprintf(gainBuf, sizeof(gainBuf), "%d", gainVal);
    else snprintf(gainBuf, sizeof(gainBuf), "+%d", gainVal - 100);
    NT_drawText(95, faderTopY - 4, gainBuf, 5, kNT_textCentre, kNT_textTiny);
    
    // Hit Memory indicator
    if (alg->v[kParamHitMemory] == 1) {
        NT_drawText(95, faderBottomY + 6, "MEM", 12, kNT_textCentre, kNT_textTiny);
    }
    
    // Gate visualization
    const int hitCenterX = 175;
    const int hitCenterY = 32 + yOffset;
    // Derive max safe radius from center position and screen height (63)
    // so clamps stay correct if yOffset or center ever changes
    const int maxYRadius = ((hitCenterY < (63 - hitCenterY)) ? hitCenterY : (63 - hitCenterY)) - 1;
    
    // 4-bit grayscale color clamp (0-15) — exceeding this range can index
    // past the hardware palette LUT and corrupt memory (Thornside PR #1)
    auto clampColor = [](int c) -> int { return (c < 0) ? 0 : ((c > 15) ? 15 : c); };
    
    float gateL = alg->channelL.getGateValue();
    float gateR = (alg->v[kParamStereo] == 1) ? alg->channelR.getGateValue() : gateL;
    float gate = (gateL + gateR) * 0.5f;
    
    float hitVis = alg->hitIntensity * expf(-alg->hitPhase * 0.4f);
    
    int numRays = 16;
    float baseRadius = 8.0f + gate * 15.0f;
    float burstRadius = baseRadius + hitVis * 20.0f;
    // Cap burstRadius so all derived shapes stay within screen bounds
    if (burstRadius > (float)(maxYRadius - 8)) burstRadius = (float)(maxYRadius - 8);
    
    if (hitVis > 0.1f) {
        int glowR = (int)(burstRadius + 5 + hitVis * 6);
        if (glowR > maxYRadius) glowR = maxYRadius;
        NT_drawShapeI(kNT_circle, hitCenterX - glowR, hitCenterY - glowR,
                      hitCenterX + glowR, hitCenterY + glowR, clampColor(4 + (int)(hitVis * 3)));
    }
    
    for (int r = 0; r < numRays; r++) {
        float angle = (float)r * TWO_PI / numRays;
        if (hitVis > 0.05f) angle += alg->hitPhase * 0.15f;
        
        float innerR = 3.0f + gate * 5.0f;
        int x1 = hitCenterX + (int)(cosf(angle) * innerR);
        int y1 = hitCenterY + (int)(sinf(angle) * innerR);
        
        float lenMod = (r % 4 == 0) ? 1.0f : ((r % 4 == 2) ? 0.4f : 0.65f);
        float outerR = burstRadius * lenMod;
        int x2 = hitCenterX + (int)(cosf(angle) * outerR);
        int y2 = hitCenterY + (int)(sinf(angle) * outerR);
        
        NT_drawShapeI(kNT_line, x1, y1, x2, y2, clampColor(8 + (int)(gate * 5) + (int)(hitVis * 2)));
    }
    
    if (hitVis > 0.05f) {
        int ringR = (int)(burstRadius * 0.7f + hitVis * 10);
        if (ringR > maxYRadius) ringR = maxYRadius;
        NT_drawShapeI(kNT_circle, hitCenterX - ringR, hitCenterY - ringR,
                      hitCenterX + ringR, hitCenterY + ringR, clampColor(7 + (int)(hitVis * 4)));
    }
    
    int centerR = 4 + (int)(gate * 6);
    NT_drawShapeI(kNT_rectangle, hitCenterX - centerR, hitCenterY - centerR,
                  hitCenterX + centerR, hitCenterY + centerR, 15);
    
    int boundaryR = (int)burstRadius + 8;
    if (boundaryR > maxYRadius) boundaryR = maxYRadius;
    NT_drawShapeI(kNT_circle, hitCenterX - boundaryR, hitCenterY - boundaryR,
                  hitCenterX + boundaryR, hitCenterY + boundaryR, 5);
    
    // Title and version
    NT_drawText(250, 8, "HOLY", 7, kNT_textRight, kNT_textTiny);
    NT_drawText(250, 16, "MACKEREL", 7, kNT_textRight, kNT_textTiny);
    NT_drawText(250, 24, "v7.2.0", 5, kNT_textRight, kNT_textTiny);
    
    // Gate percentage — only draw if text stays within screen bounds
    int textY = hitCenterY + boundaryR + 8;
    if (textY <= 63) {
        char gateBuf[8];
        snprintf(gateBuf, sizeof(gateBuf), "%d%%", (int)(gate * 100));
        NT_drawText(hitCenterX, textY, gateBuf, 6, kNT_textCentre, kNT_textTiny);
    }
    
    return false;
}

// ============================================================================
// FACTORY
// ============================================================================

static const _NT_factory factory = {
    .guid = NT_MULTICHAR('H', 'm', 'a', 'c'),
    .name = "Holy Mackerel",
    .description = "Low Pass Gate with Smile Pass filter and Hate - The Reunion",
    .numSpecifications = 0,
    .specifications = nullptr,
    .calculateStaticRequirements = nullptr,
    .initialise = nullptr,
    .calculateRequirements = calculateRequirements,
    .construct = construct,
    .parameterChanged = parameterChanged,
    .step = step,
    .draw = draw,
    .midiRealtime = nullptr,
    .midiMessage = nullptr,
    .tags = kNT_tagFilterEQ | kNT_tagEffect,
    .hasCustomUi = nullptr,
    .customUi = nullptr,
    .setupUi = nullptr,
    .serialise = nullptr,
    .deserialise = nullptr,
    .midiSysEx = nullptr,
    .parameterUiPrefix = nullptr,
};

uintptr_t pluginEntry(_NT_selector selector, uint32_t data) {
    switch (selector) {
        case kNT_selector_version:
            return kNT_apiVersionCurrent;
        case kNT_selector_numFactories:
            return 1;
        case kNT_selector_factoryInfo:
            return (uintptr_t)((data == 0) ? &factory : nullptr);
    }
    return 0;
}
