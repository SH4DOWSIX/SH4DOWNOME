#include "customsubdivisiondialog.h"
#include "noteassembler.h"
#include <QPainter>
#include <QMouseEvent>
#include <QGroupBox>
#include <QGridLayout>
#include <QButtonGroup>
#include <QToolTip>
#include <QApplication>
#include <QMessageBox>
#include <QComboBox>

// Helper function to get duration name
QString getDurationName(double duration) {
    if (qAbs(duration - 4.0) < 0.001) return "Whole";
    if (qAbs(duration - 2.0) < 0.001) return "Half";
    if (qAbs(duration - 1.0) < 0.001) return "Quarter";
    if (qAbs(duration - 0.6667) < 0.001) return "Triplet Qtr";
    if (qAbs(duration - 0.5) < 0.001) return "Eighth";
    if (qAbs(duration - 0.3333) < 0.001) return "Triplet 8th";
    if (qAbs(duration - 0.25) < 0.001) return "Sixteenth";
    if (qAbs(duration - 0.1667) < 0.001) return "Triplet 16th";
    if (qAbs(duration - 0.125) < 0.001) return "Thirty-second";
    if (qAbs(duration - 0.0625) < 0.001) return "Sixty-fourth";
    return QString::number(duration, 'f', 3);
}

// Expanded duration values and names
const QVector<double> DURATION_VALUES = {
    0.0625,   // Sixty-fourth
    0.125,    // Thirty-second
    0.1667,   // Triplet sixteenth
    0.25,     // Sixteenth
    0.3333,   // Triplet eighth
    0.5,      // Eighth
    0.6667,   // Triplet quarter
    1.0,      // Quarter
    2.0,      // Half
    4.0       // Whole
};

const QStringList DURATION_NAMES = {
    "64th", "32nd", "Trip 16th", "16th", "Trip 8th", "8th", "Trip Qtr", "Quarter", "Half", "Whole"
};

SubdivisionPulseWidget::SubdivisionPulseWidget(const SubdivisionPulse& pulse, QWidget* parent)
    : QWidget(parent), m_pulse(pulse)
{
    setFixedSize(40, 60);
    setCursor(Qt::PointingHandCursor);
    updateToolTip();
}

void SubdivisionPulseWidget::setPulse(const SubdivisionPulse& pulse) {
    m_pulse = pulse;
    updateToolTip();
    update();
    emit changed();
}

void SubdivisionPulseWidget::setSelected(bool selected) {
    if (m_selected != selected) {
        m_selected = selected;
        update();
    }
}

void SubdivisionPulseWidget::updateToolTip() {
    QString tooltip = getDurationName(m_pulse.duration);
    if (m_pulse.isRest) tooltip += " Rest";
    else tooltip += " Note";
    if (m_pulse.isDotted) tooltip += " (Dotted)";
    if (m_pulse.accent) tooltip += " (Accented)";
    setToolTip(tooltip);
}

void SubdivisionPulseWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    // Background - highlight if accented
    QColor bgColor;
    if (m_selected) {
        bgColor = m_pulse.accent ? QColor(150, 100, 200) : QColor(100, 150, 200);
    } else {
        bgColor = m_pulse.accent ? QColor(80, 60, 100) : QColor(60, 60, 60);
    }
    p.fillRect(rect(), bgColor);

    // Border
    p.setPen(QPen(m_selected ? Qt::white : Qt::gray, 2));
    p.drawRect(rect().adjusted(1, 1, -1, -1));

    // Accent indicator (small star or dot in corner)
    if (m_pulse.accent) {
        p.setPen(QPen(Qt::yellow, 2));
        p.setBrush(Qt::yellow);
        QRect accentRect(width() - 8, 2, 6, 6);
        p.drawEllipse(accentRect);
    }

    // Create a mini note representation using NoteAssembler
    NoteAssembler assembler;
    NoteAssemblerConfig cfg;
    cfg.pixmapSize = QSize(48, 48); // Small size for the widget
    cfg.noteCount = 1;
    cfg.centerVertically = true;

    // Convert pulse to proper note type (expanded logic)
    if (m_pulse.isRest) {
        if (m_pulse.duration <= 0.0625) cfg.noteTypes.push_back(AssembledNoteType::Rest_SixtyFourth);
        else if (m_pulse.duration <= 0.125) cfg.noteTypes.push_back(AssembledNoteType::Rest_ThirtySecond);
        else if (m_pulse.duration <= 0.1667) cfg.noteTypes.push_back(AssembledNoteType::Rest_Sixteenth);
        else if (m_pulse.duration <= 0.25) cfg.noteTypes.push_back(AssembledNoteType::Rest_Sixteenth);
        else if (m_pulse.duration <= 0.3333) cfg.noteTypes.push_back(AssembledNoteType::Rest_Eighth);
        else if (m_pulse.duration <= 0.5) cfg.noteTypes.push_back(AssembledNoteType::Rest_Eighth);
        else if (m_pulse.duration <= 0.6667) cfg.noteTypes.push_back(AssembledNoteType::Rest_Quarter);
        else if (m_pulse.duration <= 1.0) cfg.noteTypes.push_back(AssembledNoteType::Rest_Quarter);
        else if (m_pulse.duration <= 2.0) cfg.noteTypes.push_back(AssembledNoteType::Rest_Quarter);
        else cfg.noteTypes.push_back(AssembledNoteType::Rest_Quarter);
    } else {
        if (m_pulse.duration <= 0.0625) cfg.noteTypes.push_back(AssembledNoteType::SixtyFourth);
        else if (m_pulse.duration <= 0.125) cfg.noteTypes.push_back(AssembledNoteType::ThirtySecond);
        else if (m_pulse.duration <= 0.1667) cfg.noteTypes.push_back(AssembledNoteType::Sixteenth);
        else if (m_pulse.duration <= 0.25) cfg.noteTypes.push_back(AssembledNoteType::Sixteenth);
        else if (m_pulse.duration <= 0.3333) cfg.noteTypes.push_back(AssembledNoteType::Eighth);
        else if (m_pulse.duration <= 0.5) cfg.noteTypes.push_back(AssembledNoteType::Eighth);
        else if (m_pulse.duration <= 0.6667) cfg.noteTypes.push_back(AssembledNoteType::Quarter);
        else if (m_pulse.duration <= 1.0) cfg.noteTypes.push_back(AssembledNoteType::Quarter);
        else if (m_pulse.duration <= 2.0) cfg.noteTypes.push_back(AssembledNoteType::Quarter);
        else cfg.noteTypes.push_back(AssembledNoteType::Quarter);
    }
    cfg.dottedNotes.push_back(m_pulse.isDotted);
    cfg.beamed = false; // Individual notes don't show beams

    // Generate the note pixmap
    QPixmap notePixmap = assembler.assembleNote(cfg);

    // Draw the note in the center of the widget
    QRect contentRect = rect().adjusted(4, 4, -4, -4);
    QRect noteRect = notePixmap.rect();
    noteRect.moveCenter(contentRect.center());

    p.drawPixmap(noteRect, notePixmap);

    // Duration indicator at bottom (smaller text)
    p.setPen(Qt::white);
    QFont smallFont = font();
    smallFont.setPointSize(6);
    p.setFont(smallFont);
    QString durText = getDurationName(m_pulse.duration);
    if (durText.length() > 7) durText = durText.left(4) + "..";
    p.drawText(rect().adjusted(2, 0, -2, -2), Qt::AlignBottom | Qt::AlignHCenter, durText);
}

void SubdivisionPulseWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        emit clicked();
    }
    QWidget::mousePressEvent(event);
}

