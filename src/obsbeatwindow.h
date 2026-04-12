#pragma once

#include <QMainWindow>
#include <QCheckBox>

class OBSBeatWidget;

class OBSBeatWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit OBSBeatWindow(OBSBeatWidget* obsWidget, QWidget* parent = nullptr);
    ~OBSBeatWindow() override;

signals:
    void windowAboutToClose();

protected:
    void closeEvent(QCloseEvent* ev) override;
    void resizeEvent(QResizeEvent* ev) override;

private slots:
    void onLockToggled(bool checked);

private:
    OBSBeatWidget* m_obsWidget = nullptr;
    QWidget* m_container = nullptr;
    QCheckBox* m_lockSizeCheck = nullptr;
};