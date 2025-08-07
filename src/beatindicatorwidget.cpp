#include "beatindicatorwidget.h"
#include <QPainter>
#include <QtMath>
#include <algorithm>

BeatIndicatorWidget::BeatIndicatorWidget(QWidget *parent)
    : QWidget(parent) {
    setMinimumHeight(80);
    m_accentColor = QColor(150,0,0);
}

void BeatIndicatorWidget::setAccentColor(const QColor& color) {
    m_accentColor = color;
    update();
}

void BeatIndicatorWidget::setBeats(int beats) {
    m_beats = qMax(1, beats);
    update();
}

void BeatIndicatorWidget::setSubdivisions(int subdivisions) {
    m_subdivisions = qMax(1, subdivisions);
    update();
}

void BeatIndicatorWidget::setCurrent(int currentBeat, int currentSub) {
    m_currentBeat = currentBeat;
    m_currentSub = currentSub;
    update();
}

void BeatIndicatorWidget::setPolyrhythmGrid(int mainBeats, int polyBeats, int highlightStep) {
    m_polyMain = mainBeats;
    m_polySub = polyBeats;
    m_gridHighlight = highlightStep;
    m_mode = BeatIndicatorMode::PolyrhythmGrid;
    update();
}

void BeatIndicatorWidget::setMode(BeatIndicatorMode mode) {
    m_mode = mode;
    update();
}

int BeatIndicatorWidget::lcm(int a, int b) const {
    if (a == 0 || b == 0) return 0;
    int x = a, y = b;
    while (y != 0) {
        int t = y;
        y = x % y;
        x = t;
    }
    return (a / x) * b;
}

void BeatIndicatorWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter p(this);

    if (m_mode == BeatIndicatorMode::PolyrhythmGrid && m_polyMain > 0 && m_polySub > 0) {
        // --- GRID MODE ---
        p.setRenderHint(QPainter::Antialiasing, false); // CRISP SQUARES

        int columns = lcm(m_polyMain, m_polySub);
        if (columns == 0) columns = std::max(m_polyMain, m_polySub);

        const int maxCellSize = 32;
        int rows = 2;

        int availW = width();
        int availH = height();
        int cellSize = std::min({maxCellSize, availW / std::max(columns, 1), availH / rows});

        int gridW = cellSize * columns;
        int gridH = cellSize * rows;
        int x0 = (width() - gridW) / 2;
        int y0 = (height() - gridH) / 2;

        std::vector<bool> mainHits(columns, false);
        std::vector<bool> polyHits(columns, false);
        for (int i = 0; i < m_polyMain; ++i)
            mainHits[(i * columns) / m_polyMain] = true;
        for (int i = 0; i < m_polySub; ++i)
            polyHits[(i * columns) / m_polySub] = true;

        // Draw filled colored squares
        for (int col = 0; col < columns; ++col) {
            for (int row = 0; row < rows; ++row) {
                int logicalRow = row;
                int x = x0 + col * cellSize;
                int y = y0 + row * cellSize;
                QRect cell(x, y, cellSize, cellSize);

                bool isMain = (logicalRow == 1) && mainHits[col];
                bool isPoly = (logicalRow == 0) && polyHits[col];
                bool isCoincide = mainHits[col] && polyHits[col];

                QColor color = QColor(20, 20, 20);
                if (isCoincide)
                    color = m_accentColor.lighter(140); // lighter accent for overlap (optional)
                else if (isMain)
                    color = m_accentColor;
                else if (isPoly)
                    color = m_accentColor.darker(180); // dark accent for poly

                if (col == m_gridHighlight && (isMain || isPoly)) {
                    color = Qt::white;
                }

                p.fillRect(cell, color);
            }
        }

        // Draw sharp grid lines
        p.setPen(QPen(QColor(80,80,80), 1));
        for (int col = 0; col <= columns; ++col) {
            int x = x0 + col * cellSize;
            p.drawLine(x, y0, x, y0 + gridH);
        }
        for (int row = 0; row <= rows; ++row) {
            int y = y0 + row * cellSize;
            p.drawLine(x0, y, x0 + gridW, y);
        }

        return;
    }

    // --- CIRCLES MODE ---
    p.setRenderHint(QPainter::Antialiasing, true); // BEAUTIFUL CIRCLES

    const int maxPerRow = 12;
    const int minCircle = 18;
    const int maxCircle = 48;
    const int minHSpacing = 6;
    const int minVSpacing = 6;
    const int hBorder = 8;
    const int vBorder = 8;

    int numRows = (m_beats + maxPerRow - 1) / maxPerRow;
    QVector<int> rowCounts;
    int beatsRemaining = m_beats;
    for (int i = 0; i < numRows; ++i) {
        int n = std::min(beatsRemaining, maxPerRow);
        rowCounts.append(n);
        beatsRemaining -= n;
    }

    int availW = width() - 2 * hBorder;
    int availH = height() - 2 * vBorder;

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
    int y0 = std::max(vBorder, (height() - totalHeight) / 2);

    int beatIdx = 0;
    for (int row = 0; row < numRows; ++row) {
        int n = rowCounts[row];
        int rowWidth = n * circleSize + (n - 1) * minHSpacing;
        int x0 = std::max(hBorder, (width() - rowWidth) / 2);
        int y = y0 + row * (circleSize + minVSpacing);

        for (int col = 0; col < n; ++col) {
            if (beatIdx >= m_beats) break;
            QRectF rect(x0 + col * (circleSize + minHSpacing), y, circleSize, circleSize);

            p.setBrush(QColor(100, 100, 100));
            p.setPen(Qt::NoPen);
            p.drawEllipse(rect);

            if (beatIdx == m_currentBeat) {
                p.setBrush(m_accentColor);
                p.setPen(Qt::NoPen);
                p.drawEllipse(rect);
            }

            p.setPen(QColor(0, 0, 0));
            p.setBrush(Qt::NoBrush);
            p.drawEllipse(rect);

            double cx = rect.center().x();
            double cy = rect.center().y();
            double r = circleSize / 2.3;
            for (int sub = 0; sub < m_subdivisions; ++sub) {
                double angle = ((double)sub / m_subdivisions) * 2 * M_PI - M_PI_2;
                double rx = cx + r * cos(angle);
                double ry = cy + r * sin(angle);
                QColor dotColor = (beatIdx == m_currentBeat && sub == m_currentSub) ? QColor(255, 255, 255) : QColor(0, 0, 0);
                p.setBrush(dotColor);
                p.setPen(Qt::NoPen);
                p.drawEllipse(QPointF(rx, ry), std::max(2, circleSize / 12), std::max(2, circleSize / 12));
            }
            beatIdx++;
        }
    }
}