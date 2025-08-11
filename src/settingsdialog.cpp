#include "settingsdialog.h"
#include <QVBoxLayout>
#include <QLabel>
#include <QDialogButtonBox>

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle("Settings");
    QVBoxLayout* layout = new QVBoxLayout(this);

    layout->addWidget(new QLabel("Metronome Sound Set:"));
    soundCombo = new QComboBox(this);
    soundCombo->addItems({
        "Default",
        "Bongo",
        "Cowbell",
        "Digital",
        "Drum",
        "Hihat",
        "Metal",
        "Woodblock",
        "Wooden"
    });
    layout->addWidget(soundCombo);

    layout->addWidget(new QLabel("Accent Colour:"));
    colorButton = new QPushButton(this);
    colorButton->setText("Pick Colour");
    accentColor = QColor(150, 0, 0);
    connect(colorButton, &QPushButton::clicked, this, &SettingsDialog::onPickColor);
    layout->addWidget(colorButton);

    // Always On Top checkbox (NEW, placed above Hide OBS Widget)
    alwaysOnTopCheck = new QCheckBox("Always on top", this);
    layout->addWidget(alwaysOnTopCheck);

    hideObsCheck = new QCheckBox("Hide OBS Widget", this);
    layout->addWidget(hideObsCheck);

    QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    layout->addWidget(buttons);
}

QString SettingsDialog::selectedSoundSet() const
{
    return soundCombo->currentText();
}

QColor SettingsDialog::selectedAccentColor() const
{
    return accentColor;
}

bool SettingsDialog::obsWidgetHidden() const
{
    return hideObsCheck->isChecked();
}

bool SettingsDialog::alwaysOnTop() const
{
    return alwaysOnTopCheck->isChecked();
}

void SettingsDialog::setSelectedSoundSet(const QString& sound)
{
    int idx = soundCombo->findText(sound);
    if (idx >= 0)
        soundCombo->setCurrentIndex(idx);
}

void SettingsDialog::setSelectedAccentColor(const QColor& color)
{
    accentColor = color;
    QPalette pal = colorButton->palette();
    pal.setColor(QPalette::Button, color);
    colorButton->setPalette(pal);
    colorButton->setAutoFillBackground(true);
    colorButton->update();
}

void SettingsDialog::setObsWidgetHidden(bool hide)
{
    hideObsCheck->setChecked(hide);
}

void SettingsDialog::setAlwaysOnTop(bool onTop)
{
    alwaysOnTopCheck->setChecked(onTop);
}

void SettingsDialog::onPickColor()
{
    QColor chosen = QColorDialog::getColor(accentColor, this, "Pick Accent Colour");
    if (chosen.isValid()) {
        setSelectedAccentColor(chosen);
    }
}