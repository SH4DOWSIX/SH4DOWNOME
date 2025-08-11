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






// --- Add this method for polyrhythm button color ---
void MainWindow::updatePolyrhythmButtonColor() {
    if (currentSectionIdx >= 0 && currentSectionIdx < (int)currentPreset.sections.size()) {
        bool enabled = currentPreset.sections[currentSectionIdx].hasPolyrhythm;
        if (enabled) {
            ui->btnPolyrhythm->setStyleSheet("background-color: #009f00; color: white;");
        } else {
            ui->btnPolyrhythm->setStyleSheet("background-color: #9f0000; color: white;");
        }
    }
}

QString MainWindow::subdivisionImagePathFromIndex(int index) const {
    static QString imgs[] = {
        ":/resources/quarter.svg",
        ":/resources/eighth.svg",
        ":/resources/sixteenth.svg",
        ":/resources/triplet.svg",
        ":/resources/eighth_16th_16th.svg",
        ":/resources/16th_16th_eighth.svg",
        ":/resources/16th_eighth_16th.svg",
        ":/resources/dotted8th_16th.svg",
        ":/resources/16th_dotted8th.svg",
        ":/resources/eighthrest_eighth.svg",
        ":/resources/16threst_16th_16threst_16th.svg",
        ":/resources/eighthrest_eighth_eighth.svg",
        ":/resources/eighth_eighthrest_eighth.svg",
        ":/resources/eighth_eighth_eighthrest.svg",
        ":/resources/eighthrest_eighth_eighthrest.svg"
    };
    if (index < 0 || index >= int(sizeof(imgs)/sizeof(imgs[0]))) index = 0;
    return imgs[index];
}

QString MainWindow::soundFileForSet(const QString& set, bool accent) const {
    if (set == "Default")   return accent ? "qrc:/resources/accent.wav" : "qrc:/resources/click.wav";
    if (set == "Woodblock") return accent ? "qrc:/resources/woodblock_accent.wav" : "qrc:/resources/woodblock.wav";
    if (set == "Wooden")    return accent ? "qrc:/resources/wooden_accent.wav"   : "qrc:/resources/wooden.wav";
    if (set == "Bongo")     return accent ? "qrc:/resources/bongo_accent.wav"    : "qrc:/resources/bongo.wav";
    if (set == "Cowbell")   return accent ? "qrc:/resources/cowbell_accent.wav"  : "qrc:/resources/cowbell.wav";
    if (set == "Digital")   return accent ? "qrc:/resources/digital_accent.wav"  : "qrc:/resources/digital.wav";
    if (set == "Drum")      return accent ? "qrc:/resources/drum_accent.wav"     : "qrc:/resources/drum.wav";
    if (set == "Hihat")     return accent ? "qrc:/resources/hihat_accent.wav"    : "qrc:/resources/hihat.wav";
    if (set == "Metal")     return accent ? "qrc:/resources/metal_accent.wav"    : "qrc:/resources/metal.wav";
    return accent ? "qrc:/resources/accent.wav" : "qrc:/resources/click.wav";
}

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

bool MainWindow::isQuarterNotePulse(int pulseIdx) const {
    int subdiv = getCurrentSubdivisionIndex();
    int pulsesPerQuarter = 1;
    switch (noteValueFromIndex(subdiv)) {
        case NoteValue::Quarter:
            pulsesPerQuarter = 1; break;
        case NoteValue::Eighth:
        case NoteValue::EighthRest_Eighth:
        case NoteValue::EighthRest_Eighth_Eighth:
        case NoteValue::Eighth_EighthRest_Eighth:
        case NoteValue::Eighth_Eighth_EighthRest:
        case NoteValue::EighthRest_Eighth_EighthRest:
        case NoteValue::DottedEighth_Sixteenth:
        case NoteValue::Sixteenth_DottedEighth:
            pulsesPerQuarter = 2; break;
        case NoteValue::Triplet:
            pulsesPerQuarter = 3; break;
        case NoteValue::Sixteenth:
        case NoteValue::Sixteenth_Sixteenth_Eighth:
        case NoteValue::Sixteenth_Eighth_Sixteenth:
        case NoteValue::Eighth_Sixteenth_Sixteenth:
        case NoteValue::SixteenthRest_Sixteenth_SixteenthRest_Sixteenth:
            pulsesPerQuarter = 4; break;
        default:
            pulsesPerQuarter = 1; break;
    }
    return (pulseIdx % pulsesPerQuarter) == 0;
}


MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), m_obsInLayout(true)
{
    ui->setupUi(this);

    setWindowTitle("SH4DOWNOME");
    setMinimumWidth(487); //change back 487
    setMaximumWidth(487); //change back 487

    QVBoxLayout* mainLayout = qobject_cast<QVBoxLayout*>(ui->centralwidget->layout());

    m_tapTempoResumeTimer = new QTimer(this);
m_tapTempoResumeTimer->setSingleShot(true);
connect(m_tapTempoResumeTimer, &QTimer::timeout, this, [this]() {
    if (m_metronomeWasRunning) {
        metronome.start();
        m_metronomeWasRunning = false;
    }
});


connect(ui->btnTimer, &QPushButton::clicked, this, &MainWindow::onTimerToggle);
// Hide timer widgets at startup if not enabled
updateTimerUI();

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

    accentSound.setVolume(1.0f);
    clickSound.setVolume(0.9f);

    ui->btnStartStop->setText("Start");
    ui->btnStartStop->setStyleSheet("background-color: green; color: white;");
    ui->btnTimer->setText("Timer&&Speed");

    accentSound.setSource(QUrl(soundFileForSet(m_soundSet, true)));
    clickSound.setSource(QUrl(soundFileForSet(m_soundSet, false)));

    ui->tableSections->setIconSize(QSize(48, 48));

    connect(ui->btnStartStop, &QPushButton::clicked, this, &MainWindow::onStartStop);
    connect(ui->spinTempo, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onTempoChanged);
    connect(ui->btnTapTempo, &QPushButton::clicked, this, &MainWindow::onTapTempo);
    connect(ui->sliderTempo, &QSlider::valueChanged, this, &MainWindow::onTempoSliderChanged);
    connect(ui->btnRenamePreset, &QPushButton::clicked, this, &MainWindow::onRenamePreset);
    connect(ui->spinTempo, &QSpinBox::editingFinished, this, &MainWindow::onSpinTempoEditingFinished);
    ui->verticalLayoutTimeSignature->setSpacing(0);
    ui->spinTempo->setAlignment(Qt::AlignCenter);
    ui->timeEditDuration->setAlignment(Qt::AlignCenter);


    connect(ui->btnCountIn, &QPushButton::clicked, this, [this]() {
    m_countInEnabled = !m_countInEnabled;
    if (m_countInEnabled)
        ui->btnCountIn->setStyleSheet("background-color: #009f00; color: white;");
    else
        ui->btnCountIn->setStyleSheet("background-color: #9f0000; color: white;");
});

ui->btnPolyrhythm->setStyleSheet("background-color: #9f0000; color: white;");
ui->btnTimer->setStyleSheet("background-color: #9f0000; color: white;");

ui->btnCountIn->setStyleSheet("background-color: #9f0000; color: white;");
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

    QPushButton* timeSigBtn = new QPushButton(timeSigContainer);
    timeSigBtn->setFlat(true);
    timeSigBtn->setStyleSheet("background: transparent; border: none;");
    timeSigBtn->setCursor(Qt::PointingHandCursor);
    timeSigBtn->setFocusPolicy(Qt::NoFocus);
    timeSigBtn->setFixedSize(timeSigContainer->size());
    timeSigBtn->raise();
    timeSigBtn->move(0, 0);
    ui->verticalLayoutTimeSignature->addWidget(timeSigContainer, 0, Qt::AlignHCenter);
    connect(timeSigBtn, &QPushButton::clicked, this, [this]() {
        TimeSignatureDialog dlg(currentNumerator, currentDenominator, this);
        if (dlg.exec() == QDialog::Accepted)
            setTimeSignature(dlg.selectedNumerator(), dlg.selectedDenominator());
    });

    // --- Polyrhythm numbers container (mirror timeSigContainer) ---
    m_polyrhythmNumberWidget = new QWidget(this);
    m_polyrhythmNumberWidget->setFixedSize(timeSigContainer->size());
    m_polyrhythmNumberWidget->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
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
    m_labelPolyrhythmNumerator->setContentsMargins(0, 0, 0, -2);   // negative bottom
    m_labelPolyrhythmDenominator->setContentsMargins(0, 0, 0, 0); // negative top

    ui->tempoRowWidget->setFixedHeight(45);






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

    // --- Connect parameter changes: stop metronome if running ---
    auto stopOnEdit = [this]() {
        if (metronome.isRunning())
            onStartStop();
    };
    connect(ui->spinBarsPerStep, QOverload<int>::of(&QSpinBox::valueChanged), this, stopOnEdit);
    connect(ui->spinTempoStep, QOverload<int>::of(&QSpinBox::valueChanged), this, stopOnEdit);
    connect(ui->spinMaxTempo, QOverload<int>::of(&QSpinBox::valueChanged), this, stopOnEdit);

    // --- Enable/disable logic for all controls when enabled toggled ---
    connect(ui->checkEnableSpeedTrainer, &QCheckBox::toggled, this, [this, setSpeedTrainerUIEnabled](bool enabled){
        m_speedTrainerEnabled = enabled;
        setSpeedTrainerUIEnabled(enabled);
        if (metronome.isRunning())
            onStartStop();
    });


connect(ui->checkEnableSpeedTrainer, &QCheckBox::toggled, this, [this](bool enabled){
    m_speedTrainerEnabled = enabled;
    if (!enabled) resetSpeedTrainer();
    if (!metronome.isRunning()) {
        ui->btnStartStop->setText("Start");
        ui->btnStartStop->setStyleSheet("background-color: green; color: white;");
    }
});
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

    // --- Only connect the UI's polyrhythm button, don't dynamically create/insert it! ---
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
        ui->btnStartStop->setStyleSheet("background-color: green; color: white;");
        ui->timeEditDuration->setReadOnly(false);
        ui->timeEditDuration->setTime(m_lastEnteredTimerValue); // always restore here
        m_timerWasRunning = false;
        if (metronome.isRunning()) {
            metronome.stop();
            if (ui->obsBeatWidget) ui->obsBeatWidget->setPlaying(false);
        }
        // --- Restore tempo after speed trainer ---
        if (m_speedTrainerEnabled) {
            ui->spinTempo->setValue(m_speedTrainerStartTempo);
            ui->sliderTempo->setValue(m_speedTrainerStartTempo);
            metronome.setTempo(m_speedTrainerStartTempo);
            if (ui->obsBeatWidget)
                ui->obsBeatWidget->setTempo(m_speedTrainerStartTempo);
        }
        // Optional: play a sound or show a notification here!
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
    connect(ui->tableSections, &QTableWidget::cellClicked, this, &MainWindow::onSectionSelected);

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

    ui->tableSections->setColumnWidth(0, 173);
    ui->tableSections->setColumnWidth(3, 50);
    ui->tableSections->setColumnWidth(1, 60);
    ui->tableSections->setColumnWidth(2, 60);
    ui->tableSections->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
    ui->tableSections->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Fixed);
    ui->tableSections->horizontalHeader()->setSectionResizeMode(2, QHeaderView::Fixed);
    ui->tableSections->horizontalHeader()->setSectionResizeMode(3, QHeaderView::Fixed);
    ui->tableSections->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableSections->setDragEnabled(true);
    ui->tableSections->setAcceptDrops(true);
    ui->tableSections->setDropIndicatorShown(true);
    ui->tableSections->setDragDropMode(QAbstractItemView::InternalMove);
    ui->tableSections->setSelectionMode(QAbstractItemView::SingleSelection);
    ui->tableSections->setDefaultDropAction(Qt::MoveAction);

    connect(ui->sliderTempo, &QSlider::sliderPressed, this, &MainWindow::onTempoSliderPressed);
    connect(ui->sliderTempo, &QSlider::sliderReleased, this, &MainWindow::onTempoSliderReleased);
    connect(ui->sliderTempo, QOverload<int>::of(&QSlider::valueChanged), this, &MainWindow::onTempoChanged);

    connect(ui->spinTempo, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onTempoChanged);
    connect(ui->spinTempo, &QSpinBox::editingFinished, this, &MainWindow::onSpinTempoEditingFinished);

    connect(ui->tableSections, SIGNAL(rowMoved(int,int)), this, SLOT(onSectionRowMoved(int,int)));
    connect(ui->btnSettings, &QPushButton::clicked, this, &MainWindow::onSettingsClicked);
    connect(ui->tableSections, &QTableWidget::currentCellChanged,
        this, [this](int currentRow, int, int, int) {
            if (currentRow >= 0 && currentRow < (int)currentPreset.sections.size()) {
                saveUIToSection(currentSectionIdx, true);
                loadSectionToUI(currentRow);
            }
        });

    ui->btnStartStop->setStyleSheet("background-color: green; color: white;");

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

    const int obsHeight = 306;
    if (ui->obsBeatWidget && mainLayout) {
        if (!m_obsHidden) {
            if (!m_obsInLayout) {
                mainLayout->addWidget(ui->obsBeatWidget); m_obsInLayout = true;
            }
            ui->obsBeatWidget->setVisible(true);
            ui->obsBeatWidget->setMinimumHeight(obsHeight);
            ui->obsBeatWidget->setMaximumHeight(obsHeight);
            ui->obsBeatWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Fixed);
        } else {
            if (m_obsInLayout) {
                mainLayout->removeWidget(ui->obsBeatWidget); m_obsInLayout = false;
            }
            ui->obsBeatWidget->setVisible(false);
        }
        ui->obsBeatWidget->setPlaying(false);
        ui->obsBeatWidget->setTempo(ui->spinTempo->value());
        ui->obsBeatWidget->setSubdivisionImagePath(subdivisionImagePathFromIndex(getCurrentSubdivisionIndex()));
    }

    connect(ui->sliderVolume, &QSlider::valueChanged, ui->spinVolume, &QSpinBox::setValue);
    connect(ui->spinVolume, QOverload<int>::of(&QSpinBox::valueChanged), ui->sliderVolume, &QSlider::setValue);

    auto setVolume = [this](int value){
        float vol = value / 100.0f;
        accentSound.setVolume(vol);
        clickSound.setVolume(vol);
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

    presetManager.loadFromDisk(presetFile);
    refreshPresetList();

    if (ui->comboPresets->count() > 0) {
        onLoadPreset();
    } else {
        currentPreset.songName = "";
        currentPreset.sections.clear();
        onAddSection();
    }
}


