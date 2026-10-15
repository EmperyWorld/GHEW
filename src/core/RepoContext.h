#pragma once

#include "BranchListModel.h"
#include "CommitLogModel.h"
#include "ReflogListModel.h"
#include "StashListModel.h"
#include "StatusFileModel.h"
#include "RemoteListModel.h"
#include "TagListModel.h"
#include "WorktreeListModel.h"
#include "SubmoduleListModel.h"
#include "HookListModel.h"

#include <QFileSystemWatcher>
#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QTimer>

class RepoContext : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("RepoContext is owned by App")

    Q_PROPERTY(bool isOpen READ isOpen NOTIFY changed)
    Q_PROPERTY(QString rootPath READ rootPath NOTIFY changed)
    Q_PROPERTY(QString branch READ branch NOTIFY changed)
    Q_PROPERTY(bool detachedHead READ detachedHead NOTIFY changed)
    Q_PROPERTY(bool bare READ bare NOTIFY changed)
    Q_PROPERTY(bool insideSubmodule READ insideSubmodule NOTIFY changed)
    Q_PROPERTY(bool bisectActive READ bisectActive NOTIFY changed)
    /// merge | rebase | cherry-pick | revert | empty
    Q_PROPERTY(QString operationKind READ operationKind NOTIFY changed)
    Q_PROPERTY(int conflictCount READ conflictCount NOTIFY changed)
    Q_PROPERTY(QString shortStatus READ shortStatus NOTIFY changed)
    Q_PROPERTY(QString lastError READ lastError NOTIFY changed)

    Q_PROPERTY(QString upstream READ upstream NOTIFY changed)
    Q_PROPERTY(bool hasUpstream READ hasUpstream NOTIFY changed)
    Q_PROPERTY(int ahead READ ahead NOTIFY changed)
    Q_PROPERTY(int behind READ behind NOTIFY changed)
    Q_PROPERTY(int localCommits READ localCommits NOTIFY changed)
    Q_PROPERTY(int remoteCommits READ remoteCommits NOTIFY changed)
    Q_PROPERTY(int stagedCount READ stagedCount NOTIFY changed)
    Q_PROPERTY(int unstagedCount READ unstagedCount NOTIFY changed)
    Q_PROPERTY(int untrackedCount READ untrackedCount NOTIFY changed)
    Q_PROPERTY(int dirtyCount READ dirtyCount NOTIFY changed)
    Q_PROPERTY(QString diffStat READ diffStat NOTIFY changed)
    Q_PROPERTY(QString lastCommitSubject READ lastCommitSubject NOTIFY changed)
    Q_PROPERTY(StatusFileModel *files READ files CONSTANT)
    Q_PROPERTY(BranchListModel *branches READ branches CONSTANT)
    Q_PROPERTY(CommitLogModel *commits READ commits CONSTANT)
    Q_PROPERTY(StashListModel *stashes READ stashes CONSTANT)
    Q_PROPERTY(TagListModel *tags READ tags CONSTANT)
    Q_PROPERTY(RemoteListModel *remotes READ remotes CONSTANT)
    Q_PROPERTY(WorktreeListModel *worktrees READ worktrees CONSTANT)
    Q_PROPERTY(SubmoduleListModel *submodules READ submodules CONSTANT)
    Q_PROPERTY(HookListModel *hooks READ hooks CONSTANT)
    Q_PROPERTY(ReflogListModel *reflog READ reflog CONSTANT)
    Q_PROPERTY(QString selectedDiff READ selectedDiff NOTIFY diffChanged)
    Q_PROPERTY(QString selectedDiffPath READ selectedDiffPath NOTIFY diffChanged)
    Q_PROPERTY(bool selectedDiffStaged READ selectedDiffStaged NOTIFY diffChanged)
    Q_PROPERTY(QString inspectText READ inspectText NOTIFY inspectChanged)
    Q_PROPERTY(QString inspectTitle READ inspectTitle NOTIFY inspectChanged)
    Q_PROPERTY(QString inspectKind READ inspectKind NOTIFY inspectChanged)
    Q_PROPERTY(QString inspectTarget READ inspectTarget NOTIFY inspectChanged)
    Q_PROPERTY(QString compareA READ compareA NOTIFY compareChanged)
    Q_PROPERTY(QString compareB READ compareB NOTIFY compareChanged)
    Q_PROPERTY(bool watchEnabled READ watchEnabled WRITE setWatchEnabled NOTIFY watchEnabledChanged)

