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

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    QString subdivisionImagePathFromIndex(int index) const;
    bool isQuarterNotePulse(int pulseIdx) const;
    

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onStartStop();
    void onTempoChanged(int value);
    void onSubdivisionChanged(int index);
    void onAccentChanged();
    void onMetronomePulse(int idx, bool accent, bool polyAccent, bool isBeat, bool playPulse, int gridColumn);

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
    
    

    

    void onSubdivisionImageClicked();
    void setSubdivisionImage(int index);

    void onSectionRowMoved(int from, int to);

    void onSettingsClicked();

    void onPolyrhythmClicked();

private:
    Ui::MainWindow *ui;
    MetronomeEngine metronome;
    QSoundEffect accentSound, clickSound;
    QList<QCheckBox*> accentChecks;
    BeatIndicatorWidget* m_beatIndicatorWidget = nullptr;

    NoteValue m_savedAfterCountInSubdivision = NoteValue::Quarter;
    bool m_switchSubdivisionAfterCountIn = false;
    bool m_switchPolyrhythmAfterCountIn = false;
    bool m_pendingStartPoly = false;
    bool m_pendingStartSubdiv = false;
    bool m_pendingPolyrhythmStart = false;
    bool m_countInEnabled = false;

    int m_polyModeDelayMs = 225;
    bool m_pendingBarLabelUpdate = false;

    int m_polyModeBarsDelay = -1; // Number of FULL bars to wait after count-in before switching to polyrhythm
    int m_polyModeBarsElapsed = 0;
    bool m_armPolyModeAfterBars = false;
    bool m_armPolyModeNextBar = false;
    int m_playingBarCounter = 1;

    int m_polyrhythmGridStep = 0;

    void onCountInTick();
    void startMainMetronomeAfterCountIn();
    void startCountIn();
    QTimer* m_countInTimer = nullptr;     // Timer for count-in ticks
    int m_countInBeatsRemaining = 0;      // Quarter notes left in count-in

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

    int m_countInPulseIdx = 0;

    int getCurrentSubdivisionIndex() const;
    int subdivisionCountFromIndex(int index) const;
    NoteValue noteValueFromIndex(int index) const;
    QString subdivisionTextFromIndex(int index) const;

    void updatePolyrhythmButtonColor();

    QString m_soundSet = "Default";
    QColor m_accentColor = QColor(150,0,0);
    bool m_obsHidden = false;
    bool m_obsInLayout = true;

    QLabel* m_labelPolyrhythmNumerator = nullptr;
    QLabel* m_labelPolyrhythmDenominator = nullptr;
    QWidget* m_polyrhythmNumberWidget = nullptr;
    void updatePolyrhythmUI();
    void showPolyrhythmNumberDialog();
    void onPolyrhythmNumberClicked();

    bool m_alwaysOnTop = false;


    bool m_timerEnabled = false;
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
};
