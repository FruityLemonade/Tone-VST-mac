#include "Parameters.h"

#include "RateMath.h"

namespace xtp
{
using namespace juce;
using APF = AudioParameterFloat;
using APC = AudioParameterChoice;
using APB = AudioParameterBool;
using API = AudioParameterInt;

namespace
{
    String U (const char* s) { return String::fromUTF8 (s); }

    ParameterID pid (const char* id) { return ParameterID { id, 1 }; }

    NormalisableRange<float> logRange (float lo, float hi)
    {
        return NormalisableRange<float> (
            lo, hi,
            [] (float a, float b, float t) { return a * std::pow (b / a, t); },
            [] (float a, float b, float v) { return std::log (v / a) / std::log (b / a); },
            [] (float a, float b, float v) { return jlimit (a, b, v); });
    }

    NormalisableRange<float> skewRange (float lo, float hi, float centre, float interval = 0.0f)
    {
        NormalisableRange<float> r (lo, hi, interval);
        r.setSkewForCentre (centre);
        return r;
    }

    String fmt (float v, int decimals, const String& unit)
    {
        return String (v, decimals) + (unit.isNotEmpty() ? " " + unit : String());
    }

    auto unitAttr (const String& unit, int decimals)
    {
        return AudioParameterFloatAttributes().withLabel (unit).withStringFromValueFunction (
            [unit, decimals] (float v, int) { return fmt (v, decimals, unit); });
    }

