#include "HistoryModel.h"

HistoryModel::HistoryModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int HistoryModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_entries.size();
}

QVariant HistoryModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
        return {};
    const HistoryEntry &e = m_entries.at(index.row());
    switch (role) {
    case CmdlineRole:
        return e.cmdline;
    case RepoRole:
        return e.repoRoot;
    case ExitCodeRole:
        return e.exitCode;
    case OkRole:
        return e.ok;
    case TimestampRole:
        return e.timestamp;
    case CommandIdRole:
        return e.commandId;
    case ArgsJsonRole:
        return e.argsJson;
    default:
        return {};
    }
}

QHash<int, QByteArray> HistoryModel::roleNames() const
{
    return {
        {CmdlineRole, "cmdline"},
        {RepoRole, "repoRoot"},
        {ExitCodeRole, "exitCode"},
        {OkRole, "ok"},
        {TimestampRole, "timestamp"},
        {CommandIdRole, "commandId"},
        {ArgsJsonRole, "argsJson"},
    };
}

void HistoryModel::prepend(const HistoryEntry &entry)
{
    beginInsertRows(QModelIndex(), 0, 0);
    m_entries.prepend(entry);
    endInsertRows();
    emit countChanged();
}

void HistoryModel::clear()
{
    if (m_entries.isEmpty())
        return;
    beginResetModel();
    m_entries.clear();
    endResetModel();
    emit countChanged();
}

HistoryEntry HistoryModel::entryAt(int row) const
{
    if (row < 0 || row >= m_entries.size())
        return {};
    return m_entries.at(row);
}
