#include "SectionListModel.h"

SectionListModel::SectionListModel(QObject* parent)
    : QAbstractListModel(parent)
{}

void SectionListModel::resetSections(const std::vector<MetronomeSection>& sections, int selectedIdx)
{
    beginResetModel();
    m_sections.clear();
    for (const auto& s : sections)
        m_sections.push_back(s);
    m_selectedIdx = selectedIdx;
    endResetModel();
}

void SectionListModel::updateRow(int row, const MetronomeSection& s, int selectedIdx)
{
    if (row < 0 || row >= m_sections.size())
        return;
    m_sections[row] = s;
    m_selectedIdx = selectedIdx;
    QModelIndex idx = index(row, 0);
    emit dataChanged(idx, idx);
}

void SectionListModel::setSelectedIndex(int idx)
{
    int old = m_selectedIdx;
    m_selectedIdx = idx;
    // Notify both old and new row for IsSelectedRole
    if (old >= 0 && old < m_sections.size()) {
        QModelIndex midx = index(old, 0);
        emit dataChanged(midx, midx, {IsSelectedRole});
    }
    if (idx >= 0 && idx < m_sections.size()) {
        QModelIndex midx = index(idx, 0);
        emit dataChanged(midx, midx, {IsSelectedRole});
    }
}

int SectionListModel::rowCount(const QModelIndex& parent) const
{
    if (parent.isValid()) return 0;
    return m_sections.size();
}

QVariant SectionListModel::data(const QModelIndex& index, int role) const
{
    if (!index.isValid() || index.row() >= m_sections.size())
        return QVariant();

    const MetronomeSection& s = m_sections[index.row()];

    switch (role) {
    case LabelRole:
        return s.label;
    case TimeSigRole:
        return QString("%1/%2").arg(s.numerator).arg(s.denominator);
    case SubPolyRole:
        if (s.hasPolyrhythm)
            return QString("%1/%2").arg(s.polyrhythm.primaryBeats).arg(s.polyrhythm.secondaryBeats);
        else
            return s.subdivisionPattern.name;
    case TempoRole:
        return s.tempo;
    case IsSelectedRole:
        return (index.row() == m_selectedIdx);
    case IsPolyrhythmRole:
        return s.hasPolyrhythm;
    case RowNumberRole:
        return index.row() + 1;
    default:
        break;
    }
    return QVariant();
}

QHash<int, QByteArray> SectionListModel::roleNames() const
{
    QHash<int, QByteArray> h;
    h[LabelRole]       = "sectionLabel";
    h[TimeSigRole]     = "timeSig";
    h[SubPolyRole]     = "subPoly";
    h[TempoRole]       = "sectionTempo";
    h[IsSelectedRole]  = "isSelected";
    h[IsPolyrhythmRole]= "isPoly";
    h[RowNumberRole]   = "rowNumber";
    return h;
}
