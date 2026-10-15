#include "CommitLogModel.h"

CommitLogModel::CommitLogModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int CommitLogModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_commits.size();
}

QVariant CommitLogModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_commits.size())
        return {};
    const CommitEntry &e = m_commits.at(index.row());
    switch (role) {
    case HashRole:
        return e.hash;
    case ShortHashRole:
        return e.shortHash;
    case SubjectRole:
        return e.subject;
    case AuthorRole:
        return e.author;
    case RelativeTimeRole:
        return e.relativeTime;
    default:
        return {};
    }
}

QHash<int, QByteArray> CommitLogModel::roleNames() const
{
    return {
        {HashRole, "hash"},
        {ShortHashRole, "shortHash"},
        {SubjectRole, "subject"},
        {AuthorRole, "author"},
        {RelativeTimeRole, "relativeTime"},
    };
}

int CommitLogModel::totalCount() const
{
    return m_all.size();
}

QString CommitLogModel::filter() const
{
    return m_filter;
}

void CommitLogModel::setFilter(const QString &filter)
{
    if (m_filter == filter)
        return;
    m_filter = filter;
    applyFilter();
    emit filterChanged();
}

void CommitLogModel::setCommits(QVector<CommitEntry> commits)
{
    m_all = std::move(commits);
    applyFilter();
}

void CommitLogModel::clear()
{
    if (m_all.isEmpty() && m_commits.isEmpty())
        return;
    beginResetModel();
    m_all.clear();
    m_commits.clear();
    endResetModel();
    emit countChanged();
}

void CommitLogModel::applyFilter()
{
    beginResetModel();
    if (m_filter.trimmed().isEmpty()) {
        m_commits = m_all;
    } else {
        const QString needle = m_filter.trimmed().toLower();
        m_commits.clear();
        m_commits.reserve(m_all.size());
        for (const CommitEntry &e : m_all) {
            if (e.subject.toLower().contains(needle) || e.author.toLower().contains(needle)
                || e.shortHash.toLower().contains(needle) || e.hash.toLower().contains(needle)) {
                m_commits.push_back(e);
            }
        }
    }
    endResetModel();
    emit countChanged();
}
