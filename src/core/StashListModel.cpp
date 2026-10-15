#include "StashListModel.h"

StashListModel::StashListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int StashListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_entries.size();
}

QVariant StashListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
        return {};
    const StashEntry &e = m_entries.at(index.row());
    switch (role) {
    case RefRole:
        return e.ref;
    case MessageRole:
        return e.message;
    default:
        return {};
    }
}

QHash<int, QByteArray> StashListModel::roleNames() const
{
    return {
        {RefRole, "ref"},
        {MessageRole, "message"},
    };
}

void StashListModel::setEntries(QVector<StashEntry> entries)
{
    beginResetModel();
    m_entries = std::move(entries);
    endResetModel();
    emit countChanged();
}

void StashListModel::clear()
{
    if (m_entries.isEmpty())
        return;
    beginResetModel();
    m_entries.clear();
    endResetModel();
    emit countChanged();
}
