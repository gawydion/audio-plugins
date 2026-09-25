# Grok Audio — audio plugins

Audio plugins published by **Grok Audio** (bundle prefix `audio.grok.*`, AU/VST3
manufacturer code `GrkA`).

Each plugin is a self-contained JUCE 8 project in its own subdirectory:

| Plugin | Directory | Formats |
| --- | --- | --- |
| Silicon Fuzz Face | [`SiliconFuzzFace/`](SiliconFuzzFace/) | AU, VST3, Standalone |

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

## Building

Requires macOS, CMake 3.22+, Git, and the Xcode Command Line Tools
(`xcode-select --install`). JUCE 8.0.8 is fetched automatically on first
configure.

```sh
cd SiliconFuzzFace
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j"$(sysctl -n hw.ncpu)"
```

Note: the `-G Xcode` generator needs a full Xcode install selected via
`xcode-select`; with Command Line Tools only, use the default Makefiles
generator as above. If compiler detection fails, pass
`-DCMAKE_C_COMPILER=/usr/bin/clang -DCMAKE_CXX_COMPILER=/usr/bin/clang++`.

Artifacts land in `build/SiliconFuzzFace_artefacts/Release/` and are copied to
the user plug-in folders automatically (`COPY_PLUGIN_AFTER_BUILD`):

- `~/Library/Audio/Plug-Ins/Components/` — Audio Units (GarageBand, Logic)
- `~/Library/Audio/Plug-Ins/VST3/` — VST3 (Reaper, Ableton, Cubase…)

Builds are universal (`arm64` + `x86_64`) with a macOS 11.0 deployment target.

## Verifying an Audio Unit

```sh
auval -v aufx SiFF GrkA
```

In GarageBand: Settings → Audio/MIDI → enable Audio Units, then
Smart Controls → plugin slot → Audio Units → Grok Audio → Silicon Fuzz Face.
If GarageBand does not list it, quit and reopen it. On newer macOS releases an
unsigned `.component` may need a Developer ID signature before it loads.
