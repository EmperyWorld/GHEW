#pragma once

#include <QAbstractListModel>
#include <QQmlEngine>
#include <QString>
#include <QVector>

struct WorktreeEntry {
    QString path;
    QString head;
    QString branch;
    bool bare = false;
    bool locked = false;
    bool prunable = false;
};

class WorktreeListModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("WorktreeListModel is owned by RepoContext")
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Roles {
        PathRole = Qt::UserRole + 1,
        HeadRole,
        BranchRole,
        BareRole,
        LockedRole,
        PrunableRole,
        IsCurrentRole,
    };

    explicit WorktreeListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setEntries(QVector<WorktreeEntry> entries, const QString &currentRoot);
    void clear();

signals:
    void countChanged();

private:
    QVector<WorktreeEntry> m_entries;
    QString m_currentRoot;
};
