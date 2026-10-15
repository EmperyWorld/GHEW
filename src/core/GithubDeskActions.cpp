#include "GithubDeskActions.h"

namespace {

struct DeskAction {
    const char *id;
    const char *title;
    const char *args;
};

const DeskAction kActions[] = {
    {"gh_card", "Карточка репозитория", "description, homepage, visibility"},
    {"gh_rename", "Переименовать репозиторий", "name"},
    {"gh_topic_add", "Добавить тему", "topic"},
    {"gh_topic_remove", "Убрать тему", "topic"},
    {"gh_settings", "Разделы и ветка по умолчанию",
     "issues, wiki, projects, discussions, deleteOnMerge, asTemplate, defaultBranch"},
    {"gh_policy", "Политика слияния и защита",
     "mergeCommit, squashMerge, rebaseMerge, autoMerge, updateBranch, allowForking, webSignoff, pullRequests, "
     "secretScanning, pushProtection, advancedSecurity, squashTitle, squashBody, mergeTitle, mergeBody, prPolicy"},
    {"gh_archive", "Архив репозитория", "archived"},
    {"gh_fork", "Форк", "forkName, org, defaultBranchOnly, rewireOrigin"},
    {"gh_transfer", "Передача репозитория", "newOwner, newName, teamIds, typedName"},
    {"gh_delete_repo", "Удалить репозиторий", "typedName"},
    {"gh_lfs", "Git LFS", "enabled"},
    {"gh_file", "Файл оформления", "kind, body"},
    {"gh_template", "Шаблон из каталога GitHub", "kind, key"},
    {"gh_pages", "GitHub Pages", "branch, path, cname, https, enabled"},
    {"gh_wiki", "Страница wiki", "page, body"},
    {"gh_wiki_list", "Список страниц wiki", ""},
    {"gh_services", "Списки issues, PR, обсуждений, проектов и релизов", ""},
    {"gh_issue", "Создать или править issue", "number, title, body"},
    {"gh_issue_act", "Действие с issue", "number, action, extra"},
    {"gh_issue_link", "Связь issue", "number, kind, value, remove"},
    {"gh_label", "Создать метку", "name, color, description"},
    {"gh_label_edit", "Править метку", "name, color, description, newName"},
    {"gh_label_delete", "Удалить метку", "name"},
    {"gh_milestone", "Веха", "number, title, state"},
    {"gh_pull", "Создать или править pull request", "number, title, body, base, head, draft"},
    {"gh_pull_review", "Ревью pull request", "number, action, body"},
    {"gh_pull_merge", "Слить pull request", "number, method"},
    {"gh_pull_ready", "Черновик pull request", "number, draft"},
    {"gh_pull_update", "Обновить ветку pull request", "number, rebase"},
    {"gh_discussion", "Тема обсуждения", "number, title, body, category"},
    {"gh_discussion_comment", "Комментарий обсуждения", "target, body, remove"},
    {"gh_release", "Создать релиз", "tag, title, notes, draft, prerelease, target"},
    {"gh_release_edit", "Править релиз", "tag, title, notes, draft, prerelease, target"},
    {"gh_release_delete", "Удалить релиз", "tag, cleanupTag"},
    {"gh_release_upload", "Файл релиза", "tag, filePath"},
    {"gh_autolink", "Автоссылка", "prefix, urlTemplate, numeric"},
    {"gh_autolink_delete", "Удалить автоссылку", "id"},
    {"gh_project", "Создать проект", "title"},
    {"gh_project_field", "Поле проекта", "number, name, dataType, options"},
    {"gh_project_item", "Черновая карточка проекта", "number, title, body"},
    {"gh_project_add", "Привязать issue или PR", "number, url"},
    {"gh_project_edit", "Значение поля карточки", "number, url, field, value"},
    {"gh_project_link", "Привязать проект к репозиторию", "number"},
    {"gh_project_view", "Вид проекта", "number, name, layout"},
    {"gh_project_view_edit", "Переименовать или удалить вид", "number, viewId, name, remove"},
    {"gh_collaborator", "Соавтор", "login, permission, remove"},
    {"gh_invitation_delete", "Отозвать приглашение", "id"},
    {"gh_invitation_accept", "Принять приглашение", "id"},
    {"gh_workflow", "Workflow", "action, workflow, ref, inputKey, inputValue"},
    {"gh_run", "Запуск Actions", "action, runId, failedOnly"},
    {"gh_variable", "Переменная", "name, value, envName, remove"},
    {"gh_secret_delete", "Удалить секрет", "name, envName, app"},
    {"gh_environment", "Окружение деплоя", "name, waitMinutes, protectedBranches, reviewers, remove"},
    {"gh_cache_delete", "Удалить один кэш", "keyOrId"},
    {"gh_alert", "Статус оповещения", "kind, number, state, reason"},
    {"gh_ruleset", "Ruleset", "id, name, includeRef, enforcement, rules"},
    {"gh_protection", "Защита ветки", "branch, reviews, enforceAdmins, blockForce, blockDelete, remove"},
    {"gh_deploy_key", "Deploy key", "title, key, readOnly"},
    {"gh_deploy_key_delete", "Удалить deploy key", "id"},
    {"gh_webhook", "Вебхук репозитория", "id, url, events, active, remove"},
    {"gh_advisory", "Черновик совета безопасности", "ghsa, summary, description, severity, ecosystem, packageName, versionRange"},
    {"gh_interaction", "Лимит взаимодействий", "limit, clear"},
    {"gh_codespace", "Создать codespace", "branch, displayName"},
    {"gh_codespace_edit", "Переименовать codespace", "name, displayName"},
    {"gh_codespace_delete", "Удалить codespace", "name"},
    {"gh_profile", "Профиль", "name, bio, blog, company, location, twitter, hireable"},
    {"gh_email", "Почта", "email, action"},
    {"gh_emails", "Показать список почты", ""},
    {"gh_social", "Соцаккаунт", "url, remove"},
    {"gh_user_key", "Ключ пользователя", "kind, title, key, id, remove"},
    {"gh_profile_repo", "Репозиторий README профиля", "login"},
    {"gh_profile_readme", "Текст README профиля", "login, text, sha"},
    {"gh_org_card", "Карточка организации", "org, name, description, blog, email, twitter, location, company"},
    {"gh_org_policy", "Политика организации",
     "org, permission, publicRepos, privateRepos, internalRepos, pages, publicPages, privatePages, forkPrivate"},
    {"gh_org_team", "Команда", "org, name, description, privacy, slug, remove"},
    {"gh_org_member", "Участник команды", "org, slug, login, role, remove"},
    {"gh_org_people", "Люди организации", "org, login, email, invitationId, role, action"},
    {"gh_org_role", "Роль репозитория", "org, roleId, name, description, baseRole, permissions, remove"},
    {"gh_org_property", "Свойство организации", "org, name, valueType, description, allowedCsv, remove"},
    {"gh_org_hook", "Вебхук организации", "org, id, url, events, active, remove"},
    {"gh_org_banner", "Баннер организации", "org, text, expiresAt, dismissible, remove"},
    {"gh_org_security", "Конфигурация безопасности", "org, name, dependabot, secretScanning, pushProtection, configId, remove"},
    {"gh_org_attach", "Назначить конфигурацию", "org, configId, repoIds"},
    {"gh_org_manager", "Security manager", "org, teamSlug, remove"},
    {"gh_org_immutable", "Immutable releases", "org, policy, repoIds"},
    {"gh_org_role_assign", "Назначить роль организации", "org, login, roleId, remove"},
};

const DeskAction *findAction(const QString &id)
{
    for (const DeskAction &action : kActions) {
        if (id == QLatin1String(action.id))
            return &action;
    }
    return nullptr;
}

bool hasSecret(const QVariantMap &args)
{
    const QString value = args.value(QStringLiteral("secret")).toString();
    return !value.trimmed().isEmpty();
}

bool privateKey(const QVariantMap &args)
{
    return args.value(QStringLiteral("key")).toString().contains(QStringLiteral("PRIVATE KEY"), Qt::CaseInsensitive);
}

} // namespace

