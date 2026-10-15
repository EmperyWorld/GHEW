#include "RepoContext.h"

#include "GitRunner.h"
#include "GitStatusParse.h"

#include <QDir>
#include <QFileInfo>
#include <QHash>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>
#include <QSettings>

#include <algorithm>

namespace {

int parseCount(const GitRunResult &r)
{
    if (!r.ok)
        return 0;
    bool ok = false;
    const int n = QString::fromUtf8(r.stdoutBytes).trimmed().toInt(&ok);
    return ok ? n : 0;
}

} // namespace

RepoContext::RepoContext(QObject *parent)
    : QObject(parent)
{
    // Models are value members — do not parent to `this` (avoids double-delete).
    QSettings settings;
    m_watchEnabled = settings.value(QStringLiteral("ui/watchFs"), true).toBool();

    m_watchDebounce.setSingleShot(true);
    m_watchDebounce.setInterval(450);
    connect(&m_watchDebounce, &QTimer::timeout, this, [this]() {
        if (!m_open || m_refreshing || !m_watchEnabled)
            return;
        emit externalChangeDetected();
    });
    connect(&m_watcher, &QFileSystemWatcher::fileChanged, this, [this](const QString &) {
        if (m_watchEnabled && !m_refreshing)
            m_watchDebounce.start();
    });
    connect(&m_watcher, &QFileSystemWatcher::directoryChanged, this, [this](const QString &) {
        if (m_watchEnabled && !m_refreshing)
            m_watchDebounce.start();
    });
}

bool RepoContext::watchEnabled() const
{
    return m_watchEnabled;
}

void RepoContext::setWatchEnabled(bool enabled)
{
    if (m_watchEnabled == enabled)
        return;
    m_watchEnabled = enabled;
    QSettings settings;
    settings.setValue(QStringLiteral("ui/watchFs"), m_watchEnabled);
    if (m_watchEnabled && m_open)
        setupWatcher();
    else
        clearWatcher();
    emit watchEnabledChanged();
}

void RepoContext::clearWatcher()
{
    const QStringList files = m_watcher.files();
    const QStringList dirs = m_watcher.directories();
    if (!files.isEmpty())
        m_watcher.removePaths(files);
    if (!dirs.isEmpty())
        m_watcher.removePaths(dirs);
}

void RepoContext::setupWatcher()
{
    clearWatcher();
    if (!m_open || !m_watchEnabled || m_rootPath.isEmpty())
        return;
    const QString gitDir = m_rootPath + QStringLiteral("/.git");
    m_watcher.addPath(m_rootPath);
    if (QFileInfo::exists(gitDir)) {
        m_watcher.addPath(gitDir);
        const QString head = gitDir + QStringLiteral("/HEAD");
        const QString index = gitDir + QStringLiteral("/index");
        if (QFileInfo::exists(head))
            m_watcher.addPath(head);
        if (QFileInfo::exists(index))
            m_watcher.addPath(index);
    }
}

bool RepoContext::isOpen() const
{
    return m_open;
}

QString RepoContext::rootPath() const
{
    return m_rootPath;
}

QString RepoContext::branch() const
{
    return m_branch;
}

bool RepoContext::detachedHead() const
{
    return m_detachedHead;
}

bool RepoContext::bare() const
{
    return m_bare;
}

bool RepoContext::insideSubmodule() const
{
    return m_insideSubmodule;
}

bool RepoContext::bisectActive() const
{
    return m_bisectActive;
}

QString RepoContext::operationKind() const
{
    return m_operationKind;
}

int RepoContext::conflictCount() const
{
    return m_conflictCount;
}

QString RepoContext::shortStatus() const
{
    return m_shortStatus;
}

QString RepoContext::lastError() const
{
    return m_lastError;
}

QString RepoContext::upstream() const
{
    return m_upstream;
}

bool RepoContext::hasUpstream() const
{
    return m_hasUpstream;
}

int RepoContext::ahead() const
{
    return m_ahead;
}

int RepoContext::behind() const
{
    return m_behind;
}

int RepoContext::localCommits() const
{
    return m_localCommits;
}

int RepoContext::remoteCommits() const
{
    return m_remoteCommits;
}

int RepoContext::stagedCount() const
{
    return m_stagedCount;
}

int RepoContext::unstagedCount() const
{
    return m_unstagedCount;
}

int RepoContext::untrackedCount() const
{
    return m_untrackedCount;
}

int RepoContext::dirtyCount() const
{
    return m_files.rowCount();
}

QString RepoContext::diffStat() const
{
    return m_diffStat;
}

QString RepoContext::lastCommitSubject() const
{
    return m_lastCommitSubject;
}

StatusFileModel *RepoContext::files()
{
    return &m_files;
}

BranchListModel *RepoContext::branches()
{
    return &m_branches;
}

CommitLogModel *RepoContext::commits()
{
    return &m_commits;
}

StashListModel *RepoContext::stashes()
{
    return &m_stashes;
}

TagListModel *RepoContext::tags()
{
    return &m_tags;
}

RemoteListModel *RepoContext::remotes()
{
    return &m_remotes;
}

