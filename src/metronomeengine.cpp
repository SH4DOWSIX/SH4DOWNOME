#include "metronomeengine.h"
#include <QDebug>
#include <QThread> 
#include <cmath>
#include <algorithm>

// --- Helper ---
static int lcm(int a, int b) {
    if (a == 0 || b == 0) return 0;
    int x = a, y = b;
    while (y != 0) {
        int t = y;
        y = x % y;
        x = t;
    }
    return (a / x) * b;
}

MetronomeEngine::MetronomeEngine(QObject *parent)
    : QObject(parent)
{
    m_audioEngine = new AudioEngine(this);
    connect(m_audioEngine, &AudioEngine::pulseUiEvent,
            this, &MetronomeEngine::onAudioPulse);
    connect(m_audioEngine, &AudioEngine::tempoSteppedUp,
            this, &MetronomeEngine::onAudioTempoSteppedUp);

    // Default subdivision
    m_subdivisionPattern = SubdivisionPattern{
        SubdivisionCategory::Standard,
        "Quarter Note",
        QVector<SubdivisionPulse>{ {NoteValue::Quarter, false, false} }
    };
}

void MetronomeEngine::setTempo(int bpm) {
    m_tempoBpm = bpm;
    if (m_running) m_audioEngine->setEngineParams(buildEngineParams());
}

// Kept for backward compat — now just an alias for setTempo when running.
void MetronomeEngine::setTempoNow(int bpm) {
    m_tempoBpm = bpm;
    if (m_running) m_audioEngine->setEngineParams(buildEngineParams());
}

void MetronomeEngine::setTimeSignature(int num, int denom) {
    m_numerator = num;
    m_denominator = denom;
    if (m_running) m_audioEngine->setEngineParams(buildEngineParams());
}

void MetronomeEngine::setAccentPattern(const std::vector<bool> &accents) {
    m_accentPattern = accents;
    if (m_running) m_audioEngine->setEngineParams(buildEngineParams());
}

void MetronomeEngine::setSubdivisionPattern(const SubdivisionPattern& pattern) {
    m_subdivisionPattern = pattern;
    if (m_running) m_audioEngine->setEngineParams(buildEngineParams());
}

void MetronomeEngine::setPolyrhythmEnabled(bool enable) {
    m_polyrhythmEnabled = enable;
    if (m_running) m_audioEngine->setEngineParams(buildEngineParams());
}

void MetronomeEngine::setPolyrhythm(int main, int poly) {
    m_polyrhythm.primaryBeats = main;
    m_polyrhythm.secondaryBeats = poly;
    if (m_running) m_audioEngine->setEngineParams(buildEngineParams());
}

int MetronomeEngine::beatsPerBar() const {
    if (isCompoundTime())
        return m_numerator / 3;
    return m_numerator;
}

bool MetronomeEngine::isCompoundTime() const {
    return (m_denominator == 8) && (m_numerator % 3 == 0) && (m_numerator > 3);
}

void MetronomeEngine::start() {
    if (m_running) return;

    // 1. Initialize device (open, get correct sample rate, but do NOT start playback)
    bool deviceReady = m_audioEngine->initializeDevice(m_tempoBpm);
    if (!deviceReady) return;

    // 2. Reset counters/state
    m_pulseIdx = 0;
    m_patternPulseIdx = 0;
    m_lastPulseEmittedIdx = -1;
    m_lastPulseEmittedPatternIdx = -1;
    m_globalPulseCount = 0;
    m_globalBarCount = 0;
    m_justExitedCountIn = false;

    // 3. Start via state machine (count-in handled automatically if m_countInEnabled)
    m_audioEngine->startWithParams(buildEngineParams(), m_countInEnabled);

    // Snapshot the run ID that startWithParams just assigned. Any pulseUiEvent
    // signal carrying a different runId belongs to an old session and must be dropped.
    m_expectedRunId = m_audioEngine->currentRunId();

    m_running = true;
}

void MetronomeEngine::stop() {
    if (!m_running) {
        return;
    }
    m_running = false;
    m_audioEngine->stop();
    m_pulseSchedule.clear();
    m_globalPulseCount = 0;
    m_globalBarCount = 0;
}

