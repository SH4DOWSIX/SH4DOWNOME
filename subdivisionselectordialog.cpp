#include "subdivisionselectordialog.h"
#include <QGridLayout>
#include <QSvgRenderer>
#include <QPixmap>
#include <QPainter>
#include <QEvent>
#include <QMouseEvent>

SubdivisionSelectorDialog::SubdivisionSelectorDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Choose Subdivision");
    QGridLayout* layout = new QGridLayout(this);

    QStringList images = {
        ":/resources/quarter.svg",
        ":/resources/eighth.svg",
        ":/resources/sixteenth.svg",
        ":/resources/triplet.svg",
        ":/resources/eighth_16th_16th.svg",
        ":/resources/16th_16th_eighth.svg",
        ":/resources/16th_eighth_16th.svg",
        ":/resources/dotted8th_16th.svg",
        ":/resources/16th_dotted8th.svg",
        ":/resources/eighthrest_eighth.svg",
        ":/resources/16threst_16th_16threst_16th.svg",
        ":/resources/eighthrest_eighth_eighth.svg",
        ":/resources/eighth_eighthrest_eighth.svg",
        ":/resources/eighth_eighth_eighthrest.svg",
        ":/resources/eighthrest_eighth_eighthrest.svg"
    };
    QStringList tooltips = {
        "Quarter Note",
        "Eighth Note",
        "Sixteenth Note",
        "Triplet Note",
        "Eighth + 2 Sixteenths",
        "2 Sixteenths + Eighth",
        "Sixteenth + Eighth + Sixteenth",
        "Dotted Eighth + Sixteenth",
        "Sixteenth + Dotted Eighth",
        "Eighth rest + Eighth",
        "16th rest + 16th + 16th rest + 16th",
        "Eighth rest + Eighth + Eighth",
        "Eighth + Eighth rest + Eighth",
        "Eighth + Eighth + Eighth rest",
        "Eighth rest + Eighth + Eighth rest"
    };

    const int targetHeight = 48;
    const int wrapAfter = 5;
    for (int i = 0; i < images.size(); ++i) {
        QLabel* label = new QLabel(this);
        QSvgRenderer renderer(images[i]);
        QSizeF svgSize = renderer.defaultSize();
        int targetWidth = targetHeight;
        if (svgSize.height() > 0) {
            targetWidth = int(svgSize.width() * (targetHeight / svgSize.height()));
        }
        QPixmap pix(targetWidth, targetHeight);
        pix.fill(Qt::transparent);
        QPainter painter(&pix);
        renderer.render(&painter, QRectF(0, 0, targetWidth, targetHeight));
        label->setPixmap(pix);
        label->setToolTip(tooltips[i]);
        label->setCursor(Qt::PointingHandCursor);
        label->setAlignment(Qt::AlignCenter);
        int row = i / wrapAfter;
        int col = i % wrapAfter;
        layout->addWidget(label, row, col);
        m_labels.append(label);

        label->installEventFilter(this);
    }
    setLayout(layout);
    setModal(true);
}

bool SubdivisionSelectorDialog::eventFilter(QObject* obj, QEvent* event) {
    if (event->type() == QEvent::MouseButtonRelease) {
        for (int i = 0; i < m_labels.size(); ++i) {
            if (obj == m_labels[i]) {
                m_chosen = i;
                accept();
                return true;
            }
        }
    }
    return QDialog::eventFilter(obj, event);
}