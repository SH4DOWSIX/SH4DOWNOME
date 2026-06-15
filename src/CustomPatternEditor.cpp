#include "CustomPatternEditor.h"
#include "metronomeengine.h"
#include "noteassembler.h"
#include <QPainter>
#include <QSvgRenderer>
#include <cmath>
#include <algorithm>

// ─── Note-value tile table (mirrors customsubdivisiondialog.cpp) ──────────────
struct NVTile {
    NoteValue base;
    NoteValue dotted;
    QString   name;
    bool      canBeDotted;
};
static const QVector<NVTile> TILES = {
    { NoteValue::SixtyFourth,  NoteValue::SixtyFourth,     "64th",    false },
    { NoteValue::ThirtySecond, NoteValue::ThirtySecond,    "32nd",    false },
    { NoteValue::Sixteenth,    NoteValue::DottedSixteenth, "16th",    true  },
    { NoteValue::Eighth,       NoteValue::DottedEighth,    "8th",     true  },
    { NoteValue::Quarter,      NoteValue::DottedQuarter,   "Quarter", true  },
    { NoteValue::Half,         NoteValue::DottedHalf,      "Half",    true  },
    { NoteValue::Whole,        NoteValue::Whole,           "Whole",   false },
};

// ─── Preset table ─────────────────────────────────────────────────────────────
struct Preset { QString name; QVector<SubdivisionPulse> pulses; };
static const QVector<Preset>& PRESETS() {
    static QVector<Preset> p = {
        {"Two 8ths",    {{NoteValue::Eighth,        false,false},{NoteValue::Eighth,        false,false}}},
        {"Four 16ths",  {{NoteValue::Sixteenth,     false,false},{NoteValue::Sixteenth,     false,false},
                         {NoteValue::Sixteenth,     false,false},{NoteValue::Sixteenth,     false,false}}},
        {"8th+2x16th",  {{NoteValue::Eighth,        false,false},{NoteValue::Sixteenth,     false,false},
                         {NoteValue::Sixteenth,     false,false}}},
        {"2x16th+8th",  {{NoteValue::Sixteenth,     false,false},{NoteValue::Sixteenth,     false,false},
                         {NoteValue::Eighth,        false,false}}},
        {"Triplets",    {{NoteValue::TripletEighth, false,false},{NoteValue::TripletEighth, false,false},
                         {NoteValue::TripletEighth, false,false}}},
        {"Dot8th+16th", {{NoteValue::DottedEighth,  false,false},{NoteValue::Sixteenth,     false,false}}},
        {"16th+Dot8th", {{NoteValue::Sixteenth,     false,false},{NoteValue::DottedEighth,  false,false}}},
        {"Swing",       {{NoteValue::DottedEighth,  false,false},{NoteValue::Sixteenth,     false,false}}},
        {"Quarter",     {{NoteValue::Quarter,       false,false}}},
    };
    return p;
}

// ─── Helpers ──────────────────────────────────────────────────────────────────
static int tileIndexForBase(NoteValue base) {
    for (int i = 0; i < TILES.size(); ++i)
        if (TILES[i].base == base) return i;
    return 4;  // Quarter fallback
}

// Decompose a NoteValue into (tileIdx, dotted, tupletN)
static void decomposeNoteValue(NoteValue nv, int& tileIdx, bool& dotted, int& tupletN) {
    NoteValueInfo info = getNoteValueInfo(nv);
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
    tileIdx = tileIndexForBase(base);
    dotted  = info.dotted;
    tupletN = info.tupletNumber;
}

static NoteValue resolveNoteValue(int tileIdx, bool dotted, int tupletN) {
    const auto& t = TILES[tileIdx];
    if (tupletN > 0)               return resolveTupletNoteValue(t.base, tupletN);
    if (dotted && t.canBeDotted)   return t.dotted;
    return t.base;
}

