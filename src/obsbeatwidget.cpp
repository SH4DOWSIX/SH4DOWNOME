#include "obsbeatwidget.h"
#include <QPainter>

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

void OBSBeatWidget::setSubdivisionPixmap(const QPixmap &pixmap) {
    m_subdivisionPixmap = pixmap;
    update();
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

void OBSBeatWidget::setPulseOn(bool pulse) {
    if (m_pulseOn != pulse) {
        m_pulseOn = pulse;
        update();
    }
}

void OBSBeatWidget::setPolyrhythmMode(bool enabled, int mainBeats, int polyBeats) {
    if (m_polyrhythmMode != enabled || m_polyMain != mainBeats || m_polySub != polyBeats) {
        m_polyrhythmMode = enabled;
        m_polyMain = mainBeats;
        m_polySub = polyBeats;
        update();
    }
}

void OBSBeatWidget::resizeEvent(QResizeEvent *event) {
    QWidget::resizeEvent(event);
    // No pixmap regeneration here; MainWindow is responsible for size!
}

void OBSBeatWidget::paintEvent(QPaintEvent *event) {
    Q_UNUSED(event);
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.setRenderHint(QPainter::SmoothPixmapTransform);

    // --- Background ---
    p.fillRect(rect(), Qt::black);

    int margin = qMin(width(), height()) / 12;
    int circleSize = qMin(width(), height()) - 2 * margin;
    if (circleSize < 20) circleSize = qMin(width(), height());

    QPoint center(width() / 2, height() / 2);
    QRect circleRect(center.x() - circleSize / 2, center.y() - circleSize / 2, circleSize, circleSize);

    // --- Pulse "flash" circle ---
    int pulseDiameter = int(circleSize * 1.08);
    QRect pulseRect(center.x() - pulseDiameter / 2, center.y() - pulseDiameter / 2, pulseDiameter, pulseDiameter);
    QColor pulseColor = m_pulseOn ? QColor("#ffffff") : Qt::black;
    p.setBrush(pulseColor);
    p.setPen(Qt::NoPen);
    p.drawEllipse(pulseRect);

    // --- Main circle ---
    QColor mainCircleColor = QColor("#000000");
    p.setBrush(mainCircleColor);
    p.setPen(Qt::NoPen);
    p.drawEllipse(circleRect);

    // --- Prepare block: tempo, subdivision image, poly text ---
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

    // 2. Subdivision image
    int subdivImgW = 0, subdivImgH = 0;
    if (!m_polyrhythmMode && !m_subdivisionPixmap.isNull()) {
        subdivImgW = m_subdivisionPixmap.width();
        subdivImgH = m_subdivisionPixmap.height();
    }

    // 3. Polyrhythm text (if in polyrhythm mode)
    QString polyStr;
    int polyW = 0, polyH = 0;
    if (m_polyrhythmMode && m_polyMain > 0 && m_polySub > 0) {
        polyStr = QString("%1 : %2").arg(m_polyMain).arg(m_polySub);
        QFont polyFont = font;
        polyFont.setPixelSize(int(circleSize * 0.17));
        p.setFont(polyFont);
        QFontMetrics polyFm(polyFont);
        polyW = polyFm.horizontalAdvance(polyStr);
        polyH = polyFm.height();
    }

    // --- Calculate block height ---
    int spacing = int(circleSize * - 0.02); // vertical spacing between items
    int blockH = tempoH;
    if (subdivImgH > 0) blockH += spacing + subdivImgH;
    if (polyH > 0) blockH += spacing + polyH;

    // --- Block top ---
    int blockTop = center.y() - blockH / 2;
    int y = blockTop;

    // --- Draw Tempo ---
    p.setFont(font);
    QColor textColor = m_playing ? Qt::white : Qt::black;
    p.setPen(textColor);
    QRect tempoRect(center.x() - tempoW/2, y, tempoW, tempoH);
    p.drawText(tempoRect, Qt::AlignCenter, tempoStr);
    y += tempoH;

    // --- Draw Subdivision Image ---
    if (subdivImgH > 0) {
        y += spacing;
        int imgX = center.x() - subdivImgW / 2;
        if (m_playing) {
            p.setCompositionMode(QPainter::CompositionMode_SourceOver);
            p.drawPixmap(imgX, y, m_subdivisionPixmap);
        } else {
            p.setOpacity(0.0);
            p.drawPixmap(imgX, y, m_subdivisionPixmap);
            p.setOpacity(1.0);
        }
        y += subdivImgH;
    }

    // --- Draw Polyrhythm Text ---
    if (polyH > 0) {
        y += spacing;
        QFont polyFont = font;
        polyFont.setPixelSize(int(circleSize * 0.17));
        p.setFont(polyFont);
        QRect polyRect(center.x() - polyW/2, y, polyW, polyH);
        QColor polyColor = m_playing ? Qt::white : QColor("#000000");
        p.setPen(polyColor);
        p.drawText(polyRect, Qt::AlignCenter, polyStr);
        y += polyH;
    }
}