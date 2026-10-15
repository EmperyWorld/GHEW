#include "CommandCatalog.h"

#include <QDir>
#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

CommandCatalog::CommandCatalog(QObject *parent)
    : QAbstractListModel(parent)
{
}

int CommandCatalog::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid()) {
        return 0;
    }
    return m_filtered.size();
}

QVariant CommandCatalog::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 || index.row() >= m_filtered.size()) {
        return {};
    }
    const CommandSpec &spec = m_filtered.at(index.row());
    switch (role) {
    case IdRole:
        return spec.id;
    case GroupRole:
        return spec.group;
    case TitleRole:
        return spec.title;
    case SynopsisRole:
        return spec.synopsis;
    case DangerRole:
        return CommandSpec::dangerToString(spec.danger);
    case ExplanationRole:
        return spec.explanation;
    default:
        return {};
    }
}

QHash<int, QByteArray> CommandCatalog::roleNames() const
{
    return {
        {IdRole, "commandId"},
        {GroupRole, "group"},
        {TitleRole, "title"},
        {SynopsisRole, "synopsis"},
        {DangerRole, "danger"},
        {ExplanationRole, "explanation"},
    };
}

QStringList CommandCatalog::groups() const
{
    QStringList result;
    for (const CommandSpec &spec : m_all) {
        if (!result.contains(spec.group)) {
            result.append(spec.group);
        }
    }
    return result;
}

QStringList CommandCatalog::allIds() const
{
    QStringList ids;
    ids.reserve(m_all.size());
    for (const CommandSpec &spec : m_all)
        ids.append(spec.id);
    return ids;
}

int CommandCatalog::totalCount() const
{
    return m_all.size();
}

int CommandCatalog::filteredCount() const
{
    return m_filtered.size();
}

bool CommandCatalog::loadFromDirectory(const QString &directory)
{
    beginResetModel();
    m_all.clear();
    m_filtered.clear();

    QDir dir(directory);
    if (!dir.exists()) {
        endResetModel();
        emit catalogChanged();
        return false;
    }

    const QStringList files = dir.entryList({QStringLiteral("*.json")}, QDir::Files, QDir::Name);
    for (const QString &file : files) {
        loadFile(dir.filePath(file));
    }

    rebuildFiltered();
    endResetModel();
    emit catalogChanged();
    return !m_all.isEmpty();
}

std::optional<CommandSpec> CommandCatalog::findById(const QString &id) const
{
    for (const CommandSpec &spec : m_all) {
        if (spec.id == id) {
            return spec;
        }
    }
    return std::nullopt;
}

QVariantMap CommandCatalog::commandAt(int row) const
{
    if (row < 0 || row >= m_filtered.size()) {
        return {};
    }
    return commandById(m_filtered.at(row).id);
}

QVariantMap CommandCatalog::commandById(const QString &id) const
{
    const auto spec = findById(id);
    if (!spec) {
        return {};
    }
    return {
        {QStringLiteral("id"), spec->id},
        {QStringLiteral("group"), spec->group},
        {QStringLiteral("title"), spec->title},
        {QStringLiteral("synopsis"), spec->synopsis},
        {QStringLiteral("danger"), CommandSpec::dangerToString(spec->danger)},
        {QStringLiteral("explanation"), spec->explanation},
        {QStringLiteral("preview"), spec->previewStrategy},
    };
}

QVariantList CommandCatalog::paramsFor(const QString &id) const
{
    QVariantList list;
    const auto spec = findById(id);
    if (!spec) {
        return list;
    }
    for (const CommandParam &param : spec->params) {
        list.append(QVariantMap{
            {QStringLiteral("name"), param.name},
            {QStringLiteral("label"), param.label},
            {QStringLiteral("type"), param.type},
            {QStringLiteral("flag"), param.flag},
            {QStringLiteral("default"), param.defaultValue},
            {QStringLiteral("options"), param.options},
        });
    }
    return list;
}

