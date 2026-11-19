# XTONE – Extratone-Retrigger-Sampler für FL Studio (Windows)

**Hand-off / technische Spezifikation v1.0** · Stand 19.11.2026 · Auftraggeber: Pascal
Arbeitstitel „XTONE“ (Name, Firmenname und Plugin-Codes vor dem ersten Release festlegen, siehe §12).

Dieses Dokument ist die vollständige Grundlage für die Umsetzung. Es richtet sich an den Implementierer (z. B. Claude Code in einem GitHub-Repo). Lege es im Repo als `docs/HANDOFF.md` ab.

---

## 1. Ziel in einem Absatz

Ein VST3-Instrument für FL Studio unter Windows x64. Es lädt ein Sample (typisch eine Kick), zerlegt es in Slices, pitcht es und triggert es mit einer **eigenen, vom Projekttempo unabhängigen Uhr** immer wieder neu. FL Studio kann das Projekttempo nur zwischen 10 und 522 BPM einstellen. Extratone lebt dagegen von Retrigger-Raten ab ca. 1.000 BPM, bei denen die Kicks zu einem Ton verschmelzen. Die Retrigger-Rate wird so zum **Grundton**, das Sample (und welcher Teil davon wiederholt wird) zur **Klangfarbe**. Beides muss strikt getrennt steuerbar sein. Eine aufwendige GUI ist nicht gefordert, sie muss aber vollständig bedienbar sein.

### Kernformel

```
Rate[Hz]     = BPM / 60 / Notenlänge_in_Vierteln      (1/4 → 1, 1/16 → 0.25, punktiert ×1.5, Triole ×2/3)
BPM-Äquiv.   = Rate[Hz] × 60                          (Anzeige „wie 1/4-Kicks bei X BPM“)
Note → Hz    = A4 × 2^((Note − 69) / 12)
```

| Plugin-Tempo | Teiler | Rate | Ton (wiss. Notation / FL-Notation) |
|---|---|---|---|
| 500 BPM | 1/4 | 8,333 Hz | C-1 +33 ct / FL C0 |
| 500 BPM | 1/16 | 33,333 Hz | C1 +33 ct / FL C2 |
| 500 BPM | 1/16 punktiert | 22,222 Hz | F0 +31 ct / FL F1 |
| 500 BPM | 1/16 Triole | 50,000 Hz | G1 +35 ct / FL G2 |
| 500 BPM | 1/64 | 133,333 Hz | C3 +33 ct / FL C4 |
| 3.924 BPM | 1/4 | 65,41 Hz | C2 / FL C3 |
| 26.400 BPM | 1/4 | 440 Hz | A4 / FL A5 |

**Achtung Notennamen:** FL Studio nennt MIDI-Note 60 „C5“, die wissenschaftliche Notation „C4“. Die GUI zeigt beides an.

---

## 2. Entscheidungen des Auftraggebers (verbindlich)

| Thema | Entscheidung |
|---|---|
| MIDI-Steuerung | **Umschaltbar**: Modus „Pitch“ (Note = Tonhöhe/Rate) oder „Slice“ (Note = Slice, Rate vom BPM-Regler) |
| Chopping | Gleichmäßige Slices **+** Transienten-Erkennung **+** manuelle Marker per Maus in der Wellenform |
| Extras in v1 | **Alle**: Tune-Snap, Tempo-Rampen, 2 LFOs, Start-Scan, Unison/Detune, A/B-Wechsel, Jitter, Step-Sequencer |
| Build | **GitHub-Repo mit automatischem Windows-Build** (GitHub Actions); Pascal lädt nur die fertige `.vst3` herunter |
| GUI | Funktional statt schön: generische Regler + Wellenform-Editor + Sequencer-Raster |

---

## 3. Klangziel: Analyse des Referenztracks

Referenz: *Gabberdoom – Humans Love Speed* (2:58, MP3 128 kbit/s). Messwerte aus der Analyse (YIN-Periodizität, Lautheit, Spektrum):

- **Raster:** 78 % der Frames sind klar periodisch. Die Raten liegen *exakt* auf 500 × 2ⁿ BPM-Äquivalent: 500 (8,33 Hz), 1.000 (16,7 Hz), 2.000 (33,3 Hz), 4.000 (66,7 Hz), 8.000 (133,3 Hz), selten 16.000. Dazu kommen punktierte Werte (1.333, 2.667), die als F-Töne auftauchen. Der Track arbeitet also mit einem **Grundraster von 500 BPM** und springt zwischen den Teilern 1/4 … 1/64. Das ergibt Oktavsprünge auf „C +33 ct“, weil das 500-BPM-Raster nicht auf A440 gestimmt ist.
- **Wechsel:** Eine Rate wird typischerweise 0,2–1 s gehalten (Median 0,65 s). Die Wechsel sind harte Stufen ohne Glide. Vereinzelt gibt es kurze Abwärts-Bends von ca. 1,5 Halbtönen (z. B. bei 0:48 und 0:52). Gegen Ende kommen höhere, nicht-oktavische Raten bis ca. 45.000 BPM-Äquivalent (≈ 750 Hz).
- **Lautheit/Clipping:** Peak 0 dBFS, RMS −1,4 dBFS, Crest-Faktor nur **1,4 dB**, integriert ≈ +1 LUFS, 27 % aller Samples ≥ −0,5 dBFS. Die Wellenform ist praktisch eine **hart geclippte Rechteckwelle**: Pro Periode wird der Kick-Körper in ein Rechteck gepresst.
- **Stereo:** Praktisch mono (Side 17,6 dB unter Mid).
- **Spektrum:** 47 % der Energie liegen bei 60–150 Hz, 17 % bei 150–400 Hz und 9 % unter 60 Hz. Oben wird es schnell dünn.

**Konsequenzen für das Design:**
1. Teiler-Wechsel (inkl. punktiert/Triole) und Noten-Oktavsprünge müssen sofort und phasenrichtig wirken. Der Modus „Keytrack relativ“ (§5.2) bildet genau das Referenz-Raster ab: Grundnote = 500 BPM 1/16, eine Oktave höher spielen = doppelte Rate.
2. **Drive + Hard-Clip mit Oversampling gehören zum Kern**, weil sie den Sound tragen. Sie sind nicht nur Schutz.
3. Default ist Mono.
4. Factory-Preset „Speed Grid 500“ reproduziert den Referenzcharakter (§9), ein Test prüft das (§10.1, T15).

---

## 4. Technischer Rahmen