bool MetronomeEngine::isRunning() const {
    return m_running;
}

int MetronomeEngine::currentPulse() const {
    return m_pulseIdx;
}

bool MetronomeEngine::isAccent(int beatIdx) const {
    if (beatIdx >= 0 && beatIdx < static_cast<int>(m_accentPattern.size()))
        return m_accentPattern[beatIdx];
    return false;
}

void MetronomeEngine::resetDeduplication() {
    m_lastPulseEmittedIdx = -1;
    m_lastPulseEmittedPatternIdx = -1;
}

// --- Sample-accurate pulse schedule generation ---
void MetronomeEngine::updatePulseSchedule(int /*countInBeats*/, bool skipAudioScheduling) {
    m_pulseSchedule.clear();
    int sampleRate = m_audioEngine->getSampleRate();

    // --- Calculate beat and bar length for simple and compound time ---
    bool compound = isCompoundTime();
    int beatsPerBar = compound ? (m_numerator / 3) : m_numerator;
    double secondsPerBeat;
    int samplesPerBeat;
    double barLenSec;
    int barLenSamples;

    // Interpret tempo as BPM for the metronome beat. For compound meters the beat is a dotted-quarter;
    // we keep secondsPerBeat = 60 / BPM so a dotted-quarter at the same BPM occupies the same seconds as a quarter in simple time.
    secondsPerBeat = 60.0 / m_tempoBpm;
    samplesPerBeat = int(sampleRate * secondsPerBeat);
    barLenSec = beatsPerBar * secondsPerBeat;
    barLenSamples = beatsPerBar * samplesPerBeat;

    // Store the time signature bar length
    double timeSignatureBarLength = barLenSec;

    // --- SCHEDULE COUNT-IN ONLY IF ENABLED ---
    int countInEndSample = 0;
    if (m_countInEnabled) {
        // In compound time, count-in should use compound beats (dotted quarters)
        int countInPulses = compound ? beatsPerBar : m_numerator;
        int samplesPerCountInPulse = samplesPerBeat;

        for (int i = 0; i < countInPulses; ++i) {
            AudioPulseEvent ev;
            ev.idx = -1000 + i;
            ev.accent = (i == 0);
            ev.polyAccent = false;
            ev.isBeat = true;
            ev.playPulse = true;
            ev.gridColumn = -1;
            ev.samplePosInBar = i * samplesPerCountInPulse;
            m_pulseSchedule.push_back(ev);
        }
        countInEndSample = countInPulses * samplesPerCountInPulse;

        AudioPulseEvent gapEv;
        gapEv.idx = -9999;
        gapEv.accent = false;
        gapEv.polyAccent = false;
        gapEv.isBeat = false;
        gapEv.playPulse = false;
        gapEv.gridColumn = -1;
        gapEv.samplePosInBar = countInEndSample;
        m_pulseSchedule.push_back(gapEv);
    }

    // --- SCHEDULE MAIN PULSES ---
    if (m_polyrhythmEnabled && m_polyrhythm.primaryBeats > 0 && m_polyrhythm.secondaryBeats > 0) {
        // Polyrhythm scheduling (unchanged)
        int main = m_polyrhythm.primaryBeats;
        int poly = m_polyrhythm.secondaryBeats;
        int columns = lcm(main, poly);

        struct PulseInfo {
            double time;
            int gridColumn;
            bool isMain;
            bool isPoly;
        };

        std::vector<PulseInfo> pulses;
        for (int i = 0; i < main; ++i) {
            double t = i * barLenSec / main;
            int col = int(std::round(t / barLenSec * columns));
            pulses.push_back({t, col, true, false});
        }
        for (int i = 0; i < poly; ++i) {
            double t = i * barLenSec / poly;
            int col = int(std::round(t / barLenSec * columns));
            pulses.push_back({t, col, false, true});
        }
        std::sort(pulses.begin(), pulses.end(), [](const PulseInfo& a, const PulseInfo& b) {
            return a.time < b.time;
        });

        std::vector<PulseInfo> merged;
        const double eps = 1e-6;
        for (const auto& pi : pulses) {
            if (!merged.empty() && std::abs(pi.time - merged.back().time) < eps) {
                merged.back().isMain |= pi.isMain;
                merged.back().isPoly |= pi.isPoly;
            } else {
                merged.push_back(pi);
            }
        }

        int barsNeeded = 1;
        for (int barNum = 0; barNum < barsNeeded; ++barNum) {
            for (size_t idx = 0; idx < merged.size(); ++idx) {
                const auto& pi = merged[idx];
                AudioPulseEvent ev;
                ev.idx = int(idx);
                ev.gridColumn = pi.gridColumn;
                ev.isBeat = (pi.isMain && idx == 0);
                ev.accent = pi.isMain;
                ev.polyAccent = pi.isPoly;
                ev.playPulse = true;
                ev.samplePosInBar = int(std::round(pi.time * sampleRate)) + barNum * barLenSamples + countInEndSample;
                ev.startOfCycle = (idx == 0);
                m_pulseSchedule.push_back(ev);
            }
        }
        m_barLengthSeconds = timeSignatureBarLength;
    } else {
        // Subdivision / classic / custom scheduling
        int beats = beatsPerBar;
        int subdivs = m_subdivisionPattern.pulses.size();
        if (subdivs == 0) subdivs = 1;

        // Compute total pattern duration in beat fractions using NoteValue
        double totalPatternDuration = 0.0;
        for (const auto& pulse : m_subdivisionPattern.pulses) {
            totalPatternDuration += noteValueBeatFraction(pulse.noteValue, compound);
        }

        // Determine whether this pattern is a "standard" pattern (fits exactly in one beat)
        const double TOLERANCE = 1e-3;
        bool isStandardPattern = false;
        if (m_subdivisionPattern.category == SubdivisionCategory::Custom) {
            isStandardPattern = false;
        } else if (compound && m_subdivisionPattern.pulses.size() == 1 &&
                   std::abs(noteValueBeatFraction(m_subdivisionPattern.pulses[0].noteValue, compound) - 1.0) < TOLERANCE) {
            isStandardPattern = true;
        } else {
            isStandardPattern = (std::abs(totalPatternDuration - 1.0) < TOLERANCE);
        }

        if (isStandardPattern) {
            // Classic metronome: repeat the pattern for each beat
            for (int b = 0; b < beats; ++b) {
                double beatStartSec = b * secondsPerBeat;
                double pulseOffsetSec = 0.0;
                for (int s = 0; s < subdivs; ++s) {
                    AudioPulseEvent ev;
                    ev.idx = b * subdivs + s;
                    ev.gridColumn = -1;
                    ev.isBeat = (s == 0);

                    if (m_subdivisionPattern.category == SubdivisionCategory::Custom) {
                        ev.accent = m_subdivisionPattern.pulses[s].accent;
                    } else {
                        ev.accent = (s == 0 && m_accentPattern.size() > b && m_accentPattern[b]);
                    }

                    ev.polyAccent = false;
                    ev.isRest = m_subdivisionPattern.pulses[s].isRest;
                    ev.playPulse = !ev.isRest;

                    double pulseDurSec = secondsPerBeat * noteValueBeatFraction(m_subdivisionPattern.pulses[s].noteValue, compound);

                    ev.samplePosInBar = int(std::round((beatStartSec + pulseOffsetSec) * sampleRate)) + countInEndSample;
                    ev.startOfCycle = false;

                    pulseOffsetSec += pulseDurSec;
                    m_pulseSchedule.push_back(ev);
                }
            }
            m_barLengthSeconds = timeSignatureBarLength;
        } else {
            // Custom subdivision: the pattern defines its own bar length
            double actualPatternDurationSeconds = 0.0;
            for (const auto& p : m_subdivisionPattern.pulses) {
                actualPatternDurationSeconds += noteValueBeatFraction(p.noteValue, compound) * secondsPerBeat;
            }

            std::vector<double> beatStartTimes;
            for (int b = 0; b < beats; ++b)
                beatStartTimes.push_back(b * secondsPerBeat);

            double pulseOffsetSec = 0.0;
            for (int s = 0; s < m_subdivisionPattern.pulses.size(); ++s) {
                const auto& pulse = m_subdivisionPattern.pulses[s];
                AudioPulseEvent ev;
                ev.idx = s;
                ev.gridColumn = -1;
                ev.isBeat = false;

                double pulseStartSec = pulseOffsetSec;

                bool accent = false;
                for (int b = 0; b < beats; ++b) {
                    if (std::abs(pulseStartSec - beatStartTimes[b]) < 1e-6) {
                        accent = (m_accentPattern.size() > b && m_accentPattern[b]);
                        ev.isBeat = true;
                        break;
                    }
                }

                ev.accent = pulse.accent;
                ev.isBeat = (s == 0) ? true : ev.isBeat;
                ev.polyAccent = false;
                ev.isRest = pulse.isRest;
                ev.playPulse = !ev.isRest;

                double pulseDurSec = secondsPerBeat * noteValueBeatFraction(pulse.noteValue, compound);

                ev.samplePosInBar = int(std::round(pulseOffsetSec * sampleRate)) + countInEndSample;
                ev.startOfCycle = (s == 0);

                pulseOffsetSec += pulseDurSec;
                m_pulseSchedule.push_back(ev);
            }

            m_barLengthSeconds = actualPatternDurationSeconds;
        }
    }

    // Audio engine scheduling
    if (skipAudioScheduling) return; // caller (e.g. setTempoNow) handles dispatch itself
    if (m_audioEngine->isRunning()) {
        bool isCountIn = !m_pulseSchedule.empty() && m_pulseSchedule[0].idx < 0;
        if (isCountIn) {
            m_audioEngine->schedulePulses(m_pulseSchedule, m_barLengthSeconds, sampleRate);
        } else {
            m_audioEngine->requestScheduleChange(m_pulseSchedule, m_barLengthSeconds, sampleRate);
            m_audioEngine->flushAtNextBarBoundary();
        }
    } else {
        m_audioEngine->schedulePulses(m_pulseSchedule, m_barLengthSeconds, sampleRate);
    }
}

