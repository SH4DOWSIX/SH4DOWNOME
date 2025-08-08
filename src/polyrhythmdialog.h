// REPLACE the entire contents with the new dialog for polyrhythm number selection
#pragma once

#include <QDialog>
#include <QLabel>
#include <QPushButton>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QDialog>
#include <QDialogButtonBox>
#include <QLabel>
#include <QPushButton>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QVBoxLayout>

class PolyrhythmNumberDialog : public QDialog {
    Q_OBJECT
public:
    explicit PolyrhythmNumberDialog(int mainBeats, int polyBeats, QWidget* parent = nullptr)
        : QDialog(parent), m_mainBeats(mainBeats), m_polyBeats(polyBeats)
    {
        setWindowTitle("Select Polyrhythm");
        setModal(true);
        setMinimumWidth(260);

        QVBoxLayout* mainLayout = new QVBoxLayout(this);

        // Styled like time signature dialog (no clef)
        QPushButton* mainMinus = new QPushButton("-", this);
        QPushButton* mainPlus = new QPushButton("+", this);
        mainLabel = new QLabel(QString::number(m_mainBeats), this);
        mainLabel->setAlignment(Qt::AlignCenter);
        mainLabel->setStyleSheet("font-size: 32pt; color: white;");

        QHBoxLayout* mainRow = new QHBoxLayout;
        mainRow->addWidget(mainMinus);
        mainRow->addWidget(mainLabel);
        mainRow->addWidget(mainPlus);

        QPushButton* polyMinus = new QPushButton("-", this);
        QPushButton* polyPlus = new QPushButton("+", this);
        polyLabel = new QLabel(QString::number(m_polyBeats), this);
        polyLabel->setAlignment(Qt::AlignCenter);
        polyLabel->setStyleSheet("font-size: 32pt; color: white;");

        QHBoxLayout* polyRow = new QHBoxLayout;
        polyRow->addWidget(polyMinus);
        polyRow->addWidget(polyLabel);
        polyRow->addWidget(polyPlus);

        QVBoxLayout* numLayout = new QVBoxLayout;
        numLayout->addLayout(mainRow);
        numLayout->addLayout(polyRow);

        mainLayout->addLayout(numLayout);

        // Presets row, e.g. 3:2, 3:4, 5:4, 7:3
        QGridLayout* presetLayout = new QGridLayout;
        struct Preset { int main, poly; };
        std::vector<Preset> presets = { {3,2},{3,4},{4,3},{5,4},{7,3},{4,5} };
        int col = 0;
        for (const auto& p : presets) {
            QPushButton* btn = new QPushButton(QString("%1:%2").arg(p.main).arg(p.poly), this);
            btn->setStyleSheet("font-size: 12pt; padding: 8px;");
            connect(btn, &QPushButton::clicked, this, [this, p]() {
                m_mainBeats = p.main; m_polyBeats = p.poly;
                updateLabels();
            });
            presetLayout->addWidget(btn, 0, col++);
        }
        mainLayout->addLayout(presetLayout);

        QDialogButtonBox* buttonBox = new QDialogButtonBox(QDialogButtonBox::Ok, this);
        connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
        mainLayout->addWidget(buttonBox);

        connect(mainPlus, &QPushButton::clicked, this, [this](){ if (m_mainBeats < 16) ++m_mainBeats; updateLabels(); });
        connect(mainMinus, &QPushButton::clicked, this, [this](){ if (m_mainBeats > 2) --m_mainBeats; updateLabels(); });
        connect(polyPlus, &QPushButton::clicked, this, [this](){ if (m_polyBeats < 16) ++m_polyBeats; updateLabels(); });
        connect(polyMinus, &QPushButton::clicked, this, [this](){ if (m_polyBeats > 2) --m_polyBeats; updateLabels(); });
    }

    int primaryBeats() const { return m_mainBeats; }
    int secondaryBeats() const { return m_polyBeats; }

private:
    int m_mainBeats, m_polyBeats;
    QLabel* mainLabel;
    QLabel* polyLabel;
    void updateLabels() {
        mainLabel->setText(QString::number(m_mainBeats));
        polyLabel->setText(QString::number(m_polyBeats));
    }
};