#include "BeatIndicatorItem.h"
#include <QPainter>
#include <QtMath>
#include <algorithm>
#include <cmath>

BeatIndicatorItem::BeatIndicatorItem(QQuickItem* parent)
    : QQuickPaintedItem(parent)
{
    setRenderTarget(QQuickPaintedItem::Image);
    setAntialiasing(true);
}

int BeatIndicatorItem::lcmHelper(int a, int b)
{
    if (a == 0 || b == 0) return 0;
    int x = a, y = b;
    while (y != 0) {
        int t = y;
        y = x % y;
        x = t;
    }
    return (a / x) * b;
}

void BeatIndicatorItem::setBeats(int v)        { if (m_beats != v)        { m_beats = qMax(1,v);         emit beatsChanged();         update(); } }
void BeatIndicatorItem::setSubdivisions(int v) { if (m_subdivisions != v)  { m_subdivisions = qMax(1,v);  emit subdivisionsChanged();   update(); } }
void BeatIndicatorItem::setCurrentBeat(int v)  { if (m_currentBeat != v)   { m_currentBeat = v;           emit currentChanged();        update(); } }
void BeatIndicatorItem::setCurrentSub(int v)   { if (m_currentSub != v)    { m_currentSub = v;            emit currentChanged();        update(); } }
void BeatIndicatorItem::setAccentColor(const QColor& v) { if (m_accentColor != v) { m_accentColor = v;   emit accentColorChanged();    update(); } }
void BeatIndicatorItem::setBackgroundColor(const QColor& v) { if (m_backgroundColor != v) { m_backgroundColor = v; emit backgroundColorChanged(); update(); } }
void BeatIndicatorItem::setCircleBorderColor(const QColor& v) { if (m_circleBorderColor != v) { m_circleBorderColor = v; emit circleBorderColorChanged(); update(); } }
void BeatIndicatorItem::setCircleBorderWidth(qreal v) { if (m_circleBorderWidth != v) { m_circleBorderWidth = v; emit circleBorderWidthChanged(); update(); } }
void BeatIndicatorItem::setMode(int v)          { if (m_mode != v)           { m_mode = v;                 emit modeChanged();           update(); } }
void BeatIndicatorItem::setPolyMain(int v)      { if (m_polyMain != v)       { m_polyMain = qMax(1,v);     emit polyChanged();           update(); } }
void BeatIndicatorItem::setPolySecondary(int v) { if (m_polySub != v)        { m_polySub = qMax(1,v);      emit polyChanged();           update(); } }
void BeatIndicatorItem::setGridHighlight(int v) { if (m_gridHighlight != v)  { m_gridHighlight = v;        emit gridHighlightChanged();  update(); } }

