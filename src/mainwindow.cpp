#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "beatindicatorwidget.h"
#include "subdivisionselectordialog.h"
#include "timesignaturedialog.h"
#include <QHBoxLayout>
#include <QLabel>
#include <QInputDialog>
#include <QMessageBox>
#include <QTableWidgetItem>
#include <QDateTime>
#include <QSignalBlocker>
#include <numeric>
#include <QSettings>
#include "obsbeatwidget.h"
#include "sectiontablewidget.h"
#include "settingsdialog.h"
#include <QPushButton>
#include <QVBoxLayout>
#include "svgutils.h"
#include "polyrhythmdialog.h"
#include <vector>
#include <algorithm>
#include <QWindow>
#include <QProcess>
#include "subdivisionpattern.h"
#include "noteassembler.h"
#include <QStringList>
#include <QTextEdit>
#include <QPlainTextEdit>
#include "audioengine.h"
#include "obsbeatwindow.h"



// LCM helper for polyrhythm grid
static int lcm(int a, int b) {
    if (a == 0 || b == 0) return 0;
    int x = a, y = b;
    while (y != 0) {
        int t = y;
        y = x % y;
        x = t;
    }
    return (a / x) * b;
}

static const struct {
    const char* name;
    int minBpm;
    int maxBpm;
} ItalianTempoMarkings[] = {
    {"Grave", 20, 40},
    {"Largo", 40, 60},
    {"Larghetto", 60, 66},
    {"Adagio", 66, 76},
    {"Andante", 76, 108},
    {"Moderato", 108, 120},
    {"Allegro", 120, 168},
    {"Vivace", 168, 176},
    {"Presto", 176, 200},
    {"Prestissimo", 200, 208},
    {"Speeeeed!", 209, 300},
};

static bool g_appIsQuitting = false;

// Helper: Get all matching tempo marking names for a given BPM
static QString getTempoMarkingsForBpm(int bpm) {
    QStringList names;
    for (const auto& marking : ItalianTempoMarkings) {
        if (bpm >= marking.minBpm && bpm <= marking.maxBpm)
            names << marking.name;
    }
    return names.join("\n");  // Each marking on its own line
}





QString MainWindow::soundFileForSet(const QString& set, bool accent) const {
    if (set == "Default")   return accent ? ":/resources/accent.wav" : ":/resources/click.wav";
    if (set == "Woodblock") return accent ? ":/resources/woodblock_accent.wav" : ":/resources/woodblock.wav";
    if (set == "Wooden")    return accent ? ":/resources/wooden_accent.wav"   : ":/resources/wooden.wav";
    if (set == "Bongo")     return accent ? ":/resources/bongo_accent.wav"    : ":/resources/bongo.wav";
    if (set == "Cowbell")   return accent ? ":/resources/cowbell_accent.wav"  : ":/resources/cowbell.wav";
    if (set == "Digital")   return accent ? ":/resources/digital_accent.wav"  : ":/resources/digital.wav";
    if (set == "Drum")      return accent ? ":/resources/drum_accent.wav"     : ":/resources/drum.wav";
    if (set == "Hihat")     return accent ? ":/resources/hihat_accent.wav"    : ":/resources/hihat.wav";
    if (set == "Metal")     return accent ? ":/resources/metal_accent.wav"    : ":/resources/metal.wav";
    return accent ? ":/resources/accent.wav" : ":/resources/click.wav";
}

void MainWindow::updatePolyrhythmButtonColor() {
    if (currentSectionIdx >= 0 && currentSectionIdx < (int)currentPreset.sections.size()) {
        bool enabled = currentPreset.sections[currentSectionIdx].hasPolyrhythm;
        QColor color = enabled ? m_accentColor : m_accentColor.darker(200);
        ui->btnPolyrhythm->setText("Polyrhythm");
        ui->btnPolyrhythm->setStyleSheet(QString("background-color: %1; color: white;").arg(color.name()));
    }
}

// Helper: Map a SubdivisionPattern to NoteAssemblerConfig for atomic SVG assembly
NoteAssemblerConfig MainWindow::configForPattern(const SubdivisionPattern& pattern) const {
    NoteAssemblerConfig cfg;
    cfg.pixmapSize = QSize(48, 48);

    cfg.noteCount = pattern.pulses.size();
    cfg.noteTypes.clear();
    cfg.dottedNotes.clear();

    auto isClose = [](double a, double b) { return std::abs(a - b) < 1e-6; }; // Tighter tolerance

    // Check if we're in compound time
    bool isCompoundTime = (currentDenominator == 8 && currentNumerator % 3 == 0 && currentNumerator > 3);

    // --- Ceiling logic for tuplets/triplets ---
    auto ceilingNoteTypeForDuration = [isCompoundTime](double dur) {
        if (isCompoundTime) {
            // In compound time, durations are relative to dotted quarter
            if (dur <= 1.0/6)        return AssembledNoteType::Sixteenth;     // 1/6 of dotted quarter = sixteenth
            if (dur <= 1.0/3)        return AssembledNoteType::Eighth;        // 1/3 of dotted quarter = eighth
            if (dur <= 2.0/3)        return AssembledNoteType::Eighth;        // 2/3 of dotted quarter = dotted eighth (use eighth with dot)
            return AssembledNoteType::Quarter;                                 // 1.0 = dotted quarter (use quarter with dot)
        } else {
            // Simple time logic (unchanged)
            if (dur <= 0.125)    return AssembledNoteType::ThirtySecond;
            if (dur <= 0.25)     return AssembledNoteType::Sixteenth;
            if (dur <= 0.5)      return AssembledNoteType::Eighth;
            return AssembledNoteType::Quarter;
        }
    };
    
    auto ceilingRestTypeForDuration = [isCompoundTime](double dur) {
        if (isCompoundTime) {
            if (dur <= 1.0/6)        return AssembledNoteType::Rest_Sixteenth;
            if (dur <= 1.0/3)        return AssembledNoteType::Rest_Eighth;
            if (dur <= 2.0/3)        return AssembledNoteType::Rest_Eighth;
            return AssembledNoteType::Rest_Quarter;                            // 1.0 = dotted quarter rest
        } else {
            if (dur <= 0.125)    return AssembledNoteType::Rest_ThirtySecond;
            if (dur <= 0.25)     return AssembledNoteType::Rest_Sixteenth;
            if (dur <= 0.5)      return AssembledNoteType::Rest_Eighth;
            return AssembledNoteType::Rest_Quarter;
        }
    };

    bool isTupletOrTriplet =
        (pattern.category == SubdivisionCategory::Tuplet) ||
        pattern.name.toLower().contains("triplet");

    if (isTupletOrTriplet) {
        for (const SubdivisionPulse& p : pattern.pulses) {
            if (p.isRest)
                cfg.noteTypes.push_back(ceilingRestTypeForDuration(p.duration));
            else
                cfg.noteTypes.push_back(ceilingNoteTypeForDuration(p.duration));
            cfg.dottedNotes.push_back(p.isDotted);
        }
    } else {
        for (const SubdivisionPulse& p : pattern.pulses) {
            if (p.isRest) {
                if (isCompoundTime) {
                    // Compound time rest logic - exact matches first
                    if (isClose(p.duration, 1.0/6))      cfg.noteTypes.push_back(AssembledNoteType::Rest_Sixteenth);
                    else if (isClose(p.duration, 1.0/3)) cfg.noteTypes.push_back(AssembledNoteType::Rest_Eighth);
                    else if (isClose(p.duration, 2.0/3)) cfg.noteTypes.push_back(AssembledNoteType::Rest_Eighth);
                    else if (isClose(p.duration, 0.5))   cfg.noteTypes.push_back(AssembledNoteType::Rest_Eighth);
                    else if (isClose(p.duration, 1.0))   cfg.noteTypes.push_back(AssembledNoteType::Rest_Quarter);  // Dotted quarter rest
                    else if (p.duration < 1.0/3)         cfg.noteTypes.push_back(AssembledNoteType::Rest_Sixteenth);
                    else if (p.duration < 2.0/3)         cfg.noteTypes.push_back(AssembledNoteType::Rest_Eighth);
                    else                                 cfg.noteTypes.push_back(AssembledNoteType::Rest_Quarter);
                } else {
                    // Simple time rest logic (unchanged)
                    if (isClose(p.duration, 1.0/3))      cfg.noteTypes.push_back(AssembledNoteType::Rest_Eighth);
                    else if (isClose(p.duration, 0.25))  cfg.noteTypes.push_back(AssembledNoteType::Rest_Sixteenth);
                    else if (isClose(p.duration, 0.125)) cfg.noteTypes.push_back(AssembledNoteType::Rest_ThirtySecond);
                    else if (isClose(p.duration, 0.5))   cfg.noteTypes.push_back(AssembledNoteType::Rest_Eighth);
                    else if (isClose(p.duration, 1.0))   cfg.noteTypes.push_back(AssembledNoteType::Rest_Quarter);
                    else if (p.duration < 0.25)          cfg.noteTypes.push_back(AssembledNoteType::Rest_ThirtySecond);
                    else if (p.duration < 0.5)           cfg.noteTypes.push_back(AssembledNoteType::Rest_Sixteenth);
                    else                                 cfg.noteTypes.push_back(AssembledNoteType::Rest_Quarter);
                }
            } else {
                if (isCompoundTime) {
                    // Compound time note logic - distinguish between dotted quarter (1.0) and dotted eighth (2/3)
                    if (isClose(p.duration, 1.0/6))      cfg.noteTypes.push_back(AssembledNoteType::Sixteenth); // 1/6 = sixteenth note
                    else if (isClose(p.duration, 1.0/3)) cfg.noteTypes.push_back(AssembledNoteType::Eighth);    // 1/3 = eighth note
                    else if (isClose(p.duration, 2.0/3)) cfg.noteTypes.push_back(AssembledNoteType::Eighth);    // 2/3 = dotted eighth (eighth note with dot)
                    else if (isClose(p.duration, 0.5))   cfg.noteTypes.push_back(AssembledNoteType::Eighth);    // For other dotted eighth patterns
                    else if (isClose(p.duration, 1.0))   cfg.noteTypes.push_back(AssembledNoteType::Quarter);   // 1.0 = dotted quarter (quarter note with dot)
                    else if (p.duration < 1.0/3)         cfg.noteTypes.push_back(AssembledNoteType::Sixteenth);
                    else if (p.duration < 2.0/3)         cfg.noteTypes.push_back(AssembledNoteType::Eighth);
                    else                                 cfg.noteTypes.push_back(AssembledNoteType::Quarter);   // Fallback to quarter for full beat
                } else {
                    // Simple time note logic (unchanged)
                    if (isClose(p.duration, 1.0/3))      cfg.noteTypes.push_back(AssembledNoteType::Eighth);
                    else if (isClose(p.duration, 0.25))  cfg.noteTypes.push_back(AssembledNoteType::Sixteenth);
                    else if (isClose(p.duration, 0.125)) cfg.noteTypes.push_back(AssembledNoteType::ThirtySecond);
                    else if (isClose(p.duration, 0.5))   cfg.noteTypes.push_back(AssembledNoteType::Eighth);
                    else if (isClose(p.duration, 0.75))  cfg.noteTypes.push_back(AssembledNoteType::Eighth); // DottedEighth if you add it
                    else if (isClose(p.duration, 1.0))   cfg.noteTypes.push_back(AssembledNoteType::Quarter);
                    else if (p.duration < 0.25)          cfg.noteTypes.push_back(AssembledNoteType::ThirtySecond);
                    else if (p.duration < 0.5)           cfg.noteTypes.push_back(AssembledNoteType::Sixteenth);
                    else                                 cfg.noteTypes.push_back(AssembledNoteType::Quarter);
                }
            }
            cfg.dottedNotes.push_back(p.isDotted);
        }
    }
    cfg.beamed = pattern.pulses.size() > 1;

    // --- FIX: Always check for triplets by name OR tuplets by category ---
    bool isTriplet = pattern.name.toLower().contains("triplet");
    if (pattern.category == SubdivisionCategory::Tuplet || isTriplet) {
        cfg.tupletNumber = pattern.pulses.size();
    }
    return cfg;
}






MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), m_obsInLayout(true)
{
    ui->setupUi(this);

    connect(qApp, &QCoreApplication::aboutToQuit, this, []() {
    g_appIsQuitting = true;
});

    setWindowTitle("SH4DOWNOME");
    setMinimumWidth(487);
    setMaximumWidth(487);

    // ... unchanged setup code ...

    // RtAudio engine setup
bool accentOk = metronome.loadSample("accent", soundFileForSet(m_soundSet, true));
bool clickOk  = metronome.loadSample("click",  soundFileForSet(m_soundSet, false));

metronome.setAccentSound("accent");
metronome.setClickSound("click");
metronome.setVolume(1.0f);

    connect(&metronome, &MetronomeEngine::pulse, this, &MainWindow::onMetronomePulse);


    QVBoxLayout* mainLayout = qobject_cast<QVBoxLayout*>(ui->centralwidget->layout());

    m_tapTempoResumeTimer = new QTimer(this);
    m_tapTempoResumeTimer->setSingleShot(true);
    connect(m_tapTempoResumeTimer, &QTimer::timeout, this, [this]() {
        if (m_metronomeWasRunning) {
            metronome.start();
            m_metronomeWasRunning = false;
        }
    });

    const int minRows = 10;
    int rowH = ui->tableSections->verticalHeader()->defaultSectionSize();
    int headerH = ui->tableSections->horizontalHeader()->height();
    int frame = 2 * ui->tableSections->frameWidth();
    int minTableHeight = minRows * rowH + headerH + frame;
    ui->tableSections->setMinimumHeight(minTableHeight);
    ui->tableSections->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->tableSections->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->tableSections->verticalHeader()->setDefaultAlignment(Qt::AlignCenter);

    QSettings settings("YourCompany", "MetronomeApp");
    QString savedColor = settings.value("accentColor", "#960000").toString();
    m_accentColor = QColor(savedColor);
    m_soundSet = settings.value("soundSet", "Default").toString();
    m_obsHidden = settings.value("obsHidden", false).toBool();
    m_alwaysOnTop = settings.value("alwaysOnTop", false).toBool();

    if (m_alwaysOnTop) {
        setWindowFlag(Qt::WindowStaysOnTopHint, true);
    }

    ui->btnStartStop->setText("Start");
    ui->btnStartStop->setStyleSheet("background-color: #004100; color: white;");



    ui->tableSections->setIconSize(QSize(48, 48));



    connect(ui->btnStartStop, &QPushButton::clicked, this, &MainWindow::onStartStop);
    connect(ui->spinTempo, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onTempoChanged);
    connect(ui->btnTapTempo, &QPushButton::clicked, this, &MainWindow::onTapTempo);
    connect(ui->sliderTempo, &QSlider::valueChanged, this, &MainWindow::onTempoSliderChanged);
    connect(ui->sliderTempo, QOverload<int>::of(&QSlider::valueChanged), this, &MainWindow::onTempoChanged);
    connect(ui->btnRenamePreset, &QPushButton::clicked, this, &MainWindow::onRenamePreset);
    connect(ui->spinTempo, &QSpinBox::editingFinished, this, &MainWindow::onSpinTempoEditingFinished);
    connect(ui->sliderTempo, &QSlider::sliderReleased, this, &MainWindow::onTempoSliderReleased);
    ui->verticalLayoutTimeSignature->setSpacing(0);
    ui->spinTempo->setAlignment(Qt::AlignCenter);
    ui->timeEditDuration->setAlignment(Qt::AlignCenter);

    connect(ui->btnTimer, &QPushButton::clicked, this, &MainWindow::onTimerToggle);
    connect(ui->btnSpeed, &QPushButton::clicked, this, &MainWindow::onSpeedToggle);
    m_timerEnabled = false;
    m_speedEnabled = false;
    updateTimerUI();
    updateSpeedUI();

    ui->btnTimer->setMaximumWidth(47);
    ui->btnSpeed->setMaximumWidth(47);

connect(ui->btnCountIn, &QPushButton::clicked, this, [this]() {
    m_countInEnabled = !m_countInEnabled;
    QColor color = m_countInEnabled ? m_accentColor : m_accentColor.darker(200);
    ui->btnCountIn->setStyleSheet(QString("background-color: %1; color: white;").arg(color.name()));
    metronome.setCountInEnabled(m_countInEnabled);
});

    ui->btnPolyrhythm->setStyleSheet(QString("background-color: %1; color: white;").arg(m_accentColor.darker(200).name()));
    ui->btnTimer->setStyleSheet(QString("background-color: %1; color: white;").arg(m_accentColor.darker(200).name()));
    ui->btnCountIn->setStyleSheet(QString("background-color: %1; color: white;").arg(m_accentColor.darker(200).name()));
    m_countInEnabled = false;

    // Remove labels from old layout
    ui->verticalLayoutTimeSignature->removeWidget(ui->labelNumerator);
    ui->verticalLayoutTimeSignature->removeWidget(ui->labelDenominator);

    // --- Time signature container and layout (create ONCE) ---
QWidget* timeSigContainer = new QWidget(this);
timeSigContainer->setFixedSize(40, 30);
timeSigContainer->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
QVBoxLayout* timeSigLayout = new QVBoxLayout(timeSigContainer);
timeSigLayout->setContentsMargins(0, 0, 0, 0);
timeSigLayout->setSpacing(0);
ui->labelNumerator->setContentsMargins(0, 0, 0, 0);
ui->labelDenominator->setContentsMargins(0, 0, 0, 0);
ui->labelNumerator->setAlignment(Qt::AlignHCenter | Qt::AlignBottom);
ui->labelDenominator->setAlignment(Qt::AlignHCenter | Qt::AlignTop);
timeSigLayout->addWidget(ui->labelNumerator, 1);
timeSigLayout->addWidget(ui->labelDenominator, 1);

// Use member variable for timeSigBtn
m_timeSigBtn = new QPushButton(timeSigContainer);
m_timeSigBtn->setFlat(true);
m_timeSigBtn->setStyleSheet("background: transparent; border: none;");
m_timeSigBtn->setCursor(Qt::PointingHandCursor);
m_timeSigBtn->setFocusPolicy(Qt::NoFocus);
m_timeSigBtn->setFixedSize(timeSigContainer->size());
m_timeSigBtn->raise();
m_timeSigBtn->move(0, 0);
ui->verticalLayoutTimeSignature->addWidget(timeSigContainer, 0, Qt::AlignHCenter);
connect(m_timeSigBtn, &QPushButton::clicked, this, [this]() {
    TimeSignatureDialog dlg(currentNumerator, currentDenominator, this);
    if (dlg.exec() == QDialog::Accepted)
        setTimeSignature(dlg.selectedNumerator(), dlg.selectedDenominator());
});

    // --- Polyrhythm numbers container (mirror timeSigContainer) ---
    m_polyrhythmNumberWidget = new QWidget(this);
    m_polyrhythmNumberWidget->setFixedSize(timeSigContainer->size());
    m_polyrhythmNumberWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
    m_polyrhythmEnableBtn = ui->btnPolyrhythm;
    QVBoxLayout* polyNumLayout = new QVBoxLayout(m_polyrhythmNumberWidget);
    polyNumLayout->setContentsMargins(0, 0, 0, 0);
    polyNumLayout->setSpacing(0);

    m_labelPolyrhythmNumerator = new QLabel("3", m_polyrhythmNumberWidget);
    m_labelPolyrhythmDenominator = new QLabel("2", m_polyrhythmNumberWidget);

    QFont tsFont = ui->labelNumerator->font();
    QString tsStyle = ui->labelNumerator->styleSheet();
    Qt::Alignment numeratorAlign = Qt::AlignHCenter | Qt::AlignBottom;
    Qt::Alignment denominatorAlign = Qt::AlignHCenter | Qt::AlignTop;

    m_labelPolyrhythmNumerator->setFont(tsFont);
    m_labelPolyrhythmDenominator->setFont(tsFont);
    m_labelPolyrhythmNumerator->setStyleSheet(tsStyle);
    m_labelPolyrhythmDenominator->setStyleSheet(tsStyle);
    m_labelPolyrhythmNumerator->setAlignment(numeratorAlign);
    m_labelPolyrhythmDenominator->setAlignment(denominatorAlign);
    m_labelPolyrhythmNumerator->setContentsMargins(0, 0, 0, -2);
    m_labelPolyrhythmDenominator->setContentsMargins(0, 0, 0, 0);

    ui->labelSubdivisionImage->setAlignment(Qt::AlignCenter);



    ui->tempoRowWidget->setFixedHeight(45);

    // ... rest of constructor will continue in next chunk ...







        // --- Speed Trainer UI enable/disable ---
    auto setSpeedTrainerUIEnabled = [this](bool enabled) {
        ui->spinBarsPerStep->setEnabled(enabled);
        ui->spinTempoStep->setEnabled(enabled);
        ui->spinMaxTempo->setEnabled(enabled);
    };

    // Sync UI to variable
    auto updateSpeedTrainerVarsFromUI = [this]() {
        m_speedTrainerBarsPerStep = ui->spinBarsPerStep->value();
        m_speedTrainerTempoStep = ui->spinTempoStep->value();
        m_speedTrainerMaxTempo = ui->spinMaxTempo->value();
    };
    updateSpeedTrainerVarsFromUI();

    auto stopOnEdit = [this]() {
        if (metronome.isRunning())
            onStartStop();
    };
    connect(ui->spinBarsPerStep, QOverload<int>::of(&QSpinBox::valueChanged), this, stopOnEdit);
    connect(ui->spinTempoStep, QOverload<int>::of(&QSpinBox::valueChanged), this, stopOnEdit);
    connect(ui->spinMaxTempo, QOverload<int>::of(&QSpinBox::valueChanged), this, stopOnEdit);

    connect(ui->spinBarsPerStep, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int){
        resetSpeedTrainer();
        if (metronome.isRunning()) onStartStop();
    });
    connect(ui->spinTempoStep, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int){
        resetSpeedTrainer();
        if (metronome.isRunning()) onStartStop();
    });
    connect(ui->spinMaxTempo, QOverload<int>::of(&QSpinBox::valueChanged), this, [this](int){
        resetSpeedTrainer();
        if (metronome.isRunning()) onStartStop();
    });

    polyNumLayout->addWidget(m_labelPolyrhythmNumerator);
    polyNumLayout->addWidget(m_labelPolyrhythmDenominator);

    // Insert in the same place as before
    int idx = ui->tempoLayout->indexOf(ui->labelSubdivisionImage);
    ui->tempoLayout->insertWidget(idx, m_polyrhythmNumberWidget);
    m_polyrhythmNumberWidget->setCursor(Qt::PointingHandCursor);
    m_polyrhythmNumberWidget->setVisible(false);
    m_polyrhythmNumberWidget->installEventFilter(this);
    ui->labelSpeedTrainerStatus->hide();

    connect(ui->btnPolyrhythm, &QPushButton::clicked, this, &MainWindow::onPolyrhythmClicked);
    ui->labelSubdivisionImage->installEventFilter(this);
    ui->spinTempo->installEventFilter(this);

    timer = new QTimer(this);
    timer->setInterval(1000);
    connect(timer, &QTimer::timeout, this, [this]() {
        QTime t = ui->timeEditDuration->time();
        if (t == QTime(0, 0, 0)) {
            timer->stop();
            ui->btnStartStop->setText("Start");
            ui->btnStartStop->setStyleSheet("background-color: #004100; color: white;");
            ui->timeEditDuration->setReadOnly(false);
            ui->timeEditDuration->setTime(m_lastEnteredTimerValue);
            m_timerWasRunning = false;
            if (metronome.isRunning()) {
                metronome.stop();
                if (ui->obsBeatWidget) ui->obsBeatWidget->setPlaying(false);
            }
            if (m_speedEnabled) {
                ui->spinTempo->setValue(m_speedTrainerStartTempo);
                ui->sliderTempo->setValue(m_speedTrainerStartTempo);
                metronome.setTempo(m_speedTrainerStartTempo);
                if (ui->obsBeatWidget)
                    ui->obsBeatWidget->setTempo(m_speedTrainerStartTempo);
            }
        } else {
            t = t.addSecs(-1);
            ui->timeEditDuration->setTime(t);
        }
    });

    connect(&metronome, &MetronomeEngine::pulse, this, &MainWindow::onMetronomePulse);

    connect(ui->btnSavePreset, &QPushButton::clicked, this, &MainWindow::onSavePreset);
    connect(ui->comboPresets, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &MainWindow::onLoadPreset);
    connect(ui->btnDeletePreset, &QPushButton::clicked, this, &MainWindow::onDeletePreset);

    connect(ui->btnAddSection, &QPushButton::clicked, this, &MainWindow::onAddSection);
    connect(ui->btnRemoveSection, &QPushButton::clicked, this, &MainWindow::onRemoveSection);
    connect(ui->btnSaveSection, &QPushButton::clicked, this, &MainWindow::onSaveSection);
    ui->btnSaveSection->hide();
    connect(ui->btnMoveSectionUp, &QPushButton::clicked, this, &MainWindow::onMoveSectionUp);
    connect(ui->btnMoveSectionDown, &QPushButton::clicked, this, &MainWindow::onMoveSectionDown);
    ui->btnMoveSectionUp->hide();
    ui->btnMoveSectionDown->hide();
    connect(ui->tableSections, &QTableWidget::currentCellChanged, this, &MainWindow::onSectionSelected);

    connect(ui->tableSections, &QTableWidget::cellChanged, this, [this](int row, int col) {
        if (col == 0 && row >= 0 && row < (int)currentPreset.sections.size()) {
            QTableWidgetItem* item = ui->tableSections->item(row, 0);
            if (!item) return;
            QString newLabel = item->text();
            currentPreset.sections[row].label = newLabel;
            presetManager.savePreset(currentPreset);
            presetManager.saveToDisk(presetFile);
        }
    });

    QHeaderView* header = ui->tableSections->horizontalHeader();
    ui->tableSections->setColumnWidth(3, 50);
    ui->tableSections->setColumnWidth(1, 60);
    ui->tableSections->setColumnWidth(2, 60);
    header->setSectionResizeMode(0, QHeaderView::Stretch);
    ui->tableSections->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    ui->tableSections->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    ui->tableSections->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    ui->tableSections->setSelectionBehavior(QAbstractItemView::SelectRows);
    connect(ui->tableSections, &SectionTableWidget::moveSectionRequested,
        this, &MainWindow::onMoveSectionViaShortcut);
    ui->tableSections->setSelectionMode(QAbstractItemView::SingleSelection);

    connect(ui->sliderTempo, &QSlider::sliderPressed, this, &MainWindow::onTempoSliderPressed);
    connect(ui->sliderTempo, &QSlider::sliderReleased, this, &MainWindow::onTempoSliderReleased);
    connect(ui->sliderTempo, QOverload<int>::of(&QSlider::valueChanged), this, &MainWindow::onTempoChanged);

    connect(ui->spinTempo, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onTempoChanged);
    connect(ui->spinTempo, &QSpinBox::editingFinished, this, &MainWindow::onSpinTempoEditingFinished);

    connect(ui->btnSettings, &QPushButton::clicked, this, &MainWindow::onSettingsClicked);
    connect(ui->tableSections, &QTableWidget::currentCellChanged,
        this, [this](int currentRow, int, int, int) {
            if (currentRow >= 0 && currentRow < (int)currentPreset.sections.size()) {
                saveUIToSection(currentSectionIdx, true);
                loadSectionToUI(currentRow);
            }
        });

    ui->btnStartStop->setStyleSheet("background-color: #004100; color: white;");

    if (ui->beatIndicator) {
        QLayout* layout = ui->beatIndicator->parentWidget()->layout();
        if (layout) layout->removeWidget(ui->beatIndicator);
        ui->beatIndicator->deleteLater();
    }
    m_beatIndicatorWidget = new BeatIndicatorWidget(this);
    ui->beatLayout->addWidget(m_beatIndicatorWidget);
    if (m_beatIndicatorWidget)
        m_beatIndicatorWidget->setAccentColor(m_accentColor);

    QPalette pal = qApp->palette();
    pal.setColor(QPalette::Highlight, m_accentColor);
    qApp->setPalette(pal);

// Replace the original block with this updated popout-aware logic.
// Paste this in place of the original snippet.

    const int obsHeight = 306;
    if (ui->obsBeatWidget && mainLayout) {
        if (!m_obsHidden) {
            // Ensure we have a hidden host to keep the widget alive when hidden/disabled
            if (!m_obsHiddenHost) {
                m_obsHiddenHost = new QWidget(this);
                m_obsHiddenHost->hide();
            }

            // If there is no popout window yet, create one and reparent the obs widget into it.
            // This gives the behaviour: enabling OBS opens a separate window for the OBS widget.
            if (!m_obsWindow) {
                // Remove from any existing layout first
                QWidget* currentParent = ui->obsBeatWidget->parentWidget();
                if (currentParent) {
                    QLayout* parentLayout = currentParent->layout();
                    if (parentLayout) parentLayout->removeWidget(ui->obsBeatWidget);
                }

                // Create popout window (constructor reparents the widget into the window)
                m_obsWindow = new OBSBeatWindow(ui->obsBeatWidget, this);
                connect(m_obsWindow, &OBSBeatWindow::windowAboutToClose, this, &MainWindow::onObsWindowAboutToClose);
                m_obsWindow->show();
            } else {
                // If popout already exists, ensure it's visible and focused
                m_obsWindow->show();
                m_obsWindow->raise();
                m_obsWindow->activateWindow();
            }

            // Configure widget sizing while in its window (keeps consistent look if you reparent back)
            ui->obsBeatWidget->setVisible(true);
            ui->obsBeatWidget->setMinimumHeight(obsHeight);
            ui->obsBeatWidget->setMaximumHeight(obsHeight);
            ui->obsBeatWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);

            // Mark that we are not showing it inline (it's in the popout)
            m_obsInLayout = false;
        } else {
            // OBS is disabled in settings: ensure popout is closed and widget is hidden.
            if (m_obsWindow) {
                // Disconnect to avoid signals during teardown
                m_obsWindow->disconnect(this);
                // Closing will emit windowAboutToClose which will reparent the widget into hidden host
                m_obsWindow->close();
                // Ensure deletion scheduled
                m_obsWindow->deleteLater();
                m_obsWindow = nullptr;
            }

            // Ensure widget is parented to hidden host and hidden from UI
            if (!m_obsHiddenHost) {
                m_obsHiddenHost = new QWidget(this);
                m_obsHiddenHost->hide();
            }

            QWidget* currentParent = ui->obsBeatWidget->parentWidget();
            if (currentParent) {
                QLayout* parentLayout = currentParent->layout();
                if (parentLayout) parentLayout->removeWidget(ui->obsBeatWidget);
            }
            ui->obsBeatWidget->setParent(m_obsHiddenHost);
            ui->obsBeatWidget->hide();
            m_obsInLayout = false;
        }

        // Common updates (tempo / playing state) still apply to the same widget instance
        ui->obsBeatWidget->setPlaying(false);
        ui->obsBeatWidget->setTempo(ui->spinTempo->value());
        // Pattern image will be set when section loads.
    }

    connect(ui->sliderVolume, &QSlider::valueChanged, ui->spinVolume, &QSpinBox::setValue);
    connect(ui->spinVolume, QOverload<int>::of(&QSpinBox::valueChanged), ui->sliderVolume, &QSlider::setValue);

    auto setVolume = [this](int value){
        float vol = value / 100.0f;
        metronome.setVolume(vol);
    };
    connect(ui->sliderVolume, &QSlider::valueChanged, setVolume);
    connect(ui->spinVolume, QOverload<int>::of(&QSpinBox::valueChanged), setVolume);

    connect(ui->sliderVolume, &QSlider::valueChanged, this, [](int value){
        QSettings settings("YourCompany", "MetronomeApp");
        settings.setValue("volume", value);
    });

    int savedVolume = settings.value("volume", 90).toInt();
    ui->sliderVolume->setValue(savedVolume);
    setVolume(savedVolume);

    currentNumerator = 4;
    currentDenominator = 4;
    updateTimeSignatureDisplay();
