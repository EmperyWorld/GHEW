#include "RecentReposModel.h"

#include "EphemeralRepoPath.h"

#include <QDir>
#include <QFileInfo>
#include <QSettings>

RecentReposModel::RecentReposModel(QObject *parent)
    : QAbstractListModel(parent)
{
    load();
}

int RecentReposModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_paths.size();
}

QVariant RecentReposModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_paths.size())
        return {};
    const QString &path = m_paths.at(index.row());
    switch (role) {
    case PathRole:
        return path;
    case NameRole:
        return QFileInfo(path).fileName();
    case ParentRole: {
        const QString parent = QFileInfo(path).absolutePath();
        const QString home = QDir::homePath();
        if (!home.isEmpty() && parent.startsWith(home))
            return QStringLiteral("~") + parent.mid(home.size());
        return parent;
    }
    default:
        return {};
    }
}

QHash<int, QByteArray> RecentReposModel::roleNames() const
{
    return {
        {PathRole, "path"},
        {NameRole, "name"},
        {ParentRole, "parentDir"},
    };
}

void RecentReposModel::load()
{
    QSettings settings;
    const QStringList stored = settings.value(QStringLiteral("recentRepos")).toStringList();
    beginResetModel();
    m_paths.clear();
    bool pruned = false;
    for (const QString &p : stored) {
        const QString cleaned = QDir::cleanPath(p);
        if (isEphemeralRepoPath(cleaned)) {
            pruned = true;
            continue;
        }
        if (QDir(cleaned).exists() && !m_paths.contains(cleaned))
            m_paths.append(cleaned);
        if (m_paths.size() >= kMaxRecent)
            break;
    }
    endResetModel();
    emit countChanged();
    if (pruned || m_paths.size() != stored.size())
        save();
}

void RecentReposModel::save() const
{
    QSettings settings;
    settings.setValue(QStringLiteral("recentRepos"), m_paths);
}

void RecentReposModel::touch(const QString &path)
{
    const QString cleaned = QDir::cleanPath(path);
    if (cleaned.isEmpty() || !QDir(cleaned).exists())
        return;
    if (isEphemeralRepoPath(cleaned))
        return;
    beginResetModel();
    m_paths.removeAll(cleaned);
    m_paths.prepend(cleaned);
    while (m_paths.size() > kMaxRecent)
        m_paths.removeLast();
    endResetModel();
    emit countChanged();
    save();
}

void RecentReposModel::remove(const QString &path)
{
    const QString cleaned = QDir::cleanPath(path);
    const int idx = m_paths.indexOf(cleaned);
    if (idx < 0)
        return;
    beginRemoveRows(QModelIndex(), idx, idx);
    m_paths.removeAt(idx);
    endRemoveRows();
    emit countChanged();
    save();
}

void RecentReposModel::clear()
{
    if (m_paths.isEmpty())
        return;
    beginResetModel();
    m_paths.clear();
    endResetModel();
    emit countChanged();
    save();
}
