#include "obsbeatwindow.h"
#include "obsbeatwidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QCloseEvent>
#include <QResizeEvent>
#include <QSettings>
#include <QCoreApplication>

OBSBeatWindow::OBSBeatWindow(OBSBeatWidget* obsWidget, QWidget* parent)
    : QMainWindow(parent), m_obsWidget(obsWidget)
{
    setWindowTitle("OBS Beat");
    setAttribute(Qt::WA_DeleteOnClose, false);
    setWindowFlags(windowFlags() | Qt::Window);

    m_container = new QWidget(this);
    auto *lay = new QVBoxLayout(m_container);
    lay->setContentsMargins(4, 4, 4, 4);
    lay->setSpacing(0);

    if (m_obsWidget) {
        m_obsWidget->setParent(m_container);
        m_obsWidget->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
        lay->addWidget(m_obsWidget);
    }

    // Bottom bar with lock size checkbox
    QWidget* bottomBar = new QWidget(m_container);
    bottomBar->setFixedHeight(24);
    QHBoxLayout* barLayout = new QHBoxLayout(bottomBar);
    barLayout->setContentsMargins(4, 0, 4, 0);
    barLayout->addStretch();
    m_lockSizeCheck = new QCheckBox("Lock size", bottomBar);
    m_lockSizeCheck->setFocusPolicy(Qt::NoFocus);
    barLayout->addWidget(m_lockSizeCheck);
    lay->addWidget(bottomBar);

    setCentralWidget(m_container);

    // Restore saved size and lock state
    QString iniPath = QCoreApplication::applicationDirPath() + "/data/settings.ini";
    QSettings settings(iniPath, QSettings::IniFormat);
    int w = settings.value("obsWindowWidth",  600).toInt();
    int h = settings.value("obsWindowHeight", 320).toInt();
    resize(w, h);

    bool locked = settings.value("obsSizeLocked", false).toBool();
    m_lockSizeCheck->setChecked(locked);
    if (locked)
        setFixedSize(w, h);

    connect(m_lockSizeCheck, &QCheckBox::toggled, this, &OBSBeatWindow::onLockToggled);
}

OBSBeatWindow::~OBSBeatWindow() {}

void OBSBeatWindow::onLockToggled(bool checked)
{
    QString iniPath = QCoreApplication::applicationDirPath() + "/data/settings.ini";
    QSettings settings(iniPath, QSettings::IniFormat);
    settings.setValue("obsSizeLocked", checked);
    if (checked) {
        setFixedSize(size());
        settings.setValue("obsWindowWidth",  width());
        settings.setValue("obsWindowHeight", height());
    } else {
        setMinimumSize(0, 0);
        setMaximumSize(QWIDGETSIZE_MAX, QWIDGETSIZE_MAX);
    }
    settings.sync();
}

void OBSBeatWindow::resizeEvent(QResizeEvent* ev)
{
    QMainWindow::resizeEvent(ev);
    if (m_lockSizeCheck && !m_lockSizeCheck->isChecked()) {
        QString iniPath = QCoreApplication::applicationDirPath() + "/data/settings.ini";
        QSettings settings(iniPath, QSettings::IniFormat);
        settings.setValue("obsWindowWidth",  width());
        settings.setValue("obsWindowHeight", height());
    }
}

void OBSBeatWindow::closeEvent(QCloseEvent* ev)
{
    emit windowAboutToClose();
    QMainWindow::closeEvent(ev);
}