public:
    explicit RepoContext(QObject *parent = nullptr);

    bool isOpen() const;
    QString rootPath() const;
    QString branch() const;
    bool detachedHead() const;
    bool bare() const;
    bool insideSubmodule() const;
    bool bisectActive() const;
    QString operationKind() const;
    int conflictCount() const;
    QString shortStatus() const;
    QString lastError() const;

    QString upstream() const;
    bool hasUpstream() const;
    int ahead() const;
    int behind() const;
    int localCommits() const;
    int remoteCommits() const;
    int stagedCount() const;
    int unstagedCount() const;
    int untrackedCount() const;
    int dirtyCount() const;
    QString diffStat() const;
    QString lastCommitSubject() const;
    StatusFileModel *files();
    BranchListModel *branches();
    CommitLogModel *commits();
    StashListModel *stashes();
    TagListModel *tags();
    RemoteListModel *remotes();
    WorktreeListModel *worktrees();
    SubmoduleListModel *submodules();
    HookListModel *hooks();
    ReflogListModel *reflog();
    QString selectedDiff() const;
    QString selectedDiffPath() const;
    bool selectedDiffStaged() const;
    QString inspectText() const;
    QString inspectTitle() const;
    QString inspectKind() const;
    QString inspectTarget() const;
    QString compareA() const;
    QString compareB() const;
    bool watchEnabled() const;
    void setWatchEnabled(bool enabled);

    bool open(const QString &path);
    bool refresh();
    void close();

    Q_INVOKABLE bool loadFileDiff(const QString &path, bool staged, bool wordDiff = false);
    Q_INVOKABLE void clearFileDiff();
    Q_INVOKABLE bool loadCommitShow(const QString &hash);
    Q_INVOKABLE bool loadStashShow(const QString &ref);
    Q_INVOKABLE bool loadFileBlame(const QString &path);
    Q_INVOKABLE bool loadFileHistory(const QString &path);
    Q_INVOKABLE bool loadFileAttributes(const QString &path);
    Q_INVOKABLE bool loadCleanPreview();
    Q_INVOKABLE void clearInspect();
    Q_INVOKABLE void setCompareEndpoint(const QString &hash, bool asA);
    Q_INVOKABLE bool loadCompareDiff();
    Q_INVOKABLE void clearCompare();

    QString dumpStatusJson() const;

signals:
    void changed();
    void diffChanged();
    void inspectChanged();
    void compareChanged();
    void watchEnabledChanged();
    void externalChangeDetected();

private:
    void clearState();
    void parsePorcelain(const QString &porcelain);
    void parseBranches(const QString &localOut, const QString &remoteOut);
    void parseCommits(const QString &logOut);
    void parseStashes(const QString &stashOut);
    void parseTags(const QString &tagOut);
    void parseRemotes(const QString &remoteOut);
    void parseWorktrees(const QString &worktreeOut);
    void parseSubmodules(const QString &submoduleOut);
    void loadHooks();
    void parseReflog(const QString &reflogOut);
    void setupWatcher();
    void clearWatcher();

    bool m_open = false;
    QString m_rootPath;
    QString m_branch;
    bool m_detachedHead = false;
    bool m_bare = false;
    bool m_insideSubmodule = false;
    bool m_bisectActive = false;
    QString m_operationKind;
    int m_conflictCount = 0;
    QString m_shortStatus;
    QString m_lastError;
    QString m_upstream;
    bool m_hasUpstream = false;
    int m_ahead = 0;
    int m_behind = 0;
    int m_localCommits = 0;
    int m_remoteCommits = 0;
    int m_stagedCount = 0;
    int m_unstagedCount = 0;
    int m_untrackedCount = 0;
    QString m_diffStat;
    QString m_lastCommitSubject;
    StatusFileModel m_files;
    BranchListModel m_branches;
    CommitLogModel m_commits;
    StashListModel m_stashes;
    TagListModel m_tags;
    RemoteListModel m_remotes;
    WorktreeListModel m_worktrees;
    SubmoduleListModel m_submodules;
    HookListModel m_hooks;
    ReflogListModel m_reflog;
    QString m_selectedDiff;
    QString m_selectedDiffPath;
    bool m_selectedDiffStaged = false;
    QString m_inspectText;
    QString m_inspectTitle;
    QString m_inspectKind;
    QString m_inspectTarget;
    QString m_compareA;
    QString m_compareB;
    QFileSystemWatcher m_watcher;
    QTimer m_watchDebounce;
    bool m_watchEnabled = true;
    bool m_refreshing = false;
};
