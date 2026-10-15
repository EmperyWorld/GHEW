#include "RecommendationModel.h"

#include <QSet>
#include <algorithm>

RecommendationModel::RecommendationModel(QObject *parent)
    : QAbstractListModel(parent)
{
}

int RecommendationModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;
    return m_items.size();
}

QVariant RecommendationModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_items.size())
        return {};
    const Recommendation &r = m_items.at(index.row());
    switch (role) {
    case CommandIdRole:
        return r.commandId;
    case TitleRole:
        return r.title;
    case ReasonRole:
        return r.reason;
    case SourceRole:
        return r.source;
    case PriorityRole:
        return r.priority;
    default:
        return {};
    }
}

QHash<int, QByteArray> RecommendationModel::roleNames() const
{
    return {
        {CommandIdRole, "commandId"},
        {TitleRole, "title"},
        {ReasonRole, "reason"},
        {SourceRole, "source"},
        {PriorityRole, "priority"},
    };
}

void RecommendationModel::setItems(QVector<Recommendation> items)
{
    std::sort(items.begin(), items.end(), [](const Recommendation &a, const Recommendation &b) {
        return a.priority > b.priority;
    });
    beginResetModel();
    m_items = std::move(items);
    endResetModel();
    emit countChanged();
}

void RecommendationModel::mergeLlmItems(const QVector<Recommendation> &llmItems)
{
    QVector<Recommendation> next = m_items;
    // Drop previous LLM rows, keep rules.
    next.erase(std::remove_if(next.begin(), next.end(),
                               [](const Recommendation &r) {
                                   return r.source == QLatin1String("llm");
                               }),
               next.end());

    QSet<QString> seen;
    for (const Recommendation &r : next)
        seen.insert(r.commandId);

    for (const Recommendation &r : llmItems) {
        if (r.commandId.isEmpty() || seen.contains(r.commandId))
            continue;
        seen.insert(r.commandId);
        next.push_back(r);
    }
    setItems(std::move(next));
}

void RecommendationModel::clear()
{
    if (m_items.isEmpty())
        return;
    beginResetModel();
    m_items.clear();
    endResetModel();
    emit countChanged();
}

QVector<Recommendation> RecommendationModel::items() const
{
    return m_items;
}
