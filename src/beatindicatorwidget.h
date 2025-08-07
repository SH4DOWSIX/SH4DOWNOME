#pragma once

#include <QWidget>
#include <QColor>
#include <vector>

// Visual mode: classic circles or polyrhythm grid
enum class BeatIndicatorMode {
    Circles, // normal
    PolyrhythmGrid // grid, as in reference image
};

class BeatIndicatorWidget : public QWidget {
    Q_OBJECT
public:
    explicit BeatIndicatorWidget(QWidget *parent = nullptr);

    // Circles mode
    void setBeats(int beats);
    void setSubdivisions(int subdivisions);
    void setCurrent(int currentBeat, int currentSub);
    void setAccentColor(const QColor& color);

    // Polyrhythm grid mode
    void setPolyrhythmGrid(int mainBeats, int polyBeats, int highlightStep = -1);

    void setMode(BeatIndicatorMode mode);

protected:
    void paintEvent(QPaintEvent *event) override;

private:
    // For circles mode
    int m_beats = 4;
    int m_subdivisions = 1;
    int m_currentBeat = 0;
    int m_currentSub = 0;
    QColor m_accentColor = QColor(150,0,0);

    // For grid mode
    int m_polyMain = 0;
    int m_polySub = 0;
    int m_gridHighlight = -1; // which column is active/highlighted
    BeatIndicatorMode m_mode = BeatIndicatorMode::Circles;

    // Helper
    int lcm(int a, int b) const;
};