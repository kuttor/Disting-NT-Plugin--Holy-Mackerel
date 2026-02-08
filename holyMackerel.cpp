/*
 * Holy Mackerel v5.6.0 - Vactrol-less Low Pass Gate with Hate
 * Expert Sleepers Disting NT
 *
 * v5.6.1 - CORRECT OPEN/DECAY ARCHITECTURE
 *
 * OPEN parameter: Controls HOW MUCH the gate opens (ceiling)
 * - 0%: Gate barely opens → very quiet, filtered clicks
 * - 50%: Gate opens halfway → medium volume and brightness
 * - 100%: Gate fully opens → full volume and brightness
 * 
 * DECAY parameter: How long the envelope takes to decay back to ZERO
 * - 0-5%: Clicks (5-20ms)
 * - 5-20%: Short plucks (20-100ms)
 * - 20-50%: Medium (100-500ms)
 * - 50-80%: Long (500ms-2s)
 * - 80-100%: Very long (2s-8s)
 *
 * INTERACTION:
 * - Open controls the CEILING (max level the gate reaches)
 * - Decay controls how long it takes to close back to silence
 * - Both affect volume AND harmonics (LPG characteristic)
 *
 * HIT MEMORY:
 * - Triggers ADD to current level when Memory is ON
 * - Fast repeated hits build up brightness and volume
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
    0.005f,     // Natural - 5ms
    0.0008f,    // Hard - 0.8ms aggressive snap
    0.020f      // Soft - 20ms rounded
};

// Decay multipliers
static const float kMaterialDecayMult[3] = {
    1.0f,       // Natural
    0.6f,       // Hard - shorter, snappier
    1.8f        // Soft - longer, more sustain
};

// Filter brightness (affects cutoff range)
static const float kMaterialBrightness[3] = {
    1.0f,       // Natural - full range
    1.8f,       // Hard - brighter, more highs
    0.4f        // Soft - darker, muted highs
};

// How much faster filter decays vs VCA
static const float kMaterialFilterDecayRatio[3] = {
    2.5f,       // Natural - classic LPG pluck
    4.0f,       // Hard - very fast filter decay (maximum pluck)
    1.5f        // Soft - gentler difference (rounder sound)
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
// VACTROL MODEL - Simple attack/decay smoother
// 
// The vactrol smooths toward the target with different attack and decay rates.
// Attack = how fast it responds to increasing target
// Decay = how fast it responds to decreasing target
// ============================================================================

class VactrolModel {
public:
    void setSampleRate(float sr) {
        sampleRate = sr;
    }
    
    void setTimes(float attackMs, float decayMs) {
        // Convert ms to coefficient
        if (attackMs > 0.0f) {
            attackCoef = 1.0f - expf(-1.0f / (attackMs * 0.001f * sampleRate));
        } else {
            attackCoef = 1.0f;  // Instant
        }
        
        if (decayMs > 0.0f) {
            decayCoef = 1.0f - expf(-1.0f / (decayMs * 0.001f * sampleRate));
        } else {
            decayCoef = 1.0f;  // Instant
        }
    }
    
    void setMemoryMode(bool enabled) {
        // Not used in simplified model - memory handled in LPGChannel
    }
    
    float trigger(float velocity) {
        // Not used in simplified model - triggering handled in LPGChannel
        return velocity;
    }
    
    float process(float target) {
        // Simple one-pole smoother with different attack/decay rates
        float coef;
        if (target > state) {
            coef = attackCoef;  // Rising toward target
        } else {
            coef = decayCoef;   // Falling toward target
        }
        
        state += coef * (target - state);
        return clampf(state, 0.0f, 1.5f);
    }
    
    float getValue() const { return state; }
    void reset() { state = 0.0f; }
    void setState(float s) { state = clampf(s, 0.0f, 1.5f); }  // For instant trigger
    
private:
    float sampleRate = 48000.0f;
    float attackCoef = 0.1f;
    float decayCoef = 0.001f;
    float state = 0.0f;
};

// ============================================================================
// LPG FILTER - Clean at 0% resonance
// ============================================================================

class BuchlaLPGFilter {
public:
    void setSampleRate(float sr) {
        sampleRate = sr;
        maxCutoff = sr * 0.45f;
    }
    
    void setResonance(float res) {
        resonance = clampf(res, 0.0f, 1.0f);
        
        // At 0% resonance: Q=0.5 (very gentle, essentially no resonance)
        // This gives clean, uncolored response
        // At 100%: Q=25 for self-oscillation
        float q = 0.5f + res * res * 24.5f;
        k = 1.0f / q;
        
        // Makeup gain only kicks in above ~20% resonance
        if (res > 0.2f) {
            resMakeupGain = 1.0f + (res - 0.2f) * (res - 0.2f) * 2.5f;
        } else {
            resMakeupGain = 1.0f;
        }
        
        // Bass compensation above 20% resonance - MORE AGGRESSIVE
        // Resonant filters lose bass, this compensates
        if (res > 0.2f) {
            resBassBoost = 1.0f + (res - 0.2f) * 2.5f;  // Much stronger
        } else {
            resBassBoost = 1.0f;
        }
    }
    
    void setBrightness(float bright) {
        brightness = clampf(bright, 0.1f, 2.0f);
    }
    
    float process(float input, float filterGate, float vcaGate) {
        // At very low resonance, blend toward bypass for clean tone
        float bypassMix = (resonance < 0.1f) ? (1.0f - resonance / 0.1f) * 0.5f : 0.0f;
        
        // Cutoff follows filter gate
        float minCutoff = 20.0f;
        float cutoff = minCutoff + filterGate * brightness * (maxCutoff - minCutoff);
        cutoff = clampf(cutoff, minCutoff, maxCutoff);
        
        // SVF coefficients
        float w = TWO_PI * cutoff / sampleRate;
        float g = tanf(w * 0.5f);
        g = clampf(g, 0.0001f, 0.9999f);
        
        // Two-pole SVF
        float hp = (input - (2.0f * k + g) * s1 - s2) / (1.0f + g * (g + 2.0f * k));
        float bp = g * hp + s1;
        float lp = g * bp + s2;
        
        // Update state with gentle saturation
        s1 = soft_saturate(g * hp + bp, 0.9f);
        s2 = soft_saturate(g * bp + lp, 0.9f);
        
        // Mix LP with input for gentler slope (~6dB/oct feel)
        float filtered = lp * 0.65f + input * vcaGate * 0.35f;
        
        // Bass boost at high resonance
        if (resBassBoost > 1.0f) {
            float bassContent = s2 * 0.3f;
            filtered += bassContent * (resBassBoost - 1.0f);
        }
        
        // Apply VCA
        float output = filtered * vcaGate;
        
        // Apply resonance makeup
        output *= resMakeupGain;
        
        // Blend toward clean bypass at very low resonance
        if (bypassMix > 0.0f) {
            float cleanPath = input * vcaGate;
            output = lerpf(output, cleanPath, bypassMix);
        }
        
        lastBP = bp;
        return output;
    }
    
    float getBandpass() const { return lastBP; }
    void reset() { s1 = s2 = lastBP = 0.0f; }
    
private:
    float sampleRate = 48000.0f;
    float maxCutoff = 20000.0f;
    float brightness = 1.0f;
    float resonance = 0.0f;
    float k = 2.0f;  // 1/Q, Q=0.5 at start
    float resMakeupGain = 1.0f;
    float resBassBoost = 1.0f;
    float s1 = 0.0f, s2 = 0.0f;
    float lastBP = 0.0f;
};

// ============================================================================
// FX PROCESSOR - Fixed volume drops
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
        
        // Scale amount: <30% subtle, 30-70% transitional, 70%+ full character
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
    
    // TUBE - Rich 12AX7 style saturation
    float processTube(float x, float gate, float amt, float& makeup) {
        // More drive, more saturation
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
        
        // Makeup gain - tube should be warm but present
        makeup = 1.4f + amt * 0.4f;
        return out;
    }
    
    // SCREAMER - Aggressive Tube Screamer overdrive
    float processScreamer(float x, float gate, float amt, float& makeup) {
        // Higher gain for more bite
        float gain = 6.0f + amt * 50.0f;  // More aggressive
        
        // Highpass - bass bypass
        float hp = x - screamerHP_z;
        screamerHP_z += screamerHPCoef * (x - screamerHP_z);
        
        // Mix back bass that bypasses the distortion
        float bassMix = 0.35f + (1.0f - amt) * 0.25f;
        float gained = hp * gain + x * bassMix;
        
        // Harder clipping for that Screamer bite
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
        
        // AGGRESSIVE makeup gain
        makeup = 1.6f + amt * 0.6f;
        return out;
    }
    
    // GRIT - Fuzz + Bit Crush with fixed volume
    float processGrit(float x, float bp, float gate, float amt, float& makeup) {
        float dry = x;
        
        // AGGRESSIVE drive - this is supposed to be GRITTY
        float fuzzDrive = 2.0f + amt * 15.0f;  // Much more drive
        float fuzzed = x * fuzzDrive;
        
        // Add some rectification for asymmetric harmonics
        float rectify = amt * 0.3f;
        fuzzed = fuzzed * (1.0f - rectify) + fabsf(fuzzed) * rectify;
        
        // Feedback for self-oscillation character
        fuzzed -= gritFeedback * amt * 0.4f;
        
        // DC bias for asymmetric clipping
        fuzzed += 0.15f * amt;
        
        // HARD asymmetric clipping - this is where the grit comes from
        if (fuzzed > 0.3f) {
            fuzzed = 0.3f + fast_tanh((fuzzed - 0.3f) * 3.0f) * 0.4f;
        } else if (fuzzed < -0.5f) {
            fuzzed = -0.5f + fast_tanh((fuzzed + 0.5f) * 2.0f) * 0.3f;
        }
        
        // Aggressive bit crush at higher amounts
        float crushed = fuzzed;
        if (amt > 0.3f) {
            float crushAmt = (amt - 0.3f) / 0.7f;
            float bits = 10.0f - crushAmt * 7.0f;  // 10-bit down to 3-bit!
            float levels = powf(2.0f, bits);
            crushed = floorf(fuzzed * levels + 0.5f) / levels;
            
            // Sample rate reduction for that lo-fi crunch
            if (amt > 0.5f) {
                float srReduce = 1.0f + (amt - 0.5f) * 12.0f;  // More aggressive
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
        
        // Light lowpass to tame harsh aliasing
        gritLP_z += gritLPCoef * (crushed - gritLP_z);
        float out = gritLP_z;
        
        // Keep some dry signal for bass integrity
        float dryMix = 0.15f * (1.0f - amt * 0.5f);  // Less dry at high amounts
        out = out * (1.0f - dryMix) + dry * dryMix;
        
        // AGGRESSIVE makeup gain to maintain volume
        makeup = 1.8f + amt * 0.8f;  // Gets louder as grit increases
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
// TRIGGER DETECTOR
// ============================================================================

class TriggerDetector {
public:
    void setThreshold(float v) { threshold = clampf(v, 0.01f, 5.0f); }
    
    bool process(float input) {
        bool high = input > threshold;
        bool trig = high && !wasHigh;
        if (trig) lastLevel = input;
        wasHigh = high;
        return trig;
    }
    
    float getLastLevel() const { return lastLevel; }
    void reset() { wasHigh = false; lastLevel = 0.0f; }
    
private:
    float threshold = 0.1f;
    bool wasHigh = false;
    float lastLevel = 0.0f;
};

// ============================================================================
// LPG CHANNEL
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
        // OPEN = ceiling (how much the gate CAN open)
        // Not a floor! At Open 0%, gate barely opens. At Open 100%, gate fully opens.
        this->openCeiling = openParam;
        this->dampening = dampening;
        this->material = material;
        this->inputGain = inputGain;
        this->hitMemoryOn = hitMemory;
        
        // DECAY parameter: How long the envelope takes to decay back to ZERO
        // Need musical tone starting earlier - not just clicks in 0-30%
        //
        // New scaling - more tone earlier:
        // 0-5%: Clicks (5-15ms) - very short, barely tone
        // 5-15%: Short with tone (15-40ms) - you hear the note
        // 15-30%: Plucky (40-100ms) - clear LPG character
        // 30-50%: Medium (100-200ms) - good for 120bpm
        // 50-70%: Long (200-500ms) - notes overlap
        // 70-85%: Very long (500ms-1.5s) - sustained
        // 85-100%: Drone (1.5s-5s)
        
        float baseDecayMs;
        if (decayParam < 0.05f) {
            // 0-5%: Clicks (5-15ms)
            float t = decayParam / 0.05f;
            baseDecayMs = 5.0f + t * 10.0f;
        } else if (decayParam < 0.15f) {
            // 5-15%: Short with tone (15-40ms)
            float t = (decayParam - 0.05f) / 0.10f;
            baseDecayMs = 15.0f + t * 25.0f;
        } else if (decayParam < 0.30f) {
            // 15-30%: Plucky (40-100ms)
            float t = (decayParam - 0.15f) / 0.15f;
            baseDecayMs = 40.0f + t * 60.0f;
        } else if (decayParam < 0.50f) {
            // 30-50%: Medium (100-200ms)
            float t = (decayParam - 0.30f) / 0.20f;
            baseDecayMs = 100.0f + t * 100.0f;
        } else if (decayParam < 0.70f) {
            // 50-70%: Long (200-500ms)
            float t = (decayParam - 0.50f) / 0.20f;
            baseDecayMs = 200.0f + t * 300.0f;
        } else if (decayParam < 0.85f) {
            // 70-85%: Very long (500ms-1.5s)
            float t = (decayParam - 0.70f) / 0.15f;
            baseDecayMs = 500.0f + t * 1000.0f;
        } else {
            // 85-100%: Drone (1.5s-5s)
            float t = (decayParam - 0.85f) / 0.15f;
            baseDecayMs = 1500.0f + t * 3500.0f;
        }
        
        // Apply material multiplier to decay
        float vcaDecayMs = baseDecayMs * kMaterialDecayMult[material];
        
        // Apply dampening (shortens decay further)
        vcaDecayMs *= (1.0f - dampening * 0.6f);
        
        // Calculate decay coefficient for per-sample exponential decay
        // coefficient = e^(-1 / (tau * sampleRate))
        // tau = time constant in seconds
        // For envelope to decay to ~5% in vcaDecayMs, tau ≈ vcaDecayMs / 3
        // But we want the FULL decay time, not 1/3 of it
        float tauSamples = vcaDecayMs * 0.001f * sampleRate;
        if (tauSamples > 0.0f) {
            decayCoefficient = expf(-1.0f / tauSamples);
        } else {
            decayCoefficient = 0.0f;  // Instant decay
        }
        
        // Attack coefficient - fast but not instant to avoid scratchy clicks
        // Attack time scales with decay: shorter decay = faster attack
        // But always at least 0.5ms to smooth the transient
        float attackMs = fmaxf(0.5f, vcaDecayMs * 0.02f);  // 2% of decay time, min 0.5ms
        attackMs = fminf(attackMs, 5.0f);  // Max 5ms attack
        float attackTauSamples = attackMs * 0.001f * sampleRate;
        attackCoefficient = 1.0f - expf(-1.0f / attackTauSamples);
        
        // Filter setup
        filter.setResonance(resonance);
        filter.setBrightness(kMaterialBrightness[material] * (1.0f - dampening * 0.4f));
        
        // FX
        fx.setMode(fxMode);
        fx.setAmount(fxAmount);
    }
    
    void trigger(float velocity = 1.0f) {
        // Trigger opens gate to (velocity * openCeiling)
        // Set target level - the envelope will attack toward this
        
        float targetLevel = velocity * openCeiling;
        
        if (hitMemoryOn) {
            // Memory mode: ADD to current envelope level
            targetLevel = clampf(envelopeState + targetLevel, 0.0f, 1.2f);
        }
        
        // Set the TARGET, not the state directly
        // This allows for a tiny attack time to avoid scratchy clicks
        envelopeTarget = targetLevel;
        filterTarget = targetLevel;
        
        triggerVisual = 1.0f;
    }
    
    float process(float input) {
        // Envelope attacks toward target, then decays toward zero
        // The tiny attack time smooths the click into the tone
        
        // Attack toward target (fast but not instant)
        if (envelopeTarget > envelopeState) {
            envelopeState += (envelopeTarget - envelopeState) * attackCoefficient;
            // Once we've reached the target, switch to decay
            if (envelopeState >= envelopeTarget * 0.99f) {
                envelopeState = envelopeTarget;
                envelopeTarget = 0.0f;  // Now decay toward zero
            }
        } else {
            // Decay toward zero
            envelopeState *= decayCoefficient;
        }
        
        // Filter has same attack but faster decay
        if (filterTarget > filterState) {
            filterState += (filterTarget - filterState) * attackCoefficient;
            if (filterState >= filterTarget * 0.99f) {
                filterState = filterTarget;
                filterTarget = 0.0f;
            }
        } else {
            // Filter decays faster
            float filterRatio = kMaterialFilterDecayRatio[material];
            filterState *= powf(decayCoefficient, filterRatio);
        }
        
        // Clamp to zero
        if (envelopeState < 0.0001f) envelopeState = 0.0f;
        if (filterState < 0.0001f) filterState = 0.0f;
        
        float vcaGate = envelopeState;
        float filterGate = filterState;
        
        lastGate = vcaGate;
        
        input *= inputGain;
        
        float filtered = filter.process(input, filterGate, vcaGate);
        
        float bp = filter.getBandpass();
        float processed = fx.process(filtered, bp, vcaGate);
        
        processed = dcBlocker.process(processed);
        
        triggerVisual *= 0.96f;
        
        return processed;
    }
    
    float getGateValue() const { return lastGate; }
    float getTriggerVisual() const { return triggerVisual; }
    
    void reset() {
        filter.reset();
        fx.reset();
        dcBlocker.reset();
        envelopeState = 0.0f;
        envelopeTarget = 0.0f;
        filterState = 0.0f;
        filterTarget = 0.0f;
        triggerVisual = 0.0f;
        lastGate = 0.0f;
    }
    
private:
    float sampleRate = 48000.0f;
    float openCeiling = 1.0f;  // CEILING - how much gate CAN open
    float dampening = 0.0f;
    float inputGain = 1.0f;
    MaterialMode material = MATERIAL_NATURAL;
    bool hitMemoryOn = false;
    
    float envelopeState = 0.0f;     // Current envelope level
    float envelopeTarget = 0.0f;    // Target for attack phase
    float filterState = 0.0f;       // Filter envelope (decays faster)
    float filterTarget = 0.0f;      // Filter target for attack
    float decayCoefficient = 0.99f; // Per-sample decay multiplier
    float attackCoefficient = 0.1f; // Per-sample attack multiplier
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
// PARAMETERS
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

static const char* const materialStrings[] = { "Natural", "Hard", "Soft" };
static const char* const fxStrings[] = { "Clean", "Tube", "Screamer", "Grit" };
static const char* const stereoStrings[] = { "Mono", "Stereo" };
static const char* const onOffStrings[] = { "Off", "On" };

static const _NT_parameter parameters[] = {
    { .name = "Resonance", .min = 0, .max = 100, .def = 0, .unit = kNT_unitPercent, .scaling = kNT_scalingNone, .enumStrings = NULL },
    { .name = "Decay", .min = 0, .max = 100, .def = 50, .unit = kNT_unitPercent, .scaling = kNT_scalingNone, .enumStrings = NULL },
    { .name = "Open", .min = 0, .max = 100, .def = 100, .unit = kNT_unitPercent, .scaling = kNT_scalingNone, .enumStrings = NULL },
    { .name = "Dampening", .min = 0, .max = 100, .def = 0, .unit = kNT_unitPercent, .scaling = kNT_scalingNone, .enumStrings = NULL },
    { .name = "Material", .min = 0, .max = 2, .def = 0, .unit = kNT_unitEnum, .scaling = kNT_scalingNone, .enumStrings = materialStrings },
    { .name = "FX", .min = 0, .max = 3, .def = 0, .unit = kNT_unitEnum, .scaling = kNT_scalingNone, .enumStrings = fxStrings },
    { .name = "FX Amount", .min = 0, .max = 100, .def = 0, .unit = kNT_unitPercent, .scaling = kNT_scalingNone, .enumStrings = NULL },
    { .name = "Gain", .min = 0, .max = 106, .def = 100, .unit = kNT_unitNone, .scaling = kNT_scalingNone, .enumStrings = NULL },
    { .name = "Hit Memory", .min = 0, .max = 1, .def = 0, .unit = kNT_unitEnum, .scaling = kNT_scalingNone, .enumStrings = onOffStrings },
    
    { .name = "Resonance CV", .min = 0, .max = 28, .def = 0, .unit = kNT_unitCvInput, .scaling = kNT_scalingNone, .enumStrings = NULL },
    { .name = "Decay CV", .min = 0, .max = 28, .def = 0, .unit = kNT_unitCvInput, .scaling = kNT_scalingNone, .enumStrings = NULL },
    { .name = "Open CV", .min = 0, .max = 28, .def = 0, .unit = kNT_unitCvInput, .scaling = kNT_scalingNone, .enumStrings = NULL },
    { .name = "Dampening CV", .min = 0, .max = 28, .def = 0, .unit = kNT_unitCvInput, .scaling = kNT_scalingNone, .enumStrings = NULL },
    { .name = "FX Amt CV", .min = 0, .max = 28, .def = 0, .unit = kNT_unitCvInput, .scaling = kNT_scalingNone, .enumStrings = NULL },
    
    NT_PARAMETER_CV_INPUT( "Trigger Input", 0, 3 )
    { .name = "Trig Threshold", .min = 10, .max = 500, .def = 100, .unit = kNT_unitMillivolts, .scaling = kNT_scalingNone, .enumStrings = NULL },
    { .name = "Stereo", .min = 0, .max = 1, .def = 1, .unit = kNT_unitEnum, .scaling = kNT_scalingNone, .enumStrings = stereoStrings },
    NT_PARAMETER_AUDIO_INPUT( "Left Input", 1, 1 )
    NT_PARAMETER_AUDIO_INPUT( "Right Input", 1, 2 )
    NT_PARAMETER_AUDIO_OUTPUT_WITH_MODE( "Left Output", 1, 13 )
    NT_PARAMETER_AUDIO_OUTPUT_WITH_MODE( "Right Output", 1, 14 )
    { .name = "Env Follower", .min = 0, .max = 1, .def = 0, .unit = kNT_unitEnum, .scaling = kNT_scalingNone, .enumStrings = onOffStrings },
    NT_PARAMETER_CV_OUTPUT( "Env Output", 0, 0 )
};

static const uint8_t page1[] = { kParamResonance, kParamDecay, kParamOpen, kParamDampening, kParamMaterial, kParamFX, kParamFXAmount, kParamGain, kParamHitMemory };
static const uint8_t page2[] = { kParamResonanceCV, kParamDecayCV, kParamOpenCV, kParamDampeningCV, kParamFXAmountCV };
static const uint8_t page3[] = { kParamTriggerInput, kParamTriggerThreshold, kParamStereo, kParamLeftInput, kParamRightInput, kParamLeftOutput, kParamLeftOutputMode, kParamRightOutput, kParamRightOutputMode, kParamEnvFollower, kParamEnvOutput };

static const _NT_parameterPage pages[] = {
    { .name = "Holy Mackerel", .numParams = ARRAY_SIZE(page1), .params = page1 },
    { .name = "CV Control", .numParams = ARRAY_SIZE(page2), .params = page2 },
    { .name = "Routing", .numParams = ARRAY_SIZE(page3), .params = page3 },
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
        return 1.0f + (g - 100) / 6.0f;
    }
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
    
    alg->hitIntensity = 0.0f;
    alg->hitPhase = 0.0f;
    
    return alg;
}

void updateGrayed(_holyMackerelAlgorithm* alg) {
    int idx = NT_algorithmIndex(alg);
    int off = NT_parameterOffset();
    bool mono = (alg->v[kParamStereo] == 0);
    bool clean = (alg->v[kParamFX] == FX_CLEAN);
    bool envOff = (alg->v[kParamEnvFollower] == 0);
    
    NT_setParameterGrayedOut(idx, kParamRightInput + off, mono);
    NT_setParameterGrayedOut(idx, kParamRightOutput + off, mono);
    NT_setParameterGrayedOut(idx, kParamRightOutputMode + off, mono);
    NT_setParameterGrayedOut(idx, kParamFXAmount + off, clean);
    NT_setParameterGrayedOut(idx, kParamFXAmountCV + off, clean);
    NT_setParameterGrayedOut(idx, kParamEnvOutput + off, envOff);
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
        alg->trigger.setThreshold(alg->v[kParamTriggerThreshold] / 1000.0f);
    }
    
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
            float vel = clampf(alg->trigger.getLastLevel() / 5.0f, 0.5f, 1.0f);
            alg->channelL.trigger(vel);
            if (stereo) alg->channelR.trigger(vel);
            
            alg->hitIntensity = vel;
            alg->hitPhase = 0.0f;
        }
        
        if (resCV || decCV || openCV || dampCV || fxCV) {
            float r = baseRes, d = baseDec, o = baseOpen, dp = baseDamp, f = baseFX;
            if (resCV) r = clampf(baseRes + resCV[i] * 0.1f, 0.0f, 1.0f);
            if (decCV) d = clampf(baseDec + decCV[i] * 0.1f, 0.0f, 1.0f);
            if (openCV) o = clampf(baseOpen + openCV[i] * 0.1f, 0.0f, 1.0f);
            if (dampCV) dp = clampf(baseDamp + dampCV[i] * 0.1f, 0.0f, 1.0f);
            if (fxCV) f = clampf(baseFX + fxCV[i] * 0.1f, 0.0f, 1.0f);
            
            alg->channelL.setParams(r, d, o, dp, material, fxMode, f, gain, hitMemory);
            if (stereo) alg->channelR.setParams(r, d, o, dp, material, fxMode, f, gain, hitMemory);
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
}

// ============================================================================
// UI
// ============================================================================

bool draw(_NT_algorithm* self) {
    _holyMackerelAlgorithm* alg = (_holyMackerelAlgorithm*)self;
    
    const int yOffset = 6;
    
    const int faderX = 4;
    const int faderSpacing = 18;  // Tighter spacing for 5 faders
    const int faderTopY = 14 + yOffset;
    const int faderBottomY = 48 + yOffset;
    const int faderHeight = faderBottomY - faderTopY;
    const int faderWidth = 6;  // Slightly narrower
    
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
    
    const char* matStr[] = { "NAT", "HRD", "SFT" };
    const char* fxStr[] = { "CLN", "TUB", "SCR", "GRT" };
    NT_drawText(32, faderTopY - 8, matStr[alg->v[kParamMaterial]], 5, kNT_textCentre, kNT_textTiny);
    NT_drawText(68, faderTopY - 8, fxStr[alg->v[kParamFX]], 5, kNT_textCentre, kNT_textTiny);
    
    char gainBuf[8];
    int gainVal = alg->v[kParamGain];
    if (gainVal <= 100) snprintf(gainBuf, sizeof(gainBuf), "%d", gainVal);
    else snprintf(gainBuf, sizeof(gainBuf), "+%d", gainVal - 100);
    NT_drawText(95, faderTopY - 8, gainBuf, 5, kNT_textCentre, kNT_textTiny);
    
    // Hit Memory indicator
    if (alg->v[kParamHitMemory] == 1) {
        NT_drawText(95, faderBottomY + 6, "MEM", 12, kNT_textCentre, kNT_textTiny);
    }
    
    const int hitCenterX = 175;
    const int hitCenterY = 32 + yOffset;
    const int maxYRadius = 25;  // min(hitCenterY, 63 - hitCenterY) - 1px margin

    float gateL = alg->channelL.getGateValue();
    float gateR = (alg->v[kParamStereo] == 1) ? alg->channelR.getGateValue() : gateL;
    float gate = (gateL + gateR) * 0.5f;

    float hitVis = alg->hitIntensity * expf(-alg->hitPhase * 0.4f);

    int numRays = 16;
    float baseRadius = 8.0f + gate * 15.0f;
    float burstRadius = baseRadius + hitVis * 20.0f;
    if (burstRadius > (float)maxYRadius - 8.0f) burstRadius = (float)maxYRadius - 8.0f;

    if (hitVis > 0.1f) {
        int glowR = (int)(burstRadius + 5 + hitVis * 6);
        if (glowR > maxYRadius) glowR = maxYRadius;
        NT_drawShapeI(kNT_circle, hitCenterX - glowR, hitCenterY - glowR,
                      hitCenterX + glowR, hitCenterY + glowR, 4 + (int)(hitVis * 3));
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

        NT_drawShapeI(kNT_line, x1, y1, x2, y2, 8 + (int)(gate * 5) + (int)(hitVis * 2));
    }

    if (hitVis > 0.05f) {
        int ringR = (int)(burstRadius * 0.7f + hitVis * 10);
        if (ringR > maxYRadius) ringR = maxYRadius;
        NT_drawShapeI(kNT_circle, hitCenterX - ringR, hitCenterY - ringR,
                      hitCenterX + ringR, hitCenterY + ringR, 7 + (int)(hitVis * 4));
    }

    int centerR = 4 + (int)(gate * 6);
    NT_drawShapeI(kNT_rectangle, hitCenterX - centerR, hitCenterY - centerR,
                  hitCenterX + centerR, hitCenterY + centerR, 15);

    int boundaryR = (int)burstRadius + 8;
    if (boundaryR > maxYRadius) boundaryR = maxYRadius;
    NT_drawShapeI(kNT_circle, hitCenterX - boundaryR, hitCenterY - boundaryR,
                  hitCenterX + boundaryR, hitCenterY + boundaryR, 5);

    NT_drawText(250, 8, "HOLY", 7, kNT_textRight, kNT_textTiny);
    NT_drawText(250, 16, "MACKEREL", 7, kNT_textRight, kNT_textTiny);

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
    .description = "Vactrol-less Low Pass Gate with Hate",
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
