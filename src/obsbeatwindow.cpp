#include "obsbeatwindow.h"
#include "obsbeatwidget.h"
#include <QVBoxLayout>
#include <QCloseEvent>

OBSBeatWindow::OBSBeatWindow(OBSBeatWidget* obsWidget, QWidget* parent)
    : QMainWindow(parent), m_obsWidget(obsWidget)
{
    setWindowTitle("OBS Beat");
    setAttribute(Qt::WA_DeleteOnClose, false); // let MainWindow manage deletion if desired
    setWindowFlags(windowFlags() | Qt::Window);

    m_container = new QWidget(this);
    auto *lay = new QVBoxLayout(m_container);
    lay->setContentsMargins(4,4,4,4);
    lay->setSpacing(0);

    if (m_obsWidget) {
        // Reparent the widget into this container
        m_obsWidget->setParent(m_container);
        m_obsWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        lay->addWidget(m_obsWidget);
    }

    setCentralWidget(m_container);
    resize(600, 320);
}

OBSBeatWindow::~OBSBeatWindow()
{
    // Do not delete m_obsWidget here; MainWindow will own it and reparent it back to a hidden host
}

void OBSBeatWindow::closeEvent(QCloseEvent* ev)
{
    emit windowAboutToClose();
    QMainWindow::closeEvent(ev);
}