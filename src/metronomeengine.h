#pragma once

#include <QObject>
#include <QTimer>
#include <QDateTime>
#include <vector>

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
    EighthRest_Eighth_EighthRest
};

struct Polyrhythm {
    int primaryBeats = 3;
    int secondaryBeats = 2;
};

// Updated PolyEvent struct to include the grid column
struct PolyEvent {
    double timeMs;
    int type; // 0 = main, 1 = poly, 2 = both
    int gridCol; // grid column index for visual highlight
};

class MetronomeEngine : public QObject
{
    Q_OBJECT
public:
    explicit MetronomeEngine(QObject *parent = nullptr);

    void setPulseIdx(int idx) { pulseIdx = idx; }

    int currentTempo() const { return tempoBpm; }

    void setTempo(int bpm);
    void setTimeSignature(int num, int denom);
    void setAccentPattern(const std::vector<bool> &accents);
    void setSubdivision(NoteValue noteValue);
    void setPolyrhythmEnabled(bool enable);
    void setPolyrhythm(int main, int poly);

    void start();
    void stop();
    bool isRunning() const;
    int currentPulse() const;
    bool isAccent(int beatIdx) const;
    bool isPolyrhythmEnabled() const { return polyrhythmEnabled; }
    Polyrhythm getPolyrhythm() const { return polyrhythm; }

    void startPolyrhythmBar(bool newBar);
    double pulseIntervalMs(int pulseInPattern) const;

    int pulsesPerBar() const;

    // New: Arm a tempo to be applied at the next bar
    void armTempo(int t) { armedTempo = t; }

signals:
    // Now passes gridColumn for visual highlight
    void pulse(int eventIdx, bool accent, bool polyAccent, bool isMainBeat, bool playPulse, int gridColumn);

protected:
    void timerEvent(QTimerEvent *event) override;

private slots:
    void tick();

private:
    // General metronome state
    QTimer timer;
    int tempoBpm = 120;
    int numerator = 4;
    int denominator = 4;
    int pulseIdx = 0;
    int patternStep = 0;
    bool running = false;
    int mainBeatInBar = 0;

    int armedTempo = -1;

    NoteValue subdivision = NoteValue::Quarter;
    std::vector<bool> accentPattern;

    // Polyrhythm state
    bool polyrhythmEnabled = false;
    Polyrhythm polyrhythm;
    double barMs = 0.0;
    qint64 lastBarStartMs = 0;

    // --- Polyrhythm event scheduling ---
    std::vector<PolyEvent> scheduledEvents;
    int schedIdx = 0;
    
    void scheduleNextPolyrhythmPulse();
    void handlePolyrhythmPulse();

    // --- Standard metronome logic ---
    bool isMainBeat(int pulseIdx) const;
};