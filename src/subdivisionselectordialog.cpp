#include "subdivisionselectordialog.h"
#include "noteassembler.h"
#include "customsubdivisiondialog.h"
#include <QVBoxLayout>
#include <QPixmap>
#include <QPainter>
#include <QEvent>
#include <QMouseEvent>
#include <QDialogButtonBox>
#include <QFont>
#include <QHBoxLayout>
#include <QTabWidget>
#include <QPushButton>
#include <QWidget>
#include <QLabel>
#include <QCheckBox>
#include <QSettings>
#include <QCoreApplication>
#include <QMessageBox>
#include <QInputDialog>
#include <QContextMenuEvent>
#include <QMenu>

namespace {

// Helper to build a SubdivisionPulse quickly
static SubdivisionPulse P(NoteValue nv, bool rest = false, bool accent = false) {
    return {nv, rest, accent};
}

QVector<SubdivisionPattern> getStandardPatterns(bool compound) {
    QVector<SubdivisionPattern> v;
    if (!compound) {
        v << SubdivisionPattern{SubdivisionCategory::Standard, "Quarter Note",
                {P(NoteValue::Quarter)}}
          << SubdivisionPattern{SubdivisionCategory::Standard, "Eighth Notes",
                {P(NoteValue::Eighth), P(NoteValue::Eighth)}}
          << SubdivisionPattern{SubdivisionCategory::Standard, "Sixteenth Notes",
                {P(NoteValue::Sixteenth), P(NoteValue::Sixteenth),
                 P(NoteValue::Sixteenth), P(NoteValue::Sixteenth)}}
          << SubdivisionPattern{SubdivisionCategory::Standard, "Eighth Triplets",
                {P(NoteValue::TripletEighth), P(NoteValue::TripletEighth),
                 P(NoteValue::TripletEighth)}}
          << SubdivisionPattern{SubdivisionCategory::Standard, "Sixteenth Triplets",
                {P(NoteValue::TripletSixteenth), P(NoteValue::TripletSixteenth),
                 P(NoteValue::TripletSixteenth), P(NoteValue::TripletSixteenth),
                 P(NoteValue::TripletSixteenth), P(NoteValue::TripletSixteenth)}};
    } else {
        v << SubdivisionPattern{SubdivisionCategory::Standard, "Dotted Quarter",
                {P(NoteValue::DottedQuarter)}}
          << SubdivisionPattern{SubdivisionCategory::Standard, "Eighths (3 per beat)",
                {P(NoteValue::Eighth), P(NoteValue::Eighth), P(NoteValue::Eighth)}}
          << SubdivisionPattern{SubdivisionCategory::Standard, "Sixteenths (6 per beat)",
                {P(NoteValue::Sixteenth), P(NoteValue::Sixteenth), P(NoteValue::Sixteenth),
                 P(NoteValue::Sixteenth), P(NoteValue::Sixteenth), P(NoteValue::Sixteenth)}};
    }
    return v;
}

QVector<SubdivisionPattern> getCompositePatterns(bool compound) {
    QVector<SubdivisionPattern> v;
    if (!compound) {
        // Each pattern sums to exactly one quarter-note beat
        v << SubdivisionPattern{SubdivisionCategory::Composite, "Eighth + Two Sixteenths",
                {P(NoteValue::Eighth), P(NoteValue::Sixteenth), P(NoteValue::Sixteenth)}}
          << SubdivisionPattern{SubdivisionCategory::Composite, "Two Sixteenths + Eighth",
                {P(NoteValue::Sixteenth), P(NoteValue::Sixteenth), P(NoteValue::Eighth)}}
          << SubdivisionPattern{SubdivisionCategory::Composite, "Sixteenth + Eighth + Sixteenth",
                {P(NoteValue::Sixteenth), P(NoteValue::Eighth), P(NoteValue::Sixteenth)}}
          << SubdivisionPattern{SubdivisionCategory::Composite, "Dotted Eighth + Sixteenth",
                {P(NoteValue::DottedEighth), P(NoteValue::Sixteenth)}}
          << SubdivisionPattern{SubdivisionCategory::Composite, "Sixteenth + Dotted Eighth",
                {P(NoteValue::Sixteenth), P(NoteValue::DottedEighth)}}
          << SubdivisionPattern{SubdivisionCategory::Composite, "Dotted Eighth + Two 32nds",
                {P(NoteValue::DottedEighth), P(NoteValue::ThirtySecond), P(NoteValue::ThirtySecond)}}
          << SubdivisionPattern{SubdivisionCategory::Composite, "Two 32nds + Dotted Eighth",
                {P(NoteValue::ThirtySecond), P(NoteValue::ThirtySecond), P(NoteValue::DottedEighth)}}
          << SubdivisionPattern{SubdivisionCategory::Composite, "32nd + Dotted Eighth + 32nd",
                {P(NoteValue::ThirtySecond), P(NoteValue::DottedEighth), P(NoteValue::ThirtySecond)}};
    } else {
        // Each pattern sums to exactly one dotted-quarter beat (= Quarter + Eighth)
        v << SubdivisionPattern{SubdivisionCategory::Composite, "Quarter + Eighth",
                {P(NoteValue::Quarter), P(NoteValue::Eighth)}}
          << SubdivisionPattern{SubdivisionCategory::Composite, "Eighth + Quarter",
                {P(NoteValue::Eighth), P(NoteValue::Quarter)}}
          << SubdivisionPattern{SubdivisionCategory::Composite, "Dotted Eighth + Sixteenth + Eighth",
                {P(NoteValue::DottedEighth), P(NoteValue::Sixteenth), P(NoteValue::Eighth)}}
          << SubdivisionPattern{SubdivisionCategory::Composite, "Sixteenth + Quarter + Sixteenth",
                {P(NoteValue::Sixteenth), P(NoteValue::Quarter), P(NoteValue::Sixteenth)}}
          << SubdivisionPattern{SubdivisionCategory::Composite, "Quarter + Two Sixteenths",
                {P(NoteValue::Quarter), P(NoteValue::Sixteenth), P(NoteValue::Sixteenth)}}
          << SubdivisionPattern{SubdivisionCategory::Composite, "Two Sixteenths + Quarter",
                {P(NoteValue::Sixteenth), P(NoteValue::Sixteenth), P(NoteValue::Quarter)}}
          << SubdivisionPattern{SubdivisionCategory::Composite, "Eighth + Dotted Eighth + Sixteenth",
                {P(NoteValue::Eighth), P(NoteValue::DottedEighth), P(NoteValue::Sixteenth)}}
          << SubdivisionPattern{SubdivisionCategory::Composite, "Sixteenth + Dotted Eighth + Eighth",
                {P(NoteValue::Sixteenth), P(NoteValue::DottedEighth), P(NoteValue::Eighth)}};
    }
    return v;
}

QVector<SubdivisionPattern> getTupletPatterns(bool compound) {
    QVector<SubdivisionPattern> v;
    if (!compound) {
        v << SubdivisionPattern{SubdivisionCategory::Tuplet, "Quintuplets",
                {P(NoteValue::QuintupletNote), P(NoteValue::QuintupletNote),
                 P(NoteValue::QuintupletNote), P(NoteValue::QuintupletNote),
                 P(NoteValue::QuintupletNote)}}
          << SubdivisionPattern{SubdivisionCategory::Tuplet, "Septuplets",
                {P(NoteValue::SeptupletNote), P(NoteValue::SeptupletNote),
                 P(NoteValue::SeptupletNote), P(NoteValue::SeptupletNote),
                 P(NoteValue::SeptupletNote), P(NoteValue::SeptupletNote),
                 P(NoteValue::SeptupletNote)}};
    } else {
        v << SubdivisionPattern{SubdivisionCategory::Tuplet, "Duplets",
                {P(NoteValue::DupletNote), P(NoteValue::DupletNote)}}
          << SubdivisionPattern{SubdivisionCategory::Tuplet, "Quartuplets",
                {P(NoteValue::QuartupletNote), P(NoteValue::QuartupletNote),
                 P(NoteValue::QuartupletNote), P(NoteValue::QuartupletNote)}}
          << SubdivisionPattern{SubdivisionCategory::Tuplet, "Quintuplets",
                {P(NoteValue::QuintupletNote), P(NoteValue::QuintupletNote),
                 P(NoteValue::QuintupletNote), P(NoteValue::QuintupletNote),
                 P(NoteValue::QuintupletNote)}};
    }
    return v;
}

QVector<SubdivisionPattern> getCustomPatterns(bool /*compound*/) { return {}; }

} // namespace

