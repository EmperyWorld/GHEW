#pragma once

#include "HunkParse.h"

#include <QAbstractListModel>
#include <QQmlEngine>

class HunkListModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("HunkListModel is owned by App")
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)
    Q_PROPERTY(int selectedCount READ selectedCount NOTIFY countChanged)

public:
    enum Roles {
        FilePathRole = Qt::UserRole + 1,
        HeaderRole,
        BodyRole,
        SelectedRole,
        PreviewRole,
    };

    explicit HunkListModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    bool setData(const QModelIndex &index, const QVariant &value, int role) override;
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setHunks(QVector<DiffHunk> hunks);
    void clear();
    int selectedCount() const;
    QVector<DiffHunk> hunks() const;
    Q_INVOKABLE void setSelected(int row, bool selected);
    Q_INVOKABLE void selectAll(bool selected);

signals:
    void countChanged();

private:
    QVector<DiffHunk> m_hunks;
};
