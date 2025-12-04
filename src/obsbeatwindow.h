#pragma once

#include <QMainWindow>

class OBSBeatWidget;

class OBSBeatWindow : public QMainWindow {
    Q_OBJECT
public:
    // Reparents obsWidget into this window. The window does NOT take ownership
    // of the widget pointer lifetime — MainWindow still owns the widget.
    explicit OBSBeatWindow(OBSBeatWidget* obsWidget, QWidget* parent = nullptr);
    ~OBSBeatWindow() override;

signals:
    // Emitted when the window is about to close (before QWidget children are reparented back).
    void windowAboutToClose();

protected:
    void closeEvent(QCloseEvent* ev) override;

private:
    OBSBeatWidget* m_obsWidget = nullptr;
    QWidget* m_container = nullptr;
};