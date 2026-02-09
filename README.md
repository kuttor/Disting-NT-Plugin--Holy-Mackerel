# Holy Mackerel
### A Vactrol-Modeled Stereo Low Pass Gate for Expert Sleepers Disting NT
---

```
    ██╗  ██╗ ██████╗ ██╗  ██╗   ██╗    ███╗   ███╗ █████╗  ██████╗██╗  ██╗███████╗██████╗ ███████╗██╗     
    ██║  ██║██╔═══██╗██║  ╚██╗ ██╔╝    ████╗ ████║██╔══██╗██╔════╝██║ ██╔╝██╔════╝██╔══██╗██╔════╝██║     
    ███████║██║   ██║██║   ╚████╔╝     ██╔████╔██║███████║██║     █████╔╝ █████╗  ██████╔╝█████╗  ██║     
    ██╔══██║██║   ██║██║    ╚██╔╝      ██║╚██╔╝██║██╔══██║██║     ██╔═██╗ ██╔══╝  ██╔══██╗██╔══╝  ██║     
    ██║  ██║╚██████╔╝███████╗██║       ██║ ╚═╝ ██║██║  ██║╚██████╗██║  ██╗███████╗██║  ██║███████╗███████╗
    ╚═╝  ╚═╝ ╚═════╝ ╚══════╝╚═╝       ╚═╝     ╚═╝╚═╝  ╚═╝ ╚═════╝╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝╚══════╝╚══════╝

                                 🐟 Vactrol-less LPG with Hate 🐟

## Overview

**Holy Mackerel** is a research-driven Low Pass Gate that brings the organic, percussive character of acoustic instruments to your modular synthesizer — without a single vactrol in sight.

Inspired by the Rabid Elephant Natural Gate, SSF Steady State Gate, and Buchla 292, Holy Mackerel models the vactrol's photoresistive behavior digitally using a single continuous envelope derived from Parker & D'Angelo's DAFX-13 research on vactrol dynamics. The result is authentic "struck object" character — the plucky snap of wood, the ring of metal, the thud of rubber — with features no hardware LPG can offer.

Three material models capture different acoustic behaviors:

* **Natural** — Classic LPG character: wood, bongos, organic percussion
* **Hard** — Metallic ring and shimmer: bells, cymbals, glass
* **Soft** — Dampened thud and absorption: rubber, felt mallets, muted drums

Four saturation modes add harmonic character from warm tube glow to lo-fi destruction.

---

## Features

### 🎛️ Single Vactrol Envelope Model

Unlike dual-envelope approaches, Holy Mackerel uses a single photoresistive state that controls both the filter and VCA simultaneously — just like a real Buchla 292. The filter and VCA separate naturally through nonlinear transfer curves, not independent envelopes.

* **Level-Dependent Decay** — Higher levels decay faster, lower levels linger (carrier recombination physics)
* **Continuous Curve** — No stage boundaries, no crossfade artifacts, one smooth contour
* **Velocity Shaping** — Harder hits have sharper transients, soft hits are gentle throughout

### 🔨 Three Material Modes

| Material | Character | Decay | Brightness | Best For |
| --- | --- | --- | --- | --- |
| **Natural** | Balanced, organic | Baseline | Full range | Bongos, wood blocks, general percussion |
| **Hard** | Ringing, shimmery | 1.4× longer | Bright, highs sustained | Bells, metallic hits, glass, cymbals |
| **Soft** | Absorptive, thuddy | 0.7× shorter | Dark, muted | Felt mallets, rubber, muted drums |

Hard materials **ring** (think cymbal). Soft materials **absorb** (think cardboard box). This mirrors real acoustic physics — harder materials sustain vibration longer with more high-frequency content.

### 🖐️ Authentic Dampening

Dampening works like placing your hand on a drum head — it doesn't shorten the decay, it mutes the tone:

* **Brightness** reduced up to 85% (filter closes toward minimum)
* **Resonance** reduced up to 40% (dampened objects don't ring)
* **VCA Ceiling** reduced up to 75% (quieter output, same decay shape)

The envelope shape stays identical. You hear less of the sound, not a shorter sound.

### 💥 Hit Memory

When enabled, triggers accumulate energy rather than restarting the envelope — recreating the Natural Gate's "Memory" feature:

* Rapid triggers **build** brightness and volume (vactrol state stacks)
* Accumulated energy **extends decay** up to 40% (warm vactrol stays open longer)
* Creates natural crescendos from sequenced patterns without automation

### 🎭 Four FX Modes

| Mode | Character | Best For |
| --- | --- | --- |
| **Clean** | No processing — pure LPG | Natural acoustic sounds |
| **Tube** | Warm 12AX7 saturation with even harmonics | Warming cold digital sources |
| **Screamer** | Aggressive Tube Screamer overdrive with mid boost | Punchy bass, aggressive leads |
| **Grit** | Fuzz + bit crush + sample rate reduction | Industrial, noise, lo-fi texture |

FX Amount uses a scaled curve: subtle below 30%, transitional 30–70%, full character above 70%.

### 🎯 Schmitt Trigger Detection

Professional-grade trigger input with:

* **Hysteresis** — Separate rising/falling thresholds (70% ratio) prevent noise retriggering
* **Rearm Guard** — Signal must be low for 16+ consecutive samples before accepting next trigger
* **15ms Lockout** — Covers even long Eurorack trigger pulses
* **Adjustable Threshold** — 10mV to 500mV

---

## Installation

1. Copy `holyMackerel.instruments` to your Disting NT SD card:

   ```
   /plugins/holyMackerel.instruments
   ```
2. Power cycle your Disting NT
3. Navigate to **Instruments → Holy Mackerel**

---

## Parameters

### Holy Mackerel Page (Sound)

| Parameter | Range | Default | Description |
| --- | --- | --- | --- |
| **Resonance** | 0–100% | 0% | Filter resonance. Low = warm, natural. High = ringy, wild, self-oscillation territory |
| **Decay** | 0–100% | 50% | How long the gate stays open after trigger |
| **Open** | 0–100% | 100% | How much the gate opens (ceiling). Controls both volume AND brightness |
| **Dampening** | 0–100% | 0% | Mutes the sound — like a hand on a drum head |
| **Material** | Natural / Hard / Soft | Natural | Character of the "struck object" |
| **FX** | Clean / Tube / Screamer / Grit | Clean | Saturation and distortion mode |
| **FX Amount** | 0–100% | 0% | Intensity of the selected effect |
| **Gain** | 0–106 | 100 | Input gain (100 = unity, 101–106 = +1dB steps) |
| **Hit Memory** | Off / On | Off | Triggers accumulate energy (see above) |

### CV Control Page

| Parameter | Range | Description |
| --- | --- | --- |
| **Resonance CV** | Bus 1–6 / Off | CV modulation of resonance (±5V) |
| **Decay CV** | Bus 1–6 / Off | CV modulation of decay time |
| **Open CV** | Bus 1–6 / Off | CV modulation of gate ceiling |
| **Dampening CV** | Bus 1–6 / Off | CV modulation of dampening amount |
| **FX Amount CV** | Bus 1–6 / Off | CV modulation of FX intensity |

All CV inputs accept ±5V and are additive to the base parameter value.

### Routing Page

| Parameter | Range | Description |
| --- | --- | --- |
| **Trigger** | Bus 1–6 / Off | CV trigger input |
| **Trig Threshold** | 10–500 mV | Trigger detection threshold |
| **Stereo** | Mono / Stereo | Mono or stereo processing |
| **Left Input** | Bus 1–6 | Audio input (mono or stereo left) |
| **Right Input** | Bus 1–6 | Stereo right audio input |
| **Left Output** | Bus 1–6 | Processed audio output |
| **L Out Mode** | Add / Replace | Mix with bus or overwrite |
| **Right Output** | Bus 1–6 | Stereo right output |
| **R Out Mode** | Add / Replace | Mix with bus or overwrite |
| **Env Follower** | Off / On | Enable envelope output |
| **Env Output** | Bus 1–6 / Off | Envelope follower output (0–5V) |

---

## Understanding Open vs Decay

These two parameters work together to shape your sound:

**OPEN** = How much the gate opens (ceiling)
- Controls the MAXIMUM level the gate reaches
- Affects both volume AND filter brightness
- At 0%: Gate barely opens (very quiet, heavily filtered)
- At 100%: Gate fully opens (full volume and brightness)

**DECAY** = How long the gate takes to close
- Controls the time from trigger to silence
- The vactrol model's level-dependent decay means the initial transient always decays faster than the tail

| Decay Range | Time | Character |
| --- | --- | --- |
| 0–5% | 5–15ms | Clicks, rim shots |
| 5–15% | 15–40ms | Ticks, short plucks |
| 15–30% | 40–100ms | Snappy percussion |
| 30–50% | 100–200ms | Plucky, tom-like |
| 50–70% | 200–500ms | Sustained percussion |
| 70–85% | 500ms–1.5s | Long notes, pads |
| 85–100% | 1.5–5s | Drones |

### Musical Presets

| Open | Decay | Material | Result |
| --- | --- | --- | --- |
| 100% | 10% | Natural | Bright, snappy pluck |
| 100% | 40% | Natural | Full tom-like hit |
| 50% | 30% | Soft | Dark, muted thud |
| 100% | 80% | Hard | Sustained metallic ring |
| 30% | 15% | Soft | Dark, muted clicks |
| 100% | 50% | Hard | Bell-like shimmer |

---

## FX Mode Details

### Tube — Rich 12AX7 Saturation

Modeled after triode tube characteristics:

* **Asymmetric Soft Clipping** — Positive rail clips softer (triode character)
* **Even Harmonic Enhancement** — 2nd harmonic warmth via signal rectification
* **Grid Blocking Compression** — At high levels, simulates grid current blocking
* **Gate-Responsive Drive** — Drive intensity follows envelope level

### Screamer — Tube Screamer Overdrive

Classic overdrive topology:

* **Bass Bypass** — Low frequencies pass clean under the distortion
* **720Hz Highpass** — Focuses distortion on mids and highs
* **Hard Clip with Tanh Softening** — Bite without digital harshness
* **Signature Mid Boost** — The Screamer sound

### Grit — Lo-Fi Destruction

Multi-stage degradation:

* **Fuzz** — Asymmetric clipping with DC bias (2–17× drive)
* **Rectification** — Half-wave rectification for odd harmonics
* **Bit Crushing** — 10-bit down to 3-bit at high amounts
* **Sample Rate Reduction** — Aliasing and staircase artifacts
* **Feedback** — Self-oscillation character at extreme settings

---

## Signal Flow

```
                                      TRIGGER INPUT
                                           │
                                    ┌──────┴──────┐
                                    │   SCHMITT   │
                                    │  TRIGGER    │
                                    │ DETECTOR    │
                                    │             │
                                    │ • Hysteresis│
                                    │ • Rearm     │
                                    │ • 15ms lock │
                                    └──────┬──────┘
                                           │
                                      velocity
                                           │
                           ┌───────────────┴───────────────┐
                           │     SINGLE VACTROL MODEL      │
                           │                               │
                           │  vactrolState (0.0 → 1.2)     │
                           │         │                     │
                           │  Level-Dependent Decay:       │
                           │  speed = 1 + state² × mod     │
                           │         │                     │
                           │    ┌────┴────┐                │
                           │    │         │                │
                           │  pow(s,exp) sqrt(s)           │
                           │    │         │                │
                           │ filterGate  vcaGate           │
                           │ (fast drop) (slow drop)       │
                           └────┬─────────┬────────────────┘
                                │         │
                                │         │ × dampeningVCA
  AUDIO         ┌───────┐      │         │
  INPUT ────────┤ INPUT ├──────┤         │
                │ GAIN  │      │         │
                └───────┘      │         │
                               │         │
                          ┌────┴─────────┴────┐
                          │     SVF FILTER     │
                          │                    │
                          │  cutoff ← filterGate × brightness
                          │  Q ← resonance × dampeningResCut
                          │                    │
                          │  LP + BP × bpMix   │
                          │       │            │
                          │       × vcaGate    │
                          │       │            │
                          │    × resMakeup     │
                          └────────┬───────────┘
                                   │
                              ┌────┴────┐
                              │   FX    │
                              │         │
                              │ Tube    │
                              │ Screamer│
                              │ Grit    │
                              └────┬────┘
                                   │
                              ┌────┴────┐
                              │   DC    │
                              │  BLOCK  │
                              └────┬────┘
                                   │
                              ┌────┴────┐
                              │  SOFT   │
                              │  CLIP   │
                              └────┬────┘
                                   │
                              OUTPUT L/R


               ┌──────────────────────────────────────────┐
               │         CV MODULATION (±5V)              │
               │                                          │
               │  Resonance CV ──→ resonance              │
               │  Decay CV ──────→ decay time             │
               │  Open CV ───────→ gate ceiling           │
               │  Dampening CV ──→ dampening amount       │
               │  FX Amount CV ──→ FX intensity           │
               │                                          │
               │  Updated every 32 samples (~1.5kHz)      │
               └──────────────────────────────────────────┘
