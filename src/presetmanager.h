#pragma once

#include <QObject>
#include <QString>
#include <QMap>
#include <QVector>
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
    bool polyrhythmPerBeat = true;
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

    bool exportToFile(const QStringList& names, const QString& filePath) const;
    static QStringList presetsInFile(const QString& filePath);
    static int customPatternsInFile(const QString& filePath);
    bool importFromFile(const QStringList& names, const QString& filePath);

    QVector<SubdivisionPattern> customPatterns() const { return m_customPatterns; }
    void setCustomPatterns(const QVector<SubdivisionPattern>& patterns) { m_customPatterns = patterns; }

private:
    QMap<QString, MetronomePreset> presets;
    QVector<SubdivisionPattern> m_customPatterns;
};
