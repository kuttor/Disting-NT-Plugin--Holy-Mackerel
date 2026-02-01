Got it. This should read like it belongs next to El Applauso and Los Dishes, not like marketing copy or an academic paper. Plainspoken, confident, a little irreverent, very clear. No AI smell.

Below is a full README draft you can drop straight into the repo and tweak names/version numbers as needed.

⸻


██╗  ██╗ ██████╗ ██╗     ██╗   ██╗    ███╗   ███╗ █████╗  ██████╗██╗  ██╗███████╗██████╗ ███████╗██╗     
██║  ██║██╔═══██╗██║     ╚██╗ ██╔╝    ████╗ ████║██╔══██╗██╔════╝██║ ██╔╝██╔════╝██╔══██╗██╔════╝██║     
███████║██║   ██║██║      ╚████╔╝     ██╔████╔██║███████║██║     █████╔╝ █████╗  ██████╔╝█████╗  ██║     
██╔══██║██║   ██║██║       ╚██╔╝      ██║╚██╔╝██║██╔══██║██║     ██╔═██╗ ██╔══╝  ██╔══██╗██╔══╝  ██║     
██║  ██║╚██████╔╝███████╗   ██║       ██║ ╚═╝ ██║██║  ██║╚██████╗██║  ██╗███████╗██║  ██║███████╗███████╗
╚═╝  ╚═╝ ╚═════╝ ╚══════╝   ╚═╝       ╚═╝     ╚═╝╚═╝  ╚═╝ ╚═════╝╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝╚══════╝╚══════╝

Holy Mackerel

Vactrol-less Low Pass Gate with Hate

Holy Mackerel is a stereo low pass gate for the Expert Sleepers Disting NT, inspired by classic vactrol LPGs (Buchla 292), modern interpretations (Natural Gate, Rabbit Hole, Elephant), and solid-state designs — without using actual vactrol modeling.

The goal is not strict emulation.
The goal is feel.

From razor clicks to woody plucks, rubbery toms, and ringing organic decay, Holy Mackerel is designed to make even simple waveforms sound physical and alive.

⸻

Philosophy
	•	No vactrols
	•	No fake randomness
	•	No DAW-style envelopes

Instead:
	•	Instant excitation
	•	Musically scaled decay
	•	Material-dependent behavior
	•	Filter and VCA interacting, not just stacked

A sine wave should be able to sound like an acoustic drum if you hit it right.

⸻

Core Concepts

OPEN and DECAY (Important)

This LPG follows the Natural Gate–style architecture:
	•	Triggers always open the gate fully
	•	DECAY controls how long it takes to close
	•	OPEN sets the floor the gate decays down to

OPEN does not limit how far the gate opens.
It sets how closed the gate gets between hits.

This allows:
	•	Fully closed, percussive clicks
	•	Partial closure with natural bleed
	•	Fully open, continuous tones

OPEN is applied post-envelope.

⸻

Parameters

Resonance

Controls filter resonance.
	•	0% = clean, gentle LPG response
	•	Higher values introduce ringing, emphasis, and eventual self-oscillation
	•	Includes resonance compensation to avoid thinness

⸻

Decay

Controls how long the gate closes from peak to floor.

Musically scaled:
	•	0–5%: clicks, rimshots (5–20ms)
	•	5–20%: short plucks (20–100ms)
	•	20–50%: medium percussive tones (100–500ms)
	•	50–80%: long ringing events (500ms–2s)
	•	80–100%: very long decays (2–8s+)

⸻

Open

Sets the minimum gate level between hits.
	•	0%: fully closes between triggers
	•	Higher values allow sustained tone and harmonic bleed
	•	100%: gate never closes

OPEN interacts strongly with DECAY.

⸻

Dampening

Controls how quickly energy is removed from the system.
	•	Shortens decay
	•	Darkens brightness
	•	Reduces resonance behavior

Useful for turning ringing LPGs into tight, damped percussion.

⸻

Material

Changes the physical behavior of the gate.
	•	Natural – balanced, classic LPG feel
	•	Hard – very fast, snappy, aggressive transients
	•	Soft – slower, rounder, darker response

Material affects:
	•	Attack time
	•	Decay scaling
	•	Filter brightness
	•	Filter vs VCA decay relationship

⸻

FX

Optional post-filter coloration.
	•	Clean – no coloration
	•	Tube – asymmetrical saturation, harmonic enrichment
	•	Screamer – mid-focused overdrive
	•	Grit – fuzz, bit reduction, and feedback character

FX amount is internally gain-compensated to avoid volume jumps.

⸻

Gain

Input gain with headroom above unity.
	•	0–100 = linear gain
	•	100 adds drive into the LPG and FX stages

⸻

Hit Memory

When enabled, repeated triggers add energy instead of resetting.

Fast hits will:
	•	Increase brightness
	•	Increase perceived volume
	•	Increase sustain

Inspired by the Natural Gate “memory” behavior.

⸻

CV Control

Dedicated CV inputs for:
	•	Resonance
	•	Decay
	•	Open
	•	Dampening
	•	FX Amount

CV is summed with panel settings and safely clamped.

⸻

Trigger Input
	•	Accepts gates or triggers
	•	Threshold adjustable in millivolts
	•	Trigger velocity derived from input level

Designed for clocks, envelopes, comparators, and audio-rate triggers.

⸻

Stereo / Mono
	•	True stereo processing
	•	Optional mono mode
	•	Independent left/right envelopes in stereo

⸻

Outputs
	•	Stereo audio out
	•	Optional envelope follower CV output (derived from gate level)

⸻

What This Is Not
	•	A classic ADSR
	•	A pure vactrol model
	•	A polite filter

Holy Mackerel is intentionally physical, reactive, and sometimes unruly.

⸻

Status
	•	Actively developed
	•	Subject to tuning and refinement
	•	Sound quality prioritized over strict CPU minimalism

⸻

Credits / Inspiration
	•	Buchla 292 Low Pass Gate
	•	Rabid Elephant Natural Gate
	•	Modern solid-state LPG designs
	•	Acoustic instrument behavior

⸻

License

MIT (unless otherwise noted)

⸻

If you want, next we can:
	•	tighten the OPEN description wording to match your final intent
	•	add a short “example settings” section
	•	or do a companion diagram explaining OPEN vs DECAY visually
