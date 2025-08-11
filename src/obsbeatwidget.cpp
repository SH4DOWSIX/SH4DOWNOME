#include "obsbeatwidget.h"
#include <QPainter>
#include "svgutils.h"

OBSBeatWidget::OBSBeatWidget(QWidget *parent)
    : QWidget(parent)
{
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    m_pulseOn = false;
    m_playing = false;
    m_tempo = 0;
    m_polyrhythmMode = false;
    m_polyMain = 0;
    m_polySub = 0;
}

void OBSBeatWidget::setPlaying(bool playing) {
    if (m_playing != playing) {
        m_playing = playing;
        update();
    }
}

void OBSBeatWidget::setTempo(int tempo) {
    if (m_tempo != tempo) {
        m_tempo = tempo;
        update();
    }
}

void OBSBeatWidget::setSubdivisionImagePath(const QString &svgPath) {
    m_subdivisionSvgPath = svgPath;
    regenerateSubdivisionPixmap();
}

void OBSBeatWidget::setPulseOn(bool pulse) {
    if (m_pulseOn != pulse) {
        m_pulseOn = pulse;
        update();
    }
}

// ---- NEW: Set polyrhythm display mode and beats ----
void OBSBeatWidget::setPolyrhythmMode(bool enabled, int mainBeats, int polyBeats) {
    if (m_polyrhythmMode != enabled || m_polyMain != mainBeats || m_polySub != polyBeats) {
        m_polyrhythmMode = enabled;
        m_polyMain = mainBeats;
        m_polySub = polyBeats;
        update();
    }
}

void OBSBeatWidget::regenerateSubdivisionPixmap() {
    if (!m_subdivisionSvgPath.isEmpty()) {
        QSize pixSize = size();
        if (pixSize.width() < 64 || pixSize.height() < 64) pixSize = QSize(128, 128);
        m_subdivisionPixmap = svgToPixmap(m_subdivisionSvgPath, pixSize);
    } else {
        m_subdivisionPixmap = QPixmap();
    }
    update();
}

void OBSBeatWidget::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    regenerateSubdivisionPixmap();
}

void OBSBeatWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);

    p.fillRect(rect(), Qt::black);

    int margin = qMin(width(), height()) / 12;
    int circleSize = qMin(width(), height()) - 2 * margin;
    if (circleSize < 20) circleSize = qMin(width(), height());

    QPoint center(width() / 2, height() / 2);
    QRect circleRect(center.x() - circleSize / 2, center.y() - circleSize / 2, circleSize, circleSize);

    int pulseDiameter = int(circleSize * 1.08);
    QRect pulseRect(center.x() - pulseDiameter / 2, center.y() - pulseDiameter / 2, pulseDiameter, pulseDiameter);
    QColor pulseColor = m_pulseOn ? QColor("#ffffff") : Qt::black;
    p.setBrush(pulseColor);
    p.setPen(Qt::NoPen);
    p.drawEllipse(pulseRect);

    QColor color = QColor("#000000");
    p.setBrush(color);
    p.setPen(Qt::NoPen);
    p.drawEllipse(circleRect);

    // -- Prepare block heights --
    QString tempoStr = QString::number(m_tempo);
    QFont font = this->font();
    font.setBold(true);

    // 1. Tempo
    int tempoFontPx = int(circleSize * 0.35);
    font.setPixelSize(tempoFontPx);
    p.setFont(font);
    QFontMetrics tempoFm(font);
    int tempoW = tempoFm.horizontalAdvance(tempoStr);
    int tempoH = tempoFm.height();

    // 2. Spacing between tempo and subdivision/poly
    int spacing = int(circleSize * 0.05);

    // 3. Subdivision image or polyrhythm ratio
    int blockW = tempoW;
    int blockH = tempoH + spacing;
    int subImgH = 0, subImgW = 0, ratioW = 0, ratioH = 0, ratioFontPx = 0;

    if (m_polyrhythmMode) {
        // Prepare ratio font
        QString ratio = QString("%1 : %2").arg(m_polyMain).arg(m_polySub);
        QFont ratioFont = font;
        ratioFont.setBold(true);
        ratioFontPx = int(circleSize * 0.29);
        ratioFont.setPixelSize(ratioFontPx);
        QFontMetrics rfm(ratioFont);
        ratioW = rfm.horizontalAdvance(ratio);
        ratioH = rfm.height();
        // Ensure fits inside circle
        int maxBlockW = int(circleSize * 0.90);
        int maxRatioH = int(circleSize * 0.29);
        while ((ratioW > maxBlockW || ratioH > maxRatioH) && ratioFontPx > 5) {
            ratioFontPx -= 1;
            ratioFont.setPixelSize(ratioFontPx);
            rfm = QFontMetrics(ratioFont);
            ratioW = rfm.horizontalAdvance(ratio);
            ratioH = rfm.height();
        }
        blockW = std::max(tempoW, ratioW);
        blockH += ratioH;
    } else if (!m_subdivisionPixmap.isNull()) {
        QSize pixSize = m_subdivisionPixmap.size();
        double scale = double(circleSize * 0.30) / pixSize.height();
        subImgW = int(pixSize.width() * scale);
        subImgH = int(pixSize.height() * scale);
        blockW = std::max(tempoW, subImgW);
        blockH += subImgH;
    } else {
        // fallback: just tempo
    }

    // --- Center the block in the circle ---
    int blockX = center.x() - blockW/2;
    int blockY = center.y() - blockH/2;

    // --- Draw tempo ---
    QRect tempoRect(center.x() - tempoW/2, blockY, tempoW, tempoH);
    QColor textColor = m_playing ? Qt::white : Qt::black;
    p.setFont(font);
    p.setPen(textColor);
    p.drawText(tempoRect, Qt::AlignCenter, tempoStr);

    // --- Draw subdivision/poly directly below, with spacing ---
    if (m_polyrhythmMode) {
        int ratioY = tempoRect.bottom() + spacing;
        QRect ratioRect(center.x() - ratioW/2, ratioY, ratioW, ratioH);
        QFont ratioFont = font;
        ratioFont.setBold(true);
        ratioFont.setPixelSize(ratioFontPx);
        p.setFont(ratioFont);
        QColor ratioColor = m_playing ? Qt::white : Qt::black;
        p.setPen(ratioColor);
        QString ratio = QString("%1 : %2").arg(m_polyMain).arg(m_polySub);
        p.drawText(ratioRect, Qt::AlignCenter, ratio);
    } else if (!m_subdivisionPixmap.isNull()) {
        int imgY = tempoRect.bottom() + spacing;
        int imgX = center.x() - subImgW/2;
        QPixmap scaled = m_subdivisionPixmap.scaled(subImgW, subImgH, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        QPixmap tinted(scaled.size());
        tinted.fill(Qt::transparent);

        QPainter tintPainter(&tinted);
        tintPainter.drawPixmap(0, 0, scaled);
        QColor tintColor = m_playing ? QColor(0, 255, 0, 0) : QColor(0, 0, 0, 255);
        tintPainter.fillRect(tinted.rect(), tintColor);
        tintPainter.end();

        p.drawPixmap(imgX, imgY, subImgW, subImgH, tinted);
    }
}