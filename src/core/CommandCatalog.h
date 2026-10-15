#pragma once

#include "CommandSpec.h"

#include <QAbstractListModel>
#include <QHash>
#include <QQmlEngine>
#include <QString>
#include <QVector>
#include <optional>

class CommandCatalog : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("CommandCatalog is owned by App")

    Q_PROPERTY(QStringList groups READ groups NOTIFY catalogChanged)
    Q_PROPERTY(int count READ filteredCount NOTIFY catalogChanged)
    Q_PROPERTY(QString searchText READ searchText WRITE setSearchText NOTIFY catalogChanged)
    Q_PROPERTY(QString groupFilter READ groupFilter WRITE setGroupFilter NOTIFY catalogChanged)

public:
    enum Roles {
        IdRole = Qt::UserRole + 1,
        GroupRole,
        TitleRole,
        SynopsisRole,
        DangerRole,
        ExplanationRole,
    };

    explicit CommandCatalog(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    QStringList groups() const;
    QStringList allIds() const;
    int totalCount() const;
    int filteredCount() const;
    bool loadFromDirectory(const QString &directory);
    std::optional<CommandSpec> findById(const QString &id) const;

    Q_INVOKABLE QVariantMap commandAt(int row) const;
    Q_INVOKABLE QVariantMap commandById(const QString &id) const;
    Q_INVOKABLE QVariantList paramsFor(const QString &id) const;
    Q_INVOKABLE void setGroupFilter(const QString &group);
    Q_INVOKABLE void setSearchText(const QString &text);
    Q_INVOKABLE QString groupTitle(const QString &group) const;

    QString searchText() const;
    QString groupFilter() const;

signals:
    void catalogChanged();

private:
    bool loadFile(const QString &path);
    void rebuildFiltered();

    QVector<CommandSpec> m_all;
    QVector<CommandSpec> m_filtered;
    QString m_groupFilter;
    QString m_searchText;
};
