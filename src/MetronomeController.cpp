#include "MetronomeController.h"
#include "SectionListModel.h"
#include "customsubdivisiondialog.h"
#include "noteassembler.h"
#include "CustomPatternEditor.h"
#include "updatechecker.h"
#include <QCoreApplication>
#include <QStandardPaths>
#include <QDir>
#include <QSettings>
#include <QFile>
#include <QDateTime>
#include <QMessageBox>
#include <QVariantMap>
#include <algorithm>
#include <numeric>

// ─────────────────────────────────────────────────────────────────────────────
// Picker pattern builders (data only, mirrors subdivisionselectordialog.cpp)
// ─────────────────────────────────────────────────────────────────────────────
namespace {

static SubdivisionPulse SP(NoteValue nv, bool rest = false) { return {nv, rest, false}; }

static QVector<SubdivisionPattern> pickerStandard(bool c) {
    QVector<SubdivisionPattern> v;
    if (!c) {
        v << SubdivisionPattern{SubdivisionCategory::Standard, "Quarter Note",
                {SP(NoteValue::Quarter)}}
          << SubdivisionPattern{SubdivisionCategory::Standard, "Eighth Notes",
                {SP(NoteValue::Eighth), SP(NoteValue::Eighth)}}
          << SubdivisionPattern{SubdivisionCategory::Standard, "Sixteenth Notes",
                {SP(NoteValue::Sixteenth), SP(NoteValue::Sixteenth),
                 SP(NoteValue::Sixteenth), SP(NoteValue::Sixteenth)}}
          << SubdivisionPattern{SubdivisionCategory::Standard, "Eighth Triplets",
                {SP(NoteValue::TripletEighth), SP(NoteValue::TripletEighth),
                 SP(NoteValue::TripletEighth)}}
          << SubdivisionPattern{SubdivisionCategory::Standard, "Sixteenth Triplets",
                {SP(NoteValue::TripletSixteenth), SP(NoteValue::TripletSixteenth),
                 SP(NoteValue::TripletSixteenth), SP(NoteValue::TripletSixteenth),
                 SP(NoteValue::TripletSixteenth), SP(NoteValue::TripletSixteenth)}};
    } else {
        v << SubdivisionPattern{SubdivisionCategory::Standard, "Dotted Quarter",
                {SP(NoteValue::DottedQuarter)}}
          << SubdivisionPattern{SubdivisionCategory::Standard, "Eighths (3/beat)",
                {SP(NoteValue::Eighth), SP(NoteValue::Eighth), SP(NoteValue::Eighth)}}
          << SubdivisionPattern{SubdivisionCategory::Standard, "Sixteenths (6/beat)",
                {SP(NoteValue::Sixteenth), SP(NoteValue::Sixteenth), SP(NoteValue::Sixteenth),
                 SP(NoteValue::Sixteenth), SP(NoteValue::Sixteenth), SP(NoteValue::Sixteenth)}};
    }
    return v;
}

static QVector<SubdivisionPattern> pickerComposite(bool c) {
    QVector<SubdivisionPattern> v;
    if (!c) {
        v << SubdivisionPattern{SubdivisionCategory::Composite, "8th + 2×16th",
                {SP(NoteValue::Eighth), SP(NoteValue::Sixteenth), SP(NoteValue::Sixteenth)}}
          << SubdivisionPattern{SubdivisionCategory::Composite, "2×16th + 8th",
                {SP(NoteValue::Sixteenth), SP(NoteValue::Sixteenth), SP(NoteValue::Eighth)}}
          << SubdivisionPattern{SubdivisionCategory::Composite, "16th + 8th + 16th",
                {SP(NoteValue::Sixteenth), SP(NoteValue::Eighth), SP(NoteValue::Sixteenth)}}
          << SubdivisionPattern{SubdivisionCategory::Composite, "Dot.8th + 16th",
                {SP(NoteValue::DottedEighth), SP(NoteValue::Sixteenth)}}
          << SubdivisionPattern{SubdivisionCategory::Composite, "16th + Dot.8th",
                {SP(NoteValue::Sixteenth), SP(NoteValue::DottedEighth)}}
          << SubdivisionPattern{SubdivisionCategory::Composite, "Dot.8th + 2×32nd",
                {SP(NoteValue::DottedEighth), SP(NoteValue::ThirtySecond), SP(NoteValue::ThirtySecond)}}
          << SubdivisionPattern{SubdivisionCategory::Composite, "2×32nd + Dot.8th",
                {SP(NoteValue::ThirtySecond), SP(NoteValue::ThirtySecond), SP(NoteValue::DottedEighth)}}
          << SubdivisionPattern{SubdivisionCategory::Composite, "32nd+Dot.8th+32nd",
                {SP(NoteValue::ThirtySecond), SP(NoteValue::DottedEighth), SP(NoteValue::ThirtySecond)}};
    } else {
        v << SubdivisionPattern{SubdivisionCategory::Composite, "Qtr + 8th",
                {SP(NoteValue::Quarter), SP(NoteValue::Eighth)}}
          << SubdivisionPattern{SubdivisionCategory::Composite, "8th + Qtr",
                {SP(NoteValue::Eighth), SP(NoteValue::Quarter)}}
          << SubdivisionPattern{SubdivisionCategory::Composite, "Dot.8th + 16th + 8th",
                {SP(NoteValue::DottedEighth), SP(NoteValue::Sixteenth), SP(NoteValue::Eighth)}}
          << SubdivisionPattern{SubdivisionCategory::Composite, "16th + Qtr + 16th",
                {SP(NoteValue::Sixteenth), SP(NoteValue::Quarter), SP(NoteValue::Sixteenth)}}
          << SubdivisionPattern{SubdivisionCategory::Composite, "Qtr + 2×16th",
                {SP(NoteValue::Quarter), SP(NoteValue::Sixteenth), SP(NoteValue::Sixteenth)}}
          << SubdivisionPattern{SubdivisionCategory::Composite, "2×16th + Qtr",
                {SP(NoteValue::Sixteenth), SP(NoteValue::Sixteenth), SP(NoteValue::Quarter)}}
          << SubdivisionPattern{SubdivisionCategory::Composite, "8th + Dot.8th + 16th",
                {SP(NoteValue::Eighth), SP(NoteValue::DottedEighth), SP(NoteValue::Sixteenth)}}
          << SubdivisionPattern{SubdivisionCategory::Composite, "16th + Dot.8th + 8th",
                {SP(NoteValue::Sixteenth), SP(NoteValue::DottedEighth), SP(NoteValue::Eighth)}};
    }
    return v;
}

static QVector<SubdivisionPattern> pickerTuplets(bool c) {
    QVector<SubdivisionPattern> v;
    if (!c) {
        v << SubdivisionPattern{SubdivisionCategory::Tuplet, "Quintuplets",
                {SP(NoteValue::QuintupletNote), SP(NoteValue::QuintupletNote),
                 SP(NoteValue::QuintupletNote), SP(NoteValue::QuintupletNote),
                 SP(NoteValue::QuintupletNote)}}
          << SubdivisionPattern{SubdivisionCategory::Tuplet, "Septuplets",
                {SP(NoteValue::SeptupletNote), SP(NoteValue::SeptupletNote),
                 SP(NoteValue::SeptupletNote), SP(NoteValue::SeptupletNote),
                 SP(NoteValue::SeptupletNote), SP(NoteValue::SeptupletNote),
                 SP(NoteValue::SeptupletNote)}};
    } else {
        v << SubdivisionPattern{SubdivisionCategory::Tuplet, "Duplets",
                {SP(NoteValue::DupletNote), SP(NoteValue::DupletNote)}}
          << SubdivisionPattern{SubdivisionCategory::Tuplet, "Quartuplets",
                {SP(NoteValue::QuartupletNote), SP(NoteValue::QuartupletNote),
                 SP(NoteValue::QuartupletNote), SP(NoteValue::QuartupletNote)}}
          << SubdivisionPattern{SubdivisionCategory::Tuplet, "Quintuplets",
                {SP(NoteValue::QuintupletNote), SP(NoteValue::QuintupletNote),
                 SP(NoteValue::QuintupletNote), SP(NoteValue::QuintupletNote),
                 SP(NoteValue::QuintupletNote)}};
    }
    return v;
}

} // namespace

// ─────────────────────────────────────────────────────────────────────────────
// Italian tempo marking table (mirrors mainwindow.cpp)
// ─────────────────────────────────────────────────────────────────────────────
static const struct {
    const char* name;
    int minBpm;
    int maxBpm;
} kTempoMarkings[] = {
    {"Grave",       1,   40},
    {"Largo",       40,  60},
    {"Larghetto",   60,  66},
    {"Adagio",      66,  76},
    {"Andante",     76,  108},
    {"Moderato",    108, 120},
    {"Allegro",     120, 168},
    {"Vivace",      168, 176},
    {"Presto",      176, 200},
    {"Prestissimo", 200, 208},
    {"Speeeeed!",   209, 300},
};

