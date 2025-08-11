#include "metronomeengine.h"
#include <QDateTime>
#include <algorithm>
#include <cmath>
#include <QDebug>

// Local lcm helper
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

static bool isClose(double a, double b, double tol = 2.0) {
    return std::abs(a - b) < tol;
}

MetronomeEngine::MetronomeEngine(QObject *parent)
    : QObject(parent)
{
    connect(&timer, &QTimer::timeout, this, &MetronomeEngine::tick);
}

void MetronomeEngine::setTempo(int bpm) {
    tempoBpm = bpm;
    barMs = numerator * (60000.0 / tempoBpm); // Update for next bar!
    if (running && !polyrhythmEnabled) {
        timer.stop();
        timer.start(static_cast<int>(pulseIntervalMs(patternStep)));
    }
}

void MetronomeEngine::setTimeSignature(int num, int denom) {
    numerator = num;
    denominator = denom;
}

void MetronomeEngine::setAccentPattern(const std::vector<bool> &accents) {
    accentPattern = accents;
}

void MetronomeEngine::setSubdivision(NoteValue noteValue) {
    subdivision = noteValue;
    if (running && !polyrhythmEnabled) {
        timer.stop();
        timer.start(static_cast<int>(pulseIntervalMs(patternStep)));
    }
}

void MetronomeEngine::setPolyrhythmEnabled(bool enable) {
    polyrhythmEnabled = enable;
    if (running && polyrhythmEnabled) {
        timer.stop();
        startPolyrhythmBar(true);
    }
}

void MetronomeEngine::setPolyrhythm(int main, int poly) {
    polyrhythm.primaryBeats = main;
    polyrhythm.secondaryBeats = poly;
    if (running && polyrhythmEnabled) {
        timer.stop();
        startPolyrhythmBar(true);
    }
}

void MetronomeEngine::start() {
    timer.stop();
    running = true;
    pulseIdx = 0;
    patternStep = 0;
    schedIdx = 0;
    scheduledEvents.clear();
    lastBarStartMs = 0;
    if (polyrhythmEnabled) {
        startPolyrhythmBar(true);
    } else {
        timer.start(static_cast<int>(pulseIntervalMs(patternStep)));
    }
}

void MetronomeEngine::stop() {
    running = false;
    timer.stop();
    schedIdx = 0;
    scheduledEvents.clear();
    lastBarStartMs = 0; // Reset for next play
}

bool MetronomeEngine::isRunning() const {
    return running;
}

int MetronomeEngine::currentPulse() const {
    return pulseIdx;
}

bool MetronomeEngine::isAccent(int beatIdx) const {
    if (beatIdx >= 0 && beatIdx < static_cast<int>(accentPattern.size()))
        return accentPattern[beatIdx];
    return false;
}

// --- Polyrhythm event-driven scheduling ---

