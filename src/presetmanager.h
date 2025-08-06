#pragma once

#include <QObject>
#include <QString>
#include <QMap>
#include <vector>

struct MetronomeSection {
    int tempo;
    int numerator;
    int denominator;
    int subdivision;
    std::vector<bool> accents;
    QString label;
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