#pragma once

#include "GitRunner.h"

#include <QJsonObject>
#include <QString>
#include <QStringList>

struct GithubForkRequest {
    QString slug;
    QString forkName;
    QString org;
    bool defaultBranchOnly = false;
    bool rewireOrigin = false;
};

/// `gh` argv for a fork. Rewiring origin omits the slug so `gh` edits the current checkout.
QStringList githubForkArgv(const GithubForkRequest &request, QString *error);

/// `gh repo delete <slug> --yes`. The typed name must match the repository name exactly.
QStringList githubDeleteArgv(const QString &slug, const QString &repoName, const QString &typedName, QString *error);

/// `gh api` fields for POST /repos/{owner}/{repo}/transfer.
QStringList githubTransferArgv(const QString &currentOwner, const QString &repoName, const QString &typedName,
                               const QString &newOwner, const QString &newName, const QString &teamIds, QString *error);

/// `gh api` argv that enables or disables Git LFS on the current repository.
QStringList githubLfsArgv(bool enable);

/// `gh repo gitignore view` or `gh repo license view`. Empty argv when the key is unsafe.
QStringList githubTemplateArgv(const QString &kind, const QString &key, QString *error);

struct GithubPagesState {
    bool known = false;
    bool enabled = false;
    QString branch;
    QString path;
    QString cname;
    bool https = false;
    bool httpsKnown = false;
    QString buildType;
    QString htmlUrl;
};

GithubPagesState githubPagesFromApi(int httpCode, const QJsonObject &body);
QStringList githubPagesArgv(const GithubPagesState &want, const GithubPagesState &have, QString *error);

QString githubWikiRemoteUrl(const QString &originUrl);
bool validWikiPageName(const QString &name);

/// Clone or update a wiki checkout, write one page, commit and push. `wikiUrl` may be a local bare repo in tests.
GitRunResult publishWikiPage(const QString &authorRepo, const QString &wikiUrl, const QString &cacheDir,
                             const QString &page, const QString &body);
GitRunResult prepareWikiCheckout(const QString &wikiUrl, const QString &cacheDir);
QString githubWikiCacheDir(const QString &slug);
QStringList listWikiPages(const QString &cacheDir);
QString readWikiPage(const QString &cacheDir, const QString &page);
