#pragma once

#include <QString>
#include <QVector>
#include "noteassembler.h" // for AssembledNoteType and NoteAssemblerConfig

// ---------------------------------------------------------------------------
// NoteValue: the canonical identity of a note/rest duration.
// Storing this directly – rather than a raw float – eliminates all
// reverse-engineering (fuzzy float comparisons) in rendering and playback.
// ---------------------------------------------------------------------------
enum class NoteValue {
    // Standard values
    Whole,
    Half,
    DottedHalf,
    Quarter,
    DottedQuarter,
    Eighth,
    DottedEighth,
    Sixteenth,
    DottedSixteenth,
    ThirtySecond,
    SixtyFourth,

    // Triplets  (3 in the time of 2 of the next larger value)
    TripletQuarter,    // 3 in time of a half note  →  each = 1/6  whole
    TripletEighth,     // 3 in time of a quarter    →  each = 1/12 whole
    TripletSixteenth,  // 3 in time of an eighth    →  each = 1/24 whole

    // Beat-relative tuplets  (N equal notes filling exactly one beat)
    QuintupletNote,    // 5 per beat
    SeptupletNote,     // 7 per beat

    // Compound-time tuplets (N equal notes in the time of 3 – against the natural compound subdivision)
    DupletNote,        // 2 in the time of 3 (compound)
    QuartupletNote,    // 4 in the time of 3 (compound)

    // ── Expanded tuplets (2-9) × base note ─────────────────────────────────
    // All are beat-relative: N notes fill exactly one beat (beatDivisor = N).
    // Glyph choice is purely visual — the number in the tuplet bracket is
    // what determines the rhythm.

    // Duplets (2 per beat)
    DupletQuarter,    DupletEighth,    DupletSixteenth,
    // Triplets – 32nd added; Quarter/Eighth/Sixteenth already declared above
    TripletThirtySecond,
    // Quadruplets (4 per beat)
    QuadrupletQuarter,  QuadrupletEighth,  QuadrupletSixteenth,
    // Quintuplets (5 per beat)
    QuintupletQuarter,  QuintupletEighth,  QuintupletSixteenth,
    // Sextuplets (6 per beat)
    SextupletQuarter,   SextupletEighth,   SextupletSixteenth,
    // Septuplets (7 per beat)
    SeptupletQuarter,   SeptupletEighth,   SeptupletSixteenth,
    // Octuplets (8 per beat)
    OctupletEighth,     OctupletSixteenth,
    // Nonuplets (9 per beat)
    NonupletEighth,     NonupletSixteenth,
};

// ---------------------------------------------------------------------------
// Per-NoteValue rendering + duration metadata (returned by getNoteValueInfo)
// ---------------------------------------------------------------------------
struct NoteValueInfo {
    AssembledNoteType noteType;  // glyph to render  (dotted notes use their base glyph + dot)
    bool              dotted;    // draw augmentation dot
    int               tupletNumber; // 0 = no marker; 3 = triplet; 5 = quint; 7 = sept
    double            wholeNoteFraction; // absolute duration as fraction of a whole note
                                         // < 0  ⟹  beat-relative (use beatDivisor)
    int               beatDivisor;       // if wholeNoteFraction < 0:  duration = 1/beatDivisor beat
                                         // else 0
};

// Return metadata for any NoteValue
NoteValueInfo getNoteValueInfo(NoteValue nv);

// Return fraction of ONE beat this NoteValue occupies.
//   compoundTime = true  → beat = dotted quarter (3/8 of whole)
//   compoundTime = false → beat = quarter        (1/4 of whole)
double noteValueBeatFraction(NoteValue nv, bool compoundTime);

// Infer NoteValue from the legacy float-based format (used only when migrating
// old presets/custom patterns that predate this system).
//   beatFraction  – pulse.duration from old format (fraction of one beat, dot already included)
//   isDotted      – old pulse.isDotted flag
//   compoundTime  – whether the section was in compound time
NoteValue noteValueFromLegacy(double beatFraction, bool isDotted, bool compoundTime);

// String ↔ NoteValue helpers (for JSON serialization)
QString   noteValueToString(NoteValue nv);
NoteValue noteValueFromString(const QString& s);

// Resolve (base plain note + tuplet number) → specific NoteValue enum entry.
// base must be one of the seven plain values (no dot, no tuplet).
// tupletN = 0 → returns base unchanged.
// Unsupported combinations (e.g. Whole + 5) → returns base unchanged.
NoteValue resolveTupletNoteValue(NoteValue base, int tupletN);

// ---------------------------------------------------------------------------
// SubdivisionCategory / SubdivisionPulse / SubdivisionPattern
// ---------------------------------------------------------------------------
enum class SubdivisionCategory {
    Standard,
    Composite,
    Tuplet,
    Custom
};

struct SubdivisionPulse {
    NoteValue noteValue = NoteValue::Quarter;
    bool      isRest   = false;
    bool      accent   = false;
};

struct SubdivisionPattern {
    SubdivisionCategory category = SubdivisionCategory::Standard;
    QString             name;
    QVector<SubdivisionPulse> pulses;
};

// ---------------------------------------------------------------------------
// Build a NoteAssemblerConfig from a SubdivisionPattern.
// This is THE single authoritative rendering function – replaces all three
// copies of configForPattern() that existed previously.
// ---------------------------------------------------------------------------
NoteAssemblerConfig buildNoteAssemblerConfig(const SubdivisionPattern& pattern);