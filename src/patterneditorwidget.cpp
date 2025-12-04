#include "patterneditorwidget.h"

PatternEditorWidget::PatternEditorWidget(QWidget* parent)
    : QWidget(parent)
{
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(8);

    durationValues = {1.0, 0.5, 0.25, 1.0/3.0};

    addPulseBtn = new QPushButton("Add Pulse", this);
    connect(addPulseBtn, &QPushButton::clicked, this, &PatternEditorWidget::onAddPulse);
    mainLayout->addWidget(addPulseBtn, 0, Qt::AlignHCenter);

    pulseGridWidget = new QWidget(this);
    gridLayout = new QGridLayout(pulseGridWidget);
    gridLayout->setContentsMargins(4, 4, 4, 4);
    gridLayout->setHorizontalSpacing(12);
    gridLayout->setVerticalSpacing(6);

    pulseScrollArea = new QScrollArea(this);
    pulseScrollArea->setWidgetResizable(true);
    pulseScrollArea->setWidget(pulseGridWidget);
    pulseScrollArea->setMinimumHeight(120); // adjust as needed
    pulseScrollArea->setMaximumHeight(220); // fixed max height for scroll

    mainLayout->addWidget(pulseScrollArea);

    setMinimumHeight(180);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::MinimumExpanding);
}

QString PatternEditorWidget::durationLabelForValue(double val) const {
    if (qAbs(val - 1.0) < 0.001) return "Quarter";
    if (qAbs(val - 0.5) < 0.001) return "Eighth";
    if (qAbs(val - 0.25) < 0.001) return "Sixteenth";
    if (qAbs(val - 1.0/3.0) < 0.001) return "Triplet Eighth";
    return QString::number(val);
}

double PatternEditorWidget::valueForDurationIndex(int idx) const {
    if (idx >= 0 && idx < durationValues.size())
        return durationValues[idx];
    return 0.25;
}

int PatternEditorWidget::indexForDurationValue(double val) const {
    for (int i = 0; i < durationValues.size(); ++i)
        if (qAbs(durationValues[i] - val) < 0.001)
            return i;
    return 2;
}

void PatternEditorWidget::setPattern(const SubdivisionPattern& pattern, double baseDuration)
{
    m_baseDuration = baseDuration;

    // Remove all widgets from layout FIRST
    while (gridLayout->count() > 0) {
        QLayoutItem* item = gridLayout->takeAt(0);
        QWidget* w = item->widget();
        if (w) {
            w->setParent(nullptr);
            w->deleteLater();
        }
        delete item;
    }

    // Now: delete pulseControls widgets (if any remain)
    for (auto& pc : pulseControls) {
        if (pc.widget) {
            pc.widget->setParent(nullptr);
            pc.widget->deleteLater();
        }
    }
    pulseControls.clear();

    // Now: build controls fresh
    for (const auto& pulse : pattern.pulses) {
        PulseControls pc;
        pc.widget = new QWidget(this);
        QHBoxLayout* row = new QHBoxLayout(pc.widget);
        row->setContentsMargins(0, 0, 0, 0);
        row->setSpacing(12);

        pc.durationCombo = new QComboBox(pc.widget);
        for (double dur : durationValues)
            pc.durationCombo->addItem(durationLabelForValue(dur));
        pc.durationCombo->setCurrentIndex(indexForDurationValue(pulse.duration));
        row->addWidget(pc.durationCombo);
        connect(pc.durationCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, idx=pulseControls.size()] (int) { onDurationChanged(idx); });

        pc.noteCheck = new QCheckBox("Note", pc.widget);
        pc.noteCheck->setChecked(!pulse.isRest);
        row->addWidget(pc.noteCheck);

        pc.dotCheck = new QCheckBox("Dotted", pc.widget);
        pc.dotCheck->setChecked(pulse.isDotted);
        row->addWidget(pc.dotCheck);

        pc.removeBtn = new QPushButton("Remove", pc.widget);
        row->addWidget(pc.removeBtn);

        connect(pc.noteCheck, &QCheckBox::checkStateChanged, this, &PatternEditorWidget::onPulseChanged);
        connect(pc.dotCheck, &QCheckBox::checkStateChanged, this, &PatternEditorWidget::onPulseChanged);
        connect(pc.removeBtn, &QPushButton::clicked, this, &PatternEditorWidget::onRemovePulse);

        pulseControls.append(pc);
    }
    rebuildUI();
}