```

---

## Vactrol Model Details

### The Physics

In a real Buchla 292, a single vactrol (LED + photoresistor) controls both the filter cutoff and VCA gain simultaneously. The photoresistor's response is inherently nonlinear — it illuminates quickly but decays slowly, with the decay rate dependent on the illumination level.

Holy Mackerel models this with three components:

**1. Level-Dependent Decay**

Based on Parker & D'Angelo, DAFX-13:

```
speedFactor = 1.0 + vactrolState² × vactrolDecayMod
```

Higher vactrol illumination → faster carrier recombination → faster initial decay. This single continuous curve naturally produces the "thwack → body" contour that makes LPGs sound like struck objects.

**2. Nonlinear Transfer Curves**

One vactrol state, two derived signals:

```
filterGate = pow(vactrolState, filterExponent)   // drops fast
vcaGate    = sqrt(vactrolState)                   // holds open
```

The filter closes before the VCA — not because of separate envelopes, but because of the mathematical relationship between resistance and the two circuits it controls. This is the "pluck" that defines the LPG sound.

**3. Material-Dependent Constants**

| Material | Decay Mult | Vactrol Mod | Filter Exp | Character |
| --- | --- | --- | --- | --- |
| Natural | 1.0× | 2.5 | 1.8 | Balanced thwack + body |
| Hard | 1.4× | 1.2 | 1.2 | Even ring, bright sustain |
| Soft | 0.7× | 4.0 | 2.8 | Fast thwack, quick darkening |

---

## Typical Patches

### Basic Percussion
1. Send oscillator to audio input
2. Send trigger/gate to trigger input
3. Set Decay to 30–50%
4. Set Open to 100%
5. Adjust Material to taste

### Acoustic Tom
1. Sine or triangle wave input
2. Decay ~40%, Open 100%
3. Material: Natural
4. Resonance: 5–15%
5. Slight Tube saturation

### Metallic Bell
1. Complex waveform input (saw, FM, or noise)
2. Decay ~60%, Open 90%
3. Material: Hard
4. Resonance: 30–50%
5. FX: Clean or light Tube

### Plucked String
1. Sawtooth input
2. Decay ~20%, Open 80%
3. Material: Natural
4. Resonance: 10–20%

### Muted Percussion
1. Any waveform input
2. Decay ~40%, Open 60%
3. Material: Soft
4. Dampening: 30–60%

### Hand-Dampened Cymbal
1. Noise or metallic source
2. Decay ~50%, Open 100%
3. Material: Hard
4. Dampening: 40–70% (adjust in real-time for hand-on-cymbal effect)

### Lo-Fi Drums
1. Any input through external drums or oscillator
2. Decay to taste
3. FX: Grit, Amount 50–80%
4. Adds crunch and character

### Building Crescendo (Hit Memory)
1. Enable Hit Memory
2. Send rapid triggers (16th notes at ~120 BPM)
3. Each hit accumulates — brightness and volume build
4. Decay extends with accumulated energy
5. Stop triggering and hear the long warm tail

### Auto-Wah Effect
1. Enable Env Follower
2. Route Env Output to external filter CV
3. Feed audio through Holy Mackerel
4. Gate responds to trigger dynamics
5. Envelope CV drives external processing

---

## Tips & Tricks

### Getting Natural Sounds
- Keep Resonance low (0–20%)
- Use Material modes rather than heavy FX
- Let Decay breathe — don't make everything a click
- Dampening adds realism without shortening the sound

### Maximum Resonance
- Resonance at 100% enters self-oscillation territory
- Static makeup gain keeps bass and volume present
- Bandpass mix brings in the resonant peak character
- Combine with Material: Hard for wild metallic ringing

### Dampening as Performance Control
- Map Dampening to a CV input
- Modulate with an LFO for rhythmic muting
- Use a foot controller for real-time hand-dampening
- At 100%: heavily muted but same decay — like choking a cymbal

### CV Modulation Ideas
- Modulate Decay with an LFO for evolving textures
- Use velocity CV to control Open for dynamics
- Sequence Material changes via parameter locks
- Dampening CV from an envelope for auto-mute effects

### Preventing Clicks
- Avoid 0% Decay unless you want clicks
- Use Dampening to soften transients (doesn't shorten decay)
- Soft Material has the gentlest attack (~20ms)

---

## Display

The custom UI provides real-time feedback:

```
┌───────────────────────────────────────────────┐
│  [FADERS]                        HOLY         │
│  ▓▓  ▓▓  ▓▓  ▓▓  ▓▓            MACKEREL      │
│  ▓▓  ▓▓  ▓▓  ▓▓  ▓▓            v7.2.0        │
│  ▓▓  ▓▓  ▓▓  ░░  ░░                          │
│  ░░  ░░  ░░  ░░  ░░    ┌────────────────┐    │
│  R   D   O   Dp  Fx    │ ████░░░░  0.72 │    │
│                         │  (gate meter)  │    │
│  NAT  CLN  0dB  MEM    └────────────────┘    │
└───────────────────────────────────────────────┘
```

* **Faders**: Resonance, Decay, Open, Dampening, FX Amount (real-time values)
* **Labels**: Material, FX Mode, Gain, Hit Memory status
* **Gate Meter**: Visual gate level with numeric readout
* **Hit Flash**: Animated trigger indicator on each hit

---

## Technical Specifications

| Specification | Value |
| --- | --- |
| Platform | Expert Sleepers Disting NT |
| Processor | ARM Cortex-M7 |
| Sample Rate | Follows system (48kHz typical) |
| Latency | Zero (direct synthesis) |
| Filter Topology | 2-pole State Variable Filter (SVF) |
| Trigger Detection | Schmitt trigger with hysteresis |
| Trigger Threshold | 10–500 mV (adjustable) |
| Trigger Lockout | 15ms |
| CV Update Rate | ~1.5kHz (every 32 samples) |
| Stereo | Mono or true stereo processing |
| Output | Soft-clipped (tanh) to prevent digital overs |

---

## Changelog

### v7.2.0 (February 2026)

* 🎛️ **Resonance Restored** — Bass and volume maintained at high resonance via static makeup gain and bandpass mix
* 🖐️ **Dampening Redesigned** — No longer shortens decay. Acts as hand-on-drum: reduces brightness (85%), resonance (40%), VCA ceiling (75%)
* 🔨 **Material Corrected** — Hard = metal (rings longer, bright), Soft = rubber (absorbs, dark). Matches real acoustic physics
* 💥 **Hit Memory Enhanced** — Accumulated hits now slow decay up to 40%, simulating warm vactrol thermal memory
* 🎯 **Velocity Tuned** — Floor raised to 0.35 to reduce trigger voltage wobble between hits

### v7.1.1 (February 2026)

* 🔧 **Double-Hit Fix** — Schmitt trigger detector with hysteresis replaces bare edge detector
* 🧹 **Smile Pass Removed** — Dynamic bass boost was creating non-monotonic amplitude (second peak at 3–5ms)
* ⚡ **Cutoff Smoothing Fixed** — Asymmetric smoother (0.4/0.03) replaced with uniform 0.35 tracking

### v7.1.0 (February 2026)

* 🏗️ **Architecture Rewrite** — Single vactrol envelope model replaces dual-envelope approach
* 📐 **Nonlinear Transfer Curves** — `filterGate = pow(state, exp)`, `vcaGate = sqrt(state)`
* ⚡ **Cortex-M7 Optimization** — Combined `powf` operations into single `expf` call

### v7.0–v7.0.3 (February 2026)

* 🔄 **Engine Merge** — v5.7 sonic engine + v6.0.2 cleanups
* 🐛 **9 Parameter Bugs Fixed** — Enum strings, nullptr terminators, output modes
* 🛡️ **Crash Fix** — SVF state clamping prevents filter blowup on rapid retrigger
* 🖼️ **Graphics Buffer Fix** — Eliminated UI overflow crash

### v5.x (January 2026)

* 🎹 Buchla 292 topology, dual vactrol model
* 🎭 Material modes, FX modes, Hit Memory
* 📊 Custom fader UI with gate metering

---

## Research & References

Holy Mackerel's vactrol model is informed by:

* **Parker & D'Angelo**, "Emulation of the Buchla Lowpass-Gate" (DAFX-13) — Vactrol photoresistive dynamics, level-dependent decay
* **Rabid Elephant Natural Gate** — Hit Memory accumulation, organic percussion philosophy
* **SSF Steady State Gate** — Low Pass Gate design approach
* **Buchla 292** — Original vactrol LPG circuit topology and behavior
* **Georgia Tech acoustic research** — Impact acoustics and material response modeling
* **Perfect Circuit, "What is a Low Pass Gate?"** — Vactrol obsolescence, RoHS, modern LPG landscape

---

## Credits

**Holy Mackerel** was developed for the Expert Sleepers Disting NT platform.

Developed by Andrew Kuttor with Claude (Anthropic) and ChatGPT (OpenAI).

Inspired by:
- Rabid Elephant Natural Gate
- SSF Steady State Gate
- Buchla 292 Low Pass Gate
- Make Noise LxD
- Parker & D'Angelo DAFX-13 research

---

## License

This plugin is provided as-is for use with the Expert Sleepers Disting NT.

---

## Support

For bug reports, feature requests, or general feedback — your input shapes future versions.

**GitHub**: [github.com/kuttor/Disting-NT-Plugin--Holy-Mackerel](https://github.com/kuttor/Disting-NT-Plugin--Holy-Mackerel)

---

```
        🐟🐟🐟🐟🐟🐟🐟🐟
      🐟                🐟
     🐟   HOLY          🐟
     🐟      MACKEREL   🐟
      🐟                🐟
        🐟🐟🐟🐟🐟🐟🐟🐟
```

**Holy Mackerel** — *Vactrol-less LPG with Hate.*
