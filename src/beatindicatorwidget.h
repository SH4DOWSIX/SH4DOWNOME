#pragma once

#include <QWidget>
#include <QColor>

class BeatIndicatorWidget : public QWidget {
    Q_OBJECT
public:
    explicit BeatIndicatorWidget(QWidget *parent = nullptr);

    void setBeats(int beats);
    void setSubdivisions(int subdivisions);
    void setCurrent(int currentBeat, int currentSub);
    void setAccentColor(const QColor& color);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    int m_beats = 4;
    int m_subdivisions = 1;
    int m_currentBeat = 0;
    int m_currentSub = 0;
    QColor m_accentColor = QColor(150,0,0);
};