CustomSubdivisionDialog::CustomSubdivisionDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Create Custom Subdivision");
    setModal(true);
    setMinimumSize(650, 500);
    setMaximumSize(650, 500);
    resize(650, 500);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);

    // Title
    QLabel* titleLabel = new QLabel("Custom Subdivision Pattern", this);
    QFont titleFont = titleLabel->font();
    titleFont.setPointSize(14);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    // Pulse visualization area
    QGroupBox* pulseGroup = new QGroupBox("Pattern", this);
    QVBoxLayout* pulseGroupLayout = new QVBoxLayout(pulseGroup);

    // Scrollable pulse container
    m_pulseContainer = new QWidget(this);
    m_pulseLayout = new QHBoxLayout(m_pulseContainer);
    m_pulseLayout->setSpacing(4);
    m_pulseLayout->addStretch();

    m_scrollArea = new QScrollArea(this);
    m_scrollArea->setWidget(m_pulseContainer);
    m_scrollArea->setWidgetResizable(true);
    m_scrollArea->setFixedHeight(80);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    pulseGroupLayout->addWidget(m_scrollArea);

    mainLayout->addWidget(pulseGroup);

    // Controls area
    QGroupBox* controlGroup = new QGroupBox("Edit Selected Pulse", this);
    QVBoxLayout* controlLayout = new QVBoxLayout(controlGroup);

    // Duration control
    QHBoxLayout* durationLayout = new QHBoxLayout;
    durationLayout->addWidget(new QLabel("Duration:", this));

    // Use QComboBox instead of QSlider for durations
    m_durationCombo = new QComboBox(this);
    for (const QString& name : DURATION_NAMES)
        m_durationCombo->addItem(name);
    m_durationCombo->setCurrentIndex(7); // Default to "Quarter"
    durationLayout->addWidget(m_durationCombo);

    m_durationLabel = new QLabel("Quarter", this);
    m_durationLabel->setMinimumWidth(60);
    durationLayout->addWidget(m_durationLabel);

    controlLayout->addLayout(durationLayout);

    // Type controls
    QHBoxLayout* typeLayout = new QHBoxLayout;
    typeLayout->addWidget(new QLabel("Type:", this));

    m_typeGroup = new QButtonGroup(this);
    m_noteButton = new QPushButton("Note", this);
    m_restButton = new QPushButton("Rest", this);
    m_noteButton->setCheckable(true);
    m_restButton->setCheckable(true);
    m_noteButton->setChecked(true);
    m_typeGroup->addButton(m_noteButton, 0);
    m_typeGroup->addButton(m_restButton, 1);

    typeLayout->addWidget(m_noteButton);
    typeLayout->addWidget(m_restButton);
    typeLayout->addStretch();

    m_dottedButton = new QPushButton("Dotted", this);
    m_dottedButton->setCheckable(true);
    typeLayout->addWidget(m_dottedButton);

    // NEW: Add Accented button
    m_accentedButton = new QPushButton("Accented", this);
    m_accentedButton->setCheckable(true);
    typeLayout->addWidget(m_accentedButton);

    controlLayout->addLayout(typeLayout);

    mainLayout->addWidget(controlGroup);

    // Action buttons
    QHBoxLayout* actionLayout = new QHBoxLayout;

    m_addButton = new QPushButton("Add Pulse", this);
    m_removeButton = new QPushButton("Remove Selected", this);
    m_clearButton = new QPushButton("Clear All", this);

    actionLayout->addWidget(m_addButton);
    actionLayout->addWidget(m_removeButton);
    actionLayout->addWidget(m_clearButton);
    actionLayout->addStretch();

    mainLayout->addLayout(actionLayout);

    // Common presets
    setupPresets();
    mainLayout->addLayout(m_presetLayout);

    // Preview
    m_previewLabel = new QLabel(this);
    m_previewLabel->setAlignment(Qt::AlignCenter);
    m_previewLabel->setFixedSize(625, 60);  // Fixed size - can't grow!
    m_previewLabel->setStyleSheet("border: 1px solid gray; background: #2a2a2a;");
    mainLayout->addWidget(m_previewLabel);

    // Dialog buttons
    QHBoxLayout* buttonLayout = new QHBoxLayout;
    m_okButton = new QPushButton("OK", this);
    QPushButton* cancelBtn = new QPushButton("Cancel", this);
    m_okButton->setDefault(true);
    buttonLayout->addStretch();
    buttonLayout->addWidget(m_okButton);
    buttonLayout->addWidget(cancelBtn);
    mainLayout->addLayout(buttonLayout);

    // Connections
    connect(m_durationCombo, QOverload<int>::of(&QComboBox::currentIndexChanged), this, &CustomSubdivisionDialog::onDurationChanged);
    connect(m_noteButton, &QPushButton::clicked, this, &CustomSubdivisionDialog::onTypeChanged);
    connect(m_restButton, &QPushButton::clicked, this, &CustomSubdivisionDialog::onTypeChanged);
    connect(m_dottedButton, &QPushButton::toggled, this, &CustomSubdivisionDialog::onTypeChanged);
    connect(m_accentedButton, &QPushButton::toggled, this, &CustomSubdivisionDialog::onTypeChanged);

    connect(m_addButton, &QPushButton::clicked, this, &CustomSubdivisionDialog::onAddPulse);
    connect(m_removeButton, &QPushButton::clicked, this, &CustomSubdivisionDialog::onRemoveSelected);
    connect(m_clearButton, &QPushButton::clicked, this, &CustomSubdivisionDialog::onClearAll);

    connect(m_okButton, &QPushButton::clicked, this, &CustomSubdivisionDialog::onOkClicked);
    connect(cancelBtn, &QPushButton::clicked, this, &QDialog::reject);

    // Initialize with a basic pattern
    m_pattern.category = SubdivisionCategory::Custom;
    m_pattern.name = "Custom";
    m_pattern.pulses.clear();

    rebuildPattern();
    updateControls();
    updateOkButtonState();
}

