#pragma once

#include <QAbstractListModel>
#include <QString>
#include <QVector>

#ifndef GIT_LAB_UNIT_TEST
#include <QQmlEngine>
#endif

struct CommitEntry {
    QString hash;
    QString shortHash;
    QString subject;
    QString author;
    QString relativeTime;
};

class CommitLogModel : public QAbstractListModel
{
    Q_OBJECT
#ifndef GIT_LAB_UNIT_TEST
    QML_ELEMENT
    QML_UNCREATABLE("CommitLogModel is owned by RepoContext")
#endif
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(int totalCount READ totalCount NOTIFY countChanged)
    Q_PROPERTY(QString filter READ filter WRITE setFilter NOTIFY filterChanged)

public:
    enum Roles {
        HashRole = Qt::UserRole + 1,
        ShortHashRole,
        SubjectRole,
        AuthorRole,
        RelativeTimeRole,
    };

    explicit CommitLogModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setCommits(QVector<CommitEntry> commits);
    void clear();
    int totalCount() const;
    QString filter() const;
    void setFilter(const QString &filter);

signals:
    void countChanged();
    void filterChanged();

private:
    void applyFilter();

    QVector<CommitEntry> m_all;
    QVector<CommitEntry> m_commits;
    QString m_filter;
};
