#include "subdivisionpattern.h"
#include <cmath>
#include <QMap>

// ---------------------------------------------------------------------------
// NoteValue lookup table
// ---------------------------------------------------------------------------
NoteValueInfo getNoteValueInfo(NoteValue nv) {
    // { noteType,                          dotted,  tuplet, wholeNoteFraction, beatDivisor }
    switch (nv) {
    case NoteValue::Whole:            return { AssembledNoteType::Whole,        false, 0,  1.0,        0 };
    case NoteValue::Half:             return { AssembledNoteType::Half,         false, 0,  0.5,        0 };
    case NoteValue::DottedHalf:       return { AssembledNoteType::Half,         true,  0,  0.75,       0 };
    case NoteValue::Quarter:          return { AssembledNoteType::Quarter,      false, 0,  0.25,       0 };
    case NoteValue::DottedQuarter:    return { AssembledNoteType::Quarter,      true,  0,  0.375,      0 };
    case NoteValue::Eighth:           return { AssembledNoteType::Eighth,       false, 0,  0.125,      0 };
    case NoteValue::DottedEighth:     return { AssembledNoteType::Eighth,       true,  0,  0.1875,     0 };
    case NoteValue::Sixteenth:        return { AssembledNoteType::Sixteenth,    false, 0,  0.0625,     0 };
    case NoteValue::DottedSixteenth:  return { AssembledNoteType::Sixteenth,    true,  0,  0.09375,    0 };
    case NoteValue::ThirtySecond:     return { AssembledNoteType::ThirtySecond, false, 0,  0.03125,    0 };
    case NoteValue::SixtyFourth:      return { AssembledNoteType::SixtyFourth,  false, 0,  0.015625,   0 };
    case NoteValue::TripletQuarter:   return { AssembledNoteType::Quarter,      false, 3,  1.0/6.0,    0 };
    case NoteValue::TripletEighth:    return { AssembledNoteType::Eighth,       false, 3,  1.0/12.0,   0 };
    case NoteValue::TripletSixteenth: return { AssembledNoteType::Sixteenth,    false, 3,  1.0/24.0,   0 };
    case NoteValue::QuintupletNote:   return { AssembledNoteType::Sixteenth,    false, 5,  -1.0,       5 };
    case NoteValue::SeptupletNote:    return { AssembledNoteType::Sixteenth,    false, 7,  -1.0,       7 };
    case NoteValue::DupletNote:       return { AssembledNoteType::Quarter,      false, 2,  -1.0,       2 };
    case NoteValue::QuartupletNote:   return { AssembledNoteType::Sixteenth,    false, 4,  -1.0,       4 };
    // ── Expanded tuplets ────────────────────────────────────────────────────
    case NoteValue::DupletQuarter:       return { AssembledNoteType::Quarter,      false, 2, -1.0, 2 };
    case NoteValue::DupletEighth:        return { AssembledNoteType::Eighth,       false, 2, -1.0, 2 };
    case NoteValue::DupletSixteenth:     return { AssembledNoteType::Sixteenth,    false, 2, -1.0, 2 };
    case NoteValue::TripletThirtySecond: return { AssembledNoteType::ThirtySecond, false, 3, -1.0, 3 };
    case NoteValue::QuadrupletQuarter:   return { AssembledNoteType::Quarter,      false, 4, -1.0, 4 };
    case NoteValue::QuadrupletEighth:    return { AssembledNoteType::Eighth,       false, 4, -1.0, 4 };
    case NoteValue::QuadrupletSixteenth: return { AssembledNoteType::Sixteenth,    false, 4, -1.0, 4 };
    case NoteValue::QuintupletQuarter:   return { AssembledNoteType::Quarter,      false, 5, -1.0, 5 };
    case NoteValue::QuintupletEighth:    return { AssembledNoteType::Eighth,       false, 5, -1.0, 5 };
    case NoteValue::QuintupletSixteenth: return { AssembledNoteType::Sixteenth,    false, 5, -1.0, 5 };
    case NoteValue::SextupletQuarter:    return { AssembledNoteType::Quarter,      false, 6, -1.0, 6 };
    case NoteValue::SextupletEighth:     return { AssembledNoteType::Eighth,       false, 6, -1.0, 6 };
    case NoteValue::SextupletSixteenth:  return { AssembledNoteType::Sixteenth,    false, 6, -1.0, 6 };
    case NoteValue::SeptupletQuarter:    return { AssembledNoteType::Quarter,      false, 7, -1.0, 7 };
    case NoteValue::SeptupletEighth:     return { AssembledNoteType::Eighth,       false, 7, -1.0, 7 };
    case NoteValue::SeptupletSixteenth:  return { AssembledNoteType::Sixteenth,    false, 7, -1.0, 7 };
    case NoteValue::OctupletEighth:      return { AssembledNoteType::Eighth,       false, 8, -1.0, 8 };
    case NoteValue::OctupletSixteenth:   return { AssembledNoteType::Sixteenth,    false, 8, -1.0, 8 };
    case NoteValue::NonupletEighth:      return { AssembledNoteType::Eighth,       false, 9, -1.0, 9 };
    case NoteValue::NonupletSixteenth:   return { AssembledNoteType::Sixteenth,    false, 9, -1.0, 9 };
    }
    return { AssembledNoteType::Quarter, false, 0, 0.25, 0 }; // fallback
}

