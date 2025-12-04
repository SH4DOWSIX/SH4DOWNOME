#pragma once

#include <QMainWindow>
#include <QSoundEffect>
#include <QCheckBox>
#include <QElapsedTimer>
#include "metronomeengine.h"
#include "presetmanager.h"
#include <QTimer>
#include "beatindicatorwidget.h"
#include "obsbeatwidget.h"
#include <QLabel>
#include "noteassembler.h"
#include <QPushButton>
#include <QTime>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class OBSBeatWindow;
class QWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void updateButtonColors();
    void resetBarCounterAndLabel(const MetronomeSection& s);

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onStartStop();
    void onTempoChanged(int value);
    void onAccentChanged();
    void onMetronomePulse(AudioPulseEvent ev);

    void onSavePreset();
    void onLoadPreset();
    void refreshPresetList();
    void onDeletePreset();
    void onRenamePreset();

    void onSectionSelected(int row, int col);
    void onAddSection();
    void onRemoveSection();
    void onSaveSection();
    void onMoveSectionUp();
    void onMoveSectionDown();
    void onTapTempo();
    void onTempoSliderChanged(int value);

    void onSpeedToggle();

    void onSubdivisionImageClicked();

    void onSectionRowMoved(int from, int to);

    void onSettingsClicked();

    void onPolyrhythmClicked();

    // --- ADDED FOR CTRL+UP/DOWN SUPPORT ---
    void onMoveSectionViaShortcut(int fromRow, int toRow);

    // OBS popout window close handler
    void onObsWindowAboutToClose();

