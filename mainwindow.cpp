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

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), ui(new Ui::MainWindow), m_obsInLayout(true)
{
    ui->setupUi(this);

    QVBoxLayout* mainLayout = qobject_cast<QVBoxLayout*>(ui->centralwidget->layout());

    const int minRows = 8;
    int rowH = ui->tableSections->verticalHeader()->defaultSectionSize();
    int headerH = ui->tableSections->horizontalHeader()->height();
    int frame = 2 * ui->tableSections->frameWidth();
    int minTableHeight = minRows * rowH + headerH + frame;
    ui->tableSections->setMinimumHeight(minTableHeight);
    ui->tableSections->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->tableSections->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->tableSections->verticalHeader()->setDefaultAlignment(Qt::AlignCenter);

    setWindowTitle("SH4DOWNOME");
    setMaximumWidth(487);

    QSettings settings("YourCompany", "MetronomeApp");
    QString savedColor = settings.value("accentColor", "#960000").toString();
    m_accentColor = QColor(savedColor);
    m_soundSet = settings.value("soundSet", "Default").toString();
    m_obsHidden = settings.value("obsHidden", false).toBool();

    accentSound.setVolume(1.0f);
    clickSound.setVolume(0.9f);

    accentSound.setSource(QUrl(soundFileForSet(m_soundSet, true)));
    clickSound.setSource(QUrl(soundFileForSet(m_soundSet, false)));

    ui->tableSections->setIconSize(QSize(48, 48));

    connect(ui->btnStartStop, &QPushButton::clicked, this, &MainWindow::onStartStop);
    connect(ui->spinTempo, QOverload<int>::of(&QSpinBox::valueChanged), this, &MainWindow::onTempoChanged);
    connect(ui->btnTapTempo, &QPushButton::clicked, this, &MainWindow::onTapTempo);
    connect(ui->sliderTempo, &QSlider::valueChanged, this, &MainWindow::onTempoSliderChanged);
    connect(ui->btnRenamePreset, &QPushButton::clicked, this, &MainWindow::onRenamePreset);
    ui->verticalLayoutTimeSignature->setSpacing(0);
    ui->spinTempo->setAlignment(Qt::AlignCenter);

    ui->verticalLayoutTimeSignature->removeWidget(ui->labelNumerator);
    ui->verticalLayoutTimeSignature->removeWidget(ui->labelDenominator);

    QWidget* timeSigContainer = new QWidget(this);
    timeSigContainer->setFixedSize(40, 30);
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

    ui->labelSubdivisionImage->installEventFilter(this);

    timer = new QTimer(this);
    timer->setInterval(1000);
    connect(timer, &QTimer::timeout, this, &MainWindow::onTimerTick);

    connect(&metronome, &MetronomeEngine::pulse, this, &MainWindow::onMetronomePulse);

    connect(ui->btnSavePreset, &QPushButton::clicked, this, &MainWindow::onSavePreset);
    connect(ui->btnLoadPreset, &QPushButton::clicked, this, &MainWindow::onLoadPreset);
    ui->btnLoadPreset->hide();
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
    ui->tableSections->setColumnWidth(1, 50);
    ui->tableSections->setColumnWidth(2, 60);
    ui->tableSections->setColumnWidth(3, 60);
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

    QTimer::singleShot(0, this, [this](){
        if (ui->obsBeatWidget && ui->contentContainer) {
            int obsHeight = ui->obsBeatWidget->isVisible() ? ui->obsBeatWidget->sizeHint().height() : 0;
            m_contentAreaHeight = height() - obsHeight;
        }
    });
    QTimer::singleShot(0, this, [this]() {
        int minH = this->minimumSizeHint().height();
        this->setMinimumHeight(minH);
        this->resize(this->width(), minH);
    });
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
            QTableWidgetItem* timeSigItem = ui->tableSections->item(currentSectionIdx, 2);
            if (timeSigItem)
                timeSigItem->setText(QString("%1/%2").arg(numerator).arg(denominator));
        }
    }
    metronome.setTimeSignature(numerator, denominator);
    setupAccentControls(numerator);
    int beatsPerBar = numerator;
    int subdivisions = subdivisionCountFromIndex(getCurrentSubdivisionIndex());
    m_beatIndicatorWidget->setBeats(beatsPerBar);
    m_beatIndicatorWidget->setSubdivisions(subdivisions);
    m_beatIndicatorWidget->setCurrent(0, 0);
}

void MainWindow::updateTimeSignatureDisplay() {
    ui->labelNumerator->setAlignment(Qt::AlignHCenter);
    ui->labelDenominator->setAlignment(Qt::AlignHCenter);
    ui->labelNumerator->setText(QString::number(currentNumerator));
    ui->labelDenominator->setText(QString::number(currentDenominator));
}

