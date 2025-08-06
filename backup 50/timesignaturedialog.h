#pragma once

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QGridLayout>

class TimeSignatureDialog : public QDialog {
    Q_OBJECT
public:
    explicit TimeSignatureDialog(int numerator, int denominator, QWidget* parent = nullptr);

    int selectedNumerator() const { return m_numerator; }
    int selectedDenominator() const { return m_denominator; }

private:
    int m_numerator;
    int m_denominator;
    QLabel* numeratorLabel;
    QLabel* denominatorLabel;

    void updateLabels();

private slots:
    void onNumeratorPlus();
    void onNumeratorMinus();
    void onDenominatorPlus();
    void onDenominatorMinus();
    void onPresetSelected(int num, int den);
};