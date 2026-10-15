#include "HookListModel.h"

HookListModel::HookListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int HookListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_entries.size();
}

QVariant HookListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
        return {};
    const HookEntry &e = m_entries.at(index.row());
    switch (role) {
    case NameRole:
        return e.name;
    case ExecutableRole:
        return e.executable;
    case SampleRole:
        return e.sample;
    default:
        return {};
    }
}

QHash<int, QByteArray> HookListModel::roleNames() const
{
    return {
        {NameRole, "name"},
        {ExecutableRole, "executable"},
        {SampleRole, "sample"},
    };
}

int HookListModel::activeCount() const
{
    int n = 0;
    for (const HookEntry &e : m_entries) {
        if (e.executable && !e.sample)
            ++n;
    }
    return n;
}

void HookListModel::setEntries(QVector<HookEntry> entries)
{
    beginResetModel();
    m_entries = std::move(entries);
    endResetModel();
    emit countChanged();
}

void HookListModel::clear()
{
    if (m_entries.isEmpty())
        return;
    beginResetModel();
    m_entries.clear();
    endResetModel();
    emit countChanged();
}