ui->labelTempo->setText(getTempoMarkingsForBpm(ui->spinTempo->value()));
    presetManager.loadFromDisk(presetFile);
    refreshPresetList();

    if (ui->comboPresets->count() > 0) {
        onLoadPreset();
    } else {
        currentPreset.songName = "";
        currentPreset.sections.clear();
        onAddSection();
    }
    qApp->installEventFilter(this);
}





MainWindow::~MainWindow() {
    // --- PATCH: Save current UI edits before app closes ---
    saveUIToSection(currentSectionIdx, true); // <-- This ensures any unsaved UI changes are written
    presetManager.saveToDisk(presetFile);
    delete ui;
}






bool MainWindow::eventFilter(QObject* obj, QEvent* event) {
    // 1. GLOBAL KEY SHORTCUTS (space, up, down)
    if (event->type() == QEvent::KeyPress) {
        QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);

        // Ignore if in a text-editing widget
        QWidget* fw = QApplication::focusWidget();
        if (fw &&
            (qobject_cast<QLineEdit*>(fw) ||
             qobject_cast<QTextEdit*>(fw) ||
             qobject_cast<QPlainTextEdit*>(fw) ||
             qobject_cast<QSpinBox*>(fw) ||
             qobject_cast<QDoubleSpinBox*>(fw) ||
             (qobject_cast<QComboBox*>(fw) && static_cast<QComboBox*>(fw)->isEditable())))
        {
            return false; // Let those widgets handle the key normally
        }

        // Only if app is active
        if (!this->isActiveWindow())
            return false;

        // Spacebar: start/stop
        if (keyEvent->key() == Qt::Key_Space) {
            onStartStop();
            return true; // handled
        }

        // Up arrow: previous section
// Up arrow: previous section (ONLY if Ctrl is NOT held)
if (keyEvent->key() == Qt::Key_Up && !(keyEvent->modifiers() & Qt::ControlModifier)) {
    int row = ui->tableSections->currentRow();
    if (row > 0)
        ui->tableSections->selectRow(row - 1);
    return true;
}
// Down arrow: next section (ONLY if Ctrl is NOT held)
if (keyEvent->key() == Qt::Key_Down && !(keyEvent->modifiers() & Qt::ControlModifier)) {
    int row = ui->tableSections->currentRow();
    if (row < ui->tableSections->rowCount() - 1)
        ui->tableSections->selectRow(row + 1);
    return true;
}
    }

    // 2. YOUR EXISTING MOUSE EVENTS
    if ((obj == ui->labelNumerator || obj == ui->labelDenominator) && event->type() == QEvent::MouseButtonRelease) {
        if (ui->labelNumerator->isEnabled()) {
            TimeSignatureDialog dlg(currentNumerator, currentDenominator, this);
            if (dlg.exec() == QDialog::Accepted) {
                int newNum = dlg.selectedNumerator();
                int newDen = dlg.selectedDenominator();
                setTimeSignature(newNum, newDen);
            }
        }
        return true;
    }
    if (obj == ui->labelSubdivisionImage && event->type() == QEvent::MouseButtonRelease) {
        if (ui->labelSubdivisionImage->isEnabled())
            onSubdivisionImageClicked();
        return true;
    }
    if (obj == m_polyrhythmNumberWidget && event->type() == QEvent::MouseButtonRelease) {
        if (m_polyrhythmNumberWidget->isEnabled())
            onPolyrhythmNumberClicked();
        return true;
    }
    return QMainWindow::eventFilter(obj, event);
}


void MainWindow::resizeEvent(QResizeEvent* event)
{
    QMainWindow::resizeEvent(event);
    if (ui->obsBeatWidget && ui->contentContainer) {
        int obsHeight = ui->obsBeatWidget->isVisible() ? ui->obsBeatWidget->sizeHint().height() : 0;
        m_contentAreaHeight = height() - obsHeight;
    }
}

void MainWindow::setTimeSignature(int numerator, int denominator) {
    // --- Detect if compoundness changes ---
    bool wasCompound = (currentDenominator == 8 && currentNumerator % 3 == 0 && currentNumerator > 3);
    bool isCompound = (denominator == 8 && numerator % 3 == 0 && numerator > 3);

    currentNumerator = numerator;
    currentDenominator = denominator;
    updateTimeSignatureDisplay();

    // --- PATCH: Reset accents to all off BEFORE updating accent controls/metronome ---
    if (currentSectionIdx >= 0 && currentSectionIdx < (int)currentPreset.sections.size()) {
        int accentCount = numerator;
        if (denominator == 8 && numerator % 3 == 0 && numerator > 3)
            accentCount = numerator / 3;
        auto& accents = currentPreset.sections[currentSectionIdx].accents;
        accents.resize(accentCount, false);
        std::fill(accents.begin(), accents.end(), false);
        metronome.setAccentPattern(accents);
    }

    // --- Reset subdivision if switching compoundness ---
    if (wasCompound != isCompound && currentSectionIdx >= 0 && currentSectionIdx < (int)currentPreset.sections.size()) {
        if (isCompound) {
            // Compound time: Dotted quarter
            SubdivisionPattern dottedQuarter;
            dottedQuarter.name = "Dotted Quarter";
            dottedQuarter.pulses = {{1.0, false, false, true}}; // duration=1.0, not rest, not accented, isDotted=true
            currentPreset.sections[currentSectionIdx].subdivisionPattern = dottedQuarter;
        } else {
            // Simple time: Quarter note
            currentPreset.sections[currentSectionIdx].subdivisionPattern = getDefaultSubdivisionPattern();
        }
        // Update UI and metronome
        metronome.setSubdivisionPattern(currentPreset.sections[currentSectionIdx].subdivisionPattern);
        // Update subdivision image
        NoteAssembler assembler;
        NoteAssemblerConfig cfg = configForPattern(currentPreset.sections[currentSectionIdx].subdivisionPattern);
        cfg.centerVertically = true;
        QPixmap px = assembler.assembleNote(cfg);
        QSize targetSize = ui->labelSubdivisionImage->size();
        QPixmap scaledPx = px.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
        if (ui->labelSubdivisionImage)
            ui->labelSubdivisionImage->setPixmap(scaledPx);
        updateSubPolyCell(currentSectionIdx);
        saveUIToSection(currentSectionIdx, true);
    }

    if (currentSectionIdx >= 0 && currentSectionIdx < (int)currentPreset.sections.size()) {
        currentPreset.sections[currentSectionIdx].numerator = numerator;
        currentPreset.sections[currentSectionIdx].denominator = denominator;
        saveUIToSection(currentSectionIdx, true);
        if (currentSectionIdx < ui->tableSections->rowCount()) {
            QTableWidgetItem* timeSigItem = ui->tableSections->item(currentSectionIdx, 1);
            if (timeSigItem)
                timeSigItem->setText(QString("%1/%2").arg(numerator).arg(denominator));
        }
    }
    metronome.setTimeSignature(numerator, denominator);
    setupAccentControls(numerator);

    // Update beat indicator based on mode
    if (currentSectionIdx >= 0 && currentSectionIdx < (int)currentPreset.sections.size() && 
        currentPreset.sections[currentSectionIdx].hasPolyrhythm) {
        const auto& poly = currentPreset.sections[currentSectionIdx].polyrhythm;
        m_beatIndicatorWidget->setPolyrhythmGrid(
            poly.primaryBeats,
            poly.secondaryBeats,
            -1 // no highlight when not playing
        );
        m_beatIndicatorWidget->setMode(BeatIndicatorMode::PolyrhythmGrid);
    } else {
        int beatsPerBar = currentNumerator;
        // --- PATCH: handle compound time main beats ---
        if (currentDenominator == 8 && currentNumerator % 3 == 0 && currentNumerator > 3)
            beatsPerBar = currentNumerator / 3;
        int subdivisions = 1;
        if (currentSectionIdx >= 0 && currentSectionIdx < (int)currentPreset.sections.size()) {
            subdivisions = currentPreset.sections[currentSectionIdx].subdivisionPattern.pulses.size();
        }
        if (subdivisions == 0) subdivisions = 1;
        m_beatIndicatorWidget->setBeats(beatsPerBar);
        m_beatIndicatorWidget->setSubdivisions(subdivisions);
        m_beatIndicatorWidget->setCurrent(0, 0);
        m_beatIndicatorWidget->setMode(BeatIndicatorMode::Circles);
    }

    // Restart metronome if running and in polyrhythm mode
    if (metronome.isRunning() &&
        currentSectionIdx >= 0 &&
        currentSectionIdx < (int)currentPreset.sections.size() &&
        currentPreset.sections[currentSectionIdx].hasPolyrhythm) {
        metronome.stop();
        metronome.start();
    }
}

void MainWindow::updateTimeSignatureDisplay() {
    ui->labelNumerator->setAlignment(Qt::AlignHCenter);
    ui->labelDenominator->setAlignment(Qt::AlignHCenter);
    ui->labelNumerator->setText(QString::number(currentNumerator));
    ui->labelDenominator->setText(QString::number(currentDenominator));
}




void MainWindow::startCountIn() {
    metronome.stop();
    metronome.startWithCountIn(currentNumerator);
    if (ui->obsBeatWidget) ui->obsBeatWidget->setPlaying(true);
    updateSectionTableEnabledState();
}

void MainWindow::setSpeedTrainerUIEnabled(bool enabled) {
    ui->spinBarsPerStep->setEnabled(enabled);
    ui->spinTempoStep->setEnabled(enabled);
    ui->spinMaxTempo->setEnabled(enabled);
}




void MainWindow::onAccentChanged() {
    std::vector<bool> accents(accentChecks.size(), false);
    for (int i = 0; i < accentChecks.size(); ++i) {
        accents[i] = accentChecks[i]->isChecked();
    }
    if (currentSectionIdx >= 0 && currentSectionIdx < (int)currentPreset.sections.size())
        currentPreset.sections[currentSectionIdx].accents = accents;
    metronome.setAccentPattern(accents);
    saveUIToSection(currentSectionIdx, true);

    // --- PATCH: Restart metronome if running when accents are changed ---
    if (metronome.isRunning()) {
        metronome.stop();
        metronome.start();
    }
}

void MainWindow::setupAccentControls(int count) {
    // PATCH: handle compound time accent count!
    int accentCount = count;
    if (currentDenominator == 8 && count % 3 == 0 && count > 3) {
        accentCount = count / 3; // e.g. 6/8 -> 2, 9/8 -> 3, 12/8 -> 4
    }
    
    // Only show accent controls for standard subdivisions
    bool showAccentControls = true;
    if (currentSectionIdx >= 0 && currentSectionIdx < (int)currentPreset.sections.size()) {
        const auto& pattern = currentPreset.sections[currentSectionIdx].subdivisionPattern;
        // Check if this is a custom subdivision (not standard)
        if (pattern.category == SubdivisionCategory::Custom) {
            showAccentControls = false;
        }
    }
    
    // Clear existing controls
    for (QCheckBox* cb : accentChecks) {
        if (ui->accentWidget->layout())
            ui->accentWidget->layout()->removeWidget(cb);
        delete cb;
    }
    accentChecks.clear();

    if (ui->accentWidget->layout()) {
        QLayout* oldLayout = ui->accentWidget->layout();
        QLayoutItem* child;
        while ((child = oldLayout->takeAt(0)) != nullptr) {
            delete child->widget();
            delete child;
        }
        delete oldLayout;
    }

    // Hide accent controls for custom subdivisions
    ui->accentWidget->setVisible(showAccentControls);
    ui->labelAccent->setVisible(showAccentControls);
    
    if (!showAccentControls) {
        return; // Don't create accent controls for custom subdivisions
    }

    QGridLayout* grid = new QGridLayout();
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setSpacing(4);
    ui->accentWidget->setLayout(grid);

    const int maxPerRow = 10;
    for (int i = 0; i < accentCount; ++i) {
        QCheckBox* cb = new QCheckBox(QString::number(i+1), ui->accentWidget);
        accentChecks.append(cb);
        int row = i / maxPerRow;
        int col = i % maxPerRow;
        grid->addWidget(cb, row, col);
        connect(cb, &QCheckBox::checkStateChanged, this, &MainWindow::onAccentChanged);
        cb->setChecked(false); // Always reset to unchecked
    }

    // --- PATCH: Also reset section accent data to all off ---
    if (currentSectionIdx >= 0 && currentSectionIdx < (int)currentPreset.sections.size()) {
        auto& accents = currentPreset.sections[currentSectionIdx].accents;
        accents.resize(accentCount, false);
        std::fill(accents.begin(), accents.end(), false);
    }
}


// --- Polyrhythm grid highlight order logic ---
void MainWindow::setupPolyrhythmGridHighlightOrder(int mainBeats, int polyBeats) {
    int columns = lcm(mainBeats, polyBeats);
    std::vector<std::pair<int, int>> rawEvents; // (column, type)
    for (int i = 0; i < mainBeats; ++i)
        rawEvents.push_back({(i * columns) / mainBeats, 0});
    for (int i = 0; i < polyBeats; ++i)
        rawEvents.push_back({(i * columns) / polyBeats, 1});
    std::sort(rawEvents.begin(), rawEvents.end());
    int lastCol = -1;
    m_polyrhythmGridColumns.clear();
    for (const auto& ev : rawEvents) {
        if (ev.first != lastCol) {
            m_polyrhythmGridColumns.push_back(ev.first);
            lastCol = ev.first;
        }
    }
}

void MainWindow::onSubdivisionImageClicked() {
    bool isCompoundTime = (currentDenominator == 8 && currentNumerator % 3 == 0 && currentNumerator > 3);
    SubdivisionSelectorDialog dlg(this, isCompoundTime, currentNumerator, currentDenominator);
    if (dlg.exec() == QDialog::Accepted) {
        // Change subdivision pattern first
        SubdivisionPattern chosen = dlg.chosenPattern();
        currentPreset.sections[currentSectionIdx].subdivisionPattern = chosen;

        // --- PATCH: Reset accents to all off BEFORE updating metronome ---
        if (currentSectionIdx >= 0 && currentSectionIdx < (int)currentPreset.sections.size()) {
            int accentCount = currentNumerator;
            if (currentDenominator == 8 && currentNumerator % 3 == 0 && currentNumerator > 3)
                accentCount = currentNumerator / 3;
            auto& accents = currentPreset.sections[currentSectionIdx].accents;
            accents.resize(accentCount, false);
            std::fill(accents.begin(), accents.end(), false);

            // Also clear accents in metronome BEFORE subdivision is set
            metronome.setAccentPattern(accents);
        }

        // Now update metronome subdivision
        metronome.setSubdivisionPattern(chosen);

        // Update main subdivision image with atomic rendering
        NoteAssembler assembler;
        NoteAssemblerConfig cfg = configForPattern(chosen);
        cfg.centerVertically = true;
        QPixmap px = assembler.assembleNote(cfg);

        QSize targetSize = ui->labelSubdivisionImage->size();
        QPixmap scaledPx = px.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        if (ui->labelSubdivisionImage)
            ui->labelSubdivisionImage->setPixmap(scaledPx);
        // Update table cell with atomic rendering
        updateSubPolyCell(currentSectionIdx);

        setupAccentControls(currentNumerator);

        // --- Only this section changed ---
        if (ui->obsBeatWidget) {
            QPixmap obsPx = assembleSubdivisionPixmapForOBS(chosen);
            ui->obsBeatWidget->setSubdivisionPixmap(obsPx);
        }

        // --- PATCH: Restart metronome if running ---
        if (metronome.isRunning()) {
            metronome.stop();
            metronome.start();
        }
    }
}

// Add this as a member variable in your MainWindow class (in mainwindow.h):
// bool m_pendingTempoApplied = false;

