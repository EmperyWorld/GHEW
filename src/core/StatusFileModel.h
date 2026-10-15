#pragma once

#include "StatusTypes.h"

#include <QAbstractListModel>
#include <QQmlEngine>
#include <QVector>

class StatusFileModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("StatusFileModel is owned by RepoContext")
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Roles {
        PathRole = Qt::UserRole + 1,
        CodeRole,
        StateRole,
        SummaryRole,
    };

    explicit StatusFileModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setFiles(QVector<StatusFileEntry> files);
    void clear();
    Q_INVOKABLE QString pathAt(int row) const;
    Q_INVOKABLE QString stateAt(int row) const;

signals:
    void countChanged();

private:
    QVector<StatusFileEntry> m_files;
};