void MainWindow::onStartClicked() {
    if (ui->checkTimerEnabled->isChecked()) {
        QTime time = ui->timeEditDuration->time();
        timerSecondsRemaining = time.minute() * 60 + time.second();
        updateTimerLabel();
        timer->start();
        ui->checkTimerEnabled->setEnabled(false);
        ui->timeEditDuration->setEnabled(false);
    }
}

void MainWindow::onStopClicked() {
    timer->stop();
    ui->labelTimerRemaining->setText("Time remaining: 00:00");
    ui->checkTimerEnabled->setEnabled(true);
    ui->timeEditDuration->setEnabled(true);
}

void MainWindow::onTimerTick() {
    if (timerSecondsRemaining > 0) {
        --timerSecondsRemaining;
        updateTimerLabel();
        if (timerSecondsRemaining == 0) {
            timer->stop();
            if (metronome.isRunning()) {
                onStartStop();
            }
        }
    }
}

void MainWindow::updateTimerLabel() {
    int mins = timerSecondsRemaining / 60;
    int secs = timerSecondsRemaining % 60;
    ui->labelTimerRemaining->setText(QString("Time remaining: %1:%2")
        .arg(mins, 2, 10, QChar('0')).arg(secs, 2, 10, QChar('0')));
}

void MainWindow::onStartStop() {
    if (metronome.isRunning()) {
        onStopClicked();
        metronome.stop();
        ui->btnStartStop->setStyleSheet("background-color: green; color: white;");
        ui->btnStartStop->setText("Start");
    } else {
        onStartClicked();
        metronome.start();
        ui->btnStartStop->setStyleSheet("background-color: #9f0000; color: white;");
        ui->btnStartStop->setText("Stop");
    }
    if (ui->obsBeatWidget) ui->obsBeatWidget->setPlaying(metronome.isRunning());
}

void MainWindow::onTempoSliderChanged(int value) {
    if (ui->spinTempo->value() != value)
        ui->spinTempo->setValue(value);
}

void MainWindow::onTempoChanged(int value) {
    if (ui->sliderTempo->value() != value)
        ui->sliderTempo->setValue(value);

    if (currentSectionIdx >= 0 && currentSectionIdx < (int)currentPreset.sections.size())
        currentPreset.sections[currentSectionIdx].tempo = value;

    metronome.setTempo(value);
    saveUIToSection(currentSectionIdx, true);

    if (currentSectionIdx >= 0 && currentSectionIdx < ui->tableSections->rowCount()) {
        QTableWidgetItem* tempoItem = ui->tableSections->item(currentSectionIdx, 1);
        if (tempoItem) tempoItem->setText(QString::number(value));
    }
    if (ui->obsBeatWidget) ui->obsBeatWidget->setTempo(value);
}

