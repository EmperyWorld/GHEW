#include "AppController.h"

#include "core/GhewBuild.h"
#include "core/GithubLifecycle.h"
#include "core/GithubServices.h"
#include "core/GithubAccess.h"
#include "core/GithubDeskActions.h"
#include "core/GithubIdentity.h"
#include "core/GithubPolicy.h"
#include "core/GitStatusParse.h"
#include "core/HunkParse.h"
#include "core/StatusFileModel.h"

#include <QCoreApplication>
#include <QClipboard>
#include <QDateTime>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QGuiApplication>
#include <QUrl>
#include <QVariantMap>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcessEnvironment>
#include <QRegularExpression>
#include <QResource>
#include <QSettings>
#include <QStandardPaths>
#include <QTemporaryFile>
#include <QTextStream>

AppController *AppController::s_instance = nullptr;

static QString resolveCommandsDirectory()
{
    for (const char *envName : {"GHEW_DATA_DIR", "GIT_LAB_DATA_DIR"}) {
        if (const QByteArray env = qgetenv(envName); !env.isEmpty()) {
            const QString path = QString::fromLocal8Bit(env) + QStringLiteral("/commands");
            if (QDir(path).exists())
                return path;
        }
    }

    const QString sourcePath = QStringLiteral(GHEW_SOURCE_DATA_DIR) + QStringLiteral("/commands");
    if (QDir(sourcePath).exists()) {
        return sourcePath;
    }

    const QString appDir = QCoreApplication::applicationDirPath();
    const QStringList candidates = {
        appDir + QStringLiteral("/../share/ghew/commands"),
        QStringLiteral("/usr/share/ghew/commands"),
        QStringLiteral("/usr/local/share/ghew/commands"),
        QStringLiteral(":/ghew/commands"),
        // legacy install path
        appDir + QStringLiteral("/../share/git-lab/commands"),
        QStringLiteral("/usr/share/git-lab/commands"),
    };
    for (const QString &candidate : candidates) {
        if (candidate.startsWith(QLatin1Char(':'))) {
            QDir dir(candidate);
            if (dir.exists() && !dir.entryList(QDir::Files).isEmpty()) {
                return candidate;
            }
            continue;
        }
        const QString cleaned = QDir::cleanPath(candidate);
        if (QDir(cleaned).exists()) {
            return cleaned;
        }
    }
    return sourcePath;
}

AppController::AppController(QObject *parent)
    : QObject(parent)
    , m_github(&m_repo)
    , m_advisor(&m_repo, &m_catalog, &m_llm)
    , m_preview(&m_runner)
{
    m_advisor.setGithub(&m_github);
    const QString dir = resolveCommandsDirectory();
    if (!m_catalog.loadFromDirectory(dir) || m_catalog.groups().isEmpty()) {
        m_catalog.loadFromDirectory(QStringLiteral(":/ghew/commands"));
    }

    QSettings settings;
    m_autoRefresh = settings.value(QStringLiteral("ui/autoRefresh"), true).toBool();

    connect(&m_advisor, &Advisor::selectCommandRequested, this, [this](const QString &id) {
        emit selectCommandRequested(id, {});
    });
    connect(&m_advisor, &Advisor::runCommandRequested, this, [this](const QString &id) {
        runCommand(id, {}, true);
    });
    connect(&m_repo, &RepoContext::externalChangeDetected, this, [this]() {
        if (m_busy || !m_autoRefresh)
            return;
        m_repo.refresh();
        emit infoOccurred(tr("Обнаружены внешние изменения — статус обновлён"));
    });
}

void AppController::setInstance(AppController *instance)
{
    s_instance = instance;
}

AppController *AppController::create(QQmlEngine *engine, QJSEngine *scriptEngine)
{
    Q_UNUSED(scriptEngine);
    Q_ASSERT(s_instance);
    QQmlEngine::setObjectOwnership(s_instance, QQmlEngine::CppOwnership);
    if (engine) {
        engine->setObjectOwnership(s_instance, QQmlEngine::CppOwnership);
    }
    return s_instance;
}

RepoContext *AppController::repo()
{
    return &m_repo;
}

CommandCatalog *AppController::catalog()
{
    return &m_catalog;
}

HistoryModel *AppController::history()
{
    return &m_history;
}

Advisor *AppController::advisor()
{
    return &m_advisor;
}

LlmClient *AppController::llm()
{
    return &m_llm;
}

GithubClient *AppController::github()
{
    return &m_github;
}

RecommendationModel *AppController::recommendations()
{
    return m_advisor.recommendations();
}

RecentReposModel *AppController::recentRepos()
{
    return &m_recent;
}

FavoritesModel *AppController::favorites()
{
    return &m_favorites;
}

RecentCommandsModel *AppController::recentCommands()
{
    return &m_recentCommands;
}

HunkListModel *AppController::hunks()
{
    return &m_hunks;
}

bool AppController::paletteOpen() const
{
    return m_paletteOpen;
}

void AppController::setPaletteOpen(bool open)
{
    if (m_paletteOpen == open)
        return;
    m_paletteOpen = open;
    emit paletteOpenChanged();
}

void AppController::openPalette()
{
    setPaletteOpen(true);
}

void AppController::closePalette()
{
    setPaletteOpen(false);
}

bool AppController::branchDialogOpen() const
{
    return m_branchDialogOpen;
}

void AppController::setBranchDialogOpen(bool open)
{
    if (m_branchDialogOpen == open)
        return;
    m_branchDialogOpen = open;
    emit branchDialogOpenChanged();
}

void AppController::openBranchDialog()
{
    setBranchDialogOpen(true);
}

void AppController::closeBranchDialog()
{
    setBranchDialogOpen(false);
}

bool AppController::tagDialogOpen() const
{
    return m_tagDialogOpen;
}

void AppController::setTagDialogOpen(bool open)
{
    if (m_tagDialogOpen == open)
        return;
    m_tagDialogOpen = open;
    emit tagDialogOpenChanged();
}

void AppController::openTagDialog()
{
    setTagDialogOpen(true);
}

void AppController::closeTagDialog()
{
    setTagDialogOpen(false);
}

QString AppController::lastPreviewJson() const
{
    return m_lastPreviewJson;
}

bool AppController::cancellable() const
{
    return m_busy && m_gitProcess != nullptr;
}

QString AppController::lessonRepoPath() const
{
    return m_lessonRepoPath;
}

QString AppController::lastOutput() const
{
    return m_lastOutput;
}

int AppController::lastExitCode() const
{
    return m_lastExitCode;
}

bool AppController::busy() const
{
    return m_busy;
}

int AppController::commandCount() const
{
    return m_catalog.totalCount();
}

bool AppController::autoRefresh() const
{
    return m_autoRefresh;
}

void AppController::setAutoRefresh(bool enabled)
{
    if (m_autoRefresh == enabled)
        return;
    m_autoRefresh = enabled;
    QSettings settings;
    settings.setValue(QStringLiteral("ui/autoRefresh"), m_autoRefresh);
    emit autoRefreshChanged();
}

QString AppController::lastStatusDelta() const
{
    return m_lastStatusDelta;
}

bool AppController::openRepository(const QString &path)
{
    if (!m_repo.open(path)) {
        emit errorOccurred(m_repo.lastError());
        return false;
    }
    m_recent.touch(m_repo.rootPath());
    m_github.refresh();
    m_advisor.refresh();
    m_llm.probe();
    return true;
}

void AppController::refreshRepo()
{
    if (!m_repo.refresh()) {
        emit errorOccurred(m_repo.lastError());
        return;
    }
    m_advisor.refresh();
}

void AppController::askAdvice(const QString &question)
{
    m_advisor.askAdvice(question);
}

void AppController::replyMentor(const QString &text)
{
    m_advisor.replyMentor(text);
}

void AppController::answerAskUser(const QString &option)
{
    m_advisor.answerAskUser(option);
}

void AppController::enrichRecommendations()
{
    m_advisor.enrichWithLlm();
}

void AppController::probeLlm()
{
    m_llm.probe();
}

void AppController::saveLlmSettings(const QString &baseUrl, const QString &model, const QString &apiKey)
{
    m_llm.setBaseUrl(baseUrl);
    m_llm.setModel(model);
    m_llm.setApiKey(apiKey);
    m_llm.persistSettings();
    m_llm.probe();
}

void AppController::loadFileDiff(const QString &path, bool staged, bool wordDiff)
{
    if (!m_repo.loadFileDiff(path, staged, wordDiff))
        emit errorOccurred(tr("Не удалось получить diff для %1").arg(path));
}

QString AppController::auditLogPath() const
{
    const QString dir = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(dir);
    return dir + QStringLiteral("/session-audit.jsonl");
}

void AppController::prepareCommand(const QString &commandId, const QVariantMap &args)
{
    emit selectCommandRequested(commandId, args);
}

void AppController::runCommand(const QString &commandId, const QVariantMap &args, bool fromAdvisor)
{
    if (commandId.isEmpty())
        return;
    if (m_busy || m_gitProcess) {
        emit errorOccurred(tr("Уже выполняется git-команда. Дождитесь завершения."));
        return;
    }

    if (githubDeskActionKnown(commandId)) {
        QVariantMap deskArgs = args;
        if (!githubDeskAcceptAdvisorCommand(commandId, &deskArgs)) {
            emit errorOccurred(tr("Эту правку советник не передаёт: секрет, секрет вебхука или закрытый ключ вводятся на вкладке GitHub."));
            return;
        }
        m_deskFromAdvisor = fromAdvisor;
        dispatchGithubDesk(commandId, deskArgs);
        m_deskFromAdvisor = false;
        return;
    }

    if (commandId == QLatin1String("config_set_user")) {
        saveGitUser(args.value(QStringLiteral("name")).toString(),
                    args.value(QStringLiteral("email")).toString(), fromAdvisor);
        return;
    }

    QVariantMap filled = args;
    if (commandId == QLatin1String("github_create")) {
        m_github.refreshIfStale(20);
        if (!m_github.available()) {
            emit errorOccurred(tr("GitHub CLI (gh) не найден. Установите пакет gh."));
            return;
        }
        if (!m_github.authenticated()) {
            emit errorOccurred(tr("Нет входа в GitHub. В терминале выполните gh auth login."));
            return;
        }
        if (m_github.hasOrigin()) {
            emit errorOccurred(tr("Remote origin уже есть. Новый репозиторий с --remote=origin не создаётся."));
            return;
        }
        if (filled.value(QStringLiteral("name")).toString().trimmed().isEmpty())
            filled.insert(QStringLiteral("name"), m_github.suggestedName());
        if (filled.value(QStringLiteral("public")).toBool())
            filled.insert(QStringLiteral("private"), false);
    }
    if (commandId == QLatin1String("github_describe") || commandId == QLatin1String("github_topic")
        || commandId == QLatin1String("github_topic_remove") || commandId == QLatin1String("github_view")
        || commandId == QLatin1String("github_rename") || commandId == QLatin1String("github_visibility")) {
        m_github.refreshIfStale(20);
        if (!m_github.available() || !m_github.authenticated()) {
            emit errorOccurred(tr("Для этой команды нужен gh и выполненный gh auth login."));
            return;
        }
        if (!m_github.originIsGithub()) {
            emit errorOccurred(tr("origin не указывает на GitHub."));
            return;
        }
    }
    if (commandId == QLatin1String("github_describe")) {
        const bool hasDesc = filled.contains(QStringLiteral("description"));
        const bool hasHome = filled.contains(QStringLiteral("homepage"));
        if (!hasDesc && !hasHome) {
            emit errorOccurred(tr("Укажите описание или адрес сайта."));
            return;
        }
    }
    const bool needsMessage = commandId == QLatin1String("commit")
                              || commandId == QLatin1String("commit_amend");
    if (needsMessage && filled.value(QStringLiteral("message")).toString().trimmed().isEmpty()) {
        const QString suggested = suggestCommitMessage();
        if (!suggested.isEmpty())
            filled.insert(QStringLiteral("message"), suggested);
    }

    m_pendingFromAdvisor = fromAdvisor;
    m_pendingStay = false;
    if (!fromAdvisor)
        emit selectCommandRequested(commandId, filled);
    previewCommand(commandId, filled);
    if (m_confirmToken.isEmpty()) {
        m_pendingFromAdvisor = false;
        return;
    }
    if (m_pendingDanger == DangerLevel::Destructive) {
        emit commandRunRequested(3);
        return;
    }
    executeLastPreview(false);
    if (!fromAdvisor)
        emit commandRunRequested(7);
}

bool AppController::ensureGithubRemote(QString *error) const
{
    if (!m_repo.isOpen()) {
        if (error)
            *error = tr("Репозиторий не открыт.");
        return false;
    }
    if (!m_github.available()) {
        if (error)
            *error = tr("GitHub CLI (gh) не найден. Установите пакет gh.");
        return false;
    }
    if (!m_github.authenticated()) {
        if (error)
            *error = tr("Нет входа в GitHub. В терминале выполните gh auth login.");
        return false;
    }
    if (!m_github.originIsGithub()) {
        if (error)
            *error = tr("origin не указывает на GitHub.");
        return false;
    }
    return true;
}

QString AppController::normalizeTopic(const QString &topic, QString *error)
{
    QString text = topic.trimmed().toLower();
    text.replace(QLatin1Char(' '), QLatin1Char('-'));
    const QRegularExpression re(QStringLiteral("^[a-z0-9](?:[a-z0-9-]{0,48}[a-z0-9])?$"));
    if (!re.match(text).hasMatch()) {
        if (error)
            *error = tr("Тема: строчные буквы, цифры и дефис, до 50 символов.");
        return {};
    }
    return text;
}

QString AppController::githubProjectOwner() const
{
    const QString login = m_github.login().trimmed();
    static const QRegularExpression re(QStringLiteral("^[A-Za-z0-9](?:[A-Za-z0-9-]{0,37}[A-Za-z0-9])?$"));
    if (re.match(login).hasMatch())
        return login;
    return QStringLiteral("@me");
}

void AppController::dispatchGithub(const QString &commandId, const QStringList &argv, const QString &error,
                                  DangerLevel danger, const QString &narrative, const QString &stdinText)
{
    if (!error.isEmpty() || argv.isEmpty()) {
        emit errorOccurred(error.isEmpty() ? tr("Не удалось собрать команду.") : error);
        return;
    }
    beginGh(commandId, argv, danger, narrative, QStringLiteral("gh"), stdinText);
}

void AppController::beginGh(const QString &commandId, const QStringList &argv, DangerLevel danger,
                           const QString &narrative, const QString &program, const QString &stdinText)
{
    if (m_busy || m_gitProcess) {
        emit errorOccurred(tr("Уже выполняется команда. Дождитесь завершения."));
        return;
    }
    QString error;
    if (!ensureGithubRemote(&error)) {
        emit errorOccurred(error);
        return;
    }
    if (argv.isEmpty())
        return;

    const bool fromAdvisor = m_deskFromAdvisor;
    m_pendingFromAdvisor = fromAdvisor;
    m_pendingStay = !fromAdvisor;
    m_confirmToken = QStringLiteral("github-editor");
    m_pendingArgv = argv;
    m_pendingProgram = program.isEmpty() ? QStringLiteral("gh") : program;
    m_pendingCmdline = m_pendingProgram + QLatin1Char(' ') + argv.join(QLatin1Char(' '));
    m_pendingCommandId = commandId;
    m_pendingArgs.clear();
    m_pendingStdin = stdinText;
    m_pendingDanger = danger;
    if (danger == DangerLevel::Destructive) {
        QJsonObject obj;
        obj.insert(QStringLiteral("cmdline"), m_pendingCmdline);
        obj.insert(QStringLiteral("narrative"), narrative.isEmpty() ? m_pendingCmdline : narrative);
        obj.insert(QStringLiteral("danger"), CommandSpec::dangerToString(danger));
        obj.insert(QStringLiteral("confirmToken"), m_confirmToken);
        obj.insert(QStringLiteral("commandId"), commandId);
        m_lastPreviewJson = QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
        emit previewChanged();
        emit commandRunRequested(3);
        return;
    }
    executeLastPreview(false);
    if (!fromAdvisor)
        emit commandRunRequested(11);
}

