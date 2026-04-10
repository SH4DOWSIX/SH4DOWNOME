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
#include <functional>

// Human-readable name for any NoteValue — derived dynamically from NoteValueInfo.
QString getDurationName(NoteValue nv) {
    NoteValueInfo info = getNoteValueInfo(nv);
    QString name;
    switch (info.noteType) {
    case AssembledNoteType::Whole:        name = "Whole";   break;
    case AssembledNoteType::Half:         name = "Half";    break;
    case AssembledNoteType::Quarter:      name = "Quarter"; break;
    case AssembledNoteType::Eighth:       name = "8th";     break;
    case AssembledNoteType::Sixteenth:    name = "16th";    break;
    case AssembledNoteType::ThirtySecond: name = "32nd";    break;
    case AssembledNoteType::SixtyFourth:  name = "64th";    break;
    default:                              name = "Note";    break;
    }
    if (info.dotted)           name = "Dot." + name;
    if (info.tupletNumber > 0) name = QString("%1:%2").arg(info.tupletNumber).arg(name);
    return name;
}

// Plain note tiles — 7 entries, index 4 = Quarter.
// Dotted variants are tracked separately via the Dotted toggle.
// Tuplets are applied via resolveTupletNoteValue().
struct NoteValueComboItem {
    NoteValue base;
    NoteValue dotted;
    QString   name;
    bool      canBeDotted;
};
static const QVector<NoteValueComboItem> NOTE_VALUE_COMBO = {
    { NoteValue::SixtyFourth,  NoteValue::SixtyFourth,     "64th",    false },
    { NoteValue::ThirtySecond, NoteValue::ThirtySecond,    "32nd",    false },
    { NoteValue::Sixteenth,    NoteValue::DottedSixteenth, "16th",    true  },
    { NoteValue::Eighth,       NoteValue::DottedEighth,    "8th",     true  },
    { NoteValue::Quarter,      NoteValue::DottedQuarter,   "Quarter", true  },
    { NoteValue::Half,         NoteValue::DottedHalf,      "Half",    true  },
    { NoteValue::Whole,        NoteValue::Whole,           "Whole",   false },
};

// Decompose any NoteValue into (tileIndex, dotted, tupletN) so the dialog
// controls can sync to the selected pulse.
struct PulseDecomposition { int tileIdx; bool dotted; int tupletN; };
static PulseDecomposition decomposePulseNoteValue(NoteValue nv) {
    NoteValueInfo info = getNoteValueInfo(nv);
    // Determine the plain base from the rendered glyph
    NoteValue base;
    switch (info.noteType) {
    case AssembledNoteType::Whole:        base = NoteValue::Whole;        break;
    case AssembledNoteType::Half:         base = NoteValue::Half;         break;
    case AssembledNoteType::Quarter:      base = NoteValue::Quarter;      break;
    case AssembledNoteType::Eighth:       base = NoteValue::Eighth;       break;
    case AssembledNoteType::Sixteenth:    base = NoteValue::Sixteenth;    break;
    case AssembledNoteType::ThirtySecond: base = NoteValue::ThirtySecond; break;
    case AssembledNoteType::SixtyFourth:  base = NoteValue::SixtyFourth;  break;
    default:                              base = NoteValue::Quarter;      break;
    }
    int tileIdx = 4;
    for (int i = 0; i < NOTE_VALUE_COMBO.size(); ++i)
        if (NOTE_VALUE_COMBO[i].base == base) { tileIdx = i; break; }
    return { tileIdx, info.dotted, info.tupletNumber };
}

// Resolve active tile + dotted + tuplet → concrete NoteValue
static NoteValue resolveActiveNoteValue(int tileIdx, bool dotted, int tupletN) {
    const auto& row = NOTE_VALUE_COMBO[tileIdx];
    if (tupletN > 0)               return resolveTupletNoteValue(row.base, tupletN);
    if (dotted && row.canBeDotted) return row.dotted;
    return row.base;
}

// Whether a (tile, tuplet) combination has a defined NoteValue
static bool isValidTuplet(int tileIdx, int tupletN) {
    if (tupletN == 0) return true;
    switch (NOTE_VALUE_COMBO[tileIdx].base) {
    case NoteValue::Quarter:      return tupletN >= 2 && tupletN <= 7;
    case NoteValue::Eighth:       return tupletN >= 2 && tupletN <= 9;
    case NoteValue::Sixteenth:    return tupletN >= 2 && tupletN <= 9;
    case NoteValue::ThirtySecond: return tupletN == 3;
    default:                      return false;
    }
}

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
    QString tooltip = getDurationName(m_pulse.noteValue);
    if (m_pulse.isRest)   tooltip += " Rest";
    else                  tooltip += " Note";
    if (m_pulse.accent)   tooltip += " (Accented)";
    setToolTip(tooltip);
}

void SubdivisionPulseWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    QColor bgColor;
    if (m_selected) {
        bgColor = m_pulse.accent ? QColor(150, 100, 200) : QColor(100, 150, 200);
    } else {
        bgColor = m_pulse.accent ? QColor(80, 60, 100) : QColor(60, 60, 60);
    }
    p.fillRect(rect(), bgColor);
    p.setPen(QPen(m_selected ? Qt::white : Qt::gray, 2));
    p.drawRect(rect().adjusted(1, 1, -1, -1));

    if (m_pulse.accent) {
        p.setPen(QPen(Qt::yellow, 2));
        p.setBrush(Qt::yellow);
        p.drawEllipse(QRect(width() - 8, 2, 6, 6));
    }

    // Render note glyph using the canonical NoteValue path
    SubdivisionPattern singlePulsePattern;
    singlePulsePattern.category = SubdivisionCategory::Custom;
    singlePulsePattern.pulses   = { m_pulse };
    NoteAssemblerConfig cfg = buildNoteAssemblerConfig(singlePulsePattern);
    cfg.pixmapSize        = QSize(32, 32);
    cfg.centerVertically  = true;
    cfg.beamed            = false; // single note, never beam

    NoteAssembler assembler;
    QPixmap notePixmap = assembler.assembleNote(cfg);

    const int textAreaH = 12;
    const int textGap   = 4;
    QRect contentRect = rect().adjusted(4, 4, -4, -(textAreaH + textGap));
    QPixmap scaledNote = notePixmap.scaled(contentRect.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
    QRect noteRect = scaledNote.rect();
    noteRect.moveTopLeft(QPoint(contentRect.center().x() - noteRect.width() / 2, contentRect.top()));
    p.drawPixmap(noteRect, scaledNote);

    p.setPen(Qt::white);
    QFont smallFont = font();
    smallFont.setPointSize(6);
    p.setFont(smallFont);
    QString durText = getDurationName(m_pulse.noteValue);
    if (durText.length() > 9) durText = durText.left(6) + "..";
    p.drawText(rect().adjusted(2, 0, -2, -2), Qt::AlignBottom | Qt::AlignHCenter, durText);
}

void SubdivisionPulseWidget::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        emit clicked();
    }
    QWidget::mousePressEvent(event);
}

// ─── NoteTileWidget ──────────────────────────────────────────────────────────
// A clickable tile showing a single note glyph + name. Used as the duration
// picker in CustomSubdivisionDialog.
class NoteTileWidget : public QWidget {
public:
    explicit NoteTileWidget(int comboIndex, QWidget* parent = nullptr)
        : QWidget(parent), m_comboIndex(comboIndex)
    {
        setFixedSize(50, 62);
        setCursor(Qt::PointingHandCursor);
        setToolTip(NOTE_VALUE_COMBO[comboIndex].name);

        // Pre-render note pixmap
        SubdivisionPattern p;
        p.category = SubdivisionCategory::Custom;
        p.pulses.append(SubdivisionPulse{ NOTE_VALUE_COMBO[comboIndex].base, false, false });
        NoteAssemblerConfig cfg = buildNoteAssemblerConfig(p);
        cfg.pixmapSize       = QSize(34, 38);
        cfg.centerVertically = true;
        cfg.beamed           = false;
        NoteAssembler assembler;
        m_notePixmap = assembler.assembleNote(cfg);
    }

    void setSelected(bool selected) {
        if (m_selected != selected) { m_selected = selected; update(); }
    }
    bool isSelected() const { return m_selected; }
    int  comboIndex()  const { return m_comboIndex; }