QString CommandCatalog::groupTitle(const QString &group) const
{
    static const QHash<QString, QString> titles{
        {QStringLiteral("inspect"), QStringLiteral("Просмотр")},
        {QStringLiteral("staging"), QStringLiteral("Индекс")},
        {QStringLiteral("commit"), QStringLiteral("Коммиты")},
        {QStringLiteral("branch"), QStringLiteral("Ветки")},
        {QStringLiteral("remote"), QStringLiteral("Remote")},
        {QStringLiteral("merge"), QStringLiteral("Merge / Rebase")},
        {QStringLiteral("stash"), QStringLiteral("Stash")},
        {QStringLiteral("tag"), QStringLiteral("Теги")},
        {QStringLiteral("undo"), QStringLiteral("Отмена")},
        {QStringLiteral("advanced"), QStringLiteral("Advanced")},
        {QStringLiteral("github"), QStringLiteral("GitHub")},
    };
    return titles.value(group, group);
}

void CommandCatalog::setGroupFilter(const QString &group)
{
    if (m_groupFilter == group) {
        return;
    }
    beginResetModel();
    m_groupFilter = group;
    rebuildFiltered();
    endResetModel();
    emit catalogChanged();
}

QString CommandCatalog::groupFilter() const
{
    return m_groupFilter;
}

QString CommandCatalog::searchText() const
{
    return m_searchText;
}

void CommandCatalog::setSearchText(const QString &text)
{
    const QString trimmed = text.trimmed();
    if (m_searchText == trimmed)
        return;
    beginResetModel();
    m_searchText = trimmed;
    rebuildFiltered();
    endResetModel();
    emit catalogChanged();
}

bool CommandCatalog::loadFile(const QString &path)
{
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        return false;
    }

    const QJsonDocument doc = QJsonDocument::fromJson(file.readAll());
    QJsonArray arr;
    if (doc.isArray()) {
        arr = doc.array();
    } else if (doc.isObject()) {
        arr = doc.object().value(QStringLiteral("commands")).toArray();
    } else {
        return false;
    }

    for (const QJsonValue &value : arr) {
        const QJsonObject obj = value.toObject();
        CommandSpec spec;
        spec.id = obj.value(QStringLiteral("id")).toString();
        spec.group = obj.value(QStringLiteral("group")).toString();
        spec.title = obj.value(QStringLiteral("title")).toString();
        spec.synopsis = obj.value(QStringLiteral("synopsis")).toString();
        spec.danger = CommandSpec::dangerFromString(obj.value(QStringLiteral("danger")).toString());
        spec.explanation = obj.value(QStringLiteral("explanation")).toString();
        spec.previewStrategy = obj.value(QStringLiteral("preview")).toString(QStringLiteral("generic"));
        spec.runner = obj.value(QStringLiteral("runner")).toString();
        if (spec.runner.isEmpty())
            spec.runner = QStringLiteral("git");

        const QString argvKey = spec.runner == QLatin1String("gh") ? QStringLiteral("gh") : QStringLiteral("git");
        for (const QJsonValue &g : obj.value(argvKey).toArray()) {
            spec.gitBase.append(g.toString());
        }

        for (const QJsonValue &p : obj.value(QStringLiteral("params")).toArray()) {
            const QJsonObject po = p.toObject();
            CommandParam param;
            param.name = po.value(QStringLiteral("name")).toString();
            param.label = po.value(QStringLiteral("label")).toString();
            param.type = po.value(QStringLiteral("type")).toString();
            param.flag = po.value(QStringLiteral("flag")).toString();
            param.defaultValue = po.value(QStringLiteral("default")).toVariant();
            param.optional = po.value(QStringLiteral("optional")).toBool();
            param.keepEmpty = po.value(QStringLiteral("keepEmpty")).toBool();
            for (const QJsonValue &opt : po.value(QStringLiteral("options")).toArray()) {
                param.options.append(opt.toString());
            }
            spec.params.append(param);
        }

        if (!spec.id.isEmpty() && !spec.gitBase.isEmpty()) {
            m_all.append(spec);
        }
    }
    return true;
}

void CommandCatalog::rebuildFiltered()
{
    m_filtered.clear();
    const QString needle = m_searchText.toLower();
    for (const CommandSpec &spec : m_all) {
        if (!m_groupFilter.isEmpty() && spec.group != m_groupFilter)
            continue;
        if (!needle.isEmpty()) {
            const bool hit = spec.id.toLower().contains(needle)
                             || spec.title.toLower().contains(needle)
                             || spec.synopsis.toLower().contains(needle)
                             || spec.explanation.toLower().contains(needle);
            if (!hit)
                continue;
        }
        m_filtered.append(spec);
    }
}
