#pragma once

#include <QAbstractListModel>
#include <QQmlEngine>
#include <QString>
#include <QVector>

struct BranchEntry {
    QString name;
    bool current = false;
    bool remote = false;
    QString upstream;
    int ahead = 0;
    int behind = 0;
};

class BranchListModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("BranchListModel is owned by RepoContext")
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        CurrentRole,
        RemoteRole,
        UpstreamRole,
        AheadRole,
        BehindRole,
    };

    explicit BranchListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setBranches(QVector<BranchEntry> branches);
    void clear();

signals:
    void countChanged();

private:
    QVector<BranchEntry> m_branches;
};
