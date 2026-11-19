# XTONE – Extratone-Retrigger-Sampler (VST3 für FL Studio)

XTONE lädt ein Sample (typisch eine Kick) und triggert es mit einer **eigenen, vom Projekttempo unabhängigen Uhr** immer wieder neu – bis weit über 1.000 BPM, wo die Kicks zu einem Ton verschmelzen. Die Retrigger-Rate wird zum **Grundton**, das Sample zur **Klangfarbe**.

Spezifikation: [`docs/HANDOFF.md`](docs/HANDOFF.md) · Entscheidungen: [`docs/DECISIONS.md`](docs/DECISIONS.md)

## Installation (Windows)

1. GitHub → **Actions** → letzter grüner Lauf von „build“ → Artefakt **XTONE-win64** herunterladen (oder später unter **Releases**).
2. ZIP entpacken und den Ordner `XTONE.vst3` nach `C:\Program Files\Common Files\VST3\` kopieren.
3. FL Studio → *Options → Manage plugins → Find installed plugins* → XTONE als Favorit markieren → im Channel Rack hinzufügen.
4. Update: FL schließen, den Ordner ersetzen, FL neu starten.

`XTONE.exe` ist die Standalone-Version zum schnellen Testen ohne FL (MIDI-Keyboard oder die Bildschirm-Tastatur in den Einstellungen).

## Stand

| Meilenstein | Inhalt | Status |
|---|---|---|
| M0 Gerüst | Repo, CMake, CI, Default-Kick, generische Parameter-UI | ✔ |
| M1 Kern-Engine | Rate-Quellen BPM/Hz/Host×, Teiler, Sub-Sample-Clock, Cut + Declick, Gate, Sample-Pitch, Sample laden (Dialog + Drag & Drop), State mit Einbettung, Note-ADSR, Pitch-Modus Relativ/Absolut, Glide, Pitchbend, Live-Anzeige | ✔ (wartet auf Abnahme) |
| M2 Chop | Slices, Marker-Editor, Transienten, Slice-Modus, Scan/Länge/Reverse … | teilweise (Slices, Marker-Editing, Scan/Länge/Reverse, Overlap, Pitch-Env schon drin) |
| M3 Charakter | Oversampling, Drive/Clip/DC, Mono, A/B, Jitter, Tune-Snap | teilweise (OS, Drive/Clip/DC, Mono, Jitter, Tune-Snap drin; A/B fehlt) |
| M4 Bewegung | Rampe, LFO 1/2, Unison | Unison drin; Rampe/LFO fehlen |
| M5 Sequencer | Step-Sequencer + Raster | offen |
| M6 Release | Factory-Presets, README, v1.0.0 | offen |

Parameter späterer Meilensteine sind schon sichtbar (stabile IDs), haben aber noch keine Wirkung: **A/B, Rampe, LFO 1/2, Sequencer**.

## Bedienung in Kürze

- **Grundnote** (`key_root`, Default C4 = FL C5) spielt genau das eingestellte Tempo. Eine Oktave höher = doppelte Rate.
- **Rate-Quelle**: `BPM` (Plugin-BPM + Teiler), `Hz` (direkt) oder `Host ×` (Projekttempo × Multiplikator).
- **Keytrack Absolut**: Note 69 (A4) = 440 Hz, die Rate-Quelle wird ignoriert.
- **MIDI-Modus Slice**: Die Note wählt den Slice (ab `Slice-Startnote`), die Rate kommt vom Rate-Tab.
- **Sample**: [Laden…] oder per Drag & Drop (auch aus dem FL-Browser). Dateien bis 32 MB werden ins Projekt eingebettet.
- **Slices**: Doppelklick in die Wellenform setzt einen Marker, Ziehen verschiebt, Rechtsklick löscht; [Gleichmäßig] teilt in N gleiche Slices.
- **Live-Anzeige**: Rate in Hz, BPM-Äquivalent, Ton (wissenschaftlich und FL-Notation), aktueller Slice.

> **Notennamen:** FL Studio nennt MIDI-Note 60 „C5“, die wissenschaftliche Notation „C4“. XTONE zeigt beides an.

## Selbst bauen

```
git clone --recursive https://github.com/TerreurSquad/tone-vst
cmake -B build -A x64
cmake --build build --config Release
```

Nur DSP-Kern + Tests (auch unter Linux, ohne JUCE):

```
cmake -B build -DXTONE_BUILD_PLUGIN=OFF
cmake --build build
ctest --test-dir build
```

## Lizenz

JUCE 8 ist dual lizenziert (AGPLv3 / kommerziell). Für privaten Gebrauch unkritisch – vor Verkauf oder Weitergabe die Lizenz prüfen.
