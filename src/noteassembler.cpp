#include "noteassembler.h"
#include "svgutils.h"
#include <QPainter>
#include <QSvgRenderer>
#include <algorithm>

// Helper: Identify rest types
static bool isRestType(AssembledNoteType t) {
    return t == AssembledNoteType::Rest_Quarter ||
           t == AssembledNoteType::Rest_Eighth ||
           t == AssembledNoteType::Rest_Sixteenth ||
           t == AssembledNoteType::Rest_ThirtySecond ||
           t == AssembledNoteType::Rest_SixtyFourth;
}

// Helper: Number of beams for each note type
int beamsForNoteType(AssembledNoteType t) {
    switch (t) {
        case AssembledNoteType::Eighth:       return 1;
        case AssembledNoteType::Sixteenth:    return 2;
        case AssembledNoteType::ThirtySecond: return 3;
        case AssembledNoteType::SixtyFourth:  return 4;
        default:                              return 0;
    }
}

// Default spacing for notes
double NoteAssembler::defaultSpacingForNotes(int noteCount, const QSize& pixmapSize) {
    double NOTEHEAD_SCALE = 0.25; // 12px baseline
    double noteheadWidth = pixmapSize.width() * NOTEHEAD_SCALE;
    double baseMultiplier;
    if (noteCount <= 3) baseMultiplier = 1.75;
    else if (noteCount <= 6) baseMultiplier = 1.5;
    else if (noteCount <= 8) baseMultiplier = 1.25;
    else baseMultiplier = 1.08;
    return noteheadWidth * baseMultiplier;
}

// SVG lookups
QString NoteAssembler::svgForRest(AssembledNoteType type) const {
    switch (type) {
        case AssembledNoteType::Rest_Quarter:     return m_prefix + "rest_quarter.svg";
        case AssembledNoteType::Rest_Eighth:      return m_prefix + "rest_eighth.svg";
        case AssembledNoteType::Rest_Sixteenth:   return m_prefix + "rest_sixteenth.svg";
        case AssembledNoteType::Rest_ThirtySecond:return m_prefix + "rest_thirtysecond.svg";
        case AssembledNoteType::Rest_SixtyFourth: return m_prefix + "rest_sixtyfourth.svg";
        default: return QString();
    }
}

QString NoteAssembler::svgForNotehead(AssembledNoteType type) const {
    switch (type) {
        case AssembledNoteType::Quarter:
        case AssembledNoteType::Eighth:
        case AssembledNoteType::Sixteenth:
        case AssembledNoteType::ThirtySecond:
        case AssembledNoteType::SixtyFourth:
            return m_prefix + "notehead_filled.svg";
        case AssembledNoteType::Half:
            return m_prefix + "notehead_half.svg";
        case AssembledNoteType::Whole:
            return m_prefix + "notehead_whole.svg";
        default:
            return QString();
    }
}

QString NoteAssembler::svgForFlag(AssembledNoteType type) const {
    switch (type) {
        case AssembledNoteType::Eighth:      return m_prefix + "flag_eighth_up.svg";
        case AssembledNoteType::Sixteenth:   return m_prefix + "flag_sixteenth_up.svg";
        case AssembledNoteType::ThirtySecond:return m_prefix + "flag_thirtysecond_up.svg";
        case AssembledNoteType::SixtyFourth: return m_prefix + "flag_sixtyfourth_up.svg";
        default: return QString();
    }
}

QString NoteAssembler::svgForTupletNumber(int n) const {
    return m_prefix + QString("number_%1.svg").arg(n);
}

NoteAssembler::NoteAssembler(const QString& svgResourcePrefix)
    : m_prefix(svgResourcePrefix.endsWith("/") ? svgResourcePrefix : svgResourcePrefix + "/")
{}