QVariantMap AppController::gitUserIdentity() const
{
    QVariantMap out;
    out.insert(QStringLiteral("name"), QString());
    out.insert(QStringLiteral("email"), QString());
    out.insert(QStringLiteral("complete"), false);
    if (!m_repo.isOpen())
        return out;
    const GitRunResult name = m_runner.run(m_repo.rootPath(),
                                           {QStringLiteral("config"), QStringLiteral("user.name")});
    const GitRunResult email = m_runner.run(m_repo.rootPath(),
                                            {QStringLiteral("config"), QStringLiteral("user.email")});
    const QString n = name.ok ? QString::fromUtf8(name.stdoutBytes).trimmed() : QString();
    const QString e = email.ok ? QString::fromUtf8(email.stdoutBytes).trimmed() : QString();
    out.insert(QStringLiteral("name"), n);
    out.insert(QStringLiteral("email"), e);
    out.insert(QStringLiteral("complete"), !n.isEmpty() && !e.isEmpty());
    return out;
}

bool AppController::saveGitUser(const QString &name, const QString &email, bool fromAdvisor)
{
    if (!m_repo.isOpen()) {
        emit errorOccurred(tr("Репозиторий не открыт."));
        return false;
    }
    const QString n = name.trimmed();
    const QString e = email.trimmed();
    if (n.isEmpty() || e.isEmpty()) {
        emit errorOccurred(tr("Нужны и имя, и email (локальные user.name / user.email)."));
        return false;
    }
    if (n.contains(QLatin1Char('\n')) || e.contains(QLatin1Char('\n')) || e.size() > 254 || n.size() > 100) {
        emit errorOccurred(tr("Имя или email выглядят некорректно."));
        return false;
    }
    if (!e.contains(QLatin1Char('@'))) {
        emit errorOccurred(tr("Email должен содержать @."));
        return false;
    }

    const GitRunResult setName = m_runner.run(m_repo.rootPath(),
                                              {QStringLiteral("config"), QStringLiteral("--local"),
                                               QStringLiteral("user.name"), n});
    if (!setName.ok) {
        emit errorOccurred(setName.error.isEmpty() ? tr("Не удалось записать user.name.") : setName.error);
        return false;
    }
    const GitRunResult setEmail = m_runner.run(m_repo.rootPath(),
                                               {QStringLiteral("config"), QStringLiteral("--local"),
                                                QStringLiteral("user.email"), e});
    if (!setEmail.ok) {
        emit errorOccurred(setEmail.error.isEmpty() ? tr("Не удалось записать user.email.") : setEmail.error);
        return false;
    }

    m_lastExitCode = 0;
    m_lastOutput = QStringLiteral("$ git config --local user.name / user.email\n\n"
                                  "user.name=%1\nuser.email=%2\n\n[exit 0]")
                       .arg(n, e);
    emit outputChanged();
    emit infoOccurred(tr("Локальный автор: %1 <%2>").arg(n, e));
    m_repo.refresh();

    if (fromAdvisor) {
        m_advisor.continueAfterCommand(QStringLiteral("config_set_user"),
                                       QStringLiteral("git config --local user.name/email"), 0,
                                       m_lastOutput, tr("identity ok"));
    }
    return true;
}

bool AppController::saveDressFile(const QString &kind, const QString &body)
{
    if (!m_repo.isOpen()) {
        emit errorOccurred(tr("Репозиторий не открыт."));
        return false;
    }
    const GitRunResult run = m_github.saveDressFile(kind, body);
    const QString name = m_github.dressFileNameFor(kind);
    if (!run.ok) {
        emit errorOccurred(run.error.isEmpty() ? tr("Не удалось записать файл.") : run.error);
        return false;
    }
    m_lastExitCode = 0;
    m_lastOutput = QStringLiteral("$ записать %1\n\n%2\n\n[exit 0]").arg(name, run.combinedOutput());
    emit outputChanged();

    HistoryEntry entry;
    entry.cmdline = QStringLiteral("write %1").arg(name);
    entry.repoRoot = m_repo.rootPath();
    entry.exitCode = 0;
    entry.ok = true;
    entry.timestamp = QDateTime::currentDateTime().toString(Qt::ISODate);
    entry.commandId = QStringLiteral("github_write_") + kind;
    m_history.prepend(entry);

    m_repo.refresh();
    m_github.setNote(tr("Файл %1 записан в рабочее дерево. Коммит — отдельно.").arg(name));
    return true;
}

void AppController::saveGithubCard(const QString &description, const QString &homepage, const QString &visibility)
{
    QString error;
    if (!ensureGithubRemote(&error)) {
        emit errorOccurred(error);
        return;
    }
    const QString vis = visibility.trimmed().toLower();
    if (vis != QLatin1String("public") && vis != QLatin1String("private") && vis != QLatin1String("internal")) {
        emit errorOccurred(tr("Видимость: public, private или internal."));
        return;
    }
    const bool sameDesc = description == m_github.description();
    const bool sameHome = homepage == m_github.homepage();
    const bool sameVis = vis == m_github.visibility();
    if (sameDesc && sameHome && sameVis) {
        m_github.setNote(tr("Карточка без изменений."));
        return;
    }
    QStringList argv{QStringLiteral("repo"), QStringLiteral("edit"), QStringLiteral("--description"), description,
                     QStringLiteral("--homepage"), homepage};
    if (!sameVis)
        argv << QStringLiteral("--visibility") << vis << QStringLiteral("--accept-visibility-change-consequences");
    beginGh(QStringLiteral("github_describe"), argv);
}

void AppController::renameGithubRepo(const QString &name)
{
    QString error;
    if (!ensureGithubRemote(&error)) {
        emit errorOccurred(error);
        return;
    }
    const QString next = name.trimmed();
    const QRegularExpression re(QStringLiteral("^[A-Za-z0-9._-]{1,100}$"));
    if (!re.match(next).hasMatch()) {
        emit errorOccurred(tr("Имя: буквы, цифры, точка, подчёркивание и дефис."));
        return;
    }
    if (next == m_github.repoName()) {
        m_github.setNote(tr("Имя уже такое."));
        return;
    }
    beginGh(QStringLiteral("github_rename"),
            {QStringLiteral("repo"), QStringLiteral("rename"), QStringLiteral("--yes"), next});
}

void AppController::addGithubTopic(const QString &topic)
{
    const QString text = normalizeTopic(topic, nullptr);
    QString error;
    if (text.isEmpty()) {
        normalizeTopic(topic, &error);
        emit errorOccurred(error);
        return;
    }
    beginGh(QStringLiteral("github_topic"),
            {QStringLiteral("repo"), QStringLiteral("edit"), QStringLiteral("--add-topic"), text});
}

void AppController::removeGithubTopic(const QString &topic)
{
    const QString text = normalizeTopic(topic, nullptr);
    QString error;
    if (text.isEmpty()) {
        normalizeTopic(topic, &error);
        emit errorOccurred(error);
        return;
    }
    beginGh(QStringLiteral("github_topic_remove"),
            {QStringLiteral("repo"), QStringLiteral("edit"), QStringLiteral("--remove-topic"), text});
}

void AppController::saveGithubSettings(bool issues, bool wiki, bool projects, bool discussions, bool deleteOnMerge,
                                       bool asTemplate, const QString &defaultBranch)
{
    QString error;
    if (!ensureGithubRemote(&error)) {
        emit errorOccurred(error);
        return;
    }
    QStringList argv{QStringLiteral("repo"), QStringLiteral("edit")};
    auto flag = [&argv](const char *name, bool want, bool have) {
        if (want == have)
            return;
        argv << QStringLiteral("--%1=%2").arg(QLatin1String(name), want ? QStringLiteral("true") : QStringLiteral("false"));
    };
    flag("enable-issues", issues, m_github.issuesEnabled());
    flag("enable-wiki", wiki, m_github.wikiEnabled());
    flag("enable-projects", projects, m_github.projectsEnabled());
    flag("enable-discussions", discussions, m_github.discussionsEnabled());
    flag("delete-branch-on-merge", deleteOnMerge, m_github.deleteBranchOnMerge());
    flag("template", asTemplate, m_github.isTemplate());
    const QString branch = defaultBranch.trimmed();
    if (!branch.isEmpty() && branch != m_github.defaultBranch()) {
        if (branch.contains(QLatin1Char(' '))) {
            emit errorOccurred(tr("Имя ветки по умолчанию не должно содержать пробел."));
            return;
        }
        argv << QStringLiteral("--default-branch") << branch;
    }
    if (argv.size() == 2) {
        m_github.setNote(tr("Настройки без изменений."));
        return;
    }
    beginGh(QStringLiteral("github_settings"), argv);
}

void AppController::saveGithubPolicy(const QVariantMap &draft)
{
    QString error;
    if (!ensureGithubRemote(&error)) {
        emit errorOccurred(error);
        return;
    }
    const QStringList fields = githubPolicyPatchArgs(githubPolicyFromDraft(draft), m_github.policyState(), &error);
    if (!error.isEmpty()) {
        emit errorOccurred(error);
        return;
    }
    if (fields.isEmpty()) {
        m_github.setNote(tr("Политика слияния без изменений."));
        return;
    }
    QStringList argv{QStringLiteral("api"), QStringLiteral("--method"), QStringLiteral("PATCH"),
                     QStringLiteral("repos/{owner}/{repo}")};
    argv += fields;
    beginGh(QStringLiteral("github_policy"), argv);
}

void AppController::setGithubArchived(bool archived)
{
    QString error;
    if (!ensureGithubRemote(&error)) {
        emit errorOccurred(error);
        return;
    }
    const GithubPolicy policy = m_github.policyState();
    if (!policy.archived.known) {
        emit errorOccurred(tr("Состояние архива ещё не прочитано. Обновите карточку GitHub."));
        return;
    }
    if (archived == policy.archived.value) {
        m_github.setNote(archived ? tr("Репозиторий уже в архиве.") : tr("Репозиторий уже активен."));
        return;
    }
    const QString slug = m_github.slug();
    const QStringList argv{QStringLiteral("repo"), archived ? QStringLiteral("archive") : QStringLiteral("unarchive"),
                           slug, QStringLiteral("--yes")};
    const QString narrative = archived
                                  ? tr("Репозиторий %1 станет архивным: запись закроется для всех, кроме администраторов. "
                                       "Это можно вернуть.")
                                        .arg(slug)
                                  : tr("Репозиторий %1 снова станет активным и примет изменения.").arg(slug);
    beginGh(QStringLiteral("github_archive"), argv, DangerLevel::Destructive, narrative);
}

void AppController::forkGithubRepo(const QString &forkName, const QString &org, bool defaultBranchOnly,
                                  bool rewireOrigin)
{
    QString error;
    if (!ensureGithubRemote(&error)) {
        emit errorOccurred(error);
        return;
    }
    GithubForkRequest request;
    request.slug = m_github.slug();
    request.forkName = forkName;
    request.org = org;
    request.defaultBranchOnly = defaultBranchOnly;
    request.rewireOrigin = rewireOrigin;
    const QStringList argv = githubForkArgv(request, &error);
    if (argv.isEmpty()) {
        emit errorOccurred(error.isEmpty() ? tr("Не удалось собрать команду форка.") : error);
        return;
    }
    const QString narrative = rewireOrigin
                                  ? tr("Будет создан форк %1. Текущий origin переименуется в upstream, а форк станет новым origin.")
                                        .arg(m_github.slug())
                                  : tr("Будет создан форк %1. Локальные remote не изменятся.")
                                        .arg(m_github.slug());
    beginGh(QStringLiteral("github_fork"), argv, DangerLevel::Destructive, narrative);
}

void AppController::transferGithubRepo(const QString &newOwner, const QString &newName, const QString &teamIds,
                                      const QString &typedName)
{
    QString error;
    if (!ensureGithubRemote(&error)) {
        emit errorOccurred(error);
        return;
    }
    const QString owner = m_github.slug().section(QLatin1Char('/'), 0, 0);
    const QStringList argv = githubTransferArgv(owner, m_github.repoName(), typedName, newOwner, newName, teamIds, &error);
    if (argv.isEmpty()) {
        emit errorOccurred(error.isEmpty() ? tr("Не удалось собрать передачу.") : error);
        return;
    }
    const QString narrative = tr("Репозиторий %1 будет передан %2. Если новый владелец — другой пользователь, "
                                 "он должен принять передачу. Локальный origin останется прежним, пока вы сами его не смените.")
                                  .arg(m_github.slug(), newOwner.trimmed());
    beginGh(QStringLiteral("github_transfer"), argv, DangerLevel::Destructive, narrative);
}

void AppController::deleteGithubRepo(const QString &typedName)
{
    QString error;
    if (!ensureGithubRemote(&error)) {
        emit errorOccurred(error);
        return;
    }
    const QStringList argv = githubDeleteArgv(m_github.slug(), m_github.repoName(), typedName, &error);
    if (argv.isEmpty()) {
        emit errorOccurred(error.isEmpty() ? tr("Не удалось собрать удаление.") : error);
        return;
    }
    const QString narrative = tr("Репозиторий %1 будет удалён на GitHub без восстановления. Локальные файлы останутся. "
                                 "Для этого у gh нужен доступ delete_repo. Origin продолжит указывать на удалённый адрес.")
                                  .arg(m_github.slug());
    beginGh(QStringLiteral("github_delete"), argv, DangerLevel::Destructive, narrative);
}

void AppController::setGithubLfs(bool enabled)
{
    QString error;
    if (!ensureGithubRemote(&error)) {
        emit errorOccurred(error);
        return;
    }
    const QString narrative = enabled
                                  ? tr("На %1 включится Git LFS. Хранение больших файлов на GitHub может тарифицироваться. "
                                       "Локальный git-lfs эта кнопка не устанавливает.")
                                        .arg(m_github.slug())
                                  : tr("На %1 выключится Git LFS. Новые большие файлы через LFS туда больше не примутся. "
                                       "Локальные указатели в репозитории останутся.")
                                        .arg(m_github.slug());
    beginGh(QStringLiteral("github_lfs"), githubLfsArgv(enabled), DangerLevel::Destructive, narrative);
}

QString AppController::fetchGithubTemplate(const QString &kind, const QString &key)
{
    const QString text = m_github.fetchTemplate(kind, key);
    if (text.isEmpty())
        emit errorOccurred(m_github.note().isEmpty() ? tr("Шаблон GitHub не найден.") : m_github.note());
    return text;
}

