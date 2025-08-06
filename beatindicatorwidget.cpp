#include "beatindicatorwidget.h"
#include <QPainter>
#include <QtMath>

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

void BeatIndicatorWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

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