void BeatIndicatorItem::paint(QPainter* p)
{
    int w = static_cast<int>(width());
    int h = static_cast<int>(height());

    // Always clear to the app background colour first to prevent stale-pixel artifacts
    p->fillRect(0, 0, w, h, m_backgroundColor);

    if (m_mode == 1 && m_polyMain > 0 && m_polySub > 0) {
        p->setRenderHint(QPainter::Antialiasing, false);

        int columns = lcmHelper(m_polyMain, m_polySub);
        if (columns == 0) columns = std::max(m_polyMain, m_polySub);

        const int maxCellSize = 32;
        const int rows = 2;

        int cellSize = std::min({maxCellSize, w / std::max(columns, 1), h / rows});

        int gridW = cellSize * columns;
        int gridH = cellSize * rows;
        int x0 = (w - gridW) / 2;
        int y0 = (h - gridH) / 2;

        std::vector<bool> mainHits(columns, false);
        std::vector<bool> polyHits(columns, false);
        for (int i = 0; i < m_polyMain; ++i)
            mainHits[(i * columns) / m_polyMain] = true;
        for (int i = 0; i < m_polySub; ++i)
            polyHits[(i * columns) / m_polySub] = true;

        for (int col = 0; col < columns; ++col) {
            for (int row = 0; row < rows; ++row) {
                int x = x0 + col * cellSize;
                int y = y0 + row * cellSize;
                QRect cell(x, y, cellSize, cellSize);

                bool isMain    = (row == 1) && mainHits[col];
                bool isPoly    = (row == 0) && polyHits[col];
                bool isCoincide = mainHits[col] && polyHits[col];

                QColor color(100, 100, 100);
                if (isCoincide)
                    color = m_accentColor.lighter(120);
                else if (isMain)
                    color = m_accentColor.darker(200);
                else if (isPoly)
                    color = m_accentColor.darker(120);

                if (col == m_gridHighlight && (isMain || isPoly))
                    color = Qt::white;

                p->fillRect(cell, color);
            }
        }

        p->setPen(QPen(QColor(0, 0, 0), 1));
        for (int col = 0; col <= columns; ++col)
            p->drawLine(x0 + col * cellSize, y0, x0 + col * cellSize, y0 + gridH);
        for (int row = 0; row <= rows; ++row)
            p->drawLine(x0, y0 + row * cellSize, x0 + gridW, y0 + row * cellSize);
        return;
    }

    // ---- CIRCLES MODE ----
    p->setRenderHint(QPainter::Antialiasing, true);

    const int maxPerRow   = 12;
    const int minCircle   = 18;
    const int maxCircle   = 48;
    const int minHSpacing = 6;
    const int minVSpacing = 6;
    const int hBorder     = 8;
    const int vBorder     = 8;

    int numRows = (m_beats + maxPerRow - 1) / maxPerRow;
    QVector<int> rowCounts;
    int beatsRemaining = m_beats;
    for (int i = 0; i < numRows; ++i) {
        int n = std::min(beatsRemaining, maxPerRow);
        rowCounts.append(n);
        beatsRemaining -= n;
    }

    int availW = w - 2 * hBorder;
    int availH = h - 2 * vBorder;

    int circleSizeW = maxCircle;
    for (int n : rowCounts) {
        if (n == 0) continue;
        int s = (availW - (n - 1) * minHSpacing) / n;
        circleSizeW = std::min(circleSizeW, s);
    }

    int circleSizeH = maxCircle;
    if (numRows > 0) {
        int totalMinVSpacing = (numRows - 1) * minVSpacing;
        int s = (availH - totalMinVSpacing) / numRows;
        circleSizeH = std::min(maxCircle, s);
    }

    int circleSize = std::max(minCircle, std::min(circleSizeW, circleSizeH));
    int totalHeight = numRows * circleSize + (numRows - 1) * minVSpacing;
    int y0 = std::max(vBorder, (h - totalHeight) / 2);

    int beatIdx = 0;
    for (int row = 0; row < numRows; ++row) {
        int n = rowCounts[row];
        int rowWidth = n * circleSize + (n - 1) * minHSpacing;
        int x0 = std::max(hBorder, (w - rowWidth) / 2);
        int y = y0 + row * (circleSize + minVSpacing);

        for (int col = 0; col < n; ++col) {
            if (beatIdx >= m_beats) break;
            QRectF rect(x0 + col * (circleSize + minHSpacing), y, circleSize, circleSize);

            p->setBrush(QColor(100, 100, 100));
            p->setPen(Qt::NoPen);
            p->drawEllipse(rect);

            if (beatIdx == m_currentBeat) {
                p->setBrush(m_accentColor);
                p->setPen(Qt::NoPen);
                p->drawEllipse(rect);
            }

            p->setPen(QPen(m_circleBorderColor, m_circleBorderWidth));
            p->setBrush(Qt::NoBrush);
            p->drawEllipse(rect);

            double cx = rect.center().x();
            double cy = rect.center().y();
            double r = circleSize / 2.3;
            for (int sub = 0; sub < m_subdivisions; ++sub) {
                double angle = ((double)sub / m_subdivisions) * 2 * M_PI - M_PI_2;
                double rx = cx + r * std::cos(angle);
                double ry = cy + r * std::sin(angle);
                QColor dotColor = (beatIdx == m_currentBeat && sub == m_currentSub)
                                ? QColor(255, 255, 255) : QColor(0, 0, 0);
                p->setBrush(dotColor);
                p->setPen(Qt::NoPen);
                p->drawEllipse(QPointF(rx, ry),
                               std::max(2, circleSize / 12),
                               std::max(2, circleSize / 12));
            }
            beatIdx++;
        }
    }
}