| Punkt | Vorgabe |
|---|---|
| Sprache/Framework | C++20, **JUCE 8** als Git-Submodule `JUCE/`, auf den neuesten stabilen 8.0.x-Tag gepinnt (Stand Nov. 2026 mindestens 8.0.14; neuer Tag beim Start prüfen, nicht älter als 8.0.11 wegen VS-2026-Support) |
| Formate | **VST3** (Hauptziel) + **Standalone** (zum Testen). CLAP ist später möglich (FL Studio unterstützt CLAP), nicht Teil von v1 |
| Plattform | Windows 10/11 x64, MSVC (Runner `windows-latest` = Windows Server 2025 mit Visual Studio 2026) |
| Plugin-Typ | Instrument (`IS_SYNTH TRUE`, `NEEDS_MIDI_INPUT TRUE`), 0 Eingänge, 1 Stereo-Ausgang |
| C-Runtime | statisch (`CMAKE_MSVC_RUNTIME_LIBRARY = MultiThreaded$<$<CONFIG:Debug>:Debug>`), damit keine VC-Redist-Abhängigkeit entsteht |
| DSP-Kern | **reines C++20 ohne JUCE** (statische Bibliothek `xtone_dsp`). So sind die Tests schnell und laufen auch unter Linux |
| Tests | Catch2 v3 (FetchContent, ≥ v3.8.1) für den DSP-Kern; **pluginval** (Strictness 5) für das fertige VST3 in CI |
| Lizenz | JUCE 8 ist dual lizenziert (AGPLv3 / kommerziell, „Starter“ kostenlos). Für privaten Gebrauch unkritisch. Vor Verkauf oder Weitergabe die Lizenz prüfen |

---

## 5. Architektur

### 5.1 Repo-Struktur

```
xtone/
├─ CMakeLists.txt
├─ JUCE/                         Git-Submodule (gepinnter Tag)
├─ dsp/                          reines C++20, keine JUCE-Abhängigkeit
│  ├─ RateMath.h/.cpp            BPM/Teiler/Hz/Noten, Tune-Snap, Skalen
│  ├─ RetriggerClock.h           Phasenakkumulator, Sub-Sample-Events, Jitter
│  ├─ SampleView.h               nicht-besitzende Sicht: const float* ch[2], frames, numCh, sampleRate
│  ├─ HitVoice.h/.cpp            Wiedergabe, Interpolation, Gate, Declick, Pitch-Env
│  ├─ Lane.h/.cpp                eine Unison-Stimme: Clock + Voice-Pool + Slice-Auswahl (A/B, Seq)
│  ├─ Modulators.h               LFO, Ramp, ADSR, LogSmoother
│  ├─ StepSequencer.h            Step-Daten (atomar), Fortschalten
│  ├─ SliceModel.h/.cpp          Marker → Slices, Equal-Slicing
│  ├─ OnsetDetector.h/.cpp       Transienten-Erkennung (offline)
│  ├─ DefaultKick.h/.cpp         synthetische Default-Kick (kein Asset nötig)
│  └─ Engine.h/.cpp              MIDI-Logik, Glide, Rampe, LFOs, Lanes, Note-ADSR, Drive/Clip/DC
├─ plugin/
│  ├─ Parameters.h/.cpp          Parameter-Layout (stabile IDs!), Gruppen
│  ├─ PluginProcessor.h/.cpp     APVTS, State, Oversampling, Chunking, Sample-Swap
│  ├─ SampleLoader.h/.cpp        Dekodieren im Hintergrund, Einbetten in den State
│  ├─ Presets.h/.cpp             Factory-Programme
│  └─ gui/ PluginEditor, HeaderBar, WaveformView, SequencerGrid, ParamPanel
├─ tests/                        Catch2-Tests für dsp/
├─ docs/ HANDOFF.md, DECISIONS.md
└─ .github/workflows/build.yml
```

### 5.2 Signalfluss

```
MIDI ─► Engine (Notenstack, Modus Pitch/Slice, Glide, Pitchbend, Sustain)
          │ Basis-Rate: Quelle (BPM/Hz/Host×) bzw. Note → Tune-Snap
          │ + Glide + Bend + Rampe + LFO → finale Rate (log-Domäne, Halbtöne)
          ▼
   Lane 1..N (Unison, je eigene Verstimmung/Pan)
     RetriggerClock ──Hit(subOffset)──► Slice-Wahl (Param | MIDI | A/B | Sequencer)
                                          ▼
                                   HitVoice-Pool (Cut: 2 Stimmen / Overlap: bis 32)
                                   Sample-Pitch + Pitch-Env, Scan/Länge/Reverse,
                                   Gate + Declick, Interpolation
          ▼ Summe Lanes × Note-ADSR × Velocity
   Drive → Clip (Soft/Hard) → DC-Blocker → Ausgang
   (alles mit Engine-Rate fs×OS) ─► Downsampling (JUCE dsp::Oversampling) ─► Out
```

### 5.3 Echtzeit-Regeln (verbindlich)

- Im Audio-Thread wird nichts allokiert, nichts gelockt und nichts gelöscht. Dazu gehören `juce::ScopedNoDenormals` am Blockanfang.
- **Interne Chunks:** `processBlock` zerlegt jeden Host-Block in Chunks von maximal 256 Samples (Host-Rate). Bekannte FL-Studio-Eigenheit: FL liefert unter Umständen größere Blöcke als in `prepareToPlay` angekündigt (dokumentiert in der Entwickler-Community bei niedrigen Projekttempi). Deshalb darf **nichts** von `maximumExpectedSamplesPerBlock` abhängen. Alle Puffer werden für 256 × 8 (max. OS) vorallokiert.
- MIDI-Events sample-genau: Chunks zusätzlich an Event-Positionen teilen. Die Event-Position wird im Engine-Takt mit dem OS-Faktor multipliziert.
- Parameter werden pro Chunk aus den APVTS-Atomics in einen POD-Snapshot `EngineParams` gelesen. Geglättet wird im Engine (Rate in log-Domäne).
- **Sample-/Slice-Wechsel lock-frei:** Der Loader erzeugt auf einem Hintergrund-Thread ein unveränderliches `SampleAsset` (ReferenceCounted: Audiodaten, Slice-Marker, Peak-Cache). Die Veröffentlichung läuft über `std::atomic<SampleAsset*> pending` (mit vorher erhöhtem Refcount). Der Audio-Thread tauscht am Chunk-Anfang. Das alte Asset kommt in eine lock-freie FIFO (`juce::AbstractFifo`), die ein Message-Thread-Timer leert und dort freigibt. Marker-Änderungen aus der GUI erzeugen ebenfalls ein neues Asset (Copy-on-Write, Audiodaten per Shared-Pointer geteilt).
- `double` für Phasen, Lese-Positionen und Rate-Berechnungen, `float` für Audio.
- Ausgabe absichern: NaN/Inf → 0.