QString SubdivisionSelectorDialog::getCustomPatternsFilePath() const {
    return QCoreApplication::applicationDirPath() + "/data/custom_subdivisions.dat";
}

void SubdivisionSelectorDialog::loadCustomPatterns() {
    QString filePath = getCustomPatternsFilePath();
    QSettings settings(filePath, QSettings::IniFormat);

    int count = settings.beginReadArray("CustomPatterns");
    m_savedCustomPatterns.clear();

    for (int i = 0; i < count; ++i) {
        settings.setArrayIndex(i);
        SubdivisionPattern pattern;
        pattern.category = SubdivisionCategory::Custom;
        pattern.name = settings.value("name", QString("Custom %1").arg(i + 1)).toString();

        int pulseCount = settings.beginReadArray("pulses");
        for (int j = 0; j < pulseCount; ++j) {
            settings.setArrayIndex(j);
            SubdivisionPulse pulse;
            // New format: noteValue stored as string
            QString nvStr = settings.value("noteValue", "").toString();
            if (!nvStr.isEmpty()) {
                pulse.noteValue = noteValueFromString(nvStr);
            } else {
                // Legacy migration: infer NoteValue from old float fields
                double legacyDur    = settings.value("duration",  0.5).toDouble();
                bool   legacyDotted = settings.value("isDotted",  false).toBool();
                // Custom patterns are always in simple-time context for the selector
                pulse.noteValue = noteValueFromLegacy(legacyDur, legacyDotted, false);
            }
            pulse.isRest = settings.value("isRest",  false).toBool();
            pulse.accent = settings.value("accent",  false).toBool();
            pattern.pulses.append(pulse);
        }
        settings.endArray();
        
        if (!pattern.pulses.isEmpty()) {
            m_savedCustomPatterns.append(pattern);
        }
    }
    settings.endArray();
}

