#include "BranchListModel.h"

BranchListModel::BranchListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int BranchListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_branches.size();
}

QVariant BranchListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_branches.size())
        return {};
    const BranchEntry &e = m_branches.at(index.row());
    switch (role) {
    case NameRole:
        return e.name;
    case CurrentRole:
        return e.current;
    case RemoteRole:
        return e.remote;
    case UpstreamRole:
        return e.upstream;
    case AheadRole:
        return e.ahead;
    case BehindRole:
        return e.behind;
    default:
        return {};
    }
}

QHash<int, QByteArray> BranchListModel::roleNames() const
{
    return {
        {NameRole, "name"},
        {CurrentRole, "current"},
        {RemoteRole, "remote"},
        {UpstreamRole, "upstream"},
        {AheadRole, "ahead"},
        {BehindRole, "behind"},
    };
}

void BranchListModel::setBranches(QVector<BranchEntry> branches)
{
    beginResetModel();
    m_branches = std::move(branches);
    endResetModel();
    emit countChanged();
}

void BranchListModel::clear()
{
    if (m_branches.isEmpty())
        return;
    beginResetModel();
    m_branches.clear();
    endResetModel();
    emit countChanged();
}