SubdivisionPattern PatternEditorWidget::currentPattern() const
{
    SubdivisionPattern pattern;
    pattern.category = SubdivisionCategory::Custom;
    pattern.name = "Custom";
    for (const auto& pc : pulseControls) {
        SubdivisionPulse pulse;
        pulse.duration = valueForDurationIndex(pc.durationCombo->currentIndex());
        pulse.isRest = !pc.noteCheck->isChecked();
        pulse.isDotted = pc.dotCheck->isChecked();
        pattern.pulses.push_back(pulse);
    }
    return pattern;
}

void PatternEditorWidget::rebuildUI()
{
    // Remove all items from gridLayout safely (no widget deletion here!)
    while (gridLayout->count() > 0) {
        QLayoutItem* item = gridLayout->takeAt(0);
        delete item;
    }

    // Header row
    QLabel* durationHdr = new QLabel("Duration", pulseGridWidget);
    QLabel* noteHdr = new QLabel("Note", pulseGridWidget);
    QLabel* dotHdr = new QLabel("Dotted", pulseGridWidget);
    QLabel* removeHdr = new QLabel("", pulseGridWidget);

    durationHdr->setStyleSheet("color: #ccc; font-weight: bold;");
    noteHdr->setStyleSheet("color: #ccc; font-weight: bold;");
    dotHdr->setStyleSheet("color: #ccc; font-weight: bold;");

    gridLayout->addWidget(durationHdr, 0, 0);
    gridLayout->addWidget(noteHdr, 0, 1);
    gridLayout->addWidget(dotHdr, 0, 2);
    gridLayout->addWidget(removeHdr, 0, 3);

    // Pulse controls rows
    for (int i = 0; i < pulseControls.size(); ++i) {
        auto& pc = pulseControls[i];
        gridLayout->addWidget(pc.durationCombo, i+1, 0);
        gridLayout->addWidget(pc.noteCheck, i+1, 1);
        gridLayout->addWidget(pc.dotCheck, i+1, 2);
        gridLayout->addWidget(pc.removeBtn, i+1, 3);
        pc.widget->setStyleSheet((i % 2 == 0) ? "background: #222;" : "background: #181818;");
    }
}

void PatternEditorWidget::onAddPulse()
{
    PulseControls pc;
    pc.widget = new QWidget(this);
    QHBoxLayout* row = new QHBoxLayout(pc.widget);
    row->setContentsMargins(0, 0, 0, 0);
    row->setSpacing(12);

    pc.durationCombo = new QComboBox(pc.widget);
    for (double dur : durationValues)
        pc.durationCombo->addItem(durationLabelForValue(dur));
    pc.durationCombo->setCurrentIndex(indexForDurationValue(m_baseDuration));
    row->addWidget(pc.durationCombo);
    connect(pc.durationCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, idx=pulseControls.size()] (int) { onDurationChanged(idx); });

    pc.noteCheck = new QCheckBox("Note", pc.widget);
    pc.noteCheck->setChecked(true);
    row->addWidget(pc.noteCheck);

    pc.dotCheck = new QCheckBox("Dotted", pc.widget);
    pc.dotCheck->setChecked(false);
    row->addWidget(pc.dotCheck);

    pc.removeBtn = new QPushButton("Remove", pc.widget);
    row->addWidget(pc.removeBtn);

    connect(pc.noteCheck, &QCheckBox::checkStateChanged, this, &PatternEditorWidget::onPulseChanged);
    connect(pc.dotCheck, &QCheckBox::checkStateChanged, this, &PatternEditorWidget::onPulseChanged);
    connect(pc.removeBtn, &QPushButton::clicked, this, &PatternEditorWidget::onRemovePulse);

    pulseControls.append(pc);
    rebuildUI();
    emit patternChanged();
}

void PatternEditorWidget::onRemovePulse()
{
    QPushButton* senderBtn = qobject_cast<QPushButton*>(sender());
    if (!senderBtn) return;
    for (int i = 0; i < pulseControls.size(); ++i) {
        if (pulseControls[i].removeBtn == senderBtn) {
            if (pulseControls[i].widget) {
                pulseControls[i].widget->setParent(nullptr);
                pulseControls[i].widget->deleteLater();
            }
            pulseControls.remove(i);
            break;
        }
    }
    rebuildUI();
    emit patternChanged();
}

void PatternEditorWidget::onPulseChanged()
{
    emit patternChanged();
}

void PatternEditorWidget::onDurationChanged(int /*idx*/)
{
    emit patternChanged();
}

void PatternEditorWidget::syncToControls()
{
    // For future use if you want to update controls from data
}