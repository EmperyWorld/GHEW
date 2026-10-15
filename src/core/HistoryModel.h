#pragma once

#include <QAbstractListModel>
#include <QQmlEngine>
#include <QString>
#include <QVariantMap>
#include <QVector>

struct HistoryEntry {
    QString cmdline;
    QString repoRoot;
    int exitCode = -1;
    bool ok = false;
    QString timestamp;
    QString commandId;
    QString argsJson;
};

class HistoryModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("HistoryModel is owned by App")
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Roles {
        CmdlineRole = Qt::UserRole + 1,
        RepoRole,
        ExitCodeRole,
        OkRole,
        TimestampRole,
        CommandIdRole,
        ArgsJsonRole,
    };

    explicit HistoryModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void prepend(const HistoryEntry &entry);
    Q_INVOKABLE void clear();
    HistoryEntry entryAt(int row) const;

signals:
    void countChanged();

private:
    QVector<HistoryEntry> m_entries;
};
