#include "AppController.h"

#include "core/GithubDeskActions.h"

#include <initializer_list>

namespace {

QString strArg(const QVariantMap &args, const char *key)
{
    return args.value(QLatin1String(key)).toString();
}

bool hasArg(const QVariantMap &args, const char *key)
{
    return args.contains(QLatin1String(key));
}

bool boolArg(const QVariantMap &args, const char *key)
{
    const QVariant value = args.value(QLatin1String(key));
    if (value.userType() == QMetaType::Bool)
        return value.toBool();
    const QString text = value.toString().trimmed().toLower();
    return text == QLatin1String("true") || text == QLatin1String("1") || text == QLatin1String("yes")
           || text == QLatin1String("да");
}

bool boolOr(const QVariantMap &args, const char *key, bool fallback)
{
    return hasArg(args, key) ? boolArg(args, key) : fallback;
}

QString needKeys(const QVariantMap &args, std::initializer_list<const char *> keys)
{
    QStringList missing;
    for (const char *key : keys) {
        if (!hasArg(args, key))
            missing << QLatin1String(key);
    }
    return missing.join(QStringLiteral(", "));
}

} // namespace

void AppController::dispatchGithubDesk(const QString &id, const QVariantMap &args)
{
    const QString missing = [&]() {
        if (id == QLatin1String("gh_archive"))
            return needKeys(args, {"archived"});
        if (id == QLatin1String("gh_lfs"))
            return needKeys(args, {"enabled"});
        if (id == QLatin1String("gh_pages"))
            return needKeys(args, {"enabled"});
        if (id == QLatin1String("gh_file"))
            return needKeys(args, {"kind", "body"});
        if (id == QLatin1String("gh_template"))
            return needKeys(args, {"kind", "key"});
        if (id == QLatin1String("gh_profile"))
            return needKeys(args, {"hireable"});
        if (id == QLatin1String("gh_protection"))
            return needKeys(args, {"branch", "reviews", "enforceAdmins", "blockForce", "blockDelete", "remove"});
        if (id == QLatin1String("gh_environment"))
            return needKeys(args, {"name", "protectedBranches", "remove"});
        if (id == QLatin1String("gh_org_policy"))
            return needKeys(args, {"org", "permission", "publicRepos", "privateRepos", "internalRepos", "pages",
                                   "publicPages", "privatePages", "forkPrivate"});
        return QString();
    }();
    if (!missing.isEmpty()) {
        emit errorOccurred(tr("Для %1 не хватает полей: %2.").arg(id, missing));
        return;
    }

    if (id == QLatin1String("gh_card")) {
        saveGithubCard(hasArg(args, "description") ? strArg(args, "description") : m_github.description(),
                       hasArg(args, "homepage") ? strArg(args, "homepage") : m_github.homepage(),
                       hasArg(args, "visibility") ? strArg(args, "visibility") : m_github.visibility());
    } else if (id == QLatin1String("gh_rename")) {
        renameGithubRepo(strArg(args, "name"));
    } else if (id == QLatin1String("gh_topic_add")) {
        addGithubTopic(strArg(args, "topic"));
    } else if (id == QLatin1String("gh_topic_remove")) {
        removeGithubTopic(strArg(args, "topic"));
    } else if (id == QLatin1String("gh_settings")) {
        saveGithubSettings(boolOr(args, "issues", m_github.issuesEnabled()),
                           boolOr(args, "wiki", m_github.wikiEnabled()),
                           boolOr(args, "projects", m_github.projectsEnabled()),
                           boolOr(args, "discussions", m_github.discussionsEnabled()),
                           boolOr(args, "deleteOnMerge", m_github.deleteBranchOnMerge()),
                           boolOr(args, "asTemplate", m_github.isTemplate()), strArg(args, "defaultBranch"));
    } else if (id == QLatin1String("gh_policy")) {
        QVariantMap draft = args;
        const QStringList flags{QStringLiteral("mergeCommit"),     QStringLiteral("squashMerge"),
                                QStringLiteral("rebaseMerge"),     QStringLiteral("autoMerge"),
                                QStringLiteral("updateBranch"),    QStringLiteral("allowForking"),
                                QStringLiteral("webSignoff"),      QStringLiteral("pullRequests"),
                                QStringLiteral("advancedSecurity"), QStringLiteral("secretScanning"),
                                QStringLiteral("pushProtection")};
        for (const QString &flag : flags) {
            if (draft.contains(flag))
                draft.insert(flag + QStringLiteral("Known"), true);
        }
        const QStringList texts{QStringLiteral("squashTitle"), QStringLiteral("squashBody"), QStringLiteral("mergeTitle"),
                                QStringLiteral("mergeBody"), QStringLiteral("prPolicy")};
        for (const QString &text : texts) {
            if (draft.contains(text))
                draft.insert(text + QStringLiteral("Known"), true);
        }
        saveGithubPolicy(draft);
    } else if (id == QLatin1String("gh_archive")) {
        setGithubArchived(boolArg(args, "archived"));
    } else if (id == QLatin1String("gh_fork")) {
        forkGithubRepo(strArg(args, "forkName"), strArg(args, "org"), boolArg(args, "defaultBranchOnly"),
                       boolArg(args, "rewireOrigin"));
    } else if (id == QLatin1String("gh_transfer")) {
        transferGithubRepo(strArg(args, "newOwner"), strArg(args, "newName"), strArg(args, "teamIds"),
                           strArg(args, "typedName"));
    } else if (id == QLatin1String("gh_delete_repo")) {
        deleteGithubRepo(strArg(args, "typedName"));
    } else if (id == QLatin1String("gh_lfs")) {
        setGithubLfs(boolArg(args, "enabled"));
    } else if (id == QLatin1String("gh_file")) {
        QString kind = strArg(args, "kind").trimmed().toLower();
        QString body = strArg(args, "body");
        if (kind.isEmpty() && m_deskFromAdvisor)
            kind = m_advisor.takePendingDressKind();
        if (body.trimmed().isEmpty() && m_deskFromAdvisor)
            body = m_advisor.takePendingStudentReply();
        if (body.trimmed().isEmpty())
            body = m_github.takePendingTemplate(kind);
        // UI may fall back to a stock template; mentor RUN must not silently invent text.
        if (body.trimmed().isEmpty() && !m_deskFromAdvisor) {
            body = m_github.dressTemplate(kind);
            if (body.trimmed().isEmpty() && kind == QLatin1String("license")) {
                const QString flavor = strArg(args, "key").trimmed().isEmpty() ? strArg(args, "license")
                                                                               : strArg(args, "key");
                body = m_github.licenseTemplate(flavor);
            }
            if (body.trimmed().isEmpty() && kind == QLatin1String("readme"))
                body = m_github.readmeTemplate(strArg(args, "title"), strArg(args, "blurb"));
        }
        if (kind.isEmpty() || body.trimmed().isEmpty()) {
            emit errorOccurred(tr("Для записи файла нужны kind и текст (body). "
                                  "Ответьте наставнику своим текстом или вставьте body в RUN."));
            return;
        }
        const bool ok = saveDressFile(kind, body);
        if (ok && m_deskFromAdvisor) {
            m_advisor.continueAfterCommand(id, tr("записать файл %1").arg(kind), 0, m_github.note(),
                                           m_github.note());
        }
    } else if (id == QLatin1String("gh_template")) {
        const QString kind = strArg(args, "kind");
        const QString key = strArg(args, "key");
        const QString text = fetchGithubTemplate(kind, key);
        if (text.isEmpty()) {
            if (m_deskFromAdvisor)
                emit errorOccurred(m_github.note().isEmpty() ? tr("Шаблон не получен.") : m_github.note());
        } else if (m_deskFromAdvisor) {
            // Mentor RUN must land on disk. The GitHub tab still uses draft-only fetch.
            const bool ok = saveDressFile(kind, text);
            if (ok) {
                m_advisor.continueAfterCommand(id, tr("шаблон %1 → файл").arg(key), 0, text.left(800),
                                               m_github.note());
            }
        }
    } else if (id == QLatin1String("gh_pages")) {
        saveGithubPages(strArg(args, "branch"), strArg(args, "path"), strArg(args, "cname"), boolArg(args, "https"),
                        boolArg(args, "enabled"));
    } else if (id == QLatin1String("gh_wiki")) {
        publishWikiPage(strArg(args, "page"), strArg(args, "body"));
    } else if (id == QLatin1String("gh_wiki_list")) {
        m_github.refreshWiki();
        if (m_deskFromAdvisor) {
            const QString pages = m_github.wikiPages().isEmpty() ? m_github.note() : m_github.wikiPages().join(QStringLiteral(", "));
            m_advisor.continueAfterCommand(id, QStringLiteral("gh_wiki_list"), 0, pages, m_github.note());
        }
    } else if (id == QLatin1String("gh_services")) {
        m_github.refreshServices();
        if (m_deskFromAdvisor)
            m_advisor.continueAfterCommand(id, QStringLiteral("gh_services"), 0, m_github.note(), m_github.note());
    } else if (id == QLatin1String("gh_issue")) {
        saveGithubIssue(strArg(args, "number"), strArg(args, "title"), strArg(args, "body"));
    } else if (id == QLatin1String("gh_issue_act")) {
        actGithubIssue(strArg(args, "number"), strArg(args, "action"), strArg(args, "extra"));
    } else if (id == QLatin1String("gh_issue_link")) {
        linkGithubIssue(strArg(args, "number"), strArg(args, "kind"), strArg(args, "value"), boolArg(args, "remove"));
    } else if (id == QLatin1String("gh_label")) {
        createGithubLabel(strArg(args, "name"), strArg(args, "color"), strArg(args, "description"));
    } else if (id == QLatin1String("gh_label_edit")) {
        editGithubLabel(strArg(args, "name"), strArg(args, "color"), strArg(args, "description"), strArg(args, "newName"));
    } else if (id == QLatin1String("gh_label_delete")) {
        deleteGithubLabel(strArg(args, "name"));
    } else if (id == QLatin1String("gh_milestone")) {
        saveGithubMilestone(strArg(args, "number"), strArg(args, "title"), strArg(args, "state"));
    } else if (id == QLatin1String("gh_pull")) {
        saveGithubPull(strArg(args, "number"), strArg(args, "title"), strArg(args, "body"), strArg(args, "base"),
                       strArg(args, "head"), boolArg(args, "draft"));
    } else if (id == QLatin1String("gh_pull_review")) {
        reviewGithubPull(strArg(args, "number"), strArg(args, "action"), strArg(args, "body"));
    } else if (id == QLatin1String("gh_pull_merge")) {
        mergeGithubPull(strArg(args, "number"), strArg(args, "method"));
    } else if (id == QLatin1String("gh_pull_ready")) {
        readyGithubPull(strArg(args, "number"), boolArg(args, "draft"));
    } else if (id == QLatin1String("gh_pull_update")) {
        updateGithubPullBranch(strArg(args, "number"), boolArg(args, "rebase"));
    } else if (id == QLatin1String("gh_discussion")) {
        saveGithubDiscussion(strArg(args, "number"), strArg(args, "title"), strArg(args, "body"), strArg(args, "category"));
    } else if (id == QLatin1String("gh_discussion_comment")) {
        commentGithubDiscussion(strArg(args, "target"), strArg(args, "body"), boolArg(args, "remove"));
    } else if (id == QLatin1String("gh_release")) {
        createGithubRelease(strArg(args, "tag"), strArg(args, "title"), strArg(args, "notes"), boolArg(args, "draft"),
                            boolArg(args, "prerelease"), strArg(args, "target"));
    } else if (id == QLatin1String("gh_release_edit")) {
        editGithubRelease(strArg(args, "tag"), strArg(args, "title"), strArg(args, "notes"), boolArg(args, "draft"),
                          boolArg(args, "prerelease"), strArg(args, "target"));
    } else if (id == QLatin1String("gh_release_delete")) {
        deleteGithubRelease(strArg(args, "tag"), boolArg(args, "cleanupTag"));
    } else if (id == QLatin1String("gh_release_upload")) {
        uploadGithubRelease(strArg(args, "tag"), strArg(args, "filePath"));
    } else if (id == QLatin1String("gh_autolink")) {
        createGithubAutolink(strArg(args, "prefix"), strArg(args, "urlTemplate"), boolArg(args, "numeric"));
    } else if (id == QLatin1String("gh_autolink_delete")) {
        deleteGithubAutolink(strArg(args, "id"));
    } else if (id == QLatin1String("gh_project")) {
        createGithubProject(strArg(args, "title"));
    } else if (id == QLatin1String("gh_project_field")) {
        createGithubProjectField(strArg(args, "number"), strArg(args, "name"), strArg(args, "dataType"),
                                 strArg(args, "options"));
    } else if (id == QLatin1String("gh_project_item")) {
        createGithubProjectItem(strArg(args, "number"), strArg(args, "title"), strArg(args, "body"));
    } else if (id == QLatin1String("gh_project_add")) {
        addGithubProjectItem(strArg(args, "number"), strArg(args, "url"));
    } else if (id == QLatin1String("gh_project_edit")) {
        editGithubProjectItem(strArg(args, "number"), strArg(args, "url"), strArg(args, "field"), strArg(args, "value"));
    } else if (id == QLatin1String("gh_project_link")) {
        linkGithubProject(strArg(args, "number"));
    } else if (id == QLatin1String("gh_project_view")) {
        createGithubProjectView(strArg(args, "number"), strArg(args, "name"), strArg(args, "layout"));
    } else if (id == QLatin1String("gh_project_view_edit")) {
        editGithubProjectView(strArg(args, "number"), strArg(args, "viewId"), strArg(args, "name"), boolArg(args, "remove"));
    } else if (id == QLatin1String("gh_collaborator")) {
        saveGithubCollaborator(strArg(args, "login"), strArg(args, "permission"), boolArg(args, "remove"));
    } else if (id == QLatin1String("gh_invitation_delete")) {
        deleteGithubInvitation(strArg(args, "id"));
    } else if (id == QLatin1String("gh_invitation_accept")) {
        acceptGithubInvitation(strArg(args, "id"));
    } else if (id == QLatin1String("gh_workflow")) {
        actGithubWorkflow(strArg(args, "action"), strArg(args, "workflow"), strArg(args, "ref"), strArg(args, "inputKey"),
                          strArg(args, "inputValue"));
    } else if (id == QLatin1String("gh_run")) {
        actGithubRun(strArg(args, "action"), strArg(args, "runId"), boolArg(args, "failedOnly"));
    } else if (id == QLatin1String("gh_variable")) {
        saveGithubVariable(strArg(args, "name"), strArg(args, "value"), strArg(args, "envName"), boolArg(args, "remove"));
    } else if (id == QLatin1String("gh_secret_delete")) {
        saveGithubSecret(strArg(args, "name"), QString(), strArg(args, "envName"), strArg(args, "app"), true);
    } else if (id == QLatin1String("gh_environment")) {
        saveGithubEnvironment(strArg(args, "name"), strArg(args, "waitMinutes"), boolArg(args, "protectedBranches"),
                              strArg(args, "reviewers"), boolArg(args, "remove"));
    } else if (id == QLatin1String("gh_cache_delete")) {
        deleteGithubCache(strArg(args, "keyOrId"));
    } else if (id == QLatin1String("gh_alert")) {
        saveGithubAlert(strArg(args, "kind"), strArg(args, "number"), strArg(args, "state"), strArg(args, "reason"));
    } else if (id == QLatin1String("gh_ruleset")) {
        saveGithubRuleset(strArg(args, "id"), strArg(args, "name"), strArg(args, "includeRef"), strArg(args, "enforcement"),
                          strArg(args, "rules"));
    } else if (id == QLatin1String("gh_protection")) {
        saveGithubBranchProtection(strArg(args, "branch"), args.value(QStringLiteral("reviews")).toInt(),
                                   boolArg(args, "enforceAdmins"), boolArg(args, "blockForce"), boolArg(args, "blockDelete"),
                                   boolArg(args, "remove"));
    } else if (id == QLatin1String("gh_deploy_key")) {
        saveGithubDeployKey(strArg(args, "title"), strArg(args, "key"), boolArg(args, "readOnly"));
    } else if (id == QLatin1String("gh_deploy_key_delete")) {
        deleteGithubDeployKey(strArg(args, "id"));
    } else if (id == QLatin1String("gh_webhook")) {
        saveGithubWebhook(strArg(args, "id"), strArg(args, "url"), strArg(args, "events"), QString(),
                          boolOr(args, "active", true), boolArg(args, "remove"));
    } else if (id == QLatin1String("gh_advisory")) {
        saveGithubAdvisory(strArg(args, "ghsa"), strArg(args, "summary"), strArg(args, "description"), strArg(args, "severity"),
                           strArg(args, "ecosystem"), strArg(args, "packageName"), strArg(args, "versionRange"));
    } else if (id == QLatin1String("gh_interaction")) {
        saveGithubInteraction(strArg(args, "limit"), boolArg(args, "clear"));
    } else if (id == QLatin1String("gh_codespace")) {
        createGithubCodespace(strArg(args, "branch"), strArg(args, "displayName"));
    } else if (id == QLatin1String("gh_codespace_edit")) {
        editGithubCodespace(strArg(args, "name"), strArg(args, "displayName"));
    } else if (id == QLatin1String("gh_codespace_delete")) {
        deleteGithubCodespace(strArg(args, "name"));
    } else if (id == QLatin1String("gh_profile")) {
        saveGithubProfile(strArg(args, "name"), strArg(args, "bio"), strArg(args, "blog"), strArg(args, "company"),
                          strArg(args, "location"), strArg(args, "twitter"), boolArg(args, "hireable"));
    } else if (id == QLatin1String("gh_email")) {
        saveGithubEmail(strArg(args, "email"), strArg(args, "action"));
    } else if (id == QLatin1String("gh_emails")) {
        m_github.refreshEmails();
        if (m_deskFromAdvisor)
            m_advisor.continueAfterCommand(id, QStringLiteral("gh_emails"), 0, m_github.note(), m_github.note());
    } else if (id == QLatin1String("gh_social")) {
        saveGithubSocial(strArg(args, "url"), boolArg(args, "remove"));
    } else if (id == QLatin1String("gh_user_key")) {
        saveGithubUserKey(strArg(args, "kind"), strArg(args, "title"), strArg(args, "key"), strArg(args, "id"),
                          boolArg(args, "remove"));
    } else if (id == QLatin1String("gh_profile_repo")) {
        createGithubProfileRepo(strArg(args, "login"));
    } else if (id == QLatin1String("gh_profile_readme")) {
        saveGithubProfileReadme(strArg(args, "login"), strArg(args, "text"), strArg(args, "sha"));
    } else if (id == QLatin1String("gh_org_card")) {
        saveGithubOrgCard(strArg(args, "org"), strArg(args, "name"), strArg(args, "description"), strArg(args, "blog"),
                          strArg(args, "email"), strArg(args, "twitter"), strArg(args, "location"), strArg(args, "company"));
    } else if (id == QLatin1String("gh_org_policy")) {
        saveGithubOrgPolicy(strArg(args, "org"), strArg(args, "permission"), boolArg(args, "publicRepos"),
                            boolArg(args, "privateRepos"), boolArg(args, "internalRepos"), boolArg(args, "pages"),
                            boolArg(args, "publicPages"), boolArg(args, "privatePages"), boolArg(args, "forkPrivate"));
    } else if (id == QLatin1String("gh_org_team")) {
        saveGithubOrgTeam(strArg(args, "org"), strArg(args, "name"), strArg(args, "description"), strArg(args, "privacy"),
                          strArg(args, "slug"), boolArg(args, "remove"));
    } else if (id == QLatin1String("gh_org_member")) {
        saveGithubOrgTeamMember(strArg(args, "org"), strArg(args, "slug"), strArg(args, "login"), strArg(args, "role"),
                                boolArg(args, "remove"));
    } else if (id == QLatin1String("gh_org_people")) {
        saveGithubOrgPeople(strArg(args, "org"), strArg(args, "login"), strArg(args, "email"), strArg(args, "invitationId"),
                            strArg(args, "role"), strArg(args, "action"));
    } else if (id == QLatin1String("gh_org_role")) {
        saveGithubOrgRole(strArg(args, "org"), strArg(args, "roleId"), strArg(args, "name"), strArg(args, "description"),
                          strArg(args, "baseRole"), strArg(args, "permissions"), boolArg(args, "remove"));
    } else if (id == QLatin1String("gh_org_property")) {
        saveGithubOrgProperty(strArg(args, "org"), strArg(args, "name"), strArg(args, "valueType"), strArg(args, "description"),
                              strArg(args, "allowedCsv"), boolArg(args, "remove"));
    } else if (id == QLatin1String("gh_org_hook")) {
        saveGithubOrgHook(strArg(args, "org"), strArg(args, "id"), strArg(args, "url"), strArg(args, "events"), QString(),
                          boolOr(args, "active", true), boolArg(args, "remove"));
    } else if (id == QLatin1String("gh_org_banner")) {
        saveGithubOrgBanner(strArg(args, "org"), strArg(args, "text"), strArg(args, "expiresAt"), boolArg(args, "dismissible"),
                            boolArg(args, "remove"));
    } else if (id == QLatin1String("gh_org_security")) {
        saveGithubOrgSecurityConfig(strArg(args, "org"), strArg(args, "name"), strArg(args, "dependabot"),
                                    strArg(args, "secretScanning"), strArg(args, "pushProtection"), strArg(args, "configId"),
                                    boolArg(args, "remove"));
    } else if (id == QLatin1String("gh_org_attach")) {
        attachGithubOrgSecurity(strArg(args, "org"), strArg(args, "configId"), strArg(args, "repoIds"));
    } else if (id == QLatin1String("gh_org_manager")) {
        saveGithubOrgSecurityManager(strArg(args, "org"), strArg(args, "teamSlug"), boolArg(args, "remove"));
    } else if (id == QLatin1String("gh_org_immutable")) {
        saveGithubOrgImmutable(strArg(args, "org"), strArg(args, "policy"), strArg(args, "repoIds"));
    } else if (id == QLatin1String("gh_org_role_assign")) {
        assignGithubOrgRole(strArg(args, "org"), strArg(args, "login"), strArg(args, "roleId"), boolArg(args, "remove"));
    } else {
        emit errorOccurred(tr("Неизвестная правка GitHub: %1").arg(id));
    }
}
