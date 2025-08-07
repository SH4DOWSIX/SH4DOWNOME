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

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    QString subdivisionImagePathFromIndex(int index) const;

protected:
    bool eventFilter(QObject *obj, QEvent *event) override;
    void resizeEvent(QResizeEvent* event) override;

private slots:
    void onStartStop();
    void onTempoChanged(int value);
    void onSubdivisionChanged(int index);
    void onAccentChanged();
    void onMetronomePulse(int pulseIdx, bool accent, bool polyAccent, bool isBeat);

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

    void onStartClicked();
    void onStopClicked();
    void onTimerTick();
    void updateTimerLabel();

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

    int currentNumerator = 4;
    int currentDenominator = 4;

    void setTimeSignature(int numerator, int denominator);
    void updateTimeSignatureDisplay();
    QString soundFileForSet(const QString& set, bool accent) const;

    void setupAccentControls(int count);
    void loadSectionToUI(int idx);
    void saveUIToSection(int idx, bool doAutosave);
    void saveUIToSection(int idx);
    QTimer* timer = nullptr;
    int timerSecondsRemaining = 0;

    int getCurrentSubdivisionIndex() const;
    int subdivisionCountFromIndex(int index) const;
    NoteValue noteValueFromIndex(int index) const;
    QString subdivisionTextFromIndex(int index) const;

    void updatePolyrhythmButtonColor();

    QString m_soundSet = "Default";
    QColor m_accentColor = QColor(150,0,0);
    bool m_obsHidden = false;
    bool m_obsInLayout = true;

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
