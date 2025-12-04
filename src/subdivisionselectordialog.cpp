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
#include <QStandardPaths>
#include <QDir>
#include <QMessageBox>
#include <QInputDialog>
#include <QContextMenuEvent>
#include <QMenu>

namespace {

// Example patterns for demo. Expand as needed!
// Update the pattern definitions in subdivisionselectordialog.cpp

QVector<SubdivisionPattern> getStandardPatterns(bool compound) {
    QVector<SubdivisionPattern> v;
    if (!compound) {
        v << SubdivisionPattern{SubdivisionCategory::Standard, "Quarter Note", {{1.0, false, false, false}}}
          << SubdivisionPattern{SubdivisionCategory::Standard, "Eighth Notes", {{0.5, false, false, false}, {0.5, false, false, false}}}
          << SubdivisionPattern{SubdivisionCategory::Standard, "Sixteenth Notes", {{0.25, false, false, false}, {0.25, false, false, false}, {0.25, false, false, false}, {0.25, false, false, false}}}
          << SubdivisionPattern{SubdivisionCategory::Standard, "Eighth Triplets", {{1.0/3, false, false, false}, {1.0/3, false, false, false}, {1.0/3, false, false, false}}};
    } else {
        v << SubdivisionPattern{SubdivisionCategory::Standard, "Dotted Quarter", {{1.0, false, false, true}}}
          << SubdivisionPattern{SubdivisionCategory::Standard, "Eighths (3 per beat)", {{1.0/3, false, false, false}, {1.0/3, false, false, false}, {1.0/3, false, false, false}}}
          << SubdivisionPattern{SubdivisionCategory::Standard, "Sixteenths (6 per beat)", {{1.0/6, false, false, false}, {1.0/6, false, false, false}, {1.0/6, false, false, false}, {1.0/6, false, false, false}, {1.0/6, false, false, false}, {1.0/6, false, false, false}}};
    }
    return v;
}

QVector<SubdivisionPattern> getCompositePatterns(bool compound) {
    QVector<SubdivisionPattern> v;
    if (!compound) {
        v << SubdivisionPattern{SubdivisionCategory::Composite, "Eighth + Two Sixteenths", {{0.5, false, false, false}, {0.25, false, false, false}, {0.25, false, false, false}}}
          << SubdivisionPattern{SubdivisionCategory::Composite, "Two Sixteenths + Eighth", {{0.25, false, false, false}, {0.25, false, false, false}, {0.5, false, false, false}}}
          << SubdivisionPattern{SubdivisionCategory::Composite, "Sixteenth + Eighth + Sixteenth", {{0.25, false, false, false}, {0.5, false, false, false}, {0.25, false, false, false}}}
          << SubdivisionPattern{SubdivisionCategory::Composite, "Dotted Eighth + Sixteenth", {{0.75, false, false, true}, {0.25, false, false, false}}}
          << SubdivisionPattern{SubdivisionCategory::Composite, "Sixteenth + Dotted Eighth", {{0.25, false, false, false}, {0.75, false, false, true}}};
    } else {
        v
          // CORRECTED: In compound time, use dotted eighth + eighth instead of "quarter"
          << SubdivisionPattern{SubdivisionCategory::Composite, "Dotted Eighth + Eighth", {{2.0/3, false, false, true}, {1.0/3, false, false, false}}}
          << SubdivisionPattern{SubdivisionCategory::Composite, "Eighth + Dotted Eighth", {{1.0/3, false, false, false}, {2.0/3, false, false, true}}}
          << SubdivisionPattern{SubdivisionCategory::Composite, "Dotted Eighth + Sixteenth", {{0.5, false, false, true}, {1.0/6, false, false, false}}}
          << SubdivisionPattern{SubdivisionCategory::Composite, "Sixteenth + Dotted Eighth", {{1.0/6, false, false, false}, {0.5, false, false, true}}};
    }
    return v;
}

QVector<SubdivisionPattern> getTupletPatterns(bool compound) {
    QVector<SubdivisionPattern> v;
    if (!compound) {
        v << SubdivisionPattern{SubdivisionCategory::Tuplet, "Quintuplets", {{0.2, false, false, false}, {0.2, false, false, false}, {0.2, false, false, false}, {0.2, false, false, false}, {0.2, false, false, false}}}
          << SubdivisionPattern{SubdivisionCategory::Tuplet, "Septuplets", {{1.0/7, false, false, false}, {1.0/7, false, false, false}, {1.0/7, false, false, false}, {1.0/7, false, false, false}, {1.0/7, false, false, false}, {1.0/7, false, false, false}, {1.0/7, false, false, false}}};
    } else {
        v << SubdivisionPattern{SubdivisionCategory::Tuplet, "Quintuplets", {{0.2, false, false, false}, {0.2, false, false, false}, {0.2, false, false, false}, {0.2, false, false, false}, {0.2, false, false, false}}};
    }
    return v;
}

QVector<SubdivisionPattern> getCustomPatterns(bool /*compound*/) {
    // For future: let user create their own
    return {};
}

} // namespace