WorktreeListModel *RepoContext::worktrees()
{
    return &m_worktrees;
}

SubmoduleListModel *RepoContext::submodules()
{
    return &m_submodules;
}

HookListModel *RepoContext::hooks()
{
    return &m_hooks;
}

ReflogListModel *RepoContext::reflog()
{
    return &m_reflog;
}

QString RepoContext::selectedDiff() const
{
    return m_selectedDiff;
}

QString RepoContext::selectedDiffPath() const
{
    return m_selectedDiffPath;
}

bool RepoContext::selectedDiffStaged() const
{
    return m_selectedDiffStaged;
}

QString RepoContext::inspectText() const
{
    return m_inspectText;
}

QString RepoContext::inspectTitle() const
{
    return m_inspectTitle;
}

QString RepoContext::inspectKind() const
{
    return m_inspectKind;
}

QString RepoContext::inspectTarget() const
{
    return m_inspectTarget;
}

QString RepoContext::compareA() const
{
    return m_compareA;
}

QString RepoContext::compareB() const
{
    return m_compareB;
}

void RepoContext::setCompareEndpoint(const QString &hash, bool asA)
{
    const QString h = hash.trimmed();
    if (h.isEmpty())
        return;
    if (asA)
        m_compareA = h;
    else
        m_compareB = h;
    emit compareChanged();
}

bool RepoContext::loadCompareDiff()
{
    if (!m_open || m_compareA.isEmpty() || m_compareB.isEmpty())
        return false;
    GitRunner runner;
    const GitRunResult r = runner.run(m_rootPath,
                                      {QStringLiteral("diff"), QStringLiteral("--stat"),
                                       QStringLiteral("--patch"), m_compareA, m_compareB});
    m_selectedDiffPath = QStringLiteral("%1…%2").arg(m_compareA.left(7), m_compareB.left(7));
    m_selectedDiffStaged = false;
    m_selectedDiff = QString::fromUtf8(r.stdoutBytes);
    if (m_selectedDiff.trimmed().isEmpty())
        m_selectedDiff = r.error.isEmpty()
                             ? tr("(нет отличий между коммитами)")
                             : r.error;
    emit diffChanged();
    return true;
}

void RepoContext::clearCompare()
{
    if (m_compareA.isEmpty() && m_compareB.isEmpty())
        return;
    m_compareA.clear();
    m_compareB.clear();
    emit compareChanged();
}

void RepoContext::clearState()
{
    m_branch.clear();
    m_detachedHead = false;
    m_bare = false;
    m_insideSubmodule = false;
    m_bisectActive = false;
    m_operationKind.clear();
    m_conflictCount = 0;
    m_compareA.clear();
    m_compareB.clear();
    m_shortStatus.clear();
    m_upstream.clear();
    m_hasUpstream = false;
    m_ahead = 0;
    m_behind = 0;
    m_localCommits = 0;
    m_remoteCommits = 0;
    m_stagedCount = 0;
    m_unstagedCount = 0;
    m_untrackedCount = 0;
    m_diffStat.clear();
    m_lastCommitSubject.clear();
    m_files.clear();
    m_branches.clear();
    m_commits.clear();
    m_stashes.clear();
    m_tags.clear();
    m_remotes.clear();
    m_worktrees.clear();
    m_submodules.clear();
    m_hooks.clear();
    m_reflog.clear();
    m_selectedDiff.clear();
    m_selectedDiffPath.clear();
    m_selectedDiffStaged = false;
    m_inspectText.clear();
    m_inspectTitle.clear();
    m_inspectKind.clear();
    m_inspectTarget.clear();
    emit diffChanged();
    emit inspectChanged();
}

void RepoContext::close()
{
    clearWatcher();
    m_open = false;
    m_rootPath.clear();
    m_lastError.clear();
    clearState();
    emit changed();
}

bool RepoContext::open(const QString &path)
{
    m_lastError.clear();
    const QFileInfo info(path);
    if (!info.exists() || !info.isDir()) {
        m_lastError = tr("Путь не существует или это не каталог.");
        m_open = false;
        clearState();
        emit changed();
        return false;
    }

    GitRunner runner;
    const GitRunResult top = runner.run(info.absoluteFilePath(),
                                        {QStringLiteral("rev-parse"), QStringLiteral("--show-toplevel")});
    if (!top.ok) {
        m_lastError = tr("Это не git-репозиторий (или git недоступен): %1").arg(top.error);
        m_open = false;
        clearState();
        emit changed();
        return false;
    }

    m_rootPath = QString::fromUtf8(top.stdoutBytes).trimmed();
    m_open = true;
    return refresh();
}

void RepoContext::parsePorcelain(const QString &porcelain)
{
    const PorcelainParseResult parsed = parsePorcelainStatus(porcelain);
    m_stagedCount = parsed.stagedCount;
    m_unstagedCount = parsed.unstagedCount;
    m_untrackedCount = parsed.untrackedCount;
    m_conflictCount = parsed.conflictCount;
    m_files.setFiles(parsed.files);
}

