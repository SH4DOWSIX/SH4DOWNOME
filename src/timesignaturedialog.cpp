#include "timesignaturedialog.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFont>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>

TimeSignatureDialog::TimeSignatureDialog(int numerator, int denominator, QWidget* parent)
    : QDialog(parent), m_numerator(numerator), m_denominator(denominator)
{
    setWindowTitle("Select Time Signature");
    setModal(true);
    setMinimumWidth(320);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(20);

    // Title label (like Poly dialog)
    QLabel* titleLabel = new QLabel("Time Signature", this);
    QFont titleFont;
    titleFont.setPointSize(16);
    titleFont.setBold(true);
    titleLabel->setFont(titleFont);
    titleLabel->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(titleLabel);

    // Numerator ("Beats per Bar")
    QLabel* labelBeatsPerBar = new QLabel("Beats per Bar", this);
    QFont labelFont;
    labelFont.setPointSize(10);
    labelFont.setBold(true);
    labelBeatsPerBar->setFont(labelFont);
    labelBeatsPerBar->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(labelBeatsPerBar);

    QHBoxLayout* numRow = new QHBoxLayout;
    QPushButton* numMinus = new QPushButton("-", this);
    numeratorLabel = new QLabel(QString::number(m_numerator), this);
    numeratorLabel->setAlignment(Qt::AlignCenter);
    numeratorLabel->setStyleSheet("font-size: 32pt; color: white;");
    QPushButton* numPlus = new QPushButton("+", this);
    numRow->addWidget(numMinus);
    numRow->addWidget(numeratorLabel);
    numRow->addWidget(numPlus);
    mainLayout->addLayout(numRow);

    // Denominator ("Note Value")
    QLabel* labelNoteValue = new QLabel("Note Value", this);
    labelNoteValue->setFont(labelFont);
    labelNoteValue->setAlignment(Qt::AlignCenter);
    mainLayout->addWidget(labelNoteValue);

    QHBoxLayout* denRow = new QHBoxLayout;
    QPushButton* denMinus = new QPushButton("-", this);
    denominatorLabel = new QLabel(QString::number(m_denominator), this);
    denominatorLabel->setAlignment(Qt::AlignCenter);
    denominatorLabel->setStyleSheet("font-size: 32pt; color: white;");
    QPushButton* denPlus = new QPushButton("+", this);
    denRow->addWidget(denMinus);
    denRow->addWidget(denominatorLabel);
    denRow->addWidget(denPlus);
    mainLayout->addLayout(denRow);

    // Preset options (as before)
    QGridLayout* presetLayout = new QGridLayout;
    struct Preset { int num, den; };
    std::vector<Preset> presets = { {2,4},{3,4},{4,4},{5,4},{3,8},{6,8} };
    int col = 0;
    for (const auto& preset : presets) {
        QPushButton* btn = new QPushButton(QString("%1/%2").arg(preset.num).arg(preset.den), this);
        btn->setStyleSheet("font-size: 12pt; padding: 8px;");
        connect(btn, &QPushButton::clicked, this, [this, preset]() { onPresetSelected(preset.num, preset.den); });
        presetLayout->addWidget(btn, 0, col++);
    }
    mainLayout->addLayout(presetLayout);

    QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, this);
    buttonBox->button(QDialogButtonBox::Ok)->setObjectName("buttonOK");
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    mainLayout->addWidget(buttonBox);

    // Connectors
    connect(numPlus, &QPushButton::clicked, this, &TimeSignatureDialog::onNumeratorPlus);
    connect(numMinus, &QPushButton::clicked, this, &TimeSignatureDialog::onNumeratorMinus);
    connect(denPlus, &QPushButton::clicked, this, &TimeSignatureDialog::onDenominatorPlus);
    connect(denMinus, &QPushButton::clicked, this, &TimeSignatureDialog::onDenominatorMinus);
    updateLabels();
}

void TimeSignatureDialog::updateLabels() {
    numeratorLabel->setText(QString::number(m_numerator));
    denominatorLabel->setText(QString::number(m_denominator));
}

void TimeSignatureDialog::onNumeratorPlus() {
    if (m_numerator < 32) m_numerator++;
    updateLabels();
}
void TimeSignatureDialog::onNumeratorMinus() {
    if (m_numerator > 1) m_numerator--;
    updateLabels();
}
void TimeSignatureDialog::onDenominatorPlus() {
    if (m_denominator < 16) m_denominator++;
    updateLabels();
}
void TimeSignatureDialog::onDenominatorMinus() {
    if (m_denominator > 1) m_denominator--;
    updateLabels();
}
void TimeSignatureDialog::onPresetSelected(int num, int den) {
    m_numerator = num;
    m_denominator = den;
    updateLabels();
}