void MetronomeEngine::startPolyrhythmBar(bool newBar) {
    // Save the current bar duration BEFORE any tempo change
    double oldBarMs = barMs;

    // Apply armed tempo BEFORE scheduling events for this bar
    if (armedTempo > 0) {
        setTempo(armedTempo);
        armedTempo = -1;
    }

    if (!running) return;
    if (tempoBpm <= 0 || numerator <= 0 || polyrhythm.primaryBeats <= 0 || polyrhythm.secondaryBeats <= 0)
        return;

    schedIdx = 0;
    mainBeatInBar = 0;

    double quarterNoteMs = 60000.0 / tempoBpm;
    barMs = numerator * quarterNoteMs;

    // --- THE CORRECT RHYTHMIC SCHEDULING WITH GRID COLUMN ALIGNMENT ---
    int columns = lcm(polyrhythm.primaryBeats, polyrhythm.secondaryBeats);
    // Schedule only actual musical events, but record grid column for each
    std::vector<std::tuple<double, int, int>> rawEvents; // (time, type, gridCol)
    for (int i = 0; i < polyrhythm.primaryBeats; ++i) {
        double t = (i * barMs) / polyrhythm.primaryBeats;
        int col = (i * columns) / polyrhythm.primaryBeats;
        rawEvents.push_back({t, 0, col});
    }
    for (int i = 0; i < polyrhythm.secondaryBeats; ++i) {
        double t = (i * barMs) / polyrhythm.secondaryBeats;
        int col = (i * columns) / polyrhythm.secondaryBeats;
        rawEvents.push_back({t, 1, col});
    }
    std::sort(rawEvents.begin(), rawEvents.end(),
              [](const std::tuple<double, int, int>& a, const std::tuple<double, int, int>& b) { return std::get<0>(a) < std::get<0>(b); });

    // Merge coincident pulses (within mergeThreshold ms)
    constexpr double mergeThreshold = 2.0;
    scheduledEvents.clear();
    size_t idx = 0;
    while (idx < rawEvents.size()) {
        double t = std::get<0>(rawEvents[idx]);
        int col = std::get<2>(rawEvents[idx]);
        bool isMain = false, isPoly = false;
        size_t j = idx;
        while (j < rawEvents.size() && std::abs(std::get<0>(rawEvents[j]) - t) < mergeThreshold) {
            if (std::get<1>(rawEvents[j]) == 0) isMain = true;
            if (std::get<1>(rawEvents[j]) == 1) isPoly = true;
            ++j;
        }
        int evType = isMain ? (isPoly ? 2 : 0) : 1;
        scheduledEvents.push_back({std::round(t), evType, col});
        idx = j;
    }

    schedIdx = 0;

    // Prevent drift by aligning bar start to ideal time
    if (lastBarStartMs == 0) {
        lastBarStartMs = QDateTime::currentMSecsSinceEpoch();
    } else {
        // Advance by the duration of the previous bar (at the previous tempo)
        lastBarStartMs += oldBarMs;
    }

    scheduleNextPolyrhythmPulse();
}

void MetronomeEngine::scheduleNextPolyrhythmPulse() {
    if (!running) return;
    if (schedIdx >= static_cast<int>(scheduledEvents.size())) {
        timer.start(30); // Short pause between bars
        return;
    }
    double nowMs = QDateTime::currentMSecsSinceEpoch() - lastBarStartMs;
    double eventTime = scheduledEvents[schedIdx].timeMs;
    double delay = eventTime - nowMs;
    if (delay < 1) delay = 1;
    timer.start(static_cast<int>(delay));
}

