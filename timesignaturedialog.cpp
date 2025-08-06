#include "timesignaturedialog.h"
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QFont>
#include <QPixmap>
#include <QDialogButtonBox>

TimeSignatureDialog::TimeSignatureDialog(int numerator, int denominator, QWidget* parent)
    : QDialog(parent), m_numerator(numerator), m_denominator(denominator)
{
    setWindowTitle("Select Time Signature");
    setModal(true);
    setMinimumWidth(320);

    QVBoxLayout* mainLayout = new QVBoxLayout(this);

    QLabel* clefLabel = new QLabel(this);
    clefLabel->setPixmap(QPixmap(":/resources/treble_clef.png").scaled(48, 120, Qt::KeepAspectRatio, Qt::SmoothTransformation));
    clefLabel->setAlignment(Qt::AlignLeft | Qt::AlignVCenter);

    QPushButton* numMinus = new QPushButton("-", this);
    QPushButton* numPlus = new QPushButton("+", this);
    numeratorLabel = new QLabel(QString::number(m_numerator), this);
    numeratorLabel->setAlignment(Qt::AlignCenter);
    numeratorLabel->setStyleSheet("font-size: 32pt; color: white;");

    QHBoxLayout* numRow = new QHBoxLayout;
    numRow->addWidget(numMinus);
    numRow->addWidget(numeratorLabel);
    numRow->addWidget(numPlus);

    QPushButton* denMinus = new QPushButton("-", this);
    QPushButton* denPlus = new QPushButton("+", this);
    denominatorLabel = new QLabel(QString::number(m_denominator), this);
    denominatorLabel->setAlignment(Qt::AlignCenter);
    denominatorLabel->setStyleSheet("font-size: 32pt; color: white;");

    QHBoxLayout* denRow = new QHBoxLayout;
    denRow->addWidget(denMinus);
    denRow->addWidget(denominatorLabel);
    denRow->addWidget(denPlus);

    QVBoxLayout* timeSigLayout = new QVBoxLayout;
    timeSigLayout->addLayout(numRow);
    timeSigLayout->addLayout(denRow);

    QHBoxLayout* topRow = new QHBoxLayout;
    topRow->addWidget(clefLabel);
    topRow->addLayout(timeSigLayout);
    mainLayout->addLayout(topRow);

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
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    mainLayout->addWidget(buttonBox);

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