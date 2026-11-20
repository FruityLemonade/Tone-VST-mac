# XTONE – Extratone-Retrigger-Sampler

XTONE lädt ein Sample (typisch eine Kick) und triggert es mit einer **eigenen, vom Projekttempo unabhängigen Uhr** immer wieder neu – bis weit über 1.000 BPM, wo die Kicks zu einem Ton verschmelzen. Die Retrigger-Rate wird zum **Grundton**, das Sample zur **Klangfarbe**.

Spezifikation: [`docs/HANDOFF.md`](docs/HANDOFF.md) · Entscheidungen: [`docs/DECISIONS.md`](docs/DECISIONS.md)

## Installation (Windows)

1. GitHub → **Actions** → letzter grüner Lauf von „build“ → Artefakt **XTONE-win64** herunterladen (oder später unter **Releases**).
2. ZIP entpacken und den Ordner `XTONE.vst3` nach `C:\Program Files\Common Files\VST3\` kopieren.
3. FL Studio → *Options → Manage plugins → Find installed plugins* → XTONE als Favorit markieren → im Channel Rack hinzufügen.
4. Update: FL schließen, den Ordner ersetzen, FL neu starten.

`XTONE.exe` ist die Standalone-Version zum schnellen Testen ohne FL (MIDI-Keyboard oder die Bildschirm-Tastatur in den Einstellungen).

## Installation (macOS)

Das Universal-Build enthält `XTONE.component` (Audio Unit), `XTONE.vst3` und `XTONE.app` für Intel-Macs und Apple Silicon. Das Build-Ziel ist macOS 11 oder neuer.

ZIP entpacken oder XTONE wie unten beschrieben bauen. Die Plug-in-Ordner bei Bedarf anlegen.

- Für **Logic Pro** `XTONE.component` nach `~/Library/Audio/Plug-Ins/Components/` kopieren und Logic Pro starten. Auf einer Software-Instrument-Spur im Instrument-Slot *AU Instruments → XTONE Dev → XTONE → Stereo* auswählen. Falls XTONE fehlt, im Plug-in-Manager erneut prüfen.
- Für **FL Studio** `XTONE.vst3` nach `~/Library/Audio/Plug-Ins/VST3/` kopieren. Unter *Options → Manage plugins* auf *Find installed plugins* klicken und XTONE als Instrument hinzufügen.
- Für den **Standalone-Betrieb** `XTONE.app` direkt starten.

Die Bundles sind ad hoc signiert und nicht von Apple notarisiert.

## Stand

| Meilenstein | Inhalt | Status |
|---|---|---|
| M0 Gerüst | Repo, CMake, CI, Default-Kick, generische Parameter-UI | ✔ |
| M1 Kern-Engine | Rate-Quellen BPM/Hz/Host×, Teiler, Sub-Sample-Clock, Cut + Declick, Gate, Sample-Pitch, Sample laden (Dialog + Drag & Drop), State mit Einbettung, Note-ADSR, Pitch-Modus Relativ/Absolut, Glide, Pitchbend, Live-Anzeige | ✔ (wartet auf Abnahme) |
| M2 Chop | Slices, Marker-Editor, Transienten, Slice-Modus, Scan/Länge/Reverse … | Slices, Marker-Editor mit Zoom, Transienten, Scan/Länge/Reverse, Overlap und Pitch-Env verfügbar |
| M3 Charakter | Oversampling, Drive/Clip/DC, Mono, A/B, Jitter, Tune-Snap | teilweise (OS, Drive/Clip/DC, Mono, Jitter, Tune-Snap drin; A/B fehlt) |
| M4 Bewegung | Rampe, LFO 1/2, Unison | Unison drin; Rampe/LFO fehlen |
| M5 Sequencer | Step-Sequencer + Raster | offen |
| M6 Release | Factory-Presets, README, v1.0.0 | offen |

Parameter späterer Meilensteine sind schon sichtbar (stabile IDs), haben aber noch keine Wirkung: **A/B, Rampe, LFO 1/2, Sequencer**.

## Bedienung in Kürze

- **Root Note** (`key_root`, Default C4 = FL C5) spielt genau das eingestellte Tempo. Eine Oktave höher = doppelte Rate.
- Im **Rate-Tab** wählst du `BPM` (Plugin BPM + Division), `Hz` (direkt) oder `Host ×` (Projekttempo × Multiplikator). Der große Drehregler stellt den Sollwert der gewählten Quelle ein. Bei BPM und Hz kannst du ihn auch direkt unter dem Regler eingeben.
- **Key Tracking → Absolute**: Note 69 (A4) = 440 Hz, die Rate-Quelle wird ignoriert.
- **MIDI Mode → Slice**: Die Note wählt den Slice (ab **Slice Base Note**), die Rate kommt vom Rate-Tab.
- **Load Sample…** öffnet die Dateiauswahl. Drag & Drop funktioniert auch aus dem FL-Browser. Dateien bis 32 MB werden ins Projekt eingebettet.
- Ein Doppelklick in die Wellenform setzt einen **Slice-Marker**. Ziehen am Griff verschiebt den Marker, ein Rechtsklick löscht ihn. Im Menü **Slices** kannst du das Sample gleichmäßig teilen oder alle Marker löschen.
- **Wellenform**: Mausrad zoomt am Zeiger. Mit Shift + Mausrad, einer horizontalen Trackpad-Geste oder der Scrollleiste verschiebst du den Ausschnitt. Die Tasten [−] und [+] zoomen mittig. [Fit] zeigt das ganze Sample.
- **Transienten**: **Sensitivity** einstellen und [Transients] klicken. Die Analyse läuft im Hintergrund über das ganze Sample. Gefundene Transienten ersetzen die bisherigen Marker und lassen sich danach manuell bearbeiten. Bei keinem Treffer bleiben die Marker erhalten.
- Die **Live-Anzeige** zeigt Rate in Hz, BPM-Äquivalent und Ton in wissenschaftlicher und FL-Notation. Der aktuelle Slice ist in der Wellenform markiert.

> **Notennamen:** FL Studio nennt MIDI-Note 60 „C5“, die wissenschaftliche Notation „C4“. XTONE zeigt beides an.

## Selbst bauen

### Windows

```
git clone --recursive https://github.com/TerreurSquad/tone-vst
cmake -B build -A x64
cmake --build build --config Release
```

### macOS (Universal)

Benötigt CMake 3.25 oder neuer und die Xcode Command Line Tools (`xcode-select --install`).

```sh
git clone --recurse-submodules https://github.com/FruityLemonade/Tone-VST-mac.git
cd Tone-VST-mac
cmake --preset macos-universal
cmake --build --preset macos-universal --parallel
ctest --preset macos-universal
```

Die Bundles liegen unter `build-macos-universal/Xtone_artefacts/Release/` in den Ordnern `AU/`, `VST3/` und `Standalone/`.

### DSP-Kern und Tests

Nur DSP-Kern + Tests (auch unter Linux, ohne JUCE):

```
cmake -B build -DXTONE_BUILD_PLUGIN=OFF
cmake --build build
ctest --test-dir build
```

## Lizenz

JUCE 8 ist dual lizenziert (AGPLv3 / kommerziell). Für privaten Gebrauch unkritisch – vor Verkauf oder Weitergabe die Lizenz prüfen.