    auto noteIntAttr()
    {
        return AudioParameterIntAttributes().withStringFromValueFunction (
                                              [] (int v, int) { return String (xt::noteNameBoth (v)); })
            .withValueFromStringFunction ([] (const String& s) { return s.getIntValue(); });
    }

} // namespace

StringArray groupOrder()
{
    return { "Rate", "MIDI", "Snap", "Retrigger", "Sample", "A/B", "Rampe", "LFO 1", "LFO 2",
             "Unison", "Sequencer", U ("Hüllkurve"), "Ausgang" };
}

AudioProcessorValueTreeState::ParameterLayout createParameterLayout()
{
    AudioProcessorValueTreeState::ParameterLayout layout;

    const StringArray divisions { "1/1", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64" };

    // ---- Rate
    {
        auto g = std::make_unique<AudioProcessorParameterGroup> ("rate", "Rate", "|");
        g->addChild (std::make_unique<APC> (pid ("rate_src"), "Rate-Quelle", StringArray { "BPM", "Hz", "Host x" }, 0));
        g->addChild (std::make_unique<APF> (
            pid ("bpm"), "Plugin-BPM", logRange (1.0f, 60000.0f), 500.0f,
            AudioParameterFloatAttributes().withLabel ("BPM").withStringFromValueFunction (
                [] (float v, int) { return String (v, v < 100.0f ? 2 : 1) + " BPM"; })));
        g->addChild (std::make_unique<APC> (pid ("division"), "Teiler", divisions, 4));
        g->addChild (std::make_unique<APC> (pid ("div_mod"), "Teiler-Art", StringArray { "Gerade", "Triole", "Punktiert" }, 0));
        g->addChild (std::make_unique<APF> (
            pid ("hz"), "Rate Hz", logRange (0.1f, 4000.0f), 33.33f,
            AudioParameterFloatAttributes().withLabel ("Hz").withStringFromValueFunction (
                [] (float v, int) { return String (v, 2) + " Hz (" + String (xt::noteName (xt::hzToNote (v), false)) + ")"; })));
        g->addChild (std::make_unique<APC> (pid ("host_mult"), "Host-Multiplikator",
                                            StringArray { "x1", "x2", "x4", "x8", "x16", "x32", "x64", "x128" }, 0));
        g->addChild (std::make_unique<APB> (pid ("host_lock"), "Host-Phasenlock", true));
        g->addChild (std::make_unique<APF> (pid ("rate_smooth"), U ("Rate-Glättung"), skewRange (0.0f, 500.0f, 20.0f), 2.0f, unitAttr ("ms", 1)));
        layout.add (std::move (g));
    }
    // ---- MIDI
    {
        auto g = std::make_unique<AudioProcessorParameterGroup> ("midi", "MIDI", "|");
        g->addChild (std::make_unique<APC> (pid ("midi_mode"), "MIDI-Modus", StringArray { "Pitch", "Slice" }, 0));
        g->addChild (std::make_unique<APC> (pid ("key_mode"), "Keytrack", StringArray { "Relativ", "Absolut" }, 0));
        g->addChild (std::make_unique<API> (pid ("key_root"), "Grundnote", 0, 127, 60, noteIntAttr()));
        g->addChild (std::make_unique<API> (pid ("slice_base"), "Slice-Startnote", 0, 127, 60, noteIntAttr()));
        g->addChild (std::make_unique<APF> (pid ("transpose"), "Transponieren", NormalisableRange<float> (-48.0f, 48.0f, 1.0f), 0.0f, unitAttr ("st", 0)));
        g->addChild (std::make_unique<APF> (pid ("fine"), "Feinstimmung", NormalisableRange<float> (-100.0f, 100.0f, 0.1f), 0.0f, unitAttr ("ct", 1)));
        g->addChild (std::make_unique<APF> (pid ("a4_ref"), "Stimmton A4", NormalisableRange<float> (400.0f, 480.0f, 0.01f), 440.0f, unitAttr ("Hz", 2)));
        g->addChild (std::make_unique<APF> (pid ("glide"), "Glide", skewRange (0.0f, 5000.0f, 200.0f), 0.0f, unitAttr ("ms", 1)));
        g->addChild (std::make_unique<APB> (pid ("glide_legato"), "Glide nur legato", true));
        g->addChild (std::make_unique<APF> (pid ("pb_range"), "Pitchbend-Bereich", NormalisableRange<float> (0.0f, 48.0f, 1.0f), 12.0f, unitAttr ("st", 0)));
        g->addChild (std::make_unique<APB> (pid ("phase_reset"), "Phase-Reset bei Note", true));
        g->addChild (std::make_unique<APF> (pid ("vel_sens"), "Velocity-Empfindl.", NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f, unitAttr ("%", 1)));
        layout.add (std::move (g));
    }
    // ---- Snap
    {
        auto g = std::make_unique<AudioProcessorParameterGroup> ("snap", "Snap", "|");
        g->addChild (std::make_unique<APC> (pid ("snap_mode"), "Tune-Snap", StringArray { "Aus", "Basis", "Alles" }, 0));
        g->addChild (std::make_unique<APC> (pid ("snap_scale"), "Skala",
                                            StringArray { "Chromatisch", "Dur", "Moll", "Harm. Moll", "Phrygisch", "Moll-Penta", "Nur Grundton" }, 0));
        g->addChild (std::make_unique<APC> (pid ("snap_root"), "Grundton",
                                            StringArray { "C", "C#", "D", "D#", "E", "F", "F#", "G", "G#", "A", "A#", "B" }, 0));
        layout.add (std::move (g));
    }
    // ---- Retrigger
    {
        auto g = std::make_unique<AudioProcessorParameterGroup> ("retrigger", "Retrigger", "|");
        g->addChild (std::make_unique<APC> (pid ("trig_mode"), "Modus", StringArray { "Cut", "Overlap" }, 0));
        g->addChild (std::make_unique<APF> (pid ("declick"), "Declick", NormalisableRange<float> (0.0f, 5.0f, 0.01f), 0.0f, unitAttr ("ms", 2)));
        g->addChild (std::make_unique<APF> (pid ("gate"), "Gate", NormalisableRange<float> (1.0f, 100.0f, 0.1f), 100.0f, unitAttr ("%", 1)));
        g->addChild (std::make_unique<API> (pid ("voices"), "Overlap-Stimmen", 1, 32, 8));
        g->addChild (std::make_unique<APF> (pid ("jitter"), "Jitter", NormalisableRange<float> (0.0f, 100.0f, 0.1f), 0.0f, unitAttr ("%", 1)));
        layout.add (std::move (g));
    }
    // ---- Sample
    {
        auto g = std::make_unique<AudioProcessorParameterGroup> ("sample", "Sample", "|");
        g->addChild (std::make_unique<API> (pid ("slice"), "Slice", 0, 127, 0));
        g->addChild (std::make_unique<APF> (pid ("scan"), "Start-Scan", NormalisableRange<float> (0.0f, 100.0f, 0.01f), 0.0f, unitAttr ("%", 2)));
        g->addChild (std::make_unique<APF> (pid ("length"), U ("Länge"), NormalisableRange<float> (1.0f, 100.0f, 0.01f), 100.0f, unitAttr ("%", 2)));
        g->addChild (std::make_unique<APB> (pid ("reverse"), "Reverse", false));
        g->addChild (std::make_unique<APC> (pid ("interp"), "Interpolation", StringArray { "Hermite", "Linear", "Raw" }, 0));
        g->addChild (std::make_unique<APB> (pid ("mono"), "Mono", true));
        g->addChild (std::make_unique<APF> (pid ("s_pitch"), "Sample-Pitch", NormalisableRange<float> (-48.0f, 48.0f, 0.01f), 0.0f, unitAttr ("st", 2)));
        g->addChild (std::make_unique<APF> (pid ("s_fine"), "Sample-Fein", NormalisableRange<float> (-100.0f, 100.0f, 0.1f), 0.0f, unitAttr ("ct", 1)));
        g->addChild (std::make_unique<APF> (pid ("penv_amt"), "Pitch-Env Menge", NormalisableRange<float> (-48.0f, 48.0f, 0.01f), 0.0f, unitAttr ("st", 2)));
        g->addChild (std::make_unique<APF> (pid ("penv_decay"), "Pitch-Env Decay", skewRange (0.1f, 2000.0f, 100.0f), 50.0f, unitAttr ("ms", 1)));
        layout.add (std::move (g));
    }
    // ---- A/B
    {
        auto g = std::make_unique<AudioProcessorParameterGroup> ("ab", "A/B", "|");
        g->addChild (std::make_unique<APC> (pid ("ab_mode"), "A/B-Modus", StringArray { "Aus", "AB", "AAB", "ABB", "Zufall" }, 0));
        g->addChild (std::make_unique<API> (pid ("ab_slice"), "Slice B", 0, 127, 1));
        g->addChild (std::make_unique<APF> (pid ("ab_pitch"), "Pitch B", NormalisableRange<float> (-24.0f, 24.0f, 0.01f), 0.0f, unitAttr ("st", 2)));
        g->addChild (std::make_unique<APF> (pid ("ab_gain"), "Gain B", NormalisableRange<float> (-24.0f, 6.0f, 0.1f), 0.0f, unitAttr ("dB", 1)));
        layout.add (std::move (g));
    }
    // ---- Ramp
    {
        auto g = std::make_unique<AudioProcessorParameterGroup> ("ramp", "Rampe", "|");
        g->addChild (std::make_unique<APB> (pid ("ramp_on"), "Rampe an", false));
        g->addChild (std::make_unique<APF> (pid ("ramp_start"), "Start", NormalisableRange<float> (-8.0f, 8.0f, 0.01f), -2.0f, unitAttr ("Okt.", 2)));
        g->addChild (std::make_unique<APF> (pid ("ramp_end"), "Ende", NormalisableRange<float> (-8.0f, 8.0f, 0.01f), 0.0f, unitAttr ("Okt.", 2)));
        g->addChild (std::make_unique<APF> (pid ("ramp_time"), "Zeit", skewRange (1.0f, 30000.0f, 1000.0f), 1000.0f, unitAttr ("ms", 0)));
        g->addChild (std::make_unique<APF> (pid ("ramp_curve"), "Kurve", NormalisableRange<float> (-1.0f, 1.0f, 0.01f), 0.0f, unitAttr ("", 2)));
        g->addChild (std::make_unique<APC> (pid ("ramp_loop"), "Wiederholung", StringArray { "Einmal", "Loop", "PingPong" }, 0));
        g->addChild (std::make_unique<APB> (pid ("ramp_legato"), "Neu bei Legato", false));
        layout.add (std::move (g));
    }
    // ---- LFO 1 / 2
    for (int n = 1; n <= 2; ++n)
    {
        const String id = "lfo" + String (n);
        auto P = [&] (const char* suffix) { return ParameterID { id + "_" + suffix, 1 }; };
        auto g = std::make_unique<AudioProcessorParameterGroup> (id, "LFO " + String (n), "|");
        g->addChild (std::make_unique<APC> (P ("shape"), "LFO " + String (n) + " Form",
                                            StringArray { "Sinus", "Dreieck", U ("Säge auf"), U ("Säge ab"), "Rechteck", "S&H", "Zufall weich" }, 0));
        g->addChild (std::make_unique<APC> (P ("sync"), "LFO " + String (n) + " Takt", StringArray { "Frei", "Projekt-Sync", U ("Rate-Verhältnis") }, 0));
        g->addChild (std::make_unique<APF> (P ("hz"), "LFO " + String (n) + " Frequenz", logRange (0.01f, 100.0f), 1.0f, unitAttr ("Hz", 3)));
        g->addChild (std::make_unique<APC> (P ("div"), "LFO " + String (n) + " Projekt-Teiler",
                                            StringArray { "8/1", "4/1", "2/1", "1/1", "1/2", "1/4", "1/8", "1/16", "1/32",
                                                          "1/4T", "1/8T", "1/16T", "1/4.", "1/8.", "1/16." },
                                            5));
        g->addChild (std::make_unique<APC> (P ("ratio"), "LFO " + String (n) + U (" Rate-Verhältnis"),
                                            StringArray { "1/1", "1/2", "1/3", "1/4", "1/6", "1/8", "1/12", "1/16", "1/32", "1/64" }, 5));
        g->addChild (std::make_unique<APB> (P ("retrig"), "LFO " + String (n) + " Retrigger bei Note", true));
        g->addChild (std::make_unique<APF> (P ("phase"), "LFO " + String (n) + " Startphase", NormalisableRange<float> (0.0f, 360.0f, 0.1f), 0.0f, unitAttr (U ("°"), 1)));
        g->addChild (std::make_unique<APF> (P ("to_rate"), "LFO " + String (n) + " -> Rate", NormalisableRange<float> (-24.0f, 24.0f, 0.01f), 0.0f, unitAttr ("st", 2)));
        g->addChild (std::make_unique<APF> (P ("to_scan"), "LFO " + String (n) + " -> Start-Scan", NormalisableRange<float> (-100.0f, 100.0f, 0.1f), 0.0f, unitAttr ("%", 1)));
        g->addChild (std::make_unique<APF> (P ("to_gate"), "LFO " + String (n) + " -> Gate", NormalisableRange<float> (-100.0f, 100.0f, 0.1f), 0.0f, unitAttr ("%", 1)));
        g->addChild (std::make_unique<APF> (P ("to_pitch"), "LFO " + String (n) + " -> Sample-Pitch", NormalisableRange<float> (-24.0f, 24.0f, 0.01f), 0.0f, unitAttr ("st", 2)));
        layout.add (std::move (g));
    }
    // ---- Unison
    {
        auto g = std::make_unique<AudioProcessorParameterGroup> ("unison", "Unison", "|");
        g->addChild (std::make_unique<API> (pid ("uni_voices"), "Unison-Stimmen", 1, 8, 1));
        g->addChild (std::make_unique<APF> (pid ("uni_detune"), "Verstimmung", NormalisableRange<float> (0.0f, 100.0f, 0.1f), 10.0f, unitAttr ("ct", 1)));
        g->addChild (std::make_unique<APF> (pid ("uni_spread"), "Stereobreite", NormalisableRange<float> (0.0f, 100.0f, 0.1f), 50.0f, unitAttr ("%", 1)));
        g->addChild (std::make_unique<APB> (pid ("uni_phase"), "Zufallsphase", true));
        layout.add (std::move (g));
    }
    // ---- Sequencer
    {
        auto g = std::make_unique<AudioProcessorParameterGroup> ("seq", "Sequencer", "|");
        g->addChild (std::make_unique<APB> (pid ("seq_on"), "Sequencer an", false));
        g->addChild (std::make_unique<API> (pid ("seq_len"), U ("Seq-Länge"), 1, 32, 16));
        g->addChild (std::make_unique<APB> (pid ("seq_reset"), "Seq-Reset bei Note", true));
        layout.add (std::move (g));
    }
    // ---- Envelope
    {
        auto g = std::make_unique<AudioProcessorParameterGroup> ("env", U ("Hüllkurve"), "|");
        g->addChild (std::make_unique<APF> (pid ("env_a"), "Attack", skewRange (0.0f, 5000.0f, 200.0f), 0.0f, unitAttr ("ms", 1)));
        g->addChild (std::make_unique<APF> (pid ("env_d"), "Decay", skewRange (1.0f, 10000.0f, 300.0f), 100.0f, unitAttr ("ms", 1)));
        g->addChild (std::make_unique<APF> (pid ("env_s"), "Sustain", NormalisableRange<float> (0.0f, 100.0f, 0.1f), 100.0f, unitAttr ("%", 1)));
        g->addChild (std::make_unique<APF> (pid ("env_r"), "Release", skewRange (0.0f, 10000.0f, 300.0f), 5.0f, unitAttr ("ms", 1)));
        layout.add (std::move (g));
    }
    // ---- Output
    {
        auto g = std::make_unique<AudioProcessorParameterGroup> ("out", "Ausgang", "|");
        g->addChild (std::make_unique<APC> (pid ("os"), "Oversampling", StringArray { "1x", "2x", "4x", "8x" }, 1));
        g->addChild (std::make_unique<APF> (pid ("drive"), "Drive", NormalisableRange<float> (0.0f, 48.0f, 0.1f), 0.0f, unitAttr ("dB", 1)));
        g->addChild (std::make_unique<APC> (pid ("clip"), "Clipper", StringArray { "Aus", "Soft", "Hard" }, 2));
        g->addChild (std::make_unique<APF> (pid ("out_gain"), "Ausgang", NormalisableRange<float> (-48.0f, 12.0f, 0.1f), -3.0f, unitAttr ("dB", 1)));
        layout.add (std::move (g));
    }
    return layout;
}

// ----------------------------------------------------------------------------

ParamSnapshot::ParamSnapshot (AudioProcessorValueTreeState& s)
{
    auto get = [&s] (const String& id)
    {
        auto* p = s.getRawParameterValue (id);
        jassert (p != nullptr);
        return p;
    };
    rateSrc = get ("rate_src"); bpm = get ("bpm"); division = get ("division"); divMod = get ("div_mod");
    hz = get ("hz"); hostMult = get ("host_mult"); hostLock = get ("host_lock"); rateSmooth = get ("rate_smooth");
    midiMode = get ("midi_mode"); keyMode = get ("key_mode"); keyRoot = get ("key_root"); sliceBase = get ("slice_base");
    transpose = get ("transpose"); fine = get ("fine"); a4 = get ("a4_ref"); glide = get ("glide");
    glideLegato = get ("glide_legato"); pbRange = get ("pb_range"); phaseReset = get ("phase_reset"); velSens = get ("vel_sens");
    snapMode = get ("snap_mode"); snapScale = get ("snap_scale"); snapRoot = get ("snap_root");
    trigMode = get ("trig_mode"); declick = get ("declick"); gate = get ("gate"); voices = get ("voices"); jitter = get ("jitter");
    slice = get ("slice"); scan = get ("scan"); length = get ("length"); reverse = get ("reverse"); interp = get ("interp");
    mono = get ("mono"); sPitch = get ("s_pitch"); sFine = get ("s_fine"); penvAmt = get ("penv_amt"); penvDecay = get ("penv_decay");
    abMode = get ("ab_mode"); abSlice = get ("ab_slice"); abPitch = get ("ab_pitch"); abGain = get ("ab_gain");
    rampOn = get ("ramp_on"); rampStart = get ("ramp_start"); rampEnd = get ("ramp_end"); rampTime = get ("ramp_time");
    rampCurve = get ("ramp_curve"); rampLoop = get ("ramp_loop"); rampLegato = get ("ramp_legato");
    for (int n = 0; n < 2; ++n)
    {
        const String id = "lfo" + String (n + 1) + "_";
        auto& l = lfo[n];
        l.shape = get (id + "shape"); l.sync = get (id + "sync"); l.hz = get (id + "hz"); l.div = get (id + "div");
        l.ratio = get (id + "ratio"); l.retrig = get (id + "retrig"); l.phase = get (id + "phase");
        l.toRate = get (id + "to_rate"); l.toScan = get (id + "to_scan"); l.toGate = get (id + "to_gate"); l.toPitch = get (id + "to_pitch");
    }
    uniVoices = get ("uni_voices"); uniDetune = get ("uni_detune"); uniSpread = get ("uni_spread"); uniPhase = get ("uni_phase");
    seqOn = get ("seq_on"); seqLen = get ("seq_len"); seqReset = get ("seq_reset");
    envA = get ("env_a"); envD = get ("env_d"); envS = get ("env_s"); envR = get ("env_r");
    os = get ("os"); drive = get ("drive"); clip = get ("clip"); outGain = get ("out_gain");
}

int ParamSnapshot::osIndex() const noexcept { return jlimit (0, 3, (int) std::lround (os->load())); }

void ParamSnapshot::read (xt::EngineParams& p) const noexcept
{
    auto i = [] (const std::atomic<float>* a) { return (int) std::lround (a->load (std::memory_order_relaxed)); };
    auto d = [] (const std::atomic<float>* a) { return (double) a->load (std::memory_order_relaxed); };
    auto b = [] (const std::atomic<float>* a) { return a->load (std::memory_order_relaxed) >= 0.5f; };
    auto pc = [] (const std::atomic<float>* a) { return (double) a->load (std::memory_order_relaxed) * 0.01; };

    p.rateSrc = i (rateSrc); p.bpm = d (bpm); p.division = i (division); p.divMod = i (divMod); p.hz = d (hz);
    p.hostMult = i (hostMult); p.hostLock = b (hostLock); p.rateSmoothMs = d (rateSmooth);
    p.midiMode = i (midiMode); p.keyMode = i (keyMode); p.keyRoot = i (keyRoot); p.sliceBase = i (sliceBase);
    p.transpose = d (transpose); p.fine = d (fine); p.a4 = d (a4); p.glideMs = d (glide); p.glideLegato = b (glideLegato);
    p.pbRange = d (pbRange); p.phaseReset = b (phaseReset); p.velSens = pc (velSens);
    p.snapMode = i (snapMode); p.snapScale = i (snapScale); p.snapRoot = i (snapRoot);
    p.trigMode = i (trigMode); p.declickMs = d (declick); p.gate = pc (gate); p.voices = i (voices); p.jitter = pc (jitter);
    p.slice = i (slice); p.scan = pc (scan); p.length = pc (length); p.reverse = b (reverse); p.interp = i (interp);
    p.mono = b (mono); p.sPitch = d (sPitch); p.sFine = d (sFine); p.penvAmt = d (penvAmt); p.penvDecayMs = d (penvDecay);
    p.abMode = i (abMode); p.abSlice = i (abSlice); p.abPitch = d (abPitch); p.abGainDb = d (abGain);
    p.rampOn = b (rampOn); p.rampStart = d (rampStart); p.rampEnd = d (rampEnd); p.rampTimeMs = d (rampTime);
    p.rampCurve = d (rampCurve); p.rampLoop = i (rampLoop); p.rampLegato = b (rampLegato);
    for (int n = 0; n < 2; ++n)
    {
        const auto& s = lfo[n];
        auto& l = p.lfo[n];
        l.shape = i (s.shape); l.sync = i (s.sync); l.hz = d (s.hz); l.div = i (s.div); l.ratio = i (s.ratio);
        l.retrig = b (s.retrig); l.phaseDeg = d (s.phase); l.toRate = d (s.toRate); l.toScan = pc (s.toScan);
        l.toGate = pc (s.toGate); l.toPitch = d (s.toPitch);
    }
    p.uniVoices = i (uniVoices); p.uniDetune = d (uniDetune); p.uniSpread = pc (uniSpread); p.uniPhase = b (uniPhase);
    p.seqOn = b (seqOn); p.seqLen = i (seqLen); p.seqReset = b (seqReset);
    p.envA = d (envA); p.envD = d (envD); p.envS = pc (envS); p.envR = d (envR);
    p.os = i (os); p.driveDb = d (drive); p.clip = i (clip); p.outGainDb = d (outGain);
}
} // namespace xtp
