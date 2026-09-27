# Grok Audio — audio plugins

Audio plugins published by **Grok Audio** (bundle prefix `audio.grok.*`, AU/VST3
manufacturer code `GrkA`).

## Downloads

Prebuilt, ready-to-share VST3 archives live in [`downloads/`](downloads/) (git
ignored, so binaries stay out of the repo). Unzip into
`~/Library/Audio/Plug-Ins/VST3/` on a Mac, then rescan in your DAW.

| Archive | Plugin |
| --- | --- |
| `Silicon Fuzz Face.vst3.zip` | Silicon Fuzz Face |

The bundles are universal (`arm64` + `x86_64`) but ad-hoc signed, so Gatekeeper
may block them on first open. Either right-click the unzipped bundle → Open, or
run `xattr -dr com.apple.quarantine "Silicon Fuzz Face.vst3"`. A Developer ID
signature removes this step.

## Source

Each plugin is a self-contained JUCE 8 project in its own subdirectory:

| Plugin | Directory | CMake target | AU code | Formats |
| --- | --- | --- | --- | --- |
| Silicon Fuzz Face | [`SiliconFuzzFace/`](SiliconFuzzFace/) | `SiliconFuzzFace` | `SiFF` | AU, VST3, Standalone |
| Brown Eye OD | [`FriedmanBEOD/`](FriedmanBEOD/) | `BrownEyeOD` | `BeOD` | AU, VST3, Standalone |

Note the mismatch in Brown Eye OD: the directory is `FriedmanBEOD` (a BE-OD
name) while the target, bundle ID and product name all use `BrownEyeOD` /
`audio.grok.browneyeod`. The directory name is kept as-is.

## Silicon Fuzz Face

NPN silicon Fuzz Face guitar pedal emulation, modelled on the classic two-stage
silicon circuit (Cin 2.2uF, Cout 0.01uF, 1kB fuzz pot, 500kA volume pot, 33k
collector with 100k feedback).

Signal path, per mono sample: input HPF + pickup load lowpass, Q1 `tanh` stage
with supply sag envelope (9V rail, 20uF emitter network), Q2 with emitter
degeneration, NPN asymmetry for even harmonics, silicon gating for quiet notes,
then collector rolloff and output coupling into the volume taper. Runs 2x
oversampled with JUCE's half-band polyphase IIR filters.

- `Fuzz` — 1kB emitter pot (Q2 drive)
- `Volume` — 500kA output pot
- `Input` — guitar/pickup level, like a guitar volume knob

Mono circuit: stereo input is summed, output copied to all channels. Put it
first on a guitar track.

## Brown Eye OD

BE-100 style multi-stage overdrive, in [`FriedmanBEOD/`](FriedmanBEOD/) (see the
naming note above). Stacked op-amp gain stages into a hard clipper, with a
tightening high-pass before the gain stack and tone shaping around it.

- `Gain` — drive through the stacked op-amp stages
- `Volume` — output level
- `Tight` — high-pass before the gain stack (150 Hz → 3.3 kHz)
- `Bass` — low shelf around 80 Hz
- `Treble` — high cut after the hard clipper
- `Presence` — low-pass 1.3 kHz → 7.2 kHz into the hard clip
- `Trim` — internal 4th-stage gain (the pot inside the real pedal)

Mono circuit: stereo input is summed, output copied to all channels. Intended
for the front of a guitar track into a clean or edge-of-breakup amp sim; 9–18 V
headroom is not modelled, the `Trim` knob covers the same job.

## Building

Requires macOS, CMake 3.22+, Git, and the Xcode Command Line Tools
(`xcode-select --install`). JUCE 8.0.8 is fetched automatically on first
configure.

```sh
cd SiliconFuzzFace   # or: cd FriedmanBEOD
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(sysctl -n hw.ncpu)"
```

Note: the `-G Xcode` generator needs a full Xcode install selected via
`xcode-select`; with Command Line Tools only, use the default Makefiles
generator as above. If compiler detection fails, pass
`-DCMAKE_C_COMPILER=/usr/bin/clang -DCMAKE_CXX_COMPILER=/usr/bin/clang++`.

Artifacts land in `build/<target>_artefacts/Release/` and are copied to the user
plug-in folders automatically (`COPY_PLUGIN_AFTER_BUILD`):

- `~/Library/Audio/Plug-Ins/Components/` — Audio Units (GarageBand, Logic)
- `~/Library/Audio/Plug-Ins/VST3/` — VST3 (Reaper, Ableton, Cubase…)

Builds are universal (`arm64` + `x86_64`) with a macOS 11.0 deployment target.
Each plugin builds and installs independently; both share the `GrkA`
manufacturer code but use distinct plugin codes, so they can be installed side
by side.

## Verifying an Audio Unit

```sh
auval -v aufx SiFF GrkA   # Silicon Fuzz Face
auval -v aufx BeOD GrkA   # Brown Eye OD
```

In GarageBand: Settings → Audio/MIDI → enable Audio Units, then
Smart Controls → plugin slot → Audio Units → Grok Audio → pick the plugin. The
blue guitar-track Smart Controls panel is Apple's own skin, so a third-party AU
cannot replace its layout; the full knob set is in the real editor (wrench icon
next to Controls / EQ) and in track automation.

If GarageBand does not list a plugin, quit and reopen it. On newer macOS
releases an unsigned `.component` may need a Developer ID signature before it
loads.