void AppController::saveGithubPages(const QString &branch, const QString &path, const QString &cname, bool https,
                                   bool enabled)
{
    QString error;
    if (!ensureGithubRemote(&error)) {
        emit errorOccurred(error);
        return;
    }
    GithubPagesState want = m_github.pagesState();
    want.known = true;
    want.enabled = enabled;
    want.branch = branch.trimmed();
    want.path = path.trimmed().isEmpty() ? QStringLiteral("/") : path.trimmed();
    want.cname = cname.trimmed();
    want.https = https;
    want.httpsKnown = m_github.pagesState().httpsKnown;
    const QStringList argv = githubPagesArgv(want, m_github.pagesState(), &error);
    if (argv.isEmpty()) {
        if (error.contains(QStringLiteral("без изменений")))
            m_github.setNote(error);
        else
            emit errorOccurred(error.isEmpty() ? tr("Не удалось собрать настройки Pages.") : error);
        return;
    }
    const QString narrative = enabled
                                  ? tr("Сайт GitHub Pages для %1 будет записан из ветки %2, каталог %3.")
                                        .arg(m_github.slug(), want.branch, want.path)
                                  : tr("Сайт GitHub Pages для %1 будет удалён.").arg(m_github.slug());
    beginGh(QStringLiteral("github_pages"), argv, DangerLevel::Destructive, narrative);
}

void AppController::publishWikiPage(const QString &page, const QString &body)
{
    QString error;
    if (!ensureGithubRemote(&error)) {
        emit errorOccurred(error);
        return;
    }
    if (!validWikiPageName(page)) {
        emit errorOccurred(tr("Имя страницы wiki: буквы, цифры, подчёркивание и дефис."));
        return;
    }
    if (githubWikiRemoteUrl(m_github.originUrl()).isEmpty() || githubWikiCacheDir(m_github.slug()).isEmpty()) {
        emit errorOccurred(tr("Не удалось собрать адрес wiki."));
        return;
    }
    const QString narrative = tr("Страница %1 будет закоммичена и отправлена в wiki %2. Локальный репозиторий не изменится.")
                                  .arg(page.trimmed(), m_github.slug());
    beginGh(QStringLiteral("github_wiki"), {QStringLiteral("publish"), page.trimmed()}, DangerLevel::Destructive, narrative,
            QStringLiteral("wiki"));
    if (m_pendingCommandId == QLatin1String("github_wiki")) {
        m_pendingArgs.insert(QStringLiteral("page"), page.trimmed());
        m_pendingArgs.insert(QStringLiteral("body"), body);
    }
}

void AppController::saveGithubIssue(const QString &number, const QString &title, const QString &body)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const QStringList argv = githubIssueWriteArgv(number, title, body, &error);
    dispatchGithub(QStringLiteral("github_svc_issue"), argv, error, DangerLevel::Mutates, {});
}

void AppController::actGithubIssue(const QString &number, const QString &action, const QString &extra)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const QStringList argv = githubIssueActionArgv(number, action, extra, &error);
    const bool destructive = action.trimmed() == QLatin1String("delete");
    const QString narrative = destructive ? tr("Issue #%1 в %2 будет удалён на GitHub.").arg(number.trimmed(), m_github.slug())
                                          : QString();
    dispatchGithub(QStringLiteral("github_svc_issue"), argv, error,
                   destructive ? DangerLevel::Destructive : DangerLevel::Mutates, narrative);
}

void AppController::linkGithubIssue(const QString &number, const QString &kind, const QString &value, bool remove)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const QStringList argv = githubIssueLinkArgv(number, kind, value, remove, &error);
    dispatchGithub(QStringLiteral("github_svc_issue"), argv, error, DangerLevel::Mutates, {});
}

void AppController::createGithubLabel(const QString &name, const QString &color, const QString &description)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const QStringList argv = githubLabelWriteArgv(name, color, description, {}, false, &error);
    dispatchGithub(QStringLiteral("github_svc_label"), argv, error, DangerLevel::Mutates, {});
}

void AppController::editGithubLabel(const QString &name, const QString &color, const QString &description,
                                    const QString &newName)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const QStringList argv = githubLabelWriteArgv(name, color, description, newName, true, &error);
    dispatchGithub(QStringLiteral("github_svc_label"), argv, error, DangerLevel::Mutates, {});
}

void AppController::deleteGithubLabel(const QString &name)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const QStringList argv = githubLabelDeleteArgv(name, &error);
    dispatchGithub(QStringLiteral("github_svc_label"), argv, error, DangerLevel::Destructive,
                   tr("Метка «%1» будет удалена из %2.").arg(name.trimmed(), m_github.slug()));
}

void AppController::saveGithubMilestone(const QString &number, const QString &title, const QString &state)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const QStringList argv = githubMilestoneArgv(number, title, state, &error);
    const bool destructive = state.trimmed() == QLatin1String("delete");
    dispatchGithub(QStringLiteral("github_svc_milestone"), argv, error,
                   destructive ? DangerLevel::Destructive : DangerLevel::Mutates,
                   destructive ? tr("Веха #%1 в %2 будет удалена.").arg(number.trimmed(), m_github.slug()) : QString());
}

void AppController::saveGithubPull(const QString &number, const QString &title, const QString &body, const QString &base,
                                  const QString &head, bool draft)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    const QString headBranch = head.trimmed().isEmpty() ? m_repo.branch() : head;
    QString error;
    const QStringList argv = githubPullWriteArgv(number, title, body, base, headBranch, draft, &error);
    dispatchGithub(QStringLiteral("github_svc_pr"), argv, error, DangerLevel::Mutates, {});
}

void AppController::reviewGithubPull(const QString &number, const QString &action, const QString &body)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const QStringList argv = githubPullReviewArgv(number, action, body, &error);
    dispatchGithub(QStringLiteral("github_svc_pr"), argv, error, DangerLevel::Mutates, {});
}

void AppController::mergeGithubPull(const QString &number, const QString &method)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const QStringList argv = githubPullMergeArgv(number, method, &error);
    dispatchGithub(QStringLiteral("github_svc_pr"), argv, error, DangerLevel::Destructive,
                   tr("Pull request #%1 в %2 будет слит способом %3. Ветки эта команда не удаляет.")
                       .arg(number.trimmed(), m_github.slug(), method.trimmed()));
}

void AppController::readyGithubPull(const QString &number, bool draft)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const QStringList argv = githubPullReadyArgv(number, draft, &error);
    dispatchGithub(QStringLiteral("github_svc_pr"), argv, error, DangerLevel::Mutates, {});
}

void AppController::updateGithubPullBranch(const QString &number, bool rebase)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const QStringList argv = githubPullUpdateArgv(number, rebase, &error);
    dispatchGithub(QStringLiteral("github_svc_pr"), argv, error, DangerLevel::Destructive,
                   rebase ? tr("Ветка pull request #%1 будет перебазирована на базовую ветку.").arg(number.trimmed())
                          : tr("В ветку pull request #%1 вольётся базовая ветка.").arg(number.trimmed()));
}

void AppController::saveGithubDiscussion(const QString &number, const QString &title, const QString &body,
                                        const QString &category)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const QStringList argv = githubDiscussionWriteArgv(number, title, body, category, &error);
    dispatchGithub(QStringLiteral("github_svc_discussion"), argv, error, DangerLevel::Mutates, {});
}

void AppController::commentGithubDiscussion(const QString &target, const QString &body, bool remove)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const QStringList argv = githubDiscussionCommentArgv(target, body, remove, &error);
    dispatchGithub(QStringLiteral("github_svc_discussion"), argv, error,
                   remove ? DangerLevel::Destructive : DangerLevel::Mutates,
                   remove ? tr("Комментарий обсуждения будет удалён.") : QString());
}

void AppController::createGithubRelease(const QString &tag, const QString &title, const QString &notes, bool draft,
                                       bool prerelease, const QString &target)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const QStringList argv = githubReleaseCreateArgv(tag, title, notes, draft, prerelease, target, &error);
    dispatchGithub(QStringLiteral("github_svc_release"), argv, error, DangerLevel::Destructive,
                   tr("Будет создан релиз %1 в %2. Если такого тега на GitHub ещё нет, gh создаст его.")
                       .arg(tag.trimmed(), m_github.slug()));
}

void AppController::editGithubRelease(const QString &tag, const QString &title, const QString &notes, bool draft,
                                     bool prerelease, const QString &target)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const QStringList argv = githubReleaseEditArgv(tag, title, notes, draft, prerelease, target, &error);
    dispatchGithub(QStringLiteral("github_svc_release"), argv, error, DangerLevel::Destructive,
                   tr("Текст и состояние релиза %1 в %2 будут перезаписаны.").arg(tag.trimmed(), m_github.slug()));
}

void AppController::deleteGithubRelease(const QString &tag, bool cleanupTag)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const QStringList argv = githubReleaseDeleteArgv(tag, cleanupTag, &error);
    dispatchGithub(QStringLiteral("github_svc_release"), argv, error, DangerLevel::Destructive,
                   cleanupTag ? tr("Релиз %1 и его тег будут удалены на GitHub.").arg(tag.trimmed())
                              : tr("Релиз %1 будет удалён. Тег git останется.").arg(tag.trimmed()));
}

void AppController::uploadGithubRelease(const QString &tag, const QString &filePath)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const QStringList argv = githubReleaseUploadArgv(tag, filePath, &error);
    dispatchGithub(QStringLiteral("github_svc_release"), argv, error, DangerLevel::Destructive,
                   tr("Файл будет прикреплён к релизу %1. Существующий файл с тем же именем не затирается.")
                       .arg(tag.trimmed()));
}

void AppController::createGithubAutolink(const QString &prefix, const QString &urlTemplate, bool numeric)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const QStringList argv = githubAutolinkCreateArgv(prefix, urlTemplate, numeric, &error);
    dispatchGithub(QStringLiteral("github_svc_autolink"), argv, error, DangerLevel::Mutates, {});
}

void AppController::deleteGithubAutolink(const QString &id)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const QStringList argv = githubAutolinkDeleteArgv(id, &error);
    dispatchGithub(QStringLiteral("github_svc_autolink"), argv, error, DangerLevel::Destructive,
                   tr("Автоссылка #%1 в %2 будет удалена.").arg(id.trimmed(), m_github.slug()));
}

void AppController::createGithubProject(const QString &title)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const QStringList argv = githubProjectCreateArgv(githubProjectOwner(), title, &error);
    dispatchGithub(QStringLiteral("github_svc_project"), argv, error, DangerLevel::Mutates, {});
}

void AppController::createGithubProjectField(const QString &number, const QString &name, const QString &dataType,
                                            const QString &options)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const QStringList argv = githubProjectFieldArgv(number, githubProjectOwner(), name, dataType, options, &error);
    dispatchGithub(QStringLiteral("github_svc_project"), argv, error, DangerLevel::Mutates, {});
}

void AppController::createGithubProjectItem(const QString &number, const QString &title, const QString &body)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const QStringList argv = githubProjectItemArgv(number, githubProjectOwner(), title, body, &error);
    dispatchGithub(QStringLiteral("github_svc_project"), argv, error, DangerLevel::Mutates, {});
}

void AppController::addGithubProjectItem(const QString &number, const QString &url)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const QStringList argv = githubProjectAddArgv(number, githubProjectOwner(), url, &error);
    dispatchGithub(QStringLiteral("github_svc_project"), argv, error, DangerLevel::Mutates, {});
}

void AppController::editGithubProjectItem(const QString &number, const QString &url, const QString &field,
                                         const QString &value)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const QStringList argv = githubProjectEditArgv(number, githubProjectOwner(), url, field, value, &error);
    dispatchGithub(QStringLiteral("github_svc_project"), argv, error, DangerLevel::Mutates, {});
}

void AppController::linkGithubProject(const QString &number)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const QStringList argv = githubProjectLinkArgv(number, githubProjectOwner(), &error);
    dispatchGithub(QStringLiteral("github_svc_project"), argv, error, DangerLevel::Mutates, {});
}

void AppController::createGithubProjectView(const QString &number, const QString &name, const QString &layout)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const QString id = m_github.projectNodeId(number, &error);
    if (id.isEmpty()) {
        emit errorOccurred(error.isEmpty() ? tr("Проект не найден. Для проектов нужен scope project.") : error);
        return;
    }
    const QStringList argv = githubProjectViewArgv(id, name, layout, &error);
    dispatchGithub(QStringLiteral("github_svc_project"), argv, error, DangerLevel::Mutates, {});
}

void AppController::editGithubProjectView(const QString &number, const QString &viewId, const QString &name, bool remove)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const QString id = m_github.projectNodeId(number, &error);
    if (id.isEmpty()) {
        emit errorOccurred(error.isEmpty() ? tr("Проект не найден. Для проектов нужен scope project.") : error);
        return;
    }
    const QStringList argv = githubProjectViewEditArgv(id, viewId, name, remove, &error);
    dispatchGithub(QStringLiteral("github_svc_project"), argv, error, remove ? DangerLevel::Destructive : DangerLevel::Mutates,
                   remove ? tr("Вид %1 будет удалён.").arg(viewId.trimmed()) : QString());
}

void AppController::saveGithubCollaborator(const QString &login, const QString &permission, bool remove)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const QStringList argv = githubCollaboratorArgv(login, permission, remove, &error);
    dispatchGithub(QStringLiteral("github_access_collab"), argv, error, DangerLevel::Destructive,
                   remove ? tr("Соавтор %1 потеряет доступ к %2.").arg(login.trimmed(), m_github.slug())
                          : tr("Пользователь %1 получит право %2 в %3.")
                                .arg(login.trimmed(), permission.trimmed(), m_github.slug()));
}

void AppController::acceptGithubInvitation(const QString &id)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const QStringList argv = githubInvitationAcceptArgv(id, &error);
    dispatchGithub(QStringLiteral("github_access_collab"), argv, error, DangerLevel::Destructive,
                   tr("Приглашение %1 будет принято этой учётной записью.").arg(id.trimmed()));
}

void AppController::deleteGithubInvitation(const QString &id)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const QStringList argv = githubInvitationDeleteArgv(id, &error);
    dispatchGithub(QStringLiteral("github_access_collab"), argv, error, DangerLevel::Destructive,
                   tr("Приглашение #%1 в %2 будет отозвано.").arg(id.trimmed(), m_github.slug()));
}

void AppController::actGithubWorkflow(const QString &action, const QString &workflow, const QString &ref,
                                     const QString &inputKey, const QString &inputValue)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const QStringList argv = githubWorkflowArgv(action, workflow, ref, inputKey, inputValue, &error);
    const bool run = action.trimmed() == QLatin1String("run");
    dispatchGithub(QStringLiteral("github_access_actions"), argv, error,
                   run ? DangerLevel::Destructive : DangerLevel::Mutates,
                   run ? tr("Workflow %1 будет запущен в %2.").arg(workflow.trimmed(), m_github.slug()) : QString());
}

void AppController::actGithubRun(const QString &action, const QString &runId, bool failedOnly)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const QStringList argv = githubRunArgv(action, runId, failedOnly, &error);
    const QString act = action.trimmed();
    const bool destructive = act == QLatin1String("delete") || act == QLatin1String("rerun");
    QString narrative;
    if (act == QLatin1String("delete"))
        narrative = tr("Запуск Actions #%1 будет удалён.").arg(runId.trimmed());
    else if (act == QLatin1String("rerun"))
        narrative = tr("Запуск Actions #%1 будет повторён.").arg(runId.trimmed());
    dispatchGithub(QStringLiteral("github_access_actions"), argv, error,
                   destructive ? DangerLevel::Destructive : DangerLevel::Mutates, narrative);
}

void AppController::saveGithubVariable(const QString &name, const QString &value, const QString &envName, bool remove)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const QStringList argv = githubVariableArgv(name, value, envName, remove, &error);
    dispatchGithub(QStringLiteral("github_access_actions"), argv, error,
                   remove ? DangerLevel::Destructive : DangerLevel::Mutates,
                   remove ? tr("Переменная %1 будет удалена.").arg(name.trimmed()) : QString());
}

