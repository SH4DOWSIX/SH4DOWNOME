#pragma once

#include <QObject>
#include <QTimer>
#include <vector>

enum class NoteValue {
    Quarter = 0,
    Eighth,
    Sixteenth,
    Triplet,
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

class MetronomeEngine : public QObject {
    Q_OBJECT
public:
    explicit MetronomeEngine(QObject *parent = nullptr);

    void setTempo(int bpm);
    void setTimeSignature(int numerator, int denominator);
    void setAccentPattern(const std::vector<bool> &accents);
    void setSubdivision(NoteValue noteValue);
    void start();
    void stop();
    bool isRunning() const;
    int currentPulse() const;
    bool isAccent(int beatIdx) const;

signals:
    void pulse(int pulseIdx, bool accent, bool isBeat);

private slots:
    void tick();

private:
    QTimer timer;
    int tempoBpm = 120;
    int numerator = 4;
    int denominator = 4;
    NoteValue subdivision = NoteValue::Quarter;
    int pulseIdx = 0;
    std::vector<bool> accentPattern;
    bool running = false;
    int patternStep = 0;

    int pulsesPerBar() const;
    double pulseIntervalMs(int pulseInPattern = 0) const;
    bool isMainBeat(int pulseIdx) const;
};