---

## 6. DSP-Spezifikation

Bezeichnungen: `fsE` = Engine-Rate = Host-Samplerate × OS-Faktor. Alle Zeitkonstanten werden in `fsE`-Samples umgerechnet.

### 6.1 Rate-Berechnung (pro Sample, geglättet)

**Quelle** (`rate_src`):
- `BPM`: `baseHz = bpm / 60 / lenQ(division, div_mod)`
- `Hz`: `baseHz = hz`
- `Host×`: `baseHz = hostBpm × host_mult / 60 / lenQ(...)`, mit Fallback auf 120 BPM ohne Host-Info. Bei `host_lock = an` wird die Clock-Phase bei Transport-Start und bei ppq-Sprüngen auf `frac(ppq × host_mult / lenQ)` gesetzt. Dazwischen läuft sie frei weiter, damit sie nicht gegen Jitter oder Detune kämpft.

`lenQ`: 1/1=4, 1/2=2, 1/4=1, 1/8=0.5, 1/16=0.25, 1/32=0.125, 1/64=0.0625; Triole ×2/3, punktiert ×1.5.

**MIDI-Modus** (`midi_mode`):
- `Pitch` + `key_mode = Relativ`: `noteSt = note − key_root`, die Rate ist `baseHz × 2^(noteSt/12)`. Die Grundnote spielt also genau das eingestellte Tempo. Das ist der Default und entspricht dem Referenz-Workflow.
- `Pitch` + `key_mode = Absolut`: `baseHz = a4_ref × 2^((note − 69)/12)` (die Rate-Quelle wird ignoriert, Anzeige als BPM-Äquivalent).
- `Slice`: Die Rate kommt von der Quelle, die Note wählt den Slice (§6.5).

**Halbton-Summe** (`noteSt` nur im Modus Pitch/Relativ, sonst 0): `st = noteSt + transpose + fine/100 + glide + bend×pb_range + 12×ramp + Σ lfo×to_rate + laneDetune/100`

**Tune-Snap** (`snap_mode`):
- `Basis`: Rundet `baseHz × 2^((noteSt+transpose+fine/100)/12)` vor Glide/Bend/Rampe/LFO auf die nächste erlaubte Note von `snap_scale`/`snap_root` (bezogen auf `a4_ref`).
- `Alles`: Snap auf die finale Rate ohne Lane-Detune. Modulation wird dadurch gestuft (gewollter Effekt).
- Skalen: Chromatisch, Dur, Moll, Harm. Moll, Phrygisch, Moll-Pentatonik, Nur Grundton (Oktaven).

**Finale Rate:** `rateHz = clamp(baseHz × 2^(st/12), 0.01, min(4000, 0.45 × fsE))`
**Glättung:** Rate-Parameter in log-Domäne mit `rate_smooth` (Default 2 ms). Teiler- und Notenwechsel bei `rate_smooth = 0` springen sofort (Referenz).
**Glide:** Exponentiell in Halbtönen mit Zeitkonstante `glide`. Bei `glide_legato = an` nur bei überlappenden Noten.

### 6.2 RetriggerClock (eine pro Lane)

```cpp
// pro Engine-Sample:
phase += inc;                        // inc = rateHz / fsE × jitterFactor  (≤ 0.1 durch Rate-Cap)
if (phase >= 1.0) {
    phase -= 1.0;
    double subOffset = phase / inc;  // Samples seit dem exakten Nulldurchgang, ∈ [0,1)
    onHit(subOffset);                // Hit startet sub-sample-genau
    jitterFactor = 1.0 + jitter * 0.5 * rnd(-1, 1);   // neu pro Periode
}
```
- **Phase-Reset** (`phase_reset = an`): Eine neue Note (nicht legato) feuert an ihrer Sample-Position sofort einen Hit mit `subOffset = 0` und setzt `phase = 0`.
- Unison-Lanes starten bei `uni_phase = an` mit Zufallsphase, sonst alle bei 0.
- Jitter ändert die mittlere Rate nicht (gleichverteilter Faktor, Mittel 1).

### 6.3 HitVoice

Beim Hit:
1. **Slice bestimmen** (§6.5) → `[sliceStart, sliceEnd)` in Quell-Frames, `sliceLen`.
2. **Fenster:** `winLen = length × sliceLen`, `start = sliceStart + scanEff × (sliceLen − winLen)`, `end = start + winLen`. `scanEff = clamp(scan + Σ lfo×to_scan, 0, 1)` wird beim Hit-Start abgetastet. Bei `reverse` wird von `end` nach `start` gelesen.
3. **Geschwindigkeit:** `speed = srcRate / fsE × 2^((s_pitch + s_fine/100 + hitPitch + Σ lfo×to_pitch + penv(t)) / 12)`. Das wird kontinuierlich berechnet, `hitPitch` kommt aus Sequencer oder A/B. Pitch-Env: `penv(t) = penv_amt × exp(−t / penv_decay)`.
4. **Start-Position sub-sample-genau:** `pos = start + subOffset × speed` (bei Reverse `end − subOffset × speed`).
5. **Interpolation** (`interp`):
   - `Hermite` (Default, 4-Punkt): `c0=y0; c1=0.5(y1−y−1); c2=y−1−2.5y0+2y1−0.5y2; c3=0.5(y2−y−1)+1.5(y0−y1); out=((c3x+c2)x+c1)x+c0`
   - `Linear`
   - `Raw`: Index abschneiden, als Lo-Fi-Charakter gedacht
6. **Hüllkurve pro Hit:** Fade-in über `declick` ms als Raised Cosine `0.5−0.5cos(πt/T)` (bei 0 kein Fade). Bei `gateEff < 100 %` endet der Hit nach `gateEff × Periode` mit Fade-out über `declick`, wobei die Periode beim Hit-Start gilt. `gateEff = clamp(gate + Σ lfo×to_gate, 1, 100)`. Erreicht die Wiedergabe das Fensterende, endet der Hit (mit Mini-Fade, falls `declick > 0`).
7. **Gain:** `hitGain` aus Sequencer-Velocity oder `ab_gain`.