void AppController::saveGithubSecret(const QString &name, const QString &value, const QString &envName, const QString &app,
                                    bool remove)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const GithubHiddenCommand command = githubSecretCommand(name, value, envName, app, remove, &error);
    dispatchGithub(QStringLiteral("github_access_actions"), command.argv, error, DangerLevel::Destructive,
                   remove ? tr("Секрет %1 будет удалён. Значение в журнал не пишется.").arg(name.trimmed())
                          : tr("Секрет %1 будет записан. Значение не попадает в превью и журнал.").arg(name.trimmed()),
                   command.stdinText);
}

void AppController::saveGithubEnvironment(const QString &name, const QString &waitMinutes, bool protectedBranches,
                                         const QString &reviewers, bool remove)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    bool ok = false;
    const int minutes = waitMinutes.trimmed().isEmpty() ? 0 : waitMinutes.trimmed().toInt(&ok);
    if (!waitMinutes.trimmed().isEmpty() && !ok) {
        emit errorOccurred(tr("Ожидание окружения — целое число минут."));
        return;
    }
    QString error;
    const QStringList argv = githubEnvironmentArgv(name, minutes, protectedBranches, reviewers, remove, &error);
    dispatchGithub(QStringLiteral("github_access_actions"), argv, error,
                   remove ? DangerLevel::Destructive : DangerLevel::Mutates,
                   remove ? tr("Окружение %1 будет удалено.").arg(name.trimmed()) : QString());
}

void AppController::deleteGithubCache(const QString &keyOrId)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const QStringList argv = githubCacheDeleteArgv(keyOrId, &error);
    dispatchGithub(QStringLiteral("github_access_actions"), argv, error, DangerLevel::Destructive,
                   tr("Кэш Actions «%1» будет удалён. Все кэши этой кнопкой не удаляются.").arg(keyOrId.trimmed()));
}

void AppController::saveGithubAlert(const QString &kind, const QString &number, const QString &state, const QString &reason)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const QStringList argv = githubAlertArgv(kind, number, state, reason, &error);
    dispatchGithub(QStringLiteral("github_access_security"), argv, error, DangerLevel::Mutates, {});
}

void AppController::saveGithubRuleset(const QString &id, const QString &name, const QString &includeRef,
                                     const QString &enforcement, const QString &rules)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const GithubHiddenCommand command = githubRulesetCommand(id, name, includeRef, enforcement, rules, &error);
    dispatchGithub(QStringLiteral("github_access_security"), command.argv, error, DangerLevel::Destructive,
                   tr("Ruleset «%1» для %2, режим %3, правила: %4. Требование ревью в этот набор не входит.")
                       .arg(name.trimmed(), includeRef.trimmed(), enforcement.trimmed(), rules.trimmed()),
                   command.stdinText);
}

void AppController::saveGithubBranchProtection(const QString &branch, int reviews, bool enforceAdmins, bool blockForce,
                                              bool blockDelete, bool remove)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const QStringList argv = githubBranchProtectionArgv(branch, reviews, enforceAdmins, blockForce, blockDelete, remove, &error);
    dispatchGithub(QStringLiteral("github_access_security"), argv, error, DangerLevel::Destructive,
                   remove ? tr("Классическая защита ветки %1 будет снята.").arg(branch.trimmed())
                          : tr("Классическая защита ветки %1 будет перезаписана. Линейная история здесь не включается.")
                                .arg(branch.trimmed()));
}

void AppController::saveGithubDeployKey(const QString &title, const QString &key, bool readOnly)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const QStringList argv = githubDeployKeyArgv(title, key, readOnly, &error);
    dispatchGithub(QStringLiteral("github_access_security"), argv, error, DangerLevel::Destructive,
                   readOnly ? tr("В %1 будет добавлен deploy key «%2» только для чтения.").arg(m_github.slug(), title.trimmed())
                            : tr("В %1 будет добавлен deploy key «%2» с правом записи.").arg(m_github.slug(), title.trimmed()));
}

void AppController::deleteGithubDeployKey(const QString &id)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const QStringList argv = githubDeployKeyDeleteArgv(id, &error);
    dispatchGithub(QStringLiteral("github_access_security"), argv, error, DangerLevel::Destructive,
                   tr("Deploy key #%1 будет удалён.").arg(id.trimmed()));
}

void AppController::saveGithubWebhook(const QString &id, const QString &url, const QString &events, const QString &secret,
                                     bool active, bool remove)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const GithubHiddenCommand command = githubWebhookCommand(id, url, events, secret, active, remove, &error);
    dispatchGithub(QStringLiteral("github_access_security"), command.argv, error, DangerLevel::Destructive,
                   remove ? tr("Вебхук #%1 будет удалён.").arg(id.trimmed())
                          : tr("Вебхук %1, события %2. Секрет в превью и журнал не пишется.")
                                .arg(url.trimmed(), events.trimmed()),
                   command.stdinText);
}

void AppController::saveGithubAdvisory(const QString &ghsa, const QString &summary, const QString &description,
                                      const QString &severity, const QString &ecosystem, const QString &packageName,
                                      const QString &versionRange)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const QStringList argv = githubAdvisoryArgv(ghsa, summary, description, severity, ecosystem, packageName, versionRange, &error);
    const bool create = ghsa.trimmed().isEmpty();
    dispatchGithub(QStringLiteral("github_access_security"), argv, error,
                   create ? DangerLevel::Destructive : DangerLevel::Mutates,
                   create ? tr("Будет создан черновик совета безопасности. Публикация отсюда не отправляется.") : QString());
}

void AppController::saveGithubInteraction(const QString &limit, bool clear)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const QStringList argv = githubInteractionArgv(limit, clear, &error);
    dispatchGithub(QStringLiteral("github_access_security"), argv, error, DangerLevel::Mutates, {});
}

void AppController::createGithubCodespace(const QString &branch, const QString &displayName)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const QStringList argv = githubCodespaceCreateArgv(m_github.slug(), branch, displayName, &error);
    dispatchGithub(QStringLiteral("github_access_codespace"), argv, error, DangerLevel::Destructive,
                   tr("Будет создан codespace «%1» из ветки %2. Это может тарифицироваться.").arg(displayName.trimmed(), branch.trimmed()));
}

void AppController::editGithubCodespace(const QString &name, const QString &displayName)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const QStringList argv = githubCodespaceEditArgv(name, displayName, &error);
    dispatchGithub(QStringLiteral("github_access_codespace"), argv, error, DangerLevel::Mutates, {});
}

void AppController::deleteGithubCodespace(const QString &name)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const QStringList argv = githubCodespaceDeleteArgv(name, &error);
    dispatchGithub(QStringLiteral("github_access_codespace"), argv, error, DangerLevel::Destructive,
                   tr("Codespace %1 будет удалён. Остальные codespace не трогаются.").arg(name.trimmed()));
}

void AppController::saveGithubProfile(const QString &name, const QString &bio, const QString &blog, const QString &company,
                                     const QString &location, const QString &twitter, bool hireable)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const GithubHiddenCommand command = githubProfileCommand(name, bio, blog, company, location, twitter, hireable, &error);
    dispatchGithub(QStringLiteral("github_identity_profile"), command.argv, error, DangerLevel::Mutates, {}, command.stdinText);
}

void AppController::saveGithubEmail(const QString &email, const QString &action)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const GithubHiddenCommand command = githubEmailCommand(email, action, &error);
    const bool drop = action.trimmed() == QLatin1String("delete");
    dispatchGithub(QStringLiteral("github_identity_profile"), command.argv, error,
                   drop ? DangerLevel::Destructive : DangerLevel::Mutates,
                   drop ? tr("Адрес %1 будет снят с учётной записи.").arg(email.trimmed()) : QString(), command.stdinText);
}

void AppController::saveGithubSocial(const QString &url, bool remove)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const GithubHiddenCommand command = githubSocialCommand(url, remove, &error);
    dispatchGithub(QStringLiteral("github_identity_profile"), command.argv, error,
                   remove ? DangerLevel::Destructive : DangerLevel::Mutates,
                   remove ? tr("Соцаккаунт %1 будет снят с профиля.").arg(url.trimmed()) : QString(), command.stdinText);
}

void AppController::saveGithubUserKey(const QString &kind, const QString &title, const QString &key, const QString &id,
                                     bool remove)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const GithubHiddenCommand command = githubUserKeyCommand(kind, title, key, id, remove, &error);
    const QString narrative = remove ? tr("Ключ %1 номер %2 будет удалён.").arg(kind.trimmed(), id.trimmed())
                                     : tr("Будет добавлен открытый ключ «%1» (%2). Закрытый ключ в команду не входит.")
                                           .arg(title.trimmed(), kind.trimmed());
    dispatchGithub(QStringLiteral("github_identity_profile"), command.argv, error, DangerLevel::Destructive, narrative,
                   command.stdinText);
}

void AppController::createGithubProfileRepo(const QString &login)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const QStringList argv = githubProfileRepoArgv(login, &error);
    dispatchGithub(QStringLiteral("github_identity_profile"), argv, error, DangerLevel::Destructive,
                   tr("Будет создан публичный репозиторий %1/%1. README профиля работает, только если это логин учётной записи gh.")
                       .arg(login.trimmed()));
}

void AppController::saveGithubProfileReadme(const QString &login, const QString &text, const QString &sha)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const GithubHiddenCommand command = githubProfileReadmeCommand(login, text, sha, &error);
    dispatchGithub(QStringLiteral("github_identity_profile"), command.argv, error, DangerLevel::Destructive,
                   tr("README профиля %1 будет записан коммитом на GitHub (%2 символов). Открытый репозиторий не меняется.")
                       .arg(login.trimmed())
                       .arg(text.size()),
                   command.stdinText);
}

void AppController::saveGithubOrgCard(const QString &org, const QString &name, const QString &description, const QString &blog,
                                     const QString &email, const QString &twitter, const QString &location, const QString &company)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const GithubHiddenCommand command =
        githubOrgCardCommand(org, name, description, blog, email, twitter, location, company, &error);
    dispatchGithub(QStringLiteral("github_identity_org"), command.argv, error, DangerLevel::Mutates, {}, command.stdinText);
}

void AppController::saveGithubOrgPolicy(const QString &org, const QString &permission, bool publicRepos, bool privateRepos,
                                       bool internalRepos, bool pages, bool publicPages, bool privatePages, bool forkPrivate)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const GithubHiddenCommand command = githubOrgPolicyCommand(org, permission, publicRepos, privateRepos, internalRepos, pages,
                                                              publicPages, privatePages, forkPrivate, &error);
    dispatchGithub(QStringLiteral("github_identity_org"), command.argv, error, DangerLevel::Destructive,
                   tr("Политика %1: право %2, public %3, private %4, internal %5, Pages %6, форк приватных %7.")
                       .arg(org.trimmed(), permission.trimmed(), publicRepos ? tr("да") : tr("нет"),
                            privateRepos ? tr("да") : tr("нет"), internalRepos ? tr("да") : tr("нет"),
                            pages ? tr("да") : tr("нет"), forkPrivate ? tr("да") : tr("нет")),
                   command.stdinText);
}

void AppController::saveGithubOrgTeam(const QString &org, const QString &name, const QString &description,
                                     const QString &privacy, const QString &slug, bool remove)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const GithubHiddenCommand command = githubOrgTeamCommand(org, name, description, privacy, slug, remove, &error);
    dispatchGithub(QStringLiteral("github_identity_org"), command.argv, error,
                   remove ? DangerLevel::Destructive : DangerLevel::Mutates,
                   remove ? tr("Команда %1/%2 будет удалена.").arg(org.trimmed(), slug.trimmed()) : QString(),
                   command.stdinText);
}

void AppController::saveGithubOrgTeamMember(const QString &org, const QString &slug, const QString &login, const QString &role,
                                           bool remove)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const GithubHiddenCommand command = githubOrgTeamMemberCommand(org, slug, login, role, remove, &error);
    dispatchGithub(QStringLiteral("github_identity_org"), command.argv, error,
                   remove ? DangerLevel::Destructive : DangerLevel::Mutates,
                   remove ? tr("%1 будет снят с команды %2.").arg(login.trimmed(), slug.trimmed()) : QString(),
                   command.stdinText);
}

void AppController::saveGithubOrgPeople(const QString &org, const QString &login, const QString &email,
                                       const QString &invitationId, const QString &role, const QString &action)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const GithubHiddenCommand command = githubOrgPeopleCommand(org, login, email, invitationId, role, action, &error);
    const QString act = action.trimmed();
    QString narrative = tr("Состав организации %1 изменится.").arg(org.trimmed());
    if (act == QLatin1String("invite"))
        narrative = tr("На %1 уйдёт приглашение %2 с ролью %3.").arg(org.trimmed(), email.trimmed(), role.trimmed());
    else if (act == QLatin1String("remove-member"))
        narrative = tr("%1 будет исключён из организации %2.").arg(login.trimmed(), org.trimmed());
    else if (act == QLatin1String("remove-outside"))
        narrative = tr("Внешний коллаборатор %1 будет снят с %2.").arg(login.trimmed(), org.trimmed());
    else if (act == QLatin1String("cancel"))
        narrative = tr("Приглашение %1 в %2 будет отозвано.").arg(invitationId.trimmed(), org.trimmed());
    dispatchGithub(QStringLiteral("github_identity_org"), command.argv, error, DangerLevel::Destructive, narrative,
                   command.stdinText);
}

void AppController::saveGithubOrgRole(const QString &org, const QString &roleId, const QString &name, const QString &description,
                                     const QString &baseRole, const QString &permissions, bool remove)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const GithubHiddenCommand command = githubOrgRoleCommand(org, roleId, name, description, baseRole, permissions, remove, &error);
    const QString narrative = remove ? tr("Роль %1 в %2 будет удалена. Назначенные получат базовую роль.")
                                           .arg(roleId.trimmed(), org.trimmed())
                                     : tr("Роль «%1» в %2 получит базу %3 и права %4.")
                                           .arg(name.trimmed(), org.trimmed(), baseRole.trimmed(), permissions.trimmed());
    dispatchGithub(QStringLiteral("github_identity_org"), command.argv, error, DangerLevel::Destructive, narrative,
                   command.stdinText);
}

void AppController::saveGithubOrgProperty(const QString &org, const QString &name, const QString &valueType,
                                         const QString &description, const QString &allowedCsv, bool remove)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const GithubHiddenCommand command = githubOrgPropertyCommand(org, name, valueType, description, allowedCsv, remove, &error);
    dispatchGithub(QStringLiteral("github_identity_org"), command.argv, error, DangerLevel::Destructive,
                   remove ? tr("Свойство %1 будет удалено из %2.").arg(name.trimmed(), org.trimmed())
                          : tr("Свойство %1 типа %2 будет записано в %3. Оно не обязательно и правят его только роли организации.")
                                .arg(name.trimmed(), valueType.trimmed(), org.trimmed()),
                   command.stdinText);
}

void AppController::saveGithubOrgHook(const QString &org, const QString &id, const QString &url, const QString &events,
                                     const QString &secret, bool active, bool remove)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const GithubHiddenCommand command = githubOrgHookCommand(org, id, url, events, secret, active, remove, &error);
    const QString narrative = remove ? tr("Вебхук %1 организации %2 будет удалён.").arg(id.trimmed(), org.trimmed())
                                     : tr("Вебхук организации %1: %2. Секрет в превью и журнал не попадает.").arg(org.trimmed(), url.trimmed());
    dispatchGithub(QStringLiteral("github_identity_org"), command.argv, error, DangerLevel::Destructive, narrative,
                   command.stdinText);
}

