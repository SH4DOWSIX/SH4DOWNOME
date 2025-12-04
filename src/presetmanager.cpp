#include "presetmanager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

// --- Helpers for serializing SubdivisionPattern ---

static QJsonObject toJson(const SubdivisionPattern& pattern) {
    QJsonObject obj;
    obj["category"] = int(pattern.category);
    obj["name"] = pattern.name;
    QJsonArray pulsesArr;
    for (const SubdivisionPulse& p : pattern.pulses) {
        QJsonObject pObj;
        pObj["duration"] = p.duration;
        pObj["isRest"] = p.isRest;
        pObj["isDotted"] = p.isDotted;
        pObj["accent"] = p.accent; // ADD THIS LINE
        pulsesArr.append(pObj);
    }
    obj["pulses"] = pulsesArr;
    return obj;
}

// In the fromJson function (around line 32-38), add this line:
static SubdivisionPattern fromJson(const QJsonObject& obj) {
    SubdivisionPattern pattern;
    pattern.category = SubdivisionCategory(obj.value("category").toInt(0));
    pattern.name = obj.value("name").toString();
    QJsonArray pulsesArr = obj.value("pulses").toArray();
    for (const QJsonValue& pulseVal : pulsesArr) {
        QJsonObject pObj = pulseVal.toObject();
        SubdivisionPulse p;
        p.duration = pObj.value("duration").toDouble(0.25);
        p.isRest = pObj.value("isRest").toBool(false);
        p.isDotted = pObj.value("isDotted").toBool(false);
        p.accent = pObj.value("accent").toBool(false); // ADD THIS LINE
        pattern.pulses.append(p);
    }
    return pattern;
}

PresetManager::PresetManager(QObject* parent)
    : QObject(parent)
{}

void PresetManager::savePreset(const MetronomePreset& preset) {
    if (preset.songName.trimmed().isEmpty()) return;
    presets[preset.songName] = preset;
}

bool PresetManager::loadPreset(const QString& songName, MetronomePreset& presetOut) const {
    if (!presets.contains(songName)) return false;
    presetOut = presets.value(songName);
    return true;
}

QStringList PresetManager::listPresetNames() const {
    return presets.keys();
}

void PresetManager::removePreset(const QString& songName) {
    presets.remove(songName);
}

void PresetManager::loadFromDisk(const QString& filename) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) return;
    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject root = doc.object();
    presets.clear();

    for (const QString& key : root.keys()) {
        if (key.trimmed().isEmpty()) continue;
        QJsonObject obj = root.value(key).toObject();
        MetronomePreset p;
        p.songName = key;
        QJsonArray sectionsArr = obj.value("sections").toArray();
        for (const QJsonValue& secVal : sectionsArr) {
            QJsonObject secObj = secVal.toObject();
            MetronomeSection s;
            s.tempo = secObj.value("tempo").toInt();
            s.numerator = secObj.value("numerator").toInt();
            s.denominator = secObj.value("denominator").toInt();
            // --- Deserialize subdivisionPattern ---
            if (secObj.contains("subdivisionPattern") && secObj.value("subdivisionPattern").isObject()) {
                s.subdivisionPattern = fromJson(secObj.value("subdivisionPattern").toObject());
            } else {
                int legacySubdiv = secObj.value("subdivision").toInt();
                s.subdivisionPattern = SubdivisionPattern{SubdivisionCategory::Standard, "Quarter Note", { {1.0, false} }};
            }
            s.label = secObj.value("label").toString();
            QJsonArray arr = secObj.value("accents").toArray();
            int numBeats = secObj.value("numerator").toInt();
            s.accents.resize(numBeats, false);
            for (int i = 0; i < arr.size() && i < numBeats; ++i)
                s.accents[i] = arr[i].toBool();
            if (arr.isEmpty() && numBeats > 0)
                s.accents[0] = true;
            s.hasPolyrhythm = secObj.value("hasPolyrhythm").toBool(false);
            if (s.hasPolyrhythm) {
                QJsonObject polyObj = secObj.value("polyrhythm").toObject();
                s.polyrhythm.primaryBeats = polyObj.value("primaryBeats").toInt(3);
                s.polyrhythm.secondaryBeats = polyObj.value("secondaryBeats").toInt(2);
            }
            p.sections.push_back(s);
        }
        presets[p.songName] = p;
    }
}

void PresetManager::saveToDisk(const QString& filename) const {
    QJsonObject root;
    for (const auto& songName : presets.keys()) {
        const MetronomePreset& p = presets[songName];
        QJsonObject obj;
        QJsonArray sectionsArr;
        for (const MetronomeSection& s : p.sections) {
            QJsonObject secObj;
            secObj["tempo"] = s.tempo;
            secObj["numerator"] = s.numerator;
            secObj["denominator"] = s.denominator;
            secObj["subdivisionPattern"] = toJson(s.subdivisionPattern);
            secObj["label"] = s.label;
            QJsonArray arr;
            for (bool a : s.accents) arr.append(a);
            secObj["accents"] = arr;
            secObj["hasPolyrhythm"] = s.hasPolyrhythm;
            if (s.hasPolyrhythm) {
                QJsonObject polyObj;
                polyObj["primaryBeats"] = s.polyrhythm.primaryBeats;
                polyObj["secondaryBeats"] = s.polyrhythm.secondaryBeats;
                secObj["polyrhythm"] = polyObj;
            }
            sectionsArr.append(secObj);
        }
        obj["sections"] = sectionsArr;
        root[songName] = obj;
    }
    QJsonDocument doc(root);
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) return;
    file.write(doc.toJson());
    file.close();
}