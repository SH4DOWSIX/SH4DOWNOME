#pragma once
#include <QWidget>
#include <QPixmap>

class OBSBeatWidget : public QWidget
{
    Q_OBJECT
public:
    explicit OBSBeatWidget(QWidget *parent = nullptr);

    void setPlaying(bool playing);
    void setTempo(int tempo);
    void setPulseOn(bool pulse);
    void setSubdivisionPixmap(const QPixmap &pixmap);

    // ---- Polyrhythm API ----
    void setPolyrhythmMode(bool enabled, int mainBeats = 0, int polyBeats = 0);

protected:
    void paintEvent(QPaintEvent *event) override;
    void resizeEvent(QResizeEvent *event) override;

private:
    int m_tempo = 0;
    bool m_playing = false;
    bool m_pulseOn = false;
    QPixmap m_subdivisionPixmap;

    // ---- Polyrhythm State ----
    bool m_polyrhythmMode = false;
    int m_polyMain = 0;
    int m_polySub = 0;
};