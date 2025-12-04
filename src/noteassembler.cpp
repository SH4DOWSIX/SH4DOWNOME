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

// Render SVG in white
QPixmap whiteSvgToPixmap(const QString& svgPath, const QSize& size) {
    if (svgPath.isEmpty()) {
        QPixmap dummy(size);
        dummy.fill(Qt::transparent);
        return dummy;
    }
    QPixmap pix = svgToPixmap(svgPath, size);
    QPixmap out(pix.size());
    out.fill(Qt::transparent);
    QPainter p(&out);
    p.drawPixmap(0, 0, pix);
    p.setCompositionMode(QPainter::CompositionMode_SourceIn);
    p.fillRect(out.rect(), Qt::white);
    p.end();
    return out;
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

    QPixmap result(config.pixmapSize);
    result.fill(Qt::transparent);
    QPainter p(&result);
    p.setRenderHint(QPainter::Antialiasing, true);

    int y_head;
if (config.centerVertically) {
    y_head = (config.pixmapSize.height() - noteheadSize.height()) / 1.5;
} else {
    y_head = int(config.pixmapSize.height() * 0.65); // as before
} // vertical center for notehead
    int y_rest = int(y_head - restSize.height() / 2);
    int stemHeight = int(noteheadHeight * 2.4);
    int stemWidth = std::max(2, int(noteheadWidth * 0.1667)); // 2px at 12px

    //--- Find beam groups (runs of notes uninterrupted by rests)
    std::vector<std::vector<int>> beamGroups;
    std::vector<int> group;
    for (int i = 0; i < noteCount; ++i) {
        if (!isRestType(noteTypes[i])) {
            group.push_back(i);
        } else if (!group.empty()) {
            beamGroups.push_back(group);
            group.clear();
        }
    }
    if (!group.empty()) beamGroups.push_back(group);

    //--- Draw notes/rests, collect stem tops for beams/flags
    std::vector<QPoint> stemTops(noteCount, QPoint(-999, -999)), stemTips(noteCount, QPoint(-999, -999));
    for (int i = 0; i < noteCount; ++i) {
        int x = x_positions[i] - int(noteheadSize.width() / 2);
        if (isRestType(noteTypes[i])) {
            QPixmap restPix = whiteSvgToPixmap(svgForRest(noteTypes[i]), restSize);
            p.drawPixmap(x, y_rest, restPix);
            continue;
        }
        QPixmap nh = whiteSvgToPixmap(svgForNotehead(noteTypes[i]), noteheadSize);
        p.drawPixmap(x, y_head, nh);

        // ----- Stem: attach to right edge of notehead -----
        int x_stem = x + int(noteheadWidth) - int(noteheadWidth * 0.18); // 0.08 fudge, scaled for 12px
        int y_stem_top = y_head - stemHeight + int(noteheadHeight * 0.33);
        int y_stem_bottom = y_head + int(noteheadHeight * 0.4);

        // Draw stem (white, up)
        p.setPen(Qt::NoPen); p.setBrush(Qt::white);
        p.drawRect(x_stem, y_stem_top, stemWidth, y_stem_bottom - y_stem_top);
        stemTops[i] = QPoint(x_stem, y_stem_top);
        stemTips[i] = QPoint(x_stem + stemWidth / 2, y_stem_top);

        // ----- Dot -----
        bool dotted = (!config.dottedNotes.empty() && i < int(config.dottedNotes.size()) && config.dottedNotes[i]);
        if (dotted) {
            int dotRadius = std::max(2, int(noteheadWidth * 0.1667)); // 2px at 12px
            int dotX = x + nh.width() + dotRadius + 1;
            int dotY = y_head + int(nh.height() * 0.5) - dotRadius;
            p.setPen(Qt::NoPen); p.setBrush(Qt::white);
            p.drawEllipse(QRect(dotX, dotY, dotRadius * 2, dotRadius * 2));
        }
    }

//--- Draw beams (never cross rests; always white)
int beamThickness1 = std::max(4, int(noteheadHeight * 0.2667)); // 2px at 12px (Eighth)
int beamSpacing1   = std::max(4, int(noteheadHeight * 0.2667)); // 2px at 12px
int beamThickness2 = std::max(2, int(noteheadHeight * 0.1233)); // 1px at 12px (Sixteenth)
int beamSpacing2   = std::max(2, int(noteheadHeight * 0.4667)); // 1px at 12px
int beamThickness3 = std::max(2, int(noteheadHeight * 0.1233)); // 1px at 12px (Thirty-Second)
int beamSpacing3   = std::max(2, int(noteheadHeight * 0.2667)); // 1px at 12px
int beamThickness4 = std::max(2, int(noteheadHeight * 0.1233));   // 1px at 12px (Sixty-Fourth)
int beamSpacing4   = std::max(2, int(noteheadHeight * 0.05));   // 1px at 12px

for (const auto& group : beamGroups) {
    if (group.size() < 2) continue;
    int maxBeams = 0;
    for (int idx : group) maxBeams = std::max(maxBeams, beamsForNoteType(noteTypes[idx]));

    int y_beam = stemTops[group.front()].y();
    for (int b = 1; b <= maxBeams; ++b) {
        int thickness, spacing;
        if (b == 1) {
            thickness = beamThickness1;
            spacing = beamSpacing1;
        } else if (b == 2) {
            thickness = beamThickness2;
            spacing = beamSpacing2;
        } else if (b == 3) {
            thickness = beamThickness3;
            spacing = beamSpacing3;
        } else if (b == 4) {
            thickness = beamThickness4;
            spacing = beamSpacing4;
        } else {
            thickness = beamThickness4; // fallback for extra beams
            spacing = beamSpacing4;
        }

        if (b == 1) {
            y_beam = stemTops[group.front()].y();
        } else {
            y_beam += thickness + spacing;
        }

        for (size_t k = 0; k < group.size(); ++k) {
            int i = group[k];
            int x = stemTips[i].x();
            if (beamsForNoteType(noteTypes[i]) < b) continue;

            if (k > 0 && beamsForNoteType(noteTypes[group[k-1]]) >= b) {
                int x_prev = stemTips[group[k-1]].x();
                p.fillRect(QRect(std::min(x, x_prev), y_beam, std::abs(x - x_prev), thickness), Qt::white);
            }
            else if (b > 1 && k > 0 && beamsForNoteType(noteTypes[group[k-1]]) < b &&
                     !(k+1 < group.size() && beamsForNoteType(noteTypes[group[k+1]]) >= b)) {
                int stubLen = thickness * 4;
                p.fillRect(QRect(x - stubLen, y_beam, stubLen, thickness), Qt::white);
            }
            if (b > 1 && k+1 < group.size() && beamsForNoteType(noteTypes[group[k+1]]) < b &&
                !(k > 0 && beamsForNoteType(noteTypes[group[k-1]]) >= b)) {
                int stubLen = thickness * 4;
                p.fillRect(QRect(x, y_beam, stubLen, thickness), Qt::white);
            }
        }
    }
}

    //--- Draw flags for single notes (not beamed, visually attached)
    for (int i = 0; i < noteCount; ++i) {
        if (isRestType(noteTypes[i])) continue;
        bool isBeamed = false;
        for (const auto& group : beamGroups)
            if (std::find(group.begin(), group.end(), i) != group.end() && group.size() > 1)
                isBeamed = true;
        if (!isBeamed && beamsForNoteType(noteTypes[i]) > 0) {
            QString flagSvg = svgForFlag(noteTypes[i]);
            if (!flagSvg.isEmpty()) {
                QPixmap flagPix = whiteSvgToPixmap(flagSvg, QSize(noteheadWidth * 1.65, noteheadHeight * 1.80));
                int x_flag = stemTips[i].x() - int(noteheadWidth * 0.5267); // 5px at 12px
                int y_flag = stemTips[i].y() + int(noteheadWidth * 0.0833); // 1px at 12px
                p.drawPixmap(x_flag, y_flag, flagPix);
            }
        }
    }

    //--- Tuplet bracket and number (white, always visible, centered above)
    if (config.tupletNumber > 1 && noteCount > 1) {
        int firstIdx = 0, lastIdx = noteCount - 1;
        int firstNoteIdx = -1, lastNoteIdx = -1;
        for (int i = 0; i < noteCount; ++i) {
            if (!isRestType(noteTypes[i])) {
                if (firstNoteIdx == -1) firstNoteIdx = i;
                lastNoteIdx = i;
            }
        }
        if (firstNoteIdx == -1) firstNoteIdx = firstIdx;
        if (lastNoteIdx == -1) lastNoteIdx = lastIdx;

        // Bracket ends
        int bracketPadLength = int(noteheadWidth * 0.5); // 6px at 12px
        int x_left = x_positions[firstIdx] + (isRestType(noteTypes[firstIdx]) ? restSize.width() / 2 : noteheadSize.width() / 2) - bracketPadLength;
        int x_right = x_positions[lastIdx] + (isRestType(noteTypes[lastIdx]) ? restSize.width() / 2 : noteheadSize.width() / 2) + bracketPadLength;

        // Bracket vertical position
        int y_bracket = stemTops[firstNoteIdx].y() - int(noteheadHeight * 0.5833); // 7px at 12px

        // number, centered
        QString numberSvg = svgForTupletNumber(config.tupletNumber);
        int numberSize = int(noteheadHeight * 0.6667); // 8px at 12px
        QPixmap numberPix = whiteSvgToPixmap(numberSvg, QSize(numberSize, numberSize));
        int x_num = (x_left + x_right) / 2 - numberPix.width() / 2;
        int y_num = y_bracket - numberPix.height() + int(noteheadHeight * 0.1667); // 2px at 12px

        if (config.tupletNumber == 3) {
            p.drawPixmap(x_num, y_num, numberPix);
        } else {
            int gap = numberPix.width() + int(noteheadWidth * 0.6667); // 8px at 12px
            int gapPadding = int(noteheadWidth * 0.3333); // 4px at 12px
            int x_gap_left = x_num - gapPadding;
            int x_gap_right = x_num + numberPix.width() + gapPadding;

            int bracketThickness = int(noteheadHeight * 0.1667); // 2px at 12px
            p.setPen(QPen(Qt::white, bracketThickness));

            // left segment of bracket
            p.drawLine(x_left, y_bracket, x_gap_left, y_bracket);
            // right segment of bracket
            p.drawLine(x_gap_right, y_bracket, x_right, y_bracket);

            int bracketVerticalLength = int(noteheadHeight * 0.25); // 3px at 12px
            p.drawLine(x_left, y_bracket, x_left, y_bracket + bracketVerticalLength);
            p.drawLine(x_right, y_bracket, x_right, y_bracket + bracketVerticalLength);

            p.drawPixmap(x_num, y_num, numberPix);
        }
    }

    p.end();
    return result;
}