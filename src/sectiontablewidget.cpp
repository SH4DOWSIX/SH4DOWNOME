#include "sectiontablewidget.h"
#include <QDropEvent>

void SectionTableWidget::dropEvent(QDropEvent *event) {
    QList<QTableWidgetSelectionRange> ranges = selectedRanges();
    int from = -1;
    if (!ranges.isEmpty())
        from = ranges.first().topRow();

#if QT_VERSION >= QT_VERSION_CHECK(6, 0, 0)
    QModelIndex idx = indexAt(event->position().toPoint());
#else
    QModelIndex idx = indexAt(event->pos());
#endif
    int to = idx.row();
    if (to == -1) to = rowCount() - 1;
    if (from != -1 && to > from) to--;

    if (from != -1 && to != -1 && from != to)
        emit rowMoved(from, to);
}