void MainWindow::onTapTempo() {
    qint64 now = QDateTime::currentMSecsSinceEpoch();
    const int tapResetMs = 2000;

    if (tapTimes.isEmpty() || (now - tapTimes.last()) > tapResetMs) {
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
}

void MainWindow::onSubdivisionChanged(int index) {
    if (currentSectionIdx >= 0 && currentSectionIdx < (int)currentPreset.sections.size())
        currentPreset.sections[currentSectionIdx].subdivision = index;
    NoteValue noteValue = noteValueFromIndex(index);
    metronome.setSubdivision(noteValue);
    saveUIToSection(currentSectionIdx, true);

    if (currentSectionIdx >= 0 && currentSectionIdx < ui->tableSections->rowCount()) {
        QTableWidgetItem* subdivItem = ui->tableSections->item(currentSectionIdx, 3);
        if (subdivItem) {
            subdivItem->setIcon(QIcon(subdivisionImagePathFromIndex(index)));
            subdivItem->setToolTip(subdivisionTextFromIndex(index));
            subdivItem->setText("");
        }
    }
    setSubdivisionImage(index);
    if (ui->obsBeatWidget)
        ui->obsBeatWidget->setSubdivisionImagePath(subdivisionImagePathFromIndex(index));
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

    const int maxPerRow = 12;
    for (int i = 0; i < count; ++i) {
        QCheckBox* cb = new QCheckBox(QString::number(i+1), ui->accentWidget);
        accentChecks.append(cb);
        int row = i / maxPerRow;
        int col = i % maxPerRow;
        grid->addWidget(cb, row, col);
        connect(cb, &QCheckBox::checkStateChanged, this, &MainWindow::onAccentChanged);
    }
}

void MainWindow::onMetronomePulse(int pulseIdx, bool accent, bool isBeat) {
    if (ui->obsBeatWidget) {
        ui->obsBeatWidget->setPulseOn(true);
        QTimer::singleShot(80, [this]() {
            if (ui->obsBeatWidget) ui->obsBeatWidget->setPulseOn(false);
        });
    }

    int beatsPerBar = currentNumerator;
    int subdivisions = subdivisionCountFromIndex(getCurrentSubdivisionIndex());
    int currBeat = pulseIdx / subdivisions;
    int currSub = pulseIdx % subdivisions;
    m_beatIndicatorWidget->setBeats(beatsPerBar);
    m_beatIndicatorWidget->setSubdivisions(subdivisions);
    m_beatIndicatorWidget->setCurrent(currBeat, currSub);

    if (accent)
        accentSound.play();
    else
        clickSound.play();
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
    currentPreset.sections.push_back(s);

    QSignalBlocker blocker(ui->tableSections);
    ui->tableSections->setRowCount(1);

    QTableWidgetItem* labelItem = new QTableWidgetItem(s.label);
    labelItem->setFlags(labelItem->flags() | Qt::ItemIsEditable);
    ui->tableSections->setItem(0, 0, labelItem);

    QTableWidgetItem* tempoItem = new QTableWidgetItem(QString::number(s.tempo));
    tempoItem->setFlags(tempoItem->flags() & ~Qt::ItemIsEditable);
    tempoItem->setTextAlignment(Qt::AlignCenter);
    ui->tableSections->setItem(0, 1, tempoItem);

    QTableWidgetItem* timeSigItem = new QTableWidgetItem("4/4");
    timeSigItem->setFlags(timeSigItem->flags() & ~Qt::ItemIsEditable);
    timeSigItem->setTextAlignment(Qt::AlignCenter);
    ui->tableSections->setItem(0, 2, timeSigItem);

    QTableWidgetItem* subdivItem = new QTableWidgetItem();
    subdivItem->setIcon(QIcon(svgToPixmap(subdivisionImagePathFromIndex(s.subdivision), QSize(48, 48))));
    subdivItem->setToolTip(subdivisionTextFromIndex(s.subdivision));
    subdivItem->setFlags(subdivItem->flags() & ~Qt::ItemIsEditable);
    subdivItem->setTextAlignment(Qt::AlignCenter);
    ui->tableSections->setItem(0, 3, subdivItem);

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
        ui->tableSections->setItem(i, 1, tempoItem);

        QTableWidgetItem* timeSigItem = new QTableWidgetItem(QString("%1/%2").arg(currentPreset.sections[i].numerator).arg(currentPreset.sections[i].denominator));
        timeSigItem->setFlags(timeSigItem->flags() & ~Qt::ItemIsEditable);
        timeSigItem->setTextAlignment(Qt::AlignCenter);
        ui->tableSections->setItem(i, 2, timeSigItem);

        int subdivIdx = currentPreset.sections[i].subdivision;
        QTableWidgetItem* subdivItem = new QTableWidgetItem();
        subdivItem->setIcon(QIcon(subdivisionImagePathFromIndex(subdivIdx)));
        subdivItem->setToolTip(subdivisionTextFromIndex(subdivIdx));
        subdivItem->setFlags(subdivItem->flags() & ~Qt::ItemIsEditable);
        subdivItem->setTextAlignment(Qt::AlignCenter);
        ui->tableSections->setItem(i, 3, subdivItem);
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

void MainWindow::onAddSection() {
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
    ui->tableSections->setItem(idx, 1, tempoItem);

    QTableWidgetItem* timeSigItem = new QTableWidgetItem(QString("%1/%2").arg(s.numerator).arg(s.denominator));
    timeSigItem->setFlags(timeSigItem->flags() & ~Qt::ItemIsEditable);
    timeSigItem->setTextAlignment(Qt::AlignCenter);
    ui->tableSections->setItem(idx, 2, timeSigItem);

    QTableWidgetItem* subdivItem = new QTableWidgetItem();
    subdivItem->setIcon(QIcon(svgToPixmap(subdivisionImagePathFromIndex(s.subdivision), QSize(48, 48))));
    subdivItem->setToolTip(subdivisionTextFromIndex(s.subdivision));
    subdivItem->setFlags(subdivItem->flags() & ~Qt::ItemIsEditable);
    subdivItem->setTextAlignment(Qt::AlignCenter);
    ui->tableSections->setItem(idx, 3, subdivItem);

    loadSectionToUI(idx);
    ui->tableSections->selectRow(idx);

    presetManager.savePreset(currentPreset);
    presetManager.saveToDisk(presetFile);
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
        ui->tableSections->setItem(currentSectionIdx, 1, tempoItem);

        QTableWidgetItem* timeSigItem = new QTableWidgetItem(QString("%1/%2").arg(s.numerator).arg(s.denominator));
        timeSigItem->setFlags(timeSigItem->flags() & ~Qt::ItemIsEditable);
        timeSigItem->setTextAlignment(Qt::AlignCenter);
        ui->tableSections->setItem(currentSectionIdx, 2, timeSigItem);

        QTableWidgetItem* subdivItem = new QTableWidgetItem();
        subdivItem->setIcon(QIcon(svgToPixmap(subdivisionImagePathFromIndex(s.subdivision), QSize(48, 48))));
        subdivItem->setToolTip(subdivisionTextFromIndex(s.subdivision));
        subdivItem->setFlags(subdivItem->flags() & ~Qt::ItemIsEditable);
        subdivItem->setTextAlignment(Qt::AlignCenter);
        ui->tableSections->setItem(currentSectionIdx, 3, subdivItem);
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

    metronome.setTempo(s.tempo);
    metronome.setTimeSignature(s.numerator, s.denominator);
    NoteValue noteValue = noteValueFromIndex(s.subdivision);
    metronome.setSubdivision(noteValue);
    metronome.setAccentPattern(s.accents);

    int beatsPerBar = s.numerator;
    int subdivisions = subdivisionCountFromIndex(s.subdivision);
    m_beatIndicatorWidget->setBeats(beatsPerBar);
    m_beatIndicatorWidget->setSubdivisions(subdivisions);
    m_beatIndicatorWidget->setCurrent(0, 0);

    setSubdivisionImage(s.subdivision);

    if (ui->obsBeatWidget) {
        ui->obsBeatWidget->setTempo(s.tempo);
        ui->obsBeatWidget->setSubdivisionImagePath(subdivisionImagePathFromIndex(s.subdivision));
        ui->obsBeatWidget->setPlaying(metronome.isRunning());
    }
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
        ui->tableSections->setItem(i, 1, tempoItem);

        auto* timeSigItem = new QTableWidgetItem(QString("%1/%2").arg(s.numerator).arg(s.denominator));
        timeSigItem->setFlags(timeSigItem->flags() & ~Qt::ItemIsEditable);
        timeSigItem->setTextAlignment(Qt::AlignCenter);
        ui->tableSections->setItem(i, 2, timeSigItem);

        auto* subdivItem = new QTableWidgetItem();
        subdivItem->setIcon(QIcon(svgToPixmap(subdivisionImagePathFromIndex(s.subdivision), QSize(48, 48))));
        subdivItem->setToolTip(subdivisionTextFromIndex(s.subdivision));
        subdivItem->setText("");
        subdivItem->setFlags(subdivItem->flags() & ~Qt::ItemIsEditable);
        subdivItem->setTextAlignment(Qt::AlignCenter);
        ui->tableSections->setItem(i, 3, subdivItem);
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

    if (dlg.exec() == QDialog::Accepted) {
        m_soundSet = dlg.selectedSoundSet();
        m_accentColor = dlg.selectedAccentColor();
        bool newObsHidden = dlg.obsWidgetHidden();

        QSettings settings("YourCompany", "MetronomeApp");
        settings.setValue("accentColor", m_accentColor.name());
        settings.setValue("soundSet", m_soundSet);
        settings.setValue("obsHidden", newObsHidden);
        settings.sync();

        if (m_beatIndicatorWidget)
            m_beatIndicatorWidget->setAccentColor(m_accentColor);

        QPalette pal = qApp->palette();
        pal.setColor(QPalette::Highlight, m_accentColor);
        qApp->setPalette(pal);

        QVBoxLayout* mainLayout = qobject_cast<QVBoxLayout*>(ui->centralwidget->layout());
        const int obsHeight = 306;
        if (ui->obsBeatWidget && mainLayout) {
            int wasObsVisible = ui->obsBeatWidget->isVisible();
            int contentAreaHeight = height() - (wasObsVisible ? obsHeight : 0);

            if (!newObsHidden) {
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
            ui->centralwidget->layout()->activate();
            qApp->processEvents();
            setMinimumHeight(minimumSizeHint().height());
            int afterObsHeight = ui->obsBeatWidget->isVisible() ? obsHeight : 0;
            resize(width(), contentAreaHeight + afterObsHeight);
        }

        m_obsHidden = newObsHidden;

        accentSound.setSource(QUrl(soundFileForSet(m_soundSet, true)));
        clickSound.setSource(QUrl(soundFileForSet(m_soundSet, false)));
    }
}