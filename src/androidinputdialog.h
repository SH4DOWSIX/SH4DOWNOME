#pragma once
#include <QObject>
#include <QString>
#include <QColor>

class AndroidInputDialog : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QColor accentColor READ accentColor WRITE setAccentColor)
public:
    explicit AndroidInputDialog(QObject *parent = nullptr);

    QColor accentColor() const { return m_accentColor; }
    void setAccentColor(const QColor &c) { m_accentColor = c; }

    Q_INVOKABLE void showText(const QString &title, const QString &defaultText);
    Q_INVOKABLE void showNumber(const QString &title, int value);
    Q_INVOKABLE void showBulkAdd(int fromTempo);
    Q_INVOKABLE void showImeForFocused();

    void emitAccepted(const QString &text);
    void emitCancelled();

signals:
    void accepted(const QString &text);
    void cancelled();

private:
    QColor m_accentColor = QColor("#cc0000");
};
