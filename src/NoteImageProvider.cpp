#include "NoteImageProvider.h"
#include "MetronomeController.h"

QPixmap NoteImageProvider::requestPixmap(const QString& id, QSize* size,
                                         const QSize& requestedSize)
{
    // Render at exactly the display size QML requests.
    // No inflation — rendering larger and scaling down is what causes blur.
    int dim = 48;
    if (requestedSize.isValid() && requestedSize.width() > 0 && requestedSize.height() > 0) {
        dim = qMax(requestedSize.width(), requestedSize.height());
    }
    dim = qMax(dim, 24); // safety floor
    QSize targetSize(dim, dim);

    QPixmap px;
    if (id.startsWith("picker_")) {
        // "picker_<cat>_<idx>_<rev>"
        QStringList parts = id.split('_');
        if (parts.size() >= 3) {
            bool ok1 = false, ok2 = false;
            int cat = parts[1].toInt(&ok1);
            int idx = parts[2].toInt(&ok2);
            if (ok1 && ok2)
                px = m_controller->pickerPatternPixmap(cat, idx, targetSize);
        }
    } else if (id.startsWith("section_")) {
        // "section_<idx>_<revision>"
        // Render at natural base size (48×48), then CPU-scale to the requested
        // display area with KeepAspectRatio.  This matches the old Widget app's
        // algorithm and avoids a second round of GPU interpolation in QML.
        QStringList parts = id.split('_');
        if (parts.size() >= 2) {
            bool ok = false;
            int idx = parts[1].toInt(&ok);
            if (ok) {
                px = m_controller->sectionSubdivisionPixmap(idx, QSize(48, 48));
                if (!px.isNull() && requestedSize.isValid()
                        && requestedSize.width() > 0 && requestedSize.height() > 0) {
                    px = px.scaled(requestedSize, Qt::KeepAspectRatio,
                                   Qt::SmoothTransformation);
                }
            }
        }
    } else if (id.startsWith("edittile_")) {
        // "edittile_<tileIdx>"
        QStringList parts = id.split('_');
        if (parts.size() >= 2) {
            bool ok = false;
            int tileIdx = parts[1].toInt(&ok);
            if (ok)
                px = m_controller->patternEditor()->tilePixmap(tileIdx, targetSize);
        }
    } else if (id.startsWith("editpulse_")) {
        // "editpulse_<pulseIdx>_<rev>"
        QStringList parts = id.split('_');
        if (parts.size() >= 2) {
            bool ok = false;
            int pulseIdx = parts[1].toInt(&ok);
            if (ok)
                px = m_controller->patternEditor()->pulsePixmap(pulseIdx, targetSize);
        }
    } else if (id.startsWith("editpreview_")) {
        // "editpreview_<rev>" — render at natural base size
        px = m_controller->patternEditor()->previewPixmap(QSize(48, 48));
    } else if (id.startsWith("staged_")) {
        // "staged_<rev>" — render the staged pattern for rest editor preview
        px = m_controller->stagedPatternPixmap(QSize(48, 48));
    } else {
        // "current_<revision>" or fallback — render at natural base size
        px = m_controller->currentSubdivisionPixmap(QSize(48, 48));
    }

    // For current_ and editpreview_: same treatment as section_ —
    // render at natural base size, CPU-scale to the requested display area.
    if (id.startsWith("current_") || id.startsWith("editpreview_") || id.startsWith("staged_")) {
        if (!px.isNull() && requestedSize.isValid()
                && requestedSize.width() > 0 && requestedSize.height() > 0) {
            px = px.scaled(requestedSize, Qt::KeepAspectRatio,
                           Qt::SmoothTransformation);
        }
    }

    if (px.isNull())
        px = QPixmap(targetSize);

    if (size) *size = px.size();
    return px;
}
