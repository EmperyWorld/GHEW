#pragma once

#include <QAbstractListModel>
#include <QQmlEngine>
#include <QString>
#include <QVector>

struct StashEntry {
    QString ref;     // stash@{0}
    QString message;
};

class StashListModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("StashListModel is owned by RepoContext")
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Roles {
        RefRole = Qt::UserRole + 1,
        MessageRole,
    };

    explicit StashListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setEntries(QVector<StashEntry> entries);
    void clear();

signals:
    void countChanged();

private:
    QVector<StashEntry> m_entries;
};