// ─── CustomPatternEditor ─────────────────────────────────────────────────────
CustomPatternEditor::CustomPatternEditor(MetronomeEngine* engine, QObject* parent)
    : QObject(parent), m_engine(engine)
{
    m_previewTimer = new QTimer(this);
    m_previewTimer->setSingleShot(true);
    connect(m_previewTimer, &QTimer::timeout, this, [this]() {
        stopPreview();
    });
    recomputeTotal();
}

void CustomPatternEditor::beginNew(int numerator, int denominator, bool compound) {
    m_numerator = numerator; m_denominator = denominator; m_compound = compound;
    m_pattern.pulses.clear();
    m_pattern.category = SubdivisionCategory::Custom;
    m_name = "Custom Pattern";
    m_selectedIdx = -1;
    m_tileIdx = 4; m_tuplet = 0; m_dotted = false; m_isRest = false; m_accent = false;
    m_undoStack.clear();
    ++m_rev;
    recomputeTotal();
    emit patternNameChanged();
    emit pulsesChanged();
    emit selectionChanged();
}

void CustomPatternEditor::beginEdit(const SubdivisionPattern& pattern,
                                    int numerator, int denominator, bool compound) {
    m_numerator = numerator; m_denominator = denominator; m_compound = compound;
    m_pattern = pattern;
    m_name = pattern.name.isEmpty() ? "Custom Pattern" : pattern.name;
    m_selectedIdx = -1;
    m_tileIdx = 4; m_tuplet = 0; m_dotted = false; m_isRest = false; m_accent = false;
    m_undoStack.clear();
    ++m_rev;
    recomputeTotal();
    emit patternNameChanged();
    emit pulsesChanged();
    emit selectionChanged();
}

SubdivisionPattern CustomPatternEditor::currentPattern() const {
    SubdivisionPattern p = m_pattern;
    p.name = m_name;
    p.category = SubdivisionCategory::Custom;
    return p;
}

// ── Properties ────────────────────────────────────────────────────────────────

QVariantList CustomPatternEditor::pulses() const {
    QVariantList list;
    for (const auto& p : m_pattern.pulses) {
        QVariantMap m;
        m["isRest"] = p.isRest;
        m["accent"] = p.accent;
        list.append(m);
    }
    return list;
}

bool CustomPatternEditor::canDot() const {
    return m_tuplet == 0 && m_tileIdx >= 0 && m_tileIdx < TILES.size()
           && TILES[m_tileIdx].canBeDotted;
}

QVariantList CustomPatternEditor::tupletVisibility() const {
    QVariantList v;
    v.append(true); // index 0 = "None" always visible
    v.append(false); // index 1 unused
    for (int n = 2; n <= 9; ++n)
        v.append(isValidTupletForTile(m_tileIdx, n));
    return v;
}

QStringList CustomPatternEditor::presetNames() const {
    QStringList names;
    for (const auto& p : PRESETS()) names << p.name;
    return names;
}

void CustomPatternEditor::setPatternName(const QString& n) {
    if (m_name != n) { m_name = n; emit patternNameChanged(); }
}

int CustomPatternEditor::tileCount()  const { return TILES.size(); }
QString CustomPatternEditor::tileName(int idx) const {
    if (idx < 0 || idx >= TILES.size()) return {};
    return TILES[idx].name;
}

bool CustomPatternEditor::isValidTupletForTile(int tileIdx, int tupletN) const {
    if (tupletN == 0) return true;
    if (tileIdx < 0 || tileIdx >= TILES.size()) return false;
    switch (TILES[tileIdx].base) {
    case NoteValue::Quarter:      return tupletN >= 2 && tupletN <= 7;
    case NoteValue::Eighth:       return tupletN >= 2 && tupletN <= 9;
    case NoteValue::Sixteenth:    return tupletN >= 2 && tupletN <= 9;
    case NoteValue::ThirtySecond: return tupletN == 3;
    default:                      return false;
    }
}

// ── Private helpers ───────────────────────────────────────────────────────────

