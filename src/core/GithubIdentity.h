#pragma once

#include "GithubAccess.h"

/// Profile and organization edits. Webhook secrets and GPG blocks stay in `stdinText`.

GithubHiddenCommand githubProfileCommand(const QString &name, const QString &bio, const QString &blog, const QString &company,
                                         const QString &location, const QString &twitter, bool hireable, QString *error);
GithubHiddenCommand githubEmailCommand(const QString &email, const QString &action, QString *error);
QStringList githubEmailLines(const QByteArray &json, QString *error);
GithubHiddenCommand githubSocialCommand(const QString &url, bool remove, QString *error);
GithubHiddenCommand githubUserKeyCommand(const QString &kind, const QString &title, const QString &key, const QString &id,
                                         bool remove, QString *error);

QStringList githubProfileRepoArgv(const QString &login, QString *error);
GithubHiddenCommand githubProfileReadmeCommand(const QString &login, const QString &text, const QString &sha, QString *error);

GithubHiddenCommand githubOrgCardCommand(const QString &org, const QString &name, const QString &description, const QString &blog,
                                         const QString &email, const QString &twitter, const QString &location,
                                         const QString &company, QString *error);
GithubHiddenCommand githubOrgPolicyCommand(const QString &org, const QString &permission, bool publicRepos, bool privateRepos,
                                           bool internalRepos, bool pages, bool publicPages, bool privatePages,
                                           bool forkPrivate, QString *error);
GithubHiddenCommand githubOrgTeamCommand(const QString &org, const QString &name, const QString &description,
                                         const QString &privacy, const QString &slug, bool remove, QString *error);
GithubHiddenCommand githubOrgTeamMemberCommand(const QString &org, const QString &slug, const QString &login,
                                               const QString &role, bool remove, QString *error);
GithubHiddenCommand githubOrgPeopleCommand(const QString &org, const QString &login, const QString &email,
                                           const QString &invitationId, const QString &role, const QString &action,
                                           QString *error);
GithubHiddenCommand githubOrgRoleCommand(const QString &org, const QString &roleId, const QString &name,
                                         const QString &description, const QString &baseRole, const QString &permissions,
                                         bool remove, QString *error);
GithubHiddenCommand githubOrgPropertyCommand(const QString &org, const QString &name, const QString &valueType,
                                             const QString &description, const QString &allowedCsv, bool remove,
                                             QString *error);
GithubHiddenCommand githubOrgHookCommand(const QString &org, const QString &id, const QString &url, const QString &events,
                                         const QString &secret, bool active, bool remove, QString *error);
GithubHiddenCommand githubOrgBannerCommand(const QString &org, const QString &text, const QString &expiresAt,
                                           bool dismissible, bool remove, QString *error);
QStringList githubOrgRoleAssignArgv(const QString &org, const QString &login, const QString &roleId, bool remove,
                                    QString *error);
GithubHiddenCommand githubOrgSecurityAttachCommand(const QString &org, const QString &configId, const QString &repoIds,
                                                   QString *error);
GithubHiddenCommand githubOrgSecurityConfigCommand(const QString &org, const QString &name, const QString &dependabot,
                                                   const QString &secretScanning, const QString &pushProtection,
                                                   const QString &configId, bool remove, QString *error);
QStringList githubOrgSecurityManagerArgv(const QString &org, const QString &teamSlug, bool remove, QString *error);
GithubHiddenCommand githubOrgImmutableCommand(const QString &org, const QString &policy, const QString &repoIds,
                                              QString *error);
