#include "FavoritesModel.h"

#include <QSettings>

FavoritesModel::FavoritesModel(QObject *parent)
    : QAbstractListModel(parent)
{
    load();
}

int FavoritesModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_ids.size();
}

QVariant FavoritesModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_ids.size())
        return {};
    if (role == CommandIdRole)
        return m_ids.at(index.row());
    return {};
}

QHash<int, QByteArray> FavoritesModel::roleNames() const
{
    return {{CommandIdRole, "commandId"}};
}

bool FavoritesModel::contains(const QString &commandId) const
{
    return m_ids.contains(commandId);
}

void FavoritesModel::toggle(const QString &commandId)
{
    if (commandId.isEmpty())
        return;
    const int idx = m_ids.indexOf(commandId);
    if (idx >= 0) {
        beginRemoveRows(QModelIndex(), idx, idx);
        m_ids.removeAt(idx);
        endRemoveRows();
    } else {
        beginInsertRows(QModelIndex(), m_ids.size(), m_ids.size());
        m_ids.append(commandId);
        endInsertRows();
    }
    emit countChanged();
    save();
}

void FavoritesModel::clear()
{
    if (m_ids.isEmpty())
        return;
    beginResetModel();
    m_ids.clear();
    endResetModel();
    emit countChanged();
    save();
}

void FavoritesModel::load()
{
    QSettings settings;
    beginResetModel();
    m_ids = settings.value(QStringLiteral("favorites/commandIds")).toStringList();
    endResetModel();
    emit countChanged();
}

void FavoritesModel::save() const
{
    QSettings settings;
    settings.setValue(QStringLiteral("favorites/commandIds"), m_ids);
}
