# XenaVersio

## Dynamic Stochastic Synthesis Oscillator for Versio

XenaVersio is an oscillator firmware for the Noise Engineering Versio platform implementing **Dynamic Stochastic Synthesis (DSS)**, an algorithm pioneered by composer Iannis Xenakis. It generates complex, evolving waveforms through controlled random walks, producing timbres that shift organically over time.

---

## Overview

Dynamic Stochastic Synthesis works by defining a waveform as a series of **breakpoints** connected by line segments. Each breakpoint has two properties: **duration** (how long that segment lasts) and **amplitude** (how high or low that point is). These properties evolve stochastically—wandering randomly within boundaries—creating waveforms that continuously morph while maintaining coherent pitch.

The result is an oscillator that can range from nearly static tones to wildly animated textures, all under voltage control.

---

## Controls

### Knobs

| Knob | Parameter | Description |
|------|-----------|-------------|
| **KNOB 0** | V/Oct | Pitch control with 1V/octave tracking. Frequency range depends on Switch 1 position. |
| **KNOB 1** | Duration Step | How much the segment durations change each cycle. Low = stable rhythm, High = chaotic timing. |
| **KNOB 2** | Amplitude Step | How much the segment amplitudes change each cycle. Low = stable timbre, High = rapidly shifting harmonics. |
| **KNOB 3** | Duration Barrier | Sets the boundaries for duration wandering. Low = constrained, uniform segments. High = wide duration variation. |
| **KNOB 4** | Amplitude Barrier | Sets the boundaries for amplitude wandering. Low = small amplitude variation. High = extreme peak differences. |
| **KNOB 5** | Breakpoints | Number of segments in the waveform (2–16). Fewer = simpler, purer tones. More = complex, rich harmonics. |
| **KNOB 6** | Level | Output volume. |

### Switches

#### Switch 0 (Top) — Walk Mode

| Position | Mode | Description |
|----------|------|-------------|
| **LEFT** | First-Order | Classic random walk. Direct, immediate changes. More erratic evolution. |
| **CENTER** | Second-Order | GENDY3-style walk with velocity/momentum. Smoother, more musical transitions. |
| **RIGHT** | Correlated | Duration and amplitude use the same random values. Creates tightly coupled timbral evolution. |

#### Switch 1 (Bottom) — Frequency Range

| Position | Base Frequency | Range |
|----------|----------------|-------|
| **LEFT** | 55 Hz (A1) | Bass/sub range |
| **CENTER** | 220 Hz (A3) | Mid range |
| **RIGHT** | 880 Hz (A5) | High range |

The V/Oct input (Knob 0) tracks approximately 5 octaves from the selected base frequency.

### Button

**TAP** — Resets the waveform. Reinitializes all breakpoints with new random positions. Use this to restart evolution or escape from an undesirable state.

---

## Inputs & Outputs

### Audio Outputs

- **OUT L** — Main audio output
- **OUT R** — Main audio output (identical signal)

### CV Inputs

- **FSU / CV** — Gate input for external reset trigger (directly tied to audio input, directly usable)
- **Knob 0** — V/Oct pitch CV input (active when patched, adds to knob position)

---

## LED Indicators

| LED | Color | Indicates |
|-----|-------|-----------|
| **LED 0** | Cyan | Phase indicator. Pulses to show the system is running. |
| **LED 1** | Green | Duration Step activity. Brightness reflects Knob 1 value. |
| **LED 2** | Orange | Amplitude Step activity. Brightness reflects Knob 2 value. |
| **LED 3** | White | Output level. Brightness reflects Knob 6 value. |

---

## V/Oct Calibration

XenaVersio supports precise 1V/octave pitch tracking with a calibration routine.

### To Calibrate:

1. Set **both switches to the RIGHT position**
2. Hold the **TAP button** while powering on the module
3. All LEDs will turn white to indicate calibration mode
4. Follow the 3-step process, applying 1V, 2V, and 3V reference voltages to Knob 0
5. Calibration values are saved to flash memory and persist across power cycles

### Default Calibration

If calibration data becomes corrupted or falls outside valid ranges, the module automatically restores factory defaults.

---

## Algorithm Details

### How DSS Works

1. A waveform is defined by 2–16 **breakpoints**
2. Each breakpoint has a **duration** (segment length) and **amplitude** (vertical position)
3. The waveform is drawn by connecting breakpoints with straight lines
4. After each complete cycle, every breakpoint's duration and amplitude are modified by a **random walk**
5. Walks are bounded by **elastic barriers**—when a value hits a limit, it bounces back

### Walk Modes Explained

**First-Order Walk**
- Position changes directly by a random amount each cycle
- Immediate, unpredictable changes
- Good for: glitchy textures, aggressive timbres

**Second-Order Walk**
- Random values modify *velocity*, which then modifies position
- Creates momentum and smoother transitions
- Good for: evolving pads, organic movement

**Correlated Walk**
- Same random value affects both duration and amplitude
- Tight coupling between rhythmic and timbral evolution
- Good for: coherent, unified changes

### Barrier Reflection

When a parameter's random walk reaches a barrier, it reflects back like a ball bouncing off a wall. This keeps parameters bounded while maintaining dynamic motion.

---

## Patch Ideas

### Evolving Drone
- Breakpoints: 8–12
- Duration/Amplitude Step: Low (9–10 o'clock)
- Barriers: Medium (12 o'clock)
- Walk Mode: Second-Order (center)
- Result: Slowly morphing harmonics, excellent for ambient textures

### Harsh Digital Noise
- Breakpoints: Maximum (16)
- Duration/Amplitude Step: High (3–4 o'clock)
- Barriers: Maximum
- Walk Mode: First-Order (left)
- Result: Aggressive, constantly changing digital chaos

### Pseudo-Vocal Tones
- Breakpoints: 4–6
- Duration Step: Medium
- Amplitude Step: Low
- Barriers: Medium-High
- Walk Mode: Second-Order (center)
- Frequency Range: Mid (center switch)
- Result: Formant-like shifting, speech-adjacent timbres

### Controlled Randomness
- Breakpoints: 6–8
- Duration/Amplitude Step: Medium
- Barriers: Low (constrained)
- Walk Mode: Correlated (right)
- Result: Changes feel unified and intentional despite being stochastic

### Sub Bass with Movement
- Frequency Range: Low (left switch)
- Breakpoints: 2–4
- Duration/Amplitude Step: Very low
- Barriers: Low
- Walk Mode: Second-Order (center)
- Result: Deep bass with subtle timbral drift

---

## Specifications

| Parameter | Value |
|-----------|-------|
| Sample Rate | 48 kHz |
| Breakpoint Range | 2–16 |
| Frequency Range | ~55 Hz – 7 kHz |
| V/Oct Tracking | ~5 octaves |
| Audio Output | Mono (mirrored to stereo) |

---

## Credits

XenaVersio is inspired by the stochastic synthesis work of **Iannis Xenakis**, particularly the GENDY family of programs he developed from the 1970s onward. The algorithm implemented here draws from both the original GENDYN concepts and the later GENDY3 refinements.

Firmware developed for the **Noise Engineering Versio** platform.

---

*"The musician...must master the laws of probability."*
— Iannis Xenakis