void RepoContext::parseBranches(const QString &localOut, const QString &remoteOut)
{
    QVector<BranchEntry> list;
    const auto parseLocal = [&](const QString &text) {
        for (const QString &raw : text.split(QLatin1Char('\n'), Qt::SkipEmptyParts)) {
            BranchEntry e;
            QString line = raw.trimmed();
            e.current = line.startsWith(QLatin1Char('*'));
            if (e.current)
                line = line.mid(1).trimmed();
            // "main" or "main abc1234 [origin/main: ahead 1] message"
            static const QRegularExpression re(
                QStringLiteral(R"(^(\S+)\s+[0-9a-f]+\s*(?:\[([^\]]+)\])?)"));
            const auto m = re.match(line);
            if (m.hasMatch()) {
                e.name = m.captured(1);
                const QString track = m.captured(2);
                if (!track.isEmpty()) {
                    e.upstream = track.split(QLatin1Char(':')).value(0).trimmed();
                    static const QRegularExpression aheadRe(QStringLiteral(R"(ahead\s+(\d+))"));
                    static const QRegularExpression behindRe(QStringLiteral(R"(behind\s+(\d+))"));
                    const auto am = aheadRe.match(track);
                    const auto bm = behindRe.match(track);
                    if (am.hasMatch())
                        e.ahead = am.captured(1).toInt();
                    if (bm.hasMatch())
                        e.behind = bm.captured(1).toInt();
                }
            } else {
                e.name = line.section(QLatin1Char(' '), 0, 0);
            }
            e.remote = false;
            if (!e.name.isEmpty() && e.name != QLatin1String("(HEAD") && e.name != QLatin1String("HEAD"))
                list.push_back(e);
        }
    };
    parseLocal(localOut);

    for (QString line : remoteOut.split(QLatin1Char('\n'), Qt::SkipEmptyParts)) {
        line = line.trimmed();
        if (line.endsWith(QLatin1String("/HEAD")) || line.contains(QLatin1String("->")))
            continue;
        BranchEntry e;
        e.name = line;
        e.remote = true;
        e.current = false;
        list.push_back(e);
    }
    m_branches.setBranches(std::move(list));
}

void RepoContext::parseCommits(const QString &logOut)
{
    QVector<CommitEntry> list;
    for (const QString &line : logOut.split(QLatin1Char('\n'), Qt::SkipEmptyParts)) {
        const QStringList parts = line.split(QLatin1Char('\t'));
        if (parts.size() < 4)
            continue;
        CommitEntry e;
        e.hash = parts.at(0);
        e.shortHash = e.hash.left(7);
        e.subject = parts.at(1);
        e.author = parts.at(2);
        e.relativeTime = parts.at(3);
        list.push_back(e);
    }
    m_lastCommitSubject = list.isEmpty() ? QString() : list.first().subject;
    m_commits.setCommits(std::move(list));
}

bool RepoContext::loadFileDiff(const QString &path, bool staged, bool wordDiff)
{
    if (!m_open || path.trimmed().isEmpty())
        return false;

    GitRunner runner;
    QStringList args;
    if (staged) {
        args = {QStringLiteral("diff"), QStringLiteral("--cached")};
        if (wordDiff)
            args << QStringLiteral("--word-diff");
        args << QStringLiteral("--") << path;
    } else {
        const GitRunResult tracked = runner.run(m_rootPath,
                                                {QStringLiteral("ls-files"), QStringLiteral("--error-unmatch"), path});
        if (!tracked.ok) {
            args = {QStringLiteral("diff"), QStringLiteral("--no-index")};
            if (wordDiff)
                args << QStringLiteral("--word-diff");
            args << QStringLiteral("--") << QStringLiteral("/dev/null") << path;
            const GitRunResult r = runner.run(m_rootPath, args);
            m_selectedDiffPath = path;
            m_selectedDiffStaged = false;
            m_selectedDiff = QString::fromUtf8(r.stdoutBytes);
            if (m_selectedDiff.trimmed().isEmpty())
                m_selectedDiff = tr("(нет текстового diff — возможно бинарный или пустой файл)");
            emit diffChanged();
            return true;
        }
        args = {QStringLiteral("diff")};
        if (wordDiff)
            args << QStringLiteral("--word-diff");
        args << QStringLiteral("--") << path;
    }

    const GitRunResult r = runner.run(m_rootPath, args);
    m_selectedDiffPath = path;
    m_selectedDiffStaged = staged;
    m_selectedDiff = QString::fromUtf8(r.stdoutBytes);
    if (m_selectedDiff.trimmed().isEmpty()) {
        if (!staged) {
            const GitRunResult cached = runner.run(m_rootPath,
                                                   {QStringLiteral("diff"), QStringLiteral("--cached"),
                                                    QStringLiteral("--"), path});
            const QString cachedText = QString::fromUtf8(cached.stdoutBytes);
            if (!cachedText.trimmed().isEmpty()) {
                m_selectedDiff = cachedText;
                m_selectedDiffStaged = true;
            } else {
                m_selectedDiff = tr("(нет diff для этого файла — возможно только смена режима/EOL)");
            }
        } else {
            m_selectedDiff = tr("(в индексе нет diff для этого файла)");
        }
    }
    emit diffChanged();
    return true;
}

