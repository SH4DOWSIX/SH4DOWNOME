#pragma once

#include <QObject>
#include <QString>
#include <QMap>
#include <vector>
#include "metronomeengine.h"
#include "subdivisionpattern.h"

// --- PATCH: Use only SubdivisionPattern for per-section custom playback ---
struct MetronomeSection {
    int tempo;
    int numerator;
    int denominator;
    SubdivisionPattern subdivisionPattern;        // Only one pattern per section
    std::vector<bool> accents;
    QString label;
    bool hasPolyrhythm = false;
    Polyrhythm polyrhythm;
};

struct MetronomePreset {
    QString songName;
    std::vector<MetronomeSection> sections;
};

class PresetManager : public QObject {
    Q_OBJECT
public:
    explicit PresetManager(QObject* parent = nullptr);

    void savePreset(const MetronomePreset& preset);
    bool loadPreset(const QString& songName, MetronomePreset& presetOut) const;
    QStringList listPresetNames() const;
    void removePreset(const QString& songName);

    void loadFromDisk(const QString& filename);
    void saveToDisk(const QString& filename) const;

private:
    QMap<QString, MetronomePreset> presets;
};