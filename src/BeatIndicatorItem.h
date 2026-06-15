#pragma once

#include <QQuickPaintedItem>
#include <QColor>

class BeatIndicatorItem : public QQuickPaintedItem {
    Q_OBJECT
    Q_PROPERTY(int beats READ beats WRITE setBeats NOTIFY beatsChanged)
    Q_PROPERTY(int subdivisions READ subdivisions WRITE setSubdivisions NOTIFY subdivisionsChanged)
    Q_PROPERTY(int currentBeat READ currentBeat WRITE setCurrentBeat NOTIFY currentChanged)
    Q_PROPERTY(int currentSub READ currentSub WRITE setCurrentSub NOTIFY currentChanged)
    Q_PROPERTY(QColor accentColor READ accentColor WRITE setAccentColor NOTIFY accentColorChanged)
    Q_PROPERTY(QColor backgroundColor READ backgroundColor WRITE setBackgroundColor NOTIFY backgroundColorChanged)
    Q_PROPERTY(QColor circleBorderColor READ circleBorderColor WRITE setCircleBorderColor NOTIFY circleBorderColorChanged)
    Q_PROPERTY(qreal circleBorderWidth READ circleBorderWidth WRITE setCircleBorderWidth NOTIFY circleBorderWidthChanged)
    Q_PROPERTY(int mode READ mode WRITE setMode NOTIFY modeChanged)      // 0=circles 1=grid
    Q_PROPERTY(int polyMain READ polyMain WRITE setPolyMain NOTIFY polyChanged)
    Q_PROPERTY(int polySecondary READ polySecondary WRITE setPolySecondary NOTIFY polyChanged)
    Q_PROPERTY(int gridHighlight READ gridHighlight WRITE setGridHighlight NOTIFY gridHighlightChanged)

public:
    explicit BeatIndicatorItem(QQuickItem* parent = nullptr);

    void paint(QPainter* p) override;

    int beats() const { return m_beats; }
    int subdivisions() const { return m_subdivisions; }
    int currentBeat() const { return m_currentBeat; }
    int currentSub() const { return m_currentSub; }
    QColor accentColor() const { return m_accentColor; }
    QColor backgroundColor() const { return m_backgroundColor; }
    QColor circleBorderColor() const { return m_circleBorderColor; }
    qreal circleBorderWidth() const { return m_circleBorderWidth; }
    int mode() const { return m_mode; }
    int polyMain() const { return m_polyMain; }
    int polySecondary() const { return m_polySub; }
    int gridHighlight() const { return m_gridHighlight; }

    void setBeats(int v);
    void setSubdivisions(int v);
    void setCurrentBeat(int v);
    void setCurrentSub(int v);
    void setAccentColor(const QColor& v);
    void setBackgroundColor(const QColor& v);
    void setCircleBorderColor(const QColor& v);
    void setCircleBorderWidth(qreal v);
    void setMode(int v);
    void setPolyMain(int v);
    void setPolySecondary(int v);
    void setGridHighlight(int v);

signals:
    void beatsChanged();
    void subdivisionsChanged();
    void currentChanged();
    void accentColorChanged();
    void backgroundColorChanged();
    void circleBorderColorChanged();
    void circleBorderWidthChanged();
    void modeChanged();
    void polyChanged();
    void gridHighlightChanged();

private:
    int m_beats = 4;
    int m_subdivisions = 1;
    int m_currentBeat = 0;
    int m_currentSub = 0;
    QColor m_accentColor = QColor(150, 0, 0);
    QColor m_backgroundColor = QColor("#353535");
    QColor m_circleBorderColor = QColor("#1e1e1e");
    qreal  m_circleBorderWidth = 2.0;
    int m_mode = 0;
    int m_polyMain = 3;
    int m_polySub = 2;
    int m_gridHighlight = -1;

    static int lcmHelper(int a, int b);
};
