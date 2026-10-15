#pragma once

#include <QAbstractListModel>
#include <QQmlEngine>
#include <QString>
#include <QStringList>
#include <QVector>

class RecentReposModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("RecentReposModel is owned by App")
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Roles {
        PathRole = Qt::UserRole + 1,
        NameRole,
        ParentRole,
    };

    explicit RecentReposModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void touch(const QString &path);
    Q_INVOKABLE void remove(const QString &path);
    Q_INVOKABLE void clear();
    void load();
    void save() const;

signals:
    void countChanged();

private:
    static constexpr int kMaxRecent = 8;
    QStringList m_paths;
};