    // Set by the owning dialog — called when the tile is clicked
    std::function<void(int)> onClicked;

protected:
    void paintEvent(QPaintEvent*) override {
        QPainter p(this);
        p.setRenderHint(QPainter::Antialiasing);

        QColor bg = m_selected ? QColor(70, 120, 190) : QColor(48, 48, 48);
        p.fillRect(rect(), bg);

        QPen borderPen(m_selected ? QColor(120, 175, 255) : QColor(75, 75, 75), 1);
        p.setPen(borderPen);
        p.drawRect(rect().adjusted(0, 0, -1, -1));

        // Note glyph centered in upper area
        const int textH = 14;
        QRect noteArea(2, 2, width() - 4, height() - textH - 4);
        QPixmap scaled = m_notePixmap.scaled(noteArea.size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        QRect dr = scaled.rect();
        dr.moveTopLeft({ noteArea.center().x() - dr.width() / 2, noteArea.top() });
        p.drawPixmap(dr, scaled);

        // Note name at bottom
        p.setPen(m_selected ? Qt::white : QColor(180, 180, 180));
        QFont f = font();
        f.setPointSize(6);
        p.setFont(f);
        p.drawText(QRect(0, height() - textH, width(), textH),
                   Qt::AlignHCenter | Qt::AlignVCenter,
                   NOTE_VALUE_COMBO[m_comboIndex].name);
    }

    void mousePressEvent(QMouseEvent* e) override {
        if (e->button() == Qt::LeftButton && onClicked)
            onClicked(m_comboIndex);
        QWidget::mousePressEvent(e);
    }

private:
    int     m_comboIndex;
    bool    m_selected = false;
    QPixmap m_notePixmap;
};

CustomSubdivisionDialog::CustomSubdivisionDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Create Custom Subdivision");
    setModal(true);
    setFixedSize(650, 620);

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

    // Pattern name field
    QHBoxLayout* nameLayout = new QHBoxLayout;
    nameLayout->addWidget(new QLabel("Name:", this));
    m_nameEdit = new QLineEdit(this);
    m_nameEdit->setPlaceholderText("Pattern name");
    m_nameEdit->setText("Custom Pattern");
    nameLayout->addWidget(m_nameEdit);
    mainLayout->addLayout(nameLayout);

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
    m_scrollArea->setFixedHeight(100);
    m_scrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_scrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    pulseGroupLayout->addWidget(m_scrollArea);

    mainLayout->addWidget(pulseGroup);

    // Controls area
    QGroupBox* controlGroup = new QGroupBox("Add / Edit Selected Pulse", this);
    QVBoxLayout* controlLayout = new QVBoxLayout(controlGroup);

    // Duration control — plain note tile picker (64th → Whole)
    QHBoxLayout* durationLayout = new QHBoxLayout;
    durationLayout->setSpacing(3);
    durationLayout->setContentsMargins(0, 0, 0, 0);
    m_selectedTileIndex = 4; // Quarter
    for (int i = 0; i < NOTE_VALUE_COMBO.size(); ++i) {
        NoteTileWidget* tile = new NoteTileWidget(i, this);
        tile->onClicked = [this](int idx) { onNoteTileClicked(idx); };
        tile->setSelected(i == m_selectedTileIndex);
        m_noteTiles.append(tile);
        durationLayout->addWidget(tile);
    }
    durationLayout->addStretch();
    controlLayout->addLayout(durationLayout);

    // Tuplet row: [×] [2] [3] [4] [5] [6] [7] [8] [9]
    QHBoxLayout* tupletLayout = new QHBoxLayout;
    tupletLayout->setSpacing(3);
    tupletLayout->setContentsMargins(0, 0, 0, 0);
    tupletLayout->addWidget(new QLabel("Tuplet:", this));
    m_tupletButtons.resize(10, nullptr); // indices 0 and 2-9 used
    m_selectedTuplet = 0;
    auto makeTupletBtn = [&](int n, const QString& label) {
        QPushButton* btn = new QPushButton(label, this);
        btn->setFixedWidth(30);
        btn->setCheckable(true);
        btn->setChecked(n == 0); // × is initially selected
        connect(btn, &QPushButton::clicked, this, [this, n]() { onTupletButtonClicked(n); });
        m_tupletButtons[n] = btn;
        tupletLayout->addWidget(btn);
    };
    makeTupletBtn(0, "×");
    for (int n = 2; n <= 9; ++n)
        makeTupletBtn(n, QString::number(n));
    tupletLayout->addStretch();
    controlLayout->addLayout(tupletLayout);

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
    m_previewLabel->setFixedHeight(80);
    m_previewLabel->setMinimumWidth(600);
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
    if (!pattern.name.isEmpty())
        m_nameEdit->setText(pattern.name);
    rebuildPattern();
    updateControls();
    updateOkButtonState();
}

NoteAssemblerConfig CustomSubdivisionDialog::configForPattern(const SubdivisionPattern& pattern) const {
    return buildNoteAssemblerConfig(pattern);
}

QString CustomSubdivisionDialog::chosenName() const {
    QString n = m_nameEdit->text().trimmed();
    return n.isEmpty() ? "Custom Pattern" : n;
}

