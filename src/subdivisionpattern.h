#pragma once

#include <QString>
#include <QVector>

enum class SubdivisionCategory {
    Standard,
    Composite,
    Tuplet,
    Custom
};

struct SubdivisionPulse {
    double duration; // Relative to beat, e.g. 0.25 for sixteenth
    bool isRest;
    bool accent = false; // NEW: Add this field for custom subdivision accents
    bool isDotted = false;
};

struct SubdivisionPattern {
    SubdivisionCategory category;
    QString name;
    QVector<SubdivisionPulse> pulses;
};