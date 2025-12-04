#pragma once

#include <QObject>
#include <vector>
#include "subdivisionpattern.h"
#include "audioengine.h"

enum class NoteValue {
    Quarter, Eighth, Sixteenth, Triplet,
    Eighth_Sixteenth_Sixteenth,
    Sixteenth_Sixteenth_Eighth,
    Sixteenth_Eighth_Sixteenth,
    DottedEighth_Sixteenth,
    Sixteenth_DottedEighth,
    EighthRest_Eighth,
    SixteenthRest_Sixteenth_SixteenthRest_Sixteenth,
    EighthRest_Eighth_Eighth,
    Eighth_EighthRest_Eighth,
    Eighth_Eighth_EighthRest,
    EighthRest_Eighth_EighthRest,
    DottedQuarter
};

struct Polyrhythm {
    int primaryBeats = 3;
    int secondaryBeats = 2;
};

class MetronomeEngine : public QObject
{
    Q_OBJECT
public:
    explicit MetronomeEngine(QObject *parent = nullptr);

    // --- PATCH: Add public getter and setter for count-in enabled ---
    bool countInEnabled() const { return m_countInEnabled; }
    void setCountInEnabled(bool enabled);
    // ---------------------------------------------------------------

    void setPulseIdx(int idx) { m_pulseIdx = idx; }

    int currentTempo() const { return m_tempoBpm; }
    void setTempo(int bpm);
    void setTimeSignature(int num, int denom);
    void setAccentPattern(const std::vector<bool> &accents);
    void setSubdivisionPattern(const SubdivisionPattern& pattern);
    SubdivisionPattern subdivisionPattern() const { return m_subdivisionPattern; }
    void setPolyrhythmEnabled(bool enable);
    void setPolyrhythm(int main, int poly);
    void playCountInClick(bool accent = false);

    void start();
    void stop();
    bool isRunning() const;
    int currentPulse() const;
    bool isAccent(int beatIdx) const;
    bool isPolyrhythmEnabled() const { return m_polyrhythmEnabled; }
    Polyrhythm getPolyrhythm() const { return m_polyrhythm; }

    void armTempo(int t) { 
    m_armedTempo = t; 
    setTempo(t); // Actually apply the tempo
}

    void resetDeduplication();

    bool loadSample(const QString& name, const QString& resourcePath);
    void setAccentSound(const QString& name);
    void setClickSound(const QString& name);
    void setVolume(float vol);
    void playAccent();
    void playClick();

    int globalPulseCount() const { return m_globalPulseCount; }
    int globalBarCount() const { return m_globalBarCount; }

    void startWithCountIn(int countInBeats);

    AudioEngine* audioEngine();

signals:
    void pulse(AudioPulseEvent ev);

private slots:
    void onAudioPulse(AudioPulseEvent ev);

private:
    // General metronome state
    int m_tempoBpm = 120;
    int m_numerator = 4;
    int m_denominator = 4;
    int m_pulseIdx = 0;
    int m_patternPulseIdx = 0;
    bool m_running = false;
    int m_mainBeatInBar = 0;
    int beatsPerBar() const;
    bool isCompoundTime() const;
    int m_globalPulseCount = 0;
    int m_globalBarCount = 0;
    int m_armedTempo = -1;
    bool m_countInEnabled = false; // count-in state
    void scheduleCustomSubdivisionLoop();

    SubdivisionPattern m_subdivisionPattern;
    std::vector<bool> m_accentPattern;

    // Polyrhythm state
    bool m_polyrhythmEnabled = false;
    Polyrhythm m_polyrhythm;
    double m_barLengthSeconds = 0.0;

    AudioEngine* m_audioEngine = nullptr;

    // Pulse schedule for current bar/cycle
    std::vector<AudioPulseEvent> m_pulseSchedule;
    void updatePulseSchedule(int countInBeats = 0);

    // For deduplication (UI update logic)
    int m_lastPulseEmittedIdx = -1;
    int m_lastPulseEmittedPatternIdx = -1;
};