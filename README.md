# Holy Mackerel v5.6
## Vactrol-less Low Pass Gate with Hate

A research-driven Low Pass Gate for the Expert Sleepers Disting NT, inspired by the Rabid Elephant Natural Gate and SSF Steady State Gate. Holy Mackerel brings the organic, percussive character of acoustic instruments to your modular synthesizer.

---

## Philosophy

Traditional LPGs use vactrols - photoresistors coupled with LEDs - to create their signature "plucky" sound. Holy Mackerel achieves this character digitally, with additional features that push beyond what hardware can do.

> "Natural Gate has been designed to generate a signature response evocative of the acoustics of the natural world. Think of how the plucking of a string or the beating of a drum head produces organic sounds we are all intimately familiar with."
> — Rabid Elephant Natural Gate Manual

---

## Parameters

### Page 1: Main Controls

| Parameter | Range | Default | Description |
|-----------|-------|---------|-------------|
| **Resonance** | 0-100% | 0% | Filter resonance. Low values = warm, natural. High values = ringy, metallic |
| **Decay** | 0-100% | 50% | How long the gate stays open after a trigger |
| **Open** | 0-100% | 100% | How much the gate opens (ceiling). Controls both volume AND brightness |
| **Dampening** | 0-100% | 0% | Mutes the sound, like a palm on a drum head |
| **Material** | Natural/Hard/Soft | Natural | Character of the "struck object" |
| **FX** | Clean/Tube/Screamer/Grit | Clean | Saturation and distortion effects |
| **FX Amount** | 0-100% | 50% | Intensity of the selected effect |
| **Gain** | -12 to +12dB | 0dB | Input gain |
| **Hit Memory** | Off/On | Off | Triggers accumulate energy (see below) |

### Page 2: CV Inputs & I/O

All CV inputs accept ±5V and modulate their respective parameters.

---

## Understanding Open vs Decay

These two parameters work together to shape your sound:

**OPEN** = How much the gate opens (ceiling)
- Controls the MAXIMUM level the gate reaches
- Affects both volume AND filter brightness
- At 0%: Gate barely opens (very quiet, filtered)
- At 100%: Gate fully opens (full volume and brightness)

**DECAY** = How long the gate takes to close
- Controls the time from trigger to silence
- At 0-10%: Clicks and rim shots (3-15ms)
- At 30-50%: Plucky percussion (60-200ms)
- At 70-90%: Sustained notes (600ms-2s)
- At 90-100%: Drone-like (2-6s)

### Musical Examples

| Open | Decay | Result |
|------|-------|--------|
| 100% | 10% | Bright, snappy pluck |
| 100% | 50% | Full tom-like hit |
| 50% | 50% | Muted, softer percussion |
| 100% | 90% | Sustained pad-like drone |
| 30% | 20% | Dark, muted clicks |

---

## Material Modes

Inspired by how different materials respond when struck:

### Natural (Default)
- Balanced attack and decay
- Full frequency range
- Classic LPG character
- Good for: Bongos, wood blocks, general percussion

### Hard
- Very fast attack (~2ms)
- Bright, lots of high frequencies
- Filter decays much faster than VCA (maximum "pluck")
- Good for: Metallic sounds, bells, sharp transients

### Soft
- Slower attack (~8ms)
- Darker, fewer highs
- Gentler decay curve
- Good for: Mallets, felt-covered drums, pads

---

## FX Modes

Holy Mackerel includes four saturation modes to add character:

### Clean
No processing - pure LPG sound.

### Tube
Warm 12AX7-style saturation:
- Asymmetric soft clipping
- Even harmonic enhancement (2nd harmonic warmth)
- Grid blocking compression at high levels
- Good for: Warming up cold digital sources

### Screamer
Aggressive Tube Screamer-style overdrive:
- High gain with bass bypass
- Signature mid-frequency boost
- Hard clipping for bite
- Good for: Aggressive leads, punchy bass

### Grit
Lo-fi destruction:
- Extreme asymmetric clipping
- Bit crushing (10-bit down to 3-bit)
- Sample rate reduction
- Rectification for odd harmonics
- Good for: Industrial, noise, texture

---

## Hit Memory

When enabled, triggers ADD to the current gate level rather than restarting the envelope. This recreates the Natural Gate's "Memory" feature:

> "Natural Gate will start to open more if you keep on hitting it fast enough... It is common for the human body to apply more force the faster it has to do something."
> — Rabid Elephant Natural Gate Manual

### How It Works
- **Memory OFF**: Each trigger starts a new envelope
- **Memory ON**: Rapid triggers accumulate, building brightness and volume

### Musical Use
- Play fast repeated triggers to build intensity
- Create crescendos without automation
- Add human-like dynamics to sequenced patterns

---

## Typical Patches

### Basic Percussion
1. Send oscillator to audio input
2. Send trigger/gate to trigger input
3. Set Decay to 30-50%
4. Set Open to 100%
5. Adjust Material to taste

### Acoustic Tom
1. Use sine or triangle wave input
2. Decay ~40%, Open 100%
3. Material: Natural
4. Resonance: 5-15%
5. Slight Tube saturation

### Plucked String
1. Use sawtooth input
2. Decay ~20%, Open 80%
3. Material: Hard
4. Resonance: 10-20%

### Muted Percussion
1. Any waveform input
2. Decay ~30%, Open 50%
3. Material: Soft
4. Dampening: 20-40%

### Lo-Fi Drums
1. Any input through external drums or oscillator
2. Decay to taste
3. FX: Grit, Amount 50-80%
4. Adds crunch and character

---

## I/O Configuration

### Inputs
- **Left Input**: Main audio input (mono or stereo left)
- **Right Input**: Stereo right (when stereo enabled)
- **Trigger**: Gate/trigger input for envelope

### Outputs
- **Left Output**: Processed audio
- **Right Output**: Stereo right (when stereo enabled)
- **Env Output**: Envelope follower output (0-5V)

### Output Modes
- **Replace**: Output only the processed signal
- **Add**: Mix processed signal with existing bus content

---

## Tips & Tricks

### Getting Natural Sounds
- Keep Resonance low (0-20%)
- Use Material modes rather than heavy FX
- Let Decay breathe - don't make everything a click

### Preventing Clicks
- Avoid 0% Decay unless you want clicks
- Use Dampening to soften transients
- Soft Material has the gentlest attack

### CV Modulation Ideas
- Modulate Decay with an LFO for evolving textures
- Use velocity CV to control Open for dynamics
- Sequence Material changes for variety

### Using as an Effect
- With Env Follower on, it becomes an auto-wah/filter
- Feed drums through for dynamic filtering
- Use on pads for rhythmic gating

---

## Version History

### v5.6
- Complete rewrite of Open/Decay architecture
- Open now controls ceiling (how much gate opens)
- Decay controls time to close back to silence
- Aggressive FX modes (Tube, Screamer, Grit)
- Hit Memory feature
- Research-based vactrol modeling

### Previous Versions
- v5.x: Buchla 292 topology, dual vactrol model
- v4.x: FX modes, Materials
- v3.x: Basic LPG functionality

---

## Credits

Inspired by:
- Rabid Elephant Natural Gate
- SSF Steady State Gate
- Buchla 292 Low Pass Gate
- Make Noise LxD

Developed for Expert Sleepers Disting NT.

---

*"Spread your creativity and kindness with the world..."*