**Retrigger-Modus** (`trig_mode`):
- `Cut` (Default): Ein neuer Hit beendet den vorherigen. Bei `declick = 0` geschieht das hart am Sample, bei `declick > 0` als Crossfade über `declick`. Dafür reichen 2 Stimmen pro Lane.
- `Overlap`: Hits klingen ineinander aus, Pool mit `voices` Stimmen pro Lane. Gestohlen wird die älteste Stimme (0,5 ms Fade). **Globales Limit: 64 aktive HitVoices**; pro Lane gilt `min(voices, 64 / uni_voices)`.

### 6.4 Engine: Noten, Hüllkurven, Modulation

- **Notenstack:** mono, Last-Note-Priority. Beim Loslassen kehrt die Engine zur noch gehaltenen Note zurück (legato). Sustain (CC64) hält die Noten.
- **Note-ADSR** (`env_a/d/s/r`) auf die Lane-Summe. Ist sie nach dem Release idle, stoppen Clocks und Voices.
- **Velocity:** `gain = 1 − vel_sens + vel_sens × vel/127`.
- **Pitchbend** ±`pb_range` Halbtöne auf die Rate (beide Modi), geglättet ~5 ms.
- **Rampe** (`ramp_*`): Startet bei neuer Note, bei `ramp_legato` auch bei Legato. Der Wert in Oktaven läuft von `ramp_start` nach `ramp_end` über `ramp_time`, Kurve `y = x^(2^(3×ramp_curve))` (0 = linear in Oktaven). Modus Einmal (Ende halten), Loop oder PingPong. *Beispiel Speedup 300 → 8.000 BPM:* Basis 8.000 BPM, `ramp_start = −4,74`, `ramp_end = 0`, `ramp_time = 2000`. Die GUI-Anzeige zeigt Start- und End-BPM-Äquivalent.
- **LFO 1/2:**
  - Formen: Sinus, Dreieck, Säge auf, Säge ab, Rechteck, S&H, Zufall weich (Cosinus-Interpolation).
  - Bipolar −1…+1, Auswertung pro Sample.
  - Takt `Frei` (`lfoN_hz`), `Projekt-Sync` (`lfoN_div` relativ zum Host-Tempo) oder `Rate-Verhältnis`. Bei Rate-Verhältnis gilt `f = rateHz × ratio`, und die Phase wird alle 1/ratio Hits von Lane 1 auf `lfoN_phase` zurückgesetzt. So bleibt der LFO exakt gekoppelt, das ergibt periodische Klangfarben-Muster bzw. FM-artige Effekte.
  - Bei `lfoN_retrig` Reset bei neuer Note.
  - 4 Ziele mit eigenem Depth-Regler: Rate (kontinuierlich), Scan und Gate (abgetastet pro Hit), Sample-Pitch (kontinuierlich).
- **Unison** (`uni_*`): `uni_voices` Lanes, Detune gleichmäßig in ±`uni_detune` Cent verteilt, Pan gleichmäßig × `uni_spread`, Gain-Kompensation 1/√N. Bei N = 1: kein Detune, Mitte.

### 6.5 Slice-Wahl pro Hit (Priorität von oben nach unten)

1. **Sequencer an:** Der Step des aktuellen Ticks entscheidet. Inaktiver Step: kein neuer Hit, die laufende Stimme wird mit `declick` beendet. `tie`: kein neuer Hit, die laufende Stimme spielt weiter. `slice = −1`: „aktueller Slice“ (Param/MIDI), sonst der absolute Index. Dazu kommen Step-`pitch` (Sample-Pitch) und `velocity`. Der Sequencer schaltet **pro Clock-Tick** einen Step weiter. Bei hoher Rate entsteht so eine zusammengesetzte Wellenform mit Grundton `rate / seq_len` – das ist gewollt.
2. **A/B an** (nur ohne Sequencer): Muster AB, AAB, ABB oder Zufall pro Tick. B = `ab_slice`, `ab_pitch`, `ab_gain`. Bei AB wiederholt sich das Muster erst nach 2 Hits, dadurch entsteht eine Subharmonische eine Oktave tiefer.
3. **Sonst:** `midi_mode = Slice` → `slice = (note − slice_base) + slice` (modulo Anzahl Slices); `midi_mode = Pitch` → `slice`-Parameter (modulo Anzahl).

Jede Lane hat eigene Tick-Zähler für Sequencer und A/B. Reset bei neuer Note gilt, wenn `seq_reset` gesetzt ist bzw. für A/B immer.

### 6.6 Slices & Transienten

- `SliceModel`: sortierte Marker in Quell-Frames, dazu implizit 0 und `frames`. Maximal 127 Marker (also 128 Slices). Nach dem Laden gibt es 1 Slice (das ganze Sample).
- **Gleichmäßig:** N Slices (2–64), GUI-Aktion.
- **Transienten:** Offline im Hintergrund-Thread. Mono-Summe, Hop 256, Energie mit Hochfrequenz-Gewichtung (HFC aus Differenzsignal). Danach Onset-Funktion = positive Differenz der log-Energie, adaptive Schwelle (gleitender Median über 15 Hops × (1 + 2 × (1 − sens))), Peak-Picking mit Mindestabstand 30 ms. Marker optional auf einen Nulldurchgang ≤ 2 ms davor schieben. Kein FFT nötig. Die Zahlen sind Startwerte und werden mit echten Drum-Loops nachjustiert.
- **Manuell** (WaveformView): Doppelklick setzt einen Marker, Ziehen verschiebt ihn, Rechtsklick löscht ihn. Mausrad zoomt, Shift + Mausrad scrollt. Equal und Transient erzeugen Marker, die danach ganz normal manuell editierbar sind.
- Slice-Count und Sensitivität sind GUI-Steuerelemente im Wellenform-Bereich, **keine** automatisierbaren Parameter.

### 6.7 Ausgangsstufe

- `os`: 1x / **2x (Default)** / 4x / 8x über `juce::dsp::Oversampling<float>` (Halbband-Polyphase-IIR, niedrige Latenz). Alle vier Oversampler werden in `prepareToPlay` erzeugt, umgeschaltet wird per Index. Latenz per `setLatencySamples` melden. Ablauf: `processSamplesUp` mit genulltem Block liefert den OS-Block, die Engine rendert hinein, danach `processSamplesDown`.
- `drive` 0–48 dB Vorverstärkung → `clip`: Aus, Soft (tanh) oder Hard (±1,0) → DC-Blocker (1-Pol-HP ~5 Hz) → `out_gain`. Alles auf OS-Rate.
- Nach dem Downsampling sind kleine Überschwinger über 0 dBFS normal (Gibbs-Effekt am Filter bei Rechteck-Signalen; im Prototyp mit Drive 24 dB bis ca. +2,5 dB). Deshalb steht `out_gain` per Default auf −3 dB. Ein zusätzlicher Clip nach dem Downsampling ist **nicht** vorgesehen, weil er wieder Aliasing erzeugen würde.
- `mono`: Das Sample wird vor dem Panning auf mono summiert (Default an, siehe Referenz).