void CustomSubdivisionDialog::setPattern(const SubdivisionPattern& pattern) {
    m_pattern = pattern;
    rebuildPattern();
    updateControls();
    updateOkButtonState();
}

NoteAssemblerConfig CustomSubdivisionDialog::configForPattern(const SubdivisionPattern& pattern) const {
    NoteAssemblerConfig cfg;
    cfg.pixmapSize = QSize(48, 48);

    cfg.noteCount = pattern.pulses.size();
    cfg.noteTypes.clear();
    cfg.dottedNotes.clear();

    auto isClose = [](double a, double b) { return std::abs(a - b) < 1e-6; };

    bool isCompoundTime = false;

    auto ceilingNoteTypeForDuration = [isCompoundTime](double dur) {
        if (isCompoundTime) {
            if (dur <= 1.0/6)        return AssembledNoteType::Sixteenth;
            if (dur <= 1.0/3)        return AssembledNoteType::Eighth;
            if (dur <= 2.0/3)        return AssembledNoteType::Eighth;
            return AssembledNoteType::Quarter;
        } else {
            if (dur <= 0.0625)   return AssembledNoteType::SixtyFourth;
            if (dur <= 0.125)    return AssembledNoteType::ThirtySecond;
            if (dur <= 0.1667)   return AssembledNoteType::Sixteenth;
            if (dur <= 0.25)     return AssembledNoteType::Sixteenth;
            if (dur <= 0.3333)   return AssembledNoteType::Eighth;
            if (dur <= 0.5)      return AssembledNoteType::Eighth;
            if (dur <= 0.6667)   return AssembledNoteType::Quarter;
            return AssembledNoteType::Quarter;
        }
    };

    auto ceilingRestTypeForDuration = [isCompoundTime](double dur) {
        if (isCompoundTime) {
            if (dur <= 1.0/6)        return AssembledNoteType::Rest_Sixteenth;
            if (dur <= 1.0/3)        return AssembledNoteType::Rest_Eighth;
            if (dur <= 2.0/3)        return AssembledNoteType::Rest_Eighth;
            return AssembledNoteType::Rest_Quarter;
        } else {
            if (dur <= 0.0625)   return AssembledNoteType::Rest_SixtyFourth;
            if (dur <= 0.125)    return AssembledNoteType::Rest_ThirtySecond;
            if (dur <= 0.1667)   return AssembledNoteType::Rest_Sixteenth;
            if (dur <= 0.25)     return AssembledNoteType::Rest_Sixteenth;
            if (dur <= 0.3333)   return AssembledNoteType::Rest_Eighth;
            if (dur <= 0.5)      return AssembledNoteType::Rest_Eighth;
            if (dur <= 0.6667)   return AssembledNoteType::Rest_Quarter;
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
                cfg.noteTypes.push_back(ceilingRestTypeForDuration(p.duration));
            } else {
                cfg.noteTypes.push_back(ceilingNoteTypeForDuration(p.duration));
            }
            cfg.dottedNotes.push_back(p.isDotted);
        }
    }
    cfg.beamed = pattern.pulses.size() > 1;

    bool isTriplet = pattern.name.toLower().contains("triplet");
    if (pattern.category == SubdivisionCategory::Tuplet || isTriplet) {
        cfg.tupletNumber = pattern.pulses.size();
    }
    return cfg;
}

