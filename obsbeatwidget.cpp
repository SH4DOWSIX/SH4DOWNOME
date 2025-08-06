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

    QPoint center(width()/2, height()/2);
    QRect circleRect(center.x() - circleSize/2, center.y() - circleSize/2, circleSize, circleSize);

    int pulseDiameter = int(circleSize * 1.08);
    QRect pulseRect(center.x() - pulseDiameter/2, center.y() - pulseDiameter/2, pulseDiameter, pulseDiameter);
    QColor pulseColor = m_pulseOn ? QColor("#ffffff") : Qt::black;
    p.setBrush(pulseColor);
    p.setPen(Qt::NoPen);
    p.drawEllipse(pulseRect);

    QColor color = QColor("#000000");
    p.setBrush(color);
    p.setPen(Qt::NoPen);
    p.drawEllipse(circleRect);

    QString tempoStr = QString::number(m_tempo);
    QFont font = this->font();
    font.setBold(true);
    int textHeight = circleSize * 0.35;
    font.setPixelSize(textHeight);
    p.setFont(font);
    QFontMetrics fm(font);
    int textW = fm.horizontalAdvance(tempoStr);
    int textH = fm.height();

    int imgMaxH = circleSize * 0.30;
    int spacing = circleSize * 0.06;

    int groupH = textH + (m_subdivisionPixmap.isNull() ? 0 : (spacing + imgMaxH));
    int groupY = circleRect.top() + (circleRect.height() - groupH) / 2;

    QRect textRect(center.x() - textW/2, groupY, textW, textH);
    QColor textColor = m_playing ? Qt::white : Qt::black;
    p.setPen(textColor);
    p.drawText(textRect, Qt::AlignCenter, tempoStr);

    if (!m_subdivisionPixmap.isNull()) {
        QSize pixSize = m_subdivisionPixmap.size();
        double scale = double(imgMaxH) / pixSize.height();
        int imgW = int(pixSize.width() * scale);
        int imgH = int(pixSize.height() * scale);
        int imgX = center.x() - imgW/2;
        int imgY = textRect.bottom() + spacing;

        QPixmap scaled = m_subdivisionPixmap.scaled(imgW, imgH, Qt::KeepAspectRatio, Qt::SmoothTransformation);

        QPixmap tinted(scaled.size());
        tinted.fill(Qt::transparent);

        QPainter tintPainter(&tinted);
        tintPainter.drawPixmap(0, 0, scaled);
        QColor tintColor = m_playing ? QColor(0, 255, 0, 0) : QColor(0, 0, 0, 255);
        tintPainter.fillRect(tinted.rect(), tintColor);
        tintPainter.end();

        p.drawPixmap(imgX, imgY, imgW, imgH, tinted);
    }
}