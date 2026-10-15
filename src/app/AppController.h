#pragma once

#include "core/Advisor.h"
#include "core/CommandCatalog.h"
#include "core/FavoritesModel.h"
#include "core/GitRunner.h"
#include "core/GithubClient.h"
#include "core/HistoryModel.h"
#include "core/HunkListModel.h"
#include "core/LlmClient.h"
#include "core/PreviewEngine.h"
#include "core/RecentCommandsModel.h"
#include "core/RecentReposModel.h"
#include "core/RecommendationModel.h"
#include "core/RepoContext.h"

#include <QObject>
#include <QProcess>
#include <QQmlEngine>
#include <QString>
#include <QVariantMap>

class AppController : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_SINGLETON
    QML_NAMED_ELEMENT(App)

    Q_PROPERTY(RepoContext *repo READ repo CONSTANT)
    Q_PROPERTY(CommandCatalog *catalog READ catalog CONSTANT)
    Q_PROPERTY(HistoryModel *history READ history CONSTANT)
    Q_PROPERTY(Advisor *advisor READ advisor CONSTANT)
    Q_PROPERTY(LlmClient *llm READ llm CONSTANT)
    Q_PROPERTY(GithubClient *github READ github CONSTANT)
    Q_PROPERTY(RecommendationModel *recommendations READ recommendations CONSTANT)
    Q_PROPERTY(RecentReposModel *recentRepos READ recentRepos CONSTANT)
    Q_PROPERTY(FavoritesModel *favorites READ favorites CONSTANT)
    Q_PROPERTY(RecentCommandsModel *recentCommands READ recentCommands CONSTANT)
    Q_PROPERTY(HunkListModel *hunks READ hunks CONSTANT)
    Q_PROPERTY(bool paletteOpen READ paletteOpen WRITE setPaletteOpen NOTIFY paletteOpenChanged)
    Q_PROPERTY(bool branchDialogOpen READ branchDialogOpen WRITE setBranchDialogOpen NOTIFY branchDialogOpenChanged)
    Q_PROPERTY(bool tagDialogOpen READ tagDialogOpen WRITE setTagDialogOpen NOTIFY tagDialogOpenChanged)
    Q_PROPERTY(QString lastPreviewJson READ lastPreviewJson NOTIFY previewChanged)
    Q_PROPERTY(QString lastOutput READ lastOutput NOTIFY outputChanged)
    Q_PROPERTY(int lastExitCode READ lastExitCode NOTIFY outputChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(bool cancellable READ cancellable NOTIFY busyChanged)
    Q_PROPERTY(int commandCount READ commandCount CONSTANT)
    Q_PROPERTY(bool autoRefresh READ autoRefresh WRITE setAutoRefresh NOTIFY autoRefreshChanged)
    Q_PROPERTY(QString lastStatusDelta READ lastStatusDelta NOTIFY statusDeltaChanged)
    Q_PROPERTY(QString lessonRepoPath READ lessonRepoPath NOTIFY lessonChanged)

public:
    explicit AppController(QObject *parent = nullptr);

    static void setInstance(AppController *instance);
    static AppController *create(QQmlEngine *engine, QJSEngine *scriptEngine);

    RepoContext *repo();
    CommandCatalog *catalog();
    HistoryModel *history();
    Advisor *advisor();
    LlmClient *llm();
    GithubClient *github();
    RecommendationModel *recommendations();
    RecentReposModel *recentRepos();
    FavoritesModel *favorites();
    RecentCommandsModel *recentCommands();
    HunkListModel *hunks();
    bool paletteOpen() const;
    void setPaletteOpen(bool open);
    bool branchDialogOpen() const;
    void setBranchDialogOpen(bool open);
    bool tagDialogOpen() const;
    void setTagDialogOpen(bool open);
    QString lastPreviewJson() const;
    QString lastOutput() const;
    int lastExitCode() const;
    bool busy() const;
    bool cancellable() const;
    int commandCount() const;
    bool autoRefresh() const;
    void setAutoRefresh(bool enabled);
    QString lastStatusDelta() const;
    QString lessonRepoPath() const;

    Q_INVOKABLE bool openRepository(const QString &path);
    Q_INVOKABLE void refreshRepo();
    Q_INVOKABLE void previewCommand(const QString &commandId, const QVariantMap &args);
    Q_INVOKABLE void executeLastPreview(bool destructiveConfirmed);
    Q_INVOKABLE void cancelRunning();
    Q_INVOKABLE QString validateArgs(const QString &commandId, const QVariantMap &args) const;
    Q_INVOKABLE void askAdvice(const QString &question);
    Q_INVOKABLE void replyMentor(const QString &text);
    Q_INVOKABLE void answerAskUser(const QString &option);
    Q_INVOKABLE void enrichRecommendations();
    Q_INVOKABLE void probeLlm();
    Q_INVOKABLE void saveLlmSettings(const QString &baseUrl, const QString &model, const QString &apiKey);
    Q_INVOKABLE void loadFileDiff(const QString &path, bool staged, bool wordDiff = false);
    Q_INVOKABLE QString auditLogPath() const;
    Q_INVOKABLE void prepareCommand(const QString &commandId, const QVariantMap &args);
    /// Preview, then run. Destructive commands stop on the preview tab for confirmation.
    Q_INVOKABLE void runCommand(const QString &commandId, const QVariantMap &args, bool fromAdvisor);
    Q_INVOKABLE bool saveDressFile(const QString &kind, const QString &body);
    Q_INVOKABLE void saveGithubCard(const QString &description, const QString &homepage, const QString &visibility);
    Q_INVOKABLE void renameGithubRepo(const QString &name);
    Q_INVOKABLE void addGithubTopic(const QString &topic);
    Q_INVOKABLE void removeGithubTopic(const QString &topic);
    Q_INVOKABLE void saveGithubSettings(bool issues, bool wiki, bool projects, bool discussions, bool deleteOnMerge,
                                       bool asTemplate, const QString &defaultBranch);
    Q_INVOKABLE void saveGithubPolicy(const QVariantMap &draft);
    Q_INVOKABLE void setGithubArchived(bool archived);
    Q_INVOKABLE void forkGithubRepo(const QString &forkName, const QString &org, bool defaultBranchOnly,
                                   bool rewireOrigin);
    Q_INVOKABLE void transferGithubRepo(const QString &newOwner, const QString &newName, const QString &teamIds,
                                       const QString &typedName);
    Q_INVOKABLE void deleteGithubRepo(const QString &typedName);
    Q_INVOKABLE void setGithubLfs(bool enabled);
    Q_INVOKABLE QString fetchGithubTemplate(const QString &kind, const QString &key);
    Q_INVOKABLE void saveGithubPages(const QString &branch, const QString &path, const QString &cname, bool https,
                                    bool enabled);
    Q_INVOKABLE void publishWikiPage(const QString &page, const QString &body);
    Q_INVOKABLE void saveGithubIssue(const QString &number, const QString &title, const QString &body);
    Q_INVOKABLE void actGithubIssue(const QString &number, const QString &action, const QString &extra);
    Q_INVOKABLE void linkGithubIssue(const QString &number, const QString &kind, const QString &value, bool remove);
    Q_INVOKABLE void createGithubLabel(const QString &name, const QString &color, const QString &description);
    Q_INVOKABLE void editGithubLabel(const QString &name, const QString &color, const QString &description,
                                    const QString &newName);
    Q_INVOKABLE void deleteGithubLabel(const QString &name);
    Q_INVOKABLE void saveGithubMilestone(const QString &number, const QString &title, const QString &state);
    Q_INVOKABLE void saveGithubPull(const QString &number, const QString &title, const QString &body,
                                   const QString &base, const QString &head, bool draft);
    Q_INVOKABLE void reviewGithubPull(const QString &number, const QString &action, const QString &body);
    Q_INVOKABLE void mergeGithubPull(const QString &number, const QString &method);
    Q_INVOKABLE void readyGithubPull(const QString &number, bool draft);
    Q_INVOKABLE void updateGithubPullBranch(const QString &number, bool rebase);
    Q_INVOKABLE void saveGithubDiscussion(const QString &number, const QString &title, const QString &body,
                                         const QString &category);
    Q_INVOKABLE void commentGithubDiscussion(const QString &target, const QString &body, bool remove);
    Q_INVOKABLE void createGithubRelease(const QString &tag, const QString &title, const QString &notes, bool draft,
                                        bool prerelease, const QString &target);
    Q_INVOKABLE void editGithubRelease(const QString &tag, const QString &title, const QString &notes, bool draft,
                                      bool prerelease, const QString &target);
    Q_INVOKABLE void deleteGithubRelease(const QString &tag, bool cleanupTag);
    Q_INVOKABLE void uploadGithubRelease(const QString &tag, const QString &filePath);
    Q_INVOKABLE void createGithubAutolink(const QString &prefix, const QString &urlTemplate, bool numeric);
    Q_INVOKABLE void deleteGithubAutolink(const QString &id);
    Q_INVOKABLE void createGithubProject(const QString &title);
    Q_INVOKABLE void createGithubProjectField(const QString &number, const QString &name, const QString &dataType,
                                             const QString &options);
    Q_INVOKABLE void createGithubProjectItem(const QString &number, const QString &title, const QString &body);
    Q_INVOKABLE void addGithubProjectItem(const QString &number, const QString &url);
    Q_INVOKABLE void editGithubProjectItem(const QString &number, const QString &url, const QString &field,
                                          const QString &value);
    Q_INVOKABLE void linkGithubProject(const QString &number);
    Q_INVOKABLE void createGithubProjectView(const QString &number, const QString &name, const QString &layout);
    Q_INVOKABLE void editGithubProjectView(const QString &number, const QString &viewId, const QString &name, bool remove);
    Q_INVOKABLE void saveGithubCollaborator(const QString &login, const QString &permission, bool remove);
    Q_INVOKABLE void deleteGithubInvitation(const QString &id);
    Q_INVOKABLE void acceptGithubInvitation(const QString &id);
    Q_INVOKABLE void actGithubWorkflow(const QString &action, const QString &workflow, const QString &ref,
                                      const QString &inputKey, const QString &inputValue);
    Q_INVOKABLE void actGithubRun(const QString &action, const QString &runId, bool failedOnly);
    Q_INVOKABLE void saveGithubVariable(const QString &name, const QString &value, const QString &envName, bool remove);
    Q_INVOKABLE void saveGithubSecret(const QString &name, const QString &value, const QString &envName,
                                     const QString &app, bool remove);
    Q_INVOKABLE void saveGithubEnvironment(const QString &name, const QString &waitMinutes, bool protectedBranches,
                                          const QString &reviewers, bool remove);
    Q_INVOKABLE void deleteGithubCache(const QString &keyOrId);
    Q_INVOKABLE void saveGithubAlert(const QString &kind, const QString &number, const QString &state,
                                    const QString &reason);
    Q_INVOKABLE void saveGithubRuleset(const QString &id, const QString &name, const QString &includeRef,
                                      const QString &enforcement, const QString &rules);
    Q_INVOKABLE void saveGithubBranchProtection(const QString &branch, int reviews, bool enforceAdmins, bool blockForce,
                                               bool blockDelete, bool remove);
    Q_INVOKABLE void saveGithubDeployKey(const QString &title, const QString &key, bool readOnly);
    Q_INVOKABLE void deleteGithubDeployKey(const QString &id);
    Q_INVOKABLE void saveGithubWebhook(const QString &id, const QString &url, const QString &events,
                                      const QString &secret, bool active, bool remove);
    Q_INVOKABLE void saveGithubAdvisory(const QString &ghsa, const QString &summary, const QString &description,
                                       const QString &severity, const QString &ecosystem, const QString &packageName,
                                       const QString &versionRange);
    Q_INVOKABLE void saveGithubInteraction(const QString &limit, bool clear);
    Q_INVOKABLE void createGithubCodespace(const QString &branch, const QString &displayName);
    Q_INVOKABLE void editGithubCodespace(const QString &name, const QString &displayName);
    Q_INVOKABLE void deleteGithubCodespace(const QString &name);
    Q_INVOKABLE void saveGithubProfile(const QString &name, const QString &bio, const QString &blog, const QString &company,
                                      const QString &location, const QString &twitter, bool hireable);
    Q_INVOKABLE void saveGithubEmail(const QString &email, const QString &action);
    Q_INVOKABLE void saveGithubSocial(const QString &url, bool remove);
    Q_INVOKABLE void saveGithubUserKey(const QString &kind, const QString &title, const QString &key, const QString &id,
                                      bool remove);
    Q_INVOKABLE void createGithubProfileRepo(const QString &login);
    Q_INVOKABLE void saveGithubProfileReadme(const QString &login, const QString &text, const QString &sha);
    Q_INVOKABLE void saveGithubOrgCard(const QString &org, const QString &name, const QString &description, const QString &blog,
                                      const QString &email, const QString &twitter, const QString &location,
                                      const QString &company);
    Q_INVOKABLE void saveGithubOrgPolicy(const QString &org, const QString &permission, bool publicRepos, bool privateRepos,
                                        bool internalRepos, bool pages, bool publicPages, bool privatePages, bool forkPrivate);
    Q_INVOKABLE void saveGithubOrgTeam(const QString &org, const QString &name, const QString &description,
                                      const QString &privacy, const QString &slug, bool remove);
    Q_INVOKABLE void saveGithubOrgTeamMember(const QString &org, const QString &slug, const QString &login, const QString &role,
                                            bool remove);
    Q_INVOKABLE void saveGithubOrgPeople(const QString &org, const QString &login, const QString &email,
                                        const QString &invitationId, const QString &role, const QString &action);
    Q_INVOKABLE void saveGithubOrgRole(const QString &org, const QString &roleId, const QString &name, const QString &description,
                                      const QString &baseRole, const QString &permissions, bool remove);
    Q_INVOKABLE void saveGithubOrgProperty(const QString &org, const QString &name, const QString &valueType,
                                          const QString &description, const QString &allowedCsv, bool remove);
    Q_INVOKABLE void saveGithubOrgHook(const QString &org, const QString &id, const QString &url, const QString &events,
                                      const QString &secret, bool active, bool remove);
    Q_INVOKABLE void saveGithubOrgBanner(const QString &org, const QString &text, const QString &expiresAt, bool dismissible,
                                      bool remove);
    Q_INVOKABLE void assignGithubOrgRole(const QString &org, const QString &login, const QString &roleId, bool remove);
    Q_INVOKABLE void attachGithubOrgSecurity(const QString &org, const QString &configId, const QString &repoIds);
    Q_INVOKABLE void saveGithubOrgSecurityConfig(const QString &org, const QString &name, const QString &dependabot,
                                                const QString &secretScanning, const QString &pushProtection,
                                                const QString &configId, bool remove);
    Q_INVOKABLE void saveGithubOrgSecurityManager(const QString &org, const QString &teamSlug, bool remove);
    Q_INVOKABLE void saveGithubOrgImmutable(const QString &org, const QString &policy, const QString &repoIds);
    Q_INVOKABLE void loadCommitShow(const QString &hash);
    Q_INVOKABLE void loadStashShow(const QString &ref);
    Q_INVOKABLE void loadFileBlame(const QString &path);
    Q_INVOKABLE void loadFileHistory(const QString &path);
    Q_INVOKABLE void loadFileAttributes(const QString &path);
    Q_INVOKABLE void loadCleanPreview();
    Q_INVOKABLE void copyText(const QString &text);
    Q_INVOKABLE void copyPath(const QString &relativePath);
    Q_INVOKABLE void revealInFolder(const QString &relativePath);
    Q_INVOKABLE void openInEditor(const QString &relativePath);
    Q_INVOKABLE void openTerminalInRepo();
    Q_INVOKABLE QString suggestCommitMessage();
    /// Local user.name / user.email for the open repo (mentor + catalog config_set_user).
    Q_INVOKABLE bool saveGitUser(const QString &name, const QString &email, bool fromAdvisor = false);
    Q_INVOKABLE QVariantMap gitUserIdentity() const;
    Q_INVOKABLE bool appendGitignore(const QString &relativePath);
    Q_INVOKABLE void openTagDialog();
    Q_INVOKABLE void closeTagDialog();
    Q_INVOKABLE void toggleFavorite(const QString &commandId);
    Q_INVOKABLE void loadUnstagedHunks(const QString &path = QString());
    Q_INVOKABLE void loadStagedHunks(const QString &path = QString());
    Q_INVOKABLE bool stageSelectedHunks();
    Q_INVOKABLE bool unstageSelectedHunks();
    Q_INVOKABLE bool discardSelectedHunks();
    Q_INVOKABLE QString exportSelectedHunksPatch();
    Q_INVOKABLE QVariantMap loadUiState() const;
    Q_INVOKABLE void saveUiState(const QVariantMap &state);
    Q_INVOKABLE void replayHistory(int row);
    Q_INVOKABLE bool createLessonRepository();
    Q_INVOKABLE bool openLessonRepository();
    Q_INVOKABLE bool cloneRepository(const QString &url, const QString &destDir);
    Q_INVOKABLE bool initRepository(const QString &path, const QString &branchName = QStringLiteral("main"),
                                    bool withReadme = true);
    Q_INVOKABLE void compareCommits(const QString &a, const QString &b);
    Q_INVOKABLE void setCompareEndpoint(const QString &hash, bool asA);
    Q_INVOKABLE QString exportSessionReport();
    Q_INVOKABLE void openPalette();
    Q_INVOKABLE void closePalette();
    Q_INVOKABLE void openBranchDialog();
    Q_INVOKABLE void closeBranchDialog();

    /// Headless health check for CI / приёмка.
    QString runSelfCheck() const;
    QString dumpRepositoryStatus(const QString &path);
    /// Headless integration: hunk stage + lesson sandbox (no GUI).
    QString runIntegrationTest();

signals:
    void previewChanged();
    void outputChanged();
    void busyChanged();
    void autoRefreshChanged();
    void statusDeltaChanged();
    void lessonChanged();
    void paletteOpenChanged();
    void branchDialogOpenChanged();
    void tagDialogOpenChanged();
    void errorOccurred(const QString &message);
    void infoOccurred(const QString &message);
    void selectCommandRequested(const QString &commandId, const QVariantMap &args);
    /// 3 = preview (needs confirm), 7 = output after the command was started.
    void commandRunRequested(int tabIndex);

private:
    void finishExecute(const GitRunResult &run, const QString &cmdline, const QString &repo,
                       int dirtyBefore, int stagedBefore, const QString &branchBefore);
    bool ensureGithubRemote(QString *error) const;
    void beginGh(const QString &commandId, const QStringList &argv, DangerLevel danger = DangerLevel::Mutates,
                 const QString &narrative = {}, const QString &program = QStringLiteral("gh"),
                 const QString &stdinText = {});
    void dispatchGithub(const QString &commandId, const QStringList &argv, const QString &error, DangerLevel danger,
                        const QString &narrative, const QString &stdinText = {});
    void dispatchGithubDesk(const QString &id, const QVariantMap &args);
    QString githubProjectOwner() const;
    static QString normalizeTopic(const QString &topic, QString *error);

    static AppController *s_instance;

    RepoContext m_repo;
    CommandCatalog m_catalog;
    HistoryModel m_history;
    LlmClient m_llm;
    GithubClient m_github;
    Advisor m_advisor;
    RecentReposModel m_recent;
    FavoritesModel m_favorites;
    RecentCommandsModel m_recentCommands;
    HunkListModel m_hunks;
    bool m_paletteOpen = false;
    bool m_branchDialogOpen = false;
    bool m_tagDialogOpen = false;
    GitRunner m_runner;
    PreviewEngine m_preview;
    QProcess *m_gitProcess = nullptr;
    QString m_lastPreviewJson;
    QString m_lastOutput;
    int m_lastExitCode = 0;
    QString m_confirmToken;
    QStringList m_pendingArgv;
    QString m_pendingCmdline;
    QString m_pendingCommandId;
    QString m_pendingProgram;
    QVariantMap m_pendingArgs;
    QString m_pendingStdin;
    QString m_execStdin;
    DangerLevel m_pendingDanger = DangerLevel::Safe;
    bool m_busy = false;
    bool m_autoRefresh = true;
    QString m_lastStatusDelta;
    QString m_lessonRepoPath;
    int m_execDirtyBefore = 0;
    int m_execStagedBefore = 0;
    QString m_execBranchBefore;
    QString m_execCmdline;
    QString m_execRepo;
    QString m_execCommandId;
    QString m_execProgram;
    QString m_execArgsJson;
    bool m_pendingFromAdvisor = false;
    bool m_deskFromAdvisor = false;
    bool m_execFromAdvisor = false;
    bool m_pendingStay = false;
    bool m_execStay = false;
};
