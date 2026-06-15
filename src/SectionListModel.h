#pragma once

#include <QAbstractListModel>
#include "presetmanager.h"

class SectionListModel : public QAbstractListModel {
    Q_OBJECT
public:
    enum Roles {
        LabelRole = Qt::UserRole + 1,
        TimeSigRole,
        SubPolyRole,
        TempoRole,
        IsSelectedRole,
        IsPolyrhythmRole,
        RowNumberRole,
    };

    explicit SectionListModel(QObject* parent = nullptr);

    // Update entire model from a vector of sections + selected index
    void resetSections(const std::vector<MetronomeSection>& sections, int selectedIdx);
    // Update single row without full reset
    void updateRow(int row, const MetronomeSection& s, int selectedIdx);
    // Update selection highlight only
    void setSelectedIndex(int idx);

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    QVector<MetronomeSection> m_sections;
    int m_selectedIdx = -1;
};
