#include "RemoteListModel.h"

RemoteListModel::RemoteListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int RemoteListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_entries.size();
}

QVariant RemoteListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
        return {};
    const RemoteEntry &e = m_entries.at(index.row());
    switch (role) {
    case NameRole:
        return e.name;
    case FetchUrlRole:
        return e.fetchUrl;
    case PushUrlRole:
        return e.pushUrl;
    default:
        return {};
    }
}

QHash<int, QByteArray> RemoteListModel::roleNames() const
{
    return {
        {NameRole, "name"},
        {FetchUrlRole, "fetchUrl"},
        {PushUrlRole, "pushUrl"},
    };
}

void RemoteListModel::setEntries(QVector<RemoteEntry> entries)
{
    beginResetModel();
    m_entries = std::move(entries);
    endResetModel();
    emit countChanged();
}

void RemoteListModel::clear()
{
    if (m_entries.isEmpty())
        return;
    beginResetModel();
    m_entries.clear();
    endResetModel();
    emit countChanged();
}

QString RemoteListModel::nameAt(int row) const
{
    if (row < 0 || row >= m_entries.size())
        return {};
    return m_entries.at(row).name;
}