void MetronomeEngine::tick() {
    if (!running) return;

    if (polyrhythmEnabled) {
        if (scheduledEvents.empty() || schedIdx >= static_cast<int>(scheduledEvents.size())) {
            startPolyrhythmBar(true);
            return;
        }

        double nowMs = QDateTime::currentMSecsSinceEpoch() - lastBarStartMs;

        while (schedIdx < static_cast<int>(scheduledEvents.size())
            && nowMs >= scheduledEvents[schedIdx].timeMs - 2.0) {
            int type = scheduledEvents[schedIdx].type;
            int gridColumn = scheduledEvents[schedIdx].gridCol;
            if (type == 2) {
                emit pulse(mainBeatInBar, true, true, true, true, gridColumn);
                mainBeatInBar++;
            } else if (type == 0) {
                emit pulse(mainBeatInBar, true, false, true, true, gridColumn);
                mainBeatInBar++;
            } else if (type == 1) {
                emit pulse(-1, false, true, false, true, gridColumn);
            }
            ++schedIdx;
        }
        scheduleNextPolyrhythmPulse();
        return;
    }

    // --- Regular metronome logic unchanged below ---
    int totalPulses = pulsesPerBar();
    bool isBeat = isMainBeat(pulseIdx);
    int beatNum = 0;
    int pulseInBeat = 0;

    switch (subdivision) {
        case NoteValue::Quarter:   beatNum = pulseIdx; pulseInBeat = 0; break;
        case NoteValue::Eighth:    beatNum = pulseIdx / 2; pulseInBeat = pulseIdx % 2; break;
        case NoteValue::Sixteenth: beatNum = pulseIdx / 4; pulseInBeat = pulseIdx % 4; break;
        case NoteValue::Triplet:   beatNum = pulseIdx / 3; pulseInBeat = pulseIdx % 3; break;
        case NoteValue::Eighth_Sixteenth_Sixteenth:
        case NoteValue::Sixteenth_Sixteenth_Eighth:
        case NoteValue::Sixteenth_Eighth_Sixteenth:
            beatNum = pulseIdx / 3; pulseInBeat = pulseIdx % 3; break;
        case NoteValue::DottedEighth_Sixteenth:
        case NoteValue::Sixteenth_DottedEighth:
            beatNum = pulseIdx / 2; pulseInBeat = pulseIdx % 2; break;
        case NoteValue::EighthRest_Eighth:
            beatNum = pulseIdx / 2; pulseInBeat = pulseIdx % 2; break;
        case NoteValue::SixteenthRest_Sixteenth_SixteenthRest_Sixteenth:
            beatNum = pulseIdx / 4; pulseInBeat = pulseIdx % 4; break;
        case NoteValue::EighthRest_Eighth_Eighth:
        case NoteValue::Eighth_EighthRest_Eighth:
        case NoteValue::Eighth_Eighth_EighthRest:
        case NoteValue::EighthRest_Eighth_EighthRest:
            beatNum = pulseIdx / 3; pulseInBeat = pulseIdx % 3; break;
        default:
            beatNum = pulseIdx; pulseInBeat = 0; break;
    }

    bool accent = (pulseInBeat == 0) && isAccent(beatNum);
    bool playPulse = true;

    if (subdivision == NoteValue::EighthRest_Eighth_Eighth) {
        playPulse = (pulseInBeat == 0) ? accent : true;
    }
    else if (subdivision == NoteValue::Eighth_EighthRest_Eighth) {
        playPulse = (pulseInBeat != 1);
    }
    else if (subdivision == NoteValue::Eighth_Eighth_EighthRest) {
        playPulse = (pulseInBeat != 2);
    }
    else if (subdivision == NoteValue::EighthRest_Eighth_EighthRest) {
        playPulse = (pulseInBeat == 0) ? accent : (pulseInBeat != 2);
    }
    else if (subdivision == NoteValue::EighthRest_Eighth) {
        playPulse = (pulseInBeat == 0) ? accent : true;
    }
    else if (subdivision == NoteValue::SixteenthRest_Sixteenth_SixteenthRest_Sixteenth) {
        playPulse = (pulseInBeat == 0) ? accent : (pulseInBeat != 2);
    }

    emit pulse(pulseIdx, accent, false, isBeat, playPulse, -1);

    pulseIdx++;
    if (subdivision == NoteValue::Eighth_Sixteenth_Sixteenth ||
        subdivision == NoteValue::Sixteenth_Sixteenth_Eighth ||
        subdivision == NoteValue::Sixteenth_Eighth_Sixteenth)
    {
        patternStep = (patternStep + 1) % 3;
    }
    else if (subdivision == NoteValue::DottedEighth_Sixteenth ||
             subdivision == NoteValue::Sixteenth_DottedEighth ||
             subdivision == NoteValue::EighthRest_Eighth ||
             subdivision == NoteValue::SixteenthRest_Sixteenth_SixteenthRest_Sixteenth)
    {
        patternStep = (patternStep + 1) % 2;
    }
    else if (subdivision == NoteValue::EighthRest_Eighth_Eighth ||
             subdivision == NoteValue::Eighth_EighthRest_Eighth ||
             subdivision == NoteValue::Eighth_Eighth_EighthRest ||
             subdivision == NoteValue::EighthRest_Eighth_EighthRest)
    {
        patternStep = (patternStep + 1) % 3;
    }
    if (pulseIdx >= totalPulses) {
        pulseIdx = 0;
        patternStep = 0;
    }

    if (running) {
        timer.start(static_cast<int>(pulseIntervalMs(patternStep)));
    }
}

int MetronomeEngine::pulsesPerBar() const {
    switch (subdivision) {
        case NoteValue::Quarter:   return numerator;
        case NoteValue::Eighth:    return numerator * 2;
        case NoteValue::Sixteenth: return numerator * 4;
        case NoteValue::Triplet:   return numerator * 3;
        case NoteValue::Eighth_Sixteenth_Sixteenth: return numerator * 3;
        case NoteValue::Sixteenth_Sixteenth_Eighth: return numerator * 3;
        case NoteValue::Sixteenth_Eighth_Sixteenth: return numerator * 3;
        case NoteValue::DottedEighth_Sixteenth: return numerator * 2;
        case NoteValue::Sixteenth_DottedEighth: return numerator * 2;
        case NoteValue::EighthRest_Eighth: return numerator * 2;
        case NoteValue::SixteenthRest_Sixteenth_SixteenthRest_Sixteenth: return numerator * 4;
        case NoteValue::EighthRest_Eighth_Eighth: return numerator * 3;
        case NoteValue::Eighth_EighthRest_Eighth: return numerator * 3;
        case NoteValue::Eighth_Eighth_EighthRest: return numerator * 3;
        case NoteValue::EighthRest_Eighth_EighthRest: return numerator * 3;
        default: return numerator;
    }
}

