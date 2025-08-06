#pragma once

#include <QDialog>
#include <QComboBox>
#include <QPushButton>
#include <QColor>
#include <QColorDialog>
#include <QCheckBox>

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget* parent = nullptr);

    QString selectedSoundSet() const;
    QColor selectedAccentColor() const;
    bool obsWidgetHidden() const;

    void setSelectedSoundSet(const QString& sound);
    void setSelectedAccentColor(const QColor& color);
    void setObsWidgetHidden(bool hide);

private slots:
    void onPickColor();

private:
    QComboBox* soundCombo;
    QPushButton* colorButton;
    QColor accentColor;
    QCheckBox* hideObsCheck;
};