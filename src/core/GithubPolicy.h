#pragma once

#include <QJsonObject>
#include <QString>
#include <QStringList>
#include <QVariantMap>

/// One repository setting as read from GitHub. Unknown means the editor must not write it.
struct GithubFlag {
    bool known = false;
    bool value = false;
};

/// Repository policy that GHEW can read from the REST repo object and write back.
struct GithubPolicy {
    GithubFlag mergeCommit;
    GithubFlag squashMerge;
    GithubFlag rebaseMerge;
    GithubFlag autoMerge;
    GithubFlag updateBranch;
    GithubFlag allowForking;
    GithubFlag webSignoff;
    GithubFlag pullRequests;
    GithubFlag advancedSecurity;
    GithubFlag secretScanning;
    GithubFlag pushProtection;
    GithubFlag archived;

    bool squashTitleKnown = false;
    QString squashTitle;
    bool squashBodyKnown = false;
    QString squashBody;
    bool mergeTitleKnown = false;
    QString mergeTitle;
    bool mergeBodyKnown = false;
    QString mergeBody;
    bool prPolicyKnown = false;
    QString prPolicy;

    bool loaded = false;
};

GithubPolicy githubPolicyFromApi(const QJsonObject &repo);
void githubPolicyFillFromView(GithubPolicy *policy, const QJsonObject &view);
QVariantMap githubPolicyToMap(const GithubPolicy &policy);
GithubPolicy githubPolicyFromDraft(const QVariantMap &draft);

/// `-F` / `-f` pairs for `gh api --method PATCH`. Empty when nothing changed.
/// `error` is set when the combination cannot be sent.
QStringList githubPolicyPatchArgs(const GithubPolicy &want, const GithubPolicy &have, QString *error);
