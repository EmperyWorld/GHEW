#pragma once

#include <QAbstractListModel>
#include <QQmlEngine>
#include <QString>
#include <QVector>

struct HookEntry {
    QString name;
    bool executable = false;
    bool sample = false;
};

class HookListModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("HookListModel is owned by RepoContext")
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(int activeCount READ activeCount NOTIFY countChanged)

public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        ExecutableRole,
        SampleRole,
    };

    explicit HookListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setEntries(QVector<HookEntry> entries);
    void clear();
    int activeCount() const;

signals:
    void countChanged();

private:
    QVector<HookEntry> m_entries;
};