void SubdivisionSelectorDialog::saveCustomPatterns() {
    QString filePath = getCustomPatternsFilePath();
    QSettings settings(filePath, QSettings::IniFormat);

    settings.beginWriteArray("CustomPatterns");
    for (int i = 0; i < m_savedCustomPatterns.size(); ++i) {
        settings.setArrayIndex(i);
        const SubdivisionPattern& pattern = m_savedCustomPatterns[i];
        settings.setValue("name", pattern.name);

        settings.beginWriteArray("pulses");
        for (int j = 0; j < pattern.pulses.size(); ++j) {
            settings.setArrayIndex(j);
            const SubdivisionPulse& pulse = pattern.pulses[j];
            settings.setValue("noteValue", noteValueToString(pulse.noteValue));
            settings.setValue("isRest",    pulse.isRest);
            settings.setValue("accent",    pulse.accent);
        }
        settings.endArray();
    }
    settings.endArray();
    settings.sync();
}

void SubdivisionSelectorDialog::addCustomPattern(const SubdivisionPattern& pattern) {
    SubdivisionPattern namedPattern = pattern;
    namedPattern.category = SubdivisionCategory::Custom;
    // name was already set via chosenName() at the call site
    if (namedPattern.name.isEmpty())
        namedPattern.name = QString("Custom %1").arg(m_savedCustomPatterns.size() + 1);
    
    m_savedCustomPatterns.append(namedPattern);
    saveCustomPatterns();
    
    // Refresh the custom tab if it's currently showing
    if (m_currentTab == 3) {
        reloadPatternGrid(); // This will call updateDialogSizeForCustomPatterns()
    }
}