// ---------------------------------------------------------------------------
// Beat-fraction conversion
// ---------------------------------------------------------------------------
double noteValueBeatFraction(NoteValue nv, bool compoundTime) {
    NoteValueInfo info = getNoteValueInfo(nv);
    if (info.beatDivisor > 0) {
        // Beat-relative tuplet: exactly 1/N of one beat
        return 1.0 / info.beatDivisor;
    }
    // Absolute: convert whole-note fraction to beat fraction
    double beatWhole = compoundTime ? (3.0 / 8.0) : (1.0 / 4.0);
    return info.wholeNoteFraction / beatWhole;
}

// ---------------------------------------------------------------------------
// Legacy migration helper
// ---------------------------------------------------------------------------
NoteValue noteValueFromLegacy(double beatFraction, bool isDotted, bool compoundTime) {
    double beatWhole = compoundTime ? (3.0 / 8.0) : (1.0 / 4.0);
    double absolute  = beatFraction * beatWhole; // absolute whole-note fraction

    auto close = [](double a, double b) { return std::abs(a - b) < 0.002; };

    // Beat-relative tuplets (recognized by beat-fraction, not absolute)
    if (close(beatFraction, 0.2))        return NoteValue::QuintupletNote;
    if (close(beatFraction, 1.0 / 7.0))  return NoteValue::SeptupletNote;

    // Triplets (recognized by absolute whole-note fraction)
    if (close(absolute, 1.0 / 6.0))  return NoteValue::TripletQuarter;
    if (close(absolute, 1.0 / 12.0)) return NoteValue::TripletEighth;
    if (close(absolute, 1.0 / 24.0)) return NoteValue::TripletSixteenth;

    // Dotted notes
    if (isDotted) {
        if (close(absolute, 0.75))    return NoteValue::DottedHalf;
        if (close(absolute, 0.375))   return NoteValue::DottedQuarter;
        if (close(absolute, 0.1875))  return NoteValue::DottedEighth;
        if (close(absolute, 0.09375)) return NoteValue::DottedSixteenth;
    }

    // Standard notes – match by absolute whole-note fraction
    if (close(absolute, 1.0))      return NoteValue::Whole;
    if (close(absolute, 0.5))      return NoteValue::Half;
    if (close(absolute, 0.375))    return NoteValue::DottedQuarter;
    if (close(absolute, 0.25))     return NoteValue::Quarter;
    if (close(absolute, 0.1875))   return NoteValue::DottedEighth;
    if (close(absolute, 0.125))    return NoteValue::Eighth;
    if (close(absolute, 0.09375))  return NoteValue::DottedSixteenth;
    if (close(absolute, 0.0625))   return NoteValue::Sixteenth;
    if (close(absolute, 0.03125))  return NoteValue::ThirtySecond;
    if (close(absolute, 0.015625)) return NoteValue::SixtyFourth;

    // Fallback: quantize to nearest standard value
    if (absolute >= 0.75)   return NoteValue::Whole;
    if (absolute >= 0.4)    return NoteValue::DottedQuarter;
    if (absolute >= 0.3)    return NoteValue::Quarter;
    if (absolute >= 0.15)   return NoteValue::DottedEighth;
    if (absolute >= 0.09)   return NoteValue::Eighth;
    if (absolute >= 0.05)   return NoteValue::Sixteenth;
    if (absolute >= 0.025)  return NoteValue::ThirtySecond;
    return NoteValue::SixtyFourth;
}