void AppController::saveGithubOrgBanner(const QString &org, const QString &text, const QString &expiresAt, bool dismissible,
                                     bool remove)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const GithubHiddenCommand command = githubOrgBannerCommand(org, text, expiresAt, dismissible, remove, &error);
    dispatchGithub(QStringLiteral("github_identity_org"), command.argv, error,
                   remove ? DangerLevel::Destructive : DangerLevel::Mutates,
                   remove ? tr("Баннер организации %1 будет снят.").arg(org.trimmed()) : QString(), command.stdinText);
}

void AppController::saveGithubOrgSecurityConfig(const QString &org, const QString &name, const QString &dependabot,
                                               const QString &secretScanning, const QString &pushProtection,
                                               const QString &configId, bool remove)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const GithubHiddenCommand command =
        githubOrgSecurityConfigCommand(org, name, dependabot, secretScanning, pushProtection, configId, remove, &error);
    dispatchGithub(QStringLiteral("github_identity_org"), command.argv, error, DangerLevel::Destructive,
                   remove ? tr("Конфигурация безопасности %1 будет удалена.").arg(configId.trimmed())
                          : tr("Будет создана конфигурация «%1». На репозитории она сама не назначается.").arg(name.trimmed()),
                   command.stdinText);
}

void AppController::saveGithubOrgSecurityManager(const QString &org, const QString &teamSlug, bool remove)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const QStringList argv = githubOrgSecurityManagerArgv(org, teamSlug, remove, &error);
    dispatchGithub(QStringLiteral("github_identity_org"), argv, error, DangerLevel::Destructive,
                   remove ? tr("Команда %1 перестанет быть security manager.").arg(teamSlug.trimmed())
                          : tr("Команда %1 станет security manager организации %2.").arg(teamSlug.trimmed(), org.trimmed()));
}

void AppController::assignGithubOrgRole(const QString &org, const QString &login, const QString &roleId, bool remove)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const QStringList argv = githubOrgRoleAssignArgv(org, login, roleId, remove, &error);
    dispatchGithub(QStringLiteral("github_identity_org"), argv, error, DangerLevel::Destructive,
                   remove ? tr("Роль %1 будет снята с %2.").arg(roleId.trimmed(), login.trimmed())
                          : tr("%1 получит роль организации %2.").arg(login.trimmed(), roleId.trimmed()));
}

void AppController::attachGithubOrgSecurity(const QString &org, const QString &configId, const QString &repoIds)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const GithubHiddenCommand command = githubOrgSecurityAttachCommand(org, configId, repoIds, &error);
    dispatchGithub(QStringLiteral("github_identity_org"), command.argv, error, DangerLevel::Destructive,
                   tr("Конфигурация %1 будет назначена только на репозитории %2. На все репозитории она не вешается.")
                       .arg(configId.trimmed(), repoIds.trimmed()),
                   command.stdinText);
}

void AppController::saveGithubOrgImmutable(const QString &org, const QString &policy, const QString &repoIds)
{
    QString remoteError;
    if (!ensureGithubRemote(&remoteError)) {
        emit errorOccurred(remoteError);
        return;
    }
    QString error;
    const GithubHiddenCommand command = githubOrgImmutableCommand(org, policy, repoIds, &error);
    dispatchGithub(QStringLiteral("github_identity_org"), command.argv, error, DangerLevel::Destructive,
                   tr("Immutable releases для %1: %2%3.")
                       .arg(org.trimmed(), policy.trimmed(),
                            repoIds.trimmed().isEmpty() ? QString() : tr(", репозитории %1").arg(repoIds.trimmed())),
                   command.stdinText);
}

void AppController::loadCommitShow(const QString &hash)
{
    if (!m_repo.loadCommitShow(hash))
        emit errorOccurred(tr("Не удалось показать коммит %1").arg(hash));
}

void AppController::loadStashShow(const QString &ref)
{
    if (!m_repo.loadStashShow(ref))
        emit errorOccurred(tr("Не удалось показать stash %1").arg(ref));
}

void AppController::loadFileBlame(const QString &path)
{
    if (!m_repo.loadFileBlame(path))
        emit errorOccurred(tr("Не удалось выполнить blame для %1").arg(path));
}

void AppController::loadFileHistory(const QString &path)
{
    if (!m_repo.loadFileHistory(path))
        emit errorOccurred(tr("Не удалось загрузить историю %1").arg(path));
}

void AppController::loadFileAttributes(const QString &path)
{
    if (!m_repo.loadFileAttributes(path))
        emit errorOccurred(tr("Не удалось прочитать атрибуты %1").arg(path));
}

void AppController::loadCleanPreview()
{
    if (!m_repo.loadCleanPreview())
        emit errorOccurred(tr("Не удалось выполнить clean dry-run."));
}

void AppController::copyText(const QString &text)
{
    if (text.isEmpty())
        return;
    QGuiApplication::clipboard()->setText(text);
}

void AppController::copyPath(const QString &relativePath)
{
    if (!m_repo.isOpen() || relativePath.trimmed().isEmpty()) {
        emit errorOccurred(tr("Нет пути для копирования."));
        return;
    }
    const QString abs = QDir(m_repo.rootPath()).filePath(relativePath.trimmed());
    QGuiApplication::clipboard()->setText(abs);
    emit infoOccurred(tr("Путь скопирован"));
}

void AppController::revealInFolder(const QString &relativePath)
{
    if (!m_repo.isOpen() || relativePath.trimmed().isEmpty()) {
        emit errorOccurred(tr("Нет файла для открытия."));
        return;
    }
    const QString abs = QDir(m_repo.rootPath()).filePath(relativePath.trimmed());
    const QFileInfo info(abs);
    const QString target = info.exists()
                               ? (info.isDir() ? info.absoluteFilePath() : info.absolutePath())
                               : QFileInfo(QDir(m_repo.rootPath()).absolutePath()).absoluteFilePath();
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(target)))
        emit errorOccurred(tr("Не удалось открыть каталог."));
}

void AppController::openInEditor(const QString &relativePath)
{
    if (!m_repo.isOpen() || relativePath.trimmed().isEmpty()) {
        emit errorOccurred(tr("Нет файла для редактора."));
        return;
    }
    const QString abs = QDir(m_repo.rootPath()).filePath(relativePath.trimmed());
    if (!QFileInfo::exists(abs)) {
        emit errorOccurred(tr("Файл не найден: %1").arg(relativePath));
        return;
    }
    if (!QDesktopServices::openUrl(QUrl::fromLocalFile(abs)))
        emit errorOccurred(tr("Не удалось открыть файл во внешнем приложении."));
}

QString AppController::suggestCommitMessage()
{
    if (!m_repo.isOpen())
        return {};

    QStringList stagedPaths;
    StatusFileModel *files = m_repo.files();
    for (int i = 0; i < files->rowCount(); ++i) {
        const QModelIndex idx = files->index(i);
        const QString state = files->data(idx, StatusFileModel::StateRole).toString();
        if (state == QLatin1String("staged") || state == QLatin1String("mixed")
            || state == QLatin1String("conflict")) {
            stagedPaths << files->data(idx, StatusFileModel::PathRole).toString();
        }
    }
    if (stagedPaths.isEmpty())
        return {};

    auto baseName = [](const QString &p) {
        const int slash = p.lastIndexOf(QLatin1Char('/'));
        return slash >= 0 ? p.mid(slash + 1) : p;
    };

    auto pickPrefix = [](const QStringList &paths) -> QString {
        for (const QString &p : paths) {
            const QString lower = p.toLower();
            if (lower.contains(QLatin1String("test")) || lower.endsWith(QLatin1String("_test.go"))
                || lower.endsWith(QLatin1String(".test.ts")) || lower.endsWith(QLatin1String(".spec.ts")))
                return QStringLiteral("test: ");
            if (lower.contains(QLatin1String("readme")) || lower.contains(QLatin1String("docs/"))
                || lower.endsWith(QLatin1String(".md")))
                return QStringLiteral("docs: ");
            if (lower.contains(QLatin1String("fix")) || lower.contains(QLatin1String("bug")))
                return QStringLiteral("fix: ");
        }
        return QStringLiteral("chore: ");
    };

    const QString prefix = pickPrefix(stagedPaths);
    if (stagedPaths.size() == 1)
        return prefix + baseName(stagedPaths.first());
    if (stagedPaths.size() <= 3) {
        QStringList names;
        for (const QString &p : stagedPaths)
            names << baseName(p);
        return prefix + names.join(QStringLiteral(", "));
    }
    return prefix + tr("обновить %1 файл(ов)").arg(stagedPaths.size());
}

void AppController::openTerminalInRepo()
{
    if (!m_repo.isOpen()) {
        emit errorOccurred(tr("Репозиторий не открыт."));
        return;
    }
    const QString cwd = m_repo.rootPath();
    auto tryStart = [&](const QString &program, const QStringList &args) -> bool {
        if (program.isEmpty() || QStandardPaths::findExecutable(program).isEmpty())
            return false;
        return QProcess::startDetached(program, args, cwd);
    };

    const QString termEnv = QString::fromLocal8Bit(qgetenv("TERMINAL")).trimmed();
    if (!termEnv.isEmpty() && tryStart(termEnv, {})) {
        emit infoOccurred(tr("Терминал: %1").arg(termEnv));
        return;
    }
    if (tryStart(QStringLiteral("xdg-terminal-exec"), {})) {
        emit infoOccurred(tr("Терминал открыт."));
        return;
    }
    if (tryStart(QStringLiteral("konsole"), {QStringLiteral("--workdir"), cwd})) {
        emit infoOccurred(tr("Терминал: konsole"));
        return;
    }
    if (tryStart(QStringLiteral("gnome-terminal"), {QStringLiteral("--working-directory"), cwd})) {
        emit infoOccurred(tr("Терминал: gnome-terminal"));
        return;
    }
    if (tryStart(QStringLiteral("xfce4-terminal"), {QStringLiteral("--working-directory"), cwd})) {
        emit infoOccurred(tr("Терминал: xfce4-terminal"));
        return;
    }
    if (tryStart(QStringLiteral("xterm"), {})) {
        emit infoOccurred(tr("Терминал: xterm"));
        return;
    }
    emit errorOccurred(tr("Не найден терминал (TERMINAL / xdg-terminal-exec / konsole…)."));
}

bool AppController::appendGitignore(const QString &relativePath)
{
    if (!m_repo.isOpen() || relativePath.trimmed().isEmpty()) {
        emit errorOccurred(tr("Нет пути для .gitignore."));
        return false;
    }
    const QString pattern = relativePath.trimmed();
    const QString giPath = m_repo.rootPath() + QStringLiteral("/.gitignore");
    QFile f(giPath);
    QString existing;
    if (f.exists()) {
        if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
            emit errorOccurred(tr("Не удалось прочитать .gitignore."));
            return false;
        }
        existing = QString::fromUtf8(f.readAll());
        f.close();
        const QStringList lines = existing.split(QLatin1Char('\n'));
        for (const QString &line : lines) {
            if (line.trimmed() == pattern) {
                emit infoOccurred(tr("Уже есть в .gitignore: %1").arg(pattern));
                return true;
            }
        }
    }
    if (!f.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
        emit errorOccurred(tr("Не удалось записать .gitignore."));
        return false;
    }
    QTextStream out(&f);
    if (!existing.isEmpty() && !existing.endsWith(QLatin1Char('\n')))
        out << QLatin1Char('\n');
    out << pattern << QLatin1Char('\n');
    f.close();
    m_lastOutput = QStringLiteral("$ echo '%1' >> .gitignore\n\nOK").arg(pattern);
    m_lastExitCode = 0;
    emit outputChanged();
    m_repo.refresh();
    emit infoOccurred(tr("Добавлено в .gitignore: %1").arg(pattern));
    return true;
}

void AppController::toggleFavorite(const QString &commandId)
{
    m_favorites.toggle(commandId);
}

QString AppController::runSelfCheck() const
{
    QStringList lines;
    bool ok = true;

    const int cmds = m_catalog.totalCount();
    if (cmds < 60) {
        ok = false;
        lines << QStringLiteral("FAIL catalog_count=%1 (need >=60)").arg(cmds);
    } else {
        lines << QStringLiteral("OK catalog_count=%1").arg(cmds);
    }

    GitRunner runner(5000);
    const GitRunResult ver = runner.run(QDir::homePath(), {QStringLiteral("--version")});
    if (!ver.ok) {
        ok = false;
        lines << QStringLiteral("FAIL git: %1").arg(ver.error);
    } else {
        lines << QStringLiteral("OK git %1").arg(QString::fromUtf8(ver.stdoutBytes).trimmed());
    }

    const PorcelainParseResult sample = parsePorcelainStatus(QStringLiteral("M  a\n?? b\n"));
    if (sample.files.size() != 2 || sample.untrackedCount != 1) {
        ok = false;
        lines << QStringLiteral("FAIL porcelain_parse");
    } else {
        lines << QStringLiteral("OK porcelain_parse");
    }

    if (m_llm.baseUrl().isEmpty()) {
        ok = false;
        lines << QStringLiteral("FAIL llm_base_url empty");
    } else {
        lines << QStringLiteral("OK llm_base_url=%1").arg(m_llm.baseUrl());
    }

    lines << QStringLiteral("OK version=%1").arg(QStringLiteral(GHEW_VERSION));
    lines << (ok ? QStringLiteral("SELF_CHECK_OK") : QStringLiteral("SELF_CHECK_FAIL"));
    return lines.join(QLatin1Char('\n'));
}

QString AppController::dumpRepositoryStatus(const QString &path)
{
    if (!m_repo.open(path))
        return {};
    m_github.refresh();
    m_advisor.refresh();

    QJsonDocument doc = QJsonDocument::fromJson(m_repo.dumpStatusJson().toUtf8());
    QJsonObject o = doc.object();
    QJsonArray recs;
    RecommendationModel *model = m_advisor.recommendations();
    for (int i = 0; i < model->rowCount(); ++i) {
        const QModelIndex idx = model->index(i);
        QJsonObject r;
        r.insert(QStringLiteral("commandId"), model->data(idx, RecommendationModel::CommandIdRole).toString());
        r.insert(QStringLiteral("title"), model->data(idx, RecommendationModel::TitleRole).toString());
        r.insert(QStringLiteral("reason"), model->data(idx, RecommendationModel::ReasonRole).toString());
        r.insert(QStringLiteral("source"), model->data(idx, RecommendationModel::SourceRole).toString());
        r.insert(QStringLiteral("priority"), model->data(idx, RecommendationModel::PriorityRole).toInt());
        recs.append(r);
    }
    o.insert(QStringLiteral("recommendations"), recs);
    return QString::fromUtf8(QJsonDocument(o).toJson(QJsonDocument::Compact));
}

QString AppController::validateArgs(const QString &commandId, const QVariantMap &args) const
{
    const auto spec = m_catalog.findById(commandId);
    if (!spec) {
        return tr("Команда не найдена: %1").arg(commandId);
    }
    for (const CommandParam &param : spec->params) {
        if (param.type == QLatin1String("bool") || param.optional) {
            continue;
        }
        const QVariant value = args.contains(param.name) ? args.value(param.name) : param.defaultValue;
        const QString text = value.toString().trimmed();
        const bool hasDefault = !param.defaultValue.toString().trimmed().isEmpty();
        if (!hasDefault && text.isEmpty()) {
            return tr("Заполните параметр: %1").arg(param.label.isEmpty() ? param.name : param.label);
        }
    }
    return {};
}

