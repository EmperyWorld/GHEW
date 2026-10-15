#include "HunkListModel.h"

HunkListModel::HunkListModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int HunkListModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_hunks.size();
}

QVariant HunkListModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_hunks.size())
        return {};
    const DiffHunk &h = m_hunks.at(index.row());
    switch (role) {
    case FilePathRole:
        return h.filePath;
    case HeaderRole:
        return h.header;
    case BodyRole:
        return h.lines.join(QLatin1Char('\n'));
    case SelectedRole:
        return h.selected;
    case PreviewRole: {
        QStringList preview = h.lines.mid(0, 8);
        if (h.lines.size() > 8)
            preview << QStringLiteral("…");
        return preview.join(QLatin1Char('\n'));
    }
    default:
        return {};
    }
}

bool HunkListModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (!index.isValid() || role != SelectedRole)
        return false;
    m_hunks[index.row()].selected = value.toBool();
    emit dataChanged(index, index, {SelectedRole});
    emit countChanged();
    return true;
}

Qt::ItemFlags HunkListModel::flags(const QModelIndex &index) const
{
    if (!index.isValid())
        return Qt::NoItemFlags;
    return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsEditable;
}

QHash<int, QByteArray> HunkListModel::roleNames() const
{
    return {
        {FilePathRole, "filePath"},
        {HeaderRole, "header"},
        {BodyRole, "body"},
        {SelectedRole, "selected"},
        {PreviewRole, "preview"},
    };
}

void HunkListModel::setHunks(QVector<DiffHunk> hunks)
{
    beginResetModel();
    m_hunks = std::move(hunks);
    endResetModel();
    emit countChanged();
}

void HunkListModel::clear()
{
    if (m_hunks.isEmpty())
        return;
    beginResetModel();
    m_hunks.clear();
    endResetModel();
    emit countChanged();
}

int HunkListModel::selectedCount() const
{
    int n = 0;
    for (const DiffHunk &h : m_hunks) {
        if (h.selected)
            ++n;
    }
    return n;
}

QVector<DiffHunk> HunkListModel::hunks() const
{
    return m_hunks;
}

void HunkListModel::setSelected(int row, bool selected)
{
    if (row < 0 || row >= m_hunks.size())
        return;
    setData(index(row), selected, SelectedRole);
}

void HunkListModel::selectAll(bool selected)
{
    for (int i = 0; i < m_hunks.size(); ++i)
        m_hunks[i].selected = selected;
    if (!m_hunks.isEmpty())
        emit dataChanged(index(0), index(m_hunks.size() - 1), {SelectedRole});
    emit countChanged();
}
