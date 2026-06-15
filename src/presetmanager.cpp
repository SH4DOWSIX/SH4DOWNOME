#include "presetmanager.h"
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QSet>
#include <QUrl>

PresetManager::PresetManager(QObject* parent) : QObject(parent) {}

// --- Helpers for serializing SubdivisionPattern ---

static QJsonObject toJson(const SubdivisionPattern& pattern) {
    QJsonObject obj;
    obj["category"] = int(pattern.category);
    obj["name"] = pattern.name;
    QJsonArray pulsesArr;
    for (const SubdivisionPulse& p : pattern.pulses) {
        QJsonObject pObj;
        pObj["noteValue"] = noteValueToString(p.noteValue);
        pObj["isRest"] = p.isRest;
        pObj["accent"] = p.accent;
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
        if (pObj.contains("noteValue")) {
            p.noteValue = noteValueFromString(pObj.value("noteValue").toString());
        } else {
            // Legacy migration: reconstruct NoteValue from old float + isDotted fields
            double dur = pObj.value("duration").toDouble(0.25);
            bool dotted = pObj.value("isDotted").toBool(false);
            p.noteValue = noteValueFromLegacy(dur, dotted, false);
        }
        p.isRest = pObj.value("isRest").toBool(false);
        p.accent = pObj.value("accent").toBool(false);
        pattern.pulses.append(p);
    }
    return pattern;
}

// Resolve a file:// or content:// URL string to a usable QFile path
static QString resolveFilePath(const QString& uriOrPath) {
    QUrl url(uriOrPath);
    if (url.scheme() == "file") return url.toLocalFile();
    return uriOrPath; // content:// or plain path — pass through
}

bool PresetManager::exportToFile(const QStringList& names, const QString& filePath) const {
    QJsonObject presetsObj;
    for (const QString& name : names) {
        if (!presets.contains(name)) continue;
        const MetronomePreset& p = presets[name];
        QJsonObject obj;
        QJsonArray sectionsArr;
        for (const MetronomeSection& s : p.sections) {
            QJsonObject secObj;
            secObj["tempo"]       = s.tempo;
            secObj["numerator"]   = s.numerator;
            secObj["denominator"] = s.denominator;
            secObj["subdivisionPattern"] = toJson(s.subdivisionPattern);
            secObj["label"] = s.label;
            QJsonArray arr;
            for (bool a : s.accents) arr.append(a);
            secObj["accents"] = arr;
            secObj["hasPolyrhythm"] = s.hasPolyrhythm;
            if (s.hasPolyrhythm) {
                QJsonObject poly;
                poly["primaryBeats"]   = s.polyrhythm.primaryBeats;
                poly["secondaryBeats"] = s.polyrhythm.secondaryBeats;
                poly["perBeat"]        = s.polyrhythmPerBeat;
                secObj["polyrhythm"] = poly;
            }
            sectionsArr.append(secObj);
        }
        obj["sections"] = sectionsArr;
        presetsObj[name] = obj;
    }
    QJsonArray custArr;
    QMap<QString, QJsonObject> customMap;
    for (const QString& name : names) {
        if (!presets.contains(name)) continue;
        for (const MetronomeSection& s : presets[name].sections) {
            if (s.subdivisionPattern.category == SubdivisionCategory::Custom
                && !customMap.contains(s.subdivisionPattern.name))
                customMap[s.subdivisionPattern.name] = toJson(s.subdivisionPattern);
        }
    }
    for (const auto& v : customMap) custArr.append(v);
    // Also include standalone custom patterns not referenced by any section
    for (const auto& cp : m_customPatterns)
        if (!customMap.contains(cp.name))
            custArr.append(toJson(cp));

    QJsonObject root;
    root["presets"]        = presetsObj;
    root["customPatterns"] = custArr;

    QFile file(resolveFilePath(filePath));
    if (!file.open(QIODevice::WriteOnly)) return false;
    file.write(QJsonDocument(root).toJson());
    file.close();
    return true;
}

QStringList PresetManager::presetsInFile(const QString& filePath) {
    QFile file(resolveFilePath(filePath));
    if (!file.open(QIODevice::ReadOnly)) return {};
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    QJsonObject root = doc.object();
    QJsonObject presetsObj = root.contains("presets") ? root["presets"].toObject() : root;
    QStringList names;
    for (const QString& key : presetsObj.keys()) {
        const QJsonValue& v = presetsObj[key];
        if (!key.trimmed().isEmpty() && v.isObject() && v.toObject().contains("sections"))
            names.append(key);
    }
    return names;
}