void RepoContext::clearFileDiff()
{
    if (m_selectedDiff.isEmpty() && m_selectedDiffPath.isEmpty())
        return;
    m_selectedDiff.clear();
    m_selectedDiffPath.clear();
    m_selectedDiffStaged = false;
    emit diffChanged();
}

void RepoContext::parseStashes(const QString &stashOut)
{
    QVector<StashEntry> list;
    for (const QString &line : stashOut.split(QLatin1Char('\n'), Qt::SkipEmptyParts)) {
        const QStringList parts = line.split(QLatin1Char('\t'));
        if (parts.isEmpty())
            continue;
        StashEntry e;
        e.ref = parts.at(0).trimmed();
        e.message = parts.size() > 1 ? parts.at(1).trimmed() : QString();
        list.push_back(e);
    }
    m_stashes.setEntries(std::move(list));
}

void RepoContext::parseTags(const QString &tagOut)
{
    QVector<TagEntry> list;
    for (const QString &line : tagOut.split(QLatin1Char('\n'), Qt::SkipEmptyParts)) {
        // name\thash\tshort\trelative\tsubject
        const QStringList parts = line.split(QLatin1Char('\t'));
        if (parts.isEmpty())
            continue;
        TagEntry e;
        e.name = parts.at(0).trimmed();
        e.hash = parts.size() > 1 ? parts.at(1).trimmed() : QString();
        e.shortHash = parts.size() > 2 ? parts.at(2).trimmed() : e.hash.left(7);
        e.relativeTime = parts.size() > 3 ? parts.at(3).trimmed() : QString();
        e.subject = parts.size() > 4 ? parts.at(4).trimmed() : QString();
        list.push_back(e);
        if (list.size() >= 50)
            break;
    }
    m_tags.setEntries(std::move(list));
}

void RepoContext::parseRemotes(const QString &remoteOut)
{
    QHash<QString, RemoteEntry> byName;
    for (const QString &line : remoteOut.split(QLatin1Char('\n'), Qt::SkipEmptyParts)) {
        // name\turl\t(fetch|push)
        const QStringList parts = line.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
        if (parts.size() < 3)
            continue;
        const QString name = parts.at(0);
        const QString url = parts.at(1);
        const QString kind = parts.at(2);
        RemoteEntry &e = byName[name];
        e.name = name;
        if (kind.contains(QLatin1String("push")))
            e.pushUrl = url;
        else
            e.fetchUrl = url;
        if (e.pushUrl.isEmpty())
            e.pushUrl = e.fetchUrl;
        if (e.fetchUrl.isEmpty())
            e.fetchUrl = e.pushUrl;
    }
    QVector<RemoteEntry> list;
    list.reserve(byName.size());
    for (auto it = byName.cbegin(); it != byName.cend(); ++it)
        list.push_back(it.value());
    std::sort(list.begin(), list.end(), [](const RemoteEntry &a, const RemoteEntry &b) {
        return a.name < b.name;
    });
    m_remotes.setEntries(std::move(list));
}

void RepoContext::parseSubmodules(const QString &submoduleOut)
{
    QVector<SubmoduleEntry> list;
    for (const QString &line : submoduleOut.split(QLatin1Char('\n'), Qt::SkipEmptyParts)) {
        if (line.size() < 2)
            continue;
        SubmoduleEntry e;
        const QChar mark = line.at(0);
        if (mark == QLatin1Char('-'))
            e.status = QStringLiteral("uninitialized");
        else if (mark == QLatin1Char('+'))
            e.status = QStringLiteral("modified");
        else if (mark == QLatin1Char('U'))
            e.status = QStringLiteral("conflict");
        else
            e.status = QStringLiteral("ok");
        const QString rest = line.mid(1).trimmed();
        const int sp = rest.indexOf(QLatin1Char(' '));
        if (sp <= 0)
            continue;
        e.sha = rest.left(sp).trimmed();
        QString pathPart = rest.mid(sp + 1).trimmed();
        if (pathPart.endsWith(QLatin1Char(')'))) {
            const int open = pathPart.lastIndexOf(QLatin1String(" ("));
            if (open >= 0) {
                e.describe = pathPart.mid(open + 2, pathPart.size() - open - 3);
                pathPart = pathPart.left(open);
            }
        }
        e.path = pathPart.trimmed();
        if (!e.path.isEmpty())
            list.push_back(e);
    }
    m_submodules.setEntries(std::move(list));
}

void RepoContext::loadHooks()
{
    GitRunner runner;
    const GitRunResult gp = runner.run(m_rootPath,
                                       {QStringLiteral("rev-parse"), QStringLiteral("--git-path"),
                                        QStringLiteral("hooks")});
    QVector<HookEntry> list;
    if (!gp.ok) {
        m_hooks.clear();
        return;
    }
    QString hooksDir = QString::fromUtf8(gp.stdoutBytes).trimmed();
    if (!hooksDir.isEmpty() && !QFileInfo(hooksDir).isAbsolute())
        hooksDir = QDir(m_rootPath).filePath(hooksDir);
    QDir dir(hooksDir);
    if (!dir.exists()) {
        m_hooks.clear();
        return;
    }
    const QFileInfoList files = dir.entryInfoList(QDir::Files | QDir::NoDotAndDotDot, QDir::Name);
    for (const QFileInfo &fi : files) {
        HookEntry e;
        e.name = fi.fileName();
        e.sample = e.name.endsWith(QLatin1String(".sample"));
        e.executable = fi.isExecutable() && !e.sample;
        list.push_back(e);
    }
    m_hooks.setEntries(std::move(list));
}

