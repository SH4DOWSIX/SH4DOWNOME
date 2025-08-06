#pragma once
#include <QTableWidget>

class SectionTableWidget : public QTableWidget {
    Q_OBJECT
public:
    using QTableWidget::QTableWidget;
signals:
    void rowMoved(int from, int to);
protected:
    void dropEvent(QDropEvent *event) override;
};