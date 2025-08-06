#pragma once

#include <QDialog>
#include <QLabel>
#include <QList>

class SubdivisionSelectorDialog : public QDialog {
    Q_OBJECT
public:
    explicit SubdivisionSelectorDialog(QWidget* parent = nullptr);
    int chosenIndex() const { return m_chosen; }

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private:
    int m_chosen = -1;
    QList<QLabel*> m_labels;
};