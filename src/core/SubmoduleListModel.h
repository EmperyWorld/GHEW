#pragma once

#include <QAbstractListModel>
#include <QQmlEngine>
#include <QString>
#include <QVector>

struct SubmoduleEntry {
    QString path;
    QString sha;
    QString describe;
    QString status; // ok | modified | uninitialized | conflict
};

class SubmoduleListModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("SubmoduleListModel is owned by RepoContext")
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Roles {
        PathRole = Qt::UserRole + 1,
        ShaRole,
        DescribeRole,
        StatusRole,
    };

    explicit SubmoduleListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setEntries(QVector<SubmoduleEntry> entries);
    void clear();

signals:
    void countChanged();

private:
    QVector<SubmoduleEntry> m_entries;
};
