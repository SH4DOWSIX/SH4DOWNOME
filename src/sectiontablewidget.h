#pragma once
#include <QTableWidget>
#include <QKeyEvent>

class SectionTableWidget : public QTableWidget {
    Q_OBJECT
public:
    using QTableWidget::QTableWidget;
signals:
    void moveSectionRequested(int fromRow, int toRow);
protected:
    void keyPressEvent(QKeyEvent* event) override;
};