void CustomSubdivisionDialog::setupPresets() {
    m_presetLayout = new QHBoxLayout;
    m_presetLayout->addWidget(new QLabel("Add presets:", this));

    struct Preset {
        QString name;
        QVector<SubdivisionPulse> pulses;
    };

    QVector<Preset> presets = {
        {"Two 8ths", {{0.5, false, false}, {0.5, false, false}}},
        {"Four 16ths", {{0.25, false, false}, {0.25, false, false}, {0.25, false, false}, {0.25, false, false}}},
        {"8th + 2x16th", {{0.5, false, false}, {0.25, false, false}, {0.25, false, false}}},
        {"2x16th + 8th", {{0.25, false, false}, {0.25, false, false}, {0.5, false, false}}},
        {"Triplets", {{1.0/3, false, false}, {1.0/3, false, false}, {1.0/3, false, false}}},
        {"Dot8th + 16th", {{0.75, false, false, true}, {0.25, false, false}}}
    };

    for (const auto& preset : presets) {
        QPushButton* btn = new QPushButton(preset.name, this);
        btn->setMaximumWidth(100);
        connect(btn, &QPushButton::clicked, this, [this, preset]() {
            for (const auto& pulse : preset.pulses) {
                m_pattern.pulses.append(pulse);
            }
            rebuildPattern();
            updatePreview();
            updateOkButtonState();

            if (!preset.pulses.isEmpty()) {
                int newPulseIndex = m_pattern.pulses.size() - preset.pulses.size();
                selectPulse(newPulseIndex);
            }
        });
        m_presetLayout->addWidget(btn);
    }

    m_presetLayout->addStretch();
}

void CustomSubdivisionDialog::rebuildPattern() {
    for (auto* widget : m_pulseWidgets) {
        widget->deleteLater();
    }
    m_pulseWidgets.clear();

    for (int i = 0; i < m_pattern.pulses.size(); ++i) {
        SubdivisionPulseWidget* widget = new SubdivisionPulseWidget(m_pattern.pulses[i], this);
        connect(widget, &SubdivisionPulseWidget::clicked, this, [this, i]() {
            selectPulse(i);
        });
        connect(widget, &SubdivisionPulseWidget::changed, this, &CustomSubdivisionDialog::updatePreview);

        m_pulseLayout->insertWidget(i, widget);
        m_pulseWidgets.append(widget);
    }

    if (m_selectedPulseIndex >= m_pattern.pulses.size()) {
        m_selectedPulseIndex = -1;
    }

    updateControls();
    updatePreview();
}

void CustomSubdivisionDialog::selectPulse(int index) {
    if (index < 0 || index >= m_pulseWidgets.size()) return;

    if (m_selectedPulseIndex >= 0 && m_selectedPulseIndex < m_pulseWidgets.size()) {
        m_pulseWidgets[m_selectedPulseIndex]->setSelected(false);
    }

    m_selectedPulseIndex = index;
    m_pulseWidgets[index]->setSelected(true);

    updateControls();
}

void CustomSubdivisionDialog::updateControls() {
    bool hasSelection = (m_selectedPulseIndex >= 0 && m_selectedPulseIndex < m_pattern.pulses.size());
    m_durationCombo->setEnabled(hasSelection);
    m_noteButton->setEnabled(hasSelection);
    m_restButton->setEnabled(hasSelection);
    m_dottedButton->setEnabled(hasSelection);
    m_accentedButton->setEnabled(hasSelection);
    m_removeButton->setEnabled(hasSelection);

    if (hasSelection) {
        const SubdivisionPulse& pulse = m_pattern.pulses[m_selectedPulseIndex];
        int durationIndex = 7; // default to "Quarter"
        for (int i = 0; i < DURATION_VALUES.size(); ++i) {
            if (qAbs(pulse.duration - DURATION_VALUES[i]) < 0.001) {
                durationIndex = i;
                break;
            }
        }
        m_durationCombo->blockSignals(true);
        m_durationCombo->setCurrentIndex(durationIndex);
        m_durationCombo->blockSignals(false);
        m_durationLabel->setText(DURATION_NAMES[durationIndex]);
        m_noteButton->setChecked(!pulse.isRest);
        m_restButton->setChecked(pulse.isRest);
        m_dottedButton->setChecked(pulse.isDotted);
        m_accentedButton->setChecked(pulse.accent);
    } else {
        m_durationLabel->setText("-");
    }
}

