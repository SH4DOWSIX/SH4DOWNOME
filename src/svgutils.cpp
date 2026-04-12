#include "svgutils.h"
#include <QSvgRenderer>
#include <QPainter>

QPixmap svgToPixmap(const QString& svgPath, QSize boxSize) {
    QSvgRenderer renderer(svgPath);
    QSizeF svgSize = renderer.defaultSize();
    if (svgSize.isEmpty())
        svgSize = QSizeF(48, 48);
    double scale = std::min(
        double(boxSize.width()) / svgSize.width(),
        double(boxSize.height()) / svgSize.height()
    );
    QSizeF renderSize(svgSize.width() * scale, svgSize.height() * scale);

    QPixmap pixmap(boxSize);
    pixmap.fill(Qt::transparent);
    QPainter painter(&pixmap);

    QRectF target(
        (boxSize.width() - renderSize.width())/2,
        (boxSize.height() - renderSize.height())/2,
        renderSize.width(),
        renderSize.height()
    );
    renderer.render(&painter, target);
    return pixmap;
}