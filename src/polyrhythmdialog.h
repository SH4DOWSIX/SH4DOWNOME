#pragma once

#include <QDialog>
#include <QSpinBox>
#include <QCheckBox>
#include <QVBoxLayout>
#include <QDialogButtonBox>
#include <QLabel> // <-- Add this line

class PolyrhythmDialog : public QDialog {
    Q_OBJECT
public:
    explicit PolyrhythmDialog(QWidget* parent = nullptr)
        : QDialog(parent)
    {
        setWindowTitle("Polyrhythm");
        setModal(true);

        QVBoxLayout* layout = new QVBoxLayout(this);

        enableCheck = new QCheckBox("Enable Polyrhythm", this);
        layout->addWidget(enableCheck);

        QHBoxLayout* hbox = new QHBoxLayout();
        hbox->addWidget(new QLabel("Main Beats:", this));
        mainSpin = new QSpinBox(this);
        mainSpin->setRange(2, 16); mainSpin->setValue(3);
        hbox->addWidget(mainSpin);

        hbox->addWidget(new QLabel("Polyrhythm Beats:", this));
        polySpin = new QSpinBox(this);
        polySpin->setRange(2, 16); polySpin->setValue(2);
        hbox->addWidget(polySpin);
        layout->addLayout(hbox);

        QDialogButtonBox* buttons = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
        connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
        connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
        layout->addWidget(buttons);
    }

    void setPolyrhythmEnabled(bool enabled) { enableCheck->setChecked(enabled); }
    void setPolyrhythm(int mainBeats, int polyBeats) {
        mainSpin->setValue(mainBeats);
        polySpin->setValue(polyBeats);
    }
    bool polyrhythmEnabled() const { return enableCheck->isChecked(); }
    int primaryBeats() const { return mainSpin->value(); }
    int secondaryBeats() const { return polySpin->value(); }

private:
    QCheckBox* enableCheck;
    QSpinBox* mainSpin;
    QSpinBox* polySpin;
};