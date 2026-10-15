#include "ReflogListModel.h"

ReflogListModel::ReflogListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int ReflogListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_entries.size();
}

QVariant ReflogListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
        return {};
    const ReflogEntry &e = m_entries.at(index.row());
    switch (role) {
    case SelectorRole:
        return e.selector;
    case HashRole:
        return e.hash;
    case MessageRole:
        return e.message;
    default:
        return {};
    }
}

QHash<int, QByteArray> ReflogListModel::roleNames() const
{
    return {
        {SelectorRole, "selector"},
        {HashRole, "hash"},
        {MessageRole, "message"},
    };
}

void ReflogListModel::setEntries(QVector<ReflogEntry> entries)
{
    beginResetModel();
    m_entries = std::move(entries);
    endResetModel();
    emit countChanged();
}

void ReflogListModel::clear()
{
    if (m_entries.isEmpty())
        return;
    beginResetModel();
    m_entries.clear();
    endResetModel();
    emit countChanged();
}
