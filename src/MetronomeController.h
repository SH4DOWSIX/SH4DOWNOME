#pragma once

#include <QObject>
#include <QColor>
#include <QTimer>
#include <QDateTime>
#include <QSettings>
#include <QVariant>
#include "metronomeengine.h"
#include "presetmanager.h"
#include "subdivisionpattern.h"
#include "noteassembler.h"
#include "audioengine.h"
#include "CustomPatternEditor.h"

class SectionListModel;

class MetronomeController : public QObject {
    Q_OBJECT

    // Running / start-stop
    Q_PROPERTY(bool running READ running NOTIFY runningChanged)
    Q_PROPERTY(QString startStopLabel READ startStopLabel NOTIFY startStopLabelChanged)

    // Tempo
    Q_PROPERTY(int tempo READ tempo WRITE setTempo NOTIFY tempoChanged)
    Q_PROPERTY(QString tempoMarkings READ tempoMarkings NOTIFY tempoChanged)

    // Time signature
    Q_PROPERTY(int numerator READ numerator NOTIFY timeSignatureChanged)
    Q_PROPERTY(int denominator READ denominator NOTIFY timeSignatureChanged)

    // Volume
    Q_PROPERTY(int volume READ volume WRITE setVolume NOTIFY volumeChanged)

    // Timer
    Q_PROPERTY(bool timerEnabled READ timerEnabled NOTIFY timerEnabledChanged)
    Q_PROPERTY(int timerTotalSeconds READ timerTotalSeconds WRITE setTimerTotalSeconds NOTIFY timerTotalSecondsChanged)
    Q_PROPERTY(int timerRemaining READ timerRemaining NOTIFY timerRemainingChanged)
    Q_PROPERTY(QString timerRemainingString READ timerRemainingString NOTIFY timerRemainingChanged)

    // Speed trainer
    Q_PROPERTY(bool speedEnabled READ speedEnabled NOTIFY speedEnabledChanged)
    Q_PROPERTY(int speedBarsPerStep READ speedBarsPerStep WRITE setSpeedBarsPerStep NOTIFY speedSettingsChanged)
    Q_PROPERTY(int speedTempoStep READ speedTempoStep WRITE setSpeedTempoStep NOTIFY speedSettingsChanged)
    Q_PROPERTY(int speedMaxTempo READ speedMaxTempo WRITE setSpeedMaxTempo NOTIFY speedSettingsChanged)

    // Count-in
    Q_PROPERTY(bool countInEnabled READ countInEnabled NOTIFY countInEnabledChanged)

    // Polyrhythm
    Q_PROPERTY(bool polyrhythmEnabled READ polyrhythmEnabled NOTIFY currentSectionChanged)
    Q_PROPERTY(int polyPrimaryBeats READ polyPrimaryBeats NOTIFY currentSectionChanged)
    Q_PROPERTY(int polySecondaryBeats READ polySecondaryBeats NOTIFY currentSectionChanged)
    Q_PROPERTY(bool polyrhythmPerBeat READ polyrhythmPerBeat NOTIFY currentSectionChanged)

    // Accents
    Q_PROPERTY(QVariantList accents READ accents NOTIFY accentsChanged)
    Q_PROPERTY(bool showAccents READ showAccents NOTIFY showAccentsChanged)

    // Subdivision revision counter – increments on subdivision change so QML Image refreshes
    Q_PROPERTY(int subdivisionRevision READ subdivisionRevision NOTIFY subdivisionChanged)
    Q_PROPERTY(QString subdivisionName READ subdivisionName NOTIFY subdivisionChanged)
    Q_PROPERTY(bool currentSubdivisionIsCustom READ currentSubdivisionIsCustom NOTIFY subdivisionChanged)
    Q_PROPERTY(QVariantList currentSubdivisionPulses READ currentSubdivisionPulses NOTIFY subdivisionChanged)

    // Custom pattern editor
    Q_PROPERTY(CustomPatternEditor* patternEditor READ patternEditor CONSTANT)

    // Subdivision picker (bottom sheet)
    Q_PROPERTY(int pickerRevision READ pickerRevision NOTIFY pickerPatternsChanged)

    // Staged subdivision pattern (rest editor)
    Q_PROPERTY(int     stagedRevision    READ stagedRevision    NOTIFY stagedPatternChanged)
    Q_PROPERTY(QString stagedPatternName READ stagedPatternName NOTIFY stagedPatternChanged)

    // Section list model
    Q_PROPERTY(QObject* sectionModel READ sectionModel CONSTANT)
    Q_PROPERTY(int currentSectionIndex READ currentSectionIndex NOTIFY currentSectionIndexChanged)
    Q_PROPERTY(bool sectionTableEnabled READ sectionTableEnabled NOTIFY sectionTableEnabledChanged)