// ---------------------------------------------------------------------------
// String serialization helpers
// ---------------------------------------------------------------------------
QString noteValueToString(NoteValue nv) {
    switch (nv) {
    case NoteValue::Whole:             return "Whole";
    case NoteValue::Half:              return "Half";
    case NoteValue::DottedHalf:        return "DottedHalf";
    case NoteValue::Quarter:           return "Quarter";
    case NoteValue::DottedQuarter:     return "DottedQuarter";
    case NoteValue::Eighth:            return "Eighth";
    case NoteValue::DottedEighth:      return "DottedEighth";
    case NoteValue::Sixteenth:         return "Sixteenth";
    case NoteValue::DottedSixteenth:   return "DottedSixteenth";
    case NoteValue::ThirtySecond:      return "ThirtySecond";
    case NoteValue::SixtyFourth:       return "SixtyFourth";
    case NoteValue::TripletQuarter:    return "TripletQuarter";
    case NoteValue::TripletEighth:     return "TripletEighth";
    case NoteValue::TripletSixteenth:  return "TripletSixteenth";
    case NoteValue::QuintupletNote:    return "QuintupletNote";
    case NoteValue::SeptupletNote:     return "SeptupletNote";
    case NoteValue::DupletNote:          return "DupletNote";
    case NoteValue::QuartupletNote:      return "QuartupletNote";
    case NoteValue::DupletQuarter:       return "DupletQuarter";
    case NoteValue::DupletEighth:        return "DupletEighth";
    case NoteValue::DupletSixteenth:     return "DupletSixteenth";
    case NoteValue::TripletThirtySecond: return "TripletThirtySecond";
    case NoteValue::QuadrupletQuarter:   return "QuadrupletQuarter";
    case NoteValue::QuadrupletEighth:    return "QuadrupletEighth";
    case NoteValue::QuadrupletSixteenth: return "QuadrupletSixteenth";
    case NoteValue::QuintupletQuarter:   return "QuintupletQuarter";
    case NoteValue::QuintupletEighth:    return "QuintupletEighth";
    case NoteValue::QuintupletSixteenth: return "QuintupletSixteenth";
    case NoteValue::SextupletQuarter:    return "SextupletQuarter";
    case NoteValue::SextupletEighth:     return "SextupletEighth";
    case NoteValue::SextupletSixteenth:  return "SextupletSixteenth";
    case NoteValue::SeptupletQuarter:    return "SeptupletQuarter";
    case NoteValue::SeptupletEighth:     return "SeptupletEighth";
    case NoteValue::SeptupletSixteenth:  return "SeptupletSixteenth";
    case NoteValue::OctupletEighth:      return "OctupletEighth";
    case NoteValue::OctupletSixteenth:   return "OctupletSixteenth";
    case NoteValue::NonupletEighth:      return "NonupletEighth";
    case NoteValue::NonupletSixteenth:   return "NonupletSixteenth";
    }
    return "Quarter";
}

