#include "SubmoduleListModel.h"

SubmoduleListModel::SubmoduleListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int SubmoduleListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_entries.size();
}

QVariant SubmoduleListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
        return {};
    const SubmoduleEntry &e = m_entries.at(index.row());
    switch (role) {
    case PathRole:
        return e.path;
    case ShaRole:
        return e.sha;
    case DescribeRole:
        return e.describe;
    case StatusRole:
        return e.status;
    default:
        return {};
    }
}

QHash<int, QByteArray> SubmoduleListModel::roleNames() const
{
    return {
        {PathRole, "path"},
        {ShaRole, "sha"},
        {DescribeRole, "describe"},
        {StatusRole, "status"},
    };
}

void SubmoduleListModel::setEntries(QVector<SubmoduleEntry> entries)
{
    beginResetModel();
    m_entries = std::move(entries);
    endResetModel();
    emit countChanged();
}

void SubmoduleListModel::clear()
{
    if (m_entries.isEmpty())
        return;
    beginResetModel();
    m_entries.clear();
    endResetModel();
    emit countChanged();
}