    // Beat indicator (live pulse state)
    Q_PROPERTY(int biBeats READ biBeats NOTIFY beatIndicatorChanged)
    Q_PROPERTY(int biSubdivisions READ biSubdivisions NOTIFY beatIndicatorChanged)
    Q_PROPERTY(int biCurrentBeat READ biCurrentBeat NOTIFY beatIndicatorChanged)
    Q_PROPERTY(int biCurrentSub READ biCurrentSub NOTIFY beatIndicatorChanged)
    Q_PROPERTY(int biMode READ biMode NOTIFY beatIndicatorChanged)        // 0=circles 1=grid
    Q_PROPERTY(int biGridHighlight READ biGridHighlight NOTIFY beatIndicatorChanged)
    Q_PROPERTY(int biPolyMain READ biPolyMain NOTIFY beatIndicatorChanged)
    Q_PROPERTY(int biPolySecondary READ biPolySecondary NOTIFY beatIndicatorChanged)

    // Accent colour (used by beat indicator and buttons)
    Q_PROPERTY(QColor accentColor READ accentColor NOTIFY accentColorChanged)

    // OBS pulse flash
    Q_PROPERTY(bool obsPulse READ obsPulse NOTIFY obsPulseChanged)

    // Preset management
    Q_PROPERTY(QString presetName READ presetName NOTIFY presetNameChanged)
    Q_PROPERTY(QStringList presetNames READ presetNames NOTIFY presetNamesChanged)

    // Persistent settings
    Q_PROPERTY(QString soundSet    READ soundSet    NOTIFY soundSetChanged)
    Q_PROPERTY(bool obsEnabled     READ obsEnabled  NOTIFY obsEnabledChanged)
    Q_PROPERTY(bool alwaysOnTop    READ alwaysOnTop NOTIFY alwaysOnTopChanged)
    Q_PROPERTY(bool beatWindowAuto READ beatWindowAuto NOTIFY beatWindowAutoChanged)
    Q_PROPERTY(int beatWindowSubdivisionStyle READ beatWindowSubdivisionStyle NOTIFY beatWindowStyleChanged)
    Q_PROPERTY(int beatWindowPolyrhythmStyle READ beatWindowPolyrhythmStyle NOTIFY beatWindowStyleChanged)
    Q_PROPERTY(QString terminology READ terminology NOTIFY terminologyChanged)

public:
    explicit MetronomeController(QObject* parent = nullptr);
    ~MetronomeController();

    // ---- Getters ----
    bool running() const;
    QString startStopLabel() const { return m_startStopLabel; }
    int tempo() const { return m_tempo; }
    QString tempoMarkings() const;
    int numerator() const { return m_numerator; }
    int denominator() const { return m_denominator; }
    int volume() const { return m_volume; }
    bool timerEnabled() const { return m_timerEnabled; }
    int timerTotalSeconds() const { return m_timerTotalSeconds; }
    int timerRemaining() const { return m_timerRemaining; }
    QString timerRemainingString() const;
    bool speedEnabled() const { return m_speedEnabled; }
    int speedBarsPerStep() const { return m_speedBarsPerStep; }
    int speedTempoStep() const { return m_speedTempoStep; }
    int speedMaxTempo() const { return m_speedMaxTempo; }
    bool countInEnabled() const { return m_countInEnabled; }
    bool polyrhythmEnabled() const;
    int polyPrimaryBeats() const;
    int polySecondaryBeats() const;
    bool polyrhythmPerBeat() const;
    QVariantList accents() const;
    bool showAccents() const;
    int subdivisionRevision() const { return m_subdivisionRevision; }
    QString subdivisionName() const;
    bool currentSubdivisionIsCustom() const;
    QVariantList currentSubdivisionPulses() const;
    int pickerRevision() const { return m_pickerRevision; }
    int stagedRevision() const { return m_stagedRevision; }
    QString stagedPatternName() const { return m_stagedPattern.name; }
    CustomPatternEditor* patternEditor() const { return m_patternEditor; }
    Q_INVOKABLE int pickerPatternCount(int cat) const;
    Q_INVOKABLE QString pickerPatternName(int cat, int idx) const;
    Q_INVOKABLE void applyPickerPattern(int cat, int idx);
    Q_INVOKABLE bool isCurrentPickerPattern(int cat, int idx) const;
    Q_INVOKABLE void preparePickerPattern(int cat, int idx);
    Q_INVOKABLE int  stagedPulseCount() const;
    Q_INVOKABLE bool stagedPulseIsRest(int i) const;
    Q_INVOKABLE void toggleStagedPulseRest(int i);
    Q_INVOKABLE void commitStagedPattern();
    QObject* sectionModel() const;
    int currentSectionIndex() const { return m_currentSectionIdx; }
    bool sectionTableEnabled() const;
    int biBeats() const { return m_biBeats; }
    int biSubdivisions() const { return m_biSubdivisions; }
    int biCurrentBeat() const { return m_biCurrentBeat; }
    int biCurrentSub() const { return m_biCurrentSub; }
    int biMode() const { return m_biMode; }
    int biGridHighlight() const { return m_biGridHighlight; }
    int biPolyMain() const { return m_biPolyMain; }
    int biPolySecondary() const { return m_biPolySecondary; }
    QColor accentColor() const { return m_accentColor; }
    bool obsPulse() const { return m_obsPulse; }
    QString presetName() const { return m_currentPreset.songName; }
    QStringList presetNames() const { return m_presetManager.listPresetNames(); }
    QString soundSet()    const { return m_soundSet; }
    bool obsEnabled()     const { return !m_obsHidden; }
    bool alwaysOnTop()    const { return m_alwaysOnTop; }
    bool beatWindowAuto() const { return m_beatWindowAuto; }
    int beatWindowSubdivisionStyle() const { return m_beatWindowSubdivisionStyle; }
    int beatWindowPolyrhythmStyle() const { return m_beatWindowPolyrhythmStyle; }
    QString terminology() const { return m_terminology; }