bool githubDeskActionKnown(const QString &id)
{
    return findAction(id.trimmed()) != nullptr;
}

QString githubDeskActionTitle(const QString &id)
{
    const DeskAction *action = findAction(id.trimmed());
    return action ? QString::fromUtf8(action->title) : QString();
}

QString githubDeskActionCatalog()
{
    QStringList lines;
    for (const DeskAction &action : kActions) {
        const QString args = QString::fromUtf8(action.args);
        lines << QStringLiteral("%1 — %2%3")
                     .arg(QLatin1String(action.id), QString::fromUtf8(action.title),
                          args.isEmpty() ? QString() : QStringLiteral(". args: ") + args);
    }
    return lines.join(QLatin1Char('\n'));
}

int githubDeskActionCount()
{
    return int(sizeof(kActions) / sizeof(kActions[0]));
}

bool githubDeskAcceptAdvisorCommand(const QString &id, QVariantMap *args)
{
    if (!githubDeskActionKnown(id) || !args)
        return false;
    if (id == QLatin1String("gh_secret_delete"))
        return args->value(QStringLiteral("value")).toString().isEmpty();
    if (id == QLatin1String("gh_webhook") || id == QLatin1String("gh_org_hook")) {
        if (hasSecret(*args))
            return false;
        args->remove(QStringLiteral("secret"));
        return true;
    }
    if (id == QLatin1String("gh_deploy_key") || id == QLatin1String("gh_user_key"))
        return !privateKey(*args);
    return true;
}