void SubdivisionSelectorDialog::editCustomPattern(int index) {
    if (index < 0 || index >= m_savedCustomPatterns.size()) return;
    
    CustomSubdivisionDialog dlg(this, m_metronomeEngine, m_numerator, m_denominator, m_compoundTime);
    dlg.setPattern(m_savedCustomPatterns[index]);
    
    if (dlg.exec() == QDialog::Accepted) {
        SubdivisionPattern newPattern = dlg.chosenPattern();
        newPattern.name = dlg.chosenName();
        m_savedCustomPatterns[index] = newPattern;
        saveCustomPatterns();
        reloadPatternGrid();
    }
}

void SubdivisionSelectorDialog::deleteCustomPattern(int index) {
    if (index < 0 || index >= m_savedCustomPatterns.size()) return;
    
    int ret = QMessageBox::question(this, "Delete Pattern", 
                                  QString("Delete pattern '%1'?").arg(m_savedCustomPatterns[index].name),
                                  QMessageBox::Yes | QMessageBox::No);
    
    if (ret == QMessageBox::Yes) {
        m_savedCustomPatterns.removeAt(index);
        saveCustomPatterns();
        reloadPatternGrid(); // This will call updateDialogSizeForCustomPatterns()
    }
}

NoteAssemblerConfig SubdivisionSelectorDialog::configForPattern(const SubdivisionPattern& pattern) const {
    return buildNoteAssemblerConfig(pattern);
}

SubdivisionSelectorDialog::SubdivisionSelectorDialog(QWidget* parent, bool compoundTime, int numerator, int denominator, MetronomeEngine* engine)
    : QDialog(parent), m_compoundTime(compoundTime), m_numerator(numerator), m_denominator(denominator), m_metronomeEngine(engine)
{
    setWindowTitle("Choose Subdivision");
    setModal(true);
    setMinimumWidth(600);
    setMaximumWidth(600);
    setFixedHeight(250);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    m_tabWidget = new QTabWidget(this);
    mainLayout->addWidget(m_tabWidget);

    m_patternsByTab = {
        getStandardPatterns(m_compoundTime),
        getCompositePatterns(m_compoundTime),
        getTupletPatterns(m_compoundTime),
        getCustomPatterns(m_compoundTime)
    };

    QStringList tabTitles = {"Standard", "Composite", "Tuplets", "Custom"};
    for (const auto& title : tabTitles) {
        QWidget* page = new QWidget;
        page->setLayout(new QVBoxLayout);
        m_tabWidget->addTab(page, title);
    }
    connect(m_tabWidget, &QTabWidget::currentChanged, this, &SubdivisionSelectorDialog::onTabChanged);

    m_patternGridWidget = new QWidget(this);
    m_patternGridLayout = new QGridLayout(m_patternGridWidget);
    m_patternGridLayout->setSpacing(12);
    m_patternGridLayout->setContentsMargins(16, 8, 16, 8);

    // Create and setup scroll area properly
    m_patternScrollArea = new QScrollArea(this);
    m_patternScrollArea->setWidgetResizable(true);
    m_patternScrollArea->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    m_patternScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
    m_patternScrollArea->setFrameStyle(QFrame::NoFrame);
    m_patternScrollArea->setWidget(m_patternGridWidget);

    // Add scroll area to the first tab (will be moved around in onTabChanged)
    m_tabWidget->widget(0)->layout()->addWidget(m_patternScrollArea);

    // Rest of your existing code...
    m_restBoxLayout = new QHBoxLayout;
    m_restBoxLayout->setSpacing(12);
    m_restBoxLayout->setContentsMargins(16, 8, 16, 8);
    mainLayout->addLayout(m_restBoxLayout);

    // Dialog buttons
    QHBoxLayout* dialogButtonLayout = new QHBoxLayout;
    m_addPatternButton = new QPushButton("New Pattern", this);
    m_addPatternButton->setVisible(false);

    m_okButton = new QPushButton("OK", this);
    m_okButton->setEnabled(false);

    dialogButtonLayout->addStretch();
    dialogButtonLayout->addWidget(m_addPatternButton);
    dialogButtonLayout->addWidget(m_okButton);

    mainLayout->addLayout(dialogButtonLayout);

    connect(m_okButton, &QPushButton::clicked, this, &SubdivisionSelectorDialog::onAcceptClicked);
    connect(m_addPatternButton, &QPushButton::clicked, this, &SubdivisionSelectorDialog::onPlusBoxClicked);

    loadCustomPatterns();
    reloadPatternGrid();
}