QPixmap NoteAssembler::assembleNote(const NoteAssemblerConfig& config_in) {
    NoteAssemblerConfig config = config_in;
    double NOTEHEAD_SCALE = 0.25; // 12px baseline
    double noteheadWidth = config.pixmapSize.width() * NOTEHEAD_SCALE;
    double noteheadHeight = config.pixmapSize.height() * NOTEHEAD_SCALE;

    int noteCount = config.noteCount > 1 ? config.noteCount : 1;
    std::vector<AssembledNoteType> noteTypes = config.noteTypes;
    if (noteTypes.empty()) noteTypes.assign(noteCount, config.noteType);

    double REST_SCALE = 0.41;
    int baseWidth = config.pixmapSize.width();
    QSize noteheadSize(baseWidth * NOTEHEAD_SCALE, baseWidth * NOTEHEAD_SCALE);
    QSize restSize(baseWidth * REST_SCALE, baseWidth * REST_SCALE);

    // Spacing
    std::vector<double> spacings(noteCount - 1, defaultSpacingForNotes(noteCount, config.pixmapSize));
    for (int i = 0; i < noteCount - 1; ++i) {
        if (!config.dottedNotes.empty() && i < int(config.dottedNotes.size()) && config.dottedNotes[i]) {
            spacings[i] += noteheadWidth * 0.5833; // 7px at 12 baseline
        }
    }

    // X positions and width
    std::vector<int> x_positions(noteCount);
    int leftMargin = int(noteheadSize.width() / 2);
    int curX = leftMargin;
    for (int i = 0; i < noteCount; ++i) {
        x_positions[i] = curX;
        if (i < noteCount - 1) curX += spacings[i];
    }
    int rightMargin = int(noteheadSize.width() / 2 + noteheadWidth * 0.8333); // 10px at 12px
    int totalWidth = curX + rightMargin;
    config.pixmapSize.setWidth(totalWidth);

    int extraTop = noteheadWidth * 1.6667; // 20px at 12px
    config.pixmapSize.setHeight(config.pixmapSize.height() + extraTop);

    int y_head;
    if (config.centerVertically) {
        y_head = (config.pixmapSize.height() - noteheadSize.height()) / 1.5;
    } else {
        y_head = int(config.pixmapSize.height() * 0.65);
    }
    int y_rest = int(y_head - restSize.height() / 2);
    int stemHeight = int(noteheadHeight * 2.4);
    int stemWidth = std::max(2, int(noteheadWidth * 0.1667));

    //--- Find beam groups
    std::vector<std::vector<int>> beamGroups;
    std::vector<int> group;
    for (int i = 0; i < noteCount; ++i) {
        bool canBeam = !isRestType(noteTypes[i]) && beamsForNoteType(noteTypes[i]) > 0;
        if (canBeam) {
            group.push_back(i);
        } else if (!group.empty()) {
            beamGroups.push_back(group);
            group.clear();
        }
    }
    if (!group.empty()) beamGroups.push_back(group);

    //--- Build composite SVG
    QString svg;
    svg.reserve(32768);
    svg += QString(
        R"(<svg xmlns="http://www.w3.org/2000/svg" )"
        R"(xmlns:xlink="http://www.w3.org/1999/xlink" )"
        R"(xmlns:sodipodi="http://sodipodi.sourceforge.net/DTD/sodipodi-0.0.dtd" )"
        R"(xmlns:inkscape="http://www.inkscape.org/namespaces/inkscape" )"
        R"(width="%1" height="%2">)"
    ).arg(config.pixmapSize.width()).arg(config.pixmapSize.height());

    // Embed an SVG glyph as a nested <svg> viewport at (x,y) with size (w,h)
    auto embedGlyph = [&](const QString& path, double x, double y, double w, double h) {
        if (path.isEmpty() || w <= 0 || h <= 0) return;
        QString vb    = svgViewBox(path);
        QString inner = svgInnerContent(path);
        if (vb.isEmpty() || inner.isEmpty()) return;
        // Parse viewBox "minX minY vbW vbH"
        const QStringList vbp = vb.split(QLatin1Char(' '), Qt::SkipEmptyParts);
        if (vbp.size() < 4) return;
        const double vbX = vbp[0].toDouble(), vbY = vbp[1].toDouble();
        const double vbW = vbp[2].toDouble(), vbH = vbp[3].toDouble();
        if (vbW <= 0 || vbH <= 0) return;
        // xMidYMid meet: uniform scale, centred in target rect
        const double scale = qMin(w / vbW, h / vbH);
        const double tx = x + (w - vbW * scale) / 2.0 - vbX * scale;
        const double ty = y + (h - vbH * scale) / 2.0 - vbY * scale;
        svg += QString("<g transform=\"translate(%1,%2) scale(%3)\">")
            .arg(tx, 0, 'f', 3).arg(ty, 0, 'f', 3).arg(scale, 0, 'f', 6);
        svg += inner;
        svg += "</g>";
    };
    auto addRect = [&](double x, double y, double w, double h) {
        if (w <= 0 || h <= 0) return;
        svg += QString(R"(<rect x="%1" y="%2" width="%3" height="%4" fill="white"/>)")
            .arg(x, 0, 'f', 2).arg(y, 0, 'f', 2).arg(w, 0, 'f', 2).arg(h, 0, 'f', 2);
    };
    auto addEllipse = [&](double cx, double cy, double rx, double ry) {
        svg += QString(R"(<ellipse cx="%1" cy="%2" rx="%3" ry="%4" fill="white"/>)")
            .arg(cx, 0, 'f', 2).arg(cy, 0, 'f', 2).arg(rx, 0, 'f', 2).arg(ry, 0, 'f', 2);
    };
    auto addLine = [&](double x1, double y1, double x2, double y2, double sw) {
        svg += QString(R"(<line x1="%1" y1="%2" x2="%3" y2="%4" stroke="white" stroke-width="%5" stroke-linecap="square"/>)")
            .arg(x1, 0, 'f', 2).arg(y1, 0, 'f', 2).arg(x2, 0, 'f', 2).arg(y2, 0, 'f', 2).arg(sw, 0, 'f', 2);
    };

    //--- Draw notes/rests, collect stem tops for beams/flags
    std::vector<QPoint> stemTops(noteCount, QPoint(-999, -999)), stemTips(noteCount, QPoint(-999, -999));
    for (int i = 0; i < noteCount; ++i) {
        int x = x_positions[i] - int(noteheadSize.width() / 2);
        if (isRestType(noteTypes[i])) {
            embedGlyph(svgForRest(noteTypes[i]), x, y_rest, restSize.width(), restSize.height());
            continue;
        }
        embedGlyph(svgForNotehead(noteTypes[i]), x, y_head, noteheadSize.width(), noteheadSize.height());

        bool hasStem = (noteTypes[i] != AssembledNoteType::Whole);
        int x_stem = x + int(noteheadWidth) - int(noteheadWidth * 0.18);
        int y_stem_top = y_head - stemHeight + int(noteheadHeight * 0.33);
        int y_stem_bottom = y_head + int(noteheadHeight * 0.4);

        if (hasStem) {
            addRect(x_stem, y_stem_top, stemWidth, y_stem_bottom - y_stem_top);
            stemTops[i] = QPoint(x_stem, y_stem_top);
            stemTips[i] = QPoint(x_stem + stemWidth / 2, y_stem_top);
        } else {
            stemTops[i] = QPoint(x + int(noteheadWidth / 2), y_head);
            stemTips[i] = QPoint(x + int(noteheadWidth / 2), y_head);
        }

        bool dotted = (!config.dottedNotes.empty() && i < int(config.dottedNotes.size()) && config.dottedNotes[i]);
        if (dotted) {
            int dotRadius = std::max(2, int(noteheadWidth * 0.1667));
            int dotX = x + noteheadSize.width() + dotRadius + 1;
            int dotY = y_head + int(noteheadSize.height() * 0.5) - dotRadius;
            addEllipse(dotX + dotRadius, dotY + dotRadius, dotRadius, dotRadius);
        }
    }

    //--- Draw beams
    int beamThickness1 = std::max(4, int(noteheadHeight * 0.2667));
    int beamSpacing1   = std::max(4, int(noteheadHeight * 0.2667));
    int beamThickness2 = std::max(2, int(noteheadHeight * 0.1233));
    int beamSpacing2   = std::max(2, int(noteheadHeight * 0.4667));
    int beamThickness3 = std::max(2, int(noteheadHeight * 0.1233));
    int beamSpacing3   = std::max(2, int(noteheadHeight * 0.2667));
    int beamThickness4 = std::max(2, int(noteheadHeight * 0.1233));
    int beamSpacing4   = std::max(2, int(noteheadHeight * 0.05));

    for (const auto& grp : beamGroups) {
        if (grp.size() < 2) continue;
        int maxBeams = 0;
        for (int idx : grp) maxBeams = std::max(maxBeams, beamsForNoteType(noteTypes[idx]));

        int y_beam = stemTops[grp.front()].y();
        for (int b = 1; b <= maxBeams; ++b) {
            int thickness, spacing;
            if      (b == 1) { thickness = beamThickness1; spacing = beamSpacing1; }
            else if (b == 2) { thickness = beamThickness2; spacing = beamSpacing2; }
            else if (b == 3) { thickness = beamThickness3; spacing = beamSpacing3; }
            else             { thickness = beamThickness4; spacing = beamSpacing4; }

            if (b == 1) y_beam = stemTops[grp.front()].y();
            else        y_beam += thickness + spacing;

            for (size_t k = 0; k < grp.size(); ++k) {
                int i = grp[k];
                int x = stemTips[i].x();
                if (beamsForNoteType(noteTypes[i]) < b) continue;

                if (k > 0 && beamsForNoteType(noteTypes[grp[k-1]]) >= b) {
                    int x_prev = stemTips[grp[k-1]].x();
                    addRect(std::min(x, x_prev), y_beam, std::abs(x - x_prev), thickness);
                } else if (b > 1 && k > 0 && beamsForNoteType(noteTypes[grp[k-1]]) < b &&
                           !(k+1 < grp.size() && beamsForNoteType(noteTypes[grp[k+1]]) >= b)) {
                    int stubLen = thickness * 4;
                    addRect(x - stubLen, y_beam, stubLen, thickness);
                }
                if (b > 1 && k == 0 && k+1 < grp.size() && beamsForNoteType(noteTypes[grp[k+1]]) < b) {
                    int stubLen = thickness * 4;
                    addRect(x, y_beam, stubLen, thickness);
                }
            }
        }
    }

    //--- Draw flags for single (unbeamed) notes
    for (int i = 0; i < noteCount; ++i) {
        if (isRestType(noteTypes[i])) continue;
        bool isBeamed = false;
        for (const auto& grp : beamGroups)
            if (std::find(grp.begin(), grp.end(), i) != grp.end() && grp.size() > 1)
                isBeamed = true;
        if (!isBeamed && beamsForNoteType(noteTypes[i]) > 0
            && noteTypes[i] != AssembledNoteType::Half
            && noteTypes[i] != AssembledNoteType::Whole) {
            QString flagSvg = svgForFlag(noteTypes[i]);
            if (!flagSvg.isEmpty()) {
                int x_flag = stemTips[i].x() - int(noteheadWidth * 0.5267);
                int y_flag = stemTips[i].y() + int(noteheadWidth * 0.0833);
                embedGlyph(flagSvg, x_flag, y_flag, noteheadWidth * 1.65, noteheadHeight * 1.80);
            }
        }
    }

    //--- Per-note tuplet numbers
    if (!config.perNoteTupletNumbers.empty()) {
        for (int i = 0; i < noteCount && i < (int)config.perNoteTupletNumbers.size(); ++i) {
            if (config.perNoteTupletNumbers[i] <= 1) continue;
            if (isRestType(noteTypes[i])) continue;
            int n = config.perNoteTupletNumbers[i];
            int numberSize = int(noteheadHeight * 0.6667);
            int x_num = x_positions[i] - numberSize / 2;
            int y_num = stemTops[i].y() - numberSize - int(noteheadHeight * 0.25);
            embedGlyph(svgForTupletNumber(n), x_num, y_num, numberSize, numberSize);
        }
    }

    //--- Tuplet bracket helper
    auto drawTupletBracket = [&](int runFirst, int runLast, int num) {
        if (runFirst >= noteCount || runLast >= noteCount) return;
        int firstNote = runFirst;
        for (int i = runFirst; i <= runLast; ++i)
            if (!isRestType(noteTypes[i])) { firstNote = i; break; }

        int bracketPad = int(noteheadWidth * 0.5);
        int x_left  = x_positions[runFirst] +
                      (isRestType(noteTypes[runFirst]) ? restSize.width() / 2 : noteheadSize.width() / 2)
                      - bracketPad;
        int x_right = x_positions[runLast] +
                      (isRestType(noteTypes[runLast])  ? restSize.width() / 2 : noteheadSize.width() / 2)
                      + bracketPad;
        int y_bracket = stemTops[firstNote].y() - int(noteheadHeight * 0.5833);

        int numberSize  = int(noteheadHeight * 0.6667);
        int x_num       = (x_left + x_right) / 2 - numberSize / 2;
        int y_num       = y_bracket - numberSize + int(noteheadHeight * 0.1667);
        int gapPad      = int(noteheadWidth * 0.3333);
        int x_gap_left  = x_num - gapPad;
        int x_gap_right = x_num + numberSize + gapPad;
        double thickness = noteheadHeight * 0.1667;
        int vertLen     = int(noteheadHeight * 0.25);

        if (runFirst == runLast) {
            embedGlyph(svgForTupletNumber(num), x_num, y_num, numberSize, numberSize);
        } else {
            addLine(x_left,      y_bracket, x_gap_left,  y_bracket, thickness);
            addLine(x_gap_right, y_bracket, x_right,     y_bracket, thickness);
            addLine(x_left,  y_bracket, x_left,  y_bracket + vertLen, thickness);
            addLine(x_right, y_bracket, x_right, y_bracket + vertLen, thickness);
            embedGlyph(svgForTupletNumber(num), x_num, y_num, numberSize, numberSize);
        }
    };

    if (config.tupletNumber > 1 && noteCount == 1) {
        int numberSize = int(noteheadHeight * 0.6667);
        int x_num = x_positions[0] - numberSize / 2;
        int y_num = stemTops[0].y() - numberSize - int(noteheadHeight * 0.25);
        embedGlyph(svgForTupletNumber(config.tupletNumber), x_num, y_num, numberSize, numberSize);
    } else if (config.tupletNumber > 1 && noteCount > 1) {
        drawTupletBracket(0, noteCount - 1, config.tupletNumber);
    }

    if (!config.tupletRuns.empty()) {
        for (const auto& run : config.tupletRuns)
            drawTupletBracket(run.first, run.last, run.number);
    }

    svg += "</svg>";

    //--- Single-pass render of the composite SVG
    QSvgRenderer renderer(svg.toUtf8());
    QPixmap result(config.pixmapSize);
    result.fill(Qt::transparent);
    QPainter p(&result);
    p.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform);
    renderer.render(&p);
    p.end();
    return result;
}