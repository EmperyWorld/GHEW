#pragma once

#include <QAbstractListModel>
#include <QQmlEngine>
#include <QString>
#include <QVector>

struct ReflogEntry {
    QString selector; // HEAD@{0}
    QString hash;
    QString message;
};

class ReflogListModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("ReflogListModel is owned by RepoContext")
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Roles {
        SelectorRole = Qt::UserRole + 1,
        HashRole,
        MessageRole,
    };

    explicit ReflogListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setEntries(QVector<ReflogEntry> entries);
    void clear();

signals:
    void countChanged();

private:
    QVector<ReflogEntry> m_entries;
};
