#include "metronomeengine.h"

MetronomeEngine::MetronomeEngine(QObject *parent)
    : QObject(parent)
{
    connect(&timer, &QTimer::timeout, this, &MetronomeEngine::tick);
}

void MetronomeEngine::setTempo(int bpm) {
    tempoBpm = bpm;
    if (running) {
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
    if (running) {
        timer.start(static_cast<int>(pulseIntervalMs(patternStep)));
    }
}

void MetronomeEngine::start() {
    pulseIdx = 0;
    patternStep = 0;
    running = true;
    timer.start(static_cast<int>(pulseIntervalMs(patternStep)));
}

void MetronomeEngine::stop() {
    running = false;
    timer.stop();
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

void MetronomeEngine::tick() {
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

    if (playPulse) {
        emit pulse(pulseIdx, accent, isBeat);
    }

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