NoteValue CustomPatternEditor::resolveActive() const {
    return resolveNoteValue(m_tileIdx, m_dotted && m_tuplet == 0, m_tuplet);
}

void CustomPatternEditor::syncControlsToSelectedPulse() {
    if (m_selectedIdx < 0 || m_selectedIdx >= m_pattern.pulses.size()) return;
    const auto& p = m_pattern.pulses[m_selectedIdx];
    decomposeNoteValue(p.noteValue, m_tileIdx, m_dotted, m_tuplet);
    m_isRest  = p.isRest;
    m_accent  = p.accent;
}

void CustomPatternEditor::pushUndo() {
    m_undoStack.append(m_pattern.pulses);
    if (m_undoStack.size() > 50) m_undoStack.removeFirst();
}

void CustomPatternEditor::recomputeTotal() {
    if (m_pattern.pulses.isEmpty()) {
        m_totalText  = "Total: 0 beats";
        m_totalColor = "#888888";
        return;
    }
    double beatUnit = 1.0;
    if (!m_compound) {
        if (m_denominator == 2) beatUnit = 2.0;
        else if (m_denominator == 1) beatUnit = 4.0;
    }
    double totalRaw = 0.0;
    for (const auto& p : m_pattern.pulses)
        totalRaw += noteValueBeatFraction(p.noteValue, m_compound);
    double beats = std::round((totalRaw / beatUnit) * 10000.0) / 10000.0;
    int beatsPerBar = m_compound ? (m_numerator / 3) : m_numerator;

    // Format as fraction
    auto fmtBeats = [](double v) -> QString {
        double t3 = std::round(v * 3.0);
        if (std::fabs(t3 / 3.0 - v) < 0.0001) {
            int n = int(t3);
            return (n % 3 == 0) ? QString::number(n / 3) : QString("%1/3").arg(n);
        }
        double t2 = std::round(v * 2.0);
        if (std::fabs(t2 / 2.0 - v) < 0.0001) {
            int n = int(t2);
            return (n % 2 == 0) ? QString::number(n / 2) : QString("%1/2").arg(n);
        }
        return QString::number(v, 'g', 4);
    };

    double barsExact = beats / beatsPerBar;
    bool isWholeBars  = std::fmod(std::round(barsExact * 10000.0) / 10000.0, 1.0) < 0.001;
    bool isWholeBeats = std::fmod(beats, 1.0) < 0.001;

    QString suffix;
    if (isWholeBars && barsExact >= 0.9999) {
        int bars = int(std::round(barsExact));
        suffix = (bars == 1) ? " = 1 bar" : QString(" = %1 bars").arg(bars);
    } else {
        suffix = QString(" of %1/bar").arg(beatsPerBar);
    }

    QString beatWord = m_compound ? "comp.beat" : "beat";
    if (beats != 1.0) beatWord += "s";
    m_totalText  = QString("Total: %1 %2%3").arg(fmtBeats(beats), beatWord, suffix);
    m_totalColor = isWholeBars ? "#64dc64" : isWholeBeats ? "#50c8c8" : "#dcc850";
}

// ── Invokables ────────────────────────────────────────────────────────────────

void CustomPatternEditor::selectPulse(int idx) {
    if (idx < 0 || idx >= m_pattern.pulses.size()) { m_selectedIdx = -1; emit selectionChanged(); return; }
    m_selectedIdx = idx;
    syncControlsToSelectedPulse();
    emit selectionChanged();
}

void CustomPatternEditor::setTile(int tileIdx) {
    if (tileIdx < 0 || tileIdx >= TILES.size()) return;
    m_tileIdx = tileIdx;
    // Reset tuplet if now invalid
    if (!isValidTupletForTile(m_tileIdx, m_tuplet)) { m_tuplet = 0; m_dotted = false; }
    if (!canDot()) m_dotted = false;
    // Update selected pulse in-place
    if (m_selectedIdx >= 0 && m_selectedIdx < m_pattern.pulses.size()) {
        m_pattern.pulses[m_selectedIdx].noteValue = resolveActive();
        ++m_rev;
        recomputeTotal();
        emit pulsesChanged();
    }
    emit selectionChanged();
}

