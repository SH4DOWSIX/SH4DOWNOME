#include "svgutils.h"
#include <QSvgRenderer>
#include <QPainter>
#include <QFile>
#include <QHash>
#include <QPair>
#include <QRegularExpression>

// Cache: path -> (viewBox, innerContent with white fill)
static QHash<QString, QPair<QString,QString>> s_svgContentCache;

static void parseSvgFile(const QString& path) {
    if (s_svgContentCache.contains(path)) return;

    QFile file(path);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        s_svgContentCache[path] = {"0 0 1 1", ""};
        return;
    }
    QString text = QString::fromUtf8(file.readAll());
    file.close();

    // Extract viewBox attribute value
    QString viewBox;
    int vbPos = text.indexOf("viewBox=\"");
    if (vbPos >= 0) {
        int start = vbPos + 9;
        int end = text.indexOf('"', start);
        if (end > start) viewBox = text.mid(start, end - start);
    }

    // Extract inner content: between first '>' after '<svg' and last '</svg>'
    QString inner;
    int svgOpen = text.indexOf("<svg");
    int svgClose = (svgOpen >= 0) ? text.indexOf('>', svgOpen) : -1;
    int svgEnd   = text.lastIndexOf("</svg>");
    if (svgClose >= 0 && svgEnd > svgClose)
        inner = text.mid(svgClose + 1, svgEnd - svgClose - 1);

    // Strip <defs> blocks (self-closing or paired)
    static const QRegularExpression defsRe(
        R"(<defs(?:\s[^>]*)?\s*/>|<defs(?:\s[^>]*)?>.*?</defs>)",
        QRegularExpression::DotMatchesEverythingOption);
    inner.remove(defsRe);

    // Strip sodipodi:namedview blocks (Inkscape page metadata)
    static const QRegularExpression namedViewRe(
        R"(<sodipodi:namedview(?:\s[^>]*)?>.*?</sodipodi:namedview>)",
        QRegularExpression::DotMatchesEverythingOption);
    inner.remove(namedViewRe);

    // Strip <style> blocks
    static const QRegularExpression styleRe(
        R"(<style(?:\s[^>]*)?>.*?</style>)",
        QRegularExpression::DotMatchesEverythingOption);
    inner.remove(styleRe);

    // Strip display:none groups (LilyPond attribution watermark at huge coordinates)
    static const QRegularExpression displayNoneRe(
        R"(<g\b[^>]*style\s*=\s*"[^"]*display\s*:\s*none[^"]*"[^>]*>.*?</g>)",
        QRegularExpression::DotMatchesEverythingOption);
    inner.remove(displayNoneRe);

    // Strip ALL id= attributes — when multiple glyphs of the same type are
    // embedded in one composite SVG, duplicate IDs cause Qt's renderer to
    // silently skip elements (its internal map keeps only the last entry per ID).
    static const QRegularExpression idAttrRe(R"(\s+id="[^"]*")");
    inner.remove(idAttrRe);

    // Whiten all currentColor fills/strokes
    inner.replace("fill=\"currentColor\"", "fill=\"white\"");
    inner.replace("stroke=\"currentColor\"", "stroke=\"white\"");

    s_svgContentCache[path] = {viewBox, inner};
}

QString svgViewBox(const QString& path) {
    parseSvgFile(path);
    return s_svgContentCache.value(path).first;
}

QString svgInnerContent(const QString& path) {
    parseSvgFile(path);
    return s_svgContentCache.value(path).second;
}

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
    painter.setRenderHints(QPainter::Antialiasing | QPainter::SmoothPixmapTransform | QPainter::TextAntialiasing);

    QRectF target(
        (boxSize.width() - renderSize.width())/2,
        (boxSize.height() - renderSize.height())/2,
        renderSize.width(),
        renderSize.height()
    );
    renderer.render(&painter, target);
    return pixmap;
}