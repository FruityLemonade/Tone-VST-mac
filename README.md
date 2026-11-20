# XTONE – Extratone Retrigger Sampler

XTONE repeatedly retriggers a sample, typically a kick drum, at rates well above 1,000 BPM. At high rates, individual hits merge into a pitched tone. The retrigger rate sets the pitch, and the sample shapes the timbre. XTONE can run on its own clock or follow the host tempo.

See the [technical specification](docs/HANDOFF.md) and [design decisions](docs/DECISIONS.md) for project details.

## Installation on Windows

1. Open the latest successful [Windows build](https://github.com/FruityLemonade/Tone-VST-mac/actions/workflows/build.yml) and download the **XTONE-win64** artifact.
2. Extract the ZIP and copy the `XTONE.vst3` folder to `C:\Program Files\Common Files\VST3\`.
3. In FL Studio, open *Options → Manage plugins* and click *Find installed plugins*. Mark XTONE as a favorite and add it to the Channel Rack.
4. To update, close FL Studio, replace the existing `XTONE.vst3` folder, and restart FL Studio.

To run XTONE outside a DAW, open `XTONE.exe` and select your MIDI input in **Audio/MIDI Settings**.

## Installation on macOS

The universal build includes `XTONE.component` (Audio Unit), `XTONE.vst3`, and `XTONE.app`. It supports Intel and Apple Silicon Macs running macOS 11 or later.

Extract the ZIP if you have a prebuilt bundle, or [build XTONE from source](#building-from-source). Create the plug-in folders if needed.

- For **Logic Pro**, copy `XTONE.component` to `~/Library/Audio/Plug-Ins/Components/` and launch Logic Pro. On a software instrument track, select *AU Instruments → XTONE Dev → XTONE → Stereo*. If XTONE is missing, rescan it in the Plug-in Manager.
- For **FL Studio**, copy `XTONE.vst3` to `~/Library/Audio/Plug-Ins/VST3/`. Open *Options → Manage plugins*, click *Find installed plugins*, and add XTONE as an instrument.
- For standalone use, open `XTONE.app` directly.

The macOS bundles are ad hoc signed and have no Apple notarization.

## Development status

| Milestone | Scope | Status |
|---|---|---|
| M0 Foundation | Repository, CMake, CI, built-in kick, parameter editor | Complete |
| M1 Core engine | BPM, Hz, and host-synced clock, note divisions, subsample timing, Cut and Declick, gate, pitch, sample loading and embedding, ADSR, pitch modes, glide, pitch bend, and live readouts | Implemented, pending acceptance testing |
| M2 Slicing | Slice markers, waveform editing and zoom, transient detection, MIDI slice selection, scan, length, reverse, overlap, and pitch envelope | Available |
| M3 Character | Oversampling, drive, clipping, DC filtering, mono, A/B, jitter, and Tune Snap | Available except A/B |
| M4 Modulation | Ramp, LFO 1 and 2, and unison | Unison available |
| M5 Sequencer | Step sequencer and grid | Planned |
| M6 Release | Factory presets, documentation, and v1.0.0 | Factory presets available. v1.0.0 pending. |

The editor already exposes **A/B**, **Ramp**, **LFO 1**, **LFO 2**, and **Sequencer** controls with stable parameter IDs. These features are pending implementation.

## Quick start

- Click the **XTONE** wordmark to see the plugin version and choose **Hammer Drill**, **Makita**, **Festool**, or **Hilti**. XTONE saves the skin with each plugin instance in your DAW project. Preset changes keep the current skin.
- In the **Rate** tab, choose **BPM** for **Plugin BPM** with a note **Division**, **Hz** for a direct frequency, or **Host ×** for the project tempo multiplied by the **Host Multiplier**. Turn the main dial to set the rate, or type a BPM or Hz value below it.
- Play the **Root Note** to hear the selected rate. The default is MIDI note 60, shown as C4 in scientific pitch notation and C5 in FL Studio. In **Relative** key tracking, going up an octave doubles the rate. With **Key Tracking** set to **Absolute**, MIDI note 69 (A4) plays at 440 Hz by default, independent of the rate source.
- Click **Load Sample…** or drag an audio file onto the editor. Dragging from the FL Studio browser is supported. Files up to 32 MB are embedded in the project. Larger files are stored by path.
- Click a slice in the waveform to select it. Double-click to add a marker, drag its handle to move it, or right-click it to remove it. Open **Slices** and choose **Slice Evenly** or **Clear All Markers** to edit the whole sample.
- In **Slice** MIDI mode, notes select slices relative to your selection and **Slice Base Note**. The **Rate** tab controls how fast the slice retriggers.
- Scroll over the waveform to zoom at the pointer. Pan with Shift and the mouse wheel, a horizontal trackpad gesture, or the scrollbar. Use **−** and **+** to zoom around the center, or click **Fit** to show the full sample.
- Set **Sensitivity** and click **Transients** to analyze the entire sample in the background. Detected transients replace the existing slice markers, which you can then edit by hand. If the analysis finds no transients, it keeps the existing markers.
- The rate display shows frequency in Hz, its BPM equivalent, and pitch in both scientific and FL Studio notation. The waveform shades the selected slice in the skin's accent color and marks the playing slice with a white line along its top edge.
- In **Envelope**, drag A, D, or R horizontally to adjust the time, or S vertically to adjust the sustain level. Double-click a point to reset it. The graph highlights the active stage and shows the current envelope level while playing. Click a value below the graph to enter an exact setting.
- **LFO 1** and **LFO 2** show a waveform preview. Drag the phase marker to set the start phase. Choose a timing mode to see its relevant control. The LFO modulation engine is pending.

## Building from source

### Windows

Install CMake 3.25 or later and Visual Studio with C++ desktop development support.

```sh
git clone --recurse-submodules https://github.com/FruityLemonade/Tone-VST-mac.git
cd Tone-VST-mac
cmake -B build -A x64
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

The VST3 and standalone builds are under `build/Xtone_artefacts/Release/`.

### macOS Universal

Install CMake 3.25 or later and the Xcode Command Line Tools (`xcode-select --install`).

```sh
git clone --recurse-submodules https://github.com/FruityLemonade/Tone-VST-mac.git
cd Tone-VST-mac
cmake --preset macos-universal
cmake --build --preset macos-universal --parallel
ctest --preset macos-universal
```

The `AU/`, `VST3/`, and `Standalone/` bundles are under `build-macos-universal/Xtone_artefacts/Release/`.

### DSP core and tests

The DSP core and its tests also build on Linux without JUCE. Run these commands from the checkout with CMake 3.25 or later and a C++20 compiler.

```sh
cmake -B build-dsp -DCMAKE_BUILD_TYPE=Release -DXTONE_BUILD_PLUGIN=OFF
cmake --build build-dsp --parallel
ctest --test-dir build-dsp --output-on-failure
```

## License

JUCE 8 is dual-licensed under the AGPLv3 and a commercial JUCE license. See the [JUCE license terms](https://github.com/juce-framework/JUCE/blob/91ad83ae34a81e0833b1a2b0866f54846370ae53/LICENSE.md).