// --- Audio thread callback for UI pulse ---
void MetronomeEngine::onAudioPulse(AudioPulseEvent ev) {
    // Discard any queued signals from a previous run. This handles the race where
    // old pulseUiEvent signals (emitted before stop) arrive on the main thread
    // after a new session has already started.
    if (ev.runId != m_expectedRunId) return;

    if (ev.newTempo > 0) {
        // A speed-trainer step-up just came into effect at this exact pulse.
        // Update the stored tempo so callers of currentTempo() see the new value.
        m_tempoBpm = ev.newTempo;
        emit tempoSteppedUp(ev.newTempo);
    }

    if (ev.idx < 0) {
        // Count-in pulse: mark so we reset counter on first main pulse
        m_justExitedCountIn = true;
    } else {
        if (m_justExitedCountIn) {
            m_globalPulseCount = 1;
            m_globalBarCount   = 1;
            m_justExitedCountIn = false;
        } else {
            m_globalPulseCount++;
        }
    }
    m_lastPulseEmittedIdx = ev.idx;
    emit pulse(ev);
}

// Called (via queued connection) when audio thread steps up the tempo.
void MetronomeEngine::onAudioTempoSteppedUp(int newTempo) {
    m_tempoBpm         = newTempo;
    m_globalPulseCount = 0;  // will be 1 on first pulse of new tempo (or after count-in)
    m_justExitedCountIn = false;
    emit tempoSteppedUp(newTempo);
}