void CustomPatternEditor::setTuplet(int n) {
    m_tuplet = (n == m_tuplet && n != 0) ? 0 : n; // toggle off
    if (!canDot()) m_dotted = false;
    if (m_selectedIdx >= 0 && m_selectedIdx < m_pattern.pulses.size()) {
        m_pattern.pulses[m_selectedIdx].noteValue = resolveActive();
        ++m_rev;
        recomputeTotal();
        emit pulsesChanged();
    }
    emit selectionChanged();
}

void CustomPatternEditor::setDotted(bool d) {
    if (!canDot()) d = false;
    m_dotted = d;
    if (m_selectedIdx >= 0 && m_selectedIdx < m_pattern.pulses.size()) {
        m_pattern.pulses[m_selectedIdx].noteValue = resolveActive();
        ++m_rev;
        recomputeTotal();
        emit pulsesChanged();
    }
    emit selectionChanged();
}

void CustomPatternEditor::setIsRest(bool r) {
    m_isRest = r;
    if (m_selectedIdx >= 0 && m_selectedIdx < m_pattern.pulses.size()) {
        m_pattern.pulses[m_selectedIdx].isRest = r;
        ++m_rev;
        recomputeTotal();
        emit pulsesChanged();
    }
    emit selectionChanged();
}

void CustomPatternEditor::setAccent(bool a) {
    m_accent = a;
    if (m_selectedIdx >= 0 && m_selectedIdx < m_pattern.pulses.size()) {
        m_pattern.pulses[m_selectedIdx].accent = a;
        ++m_rev;
        emit pulsesChanged();
    }
    emit selectionChanged();
}

void CustomPatternEditor::addPulse() {
    pushUndo();
    SubdivisionPulse p;
    p.noteValue = resolveActive();
    p.isRest    = m_isRest;
    p.accent    = m_accent;
    int insertAt = (m_selectedIdx >= 0 && m_selectedIdx < m_pattern.pulses.size())
                   ? m_selectedIdx + 1 : m_pattern.pulses.size();
    m_pattern.pulses.insert(insertAt, p);
    m_selectedIdx = insertAt;
    ++m_rev;
    recomputeTotal();
    emit pulsesChanged();
    emit selectionChanged();
}

void CustomPatternEditor::removeSelected() {
    if (m_selectedIdx < 0 || m_selectedIdx >= m_pattern.pulses.size()) return;
    pushUndo();
    m_pattern.pulses.removeAt(m_selectedIdx);
    if (m_selectedIdx >= m_pattern.pulses.size()) m_selectedIdx = m_pattern.pulses.size() - 1;
    ++m_rev;
    recomputeTotal();
    emit pulsesChanged();
    emit selectionChanged();
}

void CustomPatternEditor::clearAll() {
    if (!m_pattern.pulses.isEmpty()) pushUndo();
    m_pattern.pulses.clear();
    m_selectedIdx = -1;
    ++m_rev;
    recomputeTotal();
    emit pulsesChanged();
    emit selectionChanged();
}

void CustomPatternEditor::applyPreset(int presetIdx) {
    const auto& presets = PRESETS();
    if (presetIdx < 0 || presetIdx >= presets.size()) return;
    pushUndo();
    int insertAt = (m_selectedIdx >= 0 && m_selectedIdx < m_pattern.pulses.size())
                   ? m_selectedIdx + 1 : m_pattern.pulses.size();
    const auto& pulses = presets[presetIdx].pulses;
    for (int i = 0; i < pulses.size(); ++i)
        m_pattern.pulses.insert(insertAt + i, pulses[i]);
    m_selectedIdx = insertAt + pulses.size() - 1;
    ++m_rev;
    recomputeTotal();
    emit pulsesChanged();
    emit selectionChanged();
}