void CustomSubdivisionDialog::setupPresets() {
    m_presetLayout = new QHBoxLayout;
    m_presetLayout->addWidget(new QLabel("Add presets:", this));

    struct Preset {
        QString name;
        QVector<SubdivisionPulse> pulses;
    };

    QVector<Preset> presets = {
        {"Two 8ths",    {{NoteValue::Eighth,    false, false}, {NoteValue::Eighth,    false, false}}},
        {"Four 16ths",  {{NoteValue::Sixteenth, false, false}, {NoteValue::Sixteenth, false, false},
                         {NoteValue::Sixteenth, false, false}, {NoteValue::Sixteenth, false, false}}},
        {"8th + 2x16th",{{NoteValue::Eighth,    false, false}, {NoteValue::Sixteenth, false, false},
                         {NoteValue::Sixteenth, false, false}}},
        {"2x16th + 8th",{{NoteValue::Sixteenth, false, false}, {NoteValue::Sixteenth, false, false},
                         {NoteValue::Eighth,    false, false}}},
        {"Triplets",    {{NoteValue::TripletEighth, false, false}, {NoteValue::TripletEighth, false, false},
                         {NoteValue::TripletEighth, false, false}}},
        {"Dot8th + 16th",{{NoteValue::DottedEighth, false, false}, {NoteValue::Sixteenth, false, false}}}
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
    m_noteButton->setEnabled(hasSelection);
    m_restButton->setEnabled(hasSelection);
    m_dottedButton->setEnabled(hasSelection);
    m_accentedButton->setEnabled(hasSelection);
    m_removeButton->setEnabled(hasSelection);

    if (hasSelection) {
        const SubdivisionPulse& pulse = m_pattern.pulses[m_selectedPulseIndex];
        auto [tileIdx, dotted, tupletN] = decomposePulseNoteValue(pulse.noteValue);

        // Sync tile selection
        m_selectedTileIndex = tileIdx;
        for (int i = 0; i < m_noteTiles.size(); ++i)
            m_noteTiles[i]->setSelected(i == tileIdx);

        // Sync tuplet buttons
        m_selectedTuplet = tupletN;
        if (m_tupletButtons[0]) m_tupletButtons[0]->setChecked(tupletN == 0);
        for (int n = 2; n <= 9; ++n)
            if (m_tupletButtons[n]) m_tupletButtons[n]->setChecked(n == tupletN);

        // Show only valid tuplet buttons for current tile
        if (m_tupletButtons[0]) m_tupletButtons[0]->setVisible(true);
        for (int n = 2; n <= 9; ++n)
            if (m_tupletButtons[n]) m_tupletButtons[n]->setVisible(isValidTuplet(tileIdx, n));

        // Dotted: only available when no tuplet and note supports it
        bool canDot = (tupletN == 0) && NOTE_VALUE_COMBO[tileIdx].canBeDotted;
        m_dottedButton->setEnabled(hasSelection && canDot);
        m_dottedButton->blockSignals(true);
        m_dottedButton->setChecked(dotted);
        m_dottedButton->blockSignals(false);

        m_noteButton->setChecked(!pulse.isRest);
        m_restButton->setChecked(pulse.isRest);
        m_accentedButton->setChecked(pulse.accent);
    } else {
        // No selection: keep × visible, hide number buttons
        if (m_tupletButtons[0]) m_tupletButtons[0]->setVisible(true);
        for (int n = 2; n <= 9; ++n)
            if (m_tupletButtons[n]) m_tupletButtons[n]->setVisible(isValidTuplet(m_selectedTileIndex, n));
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

    // Scale to fit the label width but cap height to 60px so the notes don't grow with the taller label
    QSize scaleTarget(m_previewLabel->width(), 60);
    QPixmap scaledPreview = preview.scaled(scaleTarget, Qt::KeepAspectRatio, Qt::SmoothTransformation);
    m_previewLabel->setPixmap(scaledPreview);
    m_previewLabel->setAlignment(Qt::AlignBottom | Qt::AlignHCenter);
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
    int  idx    = m_selectedTileIndex;
    bool dotted = m_dottedButton->isChecked() && m_selectedTuplet == 0;
    SubdivisionPulse newPulse;
    newPulse.noteValue = resolveActiveNoteValue(idx, dotted, m_selectedTuplet);
    newPulse.isRest    = m_restButton->isChecked();
    newPulse.accent    = m_accentedButton->isChecked();
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

void CustomSubdivisionDialog::onNoteTileClicked(int idx) {
    m_selectedTileIndex = idx;
    for (int i = 0; i < m_noteTiles.size(); ++i)
        m_noteTiles[i]->setSelected(i == idx);

    // If current tuplet is no longer valid for the new base, reset to ×
    if (!isValidTuplet(idx, m_selectedTuplet)) {
        m_selectedTuplet = 0;
        if (m_tupletButtons[0]) m_tupletButtons[0]->setChecked(true);
        for (int n = 2; n <= 9; ++n)
            if (m_tupletButtons[n]) m_tupletButtons[n]->setChecked(false);
    }
    // Show only valid tuplet buttons for the new tile
    if (m_tupletButtons[0]) m_tupletButtons[0]->setVisible(true);
    for (int n = 2; n <= 9; ++n)
        if (m_tupletButtons[n]) m_tupletButtons[n]->setVisible(isValidTuplet(idx, n));

    // Dotted
    bool canDot = (m_selectedTuplet == 0) && NOTE_VALUE_COMBO[idx].canBeDotted;
    bool hasSel = (m_selectedPulseIndex >= 0);
    m_dottedButton->setEnabled(hasSel && canDot);
    if (!canDot) {
        m_dottedButton->blockSignals(true);
        m_dottedButton->setChecked(false);
        m_dottedButton->blockSignals(false);
    }

    bool dotted = canDot && m_dottedButton->isChecked();
    if (m_selectedPulseIndex >= 0 && m_selectedPulseIndex < m_pattern.pulses.size()) {
        // A pulse is already selected — update it in place
        m_pattern.pulses[m_selectedPulseIndex].noteValue =
            resolveActiveNoteValue(idx, dotted, m_selectedTuplet);
        m_pulseWidgets[m_selectedPulseIndex]->setPulse(m_pattern.pulses[m_selectedPulseIndex]);
        updatePreview();
    } else {
        // Nothing selected — append a new pulse immediately
        SubdivisionPulse newPulse;
        newPulse.noteValue = resolveActiveNoteValue(idx, dotted, m_selectedTuplet);
        newPulse.isRest    = m_restButton->isChecked();
        newPulse.accent    = m_accentedButton->isChecked();
        m_pattern.pulses.append(newPulse);
        rebuildPattern();
        updateOkButtonState();
        selectPulse(m_pattern.pulses.size() - 1);
    }
}

void CustomSubdivisionDialog::onTupletButtonClicked(int tupletN) {
    // Toggle off if already selected (click same number again → back to ×)
    if (tupletN != 0 && m_selectedTuplet == tupletN)
        tupletN = 0;
    m_selectedTuplet = tupletN;

    // Update button checked states
    if (m_tupletButtons[0]) m_tupletButtons[0]->setChecked(tupletN == 0);
    for (int n = 2; n <= 9; ++n)
        if (m_tupletButtons[n]) m_tupletButtons[n]->setChecked(n == tupletN);

    // Dotted is disabled whenever a tuplet is active
    bool canDot = (tupletN == 0) && NOTE_VALUE_COMBO[m_selectedTileIndex].canBeDotted;
    bool hasSel = (m_selectedPulseIndex >= 0);
    m_dottedButton->setEnabled(hasSel && canDot);
    if (!canDot) {
        m_dottedButton->blockSignals(true);
        m_dottedButton->setChecked(false);
        m_dottedButton->blockSignals(false);
    }

    if (m_selectedPulseIndex >= 0 && m_selectedPulseIndex < m_pattern.pulses.size()) {
        bool dotted = canDot && m_dottedButton->isChecked();
        m_pattern.pulses[m_selectedPulseIndex].noteValue =
            resolveActiveNoteValue(m_selectedTileIndex, dotted, tupletN);
        m_pulseWidgets[m_selectedPulseIndex]->setPulse(m_pattern.pulses[m_selectedPulseIndex]);
        updatePreview();
    }
}

void CustomSubdivisionDialog::onTypeChanged() {
    if (m_selectedPulseIndex >= 0 && m_selectedPulseIndex < m_pattern.pulses.size()) {
        bool canDot = (m_selectedTuplet == 0) && NOTE_VALUE_COMBO[m_selectedTileIndex].canBeDotted;
        bool dotted = canDot && m_dottedButton->isChecked();
        m_pattern.pulses[m_selectedPulseIndex].noteValue =
            resolveActiveNoteValue(m_selectedTileIndex, dotted, m_selectedTuplet);
        m_pattern.pulses[m_selectedPulseIndex].isRest  = m_restButton->isChecked();
        m_pattern.pulses[m_selectedPulseIndex].accent  = m_accentedButton->isChecked();
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