MainWindow::~MainWindow() {
    presetManager.saveToDisk(presetFile);
    delete ui;
}

bool MainWindow::eventFilter(QObject* obj, QEvent* event) {
    if ((obj == ui->labelNumerator || obj == ui->labelDenominator) && event->type() == QEvent::MouseButtonRelease) {
        TimeSignatureDialog dlg(currentNumerator, currentDenominator, this);
        if (dlg.exec() == QDialog::Accepted) {
            int newNum = dlg.selectedNumerator();
            int newDen = dlg.selectedDenominator();
            setTimeSignature(newNum, newDen);
        }
        return true;
    }
    if (obj == ui->labelSubdivisionImage && event->type() == QEvent::MouseButtonRelease) {
        onSubdivisionImageClicked();
        return true;
    }
    if (obj == m_polyrhythmNumberWidget && event->type() == QEvent::MouseButtonRelease) {
        onPolyrhythmNumberClicked();
        return true;
    }
    // --- ADDED: Pause metronome when spinTempo receives focus ---
    
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
    currentNumerator = numerator;
    currentDenominator = denominator;
    updateTimeSignatureDisplay();
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

    // --- Update beat indicator according to mode ---
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
        int beatsPerBar = numerator;
        int subdivisions = subdivisionCountFromIndex(getCurrentSubdivisionIndex());
        m_beatIndicatorWidget->setBeats(beatsPerBar);
        m_beatIndicatorWidget->setSubdivisions(subdivisions);
        m_beatIndicatorWidget->setCurrent(0, 0);
        m_beatIndicatorWidget->setMode(BeatIndicatorMode::Circles);
    }

    // --- Restart metronome if running and in polyrhythm mode ---
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


void MainWindow::startCountIn()
{
    metronome.stop();
    m_countInBeatsRemaining = currentNumerator;
    if (!m_countInTimer) {
        m_countInTimer = new QTimer(this);
        m_countInTimer->setSingleShot(false);
        connect(m_countInTimer, &QTimer::timeout, this, &MainWindow::onCountInTick);
    }
    double quarterNoteMs = 60000.0 / ui->spinTempo->value();
    m_countInTimer->start(static_cast<int>(quarterNoteMs));
    ui->btnStartStop->setText(QString("%1/%2\nStop").arg(1).arg(currentNumerator));
    ui->btnStartStop->setStyleSheet("background-color: #9f0000; color: white;");
    if (ui->obsBeatWidget) ui->obsBeatWidget->setPlaying(true);
}

void MainWindow::onCountInTick() {
    accentSound.play();

    int currentBeat = currentNumerator - m_countInBeatsRemaining + 1;
    if (m_countInBeatsRemaining > 0) {
        ui->btnStartStop->setText(
            QString("%1/%2\nStop").arg(currentBeat).arg(currentNumerator)
        );
        ui->btnStartStop->setStyleSheet("background-color: #9f0000; color: white;");
    }

    m_countInBeatsRemaining--;

    if (m_countInBeatsRemaining == 0) {
        ui->btnStartStop->setText(
            QString("%1/%2\nStop").arg(currentNumerator).arg(currentNumerator)
        );
        ui->btnStartStop->setStyleSheet("background-color: #9f0000; color: white;");
        m_countInTimer->stop();
        m_speedTrainerCountingIn = false;

        // PRE-APPLY section settings to metronome (unchanged)
        if (currentSectionIdx >= 0 && currentSectionIdx < (int)currentPreset.sections.size()) {
            const auto& s = currentPreset.sections[currentSectionIdx];
            metronome.setTimeSignature(s.numerator, s.denominator);
            metronome.setSubdivision(noteValueFromIndex(s.subdivision));
            metronome.setAccentPattern(s.accents);
            if (s.hasPolyrhythm) {
                metronome.setPolyrhythm(s.polyrhythm.primaryBeats, s.polyrhythm.secondaryBeats);
                metronome.setPolyrhythmEnabled(true);
            } else {
                metronome.setPolyrhythmEnabled(false);
            }
        }

        double quarterNoteMs = 60000.0 / ui->spinTempo->value();

        if (metronome.isPolyrhythmEnabled()) {
            QTimer::singleShot(static_cast<int>(quarterNoteMs), this, [this]() {
                startMainMetronomeAfterCountIn();
            });
        } else {
            NoteValue noteValue = noteValueFromIndex(getCurrentSubdivisionIndex());
            double pulseInterval = metronome.pulseIntervalMs(0);
            int delayMs = (noteValue == NoteValue::Quarter) ? 0 : static_cast<int>(quarterNoteMs - pulseInterval);
            if (delayMs < 0) delayMs = 0;
            QTimer::singleShot(delayMs, this, [this]() {
                startMainMetronomeAfterCountIn();
            });
        }
    }
}

void MainWindow::startMainMetronomeAfterCountIn() {
    m_playingBarCounter = 0;
    // DO NOT re-set time signature, subdivision, accent, or polyrhythm mode here!
    metronome.setPulseIdx(0);
    metronome.start();

    if (ui->obsBeatWidget)
        ui->obsBeatWidget->setPlaying(true);

    if (m_speedTrainerEnabled && !m_speedTrainerCountingIn) {
        m_speedTrainerBarCounter = 0;
        m_speedTrainerFirstCycle = true;
        updateSpeedTrainerStatus();
    } else {
        // Normal mode: show Bar 1 at TEMPO on the button
        
        ui->btnStartStop->setStyleSheet("background-color: #9f0000; color: white;");
    }
}



void MainWindow::onTimerToggle() {
    m_timerEnabled = !m_timerEnabled;
    updateTimerUI();
    if (!m_timerEnabled) {
        timer->stop();
        ui->btnStartStop->setText("Start");
        ui->btnStartStop->setStyleSheet("background-color: green; color: white;");
        ui->timeEditDuration->setReadOnly(false);
        // --- Turn off speed trainer if it was on ---
        if (ui->checkEnableSpeedTrainer->isChecked()) {
            ui->checkEnableSpeedTrainer->setChecked(false);
        }
    }
}

void MainWindow::updateTimerUI() {
    ui->btnTimer->setStyleSheet(
        m_timerEnabled ? "background-color: #009f00; color: white;"
                       : "background-color: #9f0000; color: white;");
    ui->timeEditDuration->setVisible(m_timerEnabled);
    ui->labelTimerRemaining->setVisible(m_timerEnabled);
    ui->speedTrainerWidget->setVisible(m_timerEnabled);
    
    
}