void AppController::previewCommand(const QString &commandId, const QVariantMap &args)
{
    if (!m_repo.isOpen()) {
        emit errorOccurred(tr("Сначала выберите git-репозиторий."));
        return;
    }

    const QString validation = validateArgs(commandId, args);
    if (!validation.isEmpty()) {
        emit errorOccurred(validation);
        return;
    }

    const auto spec = m_catalog.findById(commandId);
    if (!spec) {
        emit errorOccurred(tr("Команда не найдена: %1").arg(commandId));
        return;
    }

    const PreviewResult result = m_preview.build(*spec, args, m_repo.rootPath());
    if (!result.ok) {
        emit errorOccurred(result.error);
        return;
    }

    m_confirmToken = result.confirmToken;
    m_pendingArgv = result.argv;
    m_pendingCmdline = result.cmdline;
    m_pendingCommandId = commandId;
    m_pendingProgram = result.program;
    m_pendingArgs = args;
    m_pendingStdin.clear();
    m_pendingDanger = spec->danger;
    m_recentCommands.touch(commandId);

    QJsonObject obj;
    obj.insert(QStringLiteral("cmdline"), result.cmdline);
    obj.insert(QStringLiteral("narrative"), result.narrative);
    obj.insert(QStringLiteral("danger"), CommandSpec::dangerToString(spec->danger));
    obj.insert(QStringLiteral("confirmToken"), result.confirmToken);
    obj.insert(QStringLiteral("commandId"), commandId);
    m_lastPreviewJson = QString::fromUtf8(QJsonDocument(obj).toJson(QJsonDocument::Compact));
    emit previewChanged();
}

void AppController::executeLastPreview(bool destructiveConfirmed)
{
    if (m_confirmToken.isEmpty() || m_pendingArgv.isEmpty()) {
        emit errorOccurred(tr("Нет готового превью для выполнения."));
        return;
    }
    if (!m_repo.isOpen()) {
        emit errorOccurred(tr("Репозиторий не выбран."));
        return;
    }
    if (m_pendingDanger == DangerLevel::Destructive && !destructiveConfirmed) {
        emit errorOccurred(tr("Для опасной операции нужно явное подтверждение."));
        return;
    }
    if (m_gitProcess) {
        emit errorOccurred(tr("Уже выполняется git-команда. Отмените или дождитесь завершения."));
        return;
    }

    m_busy = true;
    emit busyChanged();

    m_execCmdline = m_pendingCmdline;
    m_execRepo = m_repo.rootPath();
    m_execDirtyBefore = m_repo.dirtyCount();
    m_execStagedBefore = m_repo.stagedCount();
    m_execBranchBefore = m_repo.branch();
    m_execCommandId = m_pendingCommandId;
    m_execProgram = m_pendingProgram.isEmpty() ? QStringLiteral("git") : m_pendingProgram;
    m_execFromAdvisor = m_pendingFromAdvisor;
    m_execStay = m_pendingStay;
    m_pendingStay = false;
    m_pendingFromAdvisor = false;
    m_execArgsJson = QString::fromUtf8(
        QJsonDocument(QJsonObject::fromVariantMap(m_pendingArgs)).toJson(QJsonDocument::Compact));

    const QStringList argv = m_pendingArgv;
    const QVariantMap execArgs = m_pendingArgs;
    m_execStdin = m_pendingStdin;
    m_confirmToken.clear();
    m_pendingArgv.clear();
    m_pendingCmdline.clear();
    m_pendingCommandId.clear();
    m_pendingArgs.clear();
    m_pendingStdin.clear();
    m_lastPreviewJson.clear();
    emit previewChanged();

    if (m_execProgram == QLatin1String("file")) {
        const GitRunResult run = m_github.applyFileCommand(m_execCommandId, execArgs);
        finishExecute(run, m_execCmdline, m_execRepo, m_execDirtyBefore, m_execStagedBefore, m_execBranchBefore);
        return;
    }
    if (m_execProgram == QLatin1String("wiki")) {
        const GitRunResult run = ::publishWikiPage(m_execRepo, githubWikiRemoteUrl(m_github.originUrl()),
                                                 githubWikiCacheDir(m_github.slug()),
                                                 execArgs.value(QStringLiteral("page")).toString(),
                                                 execArgs.value(QStringLiteral("body")).toString());
        finishExecute(run, m_execCmdline, m_execRepo, m_execDirtyBefore, m_execStagedBefore, m_execBranchBefore);
        if (run.ok)
            m_github.refreshWiki();
        return;
    }

    m_gitProcess = new QProcess(this);
    m_gitProcess->setWorkingDirectory(m_execRepo);
    m_gitProcess->setProcessChannelMode(QProcess::SeparateChannels);
    if (m_execProgram == QLatin1String("gh")) {
        QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
        env.insert(QStringLiteral("GH_PROMPT_DISABLED"), QStringLiteral("1"));
        env.insert(QStringLiteral("GH_NO_UPDATE_NOTIFIER"), QStringLiteral("1"));
        m_gitProcess->setProcessEnvironment(env);
    }
    connect(m_gitProcess, &QProcess::finished, this, [this](int exitCode, QProcess::ExitStatus status) {
        GitRunResult run;
        run.exitCode = exitCode;
        run.stdoutBytes = m_gitProcess->readAllStandardOutput();
        run.stderrBytes = m_gitProcess->readAllStandardError();
        run.ok = status == QProcess::NormalExit && exitCode == 0;
        if (!run.ok)
            run.error = QString::fromUtf8(run.stderrBytes).trimmed();
        m_gitProcess->deleteLater();
        m_gitProcess = nullptr;
        finishExecute(run, m_execCmdline, m_execRepo, m_execDirtyBefore, m_execStagedBefore, m_execBranchBefore);
    });
    m_gitProcess->start(m_execProgram, argv);
    if (!m_gitProcess->waitForStarted(5000)) {
        GitRunResult run;
        run.error = tr("Не удалось запустить %1: %2").arg(m_execProgram, m_gitProcess->errorString());
        m_execStdin.clear();
        m_gitProcess->deleteLater();
        m_gitProcess = nullptr;
        finishExecute(run, m_execCmdline, m_execRepo, m_execDirtyBefore, m_execStagedBefore, m_execBranchBefore);
        return;
    }
    if (m_execProgram == QLatin1String("gh") && !m_execStdin.isEmpty()) {
        const QByteArray hidden = m_execStdin.toUtf8();
        m_execStdin.clear();
        m_gitProcess->write(hidden);
        m_gitProcess->closeWriteChannel();
    }
}

void AppController::cancelRunning()
{
    if (m_gitProcess) {
        m_gitProcess->kill();
        return;
    }
    if (m_llm.streaming())
        m_llm.cancelStream();
}

void AppController::finishExecute(const GitRunResult &run, const QString &cmdline, const QString &repo,
                                  int dirtyBefore, int stagedBefore, const QString &branchBefore)
{
    m_execStdin.clear();
    const bool returnToAdvisor = m_execFromAdvisor;
    const bool stayOnGithub = m_execStay;
    const QString doneCommandId = m_execCommandId;
    m_execFromAdvisor = false;
    m_execStay = false;

    m_lastExitCode = run.exitCode;
    m_lastOutput = QStringLiteral("$ %1\n\n%2\n\n[exit %3]")
                       .arg(cmdline, run.combinedOutput(), QString::number(run.exitCode));
    emit outputChanged();

    HistoryEntry entry;
    entry.cmdline = cmdline;
    entry.repoRoot = repo;
    entry.exitCode = run.exitCode;
    entry.ok = run.ok;
    entry.timestamp = QDateTime::currentDateTime().toString(Qt::ISODate);
    entry.commandId = m_execCommandId;
    entry.argsJson = m_execArgsJson;
    m_history.prepend(entry);

    {
        QFile audit(auditLogPath());
        if (audit.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            QJsonObject row;
            row.insert(QStringLiteral("ts"), entry.timestamp);
            row.insert(QStringLiteral("repo"), repo);
            row.insert(QStringLiteral("cmdline"), cmdline);
            row.insert(QStringLiteral("commandId"), entry.commandId);
            row.insert(QStringLiteral("args"), QJsonDocument::fromJson(entry.argsJson.toUtf8()).object());
            row.insert(QStringLiteral("exit"), run.exitCode);
            row.insert(QStringLiteral("ok"), run.ok);
            QTextStream(&audit) << QString::fromUtf8(QJsonDocument(row).toJson(QJsonDocument::Compact))
                                << QLatin1Char('\n');
        }
    }

    m_execCommandId.clear();
    m_execArgsJson.clear();
    m_busy = false;
    emit busyChanged();

    if (!run.ok) {
        const QString tool = cmdline.section(QLatin1Char(' '), 0, 0);
        emit errorOccurred(run.error.isEmpty()
                               ? tr("%1 завершился с ошибкой (код %2)").arg(tool.isEmpty() ? QStringLiteral("git") : tool).arg(run.exitCode)
                               : run.error);
    }
    m_repo.refresh();
    const bool touchGithub = m_execProgram == QLatin1String("gh") || m_execProgram == QLatin1String("file")
                             || doneCommandId.startsWith(QLatin1String("github_"))
                             || doneCommandId == QLatin1String("push");
    if (touchGithub) {
        m_github.refresh();
        m_advisor.refresh();
    } else if (returnToAdvisor
               || doneCommandId == QLatin1String("commit") || doneCommandId == QLatin1String("add")
               || doneCommandId == QLatin1String("add_all") || doneCommandId == QLatin1String("add_update")) {
        // Commit/add change the tree the mentor reads — re-scan dress files so
        // "config not applied" is not invented from a stale snapshot.
        m_github.refreshLocalDress();
    }

    QStringList delta;
    delta << tr("После `%1`:").arg(cmdline);
    if (branchBefore != m_repo.branch())
        delta << tr("ветка %1 → %2").arg(branchBefore, m_repo.branch());
    delta << tr("dirty %1 → %2").arg(dirtyBefore).arg(m_repo.dirtyCount());
    delta << tr("staged %1 → %2").arg(stagedBefore).arg(m_repo.stagedCount());
    if (m_repo.dirtyCount() == 0)
        delta << tr("рабочее дерево чистое");
    m_lastStatusDelta = delta.join(QStringLiteral(" · "));
    emit statusDeltaChanged();

    if (returnToAdvisor) {
        emit commandRunRequested(0);
        m_advisor.continueAfterCommand(doneCommandId, cmdline, run.exitCode,
                                       run.combinedOutput(), m_lastStatusDelta);
    } else if (stayOnGithub) {
        emit commandRunRequested(11);
    }
}

void AppController::loadUnstagedHunks(const QString &path)
{
    if (!m_repo.isOpen()) {
        emit errorOccurred(tr("Сначала откройте репозиторий."));
        return;
    }
    QStringList args{QStringLiteral("diff"), QStringLiteral("-U3")};
    if (!path.trimmed().isEmpty())
        args << QStringLiteral("--") << path.trimmed();
    const GitRunResult r = m_runner.run(m_repo.rootPath(), args);
    const ParsedDiff parsed = parseUnifiedDiff(QString::fromUtf8(r.stdoutBytes));
    if (parsed.hunks.isEmpty()) {
        m_hunks.clear();
        emit errorOccurred(parsed.error.isEmpty() ? tr("Нет unstaged hunks для staging.")
                                                  : parsed.error);
        return;
    }
    m_hunks.setHunks(parsed.hunks);
}

void AppController::loadStagedHunks(const QString &path)
{
    if (!m_repo.isOpen()) {
        emit errorOccurred(tr("Сначала откройте репозиторий."));
        return;
    }
    QStringList args{QStringLiteral("diff"), QStringLiteral("--cached"), QStringLiteral("-U3")};
    if (!path.trimmed().isEmpty())
        args << QStringLiteral("--") << path.trimmed();
    const GitRunResult r = m_runner.run(m_repo.rootPath(), args);
    const ParsedDiff parsed = parseUnifiedDiff(QString::fromUtf8(r.stdoutBytes));
    if (parsed.hunks.isEmpty()) {
        m_hunks.clear();
        emit errorOccurred(parsed.error.isEmpty() ? tr("Нет staged hunks.") : parsed.error);
        return;
    }
    m_hunks.setHunks(parsed.hunks);
}

static bool applyHunkPatch(GitRunner &runner, const QString &repo, const QString &patch, bool reverse,
                           bool cached, QString *errorOut)
{
    QTemporaryFile tmp(QDir::temp().filePath(QStringLiteral("ghew-hunk-XXXXXX.patch")));
    tmp.setAutoRemove(true);
    if (!tmp.open()) {
        if (errorOut)
            *errorOut = QObject::tr("Не удалось создать временный patch.");
        return false;
    }
    tmp.write(patch.toUtf8());
    tmp.flush();

    auto makeArgs = [&](bool unidiffZero) {
        QStringList args{QStringLiteral("apply")};
        if (cached)
            args << QStringLiteral("--cached");
        if (reverse)
            args << QStringLiteral("-R");
        if (unidiffZero)
            args << QStringLiteral("--unidiff-zero");
        args << tmp.fileName();
        return args;
    };

    GitRunResult final = runner.run(repo, makeArgs(false));
    if (!final.ok)
        final = runner.run(repo, makeArgs(true));
    if (!final.ok && errorOut)
        *errorOut = final.error.isEmpty() ? QObject::tr("git apply не удался") : final.error;
    return final.ok;
}

bool AppController::stageSelectedHunks()
{
    if (!m_repo.isOpen()) {
        emit errorOccurred(tr("Репозиторий не открыт."));
        return false;
    }
    const QString patch = buildPatchFromHunks(m_hunks.hunks(), true);
    if (patch.trimmed().isEmpty()) {
        emit errorOccurred(tr("Не выбрано ни одного hunk."));
        return false;
    }
    QString err;
    if (!applyHunkPatch(m_runner, m_repo.rootPath(), patch, false, true, &err)) {
        emit errorOccurred(err);
        return false;
    }
    m_lastOutput = QStringLiteral("$ git apply --cached\n\nOK");
    m_lastExitCode = 0;
    emit outputChanged();
    m_repo.refresh();
    loadUnstagedHunks();
    m_lastStatusDelta = tr("Hunks staged · dirty %1 · staged %2")
                            .arg(m_repo.dirtyCount())
                            .arg(m_repo.stagedCount());
    emit statusDeltaChanged();
    return true;
}

bool AppController::unstageSelectedHunks()
{
    if (!m_repo.isOpen()) {
        emit errorOccurred(tr("Репозиторий не открыт."));
        return false;
    }
    const QString patch = buildPatchFromHunks(m_hunks.hunks(), true);
    if (patch.trimmed().isEmpty()) {
        emit errorOccurred(tr("Не выбрано ни одного hunk."));
        return false;
    }
    QString err;
    if (!applyHunkPatch(m_runner, m_repo.rootPath(), patch, true, true, &err)) {
        emit errorOccurred(err);
        return false;
    }
    m_lastOutput = QStringLiteral("$ git apply --cached -R\n\nOK");
    m_lastExitCode = 0;
    emit outputChanged();
    m_repo.refresh();
    loadStagedHunks();
    m_lastStatusDelta = tr("Hunks unstaged · dirty %1 · staged %2")
                            .arg(m_repo.dirtyCount())
                            .arg(m_repo.stagedCount());
    emit statusDeltaChanged();
    return true;
}

bool AppController::discardSelectedHunks()
{
    if (!m_repo.isOpen()) {
        emit errorOccurred(tr("Репозиторий не открыт."));
        return false;
    }
    const QString patch = buildPatchFromHunks(m_hunks.hunks(), true);
    if (patch.trimmed().isEmpty()) {
        emit errorOccurred(tr("Не выбрано ни одного hunk."));
        return false;
    }
    QString err;
    if (!applyHunkPatch(m_runner, m_repo.rootPath(), patch, true, false, &err)) {
        emit errorOccurred(err);
        return false;
    }
    m_lastOutput = QStringLiteral("$ git apply -R\n\nOK");
    m_lastExitCode = 0;
    emit outputChanged();
    m_repo.refresh();
    loadUnstagedHunks();
    m_lastStatusDelta = tr("Hunks discarded · dirty %1 · staged %2")
                            .arg(m_repo.dirtyCount())
                            .arg(m_repo.stagedCount());
    emit statusDeltaChanged();
    return true;
}

