#include "patterneditorwidget.h"

PatternEditorWidget::PatternEditorWidget(QWidget* parent)
    : QWidget(parent)
{
    mainLayout = new QVBoxLayout(this);
    mainLayout->setContentsMargins(0, 0, 0, 0);
    mainLayout->setSpacing(8);

    baseNoteValues = {NoteValue::Quarter, NoteValue::Eighth, NoteValue::Sixteenth, NoteValue::TripletEighth};

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

// Returns human-readable label for a base NoteValue
QString PatternEditorWidget::noteValueLabel(NoteValue nv) const {
    switch (nv) {
    case NoteValue::Quarter:       return "Quarter";
    case NoteValue::Eighth:        return "Eighth";
    case NoteValue::Sixteenth:     return "Sixteenth";
    case NoteValue::TripletEighth: return "Triplet Eighth";
    case NoteValue::Half:          return "Half";
    case NoteValue::ThirtySecond:  return "32nd";
    default:                       return "Quarter";
    }
}

NoteValue PatternEditorWidget::noteValueForIndex(int idx) const {
    if (idx >= 0 && idx < baseNoteValues.size())
        return baseNoteValues[idx];
    return NoteValue::Quarter;
}

int PatternEditorWidget::indexForBaseNoteValue(NoteValue nv) const {
    // Strip dotting to find base
    NoteValue base = nv;
    switch (nv) {
    case NoteValue::DottedHalf:      base = NoteValue::Half;     break;
    case NoteValue::DottedQuarter:   base = NoteValue::Quarter;  break;
    case NoteValue::DottedEighth:    base = NoteValue::Eighth;   break;
    case NoteValue::DottedSixteenth: base = NoteValue::Sixteenth; break;
    default: break;
    }
    for (int i = 0; i < baseNoteValues.size(); ++i)
        if (baseNoteValues[i] == base) return i;
    return 0;
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
        for (NoteValue nv : baseNoteValues)
            pc.durationCombo->addItem(noteValueLabel(nv));
        // Determine if this pulse is dotted
        bool isDotted = getNoteValueInfo(pulse.noteValue).dotted;
        pc.durationCombo->setCurrentIndex(indexForBaseNoteValue(pulse.noteValue));
        row->addWidget(pc.durationCombo);
        connect(pc.durationCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, idx=pulseControls.size()] (int) { onDurationChanged(idx); });

        pc.noteCheck = new QCheckBox("Note", pc.widget);
        pc.noteCheck->setChecked(!pulse.isRest);
        row->addWidget(pc.noteCheck);

        pc.dotCheck = new QCheckBox("Dotted", pc.widget);
        pc.dotCheck->setChecked(isDotted);
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
        NoteValue base = noteValueForIndex(pc.durationCombo->currentIndex());
        bool dotted = pc.dotCheck->isChecked();
        // Map base + dotted to the proper NoteValue
        if (dotted) {
            switch (base) {
            case NoteValue::Half:      pulse.noteValue = NoteValue::DottedHalf; break;
            case NoteValue::Quarter:   pulse.noteValue = NoteValue::DottedQuarter; break;
            case NoteValue::Eighth:    pulse.noteValue = NoteValue::DottedEighth; break;
            case NoteValue::Sixteenth: pulse.noteValue = NoteValue::DottedSixteenth; break;
            default:                   pulse.noteValue = base; break;
            }
        } else {
            pulse.noteValue = base;
        }
        pulse.isRest = !pc.noteCheck->isChecked();
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
    for (NoteValue nv : baseNoteValues)
        pc.durationCombo->addItem(noteValueLabel(nv));
    pc.durationCombo->setCurrentIndex(0);  // default to Quarter
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