void MainWindow::onStartStop() {
    m_speedTrainerBarsPerStep = ui->spinBarsPerStep->value();
    m_speedTrainerTempoStep   = ui->spinTempoStep->value();
    m_speedTrainerMaxTempo    = ui->spinMaxTempo->value();

    // STOP LOGIC
    if (metronome.isRunning() || m_speedTrainerCountingIn) {
        m_countInBeatsLeft = 0;
        m_speedTrainerCountingIn = false;
        m_speedTrainerCurrentBar = 1;
        m_speedTrainerCurrentTempo = m_speedTrainerStartTempo;
        m_speedTrainerBarCounter = 0;
        m_playingBarCounter = 0;
        ui->btnStartStop->setText("Start");
        ui->btnStartStop->setStyleSheet("background-color: green; color: white;");
        timer->stop();
        if (m_countInTimer && m_countInTimer->isActive()) m_countInTimer->stop();
        ui->timeEditDuration->setReadOnly(false);
        if (m_timerWasRunning) ui->timeEditDuration->setTime(m_lastEnteredTimerValue);
        m_timerWasRunning = false;
        metronome.stop();
        if (ui->obsBeatWidget) ui->obsBeatWidget->setPlaying(false);
        m_countInBar = 0;
        m_countInBarTotal = 1;
        m_switchSubdivisionAfterCountIn = false;
        ui->spinTempo->setValue(m_speedTrainerStartTempo);
        ui->sliderTempo->setValue(m_speedTrainerStartTempo);
        metronome.setTempo(m_speedTrainerStartTempo);
        if (ui->obsBeatWidget)
            ui->obsBeatWidget->setTempo(m_speedTrainerStartTempo);
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

    // START LOGIC

    // --- If count-in is enabled, always do count-in first ---
    if (m_countInEnabled) {
        m_speedTrainerCountingIn = true;
        startCountIn();
        return;
    }

    // --- If Speed Trainer is enabled, use speed trainer params ---
    if (ui->checkEnableSpeedTrainer->isChecked()) {
        m_speedTrainerBarsPerStep = ui->spinBarsPerStep->value();
        m_speedTrainerTempoStep = ui->spinTempoStep->value();
        m_speedTrainerMaxTempo = ui->spinMaxTempo->value();
        m_speedTrainerCountInBeats = currentNumerator;
        m_speedTrainerCurrentBar = 1;
        m_speedTrainerBarCounter = 0;
        m_speedTrainerStartTempo = ui->spinTempo->value();
        m_speedTrainerCurrentTempo = m_speedTrainerStartTempo;
        m_speedTrainerPolyrhythm = metronome.isPolyrhythmEnabled();
        m_speedTrainerFirstCycle = true;
        ui->spinTempo->setValue(m_speedTrainerStartTempo);
        metronome.setTempo(m_speedTrainerStartTempo);
        ui->btnStartStop->setText("Stop");
        ui->btnStartStop->setStyleSheet("background-color: #9f0000; color: white;");
        metronome.start();
        if (ui->obsBeatWidget) ui->obsBeatWidget->setPlaying(true);
        m_countInBar = 0;
        m_countInBarTotal = 1;
        return;
    }

    // --- Otherwise, just start the metronome normally ---
    m_playingBarCounter = 0;
    ui->btnStartStop->setText(
        QString("Bar %1/%2\nStop")
            .arg(m_playingBarCounter)
            .arg(currentNumerator)
    );
    ui->btnStartStop->setStyleSheet("background-color: #9f0000; color: white;");
    metronome.start();
    if (ui->obsBeatWidget) ui->obsBeatWidget->setPlaying(true);
    m_countInBar = 0;
    m_countInBarTotal = 1;
}


// --- Speed Trainer Main Loop ---
void MainWindow::startSpeedTrainerMain() {
    // Set up state
    m_speedTrainerCurrentBar = 1;
    m_speedTrainerBarCounter = 0;
    m_speedTrainerCurrentTempo = m_speedTrainerStartTempo;
    ui->btnStartStop->setText("Stop");
    ui->btnStartStop->setStyleSheet("background-color: #9f0000; color: white;");
    updateSpeedTrainerStatus();
    metronome.setTempo(m_speedTrainerCurrentTempo);
    metronome.start();
    if (ui->obsBeatWidget) ui->obsBeatWidget->setPlaying(true);
}

// In your timer's timeout lambda:


void MainWindow::onTempoSliderChanged(int value) {
    // No-op
}

void MainWindow::onTempoSliderPressed() {
    if (metronome.isRunning()) {
        m_metronomeWasRunning = true;
        metronome.stop();
    } else {
        m_metronomeWasRunning = false;
    }
}

void MainWindow::onTempoSliderReleased() {
    if (m_metronomeWasRunning) {
        metronome.start();
        m_metronomeWasRunning = false;
    }
}

void MainWindow::onSpinTempoEditingFinished() {
    if (m_metronomeWasRunning) {
        metronome.start();
        m_metronomeWasRunning = false;
    }
}


void MainWindow::onTempoChanged(int value) {
    QSignalBlocker block1(ui->spinTempo);
    QSignalBlocker block2(ui->sliderTempo);

    if (ui->sliderTempo->value() != value)
        ui->sliderTempo->setValue(value);
    if (ui->spinTempo->value() != value)
        ui->spinTempo->setValue(value);

    if (currentSectionIdx >= 0 && currentSectionIdx < (int)currentPreset.sections.size())
        currentPreset.sections[currentSectionIdx].tempo = value;

    // --- Key change ---
    if (metronome.isPolyrhythmEnabled() && metronome.isRunning()) {
        metronome.armTempo(value);
        // Do NOT call setTempo now
    } else {
        metronome.setTempo(value); // subdivision or stopped
    }

    saveUIToSection(currentSectionIdx, true);

    if (currentSectionIdx >= 0 && currentSectionIdx < ui->tableSections->rowCount()) {
        QTableWidgetItem* tempoItem = ui->tableSections->item(currentSectionIdx, 3);
        if (tempoItem) tempoItem->setText(QString::number(value));
    }
    if (ui->obsBeatWidget) ui->obsBeatWidget->setTempo(value);
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


void MainWindow::onSubdivisionChanged(int index) {
    if (currentSectionIdx >= 0 && currentSectionIdx < (int)currentPreset.sections.size())
        currentPreset.sections[currentSectionIdx].subdivision = index;
    NoteValue noteValue = noteValueFromIndex(index);
    metronome.setSubdivision(noteValue);
    saveUIToSection(currentSectionIdx, true);

    // Update Sub/Poly cell live (icon for subdivision, poly text if polyrhythm enabled)
    if (currentSectionIdx >= 0 && currentSectionIdx < ui->tableSections->rowCount()) {
        updateSubPolyCell(currentSectionIdx);
    }

    setSubdivisionImage(index);
    if (ui->obsBeatWidget)
        ui->obsBeatWidget->setSubdivisionImagePath(subdivisionImagePathFromIndex(index));

    m_beatIndicatorWidget->setMode(BeatIndicatorMode::Circles);
}

void MainWindow::updateSubPolyCell(int sectionIdx) {
    if (sectionIdx < 0 || sectionIdx >= (int)currentPreset.sections.size()) return;
    const auto& s = currentPreset.sections[sectionIdx];
    QTableWidgetItem* item = ui->tableSections->item(sectionIdx, 2);
    if (!item) {
        item = new QTableWidgetItem();
        ui->tableSections->setItem(sectionIdx, 2, item);
    }
    if (s.hasPolyrhythm) {
        item->setText(QString("%1/%2").arg(s.polyrhythm.primaryBeats).arg(s.polyrhythm.secondaryBeats));
        item->setIcon(QIcon()); // Remove icon
        item->setToolTip("Polyrhythm: " + QString("%1 over %2").arg(s.polyrhythm.primaryBeats).arg(s.polyrhythm.secondaryBeats));
    } else {
        item->setIcon(QIcon(svgToPixmap(subdivisionImagePathFromIndex(s.subdivision), QSize(48, 48))));
        item->setToolTip(subdivisionTextFromIndex(s.subdivision));
        item->setText("");
    }
    item->setFlags(item->flags() & ~Qt::ItemIsEditable);
    item->setTextAlignment(Qt::AlignCenter);
}


void MainWindow::onSubdivisionImageClicked() {
    SubdivisionSelectorDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted && dlg.chosenIndex() != -1) {
        int idx = dlg.chosenIndex();
        setSubdivisionImage(idx);
        onSubdivisionChanged(idx);
    }
}

void MainWindow::setSubdivisionImage(int index) {
    static QString imgs[] = {
        ":/resources/quarter.svg",
        ":/resources/eighth.svg",
        ":/resources/sixteenth.svg",
        ":/resources/triplet.svg",
        ":/resources/eighth_16th_16th.svg",
        ":/resources/16th_16th_eighth.svg",
        ":/resources/16th_eighth_16th.svg",
        ":/resources/dotted8th_16th.svg",
        ":/resources/16th_dotted8th.svg",
        ":/resources/eighthrest_eighth.svg",
        ":/resources/16threst_16th_16threst_16th.svg",
        ":/resources/eighthrest_eighth_eighth.svg",
        ":/resources/eighth_eighthrest_eighth.svg",
        ":/resources/eighth_eighth_eighthrest.svg",
        ":/resources/eighthrest_eighth_eighthrest.svg"
    };
    if (index < 0 || index >= int(sizeof(imgs)/sizeof(imgs[0]))) index = 0;
    ui->labelSubdivisionImage->setPixmap(svgToPixmap(imgs[index], QSize(48, 48)));
}

QString MainWindow::subdivisionTextFromIndex(int index) const {
    switch (index) {
        case 0: return "Quarter Note";
        case 1: return "Eighth Note";
        case 2: return "Sixteenth Note";
        case 3: return "Triplet Note";
        case 4: return "Eighth + 2 Sixteenths";
        case 5: return "2 Sixteenths + Eighth";
        case 6: return "Sixteenth + Eighth + Sixteenth";
        case 7: return "Dotted Eighth + Sixteenth";
        case 8: return "Sixteenth + Dotted Eighth";
        case 9: return "Eighth rest + Eighth";
        case 10: return "16th rest + 16th + 16th rest + 16th";
        case 11: return "Eighth rest + Eighth + Eighth";
        case 12: return "Eighth + Eighth rest + Eighth";
        case 13: return "Eighth + Eighth + Eighth rest";
        case 14: return "Eighth rest + Eighth + Eighth rest";
        default: return "Quarter Note";
    }
}

NoteValue MainWindow::noteValueFromIndex(int index) const {
    switch (index) {
        case 0: return NoteValue::Quarter;
        case 1: return NoteValue::Eighth;
        case 2: return NoteValue::Sixteenth;
        case 3: return NoteValue::Triplet;
        case 4: return NoteValue::Eighth_Sixteenth_Sixteenth;
        case 5: return NoteValue::Sixteenth_Sixteenth_Eighth;
        case 6: return NoteValue::Sixteenth_Eighth_Sixteenth;
        case 7: return NoteValue::DottedEighth_Sixteenth;
        case 8: return NoteValue::Sixteenth_DottedEighth;
        case 9: return NoteValue::EighthRest_Eighth;
        case 10: return NoteValue::SixteenthRest_Sixteenth_SixteenthRest_Sixteenth;
        case 11: return NoteValue::EighthRest_Eighth_Eighth;
        case 12: return NoteValue::Eighth_EighthRest_Eighth;
        case 13: return NoteValue::Eighth_Eighth_EighthRest;
        case 14: return NoteValue::EighthRest_Eighth_EighthRest;
        default: return NoteValue::Quarter;
    }
}

int MainWindow::subdivisionCountFromIndex(int index) const {
    switch (index) {
        case 1: return 2;
        case 2: return 4;
        case 3: return 3;
        case 4: return 3;
        case 5: return 3;
        case 6: return 3;
        case 7: return 2;
        case 8: return 2;
        case 9: return 2;
        case 10: return 4;
        case 11: return 3;
        case 12: return 3;
        case 13: return 3;
        case 14: return 3;
        default: return 1;
    }
}

int MainWindow::getCurrentSubdivisionIndex() const {
    if (!currentPreset.sections.empty() && currentSectionIdx >= 0 && currentSectionIdx < (int)currentPreset.sections.size())
        return currentPreset.sections[currentSectionIdx].subdivision;
    return 0;
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
}

void MainWindow::setupAccentControls(int count) {
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

    QGridLayout* grid = new QGridLayout();
    grid->setContentsMargins(0, 0, 0, 0);
    grid->setSpacing(4);
    ui->accentWidget->setLayout(grid);

    const int maxPerRow = 10;
    for (int i = 0; i < count; ++i) {
        QCheckBox* cb = new QCheckBox(QString::number(i+1), ui->accentWidget);
        accentChecks.append(cb);
        int row = i / maxPerRow;
        int col = i % maxPerRow;
        grid->addWidget(cb, row, col);
        connect(cb, &QCheckBox::checkStateChanged, this, &MainWindow::onAccentChanged);
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


void MainWindow::handleSpeedTrainerBarEnd() {
    if (metronome.isPolyrhythmEnabled()) {
    // --- POLYRHYTHM LOGIC: arm tempo at the last bar of every cycle (even first) ---
    if (m_speedTrainerBarCounter == m_speedTrainerBarsPerStep - 1) {
        if (m_speedTrainerCurrentTempo < m_speedTrainerMaxTempo) {
            m_speedTrainerCurrentTempo += m_speedTrainerTempoStep;
            if (m_speedTrainerCurrentTempo > m_speedTrainerMaxTempo)
                m_speedTrainerCurrentTempo = m_speedTrainerMaxTempo;
            metronome.armTempo(m_speedTrainerCurrentTempo);

            QSignalBlocker block(ui->spinTempo);
            ui->spinTempo->setValue(m_speedTrainerCurrentTempo);

            if (ui->obsBeatWidget)
                ui->obsBeatWidget->setTempo(m_speedTrainerCurrentTempo);
        }
        m_speedTrainerBarCounter = 0;
    } else {
        m_speedTrainerBarCounter++;
    }
} else {
        // --- SUBDIVISION LOGIC: step tempo at bar 1, increment/wrap as before
        if (m_speedTrainerBarCounter == 0) {
            if (m_speedTrainerFirstCycle) {
                m_speedTrainerFirstCycle = false;
            } else {
                if (m_speedTrainerCurrentTempo < m_speedTrainerMaxTempo) {
                    m_speedTrainerCurrentTempo += m_speedTrainerTempoStep;
                    if (m_speedTrainerCurrentTempo > m_speedTrainerMaxTempo)
                        m_speedTrainerCurrentTempo = m_speedTrainerMaxTempo;
                    metronome.setTempo(m_speedTrainerCurrentTempo);

                    QSignalBlocker block(ui->spinTempo);
                    ui->spinTempo->setValue(m_speedTrainerCurrentTempo);

                    if (ui->obsBeatWidget)
                        ui->obsBeatWidget->setTempo(m_speedTrainerCurrentTempo);
                }
            }
        }
        updateSpeedTrainerStatus();
        m_speedTrainerBarCounter++;
        if (m_speedTrainerBarCounter >= m_speedTrainerBarsPerStep)
            m_speedTrainerBarCounter = 0;
    }
}

// --- Display update ---
void MainWindow::updateSpeedTrainerStatus() {
    int shownBar = m_speedTrainerBarCounter + 1;
    ui->btnStartStop->setText(
        QString("Bar %1/%2\nStop")
            .arg(shownBar)
            .arg(m_speedTrainerBarsPerStep)
            
    );
    ui->btnStartStop->setStyleSheet("background-color: #9f0000; color: white;");
}

// --- Reset Speed Trainer state when stopped or parameters changed ---
void MainWindow::resetSpeedTrainer() {
    m_speedTrainerCountingIn = false;
    m_speedTrainerCurrentBar = 1;
    m_speedTrainerBarCounter = 0;
    m_speedTrainerCurrentTempo = m_speedTrainerStartTempo;
    m_speedTrainerFirstCycle = true; // Reset first cycle flag
    
}





void MainWindow::onMetronomePulse(int idx, bool accent, bool polyAccent, bool isBeat, bool playPulse, int gridColumn) {
    int denominator = currentDenominator;
    int numerator = currentNumerator;
    int subdivisions = subdivisionCountFromIndex(getCurrentSubdivisionIndex());
    int pulsesPerBar = subdivisions * numerator;

    // Speed Trainer logic
    if (m_speedTrainerEnabled && !m_speedTrainerCountingIn && metronome.isRunning()) {
        if (metronome.isPolyrhythmEnabled()) {
            Polyrhythm poly = metronome.getPolyrhythm();
            int lastMainBeatIdx = poly.primaryBeats > 0 ? poly.primaryBeats - 1 : 0;
            if (idx == 0) {
                updateSpeedTrainerStatus();
            }
            if (idx == lastMainBeatIdx) {
                handleSpeedTrainerBarEnd();
            }
        } else {
            if ((idx % pulsesPerBar) == 0) {
                handleSpeedTrainerBarEnd();
            }
        }
    }

    // --- Normal play: update bar counter & button ---
if (!m_speedTrainerEnabled && metronome.isRunning()) {
    if ((idx % pulsesPerBar) == 0) {
        m_playingBarCounter++;
        if (m_playingBarCounter > currentNumerator)
            m_playingBarCounter = 1;
        ui->btnStartStop->setText(
            QString("Bar %1/%2\nStop")
                .arg(m_playingBarCounter)
                .arg(currentNumerator)
                
        );
        ui->btnStartStop->setStyleSheet("background-color: #9f0000; color: white;");
    }
}

    // --- Playback, Beat Indicator, and Accent Logic (unchanged) ---
    if (metronome.isPolyrhythmEnabled()) {
        Polyrhythm poly = metronome.getPolyrhythm();
        int mainBeats = poly.primaryBeats;
        int polyBeats = poly.secondaryBeats;

        m_beatIndicatorWidget->setPolyrhythmGrid(mainBeats, polyBeats, gridColumn);
        m_beatIndicatorWidget->setMode(BeatIndicatorMode::PolyrhythmGrid);

        if (polyAccent) {
            accentSound.play();
        } else if (accent) {
            clickSound.play();
        } else {
            clickSound.play();
        }
    } else {
        int currBeat = idx / subdivisions;
        int currSub = idx % subdivisions;
        m_beatIndicatorWidget->setBeats(numerator);
        m_beatIndicatorWidget->setSubdivisions(subdivisions);
        m_beatIndicatorWidget->setCurrent(currBeat, currSub);
        m_beatIndicatorWidget->setMode(BeatIndicatorMode::Circles);

        // Only play accent if this is the first subdivision of the beat and user accented it
        bool userAccent = false;
        if (currSub == 0 && currentSectionIdx >= 0 && currentSectionIdx < (int)currentPreset.sections.size()) {
            const auto& accents = currentPreset.sections[currentSectionIdx].accents;
            if (currBeat >= 0 && currBeat < (int)accents.size())
                userAccent = accents[currBeat];
        }

        if (playPulse) {
            if (userAccent)
                accentSound.play();
            else
                clickSound.play();
        }
    }

    if (ui->obsBeatWidget) {
        ui->obsBeatWidget->setPulseOn(true);
        QTimer::singleShot(80, [this]() {
            if (ui->obsBeatWidget) ui->obsBeatWidget->setPulseOn(false);
        });
    }
}

void MainWindow::refreshPresetList() {
    ui->comboPresets->clear();
    QStringList names = presetManager.listPresetNames();
    for (const QString& name : names) {
        if (!name.trimmed().isEmpty())
            ui->comboPresets->addItem(name);
    }
}

void MainWindow::onSavePreset() {
    QString name;
    while (true) {
        bool ok = false;
        name = QInputDialog::getText(this, "Save Preset (Song)", "Song name:", QLineEdit::Normal, "", &ok);
        if (!ok) {
            return;
        }
        if (name.trimmed().isEmpty()) {
            QMessageBox::information(this, "No Text Entered", "Enter text to save.");
            continue;
        }
        break;
    }

    currentPreset.songName = name;
    currentPreset.sections.clear();

    MetronomeSection s;
    s.label = "Section 1";
    s.tempo = 120;
    s.numerator = 4;
    s.denominator = 4;
    s.subdivision = 0;
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
        subpolyItem->setIcon(QIcon(svgToPixmap(subdivisionImagePathFromIndex(s.subdivision), QSize(48, 48))));
        subpolyItem->setToolTip(subdivisionTextFromIndex(s.subdivision));
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
    if (!currentPreset.sections.empty()) {
        loadSectionToUI(0);
    }
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

        // --- Sub/Poly column logic ---
        const auto& sec = currentPreset.sections[i];
        QTableWidgetItem* subpolyItem = new QTableWidgetItem();
        if (sec.hasPolyrhythm) {
            subpolyItem->setText(QString("%1/%2").arg(sec.polyrhythm.primaryBeats).arg(sec.polyrhythm.secondaryBeats));
            subpolyItem->setTextAlignment(Qt::AlignCenter);
            subpolyItem->setToolTip("Polyrhythm: " + QString("%1 over %2").arg(sec.polyrhythm.primaryBeats).arg(sec.polyrhythm.secondaryBeats));
        } else {
            subpolyItem->setIcon(QIcon(svgToPixmap(subdivisionImagePathFromIndex(sec.subdivision), QSize(48, 48))));
            subpolyItem->setToolTip(subdivisionTextFromIndex(sec.subdivision));
            subpolyItem->setText("");
        }
        subpolyItem->setFlags(subpolyItem->flags() & ~Qt::ItemIsEditable);
        subpolyItem->setTextAlignment(Qt::AlignCenter);
        ui->tableSections->setItem(i, 2, subpolyItem);
    }
    if (!currentPreset.sections.empty()) {
        ui->tableSections->selectRow(0);
        currentSectionIdx = 0;
        currentNumerator = currentPreset.sections[0].numerator;
        currentDenominator = currentPreset.sections[0].denominator;
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

        // --- Sub/Poly column logic ---
        QTableWidgetItem* subpolyItem = new QTableWidgetItem();
        if (s.hasPolyrhythm) {
            subpolyItem->setText(QString("%1/%2").arg(s.polyrhythm.primaryBeats).arg(s.polyrhythm.secondaryBeats));
            subpolyItem->setTextAlignment(Qt::AlignCenter);
            subpolyItem->setToolTip("Polyrhythm: " + QString("%1 over %2").arg(s.polyrhythm.primaryBeats).arg(s.polyrhythm.secondaryBeats));
        } else {
            subpolyItem->setIcon(QIcon(svgToPixmap(subdivisionImagePathFromIndex(s.subdivision), QSize(48, 48))));
            subpolyItem->setToolTip(subdivisionTextFromIndex(s.subdivision));
            subpolyItem->setText("");
        }
        subpolyItem->setFlags(subpolyItem->flags() & ~Qt::ItemIsEditable);
        subpolyItem->setTextAlignment(Qt::AlignCenter);
        ui->tableSections->setItem(currentSectionIdx, 2, subpolyItem);
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

void MainWindow::onSectionSelected(int row, int) {
    saveUIToSection(currentSectionIdx, true);
    if (row < 0 || row >= (int)currentPreset.sections.size()) return;
    loadSectionToUI(row);
    if (metronome.isRunning()) {
        metronome.stop();
        metronome.start();
    }
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

    setupAccentControls(s.numerator);

    for (QCheckBox* cb : accentChecks)
        cb->blockSignals(true);

    for (int i = 0; i < accentChecks.size(); ++i)
        accentChecks[i]->setChecked(i < s.accents.size() ? s.accents[i] : false);

    for (QCheckBox* cb : accentChecks)
        cb->blockSignals(false);

    // Update metronome parameters to match new section
    metronome.setTempo(s.tempo);
    metronome.setTimeSignature(s.numerator, s.denominator);
    NoteValue noteValue = noteValueFromIndex(s.subdivision);
    metronome.setSubdivision(noteValue);
    metronome.setAccentPattern(s.accents);
    metronome.setPolyrhythmEnabled(s.hasPolyrhythm);
    if (s.hasPolyrhythm) {
        metronome.setPolyrhythm(s.polyrhythm.primaryBeats, s.polyrhythm.secondaryBeats);
    }

    // --- If metronome is running, restart so it starts the new section immediately ---

    if (metronome.isRunning()) {
    metronome.stop();
    metronome.start();
}

    // --- Update beat indicator immediately for polyrhythm or normal mode ---
    if (s.hasPolyrhythm) {
        m_beatIndicatorWidget->setPolyrhythmGrid(
            s.polyrhythm.primaryBeats,
            s.polyrhythm.secondaryBeats,
            -1 // no highlight when not playing
        );
        m_beatIndicatorWidget->setMode(BeatIndicatorMode::PolyrhythmGrid);
    } else {
        int beatsPerBar = s.numerator;
        int subdivisions = subdivisionCountFromIndex(s.subdivision);
        m_beatIndicatorWidget->setBeats(beatsPerBar);
        m_beatIndicatorWidget->setSubdivisions(subdivisions);
        m_beatIndicatorWidget->setCurrent(0, 0);
        m_beatIndicatorWidget->setMode(BeatIndicatorMode::Circles);
    }

    setSubdivisionImage(s.subdivision);

    // Reset polyrhythm highlight mapping
    m_polyrhythmGridColumns.clear();
    m_polyrhythmGridMain = 0;
    m_polyrhythmGridPoly = 0;

    if (ui->obsBeatWidget) {
        ui->obsBeatWidget->setTempo(s.tempo);
        ui->obsBeatWidget->setSubdivisionImagePath(subdivisionImagePathFromIndex(s.subdivision));
        ui->obsBeatWidget->setPlaying(metronome.isRunning());
    }

    // --- Update polyrhythm button color! ---
    updatePolyrhythmButtonColor();
    updatePolyrhythmUI();
}

void MainWindow::saveUIToSection(int idx, bool doAutosave) {
    if (idx < 0 || idx >= (int)currentPreset.sections.size()) return;
    MetronomeSection& s = currentPreset.sections[idx];
    s.tempo = ui->spinTempo->value();
    s.numerator = currentNumerator;
    s.denominator = currentDenominator;
    s.subdivision = getCurrentSubdivisionIndex();
    s.accents.resize(accentChecks.size());
    for (int i = 0; i < accentChecks.size(); ++i)
        s.accents[i] = accentChecks[i]->isChecked();

    if (doAutosave) {
        presetManager.savePreset(currentPreset);
        presetManager.saveToDisk(presetFile);
    }
}

void MainWindow::saveUIToSection(int idx) {
    saveUIToSection(idx, false);
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

        // --- Sub/Poly column logic ---
        auto* subpolyItem = new QTableWidgetItem();
        if (s.hasPolyrhythm) {
            subpolyItem->setText(QString("%1/%2").arg(s.polyrhythm.primaryBeats).arg(s.polyrhythm.secondaryBeats));
            subpolyItem->setTextAlignment(Qt::AlignCenter);
            subpolyItem->setToolTip("Polyrhythm: " + QString("%1 over %2").arg(s.polyrhythm.primaryBeats).arg(s.polyrhythm.secondaryBeats));
        } else {
            subpolyItem->setIcon(QIcon(svgToPixmap(subdivisionImagePathFromIndex(s.subdivision), QSize(48, 48))));
            subpolyItem->setToolTip(subdivisionTextFromIndex(s.subdivision));
            subpolyItem->setText("");
        }
        subpolyItem->setFlags(subpolyItem->flags() & ~Qt::ItemIsEditable);
        subpolyItem->setTextAlignment(Qt::AlignCenter);
        ui->tableSections->setItem(i, 2, subpolyItem);
    }

    presetManager.savePreset(currentPreset);
    presetManager.saveToDisk(presetFile);

    ui->tableSections->selectRow(to);
    loadSectionToUI(to);
}

void MainWindow::onSettingsClicked()
{
    SettingsDialog dlg(this);
    dlg.setSelectedSoundSet(m_soundSet);
    dlg.setSelectedAccentColor(m_accentColor);
    dlg.setObsWidgetHidden(m_obsHidden);
    dlg.setAlwaysOnTop(m_alwaysOnTop);

    if (dlg.exec() == QDialog::Accepted) {
        m_soundSet = dlg.selectedSoundSet();
        m_accentColor = dlg.selectedAccentColor();
        bool newObsHidden = dlg.obsWidgetHidden();
        bool newAlwaysOnTop = dlg.alwaysOnTop();

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

        // Show restart prompt if Always On Top changed
        if (newAlwaysOnTop != m_alwaysOnTop) {
            QMessageBox msgBox(this);
            msgBox.setWindowTitle("Restart Required");
            msgBox.setText("'Always on top' will take effect after a restart.");
            QPushButton* restartButton = msgBox.addButton(tr("Restart Now"), QMessageBox::AcceptRole);
            QPushButton* okButton = msgBox.addButton(QMessageBox::Ok);
            msgBox.setDefaultButton(okButton);
            msgBox.setEscapeButton(okButton);
            // Remove the Cancel button by NOT adding it!
            msgBox.exec();

            if (msgBox.clickedButton() == restartButton) {
                // Restart logic
                QString program = QCoreApplication::applicationFilePath();
                QStringList arguments = QCoreApplication::arguments();
                QProcess::startDetached(program, arguments);
                qApp->quit();
                return;
            }
            // If OK pressed, just continue (do not restart)
        }

        // --- OBS widget logic unchanged ---
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
                if (!m_obsInLayout) {
                    mainLayout->addWidget(ui->obsBeatWidget);
                    m_obsInLayout = true;
                }
                ui->obsBeatWidget->setVisible(true);
            } else {
                if (m_obsInLayout) {
                    mainLayout->removeWidget(ui->obsBeatWidget);
                    m_obsInLayout = false;
                }
                ui->obsBeatWidget->setVisible(false);
            }
            ui->centralwidget->layout()->activate();
            qApp->processEvents();
            adjustSize();
        }

        m_obsHidden = newObsHidden;

        accentSound.setSource(QUrl(soundFileForSet(m_soundSet, true)));
        clickSound.setSource(QUrl(soundFileForSet(m_soundSet, false)));
    }
}

void MainWindow::updatePolyrhythmUI() {
    bool enabled = false;
    int mainBeats = 3, polyBeats = 2;
    if (currentSectionIdx >= 0 && currentSectionIdx < (int)currentPreset.sections.size()) {
        enabled = currentPreset.sections[currentSectionIdx].hasPolyrhythm;
        mainBeats = currentPreset.sections[currentSectionIdx].polyrhythm.primaryBeats;
        polyBeats = currentPreset.sections[currentSectionIdx].polyrhythm.secondaryBeats;
    }
    if (enabled) {
        ui->labelSubdivision->setText("Polyrhythm");
        ui->labelSubdivisionImage->hide();
        m_polyrhythmNumberWidget->setVisible(true);
        m_labelPolyrhythmNumerator->setText(QString::number(mainBeats));
        m_labelPolyrhythmDenominator->setText(QString::number(polyBeats));
    } else {
        ui->labelSubdivision->setText("Subdivision");
        ui->labelSubdivisionImage->show();
        m_polyrhythmNumberWidget->setVisible(false);
    }
}

void MainWindow::onPolyrhythmClicked() {
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
    updateSubPolyCell(currentSectionIdx);

    // --- Update beat indicator immediately ---
    if (s.hasPolyrhythm) {
        m_beatIndicatorWidget->setPolyrhythmGrid(
            s.polyrhythm.primaryBeats,
            s.polyrhythm.secondaryBeats,
            -1 // no highlight when not playing
        );
        m_beatIndicatorWidget->setMode(BeatIndicatorMode::PolyrhythmGrid);
    } else {
        int beatsPerBar = s.numerator;
        int subdivisions = subdivisionCountFromIndex(s.subdivision);
        m_beatIndicatorWidget->setBeats(beatsPerBar);
        m_beatIndicatorWidget->setSubdivisions(subdivisions);
        m_beatIndicatorWidget->setCurrent(0, 0);
        m_beatIndicatorWidget->setMode(BeatIndicatorMode::Circles);
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
        updateSubPolyCell(currentSectionIdx);

        // --- Update beat indicator immediately ---
        if (section.hasPolyrhythm) {
            m_beatIndicatorWidget->setPolyrhythmGrid(
                section.polyrhythm.primaryBeats,
                section.polyrhythm.secondaryBeats,
                -1 // no highlight when not playing
            );
            m_beatIndicatorWidget->setMode(BeatIndicatorMode::PolyrhythmGrid);
        } else {
            int beatsPerBar = section.numerator;
            int subdivisions = subdivisionCountFromIndex(section.subdivision);
            m_beatIndicatorWidget->setBeats(beatsPerBar);
            m_beatIndicatorWidget->setSubdivisions(subdivisions);
            m_beatIndicatorWidget->setCurrent(0, 0);
            m_beatIndicatorWidget->setMode(BeatIndicatorMode::Circles);
        }

        // --- Restart metronome if running and in polyrhythm mode ---
        if (metronome.isRunning() && section.hasPolyrhythm) {
            metronome.stop();
            metronome.start();
        }
    }
}

void MainWindow::onAddSection()
{
    saveUIToSection(currentSectionIdx, true);

    MetronomeSection s;
    s.label = QString("Section %1").arg(currentPreset.sections.size() + 1);
    s.tempo = ui->spinTempo->value();
    s.numerator = currentNumerator;
    s.denominator = currentDenominator;
    s.subdivision = getCurrentSubdivisionIndex();
    s.accents.resize(accentChecks.size());
    for (int i = 0; i < accentChecks.size(); ++i)
        s.accents[i] = accentChecks[i]->isChecked();
    s.hasPolyrhythm = false;
    s.polyrhythm.primaryBeats = 3;
    s.polyrhythm.secondaryBeats = 2;
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

    // --- Sub/Poly column logic ---
    QTableWidgetItem* subpolyItem = new QTableWidgetItem();
    if (s.hasPolyrhythm) {
        subpolyItem->setText(QString("%1/%2").arg(s.polyrhythm.primaryBeats).arg(s.polyrhythm.secondaryBeats));
        subpolyItem->setTextAlignment(Qt::AlignCenter);
        subpolyItem->setToolTip("Polyrhythm: " + QString("%1 over %2").arg(s.polyrhythm.primaryBeats).arg(s.polyrhythm.secondaryBeats));
    } else {
        subpolyItem->setIcon(QIcon(svgToPixmap(subdivisionImagePathFromIndex(s.subdivision), QSize(48, 48))));
        subpolyItem->setToolTip(subdivisionTextFromIndex(s.subdivision));
        subpolyItem->setText("");
    }
    subpolyItem->setFlags(subpolyItem->flags() & ~Qt::ItemIsEditable);
    subpolyItem->setTextAlignment(Qt::AlignCenter);
    ui->tableSections->setItem(idx, 2, subpolyItem);

    loadSectionToUI(idx);
    ui->tableSections->selectRow(idx);

    presetManager.savePreset(currentPreset);
    presetManager.saveToDisk(presetFile);
}