void CustomPatternEditor::undo() {
    if (m_undoStack.isEmpty()) return;
    m_pattern.pulses = m_undoStack.takeLast();
    m_selectedIdx = -1;
    ++m_rev;
    recomputeTotal();
    emit pulsesChanged();
    emit selectionChanged();
}

void CustomPatternEditor::movePulse(int from, int to) {
    if (from == to || from < 0 || from >= m_pattern.pulses.size()
        || to < 0 || to >= m_pattern.pulses.size()) return;
    pushUndo();
    SubdivisionPulse moving = m_pattern.pulses.takeAt(from);
    m_pattern.pulses.insert(to, moving);
    m_selectedIdx = to;
    ++m_rev;
    recomputeTotal();
    emit pulsesChanged();
    emit selectionChanged();
}

void CustomPatternEditor::startPreview() {
    if (!m_engine || m_pattern.pulses.isEmpty()) return;
    if (m_engine->isRunning()) return;
    m_savedPreviewPattern = m_engine->subdivisionPattern();

    double totalBeats = 0.0;
    for (const auto& p : m_pattern.pulses)
        totalBeats += noteValueBeatFraction(p.noteValue, m_compound);
    int    bpm  = m_engine->currentTempo();
    if (bpm <= 0) bpm = 120;
    double barMs = totalBeats * 60000.0 / bpm;

    double lastStart = 0.0;
    for (int i = 0; i + 1 < m_pattern.pulses.size(); ++i)
        lastStart += noteValueBeatFraction(m_pattern.pulses[i].noteValue, m_compound);
    double stopMs = qMax(lastStart * 60000.0 / bpm + 50.0, barMs - 30.0);
    stopMs = qMin(stopMs, barMs - 5.0);
    stopMs = qMax(stopMs, 10.0);

    m_engine->setSubdivisionPattern(m_pattern);
    m_engine->start();
    m_isPreviewing = true;
    emit isPreviewingChanged();
    m_previewTimer->start(int(stopMs));
}

void CustomPatternEditor::stopPreview() {
    if (!m_engine) return;
    m_previewTimer->stop();
    if (m_isPreviewing) {
        m_engine->stop();
        m_engine->setSubdivisionPattern(m_savedPreviewPattern);
    }
    m_isPreviewing = false;
    emit isPreviewingChanged();
}

// ── Image rendering ───────────────────────────────────────────────────────────

QPixmap CustomPatternEditor::tilePixmap(int tileIdx, QSize size) const {
    if (tileIdx < 0 || tileIdx >= TILES.size()) return QPixmap(size);
    SubdivisionPattern p;
    p.category = SubdivisionCategory::Custom;
    p.pulses.append(SubdivisionPulse{ TILES[tileIdx].base, false, false });
    NoteAssemblerConfig cfg = buildNoteAssemblerConfig(p);
    cfg.pixmapSize       = size;
    cfg.centerVertically = true;
    cfg.beamed           = false;
    NoteAssembler assembler;
    return assembler.assembleNote(cfg);
}

QPixmap CustomPatternEditor::pulsePixmap(int pulseIdx, QSize size) const {
    if (pulseIdx < 0 || pulseIdx >= m_pattern.pulses.size()) return QPixmap(size);
    SubdivisionPattern p;
    p.category = SubdivisionCategory::Custom;
    p.pulses.append(m_pattern.pulses[pulseIdx]);
    NoteAssemblerConfig cfg = buildNoteAssemblerConfig(p);
    cfg.pixmapSize       = size;
    cfg.centerVertically = true;
    cfg.beamed           = false;
    NoteAssembler assembler;
    return assembler.assembleNote(cfg);
}

QPixmap CustomPatternEditor::previewPixmap(QSize size) const {
    if (m_pattern.pulses.isEmpty()) return QPixmap(size);
    NoteAssemblerConfig cfg = buildNoteAssemblerConfig(m_pattern);
    cfg.pixmapSize       = size;
    cfg.centerVertically = true;
    NoteAssembler assembler;
    return assembler.assembleNote(cfg);
}