QString AppController::exportSelectedHunksPatch()
{
    const QString patch = buildPatchFromHunks(m_hunks.hunks(), true);
    if (patch.trimmed().isEmpty()) {
        emit errorOccurred(tr("Не выбрано ни одного hunk."));
        return {};
    }
    const QString outDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QDir().mkpath(outDir);
    const QString path = outDir + QStringLiteral("/ghew-hunks-")
                         + QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-hhmmss"))
                         + QStringLiteral(".patch");
    QFile f(path);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        emit errorOccurred(tr("Не удалось записать patch."));
        return {};
    }
    QTextStream(&f) << patch;
    if (!patch.endsWith(QLatin1Char('\n')))
        QTextStream(&f) << QLatin1Char('\n');
    emit infoOccurred(tr("Patch сохранён: %1").arg(path));
    return path;
}

QVariantMap AppController::loadUiState() const
{
    QSettings settings;
    QVariantMap m;
    m.insert(QStringLiteral("width"), settings.value(QStringLiteral("ui/width"), 1180).toInt());
    m.insert(QStringLiteral("height"), settings.value(QStringLiteral("ui/height"), 760).toInt());
    m.insert(QStringLiteral("x"), settings.value(QStringLiteral("ui/x"), -1).toInt());
    m.insert(QStringLiteral("y"), settings.value(QStringLiteral("ui/y"), -1).toInt());
    m.insert(QStringLiteral("tabsIndex"), settings.value(QStringLiteral("ui/tabsIndex"), 0).toInt());
    m.insert(QStringLiteral("tabGroup"), settings.value(QStringLiteral("ui/tabGroup"), 0).toInt());
    m.insert(QStringLiteral("statusSection"), settings.value(QStringLiteral("ui/statusSection"), 0).toInt());
    m.insert(QStringLiteral("statusStateFilter"),
             settings.value(QStringLiteral("ui/statusStateFilter")).toString());
    m.insert(QStringLiteral("statusHistSub"), settings.value(QStringLiteral("ui/statusHistSub"), 0).toInt());
    m.insert(QStringLiteral("statusRefsSub"), settings.value(QStringLiteral("ui/statusRefsSub"), 0).toInt());
    m.insert(QStringLiteral("statusMoreSub"), settings.value(QStringLiteral("ui/statusMoreSub"), 0).toInt());
    m.insert(QStringLiteral("sidebarWidth"), settings.value(QStringLiteral("ui/sidebarWidth"), 320).toInt());
    m.insert(QStringLiteral("uiScale"), settings.value(QStringLiteral("ui/uiScale"), 1.0).toDouble());
    return m;
}

void AppController::saveUiState(const QVariantMap &state)
{
    QSettings settings;
    if (state.contains(QStringLiteral("width")))
        settings.setValue(QStringLiteral("ui/width"), state.value(QStringLiteral("width")).toInt());
    if (state.contains(QStringLiteral("height")))
        settings.setValue(QStringLiteral("ui/height"), state.value(QStringLiteral("height")).toInt());
    if (state.contains(QStringLiteral("x")))
        settings.setValue(QStringLiteral("ui/x"), state.value(QStringLiteral("x")).toInt());
    if (state.contains(QStringLiteral("y")))
        settings.setValue(QStringLiteral("ui/y"), state.value(QStringLiteral("y")).toInt());
    if (state.contains(QStringLiteral("tabsIndex")))
        settings.setValue(QStringLiteral("ui/tabsIndex"), state.value(QStringLiteral("tabsIndex")).toInt());
    if (state.contains(QStringLiteral("tabGroup")))
        settings.setValue(QStringLiteral("ui/tabGroup"), state.value(QStringLiteral("tabGroup")).toInt());
    if (state.contains(QStringLiteral("statusSection")))
        settings.setValue(QStringLiteral("ui/statusSection"), state.value(QStringLiteral("statusSection")).toInt());
    if (state.contains(QStringLiteral("statusStateFilter")))
        settings.setValue(QStringLiteral("ui/statusStateFilter"),
                          state.value(QStringLiteral("statusStateFilter")).toString());
    if (state.contains(QStringLiteral("statusHistSub")))
        settings.setValue(QStringLiteral("ui/statusHistSub"), state.value(QStringLiteral("statusHistSub")).toInt());
    if (state.contains(QStringLiteral("statusRefsSub")))
        settings.setValue(QStringLiteral("ui/statusRefsSub"), state.value(QStringLiteral("statusRefsSub")).toInt());
    if (state.contains(QStringLiteral("statusMoreSub")))
        settings.setValue(QStringLiteral("ui/statusMoreSub"), state.value(QStringLiteral("statusMoreSub")).toInt());
    if (state.contains(QStringLiteral("sidebarWidth")))
        settings.setValue(QStringLiteral("ui/sidebarWidth"), state.value(QStringLiteral("sidebarWidth")).toInt());
    if (state.contains(QStringLiteral("uiScale"))) {
        const double scale = state.value(QStringLiteral("uiScale")).toDouble();
        if (scale >= 0.5 && scale <= 2.0)
            settings.setValue(QStringLiteral("ui/uiScale"), scale);
    }
}

void AppController::replayHistory(int row)
{
    const HistoryEntry e = m_history.entryAt(row);
    if (e.commandId.isEmpty()) {
        emit errorOccurred(tr("В этой записи нет commandId для повтора."));
        return;
    }
    QVariantMap args;
    if (!e.argsJson.isEmpty()) {
        const QJsonDocument doc = QJsonDocument::fromJson(e.argsJson.toUtf8());
        if (doc.isObject())
            args = doc.object().toVariantMap();
    }
    prepareCommand(e.commandId, args);
}

bool AppController::createLessonRepository()
{
    const QString base = QStandardPaths::writableLocation(QStandardPaths::CacheLocation)
                         + QStringLiteral("/ghew-lessons");
    QDir().mkpath(base);
    const QString path = base + QStringLiteral("/lesson-")
                         + QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-hhmmss"));
    if (!QDir().mkpath(path)) {
        emit errorOccurred(tr("Не удалось создать каталог урока."));
        return false;
    }

    auto run = [&](const QStringList &args) {
        return m_runner.run(path, args);
    };
    if (!run({QStringLiteral("init"), QStringLiteral("-b"), QStringLiteral("main")}).ok) {
        emit errorOccurred(tr("git init не удался."));
        return false;
    }
    run({QStringLiteral("config"), QStringLiteral("user.email"), QStringLiteral("lesson@ghew.local")});
    run({QStringLiteral("config"), QStringLiteral("user.name"), QStringLiteral("GHEW Lesson")});

    QFile readme(path + QStringLiteral("/README.md"));
    if (readme.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream(&readme) << QStringLiteral("# GHEW lesson\n\nEdit me, stage hunks, commit.\n");
        readme.close();
    }
    run({QStringLiteral("add"), QStringLiteral("README.md")});
    run({QStringLiteral("commit"), QStringLiteral("-m"), QStringLiteral("lesson: start")});

    QFile notes(path + QStringLiteral("/NOTES.txt"));
    if (notes.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream(&notes) << QStringLiteral("untracked notes for the lesson\n");
        notes.close();
    }
    QFile readme2(path + QStringLiteral("/README.md"));
    if (readme2.open(QIODevice::Append | QIODevice::Text)) {
        QTextStream(&readme2) << QStringLiteral("\nSecond line for unstaged hunk practice.\n");
        readme2.close();
    }

    m_lessonRepoPath = path;
    emit lessonChanged();
    return openRepository(path);
}

bool AppController::openLessonRepository()
{
    if (m_lessonRepoPath.isEmpty() || !QDir(m_lessonRepoPath).exists())
        return createLessonRepository();
    return openRepository(m_lessonRepoPath);
}

bool AppController::initRepository(const QString &path, const QString &branchName, bool withReadme)
{
    const QString dir = path.trimmed();
    if (dir.isEmpty()) {
        emit errorOccurred(tr("Укажите каталог для нового репозитория."));
        return false;
    }
    if (!QDir(dir).exists() && !QDir().mkpath(dir)) {
        emit errorOccurred(tr("Не удалось создать каталог: %1").arg(dir));
        return false;
    }

    const GitRunResult inside = m_runner.run(dir, {QStringLiteral("rev-parse"),
                                                   QStringLiteral("--is-inside-work-tree")});
    if (inside.ok && QString::fromUtf8(inside.stdoutBytes).trimmed() == QLatin1String("true")) {
        emit infoOccurred(tr("Уже git-репозиторий — открываю."));
        return openRepository(dir);
    }

    QString branch = branchName.trimmed();
    if (branch.isEmpty())
        branch = QStringLiteral("main");

    GitRunResult r = m_runner.run(dir, {QStringLiteral("init"), QStringLiteral("-b"), branch});
    if (!r.ok) {
        // Older git without `init -b`
        r = m_runner.run(dir, {QStringLiteral("init")});
        if (r.ok) {
            const GitRunResult br = m_runner.run(dir, {QStringLiteral("checkout"), QStringLiteral("-b"), branch});
            if (!br.ok)
                m_runner.run(dir, {QStringLiteral("symbolic-ref"), QStringLiteral("HEAD"),
                                   QStringLiteral("refs/heads/") + branch});
        }
    }

    m_lastOutput = QStringLiteral("$ git init -b %1\n\n%2\n\n[exit %3]")
                       .arg(branch, r.combinedOutput(), QString::number(r.exitCode));
    m_lastExitCode = r.exitCode;
    emit outputChanged();
    if (!r.ok) {
        emit errorOccurred(r.error.isEmpty() ? tr("git init не удался") : r.error);
        return false;
    }

    if (withReadme) {
        const QString readme = dir + QStringLiteral("/README.md");
        if (!QFileInfo::exists(readme)) {
            QFile f(readme);
            if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
                QTextStream(&f) << QStringLiteral("# %1\n\nИнициализировано в GHEW.\n")
                                       .arg(QFileInfo(dir).fileName());
            }
        }
    }

    emit infoOccurred(tr("Репозиторий создан: %1 (%2)").arg(dir, branch));
    return openRepository(dir);
}

bool AppController::cloneRepository(const QString &url, const QString &destDir)
{
    const QString u = url.trimmed();
    const QString dest = destDir.trimmed();
    if (u.isEmpty() || dest.isEmpty()) {
        emit errorOccurred(tr("Укажите URL и каталог назначения."));
        return false;
    }
    if (!QDir(dest).exists() && !QDir().mkpath(dest)) {
        emit errorOccurred(tr("Не удалось создать каталог: %1").arg(dest));
        return false;
    }
    // Clone into dest if empty, else into dest/<repo-name>
    QString target = dest;
    const QFileInfo fi(dest);
    if (fi.isDir() && !QDir(dest).entryList(QDir::AllEntries | QDir::NoDotAndDotDot).isEmpty()) {
        QString name = u.section(QLatin1Char('/'), -1);
        if (name.endsWith(QStringLiteral(".git")))
            name.chop(4);
        if (name.isEmpty())
            name = QStringLiteral("repo");
        target = dest + QLatin1Char('/') + name;
    }
    emit infoOccurred(tr("Клонирование… %1").arg(u));
    const GitRunResult r = m_runner.run(QDir::tempPath(),
                                        {QStringLiteral("clone"), QStringLiteral("--"), u, target});
    m_lastOutput = QStringLiteral("$ git clone %1 %2\n\n%3\n\n[exit %4]")
                       .arg(u, target, r.combinedOutput(), QString::number(r.exitCode));
    m_lastExitCode = r.exitCode;
    emit outputChanged();
    if (!r.ok) {
        emit errorOccurred(r.error.isEmpty() ? tr("git clone не удался") : r.error);
        return false;
    }
    emit infoOccurred(tr("Клон готов: %1").arg(target));
    return openRepository(target);
}

void AppController::compareCommits(const QString &a, const QString &b)
{
    m_repo.setCompareEndpoint(a, true);
    m_repo.setCompareEndpoint(b, false);
    if (!m_repo.loadCompareDiff())
        emit errorOccurred(tr("Не удалось сравнить коммиты."));
}

void AppController::setCompareEndpoint(const QString &hash, bool asA)
{
    m_repo.setCompareEndpoint(hash, asA);
    if (!m_repo.compareA().isEmpty() && !m_repo.compareB().isEmpty())
        m_repo.loadCompareDiff();
}

QString AppController::exportSessionReport()
{
    QStringList md;
    md << QStringLiteral("# GHEW — отчёт сессии");
    md << QString();
    md << QStringLiteral("- Версия: `%1`").arg(QStringLiteral(GHEW_VERSION));
    md << QStringLiteral("- Время: `%1`").arg(QDateTime::currentDateTime().toString(Qt::ISODate));
    if (m_repo.isOpen()) {
        md << QStringLiteral("- Репозиторий: `%1`").arg(m_repo.rootPath());
        md << QStringLiteral("- Ветка: `%1`").arg(m_repo.branch());
        md << QStringLiteral("- Dirty / staged / untracked: `%1` / `%2` / `%3`")
                  .arg(m_repo.dirtyCount())
                  .arg(m_repo.stagedCount())
                  .arg(m_repo.untrackedCount());
        if (m_repo.hasUpstream())
            md << QStringLiteral("- Upstream: `%1` (↑%2 ↓%3)")
                      .arg(m_repo.upstream())
                      .arg(m_repo.ahead())
                      .arg(m_repo.behind());
    } else {
        md << QStringLiteral("- Репозиторий: не открыт");
    }
    md << QString();
    md << QStringLiteral("## История сессии (%1)").arg(m_history.rowCount());
    md << QString();
    for (int i = 0; i < m_history.rowCount(); ++i) {
        const HistoryEntry e = m_history.entryAt(i);
        md << QStringLiteral("%1. `%2` — exit %3%4")
                  .arg(i + 1)
                  .arg(e.cmdline)
                  .arg(e.exitCode)
                  .arg(e.commandId.isEmpty() ? QString()
                                             : QStringLiteral(" · `%1`").arg(e.commandId));
    }
    if (m_history.rowCount() == 0)
        md << QStringLiteral("_Пусто._");
    md << QString();
    md << QStringLiteral("## Рекомендации (%1)").arg(m_advisor.recommendations()->rowCount());
    md << QString();
    RecommendationModel *recs = m_advisor.recommendations();
    for (int i = 0; i < recs->rowCount(); ++i) {
        const QModelIndex idx = recs->index(i);
        md << QStringLiteral("- **%1** (`%2`, %3): %4")
                  .arg(recs->data(idx, RecommendationModel::TitleRole).toString(),
                       recs->data(idx, RecommendationModel::CommandIdRole).toString(),
                       recs->data(idx, RecommendationModel::SourceRole).toString(),
                       recs->data(idx, RecommendationModel::ReasonRole).toString());
    }
    md << QString();
    md << QStringLiteral("Аудит JSONL: `%1`").arg(auditLogPath());
    const QString report = md.join(QLatin1Char('\n'));

    const QString outDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    QDir().mkpath(outDir);
    const QString path = outDir + QStringLiteral("/ghew-session-")
                         + QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-hhmmss"))
                         + QStringLiteral(".md");
    QFile f(path);
    if (f.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream(&f) << report << QLatin1Char('\n');
        emit infoOccurred(tr("Отчёт сохранён: %1").arg(path));
        return path;
    }
    emit infoOccurred(tr("Не удалось записать файл — отчёт в буфере."));
    return report;
}