void RepoContext::parseWorktrees(const QString &worktreeOut)
{
    QVector<WorktreeEntry> list;
    WorktreeEntry cur;
    auto flush = [&]() {
        if (cur.path.isEmpty())
            return;
        list.push_back(cur);
        cur = WorktreeEntry{};
    };
    for (const QString &raw : worktreeOut.split(QLatin1Char('\n'))) {
        const QString line = raw.trimmed();
        if (line.isEmpty()) {
            flush();
            continue;
        }
        if (line.startsWith(QLatin1String("worktree "))) {
            flush();
            cur.path = line.mid(9).trimmed();
        } else if (line.startsWith(QLatin1String("HEAD "))) {
            cur.head = line.mid(5).trimmed();
        } else if (line.startsWith(QLatin1String("branch "))) {
            QString b = line.mid(7).trimmed();
            if (b.startsWith(QLatin1String("refs/heads/")))
                b = b.mid(QStringLiteral("refs/heads/").size());
            cur.branch = b;
        } else if (line == QLatin1String("detached")) {
            cur.branch = QStringLiteral("(detached)");
        } else if (line == QLatin1String("bare")) {
            cur.bare = true;
        } else if (line.startsWith(QLatin1String("locked"))) {
            cur.locked = true;
        } else if (line.startsWith(QLatin1String("prunable"))) {
            cur.prunable = true;
        }
    }
    flush();
    m_worktrees.setEntries(std::move(list), m_rootPath);
}

void RepoContext::parseReflog(const QString &reflogOut)
{
    QVector<ReflogEntry> list;
    for (const QString &line : reflogOut.split(QLatin1Char('\n'), Qt::SkipEmptyParts)) {
        // format: selector\thash\tmessage
        const QStringList parts = line.split(QLatin1Char('\t'));
        if (parts.size() < 2)
            continue;
        ReflogEntry e;
        e.selector = parts.at(0).trimmed();
        e.hash = parts.at(1).trimmed();
        e.message = parts.size() > 2 ? parts.at(2).trimmed() : QString();
        list.push_back(e);
        if (list.size() >= 40)
            break;
    }
    m_reflog.setEntries(std::move(list));
}

bool RepoContext::loadCommitShow(const QString &hash)
{
    if (!m_open || hash.trimmed().isEmpty())
        return false;
    GitRunner runner;
    const GitRunResult r = runner.run(m_rootPath,
                                      {QStringLiteral("show"), QStringLiteral("--stat"),
                                       QStringLiteral("--patch"), QStringLiteral("--format=fuller"), hash});
    m_inspectKind = QStringLiteral("commit");
    m_inspectTarget = hash.trimmed();
    m_inspectTitle = tr("Коммит %1").arg(m_inspectTarget.left(7));
    m_inspectText = QString::fromUtf8(r.stdoutBytes);
    if (m_inspectText.trimmed().isEmpty())
        m_inspectText = r.error.isEmpty() ? tr("(пустой вывод git show)") : r.error;
    emit inspectChanged();
    return true;
}

bool RepoContext::loadStashShow(const QString &ref)
{
    if (!m_open || ref.trimmed().isEmpty())
        return false;
    GitRunner runner;
    const GitRunResult r = runner.run(m_rootPath,
                                      {QStringLiteral("stash"), QStringLiteral("show"),
                                       QStringLiteral("-p"), QStringLiteral("--stat"), ref.trimmed()});
    m_inspectKind = QStringLiteral("stash");
    m_inspectTarget = ref.trimmed();
    m_inspectTitle = tr("Stash %1").arg(m_inspectTarget);
    m_inspectText = QString::fromUtf8(r.stdoutBytes);
    if (m_inspectText.trimmed().isEmpty())
        m_inspectText = r.error.isEmpty() ? tr("(пустой stash show)") : r.error;
    emit inspectChanged();
    return true;
}

bool RepoContext::loadFileBlame(const QString &path)
{
    if (!m_open || path.trimmed().isEmpty())
        return false;
    GitRunner runner;
    const GitRunResult r = runner.run(m_rootPath,
                                      {QStringLiteral("blame"), QStringLiteral("--line-porcelain"), path});
    // Prefer human-readable blame
    const GitRunResult pretty = runner.run(m_rootPath, {QStringLiteral("blame"), path});
    m_inspectKind = QStringLiteral("blame");
    m_inspectTarget = path.trimmed();
    m_inspectTitle = tr("Blame: %1").arg(path);
    m_inspectText = QString::fromUtf8(pretty.stdoutBytes);
    if (m_inspectText.trimmed().isEmpty())
        m_inspectText = QString::fromUtf8(r.stdoutBytes);
    if (m_inspectText.trimmed().isEmpty())
        m_inspectText = pretty.error.isEmpty() ? tr("(blame недоступен для этого файла)") : pretty.error;
    emit inspectChanged();
    return true;
}

