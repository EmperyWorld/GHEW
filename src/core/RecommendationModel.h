#pragma once

#include <QAbstractListModel>
#include <QQmlEngine>
#include <QString>
#include <QVector>

struct Recommendation {
    QString commandId;
    QString title;
    QString reason;
    QString source; // "rule" | "llm"
    int priority = 0;
};

class RecommendationModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("RecommendationModel is owned by App")
    Q_PROPERTY(int count READ rowCount NOTIFY countChanged)

public:
    enum Roles {
        CommandIdRole = Qt::UserRole + 1,
        TitleRole,
        ReasonRole,
        SourceRole,
        PriorityRole,
    };

    explicit RecommendationModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QHash<int, QByteArray> roleNames() const override;

    void setItems(QVector<Recommendation> items);
    void mergeLlmItems(const QVector<Recommendation> &llmItems);
    void clear();
    QVector<Recommendation> items() const;

signals:
    void countChanged();

private:
    QVector<Recommendation> m_items;
};