QString SubdivisionSelectorDialog::getCustomPatternsFilePath() const {
    QString appDataPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(appDataPath);
    return appDataPath + "/custom_subdivisions.dat";
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
    pulse.duration = settings.value("duration", 0.5).toDouble();
    pulse.isRest = settings.value("isRest", false).toBool();
    pulse.accent = settings.value("accent", false).toBool(); // NEW: Load accent
    pulse.isDotted = settings.value("isDotted", false).toBool();
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
    settings.setValue("duration", pulse.duration);
    settings.setValue("isRest", pulse.isRest);
    settings.setValue("accent", pulse.accent); // NEW: Save accent
    settings.setValue("isDotted", pulse.isDotted);
}
        settings.endArray();
    }
    settings.endArray();
    settings.sync();
}

void SubdivisionSelectorDialog::addCustomPattern(const SubdivisionPattern& pattern) {
    // Ask user for a name
    bool ok;
    QString name = QInputDialog::getText(this, "Save Custom Pattern", 
                                       "Pattern name:", QLineEdit::Normal, 
                                       "Custom Pattern", &ok);
    if (!ok || name.isEmpty()) return;
    
    SubdivisionPattern namedPattern = pattern;
    namedPattern.name = name;
    namedPattern.category = SubdivisionCategory::Custom;
    
    m_savedCustomPatterns.append(namedPattern);
    saveCustomPatterns();
    
    // Refresh the custom tab if it's currently showing
    if (m_currentTab == 3) {
        reloadPatternGrid(); // This will call updateDialogSizeForCustomPatterns()
    }
}