static void loadPresetsFromObject(const QJsonObject& presetsObj, QMap<QString, MetronomePreset>& out);

int PresetManager::customPatternsInFile(const QString& filePath) {
    QFile file(resolveFilePath(filePath));
    if (!file.open(QIODevice::ReadOnly)) return 0;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();
    return doc.object()["customPatterns"].toArray().size();
}

bool PresetManager::importFromFile(const QStringList& names, const QString& filePath) {
    QFile file(resolveFilePath(filePath));
    if (!file.open(QIODevice::ReadOnly)) return false;
    QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    file.close();

    QJsonObject root = doc.object();
    QJsonObject presetsObj = root.contains("presets") ? root["presets"].toObject() : root;

    // Parse presets from file into a temporary map
    QMap<QString, MetronomePreset> filePresets;
    loadPresetsFromObject(presetsObj, filePresets);

    // Import selected presets
    bool anyImported = false;
    for (const QString& name : names) {
        if (filePresets.contains(name)) {
            presets[name] = filePresets[name];
            anyImported = true;
        }
    }

    // Merge custom patterns we don't already have
    QSet<QString> existing;
    for (const auto& cp : m_customPatterns) existing.insert(cp.name);
    for (const QJsonValue& v : root["customPatterns"].toArray()) {
        SubdivisionPattern p = fromJson(v.toObject());
        p.category = SubdivisionCategory::Custom;
        if (!p.pulses.isEmpty() && !existing.contains(p.name)) {
            m_customPatterns.append(p);
            existing.insert(p.name);
            anyImported = true;
        }
    }

    return anyImported;
}


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

static void loadPresetsFromObject(const QJsonObject& presetsObj, QMap<QString, MetronomePreset>& out)
{
    for (const QString& key : presetsObj.keys()) {
        if (key.trimmed().isEmpty()) continue;
        QJsonObject obj = presetsObj.value(key).toObject();
        MetronomePreset p;
        p.songName = key;
        QJsonArray sectionsArr = obj.value("sections").toArray();
        for (const QJsonValue& secVal : sectionsArr) {
            QJsonObject secObj = secVal.toObject();
            MetronomeSection s;
            s.tempo = secObj.value("tempo").toInt();
            s.numerator = secObj.value("numerator").toInt();
            s.denominator = secObj.value("denominator").toInt();
            if (secObj.contains("subdivisionPattern") && secObj.value("subdivisionPattern").isObject()) {
                s.subdivisionPattern = fromJson(secObj.value("subdivisionPattern").toObject());
            } else {
                s.subdivisionPattern = SubdivisionPattern{SubdivisionCategory::Standard, "Quarter Note", { {NoteValue::Quarter, false} }};
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
                s.polyrhythmPerBeat = polyObj.contains("perBeat")
                    ? polyObj.value("perBeat").toBool(true)
                    : false;
            }
            p.sections.push_back(s);
        }
        out[p.songName] = p;
    }
}

void PresetManager::loadFromDisk(const QString& filename) {
    QFile file(filename);
    if (!file.open(QIODevice::ReadOnly)) return;
    QByteArray data = file.readAll();
    file.close();

    QJsonDocument doc = QJsonDocument::fromJson(data);
    QJsonObject root = doc.object();
    presets.clear();
    m_customPatterns.clear();

    if (root.contains("presets")) {
        // New unified format
        loadPresetsFromObject(root.value("presets").toObject(), presets);
        for (const QJsonValue& v : root.value("customPatterns").toArray()) {
            SubdivisionPattern p = fromJson(v.toObject());
            p.category = SubdivisionCategory::Custom;
            if (!p.pulses.isEmpty()) m_customPatterns.append(p);
        }
    } else {
        // Legacy flat format (presets only, no customPatterns)
        loadPresetsFromObject(root, presets);
    }
}

void PresetManager::saveToDisk(const QString& filename) const {
    QJsonObject presetsObj;
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
                polyObj["perBeat"] = s.polyrhythmPerBeat;
                secObj["polyrhythm"] = polyObj;
            }
            sectionsArr.append(secObj);
        }
        obj["sections"] = sectionsArr;
        presetsObj[songName] = obj;
    }

    QJsonArray custArr;
    for (const auto& cp : m_customPatterns)
        custArr.append(toJson(cp));

    QJsonObject root;
    root["presets"] = presetsObj;
    root["customPatterns"] = custArr;

    QJsonDocument doc(root);
    QFile file(filename);
    if (!file.open(QIODevice::WriteOnly)) return;
    file.write(doc.toJson());
    file.close();
}