### 6.8 Default-Kick (synthetisch)

Wird beim Start erzeugt, wenn kein Sample im State steht. Damit macht das Plugin sofort Ton und Tests brauchen kein Asset.
- Spezifikation: 48 kHz, 450 ms, Sinus mit Pitch-Drop `f(t) = 45 + 135 × exp(−t/0,025 s)` Hz, Amplitude `exp(−t/0,18 s)`, dazu ein 2-ms-Klick (gefiltertes Rauschen, −12 dB), normalisiert auf −1 dBFS.

---

## 7. Parameter (stabile IDs – nach dem ersten Release nie umbenennen oder entfernen)

`juce::ParameterID{ id, 1 }`. Gruppiert über `AudioProcessorParameterGroup` (= GUI-Tabs). Neue Parameter nur anhängen. Nicht automatisierbare Daten (Marker, Steps, Sample) liegen im State (§8).

| Gruppe | ID | Name | Bereich | Default |
|---|---|---|---|---|
| Rate | `rate_src` | Rate-Quelle | BPM, Hz, Host× | BPM |
| | `bpm` | Plugin-BPM | 1–60.000 (log) | 500 |
| | `division` | Teiler | 1/1, 1/2, 1/4, 1/8, 1/16, 1/32, 1/64 | 1/16 |
| | `div_mod` | Teiler-Art | Gerade, Triole, Punktiert | Gerade |
| | `hz` | Rate Hz | 0,1–4.000 (log) | 33,33 |
| | `host_mult` | Host-Multiplikator | ×1, ×2, ×4, ×8, ×16, ×32, ×64, ×128 | ×1 |
| | `host_lock` | Host-Phasenlock | an/aus | an |
| | `rate_smooth` | Rate-Glättung | 0–500 ms | 2 |
| MIDI | `midi_mode` | MIDI-Modus | Pitch, Slice | Pitch |
| | `key_mode` | Keytrack | Relativ, Absolut | Relativ |
| | `key_root` | Grundnote | 0–127 (Anzeige „C4 / FL C5“) | 60 |
| | `slice_base` | Slice-Startnote | 0–127 | 60 |
| | `transpose` | Transponieren | −48…+48 st | 0 |
| | `fine` | Feinstimmung | −100…+100 ct | 0 |
| | `a4_ref` | Stimmton A4 | 400–480 Hz | 440 |
| | `glide` | Glide | 0–5.000 ms (skew) | 0 |
| | `glide_legato` | Glide nur legato | an/aus | an |
| | `pb_range` | Pitchbend-Bereich | 0–48 st | 12 |
| | `phase_reset` | Phase-Reset bei Note | an/aus | an |
| | `vel_sens` | Velocity-Empfindl. | 0–100 % | 50 |
| Snap | `snap_mode` | Tune-Snap | Aus, Basis, Alles | Aus |
| | `snap_scale` | Skala | Chromatisch, Dur, Moll, Harm. Moll, Phrygisch, Moll-Penta, Nur Grundton | Chromatisch |
| | `snap_root` | Grundton | C…B | C |
| Retrigger | `trig_mode` | Modus | Cut, Overlap | Cut |
| | `declick` | Declick | 0–5 ms | 0 |
| | `gate` | Gate | 1–100 % | 100 |
| | `voices` | Overlap-Stimmen | 1–32 | 8 |
| | `jitter` | Jitter | 0–100 % | 0 |
| Sample | `slice` | Slice | 0–127 | 0 |
| | `scan` | Start-Scan | 0–100 % | 0 |
| | `length` | Länge | 1–100 % | 100 |
| | `reverse` | Reverse | an/aus | aus |
| | `interp` | Interpolation | Hermite, Linear, Raw | Hermite |
| | `mono` | Mono | an/aus | an |
| | `s_pitch` | Sample-Pitch | −48…+48 st | 0 |
| | `s_fine` | Sample-Fein | −100…+100 ct | 0 |
| | `penv_amt` | Pitch-Env Menge | −48…+48 st | 0 |
| | `penv_decay` | Pitch-Env Decay | 0,1–2.000 ms (skew) | 50 |
| A/B | `ab_mode` | A/B-Modus | Aus, AB, AAB, ABB, Zufall | Aus |
| | `ab_slice` | Slice B | 0–127 | 1 |
| | `ab_pitch` | Pitch B | −24…+24 st | 0 |
| | `ab_gain` | Gain B | −24…+6 dB | 0 |
| Rampe | `ramp_on` | Rampe an | an/aus | aus |
| | `ramp_start` | Start | −8…+8 Okt. | −2 |
| | `ramp_end` | Ende | −8…+8 Okt. | 0 |
| | `ramp_time` | Zeit | 1–30.000 ms (skew) | 1.000 |
| | `ramp_curve` | Kurve | −1…+1 | 0 |
| | `ramp_loop` | Wiederholung | Einmal, Loop, PingPong | Einmal |
| | `ramp_legato` | Neu bei Legato | an/aus | aus |
| LFO 1 / LFO 2 (N = 1, 2) | `lfoN_shape` | Form | Sinus, Dreieck, Säge auf, Säge ab, Rechteck, S&H, Zufall weich | Sinus |
| | `lfoN_sync` | Takt | Frei, Projekt-Sync, Rate-Verhältnis | Frei |
| | `lfoN_hz` | Frequenz | 0,01–100 Hz (log) | 1 |
| | `lfoN_div` | Projekt-Teiler | 8/1, 4/1, 2/1, 1/1, 1/2, 1/4, 1/8, 1/16, 1/32, 1/4T, 1/8T, 1/16T, 1/4., 1/8., 1/16. | 1/4 |
| | `lfoN_ratio` | Rate-Verhältnis | 1/1, 1/2, 1/3, 1/4, 1/6, 1/8, 1/12, 1/16, 1/32, 1/64 | 1/8 |
| | `lfoN_retrig` | Retrigger bei Note | an/aus | an |
| | `lfoN_phase` | Startphase | 0–360° | 0 |
| | `lfoN_to_rate` | → Rate | ±24 st | 0 |
| | `lfoN_to_scan` | → Start-Scan | ±100 % | 0 |
| | `lfoN_to_gate` | → Gate | ±100 % | 0 |
| | `lfoN_to_pitch` | → Sample-Pitch | ±24 st | 0 |
| Unison | `uni_voices` | Stimmen | 1–8 | 1 |
| | `uni_detune` | Verstimmung | 0–100 ct | 10 |
| | `uni_spread` | Stereobreite | 0–100 % | 50 |
| | `uni_phase` | Zufallsphase | an/aus | an |
| Sequencer | `seq_on` | Sequencer an | an/aus | aus |
| | `seq_len` | Länge | 1–32 | 16 |
| | `seq_reset` | Reset bei Note | an/aus | an |
| Hüllkurve | `env_a` | Attack | 0–5.000 ms (skew) | 0 |
| | `env_d` | Decay | 1–10.000 ms (skew) | 100 |
| | `env_s` | Sustain | 0–100 % | 100 |
| | `env_r` | Release | 0–10.000 ms (skew) | 5 |
| Ausgang | `os` | Oversampling | 1x, 2x, 4x, 8x | 2x |
| | `drive` | Drive | 0–48 dB | 0 |
| | `clip` | Clipper | Aus, Soft, Hard | Hard |
| | `out_gain` | Ausgang | −48…+12 dB | −3 |

