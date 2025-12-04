#include "sectiontablewidget.h"
#include <QKeyEvent>

void SectionTableWidget::keyPressEvent(QKeyEvent* event) {
    if (event->modifiers() & Qt::ControlModifier) {
        int row = currentRow();

        if (event->key() == Qt::Key_Up) {
            if (row > 0) {
                emit moveSectionRequested(row, row - 1);
                event->accept();
                return;
            }
        } else if (event->key() == Qt::Key_Down) {
            if (row >= 0 && row < rowCount() - 1) {
                emit moveSectionRequested(row, row + 1);
                event->accept();
                return;
            }
        }
    }
    QTableWidget::keyPressEvent(event);
}