// Build an EngineParams snapshot from current MetronomeEngine state.
EngineParams MetronomeEngine::buildEngineParams() const {
    EngineParams p;
    p.bpm              = m_tempoBpm;
    p.numerator        = m_numerator;
    p.denominator      = m_denominator;
    p.polyrhythmEnabled = m_polyrhythmEnabled;
    p.polyMain         = m_polyrhythm.primaryBeats;
    p.polySecondary    = m_polyrhythm.secondaryBeats;
    p.subdivision      = m_subdivisionPattern;
    p.accents          = m_accentPattern;
    p.countInEnabled   = m_countInEnabled;
    p.speedEnabled     = m_speedEnabled;
    p.barsPerStep      = m_speedBarsPerStep;
    p.tempoStep        = m_speedTempoStep;
    p.maxTempo         = m_speedMaxTempo;
    p.startTempo       = m_tempoBpm;
    return p;
}

void MetronomeEngine::setSpeedTrainer(bool enabled, int barsPerStep, int tempoStep, int maxTempo) {
    m_speedEnabled     = enabled;
    m_speedBarsPerStep = barsPerStep;
    m_speedTempoStep   = tempoStep;
    m_speedMaxTempo    = maxTempo;
}

bool MetronomeEngine::loadSample(const QString& name, const QString& resourcePath) {
    return m_audioEngine->loadSample(name, resourcePath);
}
void MetronomeEngine::setAccentSound(const QString& name) {
    m_audioEngine->setAccentSound(name);
}
void MetronomeEngine::setClickSound(const QString& name) {
    m_audioEngine->setClickSound(name);
}
void MetronomeEngine::setVolume(float vol) {
    m_audioEngine->setVolume(vol);
}
void MetronomeEngine::playAccent() {}
void MetronomeEngine::playClick() {}