double MetronomeEngine::pulseIntervalMs(int pulseInPattern) const {
    double beatsPerMinute = static_cast<double>(tempoBpm);
    double quarterNoteMs = 60000.0 / beatsPerMinute;
    if (polyrhythmEnabled) {
        // Not used in event-driven polyrhythm, but kept for compatibility
        return quarterNoteMs * numerator / (polyrhythm.primaryBeats * polyrhythm.secondaryBeats);
    }
    switch (subdivision) {
        case NoteValue::Quarter: return quarterNoteMs;
        case NoteValue::Eighth: return quarterNoteMs / 2.0;
        case NoteValue::Sixteenth: return quarterNoteMs / 4.0;
        case NoteValue::Triplet: return quarterNoteMs / 3.0;
        case NoteValue::Eighth_Sixteenth_Sixteenth:
            return (pulseInPattern == 0) ? quarterNoteMs / 2.0 : quarterNoteMs / 4.0;
        case NoteValue::Sixteenth_Sixteenth_Eighth:
            return (pulseInPattern == 2) ? quarterNoteMs / 2.0 : quarterNoteMs / 4.0;
        case NoteValue::Sixteenth_Eighth_Sixteenth:
            return (pulseInPattern == 1) ? quarterNoteMs / 2.0 : quarterNoteMs / 4.0;
        case NoteValue::DottedEighth_Sixteenth:
            return (pulseInPattern == 0) ? quarterNoteMs * 0.75 : quarterNoteMs * 0.25;
        case NoteValue::Sixteenth_DottedEighth:
            return (pulseInPattern == 0) ? quarterNoteMs * 0.25 : quarterNoteMs * 0.75;
        case NoteValue::EighthRest_Eighth: return quarterNoteMs / 2.0;
        case NoteValue::SixteenthRest_Sixteenth_SixteenthRest_Sixteenth: return quarterNoteMs / 4.0;
        case NoteValue::EighthRest_Eighth_Eighth:
        case NoteValue::Eighth_EighthRest_Eighth:
        case NoteValue::Eighth_Eighth_EighthRest:
        case NoteValue::EighthRest_Eighth_EighthRest:
            return quarterNoteMs / 3.0;
        default: return quarterNoteMs;
    }
}

bool MetronomeEngine::isMainBeat(int pulseIdx) const {
    switch (subdivision) {
        case NoteValue::Quarter:   return true;
        case NoteValue::Eighth:    return (pulseIdx % 2 == 0);
        case NoteValue::Sixteenth: return (pulseIdx % 4 == 0);
        case NoteValue::Triplet:   return (pulseIdx % 3 == 0);
        case NoteValue::Eighth_Sixteenth_Sixteenth:
        case NoteValue::Sixteenth_Sixteenth_Eighth:
        case NoteValue::Sixteenth_Eighth_Sixteenth:
            return (pulseIdx % 3 == 0);
        case NoteValue::DottedEighth_Sixteenth:
        case NoteValue::Sixteenth_DottedEighth:
            return (pulseIdx % 2 == 0);
        case NoteValue::EighthRest_Eighth:
            return (pulseIdx % 2 == 1);
        case NoteValue::SixteenthRest_Sixteenth_SixteenthRest_Sixteenth:
            return (pulseIdx % 4 == 1 || pulseIdx % 4 == 3);
        case NoteValue::EighthRest_Eighth_Eighth:
            return (pulseIdx % 3 == 1);
        case NoteValue::Eighth_EighthRest_Eighth:
        case NoteValue::Eighth_Eighth_EighthRest:
            return (pulseIdx % 3 == 0);
        case NoteValue::EighthRest_Eighth_EighthRest:
            return (pulseIdx % 3 == 1);
        default: return true;
    }
}

void MetronomeEngine::timerEvent(QTimerEvent *event)
{
    Q_UNUSED(event);
}