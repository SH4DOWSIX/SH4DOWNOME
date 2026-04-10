#pragma once

#include <QWidget>
#include <QVector>
#include <QPushButton>
#include <QCheckBox>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QLabel>
#include <QComboBox>
#include <QGridLayout>
#include "subdivisionpattern.h"
#include <QScrollArea>

class PatternEditorWidget : public QWidget {
    Q_OBJECT
public:
    explicit PatternEditorWidget(QWidget* parent = nullptr);

    void setPattern(const SubdivisionPattern& pattern, double baseDuration);
    SubdivisionPattern currentPattern() const;

signals:
    void patternChanged();

private slots:
    void onAddPulse();
    void onRemovePulse();
    void onPulseChanged();
    void onDurationChanged(int idx);

private:
    struct PulseControls {
        QComboBox* durationCombo;
        QCheckBox* noteCheck;
        QCheckBox* dotCheck;
        QPushButton* removeBtn;
        QWidget* widget;
    };

    QVector<PulseControls> pulseControls;
    double m_baseDuration = 0.25;

    QVBoxLayout* mainLayout;
    QScrollArea* pulseScrollArea;
    QWidget* pulseGridWidget;
    QGridLayout* gridLayout;
    QPushButton* addPulseBtn;

    QList<NoteValue> baseNoteValues;
    QString noteValueLabel(NoteValue nv) const;
    NoteValue noteValueForIndex(int idx) const;
    int indexForBaseNoteValue(NoteValue nv) const;

    void rebuildUI();
    void syncToControls();
};