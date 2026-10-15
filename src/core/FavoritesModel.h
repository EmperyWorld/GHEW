#pragma once

#include <QAbstractListModel>
#include <QQmlEngine>
#include <QString>
#include <QStringList>

class FavoritesModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("FavoritesModel is owned by App")
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Roles {
        CommandIdRole = Qt::UserRole + 1,
    };

    explicit FavoritesModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE bool contains(const QString &commandId) const;
    Q_INVOKABLE void toggle(const QString &commandId);
    Q_INVOKABLE void clear();

signals:
    void countChanged();

private:
    void load();
    void save() const;

    QStringList m_ids;
};