NoteValue noteValueFromString(const QString& s) {
    if (s == "Whole")             return NoteValue::Whole;
    if (s == "Half")              return NoteValue::Half;
    if (s == "DottedHalf")        return NoteValue::DottedHalf;
    if (s == "Quarter")           return NoteValue::Quarter;
    if (s == "DottedQuarter")     return NoteValue::DottedQuarter;
    if (s == "Eighth")            return NoteValue::Eighth;
    if (s == "DottedEighth")      return NoteValue::DottedEighth;
    if (s == "Sixteenth")         return NoteValue::Sixteenth;
    if (s == "DottedSixteenth")   return NoteValue::DottedSixteenth;
    if (s == "ThirtySecond")      return NoteValue::ThirtySecond;
    if (s == "SixtyFourth")       return NoteValue::SixtyFourth;
    if (s == "TripletQuarter")    return NoteValue::TripletQuarter;
    if (s == "TripletEighth")     return NoteValue::TripletEighth;
    if (s == "TripletSixteenth")  return NoteValue::TripletSixteenth;
    if (s == "QuintupletNote")    return NoteValue::QuintupletNote;
    if (s == "SeptupletNote")     return NoteValue::SeptupletNote;
    if (s == "DupletNote")          return NoteValue::DupletNote;
    if (s == "QuartupletNote")      return NoteValue::QuartupletNote;
    if (s == "DupletQuarter")       return NoteValue::DupletQuarter;
    if (s == "DupletEighth")        return NoteValue::DupletEighth;
    if (s == "DupletSixteenth")     return NoteValue::DupletSixteenth;
    if (s == "TripletThirtySecond") return NoteValue::TripletThirtySecond;
    if (s == "QuadrupletQuarter")   return NoteValue::QuadrupletQuarter;
    if (s == "QuadrupletEighth")    return NoteValue::QuadrupletEighth;
    if (s == "QuadrupletSixteenth") return NoteValue::QuadrupletSixteenth;
    if (s == "QuintupletQuarter")   return NoteValue::QuintupletQuarter;
    if (s == "QuintupletEighth")    return NoteValue::QuintupletEighth;
    if (s == "QuintupletSixteenth") return NoteValue::QuintupletSixteenth;
    if (s == "SextupletQuarter")    return NoteValue::SextupletQuarter;
    if (s == "SextupletEighth")     return NoteValue::SextupletEighth;
    if (s == "SextupletSixteenth")  return NoteValue::SextupletSixteenth;
    if (s == "SeptupletQuarter")    return NoteValue::SeptupletQuarter;
    if (s == "SeptupletEighth")     return NoteValue::SeptupletEighth;
    if (s == "SeptupletSixteenth")  return NoteValue::SeptupletSixteenth;
    if (s == "OctupletEighth")      return NoteValue::OctupletEighth;
    if (s == "OctupletSixteenth")   return NoteValue::OctupletSixteenth;
    if (s == "NonupletEighth")      return NoteValue::NonupletEighth;
    if (s == "NonupletSixteenth")   return NoteValue::NonupletSixteenth;
    return NoteValue::Quarter; // fallback
}

// ---------------------------------------------------------------------------
// buildNoteAssemblerConfig – the single authoritative rendering function
// ---------------------------------------------------------------------------
static AssembledNoteType toRestGlyph(AssembledNoteType t) {
    switch (t) {
    case AssembledNoteType::Whole:        return AssembledNoteType::Rest_Quarter;
    case AssembledNoteType::Half:         return AssembledNoteType::Rest_Quarter;
    case AssembledNoteType::Quarter:      return AssembledNoteType::Rest_Quarter;
    case AssembledNoteType::Eighth:       return AssembledNoteType::Rest_Eighth;
    case AssembledNoteType::Sixteenth:    return AssembledNoteType::Rest_Sixteenth;
    case AssembledNoteType::ThirtySecond: return AssembledNoteType::Rest_ThirtySecond;
    case AssembledNoteType::SixtyFourth:  return AssembledNoteType::Rest_SixtyFourth;
    default:                              return AssembledNoteType::Rest_Quarter;
    }
}