void MetronomeEngine::startWithCountIn(int /*countInBeats*/) {
    if (m_running) return;

    bool deviceReady = m_audioEngine->initializeDevice(m_tempoBpm);
    if (!deviceReady) return;

    m_pulseIdx = 0;
    m_patternPulseIdx = 0;
    m_lastPulseEmittedIdx = -1;
    m_lastPulseEmittedPatternIdx = -1;
    m_globalPulseCount = 0;
    m_globalBarCount = 0;
    m_justExitedCountIn = false;

    // Always start with count-in regardless of m_countInEnabled flag
    m_audioEngine->startWithParams(buildEngineParams(), true);

    m_running = true;
}

void MetronomeEngine::setCountInEnabled(bool enabled) {
    m_countInEnabled = enabled;
}

AudioEngine* MetronomeEngine::audioEngine() { 
    return m_audioEngine; 
}


void MetronomeEngine::scheduleCustomSubdivisionLoop() {
    // Create a schedule with just the custom subdivision pattern (no count-in, no gap)
    std::vector<AudioPulseEvent> loopSchedule;
    int sampleRate = m_audioEngine->getSampleRate();

    bool compound = isCompoundTime();
    int beatsPerBar = compound ? (m_numerator / 3) : m_numerator;

    // Use tempo BPM directly so dotted-quarter in compound time matches quarter-note duration for same BPM
    double secondsPerBeat = 60.0 / m_tempoBpm;

    std::vector<double> beatStartTimes;
    for (int b = 0; b < beatsPerBar; ++b)
        beatStartTimes.push_back(b * secondsPerBeat);

    double pulseOffsetSec = 0.0;
    for (int s = 0; s < m_subdivisionPattern.pulses.size(); ++s) {
        const auto& pulse = m_subdivisionPattern.pulses[s];
        AudioPulseEvent ev;
        ev.idx = s;
        ev.gridColumn = -1;
        ev.isBeat = false;

        double pulseStartSec = pulseOffsetSec;
        bool accent = false;
        for (int b = 0; b < beatsPerBar; ++b) {
            if (std::abs(pulseStartSec - beatStartTimes[b]) < 1e-6) {
                accent = (m_accentPattern.size() > b && m_accentPattern[b]);
                ev.isBeat = true;
                break;
            }
        }
        ev.accent = accent;
        ev.polyAccent = false;
        ev.playPulse = true;
        ev.isRest = pulse.isRest;

        // Use duration directly (do not multiply by dot)
        double pulseDurSec = secondsPerBeat * noteValueBeatFraction(pulse.noteValue, compound);

        // NO count-in offset - this is just the pattern
        ev.samplePosInBar = int(pulseOffsetSec * sampleRate);
        ev.startOfCycle = (s == 0);

        pulseOffsetSec += pulseDurSec;
        loopSchedule.push_back(ev);
    }

    // Schedule the loop pattern
    m_audioEngine->requestScheduleChange(loopSchedule, pulseOffsetSec, sampleRate);
    m_audioEngine->flushAtNextBarBoundary();
}