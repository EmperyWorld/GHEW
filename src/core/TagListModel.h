#pragma once

#include <QAbstractListModel>
#include <QQmlEngine>
#include <QString>
#include <QVector>

struct TagEntry {
    QString name;
    QString hash;
    QString shortHash;
    QString relativeTime;
    QString subject;
};

class TagListModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("TagListModel is owned by RepoContext")
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Roles {
        NameRole = Qt::UserRole + 1,
        HashRole,
        ShortHashRole,
        RelativeTimeRole,
        SubjectRole,
    };

    explicit TagListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setEntries(QVector<TagEntry> entries);
    void clear();

signals:
    void countChanged();

private:
    QVector<TagEntry> m_entries;
};