/*static*/ QString MetronomeController::getTempoMarkings(int bpm)
{
    QStringList names;
    for (const auto& m : kTempoMarkings)
        if (bpm >= m.minBpm && bpm <= m.maxBpm)
            names << QString::fromLatin1(m.name);
    return names.join(" / ");
}

static int sectionBeatCount(const MetronomeSection& s)
{
    if (s.denominator == 8 && s.numerator % 3 == 0 && s.numerator > 3)
        return qMax(1, s.numerator / 3);
    return qMax(1, s.numerator);
}

static Polyrhythm enginePolyrhythmForSection(const MetronomeSection& s)
{
    Polyrhythm p = s.polyrhythm;
    if (s.polyrhythmPerBeat) {
        const int beats = sectionBeatCount(s);
        p.primaryBeats *= beats;
        p.secondaryBeats *= beats;
    }
    return p;
}

// ─────────────────────────────────────────────────────────────────────────────
// Construction / destruction
// ─────────────────────────────────────────────────────────────────────────────
MetronomeController::MetronomeController(QObject* parent)
    : QObject(parent)
{
    m_sectionModel = new SectionListModel(this);
    m_patternEditor = new CustomPatternEditor(&metronome, this);

    // Countdown timer (1 Hz)
    m_timerTimer = new QTimer(this);
    m_timerTimer->setInterval(1000);
    connect(m_timerTimer, &QTimer::timeout, this, &MetronomeController::onTimerTick);

    // OBS pulse reset
    m_obsPulseTimer = new QTimer(this);
    m_obsPulseTimer->setSingleShot(true);
    m_obsPulseTimer->setInterval(80);
    connect(m_obsPulseTimer, &QTimer::timeout, this, &MetronomeController::onObsPulseReset);

    // Tap-tempo resume timer
    m_tapTempoResumeTimer = new QTimer(this);
    m_tapTempoResumeTimer->setSingleShot(true);
    m_tapTempoResumeTimer->setInterval(2000);
    connect(m_tapTempoResumeTimer, &QTimer::timeout, this, [this]() {
        if (m_metronomeWasRunning) {
            metronome.start();
            m_metronomeWasRunning = false;
            emit runningChanged();
        }
    });

    // Pulse handler
    connect(&metronome, &MetronomeEngine::pulse,
            this, &MetronomeController::onMetronomePulse);
    connect(&metronome, &MetronomeEngine::tempoSteppedUp,
            this, &MetronomeController::onTempoSteppedUp);

    loadSettings();

    // Load presets and select first (or create default)
    m_presetManager.loadFromDisk(presetFilePath());

    // One-time migration: import custom_subdivisions.dat into presets.json
    {
        QString oldFile = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/custom_subdivisions.dat";
        if (m_presetManager.customPatterns().isEmpty() && QFile::exists(oldFile)) {
            QSettings s(oldFile, QSettings::IniFormat);
            QVector<SubdivisionPattern> patterns;
            int count = s.beginReadArray("CustomPatterns");
            for (int i = 0; i < count; ++i) {
                s.setArrayIndex(i);
                SubdivisionPattern p;
                p.category = SubdivisionCategory::Custom;
                p.name = s.value("name", QString("Custom %1").arg(i + 1)).toString();
                int pc = s.beginReadArray("pulses");
                for (int j = 0; j < pc; ++j) {
                    s.setArrayIndex(j);
                    SubdivisionPulse pulse;
                    QString nvStr = s.value("noteValue").toString();
                    if (!nvStr.isEmpty())
                        pulse.noteValue = noteValueFromString(nvStr);
                    else
                        pulse.noteValue = noteValueFromLegacy(
                            s.value("duration", 0.5).toDouble(),
                            s.value("isDotted", false).toBool(), false);
                    pulse.isRest = s.value("isRest", false).toBool();
                    pulse.accent = s.value("accent", false).toBool();
                    p.pulses.append(pulse);
                }
                s.endArray();
                if (!p.pulses.isEmpty()) patterns.append(p);
            }
            s.endArray();
            if (!patterns.isEmpty()) {
                m_presetManager.setCustomPatterns(patterns);
                m_presetManager.saveToDisk(presetFilePath());
                QFile::remove(oldFile);
            }
        }
    }
    if (m_presetManager.listPresetNames().isEmpty()) {
        // Bootstrap a default preset
        m_currentPreset.songName = "";
        m_currentPreset.sections.clear();
    } else {
        QString first = m_presetManager.listPresetNames().first();
        m_presetManager.loadPreset(first, m_currentPreset);
    }

    if (m_currentPreset.sections.empty()) {
        MetronomeSection s;
        s.label       = "Section 1";
        s.tempo       = 120;
        s.numerator   = 4;
        s.denominator = 4;
        s.subdivisionPattern = getDefaultSubdivisionPattern();
        s.hasPolyrhythm = false;
        s.polyrhythm.primaryBeats   = 3;
        s.polyrhythm.secondaryBeats = 2;
        s.polyrhythmPerBeat = true;
        s.accents.resize(4, false);
        m_currentPreset.sections.push_back(s);
    }

    refreshSectionModel();
    loadSectionToEngine(0);
    buildPickerPatterns();
}

MetronomeController::~MetronomeController()
{
    metronome.stop();
}

// ─────────────────────────────────────────────────────────────────────────────
// Paths
// ─────────────────────────────────────────────────────────────────────────────
QString MetronomeController::settingsPath() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/settings.ini";
}

QString MetronomeController::presetFilePath() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/presets.json";
}