NoteAssemblerConfig buildNoteAssemblerConfig(const SubdivisionPattern& pattern) {
    NoteAssemblerConfig cfg;
    cfg.pixmapSize = QSize(48, 48);
    cfg.noteCount  = pattern.pulses.size();
    cfg.beamed     = cfg.noteCount > 1;
    cfg.noteTypes.clear();
    cfg.dottedNotes.clear();
    cfg.perNoteTupletNumbers.clear();
    cfg.tupletNumber = 0;

    if (pattern.pulses.isEmpty()) {
        cfg.noteCount = 1;
        cfg.noteTypes.push_back(AssembledNoteType::Quarter);
        cfg.dottedNotes.push_back(false);
        return cfg;
    }

    // 1. Per-note glyph and dot
    for (const SubdivisionPulse& pulse : pattern.pulses) {
        NoteValueInfo info = getNoteValueInfo(pulse.noteValue);
        AssembledNoteType nt = pulse.isRest ? toRestGlyph(info.noteType) : info.noteType;
        cfg.noteTypes.push_back(nt);
        cfg.dottedNotes.push_back(info.dotted);
    }

    // 2. Tuplet annotation
    //    If every note has the same non-zero tuplet number → group bracket + number.
    //    If some notes have a tuplet number and some don't → per-note numbers.
    int firstTuplet = getNoteValueInfo(pattern.pulses[0].noteValue).tupletNumber;
    bool allSame    = true;
    bool anyTuplet  = (firstTuplet > 0);
    for (int i = 1; i < pattern.pulses.size(); ++i) {
        int t = getNoteValueInfo(pattern.pulses[i].noteValue).tupletNumber;
        if (t != firstTuplet) allSame = false;
        if (t > 0)            anyTuplet = true;
    }

    if (allSame && firstTuplet > 0) {
        // Whole group is a uniform tuplet: draw a single bracket + number
        cfg.tupletNumber = firstTuplet;
    } else if (!allSame && anyTuplet) {
        // Mixed: build sub-group brackets for each consecutive run of same tuplet number
        int i = 0;
        while (i < pattern.pulses.size()) {
            int t = getNoteValueInfo(pattern.pulses[i].noteValue).tupletNumber;
            if (t > 0) {
                int start = i;
                while (i < pattern.pulses.size() &&
                       getNoteValueInfo(pattern.pulses[i].noteValue).tupletNumber == t)
                    ++i;
                cfg.tupletRuns.push_back({ start, i - 1, t });
            } else {
                ++i;
            }
        }
    }

    return cfg;
}

// ---------------------------------------------------------------------------
// resolveTupletNoteValue
// ---------------------------------------------------------------------------
NoteValue resolveTupletNoteValue(NoteValue base, int tupletN) {
    if (tupletN <= 0) return base;
    switch (base) {
    case NoteValue::Quarter:
        switch (tupletN) {
        case 2: return NoteValue::DupletQuarter;
        case 3: return NoteValue::TripletQuarter;
        case 4: return NoteValue::QuadrupletQuarter;
        case 5: return NoteValue::QuintupletQuarter;
        case 6: return NoteValue::SextupletQuarter;
        case 7: return NoteValue::SeptupletQuarter;
        default: break;
        }
        break;
    case NoteValue::Eighth:
        switch (tupletN) {
        case 2: return NoteValue::DupletEighth;
        case 3: return NoteValue::TripletEighth;
        case 4: return NoteValue::QuadrupletEighth;
        case 5: return NoteValue::QuintupletEighth;
        case 6: return NoteValue::SextupletEighth;
        case 7: return NoteValue::SeptupletEighth;
        case 8: return NoteValue::OctupletEighth;
        case 9: return NoteValue::NonupletEighth;
        default: break;
        }
        break;
    case NoteValue::Sixteenth:
        switch (tupletN) {
        case 2: return NoteValue::DupletSixteenth;
        case 3: return NoteValue::TripletSixteenth;
        case 4: return NoteValue::QuadrupletSixteenth;
        case 5: return NoteValue::QuintupletSixteenth;
        case 6: return NoteValue::SextupletSixteenth;
        case 7: return NoteValue::SeptupletSixteenth;
        case 8: return NoteValue::OctupletSixteenth;
        case 9: return NoteValue::NonupletSixteenth;
        default: break;
        }
        break;
    case NoteValue::ThirtySecond:
        if (tupletN == 3) return NoteValue::TripletThirtySecond;
        break;
    default:
        break;
    }
    return base; // unsupported combination → return plain base
}