private:
    Ui::MainWindow *ui;
    MetronomeEngine metronome;
    QSoundEffect accentSound, clickSound;
    QList<QCheckBox*> accentChecks;
    BeatIndicatorWidget* m_beatIndicatorWidget = nullptr;
    int m_speedTrainerTotalBarCounter = 0;

    int m_countInCurrentBeat = 0;
    int m_countInBeatsRemaining = 0;
    QTimer* m_countInTimer = nullptr;

    bool m_switchPolyrhythmAfterCountIn = false;
    bool m_pendingStartPoly = false;
    bool m_pendingStartSubdiv = false;
    bool m_pendingPolyrhythmStart = false;
    bool m_countInEnabled = false;
    int m_lastPolyrhythmCycleIdx = -1;
    bool m_polyrhythmCycleActive = false;
    int m_speedTrainerPendingTempo = -1;
    int m_lastBarIdx = -1;
    bool m_pendingTempoApplied = false;

    int m_polyModeDelayMs = 225;
    bool m_pendingBarLabelUpdate = false;
    bool m_isSpeedTrainerAutoChange = false;
    bool m_polyrhythmJustRestarted = false;

    int m_polyModeBarsDelay = -1;
    int m_polyModeBarsElapsed = 0;
    bool m_armPolyModeAfterBars = false;
    bool m_armPolyModeNextBar = false;
    int m_playingBarCounter = 1;
    int m_polyBarCount = 1;
    int m_lastPolyBarIncrementIdx = -1;
    int m_lastPolyBarIncrementCycle = -1;
    int m_polyPulseInCycle = 0;

    bool m_switchSubdivisionAfterCountIn = false;
    int m_nextSubdivisionIndex = -1;
    bool m_speedTrainerPolyFirstCycle = true;

    SubdivisionPattern m_nextSubdivisionPattern;

    int m_polyrhythmGridStep = 0;
    int m_speedTrainerBarInCycle = 0;


    NoteAssemblerConfig configForPattern(const SubdivisionPattern& pattern) const;
    QPixmap assembleSubdivisionPixmapForOBS(const SubdivisionPattern& pattern) const;


    void onCountInTick();
    void startMainMetronomeAfterCountIn();
    void startCountIn();
    

    int currentNumerator = 4;
    int currentDenominator = 4;
    bool m_pendingPolyTempoStep = false;
    bool m_pendingSpeedTrainerStep = false;

    QTime m_lastEnteredTimerValue;
    bool m_timerWasRunning = false;

    void updateSubPolyCell(int sectionIdx);
    bool m_metronomeWasRunning = false;
    void onTempoSliderPressed();
    void onTempoSliderReleased();
    void onSpinTempoEditingFinished();
    QTimer* m_tapTempoResumeTimer = nullptr;

    void setTimeSignature(int numerator, int denominator);
    void updateTimeSignatureDisplay();
    QString soundFileForSet(const QString& set, bool accent) const;

    void setupAccentControls(int count);
    void loadSectionToUI(int idx);
    void saveUIToSection(int idx, bool doAutosave);
    void saveUIToSection(int idx);
    QTimer* timer = nullptr;
    int timerSecondsRemaining = 0;
    void setSpeedTrainerUIEnabled(bool enabled);

    int m_countInPulseIdx = 0;

    void updatePolyrhythmButtonColor();

    bool m_timerEnabled = false;
    bool m_speedEnabled = false;
    
    void updateSpeedUI();

    QString m_soundSet = "Default";
    QColor m_accentColor = QColor(150,0,0);
    bool m_obsHidden = false;
    bool m_obsInLayout = true;

    // Popout support: top-level window that hosts the OBSBeatWidget when enabled
    OBSBeatWindow* m_obsWindow = nullptr;
    QWidget* m_obsHiddenHost = nullptr; // hidden host when OBS is turned off

    // Helper methods to open/close popout
    void openObsPopoutWindow();
    void closeObsPopoutWindow();

    QLabel* m_labelPolyrhythmNumerator = nullptr;
    QLabel* m_labelPolyrhythmDenominator = nullptr;
    QWidget* m_polyrhythmNumberWidget = nullptr;
    void updatePolyrhythmUI();
    void updateObsWidgetPolyrhythmDisplay();
    void showPolyrhythmNumberDialog();
    void onPolyrhythmNumberClicked();

    bool m_alwaysOnTop = false;

    void onTimerToggle();
    void updateTimerUI();

    bool m_speedTrainerEnabled = false;
    bool m_speedTrainerCountingIn = false;
    int m_speedTrainerCountInBeats = 0;
    int m_speedTrainerBarCounter = 0;
    int m_speedTrainerBarsPerStep = 4;
    int m_speedTrainerTempoStep = 2;
    int m_speedTrainerMaxTempo = 180;
    int m_speedTrainerStartTempo = 120;
    int m_speedTrainerCurrentTempo = 120;
    bool m_countingIn = false;
    
    int m_speedTrainerCurrentBar = 1;
    bool m_speedTrainerPolyrhythm = false; // If running polyrhythm
    bool m_speedTrainerFirstCycle = true;

    void startSpeedTrainerMain();
    void handleSpeedTrainerBarEnd();
    void updateSpeedTrainerStatus();
    void resetSpeedTrainer();

    void updateSectionTableEnabledState();
    SubdivisionPattern getDefaultSubdivisionPattern() const;

    int m_countInBeatsLeft = 0;
    int m_countInBar = 0;
    int m_countInBarTotal = 1;

    PresetManager presetManager;
    const QString presetFile = "presets.json";
    MetronomePreset currentPreset;
    int currentSectionIdx = -1;

    QList<qint64> tapTimes;
    QElapsedTimer tapTimer;

    int m_contentAreaHeight = -1;

    std::vector<int> m_polyrhythmGridColumns;
    int m_polyrhythmGridMain = 0, m_polyrhythmGridPoly = 0;
    void setupPolyrhythmGridHighlightOrder(int mainBeats, int polyBeats);

    // Polyrhythm UI state
    bool m_polyDialogOpen = false;

    QPushButton* m_timeSigBtn = nullptr; // Add as member
    QPushButton* m_polyrhythmEnableBtn;
};