// ─────────────────────────────────────────────────────────────────────────────
// Settings
// ─────────────────────────────────────────────────────────────────────────────
void MetronomeController::loadSettings()
{
    QDir().mkpath(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    QSettings s(settingsPath(), QSettings::IniFormat);
    m_accentColor = QColor(s.value("accentColor", "#960000").toString());
    m_soundSet    = s.value("soundSet", "Default").toString();
    m_obsHidden   = s.value("obsHidden", true).toBool();
    m_alwaysOnTop = s.value("alwaysOnTop", false).toBool();
    m_beatWindowAuto = s.value("beatWindowAuto", false).toBool();
    m_beatWindowSubdivisionStyle = qBound(0, s.value("beatWindowSubdivisionStyle", 0).toInt(), 4);
    m_beatWindowPolyrhythmStyle = s.value("beatWindowPolyrhythmStyle", 5).toInt();
    if (m_beatWindowPolyrhythmStyle != 0 && m_beatWindowPolyrhythmStyle != 5)
        m_beatWindowPolyrhythmStyle = 5;
    m_volume      = s.value("volume", 90).toInt();
    m_terminology = s.value("terminology", "Piece").toString();
    if (m_terminology != "Piece" && m_terminology != "Song" && m_terminology != "Preset")
        m_terminology = "Piece";

    metronome.loadSample("accent", soundFileForSet(m_soundSet, true));
    metronome.loadSample("click",  soundFileForSet(m_soundSet, false));
    metronome.setAccentSound("accent");
    metronome.setClickSound("click");
    metronome.setVolume(m_volume / 100.0f);
}

void MetronomeController::saveSettings()
{
    QSettings s(settingsPath(), QSettings::IniFormat);
    s.setValue("accentColor", m_accentColor.name());
    s.setValue("soundSet",    m_soundSet);
    s.setValue("obsHidden",   m_obsHidden);
    s.setValue("alwaysOnTop", m_alwaysOnTop);
    s.setValue("beatWindowAuto", m_beatWindowAuto);
    s.setValue("beatWindowSubdivisionStyle", m_beatWindowSubdivisionStyle);
    s.setValue("beatWindowPolyrhythmStyle", m_beatWindowPolyrhythmStyle);
    s.setValue("volume",      m_volume);
    s.setValue("terminology", m_terminology);
    s.sync();
}

// ─────────────────────────────────────────────────────────────────────────────
// Internal helpers
// ─────────────────────────────────────────────────────────────────────────────
SubdivisionPattern MetronomeController::getDefaultSubdivisionPattern() const
{
    SubdivisionPattern p;
    p.name   = "Quarter Note";
    p.pulses = { {NoteValue::Quarter, false, false} };
    return p;
}

QString MetronomeController::soundFileForSet(const QString& set, bool accent) const
{
    if (set == "Default")   return accent ? ":/resources/accent.wav"          : ":/resources/click.wav";
    if (set == "Wooden" || set == "Woodblock")
        return accent ? ":/resources/woodblock_accent.wav" : ":/resources/woodblock.wav";
    if (set == "Wooden 2")
        return accent ? ":/resources/wooden_accent.wav" : ":/resources/wooden.wav";
    if (set == "Wooden 3" || set == "Woodblock 2")
        return accent ? ":/resources/wooden2_accent.wav" : ":/resources/wooden2.wav";
    if (set == "Bongo")     return accent ? ":/resources/bongo_accent.wav"     : ":/resources/bongo.wav";
    if (set == "Cowbell")   return accent ? ":/resources/cowbell_accent.wav"   : ":/resources/cowbell.wav";
    if (set == "Digital")   return accent ? ":/resources/digital_accent.wav"   : ":/resources/digital.wav";
    if (set == "Drum")      return accent ? ":/resources/drum_accent.wav"      : ":/resources/drum.wav";
    if (set == "Hihat")     return accent ? ":/resources/hihat_accent.wav"     : ":/resources/hihat.wav";
    if (set == "Metal")     return accent ? ":/resources/metal_accent.wav"     : ":/resources/metal.wav";
    return accent ? ":/resources/accent.wav" : ":/resources/click.wav";
}

void MetronomeController::updateStartStopLabel(const QString& label)
{
    if (m_startStopLabel != label) {
        m_startStopLabel = label;
        emit startStopLabelChanged();
    }
}

void MetronomeController::updateBeatIndicator(int beats, int subs, int beat, int sub,
                                              int mode, int gridHighlight,
                                              int polyMain, int polySec)
{
    m_biBeats          = beats;
    m_biSubdivisions   = subs;
    m_biCurrentBeat    = beat;
    m_biCurrentSub     = sub;
    m_biMode           = mode;
    m_biGridHighlight  = gridHighlight;
    if (polyMain  > 0) m_biPolyMain      = polyMain;
    if (polySec   > 0) m_biPolySecondary = polySec;
    emit beatIndicatorChanged();
}

void MetronomeController::notifySectionTableEnabled()
{
    emit sectionTableEnabledChanged();
}

void MetronomeController::triggerObsPulse()
{
    m_obsPulse = true;
    emit obsPulseChanged();
    m_obsPulseTimer->start();
}

void MetronomeController::onObsPulseReset()
{
    m_obsPulse = false;
    emit obsPulseChanged();
}

void MetronomeController::resetSpeedTrainer()
{
    m_speedTrainerCountingIn    = false;
    m_speedTrainerTotalBarCounter = 0;
    m_playingBarCounter         = 0;
    m_speedTrainerCurrentTempo  = m_speedTrainerStartTempo;
    m_speedTrainerPolyFirstCycle = true;
}

void MetronomeController::startCountIn()
{
    metronome.stop();
    metronome.startWithCountIn(m_numerator);
    m_speedTrainerCountingIn = true;
    notifySectionTableEnabled();
    emit runningChanged();
}

void MetronomeController::stopAll()
{
    m_speedTrainerCountingIn  = false;
    m_playingBarCounter       = 0;
    m_polyrhythmCycleActive   = false;
    m_speedTrainerPolyFirstCycle = true;
    metronome.stop();
    m_timerTimer->stop();
    m_timerRemaining = m_timerTotalSeconds;
    emit timerRemainingChanged();
}

// ─────────────────────────────────────────────────────────────────────────────
// Load a section's data into the metronome engine
// ─────────────────────────────────────────────────────────────────────────────
void MetronomeController::loadSectionToEngine(int idx)
{
    if (idx < 0 || idx >= static_cast<int>(m_currentPreset.sections.size()))
        return;

    m_currentSectionIdx = idx;
    const MetronomeSection& s = m_currentPreset.sections[idx];

    m_tempo       = s.tempo;
    m_numerator   = s.numerator;
    m_denominator = s.denominator;

    metronome.setTempo(s.tempo);
    metronome.setTimeSignature(s.numerator, s.denominator);
    metronome.setSubdivisionPattern(s.subdivisionPattern);
    metronome.setAccentPattern(s.accents);
    metronome.setPolyrhythmEnabled(s.hasPolyrhythm);
    if (s.hasPolyrhythm) {
        Polyrhythm enginePoly = enginePolyrhythmForSection(s);
        metronome.setPolyrhythm(enginePoly.primaryBeats, enginePoly.secondaryBeats);
    }

    m_playingBarCounter   = 0;
    m_polyrhythmCycleActive = false;

    // Update beat indicator to idle state
    if (s.hasPolyrhythm) {
        Polyrhythm enginePoly = enginePolyrhythmForSection(s);
        updateBeatIndicator(enginePoly.primaryBeats, enginePoly.secondaryBeats,
                            0, 0, 1, -1,
                            enginePoly.primaryBeats, enginePoly.secondaryBeats);
    } else {
        int bpb = s.numerator;
        if (s.denominator == 8 && s.numerator % 3 == 0 && s.numerator > 3)
            bpb = s.numerator / 3;
        int subs = qMax(1, static_cast<int>(s.subdivisionPattern.pulses.size()));
        updateBeatIndicator(bpb, subs, 0, 0, 0);
    }

    m_subdivisionRevision++;
    emit subdivisionChanged();
    buildPickerPatterns();
    emit tempoChanged();
    emit timeSignatureChanged();
    emit accentsChanged();
    emit showAccentsChanged();
    emit currentSectionChanged();
    emit currentSectionIndexChanged();
}

// ─────────────────────────────────────────────────────────────────────────────
// Refresh the QML section model from the current preset
// ─────────────────────────────────────────────────────────────────────────────
void MetronomeController::refreshSectionModel()
{
    m_sectionModel->resetSections(m_currentPreset.sections, m_currentSectionIdx);
}

// ─────────────────────────────────────────────────────────────────────────────
// Property getters
// ─────────────────────────────────────────────────────────────────────────────
bool MetronomeController::running() const
{
    return metronome.isRunning() || m_speedTrainerCountingIn;
}

QString MetronomeController::tempoMarkings() const
{
    return getTempoMarkings(m_tempo);
}

QString MetronomeController::timerRemainingString() const
{
    int rem   = m_timerRemaining;
    int hours = rem / 3600;
    int mins  = (rem % 3600) / 60;
    int secs  = rem % 60;
    if (hours > 0)
        return QString("%1:%2:%3")
            .arg(hours)
            .arg(mins, 2, 10, QChar('0'))
            .arg(secs, 2, 10, QChar('0'));
    return QString("%1:%2")
        .arg(mins, 2, 10, QChar('0'))
        .arg(secs, 2, 10, QChar('0'));
}

bool MetronomeController::polyrhythmEnabled() const
{
    if (m_currentSectionIdx < 0 ||
        m_currentSectionIdx >= static_cast<int>(m_currentPreset.sections.size()))
        return false;
    return m_currentPreset.sections[m_currentSectionIdx].hasPolyrhythm;
}

int MetronomeController::polyPrimaryBeats() const
{
    if (m_currentSectionIdx < 0 ||
        m_currentSectionIdx >= static_cast<int>(m_currentPreset.sections.size()))
        return 3;
    return m_currentPreset.sections[m_currentSectionIdx].polyrhythm.primaryBeats;
}

int MetronomeController::polySecondaryBeats() const
{
    if (m_currentSectionIdx < 0 ||
        m_currentSectionIdx >= static_cast<int>(m_currentPreset.sections.size()))
        return 2;
    return m_currentPreset.sections[m_currentSectionIdx].polyrhythm.secondaryBeats;
}

bool MetronomeController::polyrhythmPerBeat() const
{
    if (m_currentSectionIdx < 0 ||
        m_currentSectionIdx >= static_cast<int>(m_currentPreset.sections.size()))
        return true;
    return m_currentPreset.sections[m_currentSectionIdx].polyrhythmPerBeat;
}

QVariantList MetronomeController::accents() const
{
    QVariantList result;
    if (m_currentSectionIdx < 0 ||
        m_currentSectionIdx >= static_cast<int>(m_currentPreset.sections.size()))
        return result;
    for (bool v : m_currentPreset.sections[m_currentSectionIdx].accents)
        result.append(v);
    return result;
}

bool MetronomeController::showAccents() const
{
    if (m_currentSectionIdx < 0 ||
        m_currentSectionIdx >= static_cast<int>(m_currentPreset.sections.size()))
        return true;
    const auto& pat = m_currentPreset.sections[m_currentSectionIdx].subdivisionPattern;
    bool isPoly = m_currentPreset.sections[m_currentSectionIdx].hasPolyrhythm;
    return !isPoly && (pat.category != SubdivisionCategory::Custom);
}

QString MetronomeController::subdivisionName() const
{
    if (m_currentSectionIdx < 0 ||
        m_currentSectionIdx >= static_cast<int>(m_currentPreset.sections.size()))
        return "Quarter Note";
    return m_currentPreset.sections[m_currentSectionIdx].subdivisionPattern.name;
}

bool MetronomeController::currentSubdivisionIsCustom() const
{
    if (m_currentSectionIdx < 0 ||
        m_currentSectionIdx >= static_cast<int>(m_currentPreset.sections.size()))
        return false;
    return m_currentPreset.sections[m_currentSectionIdx].subdivisionPattern.category == SubdivisionCategory::Custom;
}

QVariantList MetronomeController::currentSubdivisionPulses() const
{
    QVariantList result;
    if (m_currentSectionIdx < 0 ||
        m_currentSectionIdx >= static_cast<int>(m_currentPreset.sections.size()))
        return result;

    const auto& pulses = m_currentPreset.sections[m_currentSectionIdx].subdivisionPattern.pulses;
    for (const auto& pulse : pulses) {
        QVariantMap item;
        item["rest"] = pulse.isRest;
        item["accent"] = pulse.accent;
        result.append(item);
    }
    return result;
}

bool MetronomeController::isCurrentPickerPattern(int cat, int idx) const
{
    if (m_currentSectionIdx < 0 ||
        m_currentSectionIdx >= static_cast<int>(m_currentPreset.sections.size()))
        return false;
    if (cat < 0 || cat >= m_pickerPatterns.size()) return false;
    const auto& v = m_pickerPatterns[cat];
    if (idx < 0 || idx >= v.size()) return false;
    const auto& current  = m_currentPreset.sections[m_currentSectionIdx].subdivisionPattern;
    const auto& candidate = v[idx];
    if (current.pulses.size() != candidate.pulses.size()) return false;
    for (int i = 0; i < current.pulses.size(); ++i) {
        if (current.pulses[i].noteValue != candidate.pulses[i].noteValue) return false;
        if (current.pulses[i].isRest    != candidate.pulses[i].isRest)    return false;
    }
    return true;
}

QObject* MetronomeController::sectionModel() const
{
    return m_sectionModel;
}

bool MetronomeController::sectionTableEnabled() const
{
    return !(m_speedEnabled && (metronome.isRunning() || m_speedTrainerCountingIn));
}

// ─────────────────────────────────────────────────────────────────────────────
// Note image helpers (called by NoteImageProvider)
// ─────────────────────────────────────────────────────────────────────────────
QPixmap MetronomeController::currentSubdivisionPixmap(const QSize& size) const
{
    if (m_currentSectionIdx < 0 ||
        m_currentSectionIdx >= static_cast<int>(m_currentPreset.sections.size()))
        return QPixmap(size);
    const auto& pat = m_currentPreset.sections[m_currentSectionIdx].subdivisionPattern;
    NoteAssembler assembler;
    NoteAssemblerConfig cfg = buildNoteAssemblerConfig(pat);
    cfg.centerVertically = true;
    cfg.pixmapSize = size;
    return assembler.assembleNote(cfg);
}

QPixmap MetronomeController::sectionSubdivisionPixmap(int idx, const QSize& size) const
{
    if (idx < 0 || idx >= static_cast<int>(m_currentPreset.sections.size()))
        return QPixmap(size);
    const auto& pat = m_currentPreset.sections[idx].subdivisionPattern;
    NoteAssembler assembler;
    NoteAssemblerConfig cfg = buildNoteAssemblerConfig(pat);
    cfg.centerVertically = true;
    cfg.pixmapSize = size;
    return assembler.assembleNote(cfg);
}

// ─────────────────────────────────────────────────────────────────────────────
// Setters
// ─────────────────────────────────────────────────────────────────────────────
void MetronomeController::setTempo(int tempo)
{
    tempo = qBound(1, tempo, 300);
    if (m_tempo == tempo) return;
    m_tempo = tempo;

    // Don't override speed trainer while running
    bool canPersistSectionTempo = !m_speedEnabled || (!metronome.isRunning() && !m_speedTrainerCountingIn);
    if (canPersistSectionTempo) {
        metronome.setTempo(tempo);
        if (metronome.isRunning()) { metronome.stop(); metronome.start(); }
        if (m_currentSectionIdx >= 0 &&
            m_currentSectionIdx < static_cast<int>(m_currentPreset.sections.size()))
        {
            m_currentPreset.sections[m_currentSectionIdx].tempo = tempo;
            m_sectionModel->updateRow(m_currentSectionIdx,
                                      m_currentPreset.sections[m_currentSectionIdx],
                                      m_currentSectionIdx);
            m_presetManager.savePreset(m_currentPreset);
            m_presetManager.saveToDisk(presetFilePath());
        }
    }
    emit tempoChanged();
}

void MetronomeController::setVolume(int volume)
{
    volume = qBound(0, volume, 100);
    if (m_volume == volume) return;
    m_volume = volume;
    metronome.setVolume(volume / 100.0f);
    saveSettings();
    emit volumeChanged();
}

void MetronomeController::setTimerTotalSeconds(int seconds)
{
    if (m_timerTotalSeconds == seconds) return;
    m_timerTotalSeconds = qMax(0, seconds);
    m_timerRemaining    = m_timerTotalSeconds;
    emit timerTotalSecondsChanged();
    emit timerRemainingChanged();
}

void MetronomeController::setSpeedBarsPerStep(int v)
{
    if (m_speedBarsPerStep == v) return;
    m_speedBarsPerStep = qMax(1, v);
    if (metronome.isRunning()) stopAll();
    resetSpeedTrainer();
    emit speedSettingsChanged();
}

void MetronomeController::setSpeedTempoStep(int v)
{
    if (m_speedTempoStep == v) return;
    m_speedTempoStep = qMax(1, v);
    if (metronome.isRunning()) stopAll();
    resetSpeedTrainer();
    emit speedSettingsChanged();
}

void MetronomeController::setSpeedMaxTempo(int v)
{
    if (m_speedMaxTempo == v) return;
    m_speedMaxTempo = qBound(1, v, 300);
    if (metronome.isRunning()) stopAll();
    resetSpeedTrainer();
    emit speedSettingsChanged();
}

// ─────────────────────────────────────────────────────────────────────────────
// Invokables — playback
// ─────────────────────────────────────────────────────────────────────────────
void MetronomeController::startStop()
{
    // ---- STOP ----
    if (metronome.isRunning() || m_speedTrainerCountingIn) {
        stopAll();
        m_timerRemaining = m_timerTotalSeconds;
        emit timerRemainingChanged();

        if (m_speedEnabled && m_currentSectionIdx >= 0 &&
            m_currentSectionIdx < static_cast<int>(m_currentPreset.sections.size()))
        {
            int restoreTempo = m_currentPreset.sections[m_currentSectionIdx].tempo;
            m_speedTrainerStartTempo = restoreTempo; // resetSpeedTrainer() will sync CurrentTempo
            m_tempo = restoreTempo;
            metronome.setTempo(restoreTempo);
            emit tempoChanged();
        }

        resetSpeedTrainer();
        updateStartStopLabel("Start");
        notifySectionTableEnabled();
        emit runningChanged();
        return;
    }

    // ---- START ----
    m_lastBarIdx = -1;
    m_speedTrainerTotalBarCounter = 0;

    // Start timer if enabled
    if (m_timerEnabled && m_timerTotalSeconds > 0) {
        m_timerRemaining = m_timerTotalSeconds;
        m_timerTimer->start();
        emit timerRemainingChanged();
    }

    // Always pass speed trainer params to engine before starting
    metronome.setSpeedTrainer(m_speedEnabled, m_speedBarsPerStep, m_speedTempoStep, m_speedMaxTempo);
    metronome.setCountInEnabled(m_countInEnabled);

    if (m_speedEnabled) {
        m_speedTrainerStartTempo   = m_tempo;
        m_speedTrainerCurrentTempo = m_tempo;
        m_speedTrainerPolyFirstCycle = true;
        m_playingBarCounter = 1;
        metronome.setTempo(m_speedTrainerStartTempo);
        updateStartStopLabel(QString("Bar 1/%1\nStop").arg(m_speedBarsPerStep));
        notifySectionTableEnabled();
    }

    if (m_countInEnabled) {
        m_speedTrainerCountingIn = true;
        // metronome.start() will use m_countInEnabled to start with count-in bar
        metronome.start();
        notifySectionTableEnabled();
        emit runningChanged();
        return;
    }

    // Normal / speed-trainer start (count-in handled if m_countInEnabled)
    m_playingBarCounter = qMax(1, m_playingBarCounter);
    m_lastBarIdx = 0;
    m_polyrhythmCycleActive = false;
    if (!m_speedEnabled) {
        if (m_currentSectionIdx >= 0 &&
            m_currentSectionIdx < static_cast<int>(m_currentPreset.sections.size()))
        {
            const auto& s = m_currentPreset.sections[m_currentSectionIdx];
            updateStartStopLabel(QString("Bar 1/%1\nStop").arg(s.numerator));
        }
        m_playingBarCounter = 0;
    }
    metronome.start();
    emit runningChanged();
}

void MetronomeController::tapTempo()
{
    const int tapResetMs = 2000;
    qint64 now = QDateTime::currentMSecsSinceEpoch();

    if (m_tapTimes.isEmpty() || (now - m_tapTimes.last()) > tapResetMs) {
        if (metronome.isRunning()) {
            m_metronomeWasRunning = true;
            metronome.stop();
            emit runningChanged();
        } else {
            m_metronomeWasRunning = false;
        }
        m_tapTimes.clear();
    }

    m_tapTimes.append(now);

    if (m_tapTimes.size() >= 2) {
        QList<qint64> intervals;
        for (int i = 1; i < m_tapTimes.size(); ++i)
            intervals.append(m_tapTimes[i] - m_tapTimes[i - 1]);
        double avgInterval = std::accumulate(intervals.begin(), intervals.end(), 0LL) /
                             static_cast<double>(intervals.size());
        int bpm = static_cast<int>(60000.0 / avgInterval + 0.5);
        bpm = qBound(1, bpm, 300);
        setTempo(bpm);
    }

    if (m_tapTimes.size() > 8) m_tapTimes.removeFirst();
    m_tapTempoResumeTimer->start(tapResetMs);
}

// ─────────────────────────────────────────────────────────────────────────────
// Invokables — time signature
// ─────────────────────────────────────────────────────────────────────────────
void MetronomeController::setupTimeSignature(int num, int den)
{
    bool wasCompound = (m_denominator == 8 && m_numerator % 3 == 0 && m_numerator > 3);
    bool isCompound  = (den           == 8 && num           % 3 == 0 && num           > 3);

    m_numerator   = num;
    m_denominator = den;

    if (m_currentSectionIdx < 0 ||
        m_currentSectionIdx >= static_cast<int>(m_currentPreset.sections.size()))
    {
        emit timeSignatureChanged();
        return;
    }

    MetronomeSection& s = m_currentPreset.sections[m_currentSectionIdx];
    s.numerator   = num;
    s.denominator = den;

    // Reset accents for new beat count
    int accentCount = num;
    if (den == 8 && num % 3 == 0 && num > 3) accentCount = num / 3;
    s.accents.resize(accentCount, false);
    std::fill(s.accents.begin(), s.accents.end(), false);
    metronome.setAccentPattern(s.accents);

    // Auto-switch subdivision on compound-change
    if (wasCompound != isCompound) {
        if (isCompound) {
            SubdivisionPattern dq;
            dq.name   = "Dotted Quarter";
            dq.pulses = { {NoteValue::DottedQuarter, false, false} };
            s.subdivisionPattern = dq;
        } else {
            s.subdivisionPattern = getDefaultSubdivisionPattern();
        }
        metronome.setSubdivisionPattern(s.subdivisionPattern);
        m_subdivisionRevision++;
        emit subdivisionChanged();
        buildPickerPatterns();  // rebuild for new compound state
    }

    metronome.setTimeSignature(num, den);

    // Update beat indicator idle state
    if (s.hasPolyrhythm) {
        Polyrhythm enginePoly = enginePolyrhythmForSection(s);
        metronome.setPolyrhythm(enginePoly.primaryBeats, enginePoly.secondaryBeats);
        updateBeatIndicator(enginePoly.primaryBeats, enginePoly.secondaryBeats,
                            0, 0, 1, -1,
                            enginePoly.primaryBeats, enginePoly.secondaryBeats);
    } else {
        int bpb  = num;
        if (den == 8 && num % 3 == 0 && num > 3) bpb = num / 3;
        int subs = qMax(1, static_cast<int>(s.subdivisionPattern.pulses.size()));
        updateBeatIndicator(bpb, subs, 0, 0, 0);
    }

    // Restart if running in polyrhythm
    if (metronome.isRunning() && s.hasPolyrhythm) {
        metronome.stop(); metronome.start();
    }

    m_sectionModel->updateRow(m_currentSectionIdx, s, m_currentSectionIdx);
    m_presetManager.savePreset(m_currentPreset);
    m_presetManager.saveToDisk(presetFilePath());

    emit timeSignatureChanged();
    emit accentsChanged();
    emit showAccentsChanged();
}

void MetronomeController::requestTimeSignature(int num, int den)
{
    setupTimeSignature(num, den);
}

// ─────────────────────────────────────────────────────────────────────────────
// Invokables — subdivision picker
// ─────────────────────────────────────────────────────────────────────────────
void MetronomeController::openSubdivisionPicker()
{
    // No-op: subdivision picking is handled entirely by SubdivisionPickerSheet.qml
}

void MetronomeController::applySubdivisionToSection(int idx, const SubdivisionPattern& pattern)
{
    if (idx < 0 || idx >= static_cast<int>(m_currentPreset.sections.size())) return;
    MetronomeSection& s = m_currentPreset.sections[idx];

    s.subdivisionPattern = pattern;

    // Resize accents to match beat count, preserving existing values
    int accentCount = s.numerator;
    if (s.denominator == 8 && s.numerator % 3 == 0 && s.numerator > 3)
        accentCount = s.numerator / 3;
    s.accents.resize(accentCount, false);
    metronome.setAccentPattern(s.accents);
    metronome.setSubdivisionPattern(pattern);

    if (metronome.isRunning()) { metronome.stop(); metronome.start(); }

    m_subdivisionRevision++;
    emit subdivisionChanged();
    emit accentsChanged();
    emit showAccentsChanged();
    emit currentSectionChanged();

    m_sectionModel->updateRow(idx, s, m_currentSectionIdx);
    m_presetManager.savePreset(m_currentPreset);
    m_presetManager.saveToDisk(presetFilePath());
}

// ─────────────────────────────────────────────────────────────────────────────
// Invokables — polyrhythm
// ─────────────────────────────────────────────────────────────────────────────
void MetronomeController::togglePolyrhythm()
{
    if (m_speedEnabled && (metronome.isRunning() || m_speedTrainerCountingIn))
        return;  // blocked while speed trainer is running
    if (m_currentSectionIdx < 0 ||
        m_currentSectionIdx >= static_cast<int>(m_currentPreset.sections.size()))
        return;

    MetronomeSection& s = m_currentPreset.sections[m_currentSectionIdx];
    s.hasPolyrhythm = !s.hasPolyrhythm;
    metronome.setPolyrhythmEnabled(s.hasPolyrhythm);
    if (s.hasPolyrhythm) {
        Polyrhythm enginePoly = enginePolyrhythmForSection(s);
        metronome.setPolyrhythm(enginePoly.primaryBeats, enginePoly.secondaryBeats);
    }

    if (s.hasPolyrhythm) {
        Polyrhythm enginePoly = enginePolyrhythmForSection(s);
        updateBeatIndicator(enginePoly.primaryBeats, enginePoly.secondaryBeats,
                            0, 0, 1, -1,
                            enginePoly.primaryBeats, enginePoly.secondaryBeats);
    } else {
        int bpb  = s.numerator;
        if (s.denominator == 8 && s.numerator % 3 == 0 && s.numerator > 3) bpb = s.numerator / 3;
        int subs = qMax(1, static_cast<int>(s.subdivisionPattern.pulses.size()));
        updateBeatIndicator(bpb, subs, 0, 0, 0);
    }

    if (metronome.isRunning()) {
        m_playingBarCounter     = 0;
        m_polyrhythmCycleActive = false;
        m_polyrhythmJustRestarted = true;
        m_lastBarIdx = 0;
        metronome.stop(); metronome.start();
    }

    m_sectionModel->updateRow(m_currentSectionIdx, s, m_currentSectionIdx);
    m_presetManager.savePreset(m_currentPreset);
    m_presetManager.saveToDisk(presetFilePath());

    emit currentSectionChanged();
    emit showAccentsChanged();
    emit accentsChanged();
}

void MetronomeController::setPolyrhythm(int primary, int secondary, bool perBeat)
{
    if (m_currentSectionIdx < 0 ||
        m_currentSectionIdx >= static_cast<int>(m_currentPreset.sections.size()))
        return;
    MetronomeSection& s = m_currentPreset.sections[m_currentSectionIdx];
    s.polyrhythm.primaryBeats   = primary;
    s.polyrhythm.secondaryBeats = secondary;
    s.polyrhythmPerBeat = perBeat;
    Polyrhythm enginePoly = enginePolyrhythmForSection(s);
    metronome.setPolyrhythm(enginePoly.primaryBeats, enginePoly.secondaryBeats);

    if (s.hasPolyrhythm) {
        updateBeatIndicator(enginePoly.primaryBeats, enginePoly.secondaryBeats,
                            0, 0, 1, -1,
                            enginePoly.primaryBeats, enginePoly.secondaryBeats);
        if (metronome.isRunning()) { metronome.stop(); metronome.start(); }
    }

    m_sectionModel->updateRow(m_currentSectionIdx, s, m_currentSectionIdx);
    m_presetManager.savePreset(m_currentPreset);
    m_presetManager.saveToDisk(presetFilePath());
    emit currentSectionChanged();
}

// ─────────────────────────────────────────────────────────────────────────────
// Invokables — accents
// ─────────────────────────────────────────────────────────────────────────────
void MetronomeController::setAccent(int index, bool value)
{
    if (m_currentSectionIdx < 0 ||
        m_currentSectionIdx >= static_cast<int>(m_currentPreset.sections.size()))
        return;
    auto& accents = m_currentPreset.sections[m_currentSectionIdx].accents;
    if (index < 0 || index >= static_cast<int>(accents.size())) return;
    accents[index] = value;
    metronome.setAccentPattern(accents);
    if (metronome.isRunning()) { metronome.stop(); metronome.start(); }
    m_presetManager.savePreset(m_currentPreset);
    m_presetManager.saveToDisk(presetFilePath());
    emit accentsChanged();
}

// ─────────────────────────────────────────────────────────────────────────────
// Invokables — timer / speed / count-in toggles
// ─────────────────────────────────────────────────────────────────────────────
void MetronomeController::toggleTimer()
{
    m_timerEnabled = !m_timerEnabled;
    if (!m_timerEnabled) {
        m_timerTimer->stop();
        m_timerRemaining = m_timerTotalSeconds;
        emit timerRemainingChanged();
    }
    emit timerEnabledChanged();
}

void MetronomeController::toggleSpeed()
{
    m_speedEnabled = !m_speedEnabled;
    if (!m_speedEnabled) {
        resetSpeedTrainer();
        if (metronome.isRunning()) {
            stopAll();
            updateStartStopLabel("Start");
            emit runningChanged();
        }
    }
    notifySectionTableEnabled();
    emit speedEnabledChanged();
}

void MetronomeController::toggleCountIn()
{
    m_countInEnabled = !m_countInEnabled;
    metronome.setCountInEnabled(m_countInEnabled);
    emit countInEnabledChanged();
}

// ─────────────────────────────────────────────────────────────────────────────
// Timer tick
// ─────────────────────────────────────────────────────────────────────────────
void MetronomeController::onTimerTick()
{
    if (m_timerRemaining <= 0) {
        m_timerTimer->stop();
        stopAll();
        updateStartStopLabel("Start");
        m_timerRemaining = m_timerTotalSeconds;
        emit timerRemainingChanged();
        emit runningChanged();

        if (m_speedEnabled && m_currentSectionIdx >= 0 &&
            m_currentSectionIdx < static_cast<int>(m_currentPreset.sections.size()))
        {
            int restoreTempo = m_currentPreset.sections[m_currentSectionIdx].tempo;
            m_tempo = restoreTempo;
            metronome.setTempo(restoreTempo);
            emit tempoChanged();
        }
        return;
    }
    m_timerRemaining--;
    emit timerRemainingChanged();
}

// ─────────────────────────────────────────────────────────────────────────────
// Section management
// ─────────────────────────────────────────────────────────────────────────────
void MetronomeController::addSection()
{
    MetronomeSection s;
    if (m_currentSectionIdx >= 0 &&
        m_currentSectionIdx < static_cast<int>(m_currentPreset.sections.size()))
    {
        s = m_currentPreset.sections[m_currentSectionIdx];
    } else {
        s.tempo       = m_tempo;
        s.numerator   = m_numerator;
        s.denominator = m_denominator;
        s.subdivisionPattern = getDefaultSubdivisionPattern();
        s.hasPolyrhythm = false;
        s.polyrhythm.primaryBeats   = 3;
        s.polyrhythm.secondaryBeats = 2;
        s.polyrhythmPerBeat = true;
        s.accents.resize(m_numerator, false);
    }
    s.label = QString("Section %1").arg(static_cast<int>(m_currentPreset.sections.size()) + 1);

    int insertIdx = (m_currentSectionIdx >= 0) ? m_currentSectionIdx + 1
                                                : static_cast<int>(m_currentPreset.sections.size());
    m_currentPreset.sections.insert(m_currentPreset.sections.begin() + insertIdx, s);

    refreshSectionModel();
    loadSectionToEngine(insertIdx);

    m_presetManager.savePreset(m_currentPreset);
    m_presetManager.saveToDisk(presetFilePath());
}

void MetronomeController::addSectionRange(int targetTempo, int stepSize)
{
    if (m_currentSectionIdx < 0 ||
        m_currentSectionIdx >= static_cast<int>(m_currentPreset.sections.size()) ||
        stepSize <= 0) return;

    MetronomeSection base = m_currentPreset.sections[m_currentSectionIdx];
    int startTempo = base.tempo;
    if (targetTempo == startTempo) return;

    bool goingUp = (targetTempo > startTempo);

    // Build the list of new sections stepping from start toward target
    QVector<MetronomeSection> newSections;
    int t = startTempo + (goingUp ? stepSize : -stepSize);
    int labelBase = static_cast<int>(m_currentPreset.sections.size()) + 1;
    int i = 0;
    while (goingUp ? (t <= targetTempo) : (t >= targetTempo)) {
        MetronomeSection s = base;
        s.tempo = t;
        s.label = QString("Section %1").arg(labelBase + i);
        newSections.append(s);
        t += goingUp ? stepSize : -stepSize;
        ++i;
    }
    if (newSections.isEmpty()) return;

    // When going up, reverse so highest tempo ends up at the top (furthest above the base section)
    if (goingUp) std::reverse(newSections.begin(), newSections.end());

    // Going up → insert above the selected section (ascending ramp leading into it)
    // Going down → insert below the selected section (descending ramp)
    int insertIdx = goingUp ? m_currentSectionIdx : (m_currentSectionIdx + 1);

    auto it = m_currentPreset.sections.begin() + insertIdx;
    for (const auto &s : newSections) {
        it = m_currentPreset.sections.insert(it, s);
        ++it;
    }

    // Keep the originally selected section selected (its index shifted if we inserted above it)
    int newSelectedIdx = goingUp ? (m_currentSectionIdx + newSections.size()) : m_currentSectionIdx;

    refreshSectionModel();
    loadSectionToEngine(newSelectedIdx);

    m_presetManager.savePreset(m_currentPreset);
    m_presetManager.saveToDisk(presetFilePath());
}

void MetronomeController::removeSection()
{
    int row = m_currentSectionIdx;
    if (row < 0 || row >= static_cast<int>(m_currentPreset.sections.size())) return;
    m_currentPreset.sections.erase(m_currentPreset.sections.begin() + row);

    if (m_currentPreset.sections.empty()) {
        // Re-add a default section; reset index so addSection inserts at 0
        m_currentSectionIdx = -1;
        addSection();
        return;
    }

    refreshSectionModel();
    int newIdx = qMin(row, static_cast<int>(m_currentPreset.sections.size()) - 1);
    loadSectionToEngine(newIdx);

    m_presetManager.savePreset(m_currentPreset);
    m_presetManager.saveToDisk(presetFilePath());
}

void MetronomeController::moveSectionUp()
{
    int row = m_currentSectionIdx;
    if (row <= 0 || row >= static_cast<int>(m_currentPreset.sections.size())) return;
    std::swap(m_currentPreset.sections[row - 1], m_currentPreset.sections[row]);
    refreshSectionModel();
    m_currentSectionIdx = row - 1;
    m_sectionModel->setSelectedIndex(m_currentSectionIdx);
    emit currentSectionIndexChanged();
    m_presetManager.savePreset(m_currentPreset);
    m_presetManager.saveToDisk(presetFilePath());
}

void MetronomeController::moveSectionDown()
{
    int row = m_currentSectionIdx;
    if (row < 0 || row + 1 >= static_cast<int>(m_currentPreset.sections.size())) return;
    std::swap(m_currentPreset.sections[row + 1], m_currentPreset.sections[row]);
    refreshSectionModel();
    m_currentSectionIdx = row + 1;
    m_sectionModel->setSelectedIndex(m_currentSectionIdx);
    emit currentSectionIndexChanged();
    m_presetManager.savePreset(m_currentPreset);
    m_presetManager.saveToDisk(presetFilePath());
}

void MetronomeController::selectSection(int index)
{
    if (!sectionTableEnabled()) return;
    if (index < 0 || index >= static_cast<int>(m_currentPreset.sections.size())) return;
    if (index == m_currentSectionIdx) return;

    bool wasRunning = metronome.isRunning();
    if (wasRunning) metronome.stop();

    loadSectionToEngine(index);

    if (wasRunning) {
        m_playingBarCounter = 1;
        m_lastBarIdx        = 0;
        metronome.start();
        const auto& s = m_currentPreset.sections[index];
        updateStartStopLabel(QString("Bar 1/%1\nStop").arg(s.numerator));
        emit runningChanged();
    }
}

QString MetronomeController::sectionLabelAt(int index) const
{
    if (index < 0 || index >= static_cast<int>(m_currentPreset.sections.size()))
        return QString();
    return m_currentPreset.sections[index].label;
}

void MetronomeController::setSectionLabel(int index, const QString& label)
{
    if (index < 0 || index >= static_cast<int>(m_currentPreset.sections.size())) return;
    m_currentPreset.sections[index].label = label;
    m_sectionModel->updateRow(index, m_currentPreset.sections[index], m_currentSectionIdx);
    m_presetManager.savePreset(m_currentPreset);
    m_presetManager.saveToDisk(presetFilePath());
}

// ─────────────────────────────────────────────────────────────────────────────
// Preset management
// ─────────────────────────────────────────────────────────────────────────────
bool MetronomeController::presetNameExists(const QString& name) const
{
    return m_presetManager.listPresetNames().contains(name);
}

void MetronomeController::savePreset(const QString& name)
{
    if (name.trimmed().isEmpty()) return;

    // Start fresh: one default section, no carryover from the previous piece
    m_currentPreset = MetronomePreset();
    m_currentPreset.songName = name;

    MetronomeSection s;
    s.label              = "Section 1";
    s.tempo              = 120;
    s.numerator          = 4;
    s.denominator        = 4;
    s.subdivisionPattern = getDefaultSubdivisionPattern();
    s.hasPolyrhythm      = false;
    s.polyrhythm.primaryBeats   = 3;
    s.polyrhythm.secondaryBeats = 2;
    s.polyrhythmPerBeat = true;
    s.accents.resize(4, false);
    s.accents[0] = true;
    m_currentPreset.sections.push_back(s);

    m_presetManager.savePreset(m_currentPreset);
    m_presetManager.saveToDisk(presetFilePath());

    // Apply the fresh section to the engine
    refreshSectionModel();
    loadSectionToEngine(0);
    emit presetNameChanged();
    emit presetNamesChanged();
    emit currentSectionIndexChanged();
    emit currentSectionChanged();
}

void MetronomeController::loadPreset(const QString& name)
{
    MetronomePreset p;
    if (!m_presetManager.loadPreset(name, p)) return;
    m_currentPreset = p;

    if (m_currentPreset.sections.empty()) {
        MetronomeSection s;
        s.label        = "Section 1";
        s.tempo        = 120;
        s.numerator    = 4;
        s.denominator  = 4;
        s.subdivisionPattern = getDefaultSubdivisionPattern();
        s.hasPolyrhythm = false;
        s.polyrhythm.primaryBeats   = 3;
        s.polyrhythm.secondaryBeats = 2;
        s.polyrhythmPerBeat = true;
        s.accents.resize(4, false);
        m_currentPreset.sections.push_back(s);
    }

    refreshSectionModel();
    loadSectionToEngine(0);
    emit presetNameChanged();
}

void MetronomeController::deletePreset(const QString& name)
{
    m_presetManager.removePreset(name);
    m_presetManager.saveToDisk(presetFilePath());
    emit presetNamesChanged();

    if (!m_presetManager.listPresetNames().isEmpty())
        loadPreset(m_presetManager.listPresetNames().first());
    else {
        m_currentPreset.songName = "";
        m_currentPreset.sections.clear();
        m_currentSectionIdx = -1;
        emit presetNameChanged();
        addSection();
    }
}

void MetronomeController::renamePreset(const QString& oldName, const QString& newName)
{
    if (newName.trimmed().isEmpty() || newName == oldName) return;
    if (m_presetManager.listPresetNames().contains(newName)) return;
    m_currentPreset.songName = newName;
    m_presetManager.removePreset(oldName);
    m_presetManager.savePreset(m_currentPreset);
    m_presetManager.saveToDisk(presetFilePath());
    emit presetNameChanged();
    emit presetNamesChanged();
}

// ─────────────────────────────────────────────────────────────────────────────
// Subdivision picker
// ─────────────────────────────────────────────────────────────────────────────
void MetronomeController::buildPickerPatterns()
{
    bool compound = (m_denominator == 8 && m_numerator % 3 == 0 && m_numerator > 3);
    m_pickerPatterns.resize(4);
    m_pickerPatterns[0] = pickerStandard(compound);
    m_pickerPatterns[1] = pickerComposite(compound);
    m_pickerPatterns[2] = pickerTuplets(compound);
    m_pickerPatterns[3] = loadPickerCustomPatterns();
    m_pickerRevision++;
    emit pickerPatternsChanged();
}

QVector<SubdivisionPattern> MetronomeController::loadPickerCustomPatterns() const
{
    return m_presetManager.customPatterns();
}

int MetronomeController::pickerPatternCount(int cat) const
{
    if (cat < 0 || cat >= m_pickerPatterns.size()) return 0;
    return m_pickerPatterns[cat].size();
}

QString MetronomeController::pickerPatternName(int cat, int idx) const
{
    if (cat < 0 || cat >= m_pickerPatterns.size()) return QString();
    const auto& v = m_pickerPatterns[cat];
    if (idx < 0 || idx >= v.size()) return QString();
    return v[idx].name;
}

void MetronomeController::applyPickerPattern(int cat, int idx)
{
    if (cat < 0 || cat >= m_pickerPatterns.size()) return;
    const auto& v = m_pickerPatterns[cat];
    if (idx < 0 || idx >= v.size()) return;
    applySubdivisionToSection(m_currentSectionIdx, v[idx]);
}

void MetronomeController::preparePickerPattern(int cat, int idx)
{
    if (cat < 0 || cat >= m_pickerPatterns.size()) return;
    const auto& v = m_pickerPatterns[cat];
    if (idx < 0 || idx >= v.size()) return;
    m_stagedPattern = v[idx];
    m_stagedRevision++;
    emit stagedPatternChanged();
}

int MetronomeController::stagedPulseCount() const
{
    return m_stagedPattern.pulses.size();
}

bool MetronomeController::stagedPulseIsRest(int i) const
{
    if (i < 0 || i >= m_stagedPattern.pulses.size()) return false;
    return m_stagedPattern.pulses[i].isRest;
}

void MetronomeController::toggleStagedPulseRest(int i)
{
    if (i < 0 || i >= m_stagedPattern.pulses.size()) return;
    m_stagedPattern.pulses[i].isRest = !m_stagedPattern.pulses[i].isRest;
    m_stagedRevision++;
    emit stagedPatternChanged();
}

void MetronomeController::commitStagedPattern()
{
    applySubdivisionToSection(m_currentSectionIdx, m_stagedPattern);
}

QPixmap MetronomeController::stagedPatternPixmap(const QSize& size) const
{
    NoteAssembler assembler;
    NoteAssemblerConfig cfg = buildNoteAssemblerConfig(m_stagedPattern);
    cfg.centerVertically = true;
    cfg.pixmapSize = size;
    return assembler.assembleNote(cfg);
}

QPixmap MetronomeController::pickerPatternPixmap(int cat, int idx, const QSize& size) const
{
    if (cat < 0 || cat >= m_pickerPatterns.size()) return QPixmap(size);
    const auto& v = m_pickerPatterns[cat];
    if (idx < 0 || idx >= v.size()) return QPixmap(size);
    NoteAssembler assembler;
    NoteAssemblerConfig cfg = buildNoteAssemblerConfig(v[idx]);
    cfg.centerVertically = true;
    cfg.pixmapSize = size;
    return assembler.assembleNote(cfg);
}

// ─────────────────────────────────────────────────────────────────────────────
// Settings dialog apply
// ─────────────────────────────────────────────────────────────────────────────
void MetronomeController::applySettings(const QString& soundSet, const QColor& accentColor,
                                        bool alwaysOnTop, bool beatWindowAuto,
                                        const QString& terminology)
{
    bool soundChanged = (soundSet != m_soundSet);

    m_soundSet       = soundSet;
    m_accentColor    = accentColor;
    m_alwaysOnTop    = alwaysOnTop;
    m_beatWindowAuto = beatWindowAuto;
    m_terminology    = terminology;

    if (soundChanged) {
        metronome.loadSample("accent", soundFileForSet(m_soundSet, true));
        metronome.loadSample("click",  soundFileForSet(m_soundSet, false));
        metronome.setAccentSound("accent");
        metronome.setClickSound("click");
    }

    saveSettings();
    emit accentColorChanged();
    emit soundSetChanged();
    emit alwaysOnTopChanged();
    emit beatWindowAutoChanged();
    emit terminologyChanged();
}

void MetronomeController::setBeatWindowStyleForMode(bool polyrhythm, int style)
{
    if (polyrhythm) {
        if (style != 0 && style != 5)
            style = 5;
        if (m_beatWindowPolyrhythmStyle == style)
            return;
        m_beatWindowPolyrhythmStyle = style;
    } else {
        style = qBound(0, style, 4);
        if (m_beatWindowSubdivisionStyle == style)
            return;
        m_beatWindowSubdivisionStyle = style;
    }

    saveSettings();
    emit beatWindowStyleChanged();
}

void MetronomeController::checkForUpdates()
{
    UpdateChecker::check(nullptr, false);
}

// ─────────────────────────────────────────────────────────────────────────────
// Called when the audio thread's speed trainer steps up the tempo.
// We just update display state here — no audio restarts needed.
void MetronomeController::onTempoSteppedUp(int newTempo)
{
    // Tempo update only — bar counter and count-in transitions are handled
    // in onMetronomePulse when ev.newTempo > 0 arrives in sync with audio.
    m_speedTrainerCurrentTempo = newTempo;
    m_tempo                    = newTempo;
    emit tempoChanged();
}

// Metronome pulse handler — port of MainWindow::onMetronomePulse
// ─────────────────────────────────────────────────────────────────────────────
void MetronomeController::onMetronomePulse(AudioPulseEvent ev)
{
    if (m_currentSectionIdx < 0 ||
        m_currentSectionIdx >= static_cast<int>(m_currentPreset.sections.size()))
        return;

    const MetronomeSection& section = m_currentPreset.sections[m_currentSectionIdx];
    const SubdivisionPattern& currentPattern = metronome.subdivisionPattern();

    // ---- SPEED-TRAINER STEP-UP: fires in sync with the first pulse of the new tempo ----
    if (ev.newTempo > 0 && m_speedEnabled) {
        if (ev.idx < 0) {
            // Step-up triggered a count-in bar; enter count-in display state now
            m_speedTrainerCountingIn = true;
            notifySectionTableEnabled();
            emit runningChanged();
        } else {
            // No count-in: reset bar tracking immediately
            m_speedTrainerTotalBarCounter = 0;
        }
    }

    // ---- COUNT-IN HANDLING ----
    if (ev.idx < 0) {
        int countInNumber = ev.idx + 1001;
        int countInBeats  = m_numerator;
        if (m_denominator == 8 && m_numerator % 3 == 0 && m_numerator > 3)
            countInBeats = m_numerator / 3;

        updateStartStopLabel(
            QString("Count %1/%2").arg(countInNumber).arg(countInBeats));

        int bpb = section.numerator;
        if (section.denominator == 8 && section.numerator % 3 == 0 && section.numerator > 3)
            bpb = section.numerator / 3;
        updateBeatIndicator(bpb, 1, countInNumber - 1, 0, 0);
        triggerObsPulse();
        return;
    }

    // Transition from count-in to main
    if (m_speedTrainerCountingIn) {
        m_speedTrainerCountingIn = false;
        m_speedTrainerTotalBarCounter = 0;
        notifySectionTableEnabled();
        emit runningChanged();
    }

    // Subdivision calculations
    int subdivisionsPerBeat = qMax(1, static_cast<int>(currentPattern.pulses.size()));
    bool isCustomPat = (currentPattern.category == SubdivisionCategory::Custom);

    int bpb = section.numerator;
    if (section.denominator == 8 && section.numerator % 3 == 0 && section.numerator > 3)
        bpb = section.numerator / 3;

    int currBeat, currSub, effectiveSubs;
    if (isCustomPat) {
        // Each audio "bar" in the custom path = one full pattern playthrough.
        // ev.barNumber increments once per playthrough, so wrap it by bpb to
        // get which big circle is active. ev.idx steps through the small circles.
        currBeat      = ev.barNumber % qMax(1, bpb);
        currSub       = ev.idx;
        effectiveSubs = subdivisionsPerBeat;
    } else {
        currBeat      = ev.idx / subdivisionsPerBeat;
        currSub       = ev.idx % subdivisionsPerBeat;
        effectiveSubs = subdivisionsPerBeat;
    }

    // ---- POLYRHYTHM ----
    if (section.hasPolyrhythm && metronome.isPolyrhythmEnabled()) {
        Polyrhythm poly     = metronome.getPolyrhythm();
        int mainBeats       = poly.primaryBeats;
        int polyBeats       = poly.secondaryBeats;

        if (ev.startOfCycle && !m_polyrhythmCycleActive) {
            m_polyrhythmCycleActive = true;
            if (m_polyrhythmJustRestarted) {
                m_polyrhythmJustRestarted = false;
                m_playingBarCounter = 1;
            } else if (m_speedEnabled) {
                // Bar counter: driven by ev.barNumber which resets on each step-up
                m_playingBarCounter = (ev.barNumber % qMax(1, m_speedBarsPerStep)) + 1;
            } else {
                if (m_playingBarCounter < section.numerator) m_playingBarCounter++;
                else m_playingBarCounter = 1;
            }
        }
        if (!ev.startOfCycle) m_polyrhythmCycleActive = false;

        if (m_speedEnabled)
            updateStartStopLabel(QString("Bar %1/%2\nStop")
                .arg(qMax(1, m_playingBarCounter)).arg(m_speedBarsPerStep));
        else
            updateStartStopLabel(QString("Bar %1/%2\nStop")
                .arg(qMax(1, m_playingBarCounter)).arg(section.numerator));

        updateBeatIndicator(mainBeats, polyBeats, 0, 0, 1, ev.gridColumn, mainBeats, polyBeats);
        triggerObsPulse();
        return;
    }

    // ---- SUBDIVISION ----
    // For custom patterns ev.barNumber counts playthroughs; bpb playthroughs = 1 real bar.
    int realBarNumber = isCustomPat ? (ev.barNumber / qMax(1, bpb)) : ev.barNumber;
    bool isFirstRealBar = isCustomPat ? (ev.isFirstInBar && (ev.barNumber % qMax(1, bpb) == 0))
                                      : ev.isFirstInBar;
    if (isFirstRealBar) {
        if (m_speedEnabled) {
            m_playingBarCounter = (realBarNumber % qMax(1, m_speedBarsPerStep)) + 1;
            updateStartStopLabel(QString("Bar %1/%2\nStop")
                .arg(m_playingBarCounter).arg(m_speedBarsPerStep));
        } else {
            int displayBar = (realBarNumber % section.numerator) + 1;
            m_playingBarCounter = displayBar;
            updateStartStopLabel(QString("Bar %1/%2\nStop")
                .arg(displayBar).arg(section.numerator));
        }
    }

    updateBeatIndicator(bpb, effectiveSubs, currBeat, currSub, 0);
    triggerObsPulse();
}

// ── Backup / restore ─────────────────────────────────────────────────────────

bool MetronomeController::exportPresetsToFile(const QStringList& names, const QString& filePath)
{
    return m_presetManager.exportToFile(names, filePath);
}

QStringList MetronomeController::presetsInFile(const QString& filePath) const
{
    return PresetManager::presetsInFile(filePath);
}

int MetronomeController::customPatternsInFile(const QString& filePath) const
{
    return PresetManager::customPatternsInFile(filePath);
}

bool MetronomeController::importPresetsFromFile(const QStringList& names, const QString& filePath)
{
    if (!m_presetManager.importFromFile(names, filePath)) return false;
    m_presetManager.saveToDisk(presetFilePath());
    emit presetNamesChanged();
    buildPickerPatterns();
    return true;
}

void MetronomeController::openNewCustomPattern() {
    bool compound = m_denominator == 8 && m_numerator % 3 == 0;
    m_patternEditor->beginNew(m_numerator, m_denominator, compound);
    m_editingPatternIdx = -1;
    emit customEditorReady();
}

void MetronomeController::openEditCustomPattern(int idx) {
    auto patterns = loadPickerCustomPatterns();
    if (idx < 0 || idx >= patterns.size()) return;
    bool compound = m_denominator == 8 && m_numerator % 3 == 0;
    m_editingPatternOriginalName = patterns[idx].name;
    m_patternEditor->beginEdit(patterns[idx], m_numerator, m_denominator, compound);
    m_editingPatternIdx = idx;
    emit customEditorReady();
}

void MetronomeController::deleteCustomPattern(int idx) {
    auto patterns = loadPickerCustomPatterns();
    if (idx < 0 || idx >= patterns.size()) return;
    patterns.removeAt(idx);
    m_presetManager.setCustomPatterns(patterns);
    m_presetManager.saveToDisk(presetFilePath());
    buildPickerPatterns();
}

bool MetronomeController::customPatternNameExists(const QString& name) const
{
    auto patterns = m_presetManager.customPatterns();
    for (int i = 0; i < patterns.size(); i++) {
        if (i == m_editingPatternIdx) continue; // skip the one being edited
        if (patterns[i].name == name) return true;
    }
    return false;
}

void MetronomeController::commitCustomPattern() {
    SubdivisionPattern p = m_patternEditor->currentPattern();
    if (p.pulses.isEmpty()) return;
    if (p.name.isEmpty())
        p.name = QString("Custom %1").arg(m_pickerPatterns[3].size() + 1);

    auto patterns = loadPickerCustomPatterns();
    if (m_editingPatternIdx >= 0 && m_editingPatternIdx < patterns.size())
        patterns[m_editingPatternIdx] = p;
    else
        patterns.append(p);

    m_presetManager.setCustomPatterns(patterns);
    m_presetManager.saveToDisk(presetFilePath());
    buildPickerPatterns();

    // Update any section in the current preset that was using the edited pattern
    if (m_editingPatternIdx >= 0) {
        bool anyUpdated = false;
        for (auto& s : m_currentPreset.sections) {
            if (s.subdivisionPattern.category == SubdivisionCategory::Custom
                && s.subdivisionPattern.name == m_editingPatternOriginalName) {
                s.subdivisionPattern = p;
                anyUpdated = true;
            }
        }
        if (anyUpdated) {
            // Re-push the current section's updated pattern to the engine
            if (m_currentSectionIdx >= 0 &&
                m_currentSectionIdx < static_cast<int>(m_currentPreset.sections.size())) {
                metronome.setSubdivisionPattern(
                    m_currentPreset.sections[m_currentSectionIdx].subdivisionPattern);
            }
            m_presetManager.savePreset(m_currentPreset);
            m_presetManager.saveToDisk(presetFilePath());
            emit subdivisionChanged();
        }
    }
}