QString AppController::runIntegrationTest()
{
    QStringList lines;
    bool ok = true;

    {
        const QString initPath = QStandardPaths::writableLocation(QStandardPaths::TempLocation)
                                 + QStringLiteral("/ghew-init-")
                                 + QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMddhhmmss"));
        if (!initRepository(initPath, QStringLiteral("main"), true)) {
            return QStringLiteral("FAIL init_repo\nINTEGRATION_FAIL");
        }
        if (!m_repo.isOpen() || !QFileInfo::exists(initPath + QStringLiteral("/.git"))
            || !QFileInfo::exists(initPath + QStringLiteral("/README.md"))) {
            return QStringLiteral("FAIL init_repo_state\nINTEGRATION_FAIL");
        }
        lines << QStringLiteral("OK init_repo=%1").arg(initPath);
        m_repo.close();
    }

    if (!createLessonRepository()) {
        return QStringLiteral("FAIL lesson_repo\nINTEGRATION_FAIL");
    }
    lines << QStringLiteral("OK lesson_repo=%1").arg(m_lessonRepoPath);

    loadUnstagedHunks(QStringLiteral("README.md"));
    if (m_hunks.rowCount() < 1) {
        ok = false;
        lines << QStringLiteral("FAIL hunks_expected>=1 got=%1").arg(m_hunks.rowCount());
    } else {
        lines << QStringLiteral("OK hunks=%1").arg(m_hunks.rowCount());
        m_hunks.selectAll(true);
        if (!stageSelectedHunks()) {
            ok = false;
            lines << QStringLiteral("FAIL stage_hunks");
        } else if (m_repo.stagedCount() < 1) {
            ok = false;
            lines << QStringLiteral("FAIL staged_after_hunk=%1").arg(m_repo.stagedCount());
        } else {
            lines << QStringLiteral("OK staged_after_hunk=%1").arg(m_repo.stagedCount());
        }

        loadStagedHunks(QStringLiteral("README.md"));
        m_hunks.selectAll(true);
        if (!unstageSelectedHunks()) {
            ok = false;
            lines << QStringLiteral("FAIL unstage_hunks");
        } else {
            lines << QStringLiteral("OK unstage_hunks");
        }

        // Fresh unstaged change for discard + patch export.
        {
            QFile readme(m_lessonRepoPath + QStringLiteral("/README.md"));
            if (readme.open(QIODevice::Append | QIODevice::Text)) {
                QTextStream(&readme) << QStringLiteral("\ndiscard-me\n");
                readme.close();
            }
            m_repo.refresh();
            loadUnstagedHunks(QStringLiteral("README.md"));
            m_hunks.selectAll(true);
            const QString patchPath = exportSelectedHunksPatch();
            if (patchPath.isEmpty() || !QFileInfo::exists(patchPath)) {
                ok = false;
                lines << QStringLiteral("FAIL export_patch");
            } else {
                lines << QStringLiteral("OK export_patch=%1").arg(patchPath);
            }
            if (!discardSelectedHunks()) {
                ok = false;
                lines << QStringLiteral("FAIL discard_hunks");
            } else {
                lines << QStringLiteral("OK discard_hunks");
            }
        }
    }

    previewCommand(QStringLiteral("status"), {});
    if (m_lastPreviewJson.isEmpty()) {
        ok = false;
        lines << QStringLiteral("FAIL preview_status");
    } else {
        lines << QStringLiteral("OK preview_status");
    }

    const QString exported = exportSessionReport();
    if (exported.isEmpty()) {
        ok = false;
        lines << QStringLiteral("FAIL export_report");
    } else {
        lines << QStringLiteral("OK export_report=%1").arg(exported);
    }

    // Remotes parse should not crash; lesson repo usually has zero remotes.
    lines << QStringLiteral("OK remotes=%1").arg(m_repo.remotes()->rowCount());
    if (m_repo.worktrees()->rowCount() < 1) {
        ok = false;
        lines << QStringLiteral("FAIL worktrees_expected>=1");
    } else {
        lines << QStringLiteral("OK worktrees=%1").arg(m_repo.worktrees()->rowCount());
    }
    lines << QStringLiteral("OK bisect_active=%1").arg(m_repo.bisectActive() ? 1 : 0);
    lines << QStringLiteral("OK operation_kind=%1").arg(
        m_repo.operationKind().isEmpty() ? QStringLiteral("-") : m_repo.operationKind());
    lines << QStringLiteral("OK conflict_count=%1").arg(m_repo.conflictCount());
    lines << QStringLiteral("OK last_commit_subject=%1")
                 .arg(m_repo.lastCommitSubject().isEmpty() ? QStringLiteral("-")
                                                           : m_repo.lastCommitSubject().left(40));
    lines << QStringLiteral("OK hooks=%1 active=%2")
                 .arg(m_repo.hooks()->rowCount())
                 .arg(m_repo.hooks()->activeCount());
    lines << QStringLiteral("OK submodules=%1").arg(m_repo.submodules()->rowCount());

    if (!m_repo.loadCleanPreview()) {
        ok = false;
        lines << QStringLiteral("FAIL clean_preview");
    } else {
        lines << QStringLiteral("OK clean_preview");
    }
    if (!m_repo.loadFileAttributes(QStringLiteral("README.md"))) {
        ok = false;
        lines << QStringLiteral("FAIL file_attr");
    } else {
        lines << QStringLiteral("OK file_attr");
    }

    if (!m_repo.loadCommitShow(QStringLiteral("HEAD"))) {
        ok = false;
        lines << QStringLiteral("FAIL inspect_head");
    } else if (m_repo.inspectTarget().isEmpty()) {
        ok = false;
        lines << QStringLiteral("FAIL inspect_target");
    } else {
        lines << QStringLiteral("OK inspect_target=%1").arg(m_repo.inspectTarget().left(7));
    }

    if (!m_repo.loadFileHistory(QStringLiteral("README.md"))) {
        ok = false;
        lines << QStringLiteral("FAIL file_history");
    } else {
        lines << QStringLiteral("OK file_history");
    }

    if (!appendGitignore(QStringLiteral("tmp-ignore-me.txt"))) {
        ok = false;
        lines << QStringLiteral("FAIL gitignore");
    } else {
        lines << QStringLiteral("OK gitignore");
    }

    // Merge-conflict sandbox: operationKind + conflictCount + catalog commands.
    {
        const QString path = QStandardPaths::writableLocation(QStandardPaths::TempLocation)
                             + QStringLiteral("/ghew-conflict-")
                             + QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMddhhmmss"));
        if (!QDir().mkpath(path)) {
            ok = false;
            lines << QStringLiteral("FAIL conflict_mkdir");
        } else {
            auto run = [&](const QStringList &args) { return m_runner.run(path, args); };
            run({QStringLiteral("init"), QStringLiteral("-b"), QStringLiteral("main")});
            run({QStringLiteral("config"), QStringLiteral("user.email"), QStringLiteral("t@ghew.local")});
            run({QStringLiteral("config"), QStringLiteral("user.name"), QStringLiteral("GHEW Test")});
            {
                QFile f(path + QStringLiteral("/conflict.txt"));
                if (f.open(QIODevice::WriteOnly | QIODevice::Text))
                    QTextStream(&f) << QStringLiteral("base\n");
            }
            run({QStringLiteral("add"), QStringLiteral("conflict.txt")});
            run({QStringLiteral("commit"), QStringLiteral("-m"), QStringLiteral("base")});
            run({QStringLiteral("checkout"), QStringLiteral("-b"), QStringLiteral("feature")});
            {
                QFile f(path + QStringLiteral("/conflict.txt"));
                if (f.open(QIODevice::WriteOnly | QIODevice::Text))
                    QTextStream(&f) << QStringLiteral("feature-side\n");
            }
            run({QStringLiteral("add"), QStringLiteral("conflict.txt")});
            run({QStringLiteral("commit"), QStringLiteral("-m"), QStringLiteral("feature")});
            run({QStringLiteral("checkout"), QStringLiteral("main")});
            {
                QFile f(path + QStringLiteral("/conflict.txt"));
                if (f.open(QIODevice::WriteOnly | QIODevice::Text))
                    QTextStream(&f) << QStringLiteral("main-side\n");
            }
            run({QStringLiteral("add"), QStringLiteral("conflict.txt")});
            run({QStringLiteral("commit"), QStringLiteral("-m"), QStringLiteral("main")});
            run({QStringLiteral("merge"), QStringLiteral("feature"), QStringLiteral("--no-edit")});

            if (!openRepository(path)) {
                ok = false;
                lines << QStringLiteral("FAIL conflict_open");
            } else if (m_repo.operationKind() != QLatin1String("merge")) {
                ok = false;
                lines << QStringLiteral("FAIL operation_kind_expected=merge got=%1").arg(m_repo.operationKind());
            } else if (m_repo.conflictCount() < 1) {
                ok = false;
                lines << QStringLiteral("FAIL conflict_count_expected>=1 got=%1").arg(m_repo.conflictCount());
            } else {
                lines << QStringLiteral("OK merge_conflict op=%1 conflicts=%2")
                             .arg(m_repo.operationKind())
                             .arg(m_repo.conflictCount());
            }

            // Mentor dress/config path: write file, snapshot must see it, empty body rejected.
            {
                const QString body = QStringLiteral("# Безопасность\n\nПишите на security@ghew.test\n");
                if (!saveDressFile(QStringLiteral("security"), body)) {
                    ok = false;
                    lines << QStringLiteral("FAIL mentor_dress_write");
                } else if (!QFileInfo::exists(path + QStringLiteral("/SECURITY.md"))) {
                    ok = false;
                    lines << QStringLiteral("FAIL mentor_dress_missing_file");
                } else {
                    m_github.refreshLocalDress();
                    const QString snap = m_github.snapshotText();
                    if (!snap.contains(QStringLiteral("security=yes"))) {
                        ok = false;
                        lines << QStringLiteral("FAIL mentor_dress_snapshot");
                    } else {
                        lines << QStringLiteral("OK mentor_dress_snapshot");
                    }
                }
                if (saveDressFile(QStringLiteral("funding"), QString())) {
                    ok = false;
                    lines << QStringLiteral("FAIL mentor_dress_empty_accepted");
                } else {
                    lines << QStringLiteral("OK mentor_dress_empty_rejected");
                }

                m_advisor.applyMentorReplyText(
                    QStringLiteral("Нужен текст политики.\n"
                                   "<<<ASK_USER>>>\n"
                                   "{\"prompt\":\"Какой текст в SECURITY.md?\"}\n"
                                   "<<<END_ASK_USER>>>\n"));
                if (!m_advisor.hasAskUser() || m_advisor.askUserPrompt().isEmpty()) {
                    ok = false;
                    lines << QStringLiteral("FAIL mentor_ask_free_text");
                } else {
                    lines << QStringLiteral("OK mentor_ask_free_text");
                }

                // Free-text reply must become an executable gh_file suggestion with the student's body.
                m_advisor.replyMentor(QStringLiteral("# Политика\n\nПишите на security@ghew.test\n"));
                bool foundSecurity = false;
                for (const QVariant &row : m_advisor.suggestedCommands()) {
                    const QVariantMap m = row.toMap();
                    if (m.value(QStringLiteral("commandId")).toString() != QLatin1String("gh_file"))
                        continue;
                    const QVariantMap args = m.value(QStringLiteral("args")).toMap();
                    if (args.value(QStringLiteral("kind")).toString() != QLatin1String("security"))
                        continue;
                    if (!args.value(QStringLiteral("body")).toString().contains(QStringLiteral("security@ghew.test")))
                        continue;
                    foundSecurity = true;
                    runCommand(QStringLiteral("gh_file"), args, true);
                    break;
                }
                if (!foundSecurity) {
                    ok = false;
                    lines << QStringLiteral("FAIL mentor_reply_to_gh_file");
                } else if (!QFileInfo::exists(path + QStringLiteral("/SECURITY.md"))) {
                    ok = false;
                    lines << QStringLiteral("FAIL mentor_reply_write");
                } else {
                    lines << QStringLiteral("OK mentor_reply_to_gh_file");
                }

                m_advisor.applyMentorReplyText(
                    QStringLiteral("Пишу funding.\n"
                                   "<<<RUN>>>\n"
                                   "{\"commands\":[{\"id\":\"gh_file\",\"args\":{"
                                   "\"kind\":\"funding\",\"body\":\"# github: [ghew]\\n\"}}]}\n"
                                   "<<<END_RUN>>>\n"));
                bool foundFunding = false;
                for (const QVariant &row : m_advisor.suggestedCommands()) {
                    const QVariantMap m = row.toMap();
                    if (m.value(QStringLiteral("commandId")).toString() == QLatin1String("gh_file")
                        && m.value(QStringLiteral("args")).toMap().value(QStringLiteral("kind")).toString()
                               == QLatin1String("funding")) {
                        foundFunding = true;
                        const QVariantMap args = m.value(QStringLiteral("args")).toMap();
                        runCommand(QStringLiteral("gh_file"), args, false);
                        break;
                    }
                }
                if (!foundFunding) {
                    ok = false;
                    lines << QStringLiteral("FAIL mentor_run_gh_file_suggest");
                } else if (!QFileInfo::exists(path + QStringLiteral("/.github/FUNDING.yml"))) {
                    ok = false;
                    lines << QStringLiteral("FAIL mentor_run_gh_file_write");
                } else {
                    lines << QStringLiteral("OK mentor_run_gh_file");
                }

                if (!saveGitUser(QStringLiteral("GHEW Poet"), QStringLiteral("poet@ghew.test"), false)) {
                    ok = false;
                    lines << QStringLiteral("FAIL config_set_user");
                } else {
                    const QVariantMap id = gitUserIdentity();
                    if (!id.value(QStringLiteral("complete")).toBool()
                        || id.value(QStringLiteral("email")).toString() != QLatin1String("poet@ghew.test")) {
                        ok = false;
                        lines << QStringLiteral("FAIL config_set_user_readback");
                    } else {
                        lines << QStringLiteral("OK config_set_user");
                    }
                }

                m_advisor.clearDialogue();
                if (m_advisor.hasAskUser() || !m_advisor.activeTask().isEmpty()) {
                    ok = false;
                    lines << QStringLiteral("FAIL mentor_clear_dialogue");
                } else {
                    lines << QStringLiteral("OK mentor_clear_dialogue");
                }
            }

            if (!m_catalog.findById(QStringLiteral("cherry_pick_continue"))
                || !m_catalog.findById(QStringLiteral("stash_drop"))
                || !m_catalog.findById(QStringLiteral("revert_abort"))
                || !m_catalog.findById(QStringLiteral("pull_rebase"))
                || !m_catalog.findById(QStringLiteral("fetch_prune"))
                || !m_catalog.findById(QStringLiteral("branch_set_upstream"))
                || !m_catalog.findById(QStringLiteral("branch_unset_upstream"))
                || !m_catalog.findById(QStringLiteral("worktree_remove"))
                || !m_catalog.findById(QStringLiteral("worktree_prune"))) {
                ok = false;
                lines << QStringLiteral("FAIL catalog_op_cmds");
            } else {
                lines << QStringLiteral("OK catalog_op_cmds");
            }
        }
    }

    lines << (ok ? QStringLiteral("INTEGRATION_OK") : QStringLiteral("INTEGRATION_FAIL"));
    return lines.join(QLatin1Char('\n'));
}
