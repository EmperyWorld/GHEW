#pragma once

#include <QAbstractListModel>
#include <QQmlEngine>
#include <QString>
#include <QVector>

struct RemoteEntry {
    QString name;
    QString fetchUrl;
    QString pushUrl;
};

class RemoteListModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("RemoteListModel is owned by RepoContext")
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        FetchUrlRole,
        PushUrlRole,
    };

    explicit RemoteListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setEntries(QVector<RemoteEntry> entries);
    void clear();
    Q_INVOKABLE QString nameAt(int row) const;

signals:
    void countChanged();

private:
    QVector<RemoteEntry> m_entries;
};