void SubdivisionSelectorDialog::onTabChanged(int tabIdx) {
    m_currentTab = tabIdx;
    m_currentPatternIdx = -1;
    m_okButton->setEnabled(false);

    // Show/hide Add Pattern button based on tab
    m_addPatternButton->setVisible(tabIdx == 3); // Only show on Custom tab

    // Move scroll area to current tab
    for (int i = 0; i < m_tabWidget->count(); ++i) {
        m_tabWidget->widget(i)->layout()->removeWidget(m_patternScrollArea);
    }
    m_tabWidget->widget(tabIdx)->layout()->addWidget(m_patternScrollArea);

    // Set fixed height for non-custom tabs
    if (tabIdx != 3) {
        setFixedHeight(250); // Set a fixed height for Standard, Composite, Tuplets tabs
        if (m_patternScrollArea) {
            m_patternScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
            m_patternScrollArea->setMaximumHeight(QWIDGETSIZE_MAX); // Remove height restriction
        }
    }

    reloadPatternGrid();
    reloadRestCheckboxes();
}

void SubdivisionSelectorDialog::reloadPatternGrid() {
    // Clear existing layout
    QLayoutItem* child;
    while ((child = m_patternGridLayout->takeAt(0)) != nullptr) {
        delete child->widget();
        delete child;
    }
    m_patternLabels.clear();

    if (m_currentTab == 3) {
        // Custom patterns tab - just show the saved patterns, no + button
        const int wrapAfter = 4;
        
        // Add saved custom patterns
        for (int i = 0; i < m_savedCustomPatterns.size(); ++i) {
            QWidget* patternWidget = new QWidget(this);
            patternWidget->setMaximumWidth(120);
            QVBoxLayout* patternLayout = new QVBoxLayout(patternWidget);
            patternLayout->setSpacing(2);
            patternLayout->setContentsMargins(2, 2, 2, 2);
            
            // Pattern preview
            QLabel* patternLabel = new QLabel(this);
            NoteAssembler assembler;
            NoteAssemblerConfig cfg = configForPattern(m_savedCustomPatterns[i]);
            cfg.pixmapSize = QSize(48, 48);
            
            // Check if we need to scale down to fit in the widget
            QPixmap originalPixmap = assembler.assembleNote(cfg);
            QPixmap finalPixmap;
            const int maxWidth = 110;
            const int fixedHeight = 72; // Tall enough for tuplet brackets above notes
            
            if (originalPixmap.width() > maxWidth) {
                // Calculate scale percentage to fit within maxWidth
                double scalePercent = (double)maxWidth / originalPixmap.width();
                int newWidth = maxWidth;
                int newHeight = (int)(originalPixmap.height() * scalePercent);
                
                finalPixmap = originalPixmap.scaled(newWidth, newHeight, Qt::IgnoreAspectRatio, Qt::SmoothTransformation);
            } else {
                finalPixmap = originalPixmap;
            }
            
            patternLabel->setPixmap(finalPixmap);
            patternLabel->setFixedSize(110, fixedHeight); // Fixed size for consistent alignment
            patternLabel->setScaledContents(false);
            patternLabel->setAlignment(Qt::AlignCenter);
            patternLabel->setToolTip(m_savedCustomPatterns[i].name);
            patternLabel->setCursor(Qt::PointingHandCursor);
            patternLayout->addWidget(patternLabel);
            
            // Pattern name
            QLabel* nameLabel = new QLabel(m_savedCustomPatterns[i].name, this);
            nameLabel->setAlignment(Qt::AlignCenter);
            nameLabel->setWordWrap(true);
            nameLabel->setMaximumWidth(116);
            QFont nameFont = nameLabel->font();
            nameFont.setPointSize(8);
            nameLabel->setFont(nameFont);
            patternLayout->addWidget(nameLabel);
            
            // Edit and Delete buttons
            QHBoxLayout* buttonLayout = new QHBoxLayout;
            QPushButton* editBtn = new QPushButton("Edit", this);
            QPushButton* deleteBtn = new QPushButton("Del", this);
            editBtn->setMaximumHeight(20);
            editBtn->setMaximumWidth(58);
            deleteBtn->setMaximumHeight(20);
            deleteBtn->setMaximumWidth(58);
            
            connect(editBtn, &QPushButton::clicked, this, [this, i]() { editCustomPattern(i); });
            connect(deleteBtn, &QPushButton::clicked, this, [this, i]() { deleteCustomPattern(i); });
            
            buttonLayout->addWidget(editBtn);
            buttonLayout->addWidget(deleteBtn);
            patternLayout->addLayout(buttonLayout);
            
            // Make the pattern clickable
            patternLabel->installEventFilter(this);
            m_patternLabels.append(patternLabel); // For click handling
            
            int row = i / wrapAfter;
            int col = i % wrapAfter;
            m_patternGridLayout->addWidget(patternWidget, row, col);
        }
        
        // Calculate and update dialog size based on number of rows
        updateDialogSizeForCustomPatterns();
        
        m_patternGridWidget->update();
        return;
    }


    const auto& patterns = m_patternsByTab[m_currentTab];
    const int wrapAfter = 5;
    for (int i = 0; i < patterns.size(); ++i) {
        QLabel* label = new QLabel(this);

        NoteAssembler assembler;
        // Use the same configForPattern logic as MainWindow
        NoteAssemblerConfig cfg = configForPattern(patterns[i]);
        cfg.pixmapSize = QSize(48, 48);
        label->setPixmap(assembler.assembleNote(cfg));

        label->setMinimumSize(cfg.pixmapSize);
        label->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);

        label->setToolTip(patterns[i].name);
        label->setCursor(Qt::PointingHandCursor);
        label->setAlignment(Qt::AlignCenter);

        QFont f = label->font();
        f.setPointSize(10);
        label->setFont(f);

        int row = i / wrapAfter;
        int col = i % wrapAfter;
        m_patternGridLayout->addWidget(label, row, col);
        m_patternLabels.append(label);

        label->installEventFilter(this);
    }

    for (int i = 0; i < m_patternLabels.size(); ++i) {
        if (i == m_currentPatternIdx) {
            m_patternLabels[i]->setStyleSheet(
                "border: 2px solid #181818; border-radius: 8px; background: #222;"
            );
        } else {
            m_patternLabels[i]->setStyleSheet("");
        }
    }

    m_patternGridWidget->update();
}

