#include "TagListModel.h"

TagListModel::TagListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int TagListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_entries.size();
}

QVariant TagListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
        return {};
    const TagEntry &e = m_entries.at(index.row());
    switch (role) {
    case NameRole:
        return e.name;
    case HashRole:
        return e.hash;
    case ShortHashRole:
        return e.shortHash;
    case RelativeTimeRole:
        return e.relativeTime;
    case SubjectRole:
        return e.subject;
    default:
        return {};
    }
}

QHash<int, QByteArray> TagListModel::roleNames() const
{
    return {
        {NameRole, "name"},
        {HashRole, "hash"},
        {ShortHashRole, "shortHash"},
        {RelativeTimeRole, "relativeTime"},
        {SubjectRole, "subject"},
    };
}

void TagListModel::setEntries(QVector<TagEntry> entries)
{
    beginResetModel();
    m_entries = std::move(entries);
    endResetModel();
    emit countChanged();
}

void TagListModel::clear()
{
    if (m_entries.isEmpty())
        return;
    beginResetModel();
    m_entries.clear();
    endResetModel();
    emit countChanged();
}
