#pragma once

#include <QDialog>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QWidget>
#include <QScrollArea>
#include <QButtonGroup>
#include <QSpinBox>
#include <QGroupBox>
#include <QMouseEvent>
#include <QPaintEvent>
#include <QGridLayout>
#include <QLineEdit>
#include "subdivisionpattern.h"
#include "noteassembler.h"

class NoteTileWidget;  // defined in .cpp

class SubdivisionPulseWidget : public QWidget {
    Q_OBJECT
public:
    explicit SubdivisionPulseWidget(const SubdivisionPulse& pulse, QWidget* parent = nullptr);

    SubdivisionPulse getPulse() const { return m_pulse; }
    void setPulse(const SubdivisionPulse& pulse);

    void setSelected(bool selected);
    bool isSelected() const { return m_selected; }

signals:
    void clicked();
    void changed();

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;

private:
    SubdivisionPulse m_pulse;
    bool m_selected = false;

    void updateToolTip();
};

class CustomSubdivisionDialog : public QDialog {
    Q_OBJECT
public:
    explicit CustomSubdivisionDialog(QWidget* parent = nullptr);

    void setPattern(const SubdivisionPattern& pattern);
    SubdivisionPattern chosenPattern() const { return m_pattern; }
    QString chosenName() const;

private slots:
    void onAddPulse();
    void onRemoveSelected();
    void onPulseClicked();
    void onNoteTileClicked(int idx);
    void onTupletButtonClicked(int tupletN);
    void onTypeChanged();
    void onClearAll();
    void onPresetSelected();
    void onOkClicked();

private:
    void rebuildPattern();
    void updateControls();
    void updatePreview();
    void selectPulse(int index);
    void updateOkButtonState();

    NoteAssemblerConfig configForPattern(const SubdivisionPattern& pattern) const;

    SubdivisionPattern m_pattern;
    int m_selectedPulseIndex = -1;

    // UI elements
    QWidget* m_pulseContainer;
    QHBoxLayout* m_pulseLayout;
    QScrollArea* m_scrollArea;

    QVector<NoteTileWidget*> m_noteTiles;
    int m_selectedTileIndex = 4; // Default to Quarter (index in 7-item NOTE_VALUE_COMBO)
    // Tuplet button row: index = tupletN (0=×, 1 unused, 2–9)
    QVector<QPushButton*> m_tupletButtons; // size 10
    int m_selectedTuplet = 0;
    QPushButton* m_noteButton;
    QPushButton* m_restButton;
    QPushButton* m_dottedButton;
    QPushButton* m_accentedButton;
    QButtonGroup* m_typeGroup;

    QPushButton* m_addButton;
    QPushButton* m_removeButton;
    QPushButton* m_clearButton;

    QLabel* m_previewLabel;
    QPushButton* m_okButton;
    QLineEdit* m_nameEdit;

    QVector<SubdivisionPulseWidget*> m_pulseWidgets;

    // Common presets
    void setupPresets();
    QHBoxLayout* m_presetLayout;
};