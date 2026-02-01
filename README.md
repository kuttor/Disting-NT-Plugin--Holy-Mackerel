# Holy Mackerel v5.6.0
### A Vactrol-less Low Pass Gate for Expert Sleepers Disting NT

* * *
        ██╗  ██╗ ██████╗ ██╗     ██╗   ██╗    ███╗   ███╗ █████╗  ██████╗██╗  ██╗███████╗██████╗ ███████╗██╗
        ██║  ██║██╔═══██╗██║     ╚██╗ ██╔╝    ████╗ ████║██╔══██╗██╔════╝██║ ██╔╝██╔════╝██╔══██╗██╔════╝██║
        ███████║██║   ██║██║      ╚████╔╝     ██╔████╔██║███████║██║     █████╔╝ █████╗  ██████╔╝█████╗  ██║
        ██╔══██║██║   ██║██║       ╚██╔╝      ██║╚██╔╝██║██╔══██║██║     ██╔═██╗ ██╔══╝  ██╔══██╗██╔══╝  ██║
        ██║  ██║╚██████╔╝███████╗   ██║       ██║ ╚═╝ ██║██║  ██║╚██████╗██║  ██╗███████╗██║  ██║███████╗███████╗
        ╚═╝  ╚═╝ ╚═════╝ ╚══════╝   ╚═╝       ╚═╝     ╚═╝╚═╝  ╚═╝ ╚═════╝╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝╚══════╝╚══════╝
                                   Vactrol-less Low Pass Gate with Hate

* * *

## Overview

Holy Mackerel is a **stereo low pass gate** algorithm for the **Expert Sleepers Disting NT**, inspired by the *Buchla 292* and modern LPG interpretations (Natural Gate / Rabbit / Elephant / solid-state stereo gates).

The goal is not strict emulation.  
The goal is **feel**.

If you want: *clicks, plucks, woody thwacks, rubbery toms, ringing organic decay*, and “hit it harder = it opens up,” this is the lane.

* * *

## Core Concepts (OPEN + DECAY)

This plugin uses a **Natural Gate-style OPEN/DECAY architecture**:

- **Triggers always open the gate fully**
- **DECAY** controls how long it takes to close from peak down to a floor
- **OPEN** sets the **floor** (minimum gate level) between hits

OPEN is applied **post-envelope**.  
So OPEN does not reduce the attack, it changes how closed the gate gets *between* hits.

### Examples
- Open 0%, Decay 0%  → full snap → instant close → click/rimshot territory  
- Open 0%, Decay 50% → full hit → medium decay to silence  
- Open 50%, Decay 0% → full hit → instant decay to a 50% floor  
- Open 50%, Decay 50%→ full hit → medium decay to a 50% floor  

* * *

## Features

### Stereo LPG Engine
- True stereo processing (or mono mode)
- Filter + VCA interact like a real LPG (filter closes faster than amplitude by design)

### Material Modes
Three “materials” that change attack, decay character, brightness, and filter/VCA interaction:

- **Natural** – balanced, classic LPG feel  
- **Hard** – fast attack, brighter, snappier decay  
- **Soft** – slower attack, darker, longer decay  

### Dampening
- Shortens decay
- Darkens brightness
- Reduces ringing / resonance behavior

### Hit Memory (Natural Gate style)
When enabled, repeated hits **add energy** to the system:
- Fast triggers build brightness and volume
- “Keep hitting it and it opens more”

### Optional FX Coloring
Post-LPG character modes (gain-compensated so they don’t crater or explode volume):

- **Clean**
- **Tube**
- **Screamer**
- **Grit**

* * *

## Installation

1. Build the plugin as a `.ntplugin`
2. Copy to your Disting NT SD card:

        /plugins/HolyMackerel.ntplugin

3. Power cycle
4. Load the algorithm from the plugin list

* * *

## Parameters

### Holy Mackerel Page

Parameter | Range | Description
---|---:|---
Resonance | 0–100% | Filter resonance (0% is intentionally clean)
Decay | 0–100% | Time to close from peak down to OPEN floor (musically scaled)
Open | 0–100% | Minimum gate level between hits (post-envelope floor)
Dampening | 0–100% | Shortens/darkens/reduces ringing
Material | Natural/Hard/Soft | Changes attack/decay/brightness response
FX | Clean/Tube/Screamer/Grit | Post-LPG character mode
FX Amount | 0–100% | FX depth (grayed out when FX=Clean)
Gain | 0–106 | Output level (100 = unity, 106 = boost)
Hit Memory | Off/On | Repeated triggers accumulate energy

* * *

## CV Control

Parameter | Description
---|---
Resonance CV | CV input summed with Resonance
Decay CV | CV input summed with Decay
Open CV | CV input summed with Open
Dampening CV | CV input summed with Dampening
FX Amt CV | CV input summed with FX Amount

* * *

## Routing / IO

Parameter | Description
---|---
Trigger Input | CV input used as trigger/gate source
Trig Threshold | Trigger threshold in millivolts
Stereo | Mono / Stereo processing
Left Input | Audio input bus
Right Input | Audio input bus (grayed out in mono)
Left Output | Audio output bus + mode
Right Output | Audio output bus + mode (grayed out in mono)
Env Follower | Off/On (outputs gate level as CV)
Env Output | CV output (0–5V)

* * *

## Notes

- Very short Decay + Open at 0% can click (that’s part of the instrument).
- Material + Dampening are where the “acoustic” behavior lives.
- If you want “sine → tom,” start with:
  - Material: Natural or Soft
  - Dampening: low to medium
  - Resonance: low to moderate
  - Decay: medium
  - Hit Memory: On (if you want build-up)

* * *

## License

MIT