void MainWindow::onMetronomePulse(AudioPulseEvent ev) {

    if (currentSectionIdx < 0 || currentSectionIdx >= (int)currentPreset.sections.size())
        return;
    MetronomeSection& section = currentPreset.sections[currentSectionIdx];

    int denominator = currentDenominator;
    int numerator = currentNumerator;
    const SubdivisionPattern& currentPattern = metronome.subdivisionPattern();

// --- COUNT-IN HANDLING ---
if (ev.idx < 0) {
    int countInNumber = ev.idx + 1001;
    
    // FIXED: Use compound beats for count-in display in compound time
    int countInBeats = currentNumerator;
    if (currentDenominator == 8 && currentNumerator % 3 == 0 && currentNumerator > 3) {
        countInBeats = currentNumerator / 3;  // Compound time: use dotted quarter beats
    }
    
    ui->btnStartStop->setText(
        QString("Count %1/%2").arg(countInNumber).arg(countInBeats)  // Use countInBeats instead of numerator
    );
    ui->btnStartStop->setStyleSheet("background-color: #440000; color: white;");

    if (ev.accent)
        metronome.playAccent();
    else
        metronome.playClick();

    int beatsPerBar = section.numerator;
    if (section.denominator == 8 && section.numerator % 3 == 0 && section.numerator > 3)
        beatsPerBar = section.numerator / 3;
    m_beatIndicatorWidget->setBeats(beatsPerBar);
    m_beatIndicatorWidget->setSubdivisions(1);
    m_beatIndicatorWidget->setCurrent(countInNumber - 1, 0);
    m_beatIndicatorWidget->setMode(BeatIndicatorMode::Circles);

    if (ui->obsBeatWidget) {
        ui->obsBeatWidget->setPulseOn(true);
        QTimer::singleShot(80, [this]() {
            if (ui->obsBeatWidget) ui->obsBeatWidget->setPulseOn(false);
        });
    }
    return;
}

    // --- PATCH: Transition from count-in to main metronome, enable speed trainer stepping ---
    if (m_speedTrainerCountingIn) {
        m_speedTrainerCountingIn = false;
        m_speedTrainerBarCounter = 0;
        m_pendingTempoApplied = false;
    }

    // --- PATCH: Ensure first main bar after count-in is displayed as "Bar 1" in subdivision mode ---
if (!section.hasPolyrhythm && metronome.isRunning()) {
    if (m_playingBarCounter == 0) {
        // First main bar after count-in or after mode switch
        m_playingBarCounter = 1;
        ui->btnStartStop->setText(QString("Bar 1/%1\nStop").arg(section.numerator));
        ui->btnStartStop->setStyleSheet("background-color: #440000; color: white;");
    }
}

    // --- SUBDIVISION MODE CALCULATIONS (needed for tempo change detection) ---
    int subdivisionsPerBeat = currentPattern.pulses.size();
    if (subdivisionsPerBeat == 0) subdivisionsPerBeat = 1;

    int beatsPerBar = numerator;
    if (denominator == 8 && numerator % 3 == 0 && numerator > 3)
        beatsPerBar = numerator / 3;
    int pulsesPerBar = subdivisionsPerBeat * beatsPerBar;

    int globalPulse = metronome.globalPulseCount() - 1;
    int barIdx = (globalPulse >= 0) ? (globalPulse / pulsesPerBar) : 0;
    int idxInBar = (globalPulse >= 0) ? (globalPulse % pulsesPerBar) : 0;
    int currBeat = (idxInBar / subdivisionsPerBeat);
    int currSub = (idxInBar % subdivisionsPerBeat);



// --- POLYRHYTHM HANDLING ---
if (section.hasPolyrhythm && metronome.isPolyrhythmEnabled()) {
    Polyrhythm poly = metronome.getPolyrhythm();
    int mainBeats = poly.primaryBeats;
    int polyBeats = poly.secondaryBeats;

    // --- PATCH: Only increment bar counter if not just restarted ---
    if (ev.startOfCycle && !m_polyrhythmCycleActive) {
        m_polyrhythmCycleActive = true;

        // If we just switched to polyrhythm mode, don't increment on first cycle
        if (m_polyrhythmJustRestarted) {
            m_polyrhythmJustRestarted = false;
            m_playingBarCounter = 1;
        } else if (m_speedEnabled && !m_speedTrainerCountingIn) {
            // Speed trainer logic for polyrhythm mode
            if (!m_speedTrainerPolyFirstCycle) {
                m_speedTrainerTotalBarCounter++;

                // Check if we need to change tempo BEFORE incrementing display bar
                bool needTempoChange = (m_speedTrainerTotalBarCounter >= m_speedTrainerBarsPerStep &&
                                       m_speedTrainerCurrentTempo < m_speedTrainerMaxTempo);

                if (needTempoChange) {
                    int newTempo = m_speedTrainerCurrentTempo + m_speedTrainerTempoStep;
                    if (newTempo > m_speedTrainerMaxTempo)
                        newTempo = m_speedTrainerMaxTempo;

                    // Apply tempo change
                    m_speedTrainerCurrentTempo = newTempo;

                    bool prevCountInEnabled = metronome.countInEnabled();
                    metronome.setCountInEnabled(false);

                    metronome.stop();
                    metronome.setTempo(m_speedTrainerCurrentTempo);
                    metronome.start();

                    metronome.setCountInEnabled(prevCountInEnabled);

                    QSignalBlocker block(ui->spinTempo);
                    ui->spinTempo->setValue(m_speedTrainerCurrentTempo);
                    ui->sliderTempo->setValue(m_speedTrainerCurrentTempo);
                    if (ui->obsBeatWidget)
                        ui->obsBeatWidget->setTempo(m_speedTrainerCurrentTempo);

                    m_speedTrainerTotalBarCounter = 0; // Reset counter after applying tempo change

                    // Force bar counter to 1 since we just started a new cycle
                    m_playingBarCounter = 1;
                    m_speedTrainerPolyFirstCycle = false; // Reset for next tempo cycle

                } else {
                    // Normal bar increment (speed trainer cycle bars)
                    m_playingBarCounter++;
                    if (m_playingBarCounter > m_speedTrainerBarsPerStep) {
                        m_playingBarCounter = 1; // Wrap around at bars per step limit
                    }
                }
            } else {
                // First cycle - just set to 1, don't increment counters
                m_playingBarCounter = 1;
                m_speedTrainerPolyFirstCycle = false;
            }
        } else {
            // Normal polyrhythm bar increment
            if (m_playingBarCounter < section.numerator)
                m_playingBarCounter++;
            else
                m_playingBarCounter = 1;
        }
    }
    if (!ev.startOfCycle) {
        m_polyrhythmCycleActive = false;
    }

    // Display logic - use speed trainer bars per step if enabled
    if (m_speedEnabled) {
        int displayBar = m_playingBarCounter;
        if (displayBar == 0) displayBar = 1;
        ui->btnStartStop->setText(
            QString("Bar %1/%2\nStop").arg(displayBar).arg(m_speedTrainerBarsPerStep)
        );
        ui->btnStartStop->setStyleSheet("background-color: #440000; color: white;");
    } else {
        // Normal mode - use section numerator
        int maxBars = section.numerator;
        int displayBar = m_playingBarCounter;
        if (displayBar == 0) displayBar = 1;
        ui->btnStartStop->setText(
            QString("Bar %1/%2\nStop").arg(displayBar).arg(maxBars)
        );
        ui->btnStartStop->setStyleSheet("background-color: #440000; color: white;");
    }

    m_beatIndicatorWidget->setPolyrhythmGrid(mainBeats, polyBeats, ev.gridColumn);
    m_beatIndicatorWidget->setMode(BeatIndicatorMode::PolyrhythmGrid);

    if (ev.playPulse) {
        if (ev.polyAccent && ev.accent) {
            metronome.playAccent();
        } else if (ev.polyAccent) {
            metronome.playAccent();
        } else if (ev.accent) {
            metronome.playClick();
        } else {
            metronome.playClick();
        }
    }

    if (ui->obsBeatWidget) {
        ui->obsBeatWidget->setPulseOn(true);
        QTimer::singleShot(80, [this]() {
            if (ui->obsBeatWidget) ui->obsBeatWidget->setPulseOn(false);
        });
    }
    return;
}



// --- UI Bar Counter: Speed trainer aware tracking ---
if (currBeat == 0 && currSub == 0) {
    // Calculate what barIdx WOULD be from audio engine
    int audioBarIdx = (globalPulse >= 0) ? (globalPulse / pulsesPerBar) : 0;
    
    // For speed trainer, maintain our own continuous bar tracking
    // For speed trainer, maintain our own continuous bar tracking
if (m_speedEnabled) {
    // Check if this is a new bar (before updating m_lastBarIdx)
    bool isNewBar = (audioBarIdx != m_lastBarIdx && !m_isSpeedTrainerAutoChange);
    
    // Handle first bar specially
    if (m_lastBarIdx == -1) {
        // Very first bar - initialize
        m_playingBarCounter = 1;
        m_lastBarIdx = audioBarIdx;
    } else if (isNewBar) {
        // Subsequent bars - apply speed trainer logic
        if (!m_speedTrainerCountingIn) {
            m_speedTrainerTotalBarCounter++;

            
            // Check if we need to change tempo BEFORE incrementing display bar
            bool needTempoChange = (m_speedTrainerTotalBarCounter >= m_speedTrainerBarsPerStep && 
                                   m_speedTrainerCurrentTempo < m_speedTrainerMaxTempo);
            
            if (needTempoChange) {
                int newTempo = m_speedTrainerCurrentTempo + m_speedTrainerTempoStep;
                if (newTempo > m_speedTrainerMaxTempo)
                    newTempo = m_speedTrainerMaxTempo;
                
                
                // Apply tempo change
                m_speedTrainerCurrentTempo = newTempo;
                m_isSpeedTrainerAutoChange = true;
                
                bool prevCountInEnabled = metronome.countInEnabled();
                metronome.setCountInEnabled(false);

                metronome.stop();
                metronome.setTempo(m_speedTrainerCurrentTempo);
                metronome.start();

                metronome.setCountInEnabled(prevCountInEnabled);

                QSignalBlocker block(ui->spinTempo);
                ui->spinTempo->setValue(m_speedTrainerCurrentTempo);
                ui->sliderTempo->setValue(m_speedTrainerCurrentTempo);
                if (ui->obsBeatWidget)
                    ui->obsBeatWidget->setTempo(m_speedTrainerCurrentTempo);
                
                m_speedTrainerTotalBarCounter = 0; // Reset counter after applying tempo change
                
                // Force bar counter to 1 since we just started a new cycle
                m_playingBarCounter = 1;
                m_lastBarIdx = 0; // Reset to 0 since metronome restart resets audioBarIdx to 0
                
                m_isSpeedTrainerAutoChange = false;
                
                
                // Skip the normal bar increment logic this time
                goto display_update;
            } else {
                // Normal bar increment (speed trainer cycle bars)
                m_playingBarCounter++;
                if (m_playingBarCounter > m_speedTrainerBarsPerStep) {
                    m_playingBarCounter = 1; // Wrap around at bars per step limit
                }
            }
        } else {
            // Normal bar increment if not in speed trainer counting in
            m_playingBarCounter++;
            if (m_playingBarCounter > m_speedTrainerBarsPerStep) {
                m_playingBarCounter = 1; // Wrap around at bars per step limit
            }
        }
        m_lastBarIdx = audioBarIdx;
        
    }
    
    display_update:
    // ALWAYS update display - show speed trainer cycle progress
    ui->btnStartStop->setText(
        QString("Bar %1/%2\nStop")
            .arg(m_playingBarCounter)
            .arg(m_speedTrainerBarsPerStep)
    );
    ui->btnStartStop->setStyleSheet("background-color: #440000; color: white;");
    } else {
        // Normal mode - use audio engine calculation
        if (audioBarIdx != m_lastBarIdx) {
            int displayBar = (audioBarIdx % section.numerator) + 1;
            m_playingBarCounter = displayBar;
            m_lastBarIdx = audioBarIdx;

            ui->btnStartStop->setText(
                QString("Bar %1/%2\nStop")
                    .arg(displayBar)
                    .arg(section.numerator)
            );
            ui->btnStartStop->setStyleSheet("background-color: #440000; color: white;");
        }
    }
}



    bool userAccent = false;
    if (currSub == 0 && currBeat >= 0 && currBeat < (int)section.accents.size())
        userAccent = section.accents[currBeat];

    beatsPerBar = section.numerator;
    if (section.denominator == 8 && section.numerator % 3 == 0 && section.numerator > 3)
        beatsPerBar = section.numerator / 3;
    m_beatIndicatorWidget->setBeats(beatsPerBar);
    m_beatIndicatorWidget->setSubdivisions(subdivisionsPerBeat);
    m_beatIndicatorWidget->setCurrent(currBeat, currSub);
    m_beatIndicatorWidget->setMode(BeatIndicatorMode::Circles);

    if (ev.playPulse) {
        if (userAccent)
            metronome.playAccent();
        else
            metronome.playClick();
    }
    if (ui->obsBeatWidget) {
        ui->obsBeatWidget->setPulseOn(true);
        QTimer::singleShot(80, [this]() {
            if (ui->obsBeatWidget) ui->obsBeatWidget->setPulseOn(false);
        });
    }
}