bool RepoContext::loadFileHistory(const QString &path)
{
    if (!m_open || path.trimmed().isEmpty())
        return false;
    GitRunner runner;
    const GitRunResult r = runner.run(m_rootPath,
                                      {QStringLiteral("log"), QStringLiteral("-n"), QStringLiteral("30"),
                                       QStringLiteral("--stat"), QStringLiteral("--format=fuller"),
                                       QStringLiteral("--"), path.trimmed()});
    m_inspectKind = QStringLiteral("history");
    m_inspectTarget = path.trimmed();
    m_inspectTitle = tr("История: %1").arg(path.trimmed());
    m_inspectText = QString::fromUtf8(r.stdoutBytes);
    if (m_inspectText.trimmed().isEmpty())
        m_inspectText = r.error.isEmpty() ? tr("(нет истории для файла)") : r.error;
    emit inspectChanged();
    return true;
}

bool RepoContext::loadFileAttributes(const QString &path)
{
    if (!m_open || path.trimmed().isEmpty())
        return false;
    GitRunner runner;
    const GitRunResult r = runner.run(m_rootPath,
                                      {QStringLiteral("check-attr"), QStringLiteral("-a"),
                                       QStringLiteral("--"), path.trimmed()});
    m_inspectKind = QStringLiteral("attr");
    m_inspectTarget = path.trimmed();
    m_inspectTitle = tr("Атрибуты: %1").arg(path.trimmed());
    m_inspectText = QString::fromUtf8(r.stdoutBytes);
    if (m_inspectText.trimmed().isEmpty())
        m_inspectText = r.error.isEmpty() ? tr("(атрибуты не заданы)") : r.error;
    if (m_inspectText.contains(QLatin1String("filter: lfs"))
        || m_inspectText.contains(QLatin1String("filter:lfs"))) {
        m_inspectText += tr("\n\n— Похоже на Git LFS (filter=lfs).");
    }
    emit inspectChanged();
    return true;
}

bool RepoContext::loadCleanPreview()
{
    if (!m_open)
        return false;
    GitRunner runner;
    const GitRunResult r = runner.run(m_rootPath,
                                      {QStringLiteral("clean"), QStringLiteral("-fdn")});
    m_inspectKind = QStringLiteral("clean");
    m_inspectTarget.clear();
    m_inspectTitle = tr("git clean -fdn (dry-run)");
    m_inspectText = QString::fromUtf8(r.stdoutBytes);
    if (m_inspectText.trimmed().isEmpty())
        m_inspectText = r.error.isEmpty()
                            ? tr("Нечего удалять (дерево без untracked для clean -fd).")
                            : r.error;
    emit inspectChanged();
    return true;
}

void RepoContext::clearInspect()
{
    if (m_inspectText.isEmpty() && m_inspectTitle.isEmpty() && m_inspectTarget.isEmpty())
        return;
    m_inspectText.clear();
    m_inspectTitle.clear();
    m_inspectKind.clear();
    m_inspectTarget.clear();
    emit inspectChanged();
}

