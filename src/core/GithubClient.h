#pragma once

#include "GithubLifecycle.h"
#include "GithubPolicy.h"
#include "GitRunner.h"

#include <QDateTime>
#include <QObject>
#include <QQmlEngine>
#include <QVariantList>
#include <QVariantMap>

class RepoContext;

/// GitHub CLI state for the open repository: auth, origin, metadata, local scaffold.
class GithubClient : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("GithubClient is owned by App")

    Q_PROPERTY(bool available READ available NOTIFY changed)
    Q_PROPERTY(bool authenticated READ authenticated NOTIFY changed)
    Q_PROPERTY(QString login READ login NOTIFY changed)
    Q_PROPERTY(QString host READ host NOTIFY changed)
    Q_PROPERTY(bool hasOrigin READ hasOrigin NOTIFY changed)
    Q_PROPERTY(bool originIsGithub READ originIsGithub NOTIFY changed)
    Q_PROPERTY(QString originUrl READ originUrl NOTIFY changed)
    Q_PROPERTY(QString slug READ slug NOTIFY changed)
    Q_PROPERTY(QString description READ description NOTIFY changed)
    Q_PROPERTY(QString homepage READ homepage NOTIFY changed)
    Q_PROPERTY(QString visibility READ visibility NOTIFY changed)
    Q_PROPERTY(QString htmlUrl READ htmlUrl NOTIFY changed)
    Q_PROPERTY(QString repoName READ repoName NOTIFY changed)
    Q_PROPERTY(QString defaultBranch READ defaultBranch NOTIFY changed)
    Q_PROPERTY(QStringList topics READ topics NOTIFY changed)
    Q_PROPERTY(bool hasReadme READ hasReadme NOTIFY changed)
    Q_PROPERTY(bool hasLicense READ hasLicense NOTIFY changed)
    Q_PROPERTY(bool hasGitignore READ hasGitignore NOTIFY changed)
    Q_PROPERTY(bool hasLfsAttributes READ hasLfsAttributes NOTIFY changed)
    Q_PROPERTY(bool issuesEnabled READ issuesEnabled NOTIFY changed)
    Q_PROPERTY(bool wikiEnabled READ wikiEnabled NOTIFY changed)
    Q_PROPERTY(bool projectsEnabled READ projectsEnabled NOTIFY changed)
    Q_PROPERTY(bool discussionsEnabled READ discussionsEnabled NOTIFY changed)
    Q_PROPERTY(bool deleteBranchOnMerge READ deleteBranchOnMerge NOTIFY changed)
    Q_PROPERTY(bool isTemplate READ isTemplate NOTIFY changed)
    Q_PROPERTY(QVariantMap policy READ policy NOTIFY changed)
    Q_PROPERTY(QVariantMap pages READ pages NOTIFY changed)
    Q_PROPERTY(QStringList wikiPages READ wikiPages NOTIFY changed)
    Q_PROPERTY(QStringList emails READ emails NOTIFY changed)
    Q_PROPERTY(QString serviceDigest READ serviceDigest NOTIFY changed)
    Q_PROPERTY(QString statusLine READ statusLine NOTIFY changed)
    Q_PROPERTY(QString note READ note NOTIFY changed)
    Q_PROPERTY(QString suggestedName READ suggestedName NOTIFY changed)
    Q_PROPERTY(QVariantList checklist READ checklist NOTIFY changed)

public:
    explicit GithubClient(RepoContext *repo, QObject *parent = nullptr);

    bool available() const;
    bool authenticated() const;
    QString login() const;
    QString host() const;
    bool hasOrigin() const;
    bool originIsGithub() const;
    QString originUrl() const;
    QString slug() const;
    QString description() const;
    QString homepage() const;
    QString visibility() const;
    QString htmlUrl() const;
    QString repoName() const;
    QString defaultBranch() const;
    QStringList topics() const;
    bool hasReadme() const;
    bool hasLicense() const;
    bool hasGitignore() const;
    bool hasLfsAttributes() const;
    bool issuesEnabled() const;
    bool wikiEnabled() const;
    bool projectsEnabled() const;
    bool discussionsEnabled() const;
    bool deleteBranchOnMerge() const;
    bool isTemplate() const;
    QVariantMap policy() const;
    GithubPolicy policyState() const;
    QVariantMap pages() const;
    GithubPagesState pagesState() const;
    QStringList wikiPages() const;
    QStringList emails() const;
    QString statusLine() const;
    QString note() const;
    QString suggestedName() const;
    QVariantList checklist() const;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE QString dressText(const QString &kind) const;
    Q_INVOKABLE QString dressFileNameFor(const QString &kind) const;
    Q_INVOKABLE QString readmeTemplate(const QString &title, const QString &blurb) const;
    Q_INVOKABLE QString licenseTemplate(const QString &kind) const;
    Q_INVOKABLE QString dressTemplate(const QString &kind) const;
    Q_INVOKABLE QString fetchTemplate(const QString &kind, const QString &key);
    Q_INVOKABLE void refreshWiki();
    Q_INVOKABLE void refreshEmails();
    Q_INVOKABLE QString wikiPageText(const QString &page) const;
    Q_INVOKABLE void refreshServices();
    QString serviceDigest() const;
    QString projectNodeId(const QString &number, QString *error) const;
    void refreshIfStale(int seconds);
    /// Cheap local scan of README/LICENSE/.gitignore and other dress files (no gh API).
    void refreshLocalDress();
    /// Body of the last fetched catalog template when kind matches; clears the stash.
    QString takePendingTemplate(const QString &kind);
    void setNote(const QString &note);
    QString snapshotText() const;
    GitRunResult applyFileCommand(const QString &commandId, const QVariantMap &args);
    GitRunResult saveDressFile(const QString &kind, const QString &body);

signals:
    void changed();

private:
    RepoContext *m_repo = nullptr;
    bool m_available = false;
    bool m_authenticated = false;
    QString m_login;
    QString m_host;
    bool m_hasOrigin = false;
    bool m_originIsGithub = false;
    QString m_originUrl;
    QString m_slug;
    QString m_description;
    QString m_homepage;
    QString m_visibility;
    QString m_htmlUrl;
    QString m_repoName;
    QString m_defaultBranch;
    QStringList m_topics;
    bool m_hasReadme = false;
    bool m_hasLicense = false;
    bool m_hasGitignore = false;
    bool m_hasLfsAttributes = false;
    QVariantMap m_dressPresent;
    QString m_pendingTemplateKind;
    QString m_pendingTemplateBody;
    bool m_issues = false;
    bool m_wiki = false;
    bool m_projects = false;
    bool m_discussions = false;
    bool m_deleteBranchOnMerge = false;
    bool m_isTemplate = false;
    GithubPolicy m_policy;
    GithubPagesState m_pages;
    QStringList m_wikiPages;
    QStringList m_emails;
    QString m_serviceDigest;
    QString m_statusLine;
    QString m_note;
    QDateTime m_refreshedAt;
};
