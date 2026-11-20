#include "Engine.h"

#include "RateMath.h"

#include <algorithm>
#include <cmath>

namespace xt
{
namespace
{
    constexpr double kTwoPi = 6.28318530717958647692;

    double dbToGain (double db) noexcept { return std::pow (10.0, db / 20.0); }

    double clampRate (double hz, double fsE) noexcept
    {
        const double maxHz = std::min (4000.0, 0.45 * fsE);
        return std::clamp (hz, 0.01, maxHz);
    }
} // namespace

Engine::Engine()
{
    for (int i = 0; i < kMaxLanes; ++i)
        lanes[i].clock.setSeed (0xA5A5ull + 7919ull * (uint64_t) i);
    setEngineRate (96000.0);
    reset();
}

void Engine::setEngineRate (double newFs) noexcept
{
    fsE = newFs > 1000.0 ? newFs : 1000.0;
    adsr.setSampleRate (fsE);
    for (auto& l : lanes) l.killAll();
    paramsDirty = true;
}

void Engine::setSource (const SourceView& src) noexcept
{
    for (auto& l : lanes) l.killAll();
    source = src;
}

void Engine::setHostInfo (const HostInfo& h) noexcept
{
    host = h;
    if (params.rateSrc == 2) paramsDirty = true;
}

void Engine::reset() noexcept
{
    for (auto& l : lanes)
    {
        l.killAll();
        l.clock.reset();
    }
    stackSize = 0;
    std::fill (std::begin (sustainedFlag), std::end (sustainedFlag), false);
    sustainPedal = false;
    curNote = -1;
    hadNote = false;
    bendTarget = 0.0;
    resetPending = false;
    gliding = false;
    adsr.reset();
    running = false;
    dcX1[0] = dcX1[1] = dcY1[0] = dcY1[1] = 0.0;
    paramsDirty = true;
    updateTargets();
    paramSm.reset (paramStTarget);
    bendSm.reset (0.0);
    sPitchSm.reset (params.sPitch + params.sFine / 100.0);
    velSm.reset (1.0);
    driveSm.reset (dbToGain (params.driveDb));
    outSm.reset (dbToGain (params.outGainDb));
    glideNote = (double) params.keyRoot;
    havePpq = false;
    lastPlaying = false;
}

double Engine::noteRef() const noexcept
{
    return params.keyMode == 1 ? 69.0 : (double) params.keyRoot;
}

void Engine::updateTargets() noexcept
{
    const auto& p = params;
    switch (p.rateSrc)
    {
        case 1: srcHz = p.hz; break;
        case 2:
        {
            const double bpm = host.valid && host.bpm > 0.0 ? host.bpm : 120.0;
            const double mult = std::exp2 ((double) std::clamp (p.hostMult, 0, 7));
            srcHz = bpm * mult / 60.0 / lenQ (p.division, p.divMod);
            break;
        }
        default: srcHz = bpmToHz (p.bpm, p.division, p.divMod); break;
    }
    srcHz = std::max (srcHz, 1.0e-4);

    const bool absolute = p.midiMode == 0 && p.keyMode == 1;
    paramStTarget = 12.0 * std::log2 (absolute ? p.a4 : srcHz) + p.transpose + p.fine / 100.0;

    paramSm.setTime (p.rateSmoothMs, fsE);
    bendSm.setTime (5.0, fsE);
    sPitchSm.setTime (5.0, fsE);
    velSm.setTime (5.0, fsE);
    driveSm.setTime (20.0, fsE);
    outSm.setTime (20.0, fsE);
    {
        const double n = p.glideMs * 0.001 * fsE;
        glideCoef = n <= 0.0 ? 1.0 : 1.0 - std::exp (-1.0 / n);
    }
    adsr.setParams (p.envA, p.envD, p.envS, p.envR);
    for (auto& l : lanes) l.clock.setJitter (p.jitter);
    dcR = 1.0 - kTwoPi * 5.0 / fsE;

    configureLanes();
    updateSnapOffset();
    paramsDirty = false;
}

void Engine::configureLanes() noexcept
{
    const int n = std::clamp (params.uniVoices, 1, kMaxLanes);
    if (n == configuredLanes && params.uniDetune == configuredDetune && params.uniSpread == configuredSpread)
        return;
    for (int k = n; k < kMaxLanes; ++k) lanes[k].killAll();
    if (n > numLanes && running)
        for (int k = numLanes; k < n; ++k) lanes[k].clock.setPhase (lanes[0].clock.getPhase());
    numLanes = n;
    for (int k = 0; k < n; ++k)
    {
        const double pos = n == 1 ? 0.0 : -1.0 + 2.0 * (double) k / (double) (n - 1);
        lanes[k].detuneRatio = std::exp2 (pos * params.uniDetune / 1200.0);
        const double pan = pos * params.uniSpread;
        lanes[k].gainL = (float) std::min (1.0, 1.0 - pan);
        lanes[k].gainR = (float) std::min (1.0, 1.0 + pan);
    }
    laneGain = 1.0 / std::sqrt ((double) n);
    configuredLanes = n;
    configuredDetune = params.uniDetune;
    configuredSpread = params.uniSpread;
}

double Engine::laneDetuneCents (int i) const noexcept
{
    return 1200.0 * std::log2 (lanes[i].detuneRatio);
}

float Engine::lanePan (int i) const noexcept
{
    return lanes[i].gainR - lanes[i].gainL;
}

void Engine::updateSnapOffset() noexcept
{
    snapOffset = 0.0;
    if (params.snapMode != 1) return;
    double x = paramStTarget;
    if (params.midiMode == 0 && hadNote) x += (double) curNote - noteRef();
    const double hz = std::exp2 (x / 12.0);
    const double snapped = snapHz (hz, (Scale) std::clamp (params.snapScale, 0, (int) Scale::Count - 1),
                                   params.snapRoot, params.a4);
    snapOffset = 12.0 * std::log2 (snapped) - x;
}

// ---------------------------------------------------------------- notes

void Engine::removeNote (int note) noexcept
{
    int w = 0;
    for (int i = 0; i < stackSize; ++i)
        if (stack[i] != note) stack[w++] = stack[i];
    stackSize = w;
    sustainedFlag[note] = false;
}

void Engine::noteOn (int note, float vel) noexcept
{
    note = std::clamp (note, 0, 127);
    const bool legato = stackSize > 0;
    removeNote (note);
    stack[stackSize++] = note;

    curNote = note;
    velocity = (float) (1.0 - params.velSens + params.velSens * std::clamp ((double) vel, 0.0, 1.0));

    const bool doGlide = params.glideMs > 0.0 && hadNote && (legato || ! params.glideLegato);
    gliding = doGlide;
    if (! doGlide) glideNote = (double) note;
    hadNote = true;

    if (! legato)
    {
        const bool wasIdle = ! running;
        adsr.noteOn();
        if (wasIdle) velSm.reset (velocity);
        if (wasIdle || params.phaseReset) resetPending = true;
        running = true;
    }
    updateSnapOffset();
}

void Engine::afterRelease (int) noexcept
{
    if (stackSize == 0)
    {
        adsr.noteOff();
        return;
    }
    const int top = stack[stackSize - 1];
    if (top != curNote)
    {
        curNote = top;
        gliding = params.glideMs > 0.0;
        if (! gliding) glideNote = (double) top;
        updateSnapOffset();
    }
}

void Engine::noteOff (int note) noexcept
{
    note = std::clamp (note, 0, 127);
    bool inStack = false;
    for (int i = 0; i < stackSize; ++i) inStack |= stack[i] == note;
    if (! inStack) return;
    if (sustainPedal)
    {
        sustainedFlag[note] = true;
        return;
    }
    removeNote (note);
    afterRelease (note);
}

void Engine::handleEvent (const MidiEvent& e) noexcept
{
    switch (e.type)
    {
        case MidiEvent::NoteOn:
            if (e.value <= 0.0f) noteOff (e.note);
            else noteOn (e.note, e.value);
            break;
        case MidiEvent::NoteOff: noteOff (e.note); break;
        case MidiEvent::PitchBend: bendTarget = std::clamp ((double) e.value, -1.0, 1.0); break;
        case MidiEvent::Sustain:
        {
            const bool on = e.value >= 0.5f;
            if (sustainPedal && ! on)
            {
                sustainPedal = false;
                int w = 0;
                for (int i = 0; i < stackSize; ++i)
                {
                    if (sustainedFlag[stack[i]]) sustainedFlag[stack[i]] = false;
                    else stack[w++] = stack[i];
                }
                const bool changed = w != stackSize;
                stackSize = w;
                if (changed) afterRelease (-1);
            }
            sustainPedal = on;
            break;
        }
        case MidiEvent::AllNotesOff:
            stackSize = 0;
            sustainPedal = false;
            std::fill (std::begin (sustainedFlag), std::end (sustainedFlag), false);
            adsr.noteOff();
            break;
    }
}

// ---------------------------------------------------------------- hits

int Engine::currentSlice() const noexcept
{
    const int n = std::max (1, source.numSlices);
    int s = params.slice;
    if (params.midiMode == 1 && hadNote) s += curNote - params.sliceBase;
    return ((s % n) + n) % n;
}

void Engine::fireHit (int k, double subOffset, double periodSamples) noexcept
{
    if (! source.valid()) return;
    const int si = currentSlice();
    const double s0 = (double) source.bounds[si];
    const double s1 = (double) source.bounds[si + 1];
    const double sliceLen = std::max (1.0, s1 - s0);
    const double winLen = std::max (1.0, std::clamp (params.length, 0.01, 1.0) * sliceLen);
    const double start = s0 + std::clamp (params.scan, 0.0, 1.0) * (sliceLen - winLen);

    HitStart h;
    h.winStart = start;
    h.winEnd = start + winLen;
    h.reverse = params.reverse;
    h.subOffset = subOffset;
    h.speedRatio = source.sample.sampleRate / fsE;
    const double gate = std::clamp (params.gate, 0.01, 1.0);
    h.gateSamples = gate < 0.99999 ? gate * periodSamples : -1.0;
    h.fadeSamples = std::max (0.0, params.declickMs) * 0.001 * fsE;
    h.penvAmtSt = params.penvAmt;
    h.penvCoef = std::exp (-1.0 / (std::max (0.1, params.penvDecayMs) * 0.001 * fsE));
    h.gain = 1.0f;
    h.interp = (Interp) std::clamp (params.interp, 0, 2);
    h.mono = params.mono;

    const bool cut = params.trigMode == 0;
    // Overlap: one extra slot per lane holds a voice that is being stolen (0.5 ms fade).
    // Keep numLanes * (limit + 1) <= kMaxTotalVoices.
    int limit = 1;
    if (! cut)
    {
        limit = std::min (std::max (1, params.voices), kMaxTotalVoices / numLanes);
        if (numLanes * (limit + 1) > kMaxTotalVoices) limit = std::max (1, kMaxTotalVoices / numLanes - 1);
    }
    const double stealFade = cut ? h.fadeSamples : 0.0005 * fsE;
    lanes[k].trigger (h, limit, stealFade);
}

int Engine::activeVoiceCount() const noexcept
{
    int n = 0;
    for (int k = 0; k < kMaxLanes; ++k) n += lanes[k].activeCount();
    return n;
}

// ---------------------------------------------------------------- process

void Engine::process (float* outL, float* outR, int n, const MidiEvent* events, int numEvents) noexcept
{
    if (paramsDirty || params.rateSrc == 2) updateTargets();

    // host phase lock (on transport start / ppq jump)
    if (params.rateSrc == 2 && params.hostLock && host.valid && host.playing && running)
    {
        const bool jump = ! lastPlaying || (havePpq && std::abs (host.ppq - expectedPpq) > 1.0e-3);
        if (jump)
        {
            const double mult = std::exp2 ((double) std::clamp (params.hostMult, 0, 7));
            const double beatsPerCycle = lenQ (params.division, params.divMod) / mult;
            const double c = host.ppq / beatsPerCycle;
            for (int k = 0; k < numLanes; ++k) lanes[k].clock.setPhase (c - std::floor (c));
        }
    }

    const bool pitchMode = params.midiMode == 0;
    const double pbRange = params.pbRange;
    const double sPitchTarget = params.sPitch + params.sFine / 100.0;
    const double driveTarget = dbToGain (params.driveDb);
    const double outTarget = dbToGain (params.outGainDb);
    const int clipMode = params.clip;
    const bool snapAll = params.snapMode == 2;
    const Scale scale = (Scale) std::clamp (params.snapScale, 0, (int) Scale::Count - 1);

    int ei = 0;
    for (int i = 0; i < n; ++i)
    {
        while (ei < numEvents && events[ei].offset <= i)
            handleEvent (events[ei++]);

        // ---- rate (semitones relative to 1 Hz)
        double st = paramSm.process (paramStTarget) + snapOffset;
        if (gliding) glideNote += ((double) curNote - glideNote) * glideCoef;
        if (pitchMode && hadNote) st += glideNote - noteRef();
        st += bendSm.process (bendTarget) * pbRange;

        double hz = std::exp2 (st / 12.0);
        if (snapAll) hz = snapHz (hz, scale, params.snapRoot, params.a4);
        lastRateHz = clampRate (hz, fsE);

        const double pitchMul = std::exp2 (sPitchSm.process (sPitchTarget) / 12.0);

        float sl = 0.0f, sr = 0.0f;
        if (running)
        {
            for (int k = 0; k < numLanes; ++k)
            {
                Lane& lane = lanes[k];
                const double baseInc = clampRate (hz * lane.detuneRatio, fsE) / fsE;
                if (resetPending)
                {
                    const double ph = (params.uniPhase && numLanes > 1) ? rng.next01() : 0.0;
                    lane.clock.reset (ph);
                    fireHit (k, ph / baseInc, 1.0 / baseInc);
                }
                else
                {
                    double sub = 0.0;
                    if (lane.clock.tick (baseInc, sub))
                        fireHit (k, sub, lane.clock.getPeriodFactor() / baseInc);
                }
                lane.render (source.sample, pitchMul, sl, sr);
            }
            resetPending = false;
        }

        const double env = adsr.process();
        const double g = env * laneGain * velSm.process (velocity);
        const double drive = driveSm.process (driveTarget);
        const double outG = outSm.process (outTarget);

        double x[2] = { sl * g * drive, sr * g * drive };
        for (int c = 0; c < 2; ++c)
        {
            double v = x[c];
            if (clipMode == 1) v = std::tanh (v);
            else if (clipMode == 2) v = std::clamp (v, -1.0, 1.0);
            if (dcEnabled)
            {
                const double y = v - dcX1[c] + dcR * dcY1[c];
                dcX1[c] = v;
                dcY1[c] = y;
                v = y;
                // The HP can overshoot after a clipped step (up to 2x at sub-audio rates);
                // clip again with the same curve, still at the oversampled rate.
                if (clipMode == 1) v = std::tanh (v);
                else if (clipMode == 2) v = std::clamp (v, -1.0, 1.0);
            }
            x[c] = v * outG;
        }
        outL[i] = (float) x[0];
        outR[i] = (float) x[1];

        if (running && adsr.isIdle())
        {
            running = false;
            for (auto& l : lanes) l.killAll();
        }
    }
    while (ei < numEvents) handleEvent (events[ei++]); // events at/after n

    // host bookkeeping
    lastPlaying = host.valid && host.playing;
    if (host.valid)
    {
        expectedPpq = host.ppq + (double) n / fsE * host.bpm / 60.0;
        havePpq = true;
    }
    else havePpq = false;

    // telemetry
    telemetry.rateHz.store ((float) lastRateHz, std::memory_order_relaxed);
    telemetry.slice.store (currentSlice(), std::memory_order_relaxed);
    telemetry.numSlices.store (std::max (1, source.numSlices), std::memory_order_relaxed);
    telemetry.active.store (running, std::memory_order_relaxed);
    telemetry.voices.store (activeVoiceCount(), std::memory_order_relaxed);
    telemetry.envelopeLevel.store ((float) adsr.getLevel(), std::memory_order_relaxed);
    telemetry.envelopeStage.store (adsr.getStage(), std::memory_order_relaxed);
    const double pos = running ? lanes[0].newestPosition() : -1.0;
    telemetry.playPos.store (pos >= 0.0 && source.sample.frames > 0 ? (float) (pos / (double) source.sample.frames) : -1.0f,
                             std::memory_order_relaxed);
}
} // namespace xt