bool RepoContext::refresh()
{
    if (!m_open)
        return false;

    m_refreshing = true;
    GitRunner runner;
    const GitRunResult branch = runner.run(m_rootPath, {QStringLiteral("branch"), QStringLiteral("--show-current")});
    const GitRunResult bare = runner.run(m_rootPath, {QStringLiteral("rev-parse"), QStringLiteral("--is-bare-repository")});
    const GitRunResult super = runner.run(m_rootPath, {QStringLiteral("rev-parse"),
                                                       QStringLiteral("--show-superproject-working-tree")});
    const GitRunResult statusSb = runner.run(m_rootPath, {QStringLiteral("status"), QStringLiteral("-sb")});
    const GitRunResult porcelain = runner.run(m_rootPath, {QStringLiteral("status"), QStringLiteral("--porcelain")});
    const GitRunResult localCount = runner.run(m_rootPath, {QStringLiteral("rev-list"), QStringLiteral("--count"),
                                                            QStringLiteral("--branches")});
    const GitRunResult remoteCount = runner.run(m_rootPath, {QStringLiteral("rev-list"), QStringLiteral("--count"),
                                                             QStringLiteral("--remotes")});
    const GitRunResult upstream = runner.run(m_rootPath, {QStringLiteral("rev-parse"), QStringLiteral("--abbrev-ref"),
                                                          QStringLiteral("--symbolic-full-name"), QStringLiteral("@{u}")});
    const GitRunResult aheadBehind = runner.run(m_rootPath,
                                                {QStringLiteral("rev-list"), QStringLiteral("--left-right"),
                                                 QStringLiteral("--count"), QStringLiteral("HEAD...@{u}")});
    const GitRunResult diffStat = runner.run(m_rootPath, {QStringLiteral("diff"), QStringLiteral("--stat"),
                                                          QStringLiteral("HEAD")});
    const GitRunResult cachedStat = runner.run(m_rootPath, {QStringLiteral("diff"), QStringLiteral("--cached"),
                                                            QStringLiteral("--stat")});
    const GitRunResult localBranches = runner.run(m_rootPath, {QStringLiteral("branch"), QStringLiteral("-vv")});
    const GitRunResult remoteBranches = runner.run(m_rootPath, {QStringLiteral("branch"), QStringLiteral("-r")});
    const GitRunResult log = runner.run(m_rootPath,
                                        {QStringLiteral("log"), QStringLiteral("-n"), QStringLiteral("40"),
                                         QStringLiteral("--format=%H%x09%s%x09%an%x09%ar")});
    const GitRunResult stash = runner.run(m_rootPath,
                                          {QStringLiteral("stash"), QStringLiteral("list"),
                                           QStringLiteral("--format=%gd%x09%s")});
    const GitRunResult tags = runner.run(m_rootPath,
                                         {QStringLiteral("for-each-ref"), QStringLiteral("--sort=-creatordate"),
                                          QStringLiteral("--count=50"),
                                          QStringLiteral("--format=%(refname:short)%09%(objectname)%09%(objectname:short)%09%(creatordate:relative)%09%(contents:subject)"),
                                          QStringLiteral("refs/tags")});
    const GitRunResult remotes = runner.run(m_rootPath, {QStringLiteral("remote"), QStringLiteral("-v")});
    const GitRunResult worktrees = runner.run(m_rootPath,
                                              {QStringLiteral("worktree"), QStringLiteral("list"),
                                               QStringLiteral("--porcelain")});
    const GitRunResult submodules = runner.run(m_rootPath,
                                               {QStringLiteral("submodule"), QStringLiteral("status")});
    const GitRunResult bisectPath = runner.run(m_rootPath,
                                               {QStringLiteral("rev-parse"), QStringLiteral("--git-path"),
                                                QStringLiteral("BISECT_START")});
    const GitRunResult gitDirRes = runner.run(m_rootPath,
                                              {QStringLiteral("rev-parse"), QStringLiteral("--git-dir")});
    const GitRunResult reflog = runner.run(m_rootPath,
                                           {QStringLiteral("reflog"), QStringLiteral("-n"), QStringLiteral("40"),
                                            QStringLiteral("--format=%gd%x09%h%x09%gs")});

    if (!branch.ok || !statusSb.ok || !porcelain.ok) {
        m_lastError = !branch.ok ? branch.error : (!statusSb.ok ? statusSb.error : porcelain.error);
        m_refreshing = false;
        emit changed();
        return false;
    }

    const QString branchName = QString::fromUtf8(branch.stdoutBytes).trimmed();
    m_detachedHead = branchName.isEmpty();
    m_branch = m_detachedHead ? tr("(detached HEAD)") : branchName;
    m_bare = bare.ok && QString::fromUtf8(bare.stdoutBytes).trimmed() == QLatin1String("true");
    m_insideSubmodule = super.ok && !QString::fromUtf8(super.stdoutBytes).trimmed().isEmpty();
    m_bisectActive = false;
    if (bisectPath.ok) {
        QString p = QString::fromUtf8(bisectPath.stdoutBytes).trimmed();
        if (!p.isEmpty() && !QFileInfo(p).isAbsolute())
            p = QDir(m_rootPath).filePath(p);
        m_bisectActive = QFileInfo::exists(p);
    }
    m_operationKind.clear();
    if (gitDirRes.ok) {
        QString gd = QString::fromUtf8(gitDirRes.stdoutBytes).trimmed();
        if (!gd.isEmpty() && !QFileInfo(gd).isAbsolute())
            gd = QDir(m_rootPath).filePath(gd);
        const QDir gitDir(gd);
        if (gitDir.exists(QStringLiteral("rebase-merge")) || gitDir.exists(QStringLiteral("rebase-apply")))
            m_operationKind = QStringLiteral("rebase");
        else if (QFileInfo::exists(gitDir.filePath(QStringLiteral("MERGE_HEAD"))))
            m_operationKind = QStringLiteral("merge");
        else if (QFileInfo::exists(gitDir.filePath(QStringLiteral("CHERRY_PICK_HEAD"))))
            m_operationKind = QStringLiteral("cherry-pick");
        else if (QFileInfo::exists(gitDir.filePath(QStringLiteral("REVERT_HEAD"))))
            m_operationKind = QStringLiteral("revert");
    }
    m_shortStatus = QString::fromUtf8(statusSb.stdoutBytes).trimmed();
    parsePorcelain(QString::fromUtf8(porcelain.stdoutBytes));

    m_localCommits = parseCount(localCount);
    m_remoteCommits = parseCount(remoteCount);

    m_hasUpstream = upstream.ok;
    m_upstream = m_hasUpstream ? QString::fromUtf8(upstream.stdoutBytes).trimmed() : QString();
    m_ahead = 0;
    m_behind = 0;
    if (aheadBehind.ok) {
        const QString ab = QString::fromUtf8(aheadBehind.stdoutBytes).trimmed();
        const QStringList parts = ab.split(QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
        if (parts.size() >= 2) {
            m_ahead = parts[0].toInt();
            m_behind = parts[1].toInt();
        }
    }

    QStringList stats;
    const QString work = QString::fromUtf8(diffStat.stdoutBytes).trimmed();
    const QString staged = QString::fromUtf8(cachedStat.stdoutBytes).trimmed();
    if (!staged.isEmpty())
        stats << tr("Индекс:\n%1").arg(staged);
    if (!work.isEmpty())
        stats << tr("Рабочее дерево (к HEAD):\n%1").arg(work);
    if (stats.isEmpty() && m_files.rowCount() == 0)
        m_diffStat = tr("Нет незакоммиченных изменений.");
    else if (stats.isEmpty())
        m_diffStat = tr("Есть незакоммиченные файлы (см. список).");
    else
        m_diffStat = stats.join(QStringLiteral("\n\n"));

    parseBranches(QString::fromUtf8(localBranches.stdoutBytes), QString::fromUtf8(remoteBranches.stdoutBytes));
    if (log.ok) {
        parseCommits(QString::fromUtf8(log.stdoutBytes));
    } else {
        m_commits.clear();
        m_lastCommitSubject.clear();
    }

    if (stash.ok)
        parseStashes(QString::fromUtf8(stash.stdoutBytes));
    else
        m_stashes.clear();

    if (tags.ok)
        parseTags(QString::fromUtf8(tags.stdoutBytes));
    else
        m_tags.clear();

    if (remotes.ok)
        parseRemotes(QString::fromUtf8(remotes.stdoutBytes));
    else
        m_remotes.clear();

    if (worktrees.ok)
        parseWorktrees(QString::fromUtf8(worktrees.stdoutBytes));
    else
        m_worktrees.clear();

    if (submodules.ok)
        parseSubmodules(QString::fromUtf8(submodules.stdoutBytes));
    else
        m_submodules.clear();

    loadHooks();

    if (reflog.ok)
        parseReflog(QString::fromUtf8(reflog.stdoutBytes));
    else
        m_reflog.clear();

    m_lastError.clear();
    setupWatcher();
    m_refreshing = false;
    emit changed();
    return true;
}

QString RepoContext::dumpStatusJson() const
{
    QJsonObject o;
    o.insert(QStringLiteral("open"), m_open);
    o.insert(QStringLiteral("root"), m_rootPath);
    o.insert(QStringLiteral("branch"), m_branch);
    o.insert(QStringLiteral("detachedHead"), m_detachedHead);
    o.insert(QStringLiteral("bare"), m_bare);
    o.insert(QStringLiteral("insideSubmodule"), m_insideSubmodule);
    o.insert(QStringLiteral("bisectActive"), m_bisectActive);
    o.insert(QStringLiteral("operationKind"), m_operationKind);
    o.insert(QStringLiteral("conflictCount"), m_conflictCount);
    o.insert(QStringLiteral("worktrees"), m_worktrees.rowCount());
    o.insert(QStringLiteral("submodules"), m_submodules.rowCount());
    o.insert(QStringLiteral("hooksActive"), m_hooks.activeCount());
    o.insert(QStringLiteral("upstream"), m_upstream);
    o.insert(QStringLiteral("hasUpstream"), m_hasUpstream);
    o.insert(QStringLiteral("ahead"), m_ahead);
    o.insert(QStringLiteral("behind"), m_behind);
    o.insert(QStringLiteral("localCommits"), m_localCommits);
    o.insert(QStringLiteral("remoteCommits"), m_remoteCommits);
    o.insert(QStringLiteral("tags"), m_tags.rowCount());
    o.insert(QStringLiteral("staged"), m_stagedCount);
    o.insert(QStringLiteral("unstaged"), m_unstagedCount);
    o.insert(QStringLiteral("untracked"), m_untrackedCount);
    o.insert(QStringLiteral("dirty"), m_files.rowCount());
    o.insert(QStringLiteral("diffStat"), m_diffStat);
    o.insert(QStringLiteral("lastCommitSubject"), m_lastCommitSubject);
    o.insert(QStringLiteral("branches"), m_branches.rowCount());
    o.insert(QStringLiteral("commits"), m_commits.rowCount());
    o.insert(QStringLiteral("stashes"), m_stashes.rowCount());
    o.insert(QStringLiteral("reflog"), m_reflog.rowCount());

    QJsonArray files;
    for (int i = 0; i < m_files.rowCount(); ++i) {
        const QModelIndex idx = m_files.index(i);
        QJsonObject f;
        f.insert(QStringLiteral("path"), m_files.data(idx, StatusFileModel::PathRole).toString());
        f.insert(QStringLiteral("code"), m_files.data(idx, StatusFileModel::CodeRole).toString());
        f.insert(QStringLiteral("state"), m_files.data(idx, StatusFileModel::StateRole).toString());
        f.insert(QStringLiteral("summary"), m_files.data(idx, StatusFileModel::SummaryRole).toString());
        files.append(f);
    }
    o.insert(QStringLiteral("files"), files);
    return QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact));
}