void MainWindow::loadSectionToUI(int idx) {
    if (idx < 0 || idx >= (int)currentPreset.sections.size()) return;
    currentSectionIdx = idx;
    const MetronomeSection& s = currentPreset.sections[idx];

    ui->spinTempo->blockSignals(true);
    ui->sliderTempo->blockSignals(true);

    ui->spinTempo->setValue(s.tempo);
    ui->sliderTempo->setValue(s.tempo);

    ui->spinTempo->blockSignals(false);
    ui->sliderTempo->blockSignals(false);

    currentNumerator = s.numerator;
    currentDenominator = s.denominator;
    updateTimeSignatureDisplay();
    ui->labelTempo->setText(getTempoMarkingsForBpm(s.tempo));
    setupAccentControls(s.numerator);

    for (QCheckBox* cb : accentChecks)
        cb->blockSignals(true);

    for (int i = 0; i < accentChecks.size(); ++i)
        accentChecks[i]->setChecked(i < s.accents.size() ? s.accents[i] : false);

    for (QCheckBox* cb : accentChecks)
        cb->blockSignals(false);

    if (!m_speedEnabled || m_speedTrainerCountingIn) {
        metronome.setTempo(s.tempo);
    }
    metronome.setTimeSignature(s.numerator, s.denominator);

    // --- PATCH: Always set pattern to the section's subdivisionPattern ---
    metronome.setSubdivisionPattern(s.subdivisionPattern);

    metronome.setAccentPattern(s.accents);
    metronome.setPolyrhythmEnabled(s.hasPolyrhythm);
    if (s.hasPolyrhythm) {
        metronome.setPolyrhythm(s.polyrhythm.primaryBeats, s.polyrhythm.secondaryBeats);
    }

    // --- PATCH: Reset polyrhythm bar/cycle state ---
    m_playingBarCounter = 0;
    m_polyrhythmCycleActive = false;

    if (metronome.isRunning()) {
        // NEW: Use stop/start for section changes (always reset to bar 1)
        metronome.stop();
        metronome.start();
        m_polyBarCount = 1;
        m_lastPolyrhythmCycleIdx = -1;
        m_playingBarCounter = 1;
        m_polyrhythmCycleActive = false;
        ui->btnStartStop->setText(
            QString("Bar %1/%2\nStop").arg(m_playingBarCounter).arg(s.numerator)
        );
        ui->btnStartStop->setStyleSheet("background-color: #440000; color: white;");
        if (ui->obsBeatWidget) {
            ui->obsBeatWidget->setTempo(s.tempo);
            ui->obsBeatWidget->setPlaying(metronome.isRunning());
        }
    } else {
        if (ui->obsBeatWidget) {
            ui->obsBeatWidget->setTempo(s.tempo);
            ui->obsBeatWidget->setPlaying(false);
        }
    }

    // --- PATCH: Display image for current pattern ---
    NoteAssembler assembler;
    NoteAssemblerConfig cfg = configForPattern(s.subdivisionPattern);
    cfg.centerVertically = true;
    QPixmap px = assembler.assembleNote(cfg);

    QSize targetSize = ui->labelSubdivisionImage->size();
    QPixmap scaledPx = px.scaled(targetSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

    if (ui->labelSubdivisionImage)
        ui->labelSubdivisionImage->setPixmap(scaledPx);

    updateSubPolyCell(idx);

    if (ui->obsBeatWidget) {
        QPixmap obsPx = assembleSubdivisionPixmapForOBS(s.subdivisionPattern);
        ui->obsBeatWidget->setSubdivisionPixmap(obsPx);
    }

    if (s.hasPolyrhythm) {
        m_beatIndicatorWidget->setPolyrhythmGrid(
            s.polyrhythm.primaryBeats,
            s.polyrhythm.secondaryBeats,
            -1
        );
        m_beatIndicatorWidget->setMode(BeatIndicatorMode::PolyrhythmGrid);
    } else {
        int beatsPerBar = s.numerator;
        if (s.denominator == 8 && s.numerator % 3 == 0 && s.numerator > 3)
            beatsPerBar = s.numerator / 3;
        int subdivisions = s.subdivisionPattern.pulses.size();
        if (subdivisions == 0) subdivisions = 1;
        m_beatIndicatorWidget->setBeats(beatsPerBar);
        m_beatIndicatorWidget->setSubdivisions(subdivisions);
        m_beatIndicatorWidget->setCurrent(0, 0);
        m_beatIndicatorWidget->setMode(BeatIndicatorMode::Circles);
    }

    updatePolyrhythmButtonColor();
    updatePolyrhythmUI();
    updateObsWidgetPolyrhythmDisplay();
}


void MainWindow::saveUIToSection(int idx, bool doAutosave) {
    if (idx < 0 || idx >= (int)currentPreset.sections.size()) return;
    MetronomeSection& s = currentPreset.sections[idx];
    s.tempo = ui->spinTempo->value();
    s.numerator = currentNumerator;
    s.denominator = currentDenominator;
    s.accents.resize(accentChecks.size());
    for (int i = 0; i < accentChecks.size(); ++i)
        s.accents[i] = accentChecks[i]->isChecked();
    // subdivisionPattern is set in onSubdivisionImageClicked

    if (doAutosave) {
        presetManager.savePreset(currentPreset);
        presetManager.saveToDisk(presetFile);
    }
}

void MainWindow::saveUIToSection(int idx) {
    saveUIToSection(idx, false);
}

void MainWindow::refreshPresetList() {
    ui->comboPresets->clear();
    QStringList names = presetManager.listPresetNames();
    for (const QString& name : names) {
        if (!name.trimmed().isEmpty())
            ui->comboPresets->addItem(name);
    }
}


void MainWindow::onAddSection()
{
    saveUIToSection(currentSectionIdx, true);

    MetronomeSection s;

    // If there's a currently selected section, copy its settings
    if (currentSectionIdx >= 0 && currentSectionIdx < (int)currentPreset.sections.size()) {
        const MetronomeSection& current = currentPreset.sections[currentSectionIdx];
        s.tempo = current.tempo;
        s.numerator = current.numerator;
        s.denominator = current.denominator;
        s.subdivisionPattern = current.subdivisionPattern;
        s.hasPolyrhythm = current.hasPolyrhythm;
        s.polyrhythm = current.polyrhythm;
        s.accents = current.accents;
    } else {
        // Defaults if nothing selected
        s.tempo = ui->spinTempo->value();
        s.numerator = currentNumerator;
        s.denominator = currentDenominator;
        s.subdivisionPattern = getDefaultSubdivisionPattern();
        s.hasPolyrhythm = false; // Default to subdivision, not poly
        s.polyrhythm.primaryBeats = 3;
        s.polyrhythm.secondaryBeats = 2;
        s.accents.resize(accentChecks.size());
        for (int i = 0; i < accentChecks.size(); ++i)
            s.accents[i] = accentChecks[i]->isChecked();
    }
    s.label = QString("Section %1").arg(currentPreset.sections.size() + 1);

    currentPreset.sections.push_back(s);
    int idx = currentPreset.sections.size() - 1;

    QSignalBlocker blocker(ui->tableSections);

    ui->tableSections->setRowCount(currentPreset.sections.size());

    QTableWidgetItem* labelItem = new QTableWidgetItem(s.label);
    labelItem->setFlags(labelItem->flags() | Qt::ItemIsEditable);
    ui->tableSections->setItem(idx, 0, labelItem);

    QTableWidgetItem* tempoItem = new QTableWidgetItem(QString::number(s.tempo));
    tempoItem->setFlags(tempoItem->flags() & ~Qt::ItemIsEditable);
    tempoItem->setTextAlignment(Qt::AlignCenter);
    ui->tableSections->setItem(idx, 3, tempoItem);

    QTableWidgetItem* timeSigItem = new QTableWidgetItem(QString("%1/%2").arg(s.numerator).arg(s.denominator));
    timeSigItem->setFlags(timeSigItem->flags() & ~Qt::ItemIsEditable);
    timeSigItem->setTextAlignment(Qt::AlignCenter);
    ui->tableSections->setItem(idx, 1, timeSigItem);

    QTableWidgetItem* subpolyItem = new QTableWidgetItem();
    subpolyItem->setFlags(subpolyItem->flags() & ~Qt::ItemIsEditable);
    subpolyItem->setTextAlignment(Qt::AlignCenter);
    ui->tableSections->setItem(idx, 2, subpolyItem);

    // Use atomic rendering for Sub/Poly cell
    updateSubPolyCell(idx);

    loadSectionToUI(idx);
    ui->tableSections->selectRow(idx);

    presetManager.savePreset(currentPreset);
    presetManager.saveToDisk(presetFile);
}

void MainWindow::onRemoveSection() {
    int row = ui->tableSections->currentRow();
    if (row < 0 || row >= (int)currentPreset.sections.size()) return;
    currentPreset.sections.erase(currentPreset.sections.begin() + row);

    QSignalBlocker blocker(ui->tableSections);

    ui->tableSections->removeRow(row);
    int newIdx = (row > 0) ? row-1 : 0;
    if (!currentPreset.sections.empty())
        loadSectionToUI(newIdx);
    else
        onAddSection();

    presetManager.savePreset(currentPreset);
    presetManager.saveToDisk(presetFile);
}

void MainWindow::onSectionSelected(int row, int col) {
    if (!ui->tableSections->isEnabled()) {
        QSignalBlocker blocker(ui->tableSections);
        ui->tableSections->setCurrentCell(currentSectionIdx, 0);
        ui->tableSections->selectRow(currentSectionIdx);
        return;
    }

    saveUIToSection(currentSectionIdx, true);
    if (row < 0 || row >= (int)currentPreset.sections.size()) return;
    loadSectionToUI(row);

    const MetronomeSection& s = currentPreset.sections[row];
    if (metronome.isRunning()) {
        metronome.stop();
        if (!s.hasPolyrhythm) { // subdivision mode
            QTimer::singleShot(100, this, [this, &s]() {
                metronome.start();
                m_polyBarCount = 1;
                m_lastPolyrhythmCycleIdx = -1;
                m_playingBarCounter = 1;
                ui->btnStartStop->setText(
                    QString("Bar %1/%2\nStop").arg(m_playingBarCounter).arg(s.numerator)
                );
                ui->btnStartStop->setStyleSheet("background-color: #440000; color: white;");
                if (ui->obsBeatWidget) {
                    ui->obsBeatWidget->setTempo(s.tempo);
                    ui->obsBeatWidget->setPlaying(metronome.isRunning());
                }
            });
        } else {
            metronome.start();
            m_polyBarCount = 1;
            m_lastPolyrhythmCycleIdx = -1;
            m_playingBarCounter = 1;
            ui->btnStartStop->setText(
                QString("Bar %1/%2\nStop").arg(m_playingBarCounter).arg(s.numerator)
            );
            ui->btnStartStop->setStyleSheet("background-color: #440000; color: white;");
            if (ui->obsBeatWidget) {
                ui->obsBeatWidget->setTempo(s.tempo);
                ui->obsBeatWidget->setPlaying(metronome.isRunning());
            }
        }
    }
}

void MainWindow::onSectionRowMoved(int from, int to) {
    int sectionCount = static_cast<int>(currentPreset.sections.size());
    if (from < 0 || from >= sectionCount || to < 0 || to >= sectionCount || from == to)
        return;

    auto moved = currentPreset.sections[from];
    currentPreset.sections.erase(currentPreset.sections.begin() + from);
    currentPreset.sections.insert(currentPreset.sections.begin() + to, moved);

    QSignalBlocker blocker(ui->tableSections);
    ui->tableSections->setRowCount(0);
    ui->tableSections->setRowCount(currentPreset.sections.size());

    for (int i = 0; i < (int)currentPreset.sections.size(); ++i) {
        const auto& s = currentPreset.sections[i];
        auto* labelItem = new QTableWidgetItem(s.label);
        labelItem->setFlags(labelItem->flags() | Qt::ItemIsEditable);
        ui->tableSections->setItem(i, 0, labelItem);

        auto* tempoItem = new QTableWidgetItem(QString::number(s.tempo));
        tempoItem->setFlags(tempoItem->flags() & ~Qt::ItemIsEditable);
        tempoItem->setTextAlignment(Qt::AlignCenter);
        ui->tableSections->setItem(i, 3, tempoItem);

        auto* timeSigItem = new QTableWidgetItem(QString("%1/%2").arg(s.numerator).arg(s.denominator));
        timeSigItem->setFlags(timeSigItem->flags() & ~Qt::ItemIsEditable);
        timeSigItem->setTextAlignment(Qt::AlignCenter);
        ui->tableSections->setItem(i, 1, timeSigItem);

        auto* subpolyItem = new QTableWidgetItem();
        subpolyItem->setFlags(subpolyItem->flags() & ~Qt::ItemIsEditable);
        subpolyItem->setTextAlignment(Qt::AlignCenter);
        ui->tableSections->setItem(i, 2, subpolyItem);

        updateSubPolyCell(i);
    }

    presetManager.savePreset(currentPreset);
    presetManager.saveToDisk(presetFile);

    ui->tableSections->selectRow(to);
    loadSectionToUI(to);

    const MetronomeSection& s = currentPreset.sections[to];
    if (metronome.isRunning()) {
        metronome.stop();
        if (!s.hasPolyrhythm) {
            QTimer::singleShot(100, this, [this, &s]() {
                metronome.start();
                m_polyBarCount = 1;
                m_lastPolyrhythmCycleIdx = -1;
                m_playingBarCounter = 1;
                ui->btnStartStop->setText(
                    QString("Bar %1/%2\nStop").arg(m_playingBarCounter).arg(s.numerator)
                );
                ui->btnStartStop->setStyleSheet("background-color: #440000; color: white;");
                if (ui->obsBeatWidget) {
                    ui->obsBeatWidget->setTempo(s.tempo);
                    ui->obsBeatWidget->setPlaying(metronome.isRunning());
                }
            });
        } else {
            metronome.start();
            m_polyBarCount = 1;
            m_lastPolyrhythmCycleIdx = -1;
            m_playingBarCounter = 1;
            ui->btnStartStop->setText(
                QString("Bar %1/%2\nStop").arg(m_playingBarCounter).arg(s.numerator)
            );
            ui->btnStartStop->setStyleSheet("background-color: #440000; color: white;");
            if (ui->obsBeatWidget) {
                ui->obsBeatWidget->setTempo(s.tempo);
                ui->obsBeatWidget->setPlaying(metronome.isRunning());
            }
        }
    }
}


void MainWindow::updateSubPolyCell(int sectionIdx) {
    if (sectionIdx < 0 || sectionIdx >= (int)currentPreset.sections.size()) return;
    const auto& s = currentPreset.sections[sectionIdx];

    // Remove any previous widget
    QWidget* oldWidget = ui->tableSections->cellWidget(sectionIdx, 2);
    if (oldWidget) {
        ui->tableSections->removeCellWidget(sectionIdx, 2);
        delete oldWidget;
    }

    if (s.hasPolyrhythm) {
        QTableWidgetItem* item = ui->tableSections->item(sectionIdx, 2);
        if (!item) {
            item = new QTableWidgetItem();
            ui->tableSections->setItem(sectionIdx, 2, item);
        }
        item->setText(QString("%1/%2").arg(s.polyrhythm.primaryBeats).arg(s.polyrhythm.secondaryBeats));
        item->setIcon(QIcon());
        item->setToolTip("Polyrhythm: " + QString("%1 over %2").arg(s.polyrhythm.primaryBeats).arg(s.polyrhythm.secondaryBeats));
        item->setTextAlignment(Qt::AlignCenter);
    } else {
        NoteAssembler assembler;
        NoteAssemblerConfig cfg = configForPattern(s.subdivisionPattern);
        QPixmap p = assembler.assembleNote(cfg);
        QSize subPolyIconSize(60, 30); // Fixed icon size

        QPixmap scaledP = p.scaled(subPolyIconSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        QLabel* iconLabel = new QLabel();
        iconLabel->setPixmap(scaledP);
        iconLabel->setAlignment(Qt::AlignCenter);

        // Critical: Use expanding size policy so the widget fits the cell
        iconLabel->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

        iconLabel->setContentsMargins(0, 0, 0, 0);

        iconLabel->setToolTip(s.subdivisionPattern.name);

        ui->tableSections->setCellWidget(sectionIdx, 2, iconLabel);

        // Ensure column width and icon size are appropriate
        ui->tableSections->setColumnWidth(2, subPolyIconSize.width());
        ui->tableSections->setIconSize(subPolyIconSize);

        // Remove text/icon from item (optional, for clarity)
        QTableWidgetItem* item = ui->tableSections->item(sectionIdx, 2);
        if (item) {
            item->setText("");
            item->setIcon(QIcon());
        }
    }
}



void MainWindow::onSavePreset() {
    QString name;
    while (true) {
        bool ok = false;
        name = QInputDialog::getText(this, "Save Preset (Piece)", "Piece name:", QLineEdit::Normal, "", &ok);
        if (!ok) return;
        if (name.trimmed().isEmpty()) {
            QMessageBox::information(this, "No Text Entered", "Enter text to save.");
            continue;
        }
        // Check for duplicate preset name
        if (presetManager.listPresetNames().contains(name)) {
            QMessageBox::warning(this, "Duplicate Name", "A preset with that name already exists. Please choose a different name.");
            continue;
        }
        break;
    }

    currentPreset.songName = name;
    currentPreset.sections.clear();

    // Create a default section
    MetronomeSection s;
    s.label = "Section 1";
    s.tempo = 120;
    s.numerator = 4;
    s.denominator = 4;
    s.subdivisionPattern = getDefaultSubdivisionPattern();
    s.accents = std::vector<bool>(4, false);
    s.accents[0] = true;
    s.hasPolyrhythm = false;
    s.polyrhythm.primaryBeats = 3;
    s.polyrhythm.secondaryBeats = 2;
    currentPreset.sections.push_back(s);

    QSignalBlocker blocker(ui->tableSections);
    ui->tableSections->setRowCount(1);

    QTableWidgetItem* labelItem = new QTableWidgetItem(s.label);
    labelItem->setFlags(labelItem->flags() | Qt::ItemIsEditable);
    ui->tableSections->setItem(0, 0, labelItem);

    QTableWidgetItem* tempoItem = new QTableWidgetItem(QString::number(s.tempo));
    tempoItem->setFlags(tempoItem->flags() & ~Qt::ItemIsEditable);
    tempoItem->setTextAlignment(Qt::AlignCenter);
    ui->tableSections->setItem(0, 3, tempoItem);

    QTableWidgetItem* timeSigItem = new QTableWidgetItem("4/4");
    timeSigItem->setFlags(timeSigItem->flags() & ~Qt::ItemIsEditable);
    timeSigItem->setTextAlignment(Qt::AlignCenter);
    ui->tableSections->setItem(0, 1, timeSigItem);

    // --- Sub/Poly column logic ---
    QTableWidgetItem* subpolyItem = new QTableWidgetItem();
    if (s.hasPolyrhythm) {
        subpolyItem->setText(QString("%1/%2").arg(s.polyrhythm.primaryBeats).arg(s.polyrhythm.secondaryBeats));
        subpolyItem->setTextAlignment(Qt::AlignCenter);
        subpolyItem->setToolTip("Polyrhythm: " + QString("%1 over %2").arg(s.polyrhythm.primaryBeats).arg(s.polyrhythm.secondaryBeats));
    } else {
        NoteAssembler assembler;
        NoteAssemblerConfig cfg = configForPattern(s.subdivisionPattern);
        QPixmap p = assembler.assembleNote(cfg);
        subpolyItem->setIcon(QIcon(p));
        subpolyItem->setToolTip(s.subdivisionPattern.name);
        subpolyItem->setText("");
    }
    subpolyItem->setFlags(subpolyItem->flags() & ~Qt::ItemIsEditable);
    subpolyItem->setTextAlignment(Qt::AlignCenter);
    ui->tableSections->setItem(0, 2, subpolyItem);

    currentSectionIdx = 0;
    loadSectionToUI(0);

    presetManager.savePreset(currentPreset);
    presetManager.saveToDisk(presetFile);
    refreshPresetList();

    int idx = ui->comboPresets->findText(name);
    if (idx >= 0) ui->comboPresets->setCurrentIndex(idx);
}

void MainWindow::onLoadPreset() {
    QSignalBlocker blocker(ui->tableSections);
    QString name = ui->comboPresets->currentText();
    MetronomePreset p;
    if (!presetManager.loadPreset(name, p)) return;
    currentPreset = p;
    ui->tableSections->setRowCount(currentPreset.sections.size());
    for (int i = 0; i < (int)currentPreset.sections.size(); ++i) {
        QTableWidgetItem* labelItem = new QTableWidgetItem(currentPreset.sections[i].label);
        labelItem->setFlags(labelItem->flags() | Qt::ItemIsEditable);
        ui->tableSections->setItem(i, 0, labelItem);

        QTableWidgetItem* tempoItem = new QTableWidgetItem(QString::number(currentPreset.sections[i].tempo));
        tempoItem->setFlags(tempoItem->flags() & ~Qt::ItemIsEditable);
        tempoItem->setTextAlignment(Qt::AlignCenter);
        ui->tableSections->setItem(i, 3, tempoItem);

        QTableWidgetItem* timeSigItem = new QTableWidgetItem(QString("%1/%2").arg(currentPreset.sections[i].numerator).arg(currentPreset.sections[i].denominator));
        timeSigItem->setFlags(timeSigItem->flags() & ~Qt::ItemIsEditable);
        timeSigItem->setTextAlignment(Qt::AlignCenter);
        ui->tableSections->setItem(i, 1, timeSigItem);

        QTableWidgetItem* subpolyItem = new QTableWidgetItem();
        subpolyItem->setFlags(subpolyItem->flags() & ~Qt::ItemIsEditable);
        subpolyItem->setTextAlignment(Qt::AlignCenter);
        ui->tableSections->setItem(i, 2, subpolyItem);

        // Use atomic rendering for Sub/Poly cell
        updateSubPolyCell(i);
    }
    if (!currentPreset.sections.empty()) {
        ui->tableSections->selectRow(0);
        currentSectionIdx = 0;
        currentNumerator = currentPreset.sections[0].numerator;
        currentDenominator = currentPreset.sections[0].denominator;
        loadSectionToUI(0);
        updateTimeSignatureDisplay();
    } else {
        onAddSection();
    }
}

void MainWindow::onSaveSection() {
    saveUIToSection(currentSectionIdx, true);
    QSignalBlocker blocker(ui->tableSections);

    if (currentSectionIdx >= 0 && currentSectionIdx < (int)currentPreset.sections.size()) {
        const MetronomeSection& s = currentPreset.sections[currentSectionIdx];

        QTableWidgetItem* labelItem = new QTableWidgetItem(s.label);
        labelItem->setFlags(labelItem->flags() | Qt::ItemIsEditable);
        ui->tableSections->setItem(currentSectionIdx, 0, labelItem);

        QTableWidgetItem* tempoItem = new QTableWidgetItem(QString::number(s.tempo));
        tempoItem->setFlags(tempoItem->flags() & ~Qt::ItemIsEditable);
        tempoItem->setTextAlignment(Qt::AlignCenter);
        ui->tableSections->setItem(currentSectionIdx, 3, tempoItem);

        QTableWidgetItem* timeSigItem = new QTableWidgetItem(QString("%1/%2").arg(s.numerator).arg(s.denominator));
        timeSigItem->setFlags(timeSigItem->flags() & ~Qt::ItemIsEditable);
        timeSigItem->setTextAlignment(Qt::AlignCenter);
        ui->tableSections->setItem(currentSectionIdx, 1, timeSigItem);

        QTableWidgetItem* subpolyItem = new QTableWidgetItem();
        subpolyItem->setFlags(subpolyItem->flags() & ~Qt::ItemIsEditable);
        subpolyItem->setTextAlignment(Qt::AlignCenter);
        ui->tableSections->setItem(currentSectionIdx, 2, subpolyItem);

        // Use atomic rendering for Sub/Poly cell
        updateSubPolyCell(currentSectionIdx);
    }

    presetManager.savePreset(currentPreset);
    presetManager.saveToDisk(presetFile);
}

void MainWindow::onDeletePreset() {
    QString name = ui->comboPresets->currentText();
    if (name.isEmpty()) return;

    auto reply = QMessageBox::question(
        this, "Delete Preset",
        QString("Are you sure you want to delete \"%1\"? This cannot be undone.").arg(name),
        QMessageBox::Yes | QMessageBox::No
    );
    if (reply != QMessageBox::Yes)
        return;

    presetManager.removePreset(name);
    presetManager.saveToDisk(presetFile);
    refreshPresetList();

    if (ui->comboPresets->count() > 0) {
        onLoadPreset();
    } else {
        currentPreset.songName = "";
        currentPreset.sections.clear();
        onAddSection();
    }
}







void MainWindow::onStartStop() {
    m_speedTrainerBarsPerStep = ui->spinBarsPerStep->value();
    m_speedTrainerTempoStep   = ui->spinTempoStep->value();
    m_speedTrainerMaxTempo    = ui->spinMaxTempo->value();

    // --- PATCH: Reset bar guard for subdivision counting ---
    m_lastBarIdx = -1; // Ensure first bar always starts at 1

    // NEW PATCH: Always reset total speed trainer bar counter
    m_speedTrainerTotalBarCounter = 0;

    // STOP LOGIC
    if (metronome.isRunning() || m_speedTrainerCountingIn) {
        m_countInBeatsLeft = 0;
        m_speedTrainerCountingIn = false;
        m_speedTrainerCurrentBar = 1;
        m_speedTrainerCurrentTempo = m_speedTrainerStartTempo;
        m_speedTrainerBarCounter = 0;
        m_playingBarCounter = 0;
        m_polyrhythmCycleActive = false;
        m_speedTrainerPolyFirstCycle = true; // <-- Add this line
        ui->btnStartStop->setText("Start");
        ui->btnStartStop->setStyleSheet("background-color: #004100; color: white;");
        timer->stop();
        if (m_countInTimer && m_countInTimer->isActive()) m_countInTimer->stop();
        ui->timeEditDuration->setReadOnly(false);
        if (m_timerWasRunning) ui->timeEditDuration->setTime(m_lastEnteredTimerValue);
        m_timerWasRunning = false;
        metronome.stop();
        if (ui->obsBeatWidget) ui->obsBeatWidget->setPlaying(false);
        m_countInBar = 0;
        m_countInBarTotal = 1;
        // --- Reset delayed subdivision switch state ---
        m_switchSubdivisionAfterCountIn = false;
        m_nextSubdivisionPattern = SubdivisionPattern();
        if (m_speedEnabled && currentSectionIdx >= 0 && currentSectionIdx < (int)currentPreset.sections.size()) {
            int restoreTempo = currentPreset.sections[currentSectionIdx].tempo;
            m_speedTrainerStartTempo = restoreTempo;
            m_speedTrainerCurrentTempo = restoreTempo;
            ui->spinTempo->setValue(restoreTempo);
            ui->sliderTempo->setValue(restoreTempo);
            metronome.setTempo(restoreTempo);
            if (ui->obsBeatWidget)
                ui->obsBeatWidget->setTempo(restoreTempo);
        }
        updateSectionTableEnabledState();
        return;
    }

    // --- If timer is set, start it (move this BEFORE all return statements) ---
    QTime t = ui->timeEditDuration->time();
    if (m_timerEnabled && t != QTime(0, 0, 0)) {
        m_lastEnteredTimerValue = t;
        timer->start();
        ui->timeEditDuration->setReadOnly(true);
        m_timerWasRunning = true;
    } else {
        m_timerWasRunning = false;
    }

    // --- PROACTIVELY DISABLE TABLE BEFORE STARTING COUNT-IN OR METRONOME ---
    if (m_speedEnabled) {
        ui->tableSections->setEnabled(false);
    }

    // --- If count-in is enabled, always do count-in first ---
    if (m_countInEnabled) {
        m_speedTrainerCountingIn = true;
        startCountIn();
        // Section table stays disabled through count-in
        updateSectionTableEnabledState();
        return;
    }

    // --- If Speed Trainer is enabled, use speed trainer params ---
    if (m_speedEnabled) {
        m_speedTrainerBarsPerStep = ui->spinBarsPerStep->value();
        m_speedTrainerTempoStep = ui->spinTempoStep->value();
        m_speedTrainerMaxTempo = ui->spinMaxTempo->value();
        m_speedTrainerCountInBeats = currentNumerator;
        m_speedTrainerCurrentBar = 1;
        m_speedTrainerBarCounter = 0;
        m_playingBarCounter = 1; // PATCH: Force first bar to 1 for UI
        m_lastBarIdx = 0; // <-- Reset here too for safety
        m_speedTrainerPolyFirstCycle = true;
        // --- PATCH START: Always set start tempo from section, not just UI ---
       m_speedTrainerStartTempo = ui->spinTempo->value();
        // --- PATCH END ---
        m_speedTrainerCurrentTempo = m_speedTrainerStartTempo;
        m_speedTrainerPolyrhythm = metronome.isPolyrhythmEnabled();
        m_speedTrainerFirstCycle = true;
        // NEW PATCH: Always reset total speed trainer bar counter on START
        m_speedTrainerTotalBarCounter = 0;
        ui->spinTempo->setValue(m_speedTrainerStartTempo);
        metronome.setTempo(m_speedTrainerStartTempo);
        ui->btnStartStop->setText(
            QString("Bar 1/%1\nStop").arg(currentNumerator)
        );
        ui->btnStartStop->setStyleSheet("background-color: #440000; color: white;");
        metronome.start();
        if (ui->obsBeatWidget) ui->obsBeatWidget->setPlaying(true);
        m_countInBar = 0;
        m_countInBarTotal = 1;
        updateSectionTableEnabledState(); // Defensive, but table should already be disabled

        return;
    }

    // --- Otherwise, just start the metronome normally ---
    m_playingBarCounter = 0; // PATCH: Force first bar to 1 for UI
    m_lastBarIdx = 0; // <-- Reset here for normal mode as well
    m_polyrhythmCycleActive = false;
    ui->btnStartStop->setText(
        QString("Bar 1/%1\nStop")
            .arg(currentNumerator)
    );
    ui->btnStartStop->setStyleSheet("background-color: #440000; color: white;");
    metronome.start();
    if (ui->obsBeatWidget) ui->obsBeatWidget->setPlaying(true);
    m_countInBar = 0;
    m_countInBarTotal = 1;
    updateSectionTableEnabledState();
}

void MainWindow::onTempoChanged(int value) {
    QSignalBlocker block1(ui->spinTempo);
    QSignalBlocker block2(ui->sliderTempo);

    if (ui->sliderTempo->value() != value)
        ui->sliderTempo->setValue(value);
    if (ui->spinTempo->value() != value)
        ui->spinTempo->setValue(value);

    // --- PATCH: Allow tempo change to propagate if metronome is stopped ---
    if (!m_speedEnabled || m_speedTrainerCountingIn || !metronome.isRunning()) {
        if (currentSectionIdx >= 0 && currentSectionIdx < (int)currentPreset.sections.size())
            currentPreset.sections[currentSectionIdx].tempo = value;

        // Only update the metronome's tempo if speed trainer is NOT active or metronome is stopped
        if (metronome.isPolyrhythmEnabled() && metronome.isRunning()) {
            metronome.armTempo(value);
            // Do NOT call setTempo now
        } else {
            // NEW: Use stop/start for user-initiated changes (but reset bar counter)
            if (metronome.isRunning() && !m_isSpeedTrainerAutoChange) {
                metronome.stop();
                metronome.setTempo(value);
                metronome.start();
                // Reset to bar 1 for user changes
                m_playingBarCounter = 0;
                m_lastBarIdx = 0;
            } else if (!metronome.isRunning()) {
                metronome.setTempo(value); // subdivision or stopped
            }
        }
    }
    // If speed trainer is running and metronome is running, do NOT call setTempo here!

    if (currentSectionIdx >= 0 && currentSectionIdx < ui->tableSections->rowCount()) {
        QTableWidgetItem* tempoItem = ui->tableSections->item(currentSectionIdx, 3);
        if (tempoItem) tempoItem->setText(QString::number(value));
    }
    if (ui->obsBeatWidget) ui->obsBeatWidget->setTempo(value);
    ui->labelTempo->setText(getTempoMarkingsForBpm(value));
}

void MainWindow::onRenamePreset() {
    QString oldName = ui->comboPresets->currentText();
    if (oldName.isEmpty()) return;

    QString newName = QInputDialog::getText(this, "Rename Preset (Song)", "New song name:", QLineEdit::Normal, oldName);
    if (newName.isEmpty() || newName == oldName) return;

    if (presetManager.listPresetNames().contains(newName)) {
        QMessageBox::warning(this, "Rename Preset", "A preset with that name already exists.");
        return;
    }

    currentPreset.songName = newName;

    presetManager.removePreset(oldName);
    presetManager.savePreset(currentPreset);
    presetManager.saveToDisk(presetFile);

    refreshPresetList();

    int idx = ui->comboPresets->findText(newName);
    if (idx >= 0) ui->comboPresets->setCurrentIndex(idx);
}

void MainWindow::onMoveSectionUp() {
    int row = ui->tableSections->currentRow();
    if (row <= 0 || row >= (int)currentPreset.sections.size()) return;
    std::swap(currentPreset.sections[row-1], currentPreset.sections[row]);
    onLoadPreset();
    ui->tableSections->selectRow(row-1);

    presetManager.savePreset(currentPreset);
    presetManager.saveToDisk(presetFile);
}

void MainWindow::onMoveSectionDown() {
    int row = ui->tableSections->currentRow();
    if (row < 0 || row+1 >= (int)currentPreset.sections.size()) return;
    std::swap(currentPreset.sections[row+1], currentPreset.sections[row]);
    onLoadPreset();
    ui->tableSections->selectRow(row+1);

    presetManager.savePreset(currentPreset);
    presetManager.saveToDisk(presetFile);
}

void MainWindow::onTapTempo() {
    const int tapResetMs = 2000;
    qint64 now = QDateTime::currentMSecsSinceEpoch();

    // Stop metronome on first tap if running
    if (tapTimes.isEmpty() || (now - tapTimes.last()) > tapResetMs) {
        if (metronome.isRunning()) {
            m_metronomeWasRunning = true;
            metronome.stop();
        } else {
            m_metronomeWasRunning = false;
        }
        tapTimes.clear();
    }

    tapTimes.append(now);

    if (tapTimes.size() >= 2) {
        QList<qint64> intervals;
        for (int i = 1; i < tapTimes.size(); ++i)
            intervals.append(tapTimes[i] - tapTimes[i - 1]);
        double avgInterval = std::accumulate(intervals.begin(), intervals.end(), 0.0) / intervals.size();
        int bpm = static_cast<int>(60000.0 / avgInterval + 0.5);
        if (bpm < 20) bpm = 20;
        if (bpm > 300) bpm = 300;
        ui->spinTempo->setValue(bpm);
    }
    if (tapTimes.size() > 8)
        tapTimes.removeFirst();

    // Each tap restarts the timeout; resume after tapResetMs of inactivity
    m_tapTempoResumeTimer->start(tapResetMs);
}

void MainWindow::onSpeedToggle() {
    m_speedEnabled = !m_speedEnabled;
    updateSpeedUI();

    setSpeedTrainerUIEnabled(m_speedEnabled);

    if (!m_speedEnabled) {
        resetSpeedTrainer();
    }

    if (metronome.isRunning()) {
        onStartStop();
    } else if (!m_speedEnabled) {
        // Only reset the Start/Stop button if we're disabling and not running
        ui->btnStartStop->setText("Start");
        ui->btnStartStop->setStyleSheet("background-color: #004100; color: white;");
    }

    updateSectionTableEnabledState();
}

void MainWindow::onTempoSliderChanged(int value) {
    // No-op or sync UI logic (your version may have more, but this at least compiles)
}

void MainWindow::onSpinTempoEditingFinished() {
    if (m_metronomeWasRunning) {
        metronome.start();
        m_metronomeWasRunning = false;
    }
}

void MainWindow::onTempoSliderReleased() {
    saveUIToSection(currentSectionIdx, true); // <-- Only save when user is done dragging
    if (m_metronomeWasRunning) {
        metronome.start();
        m_metronomeWasRunning = false;
    }
}

void MainWindow::onTempoSliderPressed() {
    if (metronome.isRunning()) {
        m_metronomeWasRunning = true;
        metronome.stop();
    } else {
        m_metronomeWasRunning = false;
    }
}

void MainWindow::onTimerToggle() {
    m_timerEnabled = !m_timerEnabled;
    updateTimerUI();
}

void MainWindow::resetSpeedTrainer() {
    m_speedTrainerCountingIn = false;
    m_speedTrainerCurrentBar = 1;
    m_speedTrainerBarCounter = 0; // <--- ensure speed trainer bar resets
    m_playingBarCounter = 0;      // <--- ensure UI bar resets
    m_speedTrainerCurrentTempo = m_speedTrainerStartTempo;
    m_speedTrainerFirstCycle = true; // Reset first cycle flag
}

void MainWindow::updateTimerUI() {
    QColor color = m_timerEnabled ? m_accentColor : m_accentColor.darker(200);
    ui->btnTimer->setStyleSheet(QString("background-color: %1; color: white;").arg(color.name()));
    ui->timeEditDuration->setVisible(m_timerEnabled);
    ui->labelTimerRemaining->setVisible(m_timerEnabled);
}

void MainWindow::updateSpeedUI() {
    QColor color = m_speedEnabled ? m_accentColor : m_accentColor.darker(200);
    ui->btnSpeed->setStyleSheet(QString("background-color: %1; color: white;").arg(color.name()));
    ui->speedTrainerWidget->setVisible(m_speedEnabled);
}

void MainWindow::updateSpeedTrainerStatus() {
    int shownBar = m_speedTrainerBarCounter + 1;
    int maxBars = m_speedTrainerBarsPerStep;
    ui->btnStartStop->setText(
        QString("Bar %1/%2\nStop")
            .arg(shownBar)
            .arg(maxBars)
    );
    ui->btnStartStop->setStyleSheet("background-color: #440000; color: white;");
    updateSectionTableEnabledState();
}

void MainWindow::updateSectionTableEnabledState() {
    bool disableTrainerControls = (m_speedEnabled && (metronome.isRunning() || m_speedTrainerCountingIn));

    ui->tableSections->setEnabled(!disableTrainerControls);

    // Disable time signature
    ui->labelNumerator->setEnabled(!disableTrainerControls);
    ui->labelDenominator->setEnabled(!disableTrainerControls);
    if (m_timeSigBtn)
        m_timeSigBtn->setEnabled(!disableTrainerControls);

    // Disable subdivision image
    ui->labelSubdivisionImage->setEnabled(!disableTrainerControls);

    // Disable tempo controls
    ui->spinTempo->setEnabled(!disableTrainerControls);
    ui->sliderTempo->setEnabled(!disableTrainerControls);

    // Disable polyrhythm selector
    if (m_polyrhythmNumberWidget)
        m_polyrhythmNumberWidget->setEnabled(!disableTrainerControls);

    // --- PATCH: Disable accent controls ---
    ui->accentWidget->setEnabled(!disableTrainerControls);
    ui->labelAccent->setEnabled(!disableTrainerControls);
    for (QCheckBox* cb : accentChecks) {
        cb->setEnabled(!disableTrainerControls);
    }
}

void MainWindow::handleSpeedTrainerBarEnd() {
    if (metronome.isPolyrhythmEnabled()) {
        m_speedTrainerBarCounter++;
        if (m_speedTrainerBarCounter >= m_speedTrainerBarsPerStep) {
            if (m_speedTrainerCurrentTempo < m_speedTrainerMaxTempo) {
                int newTempo = m_speedTrainerCurrentTempo + m_speedTrainerTempoStep;
                if (newTempo > m_speedTrainerMaxTempo)
                    newTempo = m_speedTrainerMaxTempo;
                m_speedTrainerPendingTempo = newTempo; // <-- ARM tempo
            }
            m_speedTrainerBarCounter = 0;
        }
    } else {
        // --- PATCH: Check first, then increment for correct step timing ---
        if (m_speedTrainerBarCounter + 1 >= m_speedTrainerBarsPerStep) {
            if (m_speedTrainerFirstCycle) {
                m_speedTrainerFirstCycle = false;
            } else {
                if (m_speedTrainerCurrentTempo < m_speedTrainerMaxTempo) {
                    int newTempo = m_speedTrainerCurrentTempo + m_speedTrainerTempoStep;
                    if (newTempo > m_speedTrainerMaxTempo)
                        newTempo = m_speedTrainerMaxTempo;
                    m_speedTrainerPendingTempo = newTempo; // <-- ARM tempo
                }
            }
            m_speedTrainerBarCounter = 0;
        } else {
            m_speedTrainerBarCounter++;
        }
        updateSpeedTrainerStatus();
    }
}









// --- POLYRHYTHM/PATTERN UI AND SUPPORT ---

void MainWindow::updatePolyrhythmUI() {
    bool enabled = false;
    int mainBeats = 3, polyBeats = 2;
    bool isCustomSubdivision = false;
    
    if (currentSectionIdx >= 0 && currentSectionIdx < (int)currentPreset.sections.size()) {
        enabled = currentPreset.sections[currentSectionIdx].hasPolyrhythm;
        mainBeats = currentPreset.sections[currentSectionIdx].polyrhythm.primaryBeats;
        polyBeats = currentPreset.sections[currentSectionIdx].polyrhythm.secondaryBeats;
        
        // Check if current subdivision is custom
        const auto& pattern = currentPreset.sections[currentSectionIdx].subdivisionPattern;
        isCustomSubdivision = (pattern.category == SubdivisionCategory::Custom);
    }
    
    if (enabled) {
        ui->labelSubdivision->setText("Polyrhythm");
        ui->labelSubdivisionImage->hide();
        m_polyrhythmNumberWidget->setVisible(true);
        m_labelPolyrhythmNumerator->setText(QString::number(mainBeats));
        m_labelPolyrhythmDenominator->setText(QString::number(polyBeats));
        ui->accentWidget->setVisible(false);      // hide checkboxes
        ui->labelAccent->setVisible(false);      // hide "Accents" label
    } else {
        ui->labelSubdivision->setText("Subdivision");
        ui->labelSubdivisionImage->show();
        m_polyrhythmNumberWidget->setVisible(false);
        
        // Only show accent controls for standard subdivisions, not custom ones
        bool showAccentControls = !isCustomSubdivision;
        ui->accentWidget->setVisible(showAccentControls);
        ui->labelAccent->setVisible(showAccentControls);
    }
}

void MainWindow::onPolyrhythmClicked() {
    // Prevent toggling if speed trainer is on AND metronome/count-in is running
    if (m_speedEnabled && (metronome.isRunning() || m_speedTrainerCountingIn)) {
        QMessageBox::information(this, "Action Blocked", 
            "You cannot toggle polyrhythm/subdivision while Speed Trainer is running.");
        return;
    }

    if (currentSectionIdx < 0 || currentSectionIdx >= (int)currentPreset.sections.size()) return;
    MetronomeSection& s = currentPreset.sections[currentSectionIdx];
    s.hasPolyrhythm = !s.hasPolyrhythm;
    metronome.setPolyrhythmEnabled(s.hasPolyrhythm);
    if (s.hasPolyrhythm) {
        metronome.setPolyrhythm(s.polyrhythm.primaryBeats, s.polyrhythm.secondaryBeats);
    }
    saveUIToSection(currentSectionIdx, true);
    updatePolyrhythmButtonColor();
    updatePolyrhythmUI();
    updateObsWidgetPolyrhythmDisplay();
    updateSubPolyCell(currentSectionIdx);

    if (s.hasPolyrhythm) {
        m_beatIndicatorWidget->setPolyrhythmGrid(
            s.polyrhythm.primaryBeats,
            s.polyrhythm.secondaryBeats,
            -1
        );
        m_beatIndicatorWidget->setMode(BeatIndicatorMode::PolyrhythmGrid);
    } else {
        int beatsPerBar = s.numerator;
        if (s.denominator == 8 && s.numerator % 3 == 0 && s.numerator > 3)
            beatsPerBar = s.numerator / 3;
        int subdivisions = s.subdivisionPattern.pulses.size();
        if (subdivisions == 0) subdivisions = 1;
        m_beatIndicatorWidget->setBeats(beatsPerBar);
        m_beatIndicatorWidget->setSubdivisions(subdivisions);
        m_beatIndicatorWidget->setCurrent(0, 0);
        m_beatIndicatorWidget->setMode(BeatIndicatorMode::Circles);
    }

    // --- PATCH: Always restart metronome and reset bar counter/state ---
if (metronome.isRunning()) {
    // Reset bar/cycle state for both modes
    m_playingBarCounter = 0;
    m_polyrhythmCycleActive = false;
    m_polyBarCount = 1;
    m_lastPolyrhythmCycleIdx = -1;
    m_lastBarIdx = 0;
    m_polyrhythmJustRestarted = true; // <--- ADD THIS

    metronome.stop();
    metronome.start();

    int maxBars = s.hasPolyrhythm ? s.polyrhythm.primaryBeats : s.numerator;
    ui->btnStartStop->setText(QString("Bar %1/%2\nStop").arg(m_playingBarCounter).arg(maxBars));
    ui->btnStartStop->setStyleSheet("background-color: #440000; color: white;");
}
}

void MainWindow::onPolyrhythmNumberClicked() {
    showPolyrhythmNumberDialog();
}

void MainWindow::showPolyrhythmNumberDialog() {
    if (currentSectionIdx < 0 || currentSectionIdx >= (int)currentPreset.sections.size()) return;
    int mainB = currentPreset.sections[currentSectionIdx].polyrhythm.primaryBeats;
    int polyB = currentPreset.sections[currentSectionIdx].polyrhythm.secondaryBeats;
    PolyrhythmNumberDialog dlg(mainB, polyB, this);
    if (dlg.exec() == QDialog::Accepted) {
        int pb = dlg.primaryBeats();
        int sb = dlg.secondaryBeats();
        MetronomeSection& section = currentPreset.sections[currentSectionIdx];
        section.polyrhythm.primaryBeats = pb;
        section.polyrhythm.secondaryBeats = sb;
        metronome.setPolyrhythm(pb, sb);
        saveUIToSection(currentSectionIdx, true);
        updatePolyrhythmUI();
        updateObsWidgetPolyrhythmDisplay();
        updateSubPolyCell(currentSectionIdx);

        if (section.hasPolyrhythm) {
            m_beatIndicatorWidget->setPolyrhythmGrid(
                section.polyrhythm.primaryBeats,
                section.polyrhythm.secondaryBeats,
                -1
            );
            m_beatIndicatorWidget->setMode(BeatIndicatorMode::PolyrhythmGrid);
        } else {
            int beatsPerBar = section.numerator;
            // --- PATCH: handle compound time main beats ---
            if (section.denominator == 8 && section.numerator % 3 == 0 && section.numerator > 3)
                beatsPerBar = section.numerator / 3;
            int subdivisions = section.subdivisionPattern.pulses.size();
            if (subdivisions == 0) subdivisions = 1;
            m_beatIndicatorWidget->setBeats(beatsPerBar);
            m_beatIndicatorWidget->setSubdivisions(subdivisions);
            m_beatIndicatorWidget->setCurrent(0, 0);
            m_beatIndicatorWidget->setMode(BeatIndicatorMode::Circles);
        }

        if (metronome.isRunning() && section.hasPolyrhythm) {
            metronome.stop();
            metronome.start();
        }
    }
}

void MainWindow::updateObsWidgetPolyrhythmDisplay()
{
    if (!ui->obsBeatWidget)
        return;

    bool isPoly = false;
    int mainBeats = 0, polyBeats = 0;
    if (currentSectionIdx >= 0 && currentSectionIdx < (int)currentPreset.sections.size())
    {
        const auto& s = currentPreset.sections[currentSectionIdx];
        isPoly = s.hasPolyrhythm;
        mainBeats = s.polyrhythm.primaryBeats;
        polyBeats = s.polyrhythm.secondaryBeats;
    }
    if (metronome.isPolyrhythmEnabled() && isPoly) {
        ui->obsBeatWidget->setPolyrhythmMode(true, mainBeats, polyBeats);
    } else {
        ui->obsBeatWidget->setPolyrhythmMode(false);
        if (currentSectionIdx >= 0 && currentSectionIdx < (int)currentPreset.sections.size()) {
            const auto& pattern = currentPreset.sections[currentSectionIdx].subdivisionPattern;
            QPixmap obsPx = assembleSubdivisionPixmapForOBS(pattern);
            ui->obsBeatWidget->setSubdivisionPixmap(obsPx);
        }
    }
}

// --- SETTINGS DIALOG ---

void MainWindow::onSettingsClicked()
{
    SettingsDialog dlg(this);
    dlg.setSelectedSoundSet(m_soundSet);
    dlg.setSelectedAccentColor(m_accentColor);
    dlg.setObsWidgetHidden(m_obsHidden);
    dlg.setAlwaysOnTop(m_alwaysOnTop);

    // Capture previous values so we can avoid unnecessary sample reloads
    QString oldSoundSet = m_soundSet;
    QColor oldAccentColor = m_accentColor;
    bool oldObsHidden = m_obsHidden;
    bool oldAlwaysOnTop = m_alwaysOnTop;

    if (dlg.exec() == QDialog::Accepted) {
        QString newSoundSet = dlg.selectedSoundSet();
        QColor newAccentColor = dlg.selectedAccentColor();
        bool newObsHidden = dlg.obsWidgetHidden();
        bool newAlwaysOnTop = dlg.alwaysOnTop();

        // Apply UI changes immediately
        m_soundSet = newSoundSet;
        m_accentColor = newAccentColor;

        updateButtonColors();

        QSettings settings("YourCompany", "MetronomeApp");
        settings.setValue("accentColor", m_accentColor.name());
        settings.setValue("soundSet", m_soundSet);
        settings.setValue("obsHidden", newObsHidden);
        settings.setValue("alwaysOnTop", newAlwaysOnTop);
        settings.sync();

        if (m_beatIndicatorWidget)
            m_beatIndicatorWidget->setAccentColor(m_accentColor);

        QPalette pal = qApp->palette();
        pal.setColor(QPalette::Highlight, m_accentColor);
        qApp->setPalette(pal);

        if (newAlwaysOnTop != m_alwaysOnTop) {
            QMessageBox msgBox(this);
            msgBox.setWindowTitle("Restart Required");
            msgBox.setText("'Always on top' will take effect after a restart.");
            QPushButton* restartButton = msgBox.addButton(tr("Restart Now"), QMessageBox::AcceptRole);
            QPushButton* okButton = msgBox.addButton(QMessageBox::Ok);
            msgBox.setDefaultButton(okButton);
            msgBox.setEscapeButton(okButton);
            msgBox.exec();

            if (msgBox.clickedButton() == restartButton) {
                QString program = QCoreApplication::applicationFilePath();
                QStringList arguments = QCoreApplication::arguments();
                QProcess::startDetached(program, arguments);
                qApp->quit();
                return;
            }
        }

        // --- POPOUT / OBS WIDGET HANDLING (unchanged behaviour) ---
        QVBoxLayout* mainLayout = qobject_cast<QVBoxLayout*>(ui->centralwidget->layout());
        if (ui->obsBeatWidget && mainLayout) {
            static bool obsSizeSet = false;
            if (!obsSizeSet) {
                ui->obsBeatWidget->setMinimumHeight(306);
                ui->obsBeatWidget->setMaximumHeight(306);
                ui->obsBeatWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
                obsSizeSet = true;
            }

            if (!newObsHidden) {
                // Ensure we have a hidden host available (used when user later closes popout)
                if (!m_obsHiddenHost) {
                    m_obsHiddenHost = new QWidget(this);
                    m_obsHiddenHost->hide();
                }

                // If no popout window exists, create one and reparent the widget into it.
                if (!m_obsWindow) {
                    // Remove from any current layout first
                    QWidget* currentParent = ui->obsBeatWidget->parentWidget();
                    if (currentParent) {
                        QLayout* parentLayout = currentParent->layout();
                        if (parentLayout) parentLayout->removeWidget(ui->obsBeatWidget);
                    }

                    m_obsWindow = new OBSBeatWindow(ui->obsBeatWidget, this);
                    connect(m_obsWindow, &OBSBeatWindow::windowAboutToClose, this, &MainWindow::onObsWindowAboutToClose);
                    m_obsWindow->show();
                } else {
                    // Popout already exists — ensure it's visible and focused
                    m_obsWindow->show();
                    m_obsWindow->raise();
                    m_obsWindow->activateWindow();
                }

                ui->obsBeatWidget->setVisible(true);
                // While in the popout we consider it not in-layout
                m_obsInLayout = false;
            } else {
                // User disabled OBS in settings — close popout and hide widget
                if (m_obsWindow) {
                    // Disconnect to avoid handling signals during teardown
                    m_obsWindow->disconnect(this);
                    // This will trigger windowAboutToClose which will reparent the widget into the hidden host
                    m_obsWindow->close();
                    m_obsWindow->deleteLater();
                    m_obsWindow = nullptr;
                }

                // Ensure there is a hidden host to keep the widget alive off-screen
                if (!m_obsHiddenHost) {
                    m_obsHiddenHost = new QWidget(this);
                    m_obsHiddenHost->hide();
                }

                // Remove from any layout it might still be in
                QWidget* currentParent = ui->obsBeatWidget->parentWidget();
                if (currentParent) {
                    QLayout* parentLayout = currentParent->layout();
                    if (parentLayout) parentLayout->removeWidget(ui->obsBeatWidget);
                }

                // Reparent to hidden host and hide
                ui->obsBeatWidget->setParent(m_obsHiddenHost);
                ui->obsBeatWidget->hide();
                m_obsInLayout = false;
            }

            // Common follow-ups
            ui->centralwidget->layout()->activate();
            qApp->processEvents();
            adjustSize();
        }

        // Persist obsHidden state
        m_obsHidden = newObsHidden;

        // --- ONLY reload audio samples if user actually changed the sound set ---
        // Reloading samples is expensive and can cause resampling or device interactions;
        // do it only when the chosen sound set changed.
        if (oldSoundSet != m_soundSet) {
            metronome.loadSample("accent", soundFileForSet(m_soundSet, true));
            metronome.loadSample("click",  soundFileForSet(m_soundSet, false));
            metronome.setAccentSound("accent");
            metronome.setClickSound("click");
        } else {
            // If sound set didn't change, ensure the metronome is still referencing the correct names.
            metronome.setAccentSound("accent");
            metronome.setClickSound("click");
        }

        // Update stored "always on top" flag after handling restart prompt
        m_alwaysOnTop = newAlwaysOnTop;
    }
}

// --- HELPER: Default pattern for new sections (quarter note) ---
SubdivisionPattern MainWindow::getDefaultSubdivisionPattern() const {
    // You should have a list of built-in patterns somewhere (perhaps in SubdivisionSelectorDialog or a static list).
    // For example:
    extern std::vector<SubdivisionPattern> builtinSubdivisionPatterns;
    if (!builtinSubdivisionPatterns.empty())
        return builtinSubdivisionPatterns[0]; // Quarter note pattern is first
    // If not found, fallback to a 1-pulse, "Quarter Note" pattern
    SubdivisionPattern fallback;
    fallback.name = "Quarter Note";
    fallback.pulses = { {1.0, false, false} }; // duration, isRest, accent
    return fallback;
}

QPixmap MainWindow::assembleSubdivisionPixmapForOBS(const SubdivisionPattern& pattern) const {
    // First create a reasonably sized note image
    NoteAssembler assembler;
    NoteAssemblerConfig cfg = configForPattern(pattern);
    cfg.pixmapSize = QSize(100, 100);  // Start with a square base image
    
    QPixmap basePixmap = assembler.assembleNote(cfg);
    
    // Then scale it to the desired OBS size
    QSize obsSize(600, 100);
    return basePixmap.scaled(obsSize, Qt::KeepAspectRatio, Qt::SmoothTransformation);
}

void MainWindow::updateButtonColors() {
    // Use accent color for enabled, darker for disabled
    QColor accent = m_accentColor;
    QColor darkAccent = m_accentColor.darker(200);

    // Example: update Polyrhythm button
    bool polyEnabled = currentSectionIdx >= 0 && currentSectionIdx < (int)currentPreset.sections.size()
                       && currentPreset.sections[currentSectionIdx].hasPolyrhythm;
    ui->btnPolyrhythm->setStyleSheet(QString("background-color: %1; color: white;").arg(polyEnabled ? accent.name() : darkAccent.name()));

    // Timer button
    ui->btnTimer->setStyleSheet(QString("background-color: %1; color: white;").arg(m_timerEnabled ? accent.name() : darkAccent.name()));

    // Speed button
    ui->btnSpeed->setStyleSheet(QString("background-color: %1; color: white;").arg(m_speedEnabled ? accent.name() : darkAccent.name()));

    // CountIn button
    ui->btnCountIn->setStyleSheet(QString("background-color: %1; color: white;").arg(m_countInEnabled ? accent.name() : darkAccent.name()));

    // Start/Stop button (example, you may want to conditionally apply color here too)
    // ui->btnStartStop->setStyleSheet(QString("background-color: %1; color: white;").arg(accent.name()));
}

void MainWindow::onMoveSectionViaShortcut(int fromRow, int toRow)
{
    int sectionCount = static_cast<int>(currentPreset.sections.size());
    if (fromRow < 0 || fromRow >= sectionCount || toRow < 0 || toRow >= sectionCount || fromRow == toRow)
        return;

    auto moved = currentPreset.sections[fromRow];
    currentPreset.sections.erase(currentPreset.sections.begin() + fromRow);
    currentPreset.sections.insert(currentPreset.sections.begin() + toRow, moved);

    QSignalBlocker blocker(ui->tableSections);
    ui->tableSections->setRowCount(0);
    ui->tableSections->setRowCount(currentPreset.sections.size());

    for (int i = 0; i < sectionCount; ++i) {
        const auto& s = currentPreset.sections[i];
        auto* labelItem = new QTableWidgetItem(s.label);
        labelItem->setFlags(labelItem->flags() | Qt::ItemIsEditable);
        ui->tableSections->setItem(i, 0, labelItem);

        auto* tempoItem = new QTableWidgetItem(QString::number(s.tempo));
        tempoItem->setFlags(tempoItem->flags() & ~Qt::ItemIsEditable);
        tempoItem->setTextAlignment(Qt::AlignCenter);
        ui->tableSections->setItem(i, 3, tempoItem);

        auto* timeSigItem = new QTableWidgetItem(QString("%1/%2").arg(s.numerator).arg(s.denominator));
        timeSigItem->setFlags(timeSigItem->flags() & ~Qt::ItemIsEditable);
        timeSigItem->setTextAlignment(Qt::AlignCenter);
        ui->tableSections->setItem(i, 1, timeSigItem);

        auto* subpolyItem = new QTableWidgetItem();
        subpolyItem->setFlags(subpolyItem->flags() & ~Qt::ItemIsEditable);
        subpolyItem->setTextAlignment(Qt::AlignCenter);
        ui->tableSections->setItem(i, 2, subpolyItem);

        updateSubPolyCell(i);
    }

    presetManager.savePreset(currentPreset);
    presetManager.saveToDisk(presetFile);

    ui->tableSections->selectRow(toRow);
    loadSectionToUI(toRow);

    const MetronomeSection& s = currentPreset.sections[toRow];
    if (metronome.isRunning()) {
        metronome.stop();
        if (!s.hasPolyrhythm) {
            QTimer::singleShot(100, this, [this, &s]() {
                metronome.start();
                m_polyBarCount = 1;
                m_lastPolyrhythmCycleIdx = -1;
                m_playingBarCounter = 1;
                ui->btnStartStop->setText(
                    QString("Bar %1/%2\nStop").arg(m_playingBarCounter).arg(s.numerator)
                );
                ui->btnStartStop->setStyleSheet("background-color: #440000; color: white;");
                if (ui->obsBeatWidget) {
                    ui->obsBeatWidget->setTempo(s.tempo);
                    ui->obsBeatWidget->setPlaying(metronome.isRunning());
                }
            });
        } else {
            metronome.start();
            m_polyBarCount = 1;
            m_lastPolyrhythmCycleIdx = -1;
            m_playingBarCounter = 1;
            ui->btnStartStop->setText(
                QString("Bar %1/%2\nStop").arg(m_playingBarCounter).arg(s.numerator)
            );
            ui->btnStartStop->setStyleSheet("background-color: #440000; color: white;");
            if (ui->obsBeatWidget) {
                ui->obsBeatWidget->setTempo(s.tempo);
                ui->obsBeatWidget->setPlaying(metronome.isRunning());
            }
        }
    }
}


// Replace the existing MainWindow::onObsWindowAboutToClose implementation with this.

void MainWindow::onObsWindowAboutToClose()
{
    // Called when the popout OBS window is closing (user clicked the X).
    // Reparent the obs widget back to a hidden host so MainWindow still owns it,
    // but DO NOT treat a user clicking the title-bar X as "turning off" the OBS widget
    // — do not change m_obsHidden or write to QSettings here. Explicit disabling
    // should go through Settings / CloseObsPopoutWindow() where the preference is persisted.

    if (!ui || !ui->obsBeatWidget)
        return;

    // Remove the obs widget from its current parent/layout (the popout)
    QWidget* parent = ui->obsBeatWidget->parentWidget();
    if (parent && parent->layout())
        parent->layout()->removeWidget(ui->obsBeatWidget);

    // Ensure a hidden host exists so the widget remains a valid pointer but is off-screen
    if (!m_obsHiddenHost) {
        m_obsHiddenHost = new QWidget(this);
        m_obsHiddenHost->hide();
    }

    // Reparent to hidden host and hide (temporary close)
    ui->obsBeatWidget->setParent(m_obsHiddenHost);
    ui->obsBeatWidget->hide();

    // IMPORTANT: Do NOT persist the "hidden" state here. Clicking the X is treated as
    // a temporary close of the popout window, not a user preference to disable OBS.
    // The persistent setting is only modified by explicit UI actions (e.g. Settings dialog
    // or CloseObsPopoutWindow()).

    // Clean up popout window object if still present
    if (m_obsWindow) {
        m_obsWindow->disconnect(this);
        m_obsWindow->deleteLater();
        m_obsWindow = nullptr;
    }

    m_obsInLayout = false;
}

void MainWindow::openObsPopoutWindow()
{
    if (!ui || !ui->obsBeatWidget) return;

    // If already open, just ensure it's visible — do NOT bring it to the foreground or activate it.
    if (m_obsWindow) {
        // show() is fine, but do NOT call raise() or activateWindow()
        m_obsWindow->show();
        return;
    }

    // Ensure hidden host exists (so we can restore later)
    if (!m_obsHiddenHost) {
        m_obsHiddenHost = new QWidget(this);
        m_obsHiddenHost->hide();
    }

    // Remove from any layout it is currently in (keeps the widget independent)
    QWidget* currentParent = ui->obsBeatWidget->parentWidget();
    if (currentParent) {
        QLayout* parentLayout = currentParent->layout();
        if (parentLayout) parentLayout->removeWidget(ui->obsBeatWidget);
    }

    // Create the popout window as a top-level window with NO owner relationship to the main window.
    // IMPORTANT: pass nullptr as parent so the window manager treats it independently and it won't be
    // automatically raised when the main window is activated.
    m_obsWindow = new OBSBeatWindow(ui->obsBeatWidget, nullptr); // top-level, unowned

    // Use a normal top-level window flag (not Qt::Tool/owned) so the WM treats it like a separate window.
    m_obsWindow->setWindowFlag(Qt::Window, true);

    // Request "show without activating" so showing it doesn't steal focus.
    // This prevents the popout from becoming active when shown programmatically.
    m_obsWindow->setAttribute(Qt::WA_ShowWithoutActivating, true);

    // Make the embedded widget not accept focus when main window is interacted with.
    ui->obsBeatWidget->setFocusPolicy(Qt::NoFocus);
    ui->obsBeatWidget->setAttribute(Qt::WA_ShowWithoutActivating, true);

    // Connect the close handler as before (it will reparent to hidden host on manual close).
    connect(m_obsWindow, &OBSBeatWindow::windowAboutToClose, this, &MainWindow::onObsWindowAboutToClose);

    // Ensure native window exists (some platforms require this before focus tricks)
    m_obsWindow->setAttribute(Qt::WA_NativeWindow, true);

    // Show without activating — with WA_ShowWithoutActivating this should not steal focus.
    m_obsWindow->show();

    // Do NOT call raise() or activateWindow() anywhere here or in other code paths when the main window
    // is interacted with. That is the critical piece — the popout must not be owned and must not be activated.

    // Remember state in memory/prefs as before
    ui->obsBeatWidget->setVisible(true);
    m_obsInLayout = false;
    m_obsHidden = false;
    QSettings settings("YourCompany", "MetronomeApp");
    settings.setValue("obsHidden", false);
    settings.sync();
}

void MainWindow::closeObsPopoutWindow()
{
    if (!m_obsWindow) {
        // ensure widget is hidden and parented to hidden host
        if (!m_obsHiddenHost) {
            m_obsHiddenHost = new QWidget(this);
            m_obsHiddenHost->hide();
        }
        if (ui && ui->obsBeatWidget) {
            QWidget* currentParent = ui->obsBeatWidget->parentWidget();
            if (currentParent && currentParent->layout())
                currentParent->layout()->removeWidget(ui->obsBeatWidget);
            ui->obsBeatWidget->setParent(m_obsHiddenHost);
            ui->obsBeatWidget->hide();
        }
        m_obsInLayout = false;
        m_obsHidden = true;
        QSettings settings("YourCompany", "MetronomeApp");
        settings.setValue("obsHidden", true);
        settings.sync();
        return;
    }

    // Disconnect signals and close the window. The onObsWindowAboutToClose slot will handle reparenting/hiding.
    m_obsWindow->disconnect(this);
    m_obsWindow->close();
    m_obsWindow->deleteLater();
    m_obsWindow = nullptr;

    // Make sure the widget ends up hidden & parented to host
    if (!m_obsHiddenHost) {
        m_obsHiddenHost = new QWidget(this);
        m_obsHiddenHost->hide();
    }
    if (ui && ui->obsBeatWidget) {
        QWidget* currentParent = ui->obsBeatWidget->parentWidget();
        if (currentParent && currentParent->layout())
            currentParent->layout()->removeWidget(ui->obsBeatWidget);
        ui->obsBeatWidget->setParent(m_obsHiddenHost);
        ui->obsBeatWidget->hide();
    }

    m_obsInLayout = false;
    m_obsHidden = true;
    QSettings settings("YourCompany", "MetronomeApp");
    settings.setValue("obsHidden", true);
    settings.sync();
}