    // ---- Setters (Q_PROPERTY write) ----
    void setTempo(int tempo);
    void setVolume(int volume);
    void setTimerTotalSeconds(int seconds);
    void setSpeedBarsPerStep(int v);
    void setSpeedTempoStep(int v);
    void setSpeedMaxTempo(int v);

    // ---- Invokables ----
    Q_INVOKABLE void startStop();
    Q_INVOKABLE void tapTempo();
    Q_INVOKABLE void requestTimeSignature(int num, int den);
    Q_INVOKABLE void openSubdivisionPicker();
    Q_INVOKABLE void togglePolyrhythm();
    Q_INVOKABLE void setPolyrhythm(int primary, int secondary, bool perBeat);
    Q_INVOKABLE void setAccent(int index, bool value);
    Q_INVOKABLE void toggleTimer();
    Q_INVOKABLE void toggleSpeed();
    Q_INVOKABLE void toggleCountIn();
    Q_INVOKABLE void addSection();
    Q_INVOKABLE void addSectionRange(int targetTempo, int stepSize);
    Q_INVOKABLE void removeSection();
    Q_INVOKABLE void moveSectionUp();
    Q_INVOKABLE void moveSectionDown();
    Q_INVOKABLE void selectSection(int index);
    Q_INVOKABLE void savePreset(const QString& name);
    Q_INVOKABLE void loadPreset(const QString& name);
    Q_INVOKABLE void deletePreset(const QString& name);
    Q_INVOKABLE void renamePreset(const QString& oldName, const QString& newName);
    Q_INVOKABLE void applySettings(const QString& soundSet, const QColor& accentColor,
                                   bool alwaysOnTop, bool beatWindowAuto,
                                   const QString& terminology);
    Q_INVOKABLE void checkForUpdates();
    Q_INVOKABLE void setBeatWindowStyleForMode(bool polyrhythm, int style);
    Q_INVOKABLE QString sectionLabelAt(int index) const;
    Q_INVOKABLE void setSectionLabel(int index, const QString& label);
    Q_INVOKABLE bool presetNameExists(const QString& name) const;

    // Custom subdivision management
    Q_INVOKABLE void openNewCustomPattern();
    Q_INVOKABLE void openEditCustomPattern(int idx);
    Q_INVOKABLE void deleteCustomPattern(int idx);
    Q_INVOKABLE void commitCustomPattern();
    Q_INVOKABLE bool customPatternNameExists(const QString& name) const;

    // Backup / restore
    Q_INVOKABLE bool exportPresetsToFile(const QStringList& names, const QString& filePath);
    Q_INVOKABLE QStringList presetsInFile(const QString& filePath) const;
    Q_INVOKABLE int customPatternsInFile(const QString& filePath) const;
    Q_INVOKABLE bool importPresetsFromFile(const QStringList& names, const QString& filePath);

