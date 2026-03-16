# CombWerk — Multistage Comb Drum FX Plugin

A stereo drum FX plugin built with JUCE + CMake, targeting **CLAP** (Bitwig),
with optional VST3/AU builds.

**Signal chain:** Malström-style Comb → Blur Verb → Dirt/Overdrive →
Internal Sidechain Pump → Glue Compressor → Output.

Includes a **RangeMode** switch (Normal / Double / Triple) that scales
internal parameter ranges from controlled drum FX into wild, IDM/glitch
territory.

---

## Repository Structure

```
CombWerk/
├── CMakeLists.txt                     # Root build config
├── .gitmodules                        # Submodule definitions
├── Source/
│   ├── PluginProcessor.h              # DSP declarations & helper classes
│   ├── PluginProcessor.cpp            # Full DSP chain implementation
│   ├── PluginEditor.h                 # UI declarations
│   └── PluginEditor.cpp               # UI layout
├── modules/
│   └── JUCE/                          # JUCE framework (git submodule)
└── dependencies/
    └── clap-juce-extensions/          # CLAP wrapper (git submodule)
```

---

## Prerequisites

- **macOS** 11.0+ (builds universal binary: Apple Silicon + Intel)
- **CMake** ≥ 3.21
- **Xcode** (or command-line tools): `xcode-select --install`
- **Git**

---

## Setup & Build

### 1. Clone with submodules

```bash
git clone --recursive https://github.com/YOUR_USER/CombWerk.git
cd CombWerk
```

Or, if already cloned without `--recursive`:

```bash
git submodule update --init --recursive
```

### 2. Configure

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
```

For an **Xcode project** instead:

```bash
cmake -S . -B build -G Xcode
```

### 3. Build

```bash
cmake --build build --config Release
```

### 4. Find your plugins

After building, plugin binaries are under:

```
build/CombWerk_artefacts/Release/
├── CLAP/CombWerk.clap            ← primary target for Bitwig
├── VST3/CombWerk.vst3
├── AU/CombWerk.component
└── Standalone/CombWerk.app
```

> If `COPY_PLUGIN_AFTER_BUILD` is enabled (it is by default), the CLAP
> will also be copied to `~/Library/Audio/Plug-Ins/CLAP/`.

### 5. Install for Bitwig

Bitwig scans `~/Library/Audio/Plug-Ins/CLAP/` by default. If needed:

```bash
cp -R build/CombWerk_artefacts/Release/CLAP/CombWerk.clap \
      ~/Library/Audio/Plug-Ins/CLAP/
```

Restart Bitwig and scan for new plugins.

---

## Controls Reference

| Section | Control      | Range / Values                    | Default   |
|---------|-------------|-----------------------------------|-----------|
| **Comb**   | Mode       | Comb+ / Comb–                     | Comb+     |
|            | Freq       | 100 Hz – 8 kHz (log)             | 1500 Hz   |
|            | Reso       | 0 – 100%                         | 55%       |
|            | Tone       | 800 Hz – 16 kHz                  | 7000 Hz   |
|            | Mix        | 0 – 100%                         | 65%       |
| **Blur**   | Amount     | 0 – 100%                         | 45%       |
|            | Size       | 0.5x – 2.0x                      | 1.0x      |
|            | Feedback   | 0 – 100%                         | 50%       |
|            | Tone       | 1 kHz – 16 kHz                   | 7000 Hz   |
| **Dirt**   | Drive      | 0 – 24 dB                        | 6 dB      |
|            | Tone       | 1 kHz – 16 kHz                   | 7000 Hz   |
|            | Mix        | 0 – 100%                         | 45%       |
| **Pump**   | On/Off     | toggle                            | Off       |
|            | Amount     | 0 – 100%                         | 35%       |
|            | Speed      | 0 – 100%                         | 50%       |
| **Glue**   | On/Off     | toggle                            | On        |
|            | Threshold  | -30 dB – 0 dB                    | -18 dB    |
|            | Ratio      | 1.5:1 – 4:1                      | 2:1       |
|            | Attack     | 0.3 ms – 30 ms                   | 10 ms     |
|            | Release    | 50 ms – 1000 ms                  | 250 ms    |
|            | Makeup     | 0 – 12 dB                        | 2 dB      |
| **Global** | Range Mode | Normal / Double / Triple          | Normal    |
|            | Mix        | 0 – 100%                         | 50%       |
|            | Trim       | -24 dB – +12 dB                  | 0 dB      |

---

## Range Mode

- **Normal**: Sensible, musical ranges for drum buses.
- **Double**: Internal multiplier ×2 on Freq, Resonance, Blur FB, Drive,
  Pump depth — pushes into aggressive territory.
- **Triple**: ×3 — extreme, metallic, self-oscillating textures.
  Feedback loops are internally clamped via soft saturation for stability.

---

## Design Notes

- All feedback paths include `softClip()` saturation to prevent runaway
  oscillation, especially in Double/Triple modes.
- Key parameters (CombFreq, Resonance, Drive, Mix) are smoothed at 5 ms
  to avoid zipper noise while remaining responsive to fast automation.
- The delay time smoother uses per-channel one-pole filters to prevent
  clicks during comb frequency sweeps.
- Allpass diffusers in the Blur stage use fixed coefficients (0.5) — adjust
  in `PluginProcessor.cpp` if you want more or less diffusion.
- The glue compressor uses a 6 dB soft knee for transparent compression.
- Final output is hard-limited to ±4.0 (≈+12 dBFS) as a safety net.

---

## License

This project template is provided as-is. JUCE and clap-juce-extensions
have their own licenses — see their respective repositories.