void SubdivisionSelectorDialog::editCustomPattern(int index) {
    if (index < 0 || index >= m_savedCustomPatterns.size()) return;
    
    CustomSubdivisionDialog dlg(this);
    dlg.setPattern(m_savedCustomPatterns[index]);
    
    if (dlg.exec() == QDialog::Accepted) {
        SubdivisionPattern newPattern = dlg.chosenPattern();
        
        // Ask for new name
        bool ok;
        QString name = QInputDialog::getText(this, "Edit Pattern Name", 
                                           "Pattern name:", QLineEdit::Normal, 
                                           m_savedCustomPatterns[index].name, &ok);
        if (ok && !name.isEmpty()) {
            newPattern.name = name;
            m_savedCustomPatterns[index] = newPattern;
            saveCustomPatterns();
            reloadPatternGrid();
        }
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
    NoteAssemblerConfig cfg;
    cfg.pixmapSize = QSize(48, 48);

    cfg.noteCount = pattern.pulses.size();
    cfg.noteTypes.clear();
    cfg.dottedNotes.clear();

    auto isClose = [](double a, double b) { return std::abs(a - b) < 1e-6; }; // Tighter tolerance

    // Use the time signature from the dialog
    bool isCompoundTime = m_compoundTime;

    // --- Ceiling logic for tuplets/triplets ---
    auto ceilingNoteTypeForDuration = [isCompoundTime](double dur) {
        if (isCompoundTime) {
            if (dur <= 1.0/6)        return AssembledNoteType::Sixteenth;
            if (dur <= 1.0/3)        return AssembledNoteType::Eighth;
            if (dur <= 2.0/3)        return AssembledNoteType::Eighth;        // 2/3 = dotted eighth (eighth with dot)
            return AssembledNoteType::Quarter;                                 // 1.0 = dotted quarter (quarter with dot)
        } else {
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
            if (dur <= 2.0/3)        return AssembledNoteType::Rest_Eighth;        // 2/3 = dotted eighth rest
            return AssembledNoteType::Rest_Quarter;                              // 1.0 = dotted quarter rest
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
                    else if (isClose(p.duration, 2.0/3)) cfg.noteTypes.push_back(AssembledNoteType::Rest_Eighth);    // 2/3 = dotted eighth rest
                    else if (isClose(p.duration, 0.5))   cfg.noteTypes.push_back(AssembledNoteType::Rest_Eighth);
                    else if (isClose(p.duration, 1.0))   cfg.noteTypes.push_back(AssembledNoteType::Rest_Quarter);  // 1.0 = dotted quarter rest
                    else if (p.duration < 1.0/3)         cfg.noteTypes.push_back(AssembledNoteType::Rest_Sixteenth);
                    else if (p.duration < 2.0/3)         cfg.noteTypes.push_back(AssembledNoteType::Rest_Eighth);
                    else                                 cfg.noteTypes.push_back(AssembledNoteType::Rest_Quarter);
                } else {
                    // Simple time rest logic
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
                    // Simple time note logic
                    if (isClose(p.duration, 1.0/3))      cfg.noteTypes.push_back(AssembledNoteType::Eighth);
                    else if (isClose(p.duration, 0.25))  cfg.noteTypes.push_back(AssembledNoteType::Sixteenth);
                    else if (isClose(p.duration, 0.125)) cfg.noteTypes.push_back(AssembledNoteType::ThirtySecond);
                    else if (isClose(p.duration, 0.5))   cfg.noteTypes.push_back(AssembledNoteType::Eighth);
                    else if (isClose(p.duration, 0.75))  cfg.noteTypes.push_back(AssembledNoteType::Eighth);
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

SubdivisionSelectorDialog::SubdivisionSelectorDialog(QWidget* parent, bool compoundTime, int numerator, int denominator)
    : QDialog(parent), m_compoundTime(compoundTime), m_numerator(numerator), m_denominator(denominator)
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
            
            QPixmap originalPixmap = assembler.assembleNote(cfg);
            
            // Check if we need to scale down to fit in the widget
            QPixmap finalPixmap;
            const int maxWidth = 110; // Leave some margin within the 120px widget
            const int fixedHeight = 48; // Keep consistent height for alignment
            
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

    if (m_currentPatternIdx < 0 || m_currentTab > 2) return;

    const auto& pattern = m_patternsByTab[m_currentTab][m_currentPatternIdx];
    for (int i = 0; i < pattern.pulses.size(); ++i) {
        QCheckBox* box = new QCheckBox(QString("Rest %1").arg(i+1), this);
        box->setChecked(pattern.pulses[i].isRest);
        connect(box, &QCheckBox::toggled, this, [this, i](bool checked) { onRestBoxChanged(i, checked); });
        m_restBoxLayout->addWidget(box);
        m_restBoxes.append(box);
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
    CustomSubdivisionDialog dlg(this);
    if (dlg.exec() == QDialog::Accepted) {
        SubdivisionPattern pattern = dlg.chosenPattern();
        addCustomPattern(pattern); // This will ask for name and save
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
    const int patternImageHeight = 48;
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