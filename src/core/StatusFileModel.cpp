#include "StatusFileModel.h"

StatusFileModel::StatusFileModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int StatusFileModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_files.size();
}

QVariant StatusFileModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_files.size())
        return {};
    const StatusFileEntry &e = m_files.at(index.row());
    switch (role) {
    case PathRole:
        return e.path;
    case CodeRole:
        return e.code;
    case StateRole:
        return e.state;
    case SummaryRole:
        return e.summary;
    default:
        return {};
    }
}

QHash<int, QByteArray> StatusFileModel::roleNames() const
{
    return {
        {PathRole, "path"},
        {CodeRole, "code"},
        {StateRole, "fileState"},
        {SummaryRole, "summary"},
    };
}

void StatusFileModel::setFiles(QVector<StatusFileEntry> files)
{
    beginResetModel();
    m_files = std::move(files);
    endResetModel();
    emit countChanged();
}

void StatusFileModel::clear()
{
    if (m_files.isEmpty())
        return;
    beginResetModel();
    m_files.clear();
    endResetModel();
    emit countChanged();
}

QString StatusFileModel::pathAt(int row) const
{
    if (row < 0 || row >= m_files.size())
        return {};
    return m_files.at(row).path;
}

QString StatusFileModel::stateAt(int row) const
{
    if (row < 0 || row >= m_files.size())
        return {};
    return m_files.at(row).state;
}