void SubdivisionSelectorDialog::reloadRestCheckboxes() {
    QLayoutItem* item;
    while ((item = m_restBoxLayout->takeAt(0))) {
        if (item->widget()) delete item->widget();
        delete item;
    }
    m_restBoxes.clear();

    if (m_currentPatternIdx < 0 || m_currentTab > 2) {
        setFixedHeight(250);
        return;
    }

    const auto& pattern = m_patternsByTab[m_currentTab][m_currentPatternIdx];
    for (int i = 0; i < pattern.pulses.size(); ++i) {
        QCheckBox* box = new QCheckBox(QString("Rest %1").arg(i+1), this);
        box->setChecked(pattern.pulses[i].isRest);
        connect(box, &QCheckBox::toggled, this, [this, i](bool checked) { onRestBoxChanged(i, checked); });
        m_restBoxLayout->addWidget(box);
        m_restBoxes.append(box);
    }

    if (!m_restBoxes.isEmpty()) {
        setFixedHeight(290);
    }
}

void SubdivisionSelectorDialog::onPatternClicked(int patternIdx) {
    m_currentPatternIdx = patternIdx;
    m_okButton->setEnabled(true);

    if (m_currentTab == 3) {
        // Custom patterns
        m_chosenPattern = m_savedCustomPatterns[patternIdx];
    } else {
        // Standard patterns
        m_chosenPattern = m_patternsByTab[m_currentTab][patternIdx];
        reloadRestCheckboxes();
    }

    // Update visual selection
    for (int i = 0; i < m_patternLabels.size(); ++i) {
        if (i == patternIdx) {
            m_patternLabels[i]->setStyleSheet(
                "border: 2px solid #181818; border-radius: 8px; background: #222;"
            );
        } else {
            m_patternLabels[i]->setStyleSheet("");
        }
    }
}