Insgesamt 86 Parameter. Wertanzeigen mit Einheit. `bpm` und `hz` zeigen zusätzlich die Note an (z. B. „500 BPM“, Tooltip „8,33 Hz · C-1 +33 ct“).

---

## 8. State (Speichern/Laden in der FL-Projektdatei)

Binärformat: `ValueTree::writeToStream` mit Magic `XTN1` und `stateVersion` (int, beginnt bei 1, Migrationen bei Formatwechseln).

```
XTONE_STATE (stateVersion=1)
├─ PARAMS          (APVTS-State)
├─ SAMPLE          name, origPath, srcRate, frames, channels, embedded=true|false, data=<MemoryBlock: Original-Dateibytes>
├─ SLICES          markers=<MemoryBlock int64[]>
└─ SEQ             32 × STEP {on, tie, slice(−1..127), pitch(−24..24), vel(0..1)}
```
- Die Original-Datei wird **eingebettet**, solange sie ≤ 32 MB ist. So funktioniert die .flp auch dann noch, wenn die WAV verschoben wird. Größere Dateien werden nur per Pfad referenziert, mit Hinweis in der GUI.
- Fehlt beim Laden ein referenziertes Sample, kommt die Default-Kick und die GUI meldet „Sample fehlt: …“.
- Den serialisierten SAMPLE-Block cachen, damit `getStateInformation` (wird von FL oft aufgerufen) nicht jedes Mal neu kodiert.
- Sequencer-Steps sind im Plugin atomare Felder (GUI schreibt, Audio liest) und landen beim Speichern im State.

---

## 9. GUI (funktional)

Fenster ca. 1000 × 720, skalierbar (Resizer). Standard-LookAndFeel, kein Styling-Aufwand.

1. **Kopfzeile:** [Laden…] (async `FileChooser`), Drag & Drop (auch aus dem FL-Browser), Dateiname, [Normalisieren], Preset-Menü (Factory + [Speichern…]/[Laden…] als `.xtone`-Datei = kompletter State).
   **Live-Anzeige (30 Hz):** `Rate 33,33 Hz · 2.000 BPM-Äq. · Plugin 500 BPM @1/16 · C1 +33 ct (FL C2) · Slice 3/8 · Step 5/16`
2. **Wellenform** (volle Breite, ~200 px): Min/Max-Peak-Cache, Slice-Marker, aktueller Slice hervorgehoben, Scan-Fenster schattiert, Positionsanzeige (Atomic vom Audio-Thread). Rechts: [Gleichmäßig] + Zahl (2–64), [Transienten] + Sensitivität, [Marker löschen].
3. **Sequencer-Raster** (32 Spalten, ~150 px, einklappbar): Zeilen An, Tie, Slice, Pitch, Velocity. Werte per Klick und Ziehen. Buttons [Leeren], [Zufall], [◄]/[►] zum Verschieben. Der aktive Step wird hervorgehoben.
4. **Parameter-Tabs:** pro Parametergruppe ein Tab, Regler automatisch aus dem APVTS erzeugt (Float → Slider, Choice → ComboBox, Bool → Toggle, jeweils mit Attachment und Label).

---

## 10. Tests & Abnahme

### 10.1 Automatische DSP-Tests (Catch2, laufen in CI unter Linux und Windows)

| # | Test | Kriterium |
|---|---|---|
| T1 | RateMath | 500 BPM 1/16 = 33,333 Hz; punktiert = 22,222; Triole = 50,000; Note 69 = 440 Hz; Relativ: Grundnote 60, Note 72 → ×2 |
| T2 | Tune-Snap | 8,333 Hz → C-1 (8,176 Hz) chromatisch; Skala „Nur Grundton“ snappt nur auf C |
| T3 | Clock-Genauigkeit | 1234,5678 Hz bei 48 kHz, 10 s: mittlere Frequenz aus Hit-Zeitpunkten (inkl. subOffset) relativer Fehler < 1e−9; subOffset ∈ [0,1) |
| T4 | **Phasen-Kontinuität** | Sample = Sinus f₀ (z. B. 123,4 Hz), Rate = f₀, Cut, Declick 0, Hermite → Ausgang ist ein kontinuierlicher Sinus, THD+N < −60 dB (Prototyp: −65,9 dB; ohne Sub-Sample-Start nur −46,6 dB, der Test trennt also sauber) |
| T5 | Gate | Gate 50 % → zweite Periodenhälfte nach dem Fade ≈ 0 (< −90 dBFS) |
| T6 | Voices | Overlap: aktive Stimmen ≤ `voices` und ≤ 64 global; Cut: ≤ 2 pro Lane |
| T7 | Jitter | Jitter 0 deterministisch; Jitter 100 % mittlere Rate ±1 % |
| T8 | Sequencer | Step pro Tick, Wrap bei `seq_len`, inaktiv = stumm, Tie = weiterklingen |
| T9 | A/B | Reihenfolge der Muster AB/AAB/ABB korrekt; AB → Grundperiode verdoppelt (Autokorrelation) |
| T10 | Unison | Detune-Verteilung symmetrisch, Gain-Kompensation 1/√N |
| T11 | Rampe/LFO | Rampe erreicht `ramp_end` nach `ramp_time` ±1 Sample; LFO-Bereich [−1,1]; Rate-Verhältnis bleibt phasengekoppelt |
| T12 | Onsets | Synthetisches Signal mit 4 Klicks an bekannten Positionen → 4 Marker innerhalb ±5 ms |
| T13 | **Blockgrößen-Invarianz** | Gleiches MIDI und gleiche Parameter mit Blockgrößen 1, 7, 64, 256, 4096 sowie größer als angekündigt → identischer Output (max. Abweichung < 1e−6) |
| T14 | Robustheit | Extremwerte (60.000 BPM, 8 Unison, 32 Voices, Rampe ±8 Okt.) → kein NaN/Inf; mit Hard-Clip und `out_gain` 0 dB Peak ≤ 1,5 (Überschwinger durch DC-Blocker/Downsampling zulässig, siehe §6.7) |
| T15 | Referenz-Charakter | Default-Kick, 500 BPM, 1/16, 2x OS, Drive 24 dB Hard → Grundperiode (Autokorrelation) 30,00 ms ±0,05 ms, Crest-Faktor < 4 dB (Prototyp: 2,6 dB; Referenztrack: 1,4 dB) |

