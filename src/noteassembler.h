#pragma once

#include <QString>
#include <QPixmap>
#include <QColor>
#include <QSize>
#include <vector>

enum class AssembledNoteType {
    Quarter,
    Eighth,
    Sixteenth,
    ThirtySecond,
    SixtyFourth,
    Half,
    Whole,
    Rest_Quarter,
    Rest_Eighth,
    Rest_Sixteenth,
    Rest_ThirtySecond,
    Rest_SixtyFourth
};

struct NoteAssemblerConfig {
    AssembledNoteType noteType;
    bool dotted = false;
    bool beamed = false;
    int tupletNumber = 0; // 0 = none, 3 = triplet, etc.
    int noteCount = 1;
    bool centerVertically = false;

    QColor color = QColor("#cfcfcf");
    QSize pixmapSize = QSize(48, 48); // This will be set dynamically for fluid width

    // NEW: Use this to specify which notes are dotted (true = dot after this note)
    std::vector<bool> dottedNotes;
    std::vector<AssembledNoteType> noteTypes; // NEW: per-note types in group
};

class NoteAssembler {
public:
    NoteAssembler(const QString& svgResourcePrefix = ":/resources/svg");
    QPixmap assembleNote(const NoteAssemblerConfig& config);

private:
    QString svgForNotehead(AssembledNoteType type) const;
    QString svgForFlag(AssembledNoteType type) const;
    QString svgForRest(AssembledNoteType type) const;
    QString svgForDot() const { return m_prefix + "dot.svg"; }
    QString svgForStem() const { return m_prefix + "stem_up.svg"; }
    QString svgForBeam(int count = 1) const;
    QString svgForTupletNumber(int n) const;
    
    // Keep only one version of defaultSpacingForNotes
    double defaultSpacingForNotes(int noteCount, const QSize& pixmapSize = QSize(48, 48));

    QString m_prefix;
};