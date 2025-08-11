#pragma once
#include <QWidget>
#include <QPixmap>
#include <QString>

class OBSBeatWidget : public QWidget
{
    Q_OBJECT
public:
    explicit OBSBeatWidget(QWidget *parent = nullptr);

    void setPlaying(bool playing);
    void setTempo(int tempo);
    void setSubdivisionImagePath(const QString &svgPath);
    void setPulseOn(bool pulse);

    // ---- Add Polyrhythm API ----
    void setPolyrhythmMode(bool enabled, int mainBeats = 0, int polyBeats = 0);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    void regenerateSubdivisionPixmap();

    int m_tempo = 0;
    bool m_playing = false;
    bool m_pulseOn = false;
    QString m_subdivisionSvgPath;
    QPixmap m_subdivisionPixmap;

    // ---- Add Polyrhythm State ----
    bool m_polyrhythmMode = false;
    int m_polyMain = 0;
    int m_polySub = 0;
};