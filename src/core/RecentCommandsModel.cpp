#include "RecentCommandsModel.h"

#include <QSettings>

RecentCommandsModel::RecentCommandsModel(QObject *parent)
    : QAbstractListModel(parent)
{
    load();
}

int RecentCommandsModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_ids.size();
}

QVariant RecentCommandsModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_ids.size())
        return {};
    if (role == CommandIdRole || role == Qt::DisplayRole)
        return m_ids.at(index.row());
    return {};
}

QHash<int, QByteArray> RecentCommandsModel::roleNames() const
{
    return {{CommandIdRole, "commandId"}};
}

void RecentCommandsModel::touch(const QString &commandId)
{
    if (commandId.isEmpty())
        return;
    beginResetModel();
    m_ids.removeAll(commandId);
    m_ids.prepend(commandId);
    while (m_ids.size() > kMax)
        m_ids.removeLast();
    endResetModel();
    save();
    emit countChanged();
}

void RecentCommandsModel::clear()
{
    if (m_ids.isEmpty())
        return;
    beginResetModel();
    m_ids.clear();
    endResetModel();
    save();
    emit countChanged();
}

QStringList RecentCommandsModel::ids() const
{
    return m_ids;
}

void RecentCommandsModel::load()
{
    QSettings settings;
    m_ids = settings.value(QStringLiteral("commands/recentIds")).toStringList();
}

void RecentCommandsModel::save() const
{
    QSettings settings;
    settings.setValue(QStringLiteral("commands/recentIds"), m_ids);
}