### 10.2 Plugin-Validierung in CI
`pluginval --strictness-level 5 --skip-gui-tests --validate XTONE.vst3` muss bestehen.

### 10.3 Manuelle Abnahme in FL Studio (durch Pascal, nach jedem Meilenstein)
- [ ] Plugin-Scan findet XTONE, es lädt als Generator, die Default-Kick klingt.
- [ ] Piano-Roll-Noten in Oktaven → saubere Oktavsprünge der Rate.
- [ ] Automation-Clip auf `bpm` (Sweep ohne Zipper) und `division` (sofortige Stufen).
- [ ] Projekt speichern, schließen, die WAV verschieben, das Projekt öffnen → Sample, Marker, Steps und Parameter sind wieder da.
- [ ] Export (WAV-Render) klingt identisch zum Abspielen; Pitch bei 44,1 / 48 / 96 kHz identisch.
- [ ] Projekttempo 130 vs. 500: In BPM/Hz-Modus ändert sich nichts, Host× folgt dem Projekt.
- [ ] Oversampling während der Wiedergabe umschalten → kein Absturz.
- [ ] CPU-Last mit Worst-Case-Preset (8 Unison, 4x OS, Overlap 32) auf Pascals Rechner notieren.

---

## 11. Meilensteine (nach jedem: CI grün, Pre-Release-Tag, kurze deutsche Testanleitung in den Release-Notes, auf Feedback warten)

| MS | Inhalt | Abnahme |
|---|---|---|
| **M0** Gerüst | Repo, Submodule, CMake, CI (Linux-Tests + Windows-Build + pluginval + Artefakt), Plugin mit Default-Kick bei fester Rate, generische Parameter-UI | Pascal installiert die `.vst3` aus dem Actions-Artefakt und hört einen Ton in FL |
| **M1** Kern-Engine | Rate-Quellen BPM/Hz/Host×, Teiler, Sub-Sample-Clock, Cut + Declick, Gate, Sample-Pitch, Sample laden (Dialog + Drag & Drop), State mit Einbettung, Note-ADSR, Pitch-Modus Relativ/Absolut, Glide, Pitchbend, Live-Anzeige | T1, T3, T4, T5, T13, T14 |
| **M2** Chop | SliceModel, WaveformView mit Marker-Editing und Zoom, Gleichmäßig/Transienten, Slice-Modus, Scan/Länge/Reverse, Interpolationsmodi, Overlap, Pitch-Env | T6, T12 |
| **M3** Charakter | Oversampling, Drive/Clip/DC, Mono, A/B, Jitter, Tune-Snap | T2, T7, T9, T15 |
| **M4** Bewegung | Rampe, LFO 1/2 inkl. Rate-Verhältnis, Unison | T10, T11 |
| **M5** Sequencer | StepSequencer + Raster-GUI | T8 |
| **M6** Release | Factory-Programme (über `getNumPrograms`/`setCurrentProgram`, damit sie im FL-Preset-Menü erscheinen), User-Presets, README (deutsch: Installation + Parameterreferenz), Tag `v1.0.0` → GitHub Release | komplette Checkliste §10.3 |

**Factory-Presets** (nur Parameter, das aktuelle Sample bleibt): *Speed Grid 500* (Referenz: 500 BPM, 1/16, Pitch Relativ, Cut, Gate 100, Drive 24 dB Hard, Mono) · *Melodic A440* (Absolut, Glide 30 ms) · *Speedup Rise* (Rampe −4,74 → 0 Okt. in 2 s) · *Scan Wobble* (LFO1 → Scan) · *Supersaw Kick* (6 Unison, 15 ct, 80 %) · *Sub Octave AB* · *Glitch Seq* (Sequencer 16 Steps bei 300 BPM 1/16) · *Dirty Jitter*.

---

## 12. Build & CI

### 12.1 CMakeLists.txt (Gerüst)

```cmake
cmake_minimum_required(VERSION 3.25)
project(XTONE VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")

option(XTONE_BUILD_PLUGIN "Plugin bauen (braucht JUCE-Submodule)" ON)
option(XTONE_BUILD_TESTS  "DSP-Tests bauen" ON)

add_subdirectory(dsp)                 # add_library(xtone_dsp STATIC ...) – reines C++

if (XTONE_BUILD_PLUGIN)
    add_subdirectory(JUCE)
    juce_add_plugin(Xtone
        COMPANY_NAME             "XTONE Dev"      # TODO vor Release festlegen
        PLUGIN_MANUFACTURER_CODE Xtdv             # 4 Zeichen, min. 1 Großbuchstabe – nach Release NIE ändern
        PLUGIN_CODE              Xtn1             # 4 Zeichen, eindeutig – nach Release NIE ändern
        PRODUCT_NAME             "XTONE"
        FORMATS                  VST3 Standalone
        IS_SYNTH                 TRUE
        NEEDS_MIDI_INPUT         TRUE
        NEEDS_MIDI_OUTPUT        FALSE
        IS_MIDI_EFFECT           FALSE
        COPY_PLUGIN_AFTER_BUILD  FALSE
        VST3_CATEGORIES          Instrument Sampler)
    target_sources(Xtone PRIVATE
        plugin/PluginProcessor.cpp plugin/Parameters.cpp plugin/SampleLoader.cpp plugin/Presets.cpp
        plugin/gui/PluginEditor.cpp plugin/gui/HeaderBar.cpp plugin/gui/WaveformView.cpp
        plugin/gui/SequencerGrid.cpp plugin/gui/ParamPanel.cpp)
    target_compile_definitions(Xtone PUBLIC
        JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_USE_MP3AUDIOFORMAT=1 JUCE_USE_FLAC=1 JUCE_USE_OGGVORBIS=1)
    target_link_libraries(Xtone
        PRIVATE xtone_dsp juce::juce_audio_utils juce::juce_dsp
        PUBLIC  juce::juce_recommended_config_flags juce::juce_recommended_lto_flags
                juce::juce_recommended_warning_flags)
endif()

if (XTONE_BUILD_TESTS)
    include(FetchContent)
    FetchContent_Declare(Catch2 GIT_REPOSITORY https://github.com/catchorg/Catch2.git
                                GIT_TAG v3.8.1 GIT_SHALLOW TRUE)
    FetchContent_MakeAvailable(Catch2)
    enable_testing()
    add_subdirectory(tests)           # add_executable(xtone_tests ...) + catch_discover_tests
endif()
```

