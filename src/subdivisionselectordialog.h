#pragma once

#include <QDialog>
#include <QTabWidget>
#include <QGridLayout>
#include <QLabel>
#include <QList>
#include <QCheckBox>
#include <QPushButton>
#include <QVector>
#include <QHBoxLayout>
#include <QButtonGroup>
#include "subdivisionpattern.h"
#include "customsubdivisiondialog.h"
#include "noteassembler.h"

class SubdivisionSelectorDialog : public QDialog {
    Q_OBJECT
public:
    explicit SubdivisionSelectorDialog(QWidget* parent = nullptr, bool compoundTime = false, int numerator = 4, int denominator = 4);

    SubdivisionPattern chosenPattern() const { return m_chosenPattern; }

protected:
    bool eventFilter(QObject* obj, QEvent* event) override;

private slots:
    void onPatternClicked(int patternIdx);
    void onRestBoxChanged(int pulseIdx, bool checked);
    void onTabChanged(int tabIdx);
    void onAcceptClicked();
    void onPlusBoxClicked();

private:
    void reloadPatternGrid();
    void reloadRestCheckboxes();
    int m_numerator = 4;
    int m_denominator = 4;

    QTabWidget* m_tabWidget = nullptr;
    QWidget* m_patternGridWidget = nullptr;
    QGridLayout* m_patternGridLayout = nullptr;
    QHBoxLayout* m_restBoxLayout = nullptr;
    QPushButton* m_okButton = nullptr;
    QPushButton* m_addPatternButton = nullptr;
        QScrollArea* m_patternScrollArea;
    void updateDialogSizeForCustomPatterns();

    QVector<QLabel*> m_patternLabels;
    QVector<QCheckBox*> m_restBoxes;
    NoteAssemblerConfig configForPattern(const SubdivisionPattern& pattern) const;

    QVector<QVector<SubdivisionPattern>> m_patternsByTab;
    int m_currentTab = 0;
    int m_currentPatternIdx = -1;
    SubdivisionPattern m_chosenPattern;
    bool m_compoundTime = false;

    // Custom pattern management
    QVector<SubdivisionPattern> m_savedCustomPatterns;
    void loadCustomPatterns();
    void saveCustomPatterns();
    void addCustomPattern(const SubdivisionPattern& pattern);
    void editCustomPattern(int index);
    void deleteCustomPattern(int index);
    QString getCustomPatternsFilePath() const;
};