void CustomSubdivisionDialog::updatePreview() {
    if (m_pattern.pulses.isEmpty()) {
        m_previewLabel->setText("Add pulses using 'Add Pulse' button or presets");
        m_previewLabel->setPixmap(QPixmap());
        return;
    }

    NoteAssembler assembler;
    NoteAssemblerConfig cfg = configForPattern(m_pattern);

    cfg.pixmapSize = QSize(48, 48);
    cfg.centerVertically = true;

    QPixmap preview = assembler.assembleNote(cfg);

    QPixmap scaledPreview = preview.scaled(m_previewLabel->size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_previewLabel->setPixmap(scaledPreview);
    m_previewLabel->setText("");
}

void CustomSubdivisionDialog::updateOkButtonState() {
    bool hasPattern = !m_pattern.pulses.isEmpty();
    m_okButton->setEnabled(hasPattern);

    if (!hasPattern) {
        m_okButton->setToolTip("Add at least one pulse to create a subdivision pattern");
    } else {
        m_okButton->setToolTip("");
    }
}

void CustomSubdivisionDialog::onAddPulse() {
    SubdivisionPulse newPulse;
    int idx = m_durationCombo->currentIndex();
    newPulse.duration = DURATION_VALUES[idx];
    newPulse.isRest = m_restButton->isChecked();
    newPulse.isDotted = m_dottedButton->isChecked();
    newPulse.accent = m_accentedButton->isChecked();

    m_pattern.pulses.append(newPulse);
    rebuildPattern();
    updateOkButtonState();

    selectPulse(m_pattern.pulses.size() - 1);
}

void CustomSubdivisionDialog::onRemoveSelected() {
    if (m_selectedPulseIndex >= 0 && m_selectedPulseIndex < m_pattern.pulses.size()) {
        m_pattern.pulses.removeAt(m_selectedPulseIndex);
        rebuildPattern();
        updateOkButtonState();
    }
}

void CustomSubdivisionDialog::onClearAll() {
    m_pattern.pulses.clear();
    rebuildPattern();
    updateOkButtonState();
}

void CustomSubdivisionDialog::onPulseClicked() {
    // Handled by lambda connections in rebuildPattern()
}

void CustomSubdivisionDialog::onDurationChanged() {
    if (m_selectedPulseIndex >= 0 && m_selectedPulseIndex < m_pattern.pulses.size()) {
        int idx = m_durationCombo->currentIndex();
        m_pattern.pulses[m_selectedPulseIndex].duration = DURATION_VALUES[idx];
        m_durationLabel->setText(DURATION_NAMES[idx]);
        m_pulseWidgets[m_selectedPulseIndex]->setPulse(m_pattern.pulses[m_selectedPulseIndex]);
        updatePreview();
    }
}

void CustomSubdivisionDialog::onTypeChanged() {
    if (m_selectedPulseIndex >= 0 && m_selectedPulseIndex < m_pattern.pulses.size()) {
        m_pattern.pulses[m_selectedPulseIndex].isRest = m_restButton->isChecked();
        m_pattern.pulses[m_selectedPulseIndex].isDotted = m_dottedButton->isChecked();
        m_pattern.pulses[m_selectedPulseIndex].accent = m_accentedButton->isChecked();

        m_pulseWidgets[m_selectedPulseIndex]->setPulse(m_pattern.pulses[m_selectedPulseIndex]);
        updatePreview();
    }
}

void CustomSubdivisionDialog::onPresetSelected() {
    // Handled by lambda connections in setupPresets()
}

void CustomSubdivisionDialog::onOkClicked() {
    if (m_pattern.pulses.isEmpty()) {
        QMessageBox::warning(this, "Empty Pattern",
                           "Cannot save an empty subdivision pattern. Please add at least one pulse.");
        return;
    }
    accept();
}