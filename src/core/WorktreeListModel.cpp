#include "WorktreeListModel.h"

#include <QDir>
#include <QFileInfo>

WorktreeListModel::WorktreeListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int WorktreeListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_entries.size();
}

QVariant WorktreeListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_entries.size())
        return {};
    const WorktreeEntry &e = m_entries.at(index.row());
    switch (role) {
    case PathRole:
        return e.path;
    case HeadRole:
        return e.head;
    case BranchRole:
        return e.branch;
    case BareRole:
        return e.bare;
    case LockedRole:
        return e.locked;
    case PrunableRole:
        return e.prunable;
    case IsCurrentRole: {
        const QString a = QFileInfo(e.path).canonicalFilePath();
        const QString b = QFileInfo(m_currentRoot).canonicalFilePath();
        return !a.isEmpty() && a == b;
    }
    default:
        return {};
    }
}

QHash<int, QByteArray> WorktreeListModel::roleNames() const
{
    return {
        {PathRole, "path"},
        {HeadRole, "head"},
        {BranchRole, "branch"},
        {BareRole, "bare"},
        {LockedRole, "locked"},
        {PrunableRole, "prunable"},
        {IsCurrentRole, "isCurrent"},
    };
}

void WorktreeListModel::setEntries(QVector<WorktreeEntry> entries, const QString &currentRoot)
{
    beginResetModel();
    m_entries = std::move(entries);
    m_currentRoot = currentRoot;
    endResetModel();
    emit countChanged();
}

void WorktreeListModel::clear()
{
    if (m_entries.isEmpty())
        return;
    beginResetModel();
    m_entries.clear();
    m_currentRoot.clear();
    endResetModel();
    emit countChanged();
}