void SubdivisionSelectorDialog::onRestBoxChanged(int pulseIdx, bool checked) {
    if (m_currentPatternIdx < 0 || m_currentTab > 2) return;
    m_chosenPattern.pulses[pulseIdx].isRest = checked;

    if (m_currentPatternIdx >= 0 && m_currentPatternIdx < m_patternLabels.size()) {
        NoteAssembler assembler;
        NoteAssemblerConfig cfg = configForPattern(m_chosenPattern);
        cfg.pixmapSize = QSize(48, 48);
        m_patternLabels[m_currentPatternIdx]->setPixmap(assembler.assembleNote(cfg));
    }
}

void SubdivisionSelectorDialog::onAcceptClicked() {
    if (m_currentTab == 3 && !m_chosenPattern.pulses.isEmpty()) {
        accept();
    } else if (m_currentPatternIdx >= 0) {
        accept();
    }
}

void SubdivisionSelectorDialog::onPlusBoxClicked() {
    CustomSubdivisionDialog dlg(this, m_metronomeEngine, m_numerator, m_denominator, m_compoundTime);
    if (dlg.exec() == QDialog::Accepted) {
        SubdivisionPattern pattern = dlg.chosenPattern();
        pattern.name = dlg.chosenName();
        addCustomPattern(pattern);
        m_chosenPattern = pattern;
        m_okButton->setEnabled(true);
    }
}

bool SubdivisionSelectorDialog::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::MouseButtonRelease) {
        for (int i = 0; i < m_patternLabels.size(); ++i) {
            if (obj == m_patternLabels[i]) {
                onPatternClicked(i);
                return true;
            }
        }
    }
    return QDialog::eventFilter(obj, event);
}


void SubdivisionSelectorDialog::updateDialogSizeForCustomPatterns() {
    if (m_currentTab != 3) return;
    
    const int wrapAfter = 4;
    const int patternCount = m_savedCustomPatterns.size();
    const int numRows = (patternCount + wrapAfter - 1) / wrapAfter; // Ceiling division
    
    // More accurate height calculations
    const int patternImageHeight = 72;
    const int patternNameHeight = 20;
    const int buttonHeight = 20;
    const int patternSpacing = 10; // Spacing between elements
    const int patternRowHeight = patternImageHeight + patternNameHeight + buttonHeight + patternSpacing;
    
    const int baseHeight = 150; // Base height for dialog (tabs, buttons, etc.)
    const int maxRows = 3;
    const int scrollAreaPadding = 10;
    
    if (m_patternScrollArea) {
        if (numRows <= maxRows && numRows > 0) {
            // No scroll needed - resize dialog to fit exactly
            int contentHeight = numRows * patternRowHeight;
            int newDialogHeight = baseHeight + contentHeight;
            
            setFixedHeight(newDialogHeight); // Use setFixedHeight instead of resize
            m_patternScrollArea->setMaximumHeight(contentHeight + scrollAreaPadding);
            m_patternScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        } else if (numRows > maxRows) {
            // Enable scrolling for more than 3 rows
            int maxContentHeight = maxRows * patternRowHeight;
            int maxDialogHeight = baseHeight + maxContentHeight;
            
            setFixedHeight(maxDialogHeight); // Use setFixedHeight instead of resize
            m_patternScrollArea->setMaximumHeight(maxContentHeight + scrollAreaPadding);
            m_patternScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAsNeeded);
        } else {
            // No patterns - minimal size
            setFixedHeight(baseHeight + 100); // Use setFixedHeight instead of resize
            m_patternScrollArea->setMaximumHeight(100);
            m_patternScrollArea->setVerticalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
        }
    }
}