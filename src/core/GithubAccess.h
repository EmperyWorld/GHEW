#pragma once

#include <QString>
#include <QStringList>

/// Argv for repository access, Actions, and security edits. Secret values stay in `stdinText`.

struct GithubHiddenCommand {
    QStringList argv;
    QString stdinText;
};

QStringList githubCollaboratorArgv(const QString &login, const QString &permission, bool remove, QString *error);
QStringList githubInvitationDeleteArgv(const QString &id, QString *error);
QStringList githubInvitationAcceptArgv(const QString &id, QString *error);

QStringList githubWorkflowArgv(const QString &action, const QString &workflow, const QString &ref, const QString &inputKey,
                               const QString &inputValue, QString *error);
QStringList githubRunArgv(const QString &action, const QString &runId, bool failedOnly, QString *error);

QStringList githubVariableArgv(const QString &name, const QString &value, const QString &envName, bool remove,
                               QString *error);
GithubHiddenCommand githubSecretCommand(const QString &name, const QString &value, const QString &envName,
                                        const QString &app, bool remove, QString *error);

QStringList githubEnvironmentArgv(const QString &name, int waitMinutes, bool protectedBranches, const QString &reviewers,
                                  bool remove, QString *error);
QStringList githubCacheDeleteArgv(const QString &keyOrId, QString *error);

QStringList githubAlertArgv(const QString &kind, const QString &number, const QString &state, const QString &reason,
                            QString *error);
GithubHiddenCommand githubRulesetCommand(const QString &id, const QString &name, const QString &includeRef,
                                         const QString &enforcement, const QString &rulesCsv, QString *error);
QStringList githubBranchProtectionArgv(const QString &branch, int reviews, bool enforceAdmins, bool blockForce,
                                       bool blockDelete, bool remove, QString *error);

QStringList githubDeployKeyArgv(const QString &title, const QString &key, bool readOnly, QString *error);
QStringList githubDeployKeyDeleteArgv(const QString &id, QString *error);
GithubHiddenCommand githubWebhookCommand(const QString &id, const QString &url, const QString &events,
                                         const QString &secret, bool active, bool remove, QString *error);

QStringList githubAdvisoryArgv(const QString &ghsa, const QString &summary, const QString &description,
                               const QString &severity, const QString &ecosystem, const QString &packageName,
                               const QString &versionRange, QString *error);
QStringList githubInteractionArgv(const QString &limit, bool clear, QString *error);

QStringList githubCodespaceCreateArgv(const QString &slug, const QString &branch, const QString &displayName,
                                      QString *error);
QStringList githubCodespaceEditArgv(const QString &name, const QString &displayName, QString *error);
QStringList githubCodespaceDeleteArgv(const QString &name, QString *error);