### 12.2 `.github/workflows/build.yml`

```yaml
name: build
on:
  push:
    branches: [main]
    tags: ['v*']
  pull_request:
  workflow_dispatch:

jobs:
  dsp-tests:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v5            # submodules nicht nötig
      - run: cmake -B build -DCMAKE_BUILD_TYPE=Release -DXTONE_BUILD_PLUGIN=OFF
      - run: cmake --build build --parallel
      - run: ctest --test-dir build --output-on-failure

  windows:
    needs: dsp-tests
    runs-on: windows-latest                  # Fallback bei Toolchain-Problemen: windows-2022
    steps:
      - uses: actions/checkout@v5
        with:
          submodules: true
      - name: Configure
        run: cmake -B build -A x64
      - name: Build
        run: cmake --build build --config Release --parallel
      - name: DSP tests
        run: ctest --test-dir build -C Release --output-on-failure
      - name: pluginval
        shell: pwsh
        run: |
          Invoke-WebRequest https://github.com/Tracktion/pluginval/releases/latest/download/pluginval_Windows.zip -OutFile pluginval.zip
          Expand-Archive pluginval.zip -DestinationPath pluginval
          ./pluginval/pluginval.exe --strictness-level 5 --skip-gui-tests --validate "build/Xtone_artefacts/Release/VST3/XTONE.vst3"
          if ($LASTEXITCODE -ne 0) { exit 1 }
      - name: Package
        shell: pwsh
        run: |
          New-Item -ItemType Directory -Force dist | Out-Null
          Copy-Item -Recurse "build/Xtone_artefacts/Release/VST3/XTONE.vst3" dist/
          Copy-Item "build/Xtone_artefacts/Release/Standalone/XTONE.exe" dist/
          Copy-Item README.md dist/ -ErrorAction SilentlyContinue
          Compress-Archive -Path dist/* -DestinationPath XTONE-win64.zip -Force
      - uses: actions/upload-artifact@v5
        with:
          name: XTONE-win64
          path: XTONE-win64.zip
      - name: GitHub Release (nur bei Tag)
        if: startsWith(github.ref, 'refs/tags/v')
        uses: softprops/action-gh-release@v2
        with:
          files: XTONE-win64.zip
          prerelease: ${{ contains(github.ref, '-') || startsWith(github.ref, 'refs/tags/v0.') }}
```
Hinweise:
- Die Action-Versionen beim Anlegen auf die aktuellen Major-Versionen prüfen.
- Lässt sich der VS-Generator auf dem Runner nicht auflösen, auf `-G Ninja` mit MSVC-Devshell umstellen oder auf `windows-2022` pinnen.
- Private Repos: Windows-Minuten zählen mehrfach gegen das Freikontingent. Öffentliche Repos sind kostenlos.

### 12.3 Installation für Pascal (gehört auch ins README)
1. GitHub → Actions → letzter grüner Lauf → Artefakt `XTONE-win64` herunterladen (oder unter Releases).
2. ZIP entpacken und den Ordner `XTONE.vst3` nach `C:\Program Files\Common Files\VST3\` kopieren.
3. FL Studio → *Options → Manage plugins → Find installed plugins* → XTONE als Favorit markieren → im Channel Rack hinzufügen.
4. Update: FL schließen, den Ordner ersetzen, FL neu starten.

### 12.4 Vor dem ersten echten Projekt festlegen (irreversibel)
`PRODUCT_NAME`, `COMPANY_NAME`, `PLUGIN_MANUFACTURER_CODE`, `PLUGIN_CODE`. Werden sie später geändert, findet FL das Plugin in alten Projekten nicht mehr. Mit Pascal klären, spätestens vor dem Tag `v0.2.0`.

---

## 13. Arbeitsweise für den Implementierer

- Dieses Dokument ist die Quelle der Wahrheit. Abweichungen und eigene Entscheidungen kurz in `docs/DECISIONS.md` festhalten.
- Meilenstein für Meilenstein vorgehen, kleine Commits, CI immer grün halten. Nach jedem Meilenstein Pre-Release `v0.<MS>.0` taggen und eine deutsche Testanleitung in die Release-Notes schreiben.
- Zurückfragen nur bei irreversiblen Dingen (§12.4) oder wenn eine Vorgabe hier technisch nicht umsetzbar ist. Sonst sinnvoll entscheiden und dokumentieren.
- Für den DSP-Kern gilt: erst der Test, dann die Implementierung. Das gilt besonders für T3, T4 und T13, das Herz des Plugins.
- Code-Bezeichner und Kommentare auf Englisch, Nutzertexte (GUI, README, Release-Notes) auf Deutsch.

## 14. Später (nicht v1)
CLAP-Format (clap-juce-extensions), mehrere Sample-Layer, Modulationsmatrix mit freien Zielen, MPE, eigenes Skin, macOS-Build.

## Quellen
- FL Studio Manual, Toolbar (Tempo 10–522 BPM): https://www.image-line.com/fl-studio-learning/fl-studio-online-manual/html/toolbar_panels.htm
- FL Studio Manual, Plugin-Standards (Windows: VST2/VST3/CLAP): https://www.image-line.com/fl-studio-learning/fl-studio-online-manual/html/plugins_supported.htm
- HISE-Forum, FL liefert größere Buffer als angekündigt: https://forum.hise.audio/post/57611
- JUCE Releases: https://github.com/juce-framework/JUCE/releases
- GitHub Runner Images (windows-latest mit VS 2026): https://github.com/actions/runner-images
- pluginval: https://github.com/Tracktion/pluginval