    // ---- Accessed by NoteImageProvider ----
    QPixmap currentSubdivisionPixmap(const QSize& size) const;
    QPixmap sectionSubdivisionPixmap(int sectionIdx, const QSize& size) const;
    QPixmap pickerPatternPixmap(int cat, int idx, const QSize& size) const;
    QPixmap stagedPatternPixmap(const QSize& size) const;

signals:
    void runningChanged();
    void startStopLabelChanged();
    void tempoChanged();
    void timeSignatureChanged();
    void volumeChanged();
    void timerEnabledChanged();
    void timerTotalSecondsChanged();
    void timerRemainingChanged();
    void speedEnabledChanged();
    void speedSettingsChanged();
    void countInEnabledChanged();
    void currentSectionChanged();
    void accentsChanged();
    void showAccentsChanged();
    void subdivisionChanged();
    void pickerPatternsChanged();
    void stagedPatternChanged();
    void currentSectionIndexChanged();
    void sectionTableEnabledChanged();
    void beatIndicatorChanged();
    void accentColorChanged();
    void obsPulseChanged();
    void presetNameChanged();
    void presetNamesChanged();
    void soundSetChanged();
    void obsEnabledChanged();
    void alwaysOnTopChanged();
    void beatWindowAutoChanged();
    void beatWindowStyleChanged();
    void terminologyChanged();
    void customEditorReady();

private slots:
    void onMetronomePulse(AudioPulseEvent ev);
    void onTempoSteppedUp(int newTempo);
    void onTimerTick();
    void onObsPulseReset();

private:
    MetronomeEngine metronome;
    PresetManager m_presetManager;
    MetronomePreset m_currentPreset;
    int m_currentSectionIdx = -1;
    SectionListModel* m_sectionModel = nullptr;

    // Persistent settings
    QString m_soundSet    = "Default";
    QColor  m_accentColor = QColor(150, 0, 0);
    bool    m_obsHidden   = true;
    bool    m_alwaysOnTop = false;
    bool    m_beatWindowAuto = false;
    int     m_beatWindowSubdivisionStyle = 0;
    int     m_beatWindowPolyrhythmStyle = 5;
    QString m_terminology = "Piece";

    // Tempo / time signature
    int m_tempo = 120;
    int m_numerator = 4;
    int m_denominator = 4;
    int m_volume = 90;

    // Start/stop label
    QString m_startStopLabel = "Start";

    // Timer
    bool m_timerEnabled = false;
    int m_timerTotalSeconds = 0;
    int m_timerRemaining = 0;
    QTimer* m_timerTimer = nullptr;

    // Speed trainer
    bool m_speedEnabled = false;
    int m_speedBarsPerStep = 4;
    int m_speedTempoStep = 2;
    int m_speedMaxTempo = 180;
    int m_speedTrainerStartTempo = 120;
    int m_speedTrainerCurrentTempo = 120;
    int m_speedTrainerTotalBarCounter = 0;
    bool m_speedTrainerCountingIn = false;
    bool m_speedTrainerPolyFirstCycle = true;

    // Count-in
    bool m_countInEnabled = false;

    // Play state tracking
    int m_playingBarCounter = 1;
    int m_lastBarIdx = -1;
    bool m_polyrhythmCycleActive = false;
    bool m_polyrhythmJustRestarted = false;
    int m_polyBarCount = 1;

    // Beat indicator forwarded values
    int m_biBeats = 4;
    int m_biSubdivisions = 1;
    int m_biCurrentBeat = 0;
    int m_biCurrentSub = 0;
    int m_biMode = 0;       // 0=circles, 1=grid
    int m_biGridHighlight = -1;
    int m_biPolyMain = 3;
    int m_biPolySecondary = 2;

    // OBS pulse flash
    bool m_obsPulse = false;
    QTimer* m_obsPulseTimer = nullptr;

    // Custom pattern editor
    CustomPatternEditor* m_patternEditor = nullptr;
    int     m_editingPatternIdx = -1;
    QString m_editingPatternOriginalName;

    // Subdivision revision counter
    int m_subdivisionRevision = 0;

    // Subdivision picker data
    QVector<QVector<SubdivisionPattern>> m_pickerPatterns;
    int m_pickerRevision = 0;
    SubdivisionPattern m_stagedPattern;
    int m_stagedRevision = 0;
    void buildPickerPatterns();
    QVector<SubdivisionPattern> loadPickerCustomPatterns() const;

    // Tap tempo
    QList<qint64> m_tapTimes;
    QTimer* m_tapTempoResumeTimer = nullptr;
    bool m_metronomeWasRunning = false;

    // Internal helpers
    QString settingsPath() const;
    QString presetFilePath() const;
    void loadSettings();
    void saveSettings();
    void loadSectionToEngine(int idx);
    void refreshSectionModel();
    void updateStartStopLabel(const QString& label);
    void updateBeatIndicator(int beats, int subs, int beat, int sub,
                             int mode, int gridHighlight = -1,
                             int polyMain = 0, int polySec = 0);
    void notifySectionTableEnabled();
    void triggerObsPulse();
    QString soundFileForSet(const QString& set, bool accent) const;
    SubdivisionPattern getDefaultSubdivisionPattern() const;
    static QString getTempoMarkings(int bpm);
    void resetSpeedTrainer();
    void startCountIn();
    void stopAll();
    void applySubdivisionToSection(int idx, const SubdivisionPattern& pattern);
    void setupTimeSignature(int num, int den);
};
