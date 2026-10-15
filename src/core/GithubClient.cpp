#include "GithubClient.h"

#include "GithubIdentity.h"
#include "GithubLifecycle.h"
#include "GithubRemote.h"
#include "GithubScaffold.h"
#include "RepoContext.h"

#include <QDate>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QProcess>
#include <QRegularExpression>
#include <QStandardPaths>

namespace {

struct ProcOut {
    bool started = false;
    int code = -1;
    QString out;
    QString err;
};

ProcOut runProgram(const QString &program, const QStringList &args, const QString &cwd, int timeoutMs)
{
    ProcOut result;
    QProcess process;
    if (!cwd.isEmpty())
        process.setWorkingDirectory(cwd);
    QProcessEnvironment env = QProcessEnvironment::systemEnvironment();
    env.insert(QStringLiteral("GH_PROMPT_DISABLED"), QStringLiteral("1"));
    env.insert(QStringLiteral("GH_NO_UPDATE_NOTIFIER"), QStringLiteral("1"));
    process.setProcessEnvironment(env);
    process.setProcessChannelMode(QProcess::SeparateChannels);
    process.start(program, args);
    if (!process.waitForStarted(4000)) {
        result.err = process.errorString();
        return result;
    }
    result.started = true;
    if (!process.waitForFinished(timeoutMs)) {
        process.kill();
        process.waitForFinished(2000);
        result.err = QStringLiteral("timeout");
        result.code = -1;
        return result;
    }
    result.code = process.exitCode();
    result.out = QString::fromUtf8(process.readAllStandardOutput());
    result.err = QString::fromUtf8(process.readAllStandardError());
    return result;
}

bool filePresent(const QString &root, const QStringList &names)
{
    for (const QString &name : names) {
        if (QFileInfo::exists(root + QLatin1Char('/') + name))
            return true;
    }
    return false;
}

} // namespace

GithubClient::GithubClient(RepoContext *repo, QObject *parent)
    : QObject(parent)
    , m_repo(repo)
{
}

bool GithubClient::available() const { return m_available; }
bool GithubClient::authenticated() const { return m_authenticated; }
QString GithubClient::login() const { return m_login; }
QString GithubClient::host() const { return m_host; }
bool GithubClient::hasOrigin() const { return m_hasOrigin; }
bool GithubClient::originIsGithub() const { return m_originIsGithub; }
QString GithubClient::originUrl() const { return m_originUrl; }
QString GithubClient::slug() const { return m_slug; }
QString GithubClient::description() const { return m_description; }
QString GithubClient::homepage() const { return m_homepage; }
QString GithubClient::visibility() const { return m_visibility; }
QString GithubClient::htmlUrl() const { return m_htmlUrl; }
QString GithubClient::repoName() const { return m_repoName; }
QString GithubClient::defaultBranch() const { return m_defaultBranch; }
QStringList GithubClient::topics() const { return m_topics; }
bool GithubClient::hasReadme() const { return m_hasReadme; }
bool GithubClient::hasLicense() const { return m_hasLicense; }
bool GithubClient::hasGitignore() const { return m_hasGitignore; }
bool GithubClient::hasLfsAttributes() const { return m_hasLfsAttributes; }
bool GithubClient::issuesEnabled() const { return m_issues; }
bool GithubClient::wikiEnabled() const { return m_wiki; }
bool GithubClient::projectsEnabled() const { return m_projects; }
bool GithubClient::discussionsEnabled() const { return m_discussions; }
bool GithubClient::deleteBranchOnMerge() const { return m_deleteBranchOnMerge; }
bool GithubClient::isTemplate() const { return m_isTemplate; }
QVariantMap GithubClient::policy() const { return githubPolicyToMap(m_policy); }
GithubPolicy GithubClient::policyState() const { return m_policy; }

QVariantMap GithubClient::pages() const
{
    QVariantMap map;
    map.insert(QStringLiteral("known"), m_pages.known);
    map.insert(QStringLiteral("enabled"), m_pages.enabled);
    map.insert(QStringLiteral("branch"), m_pages.branch);
    map.insert(QStringLiteral("path"), m_pages.path.isEmpty() ? QStringLiteral("/") : m_pages.path);
    map.insert(QStringLiteral("cname"), m_pages.cname);
    map.insert(QStringLiteral("https"), m_pages.https);
    map.insert(QStringLiteral("httpsKnown"), m_pages.httpsKnown);
    map.insert(QStringLiteral("htmlUrl"), m_pages.htmlUrl);
    map.insert(QStringLiteral("buildType"), m_pages.buildType);
    return map;
}

GithubPagesState GithubClient::pagesState() const { return m_pages; }
QStringList GithubClient::wikiPages() const { return m_wikiPages; }
QStringList GithubClient::emails() const { return m_emails; }
QString GithubClient::statusLine() const { return m_statusLine; }
QString GithubClient::note() const { return m_note; }

void GithubClient::setNote(const QString &note)
{
    if (m_note == note)
        return;
    m_note = note;
    emit changed();
}

QString GithubClient::suggestedName() const
{
    if (!m_repo || !m_repo->isOpen())
        return {};
    QString name = QFileInfo(m_repo->rootPath()).fileName();
    name.replace(QRegularExpression(QStringLiteral("[^A-Za-z0-9._-]")), QStringLiteral("-"));
    name.replace(QRegularExpression(QStringLiteral("-{2,}")), QStringLiteral("-"));
    while (name.startsWith(QLatin1Char('-')))
        name.remove(0, 1);
    while (name.endsWith(QLatin1Char('-')))
        name.chop(1);
    return name;
}

QVariantList GithubClient::checklist() const
{
    QVariantList rows;
    auto add = [&rows](const QString &key, const QString &title, bool ok, const QString &detail) {
        QVariantMap row;
        row.insert(QStringLiteral("key"), key);
        row.insert(QStringLiteral("title"), title);
        row.insert(QStringLiteral("ok"), ok);
        row.insert(QStringLiteral("detail"), detail);
        rows.append(row);
    };
    add(QStringLiteral("gh"), QStringLiteral("GitHub CLI"), m_available,
        m_available ? QStringLiteral("gh найден") : QStringLiteral("Поставьте пакет gh"));
    add(QStringLiteral("auth"), QStringLiteral("Вход"), m_authenticated,
        m_authenticated ? m_login : QStringLiteral("gh auth login"));
    add(QStringLiteral("commits"), QStringLiteral("Локальные коммиты"),
        m_repo && m_repo->isOpen() && m_repo->localCommits() > 0,
        m_repo && m_repo->isOpen() ? QString::number(m_repo->localCommits()) : QStringLiteral("—"));
    add(QStringLiteral("readme"), QStringLiteral("README"), m_hasReadme,
        m_hasReadme ? QStringLiteral("есть") : QStringLiteral("нет файла"));
    add(QStringLiteral("license"), QStringLiteral("Лицензия"), m_hasLicense,
        m_hasLicense ? QStringLiteral("есть") : QStringLiteral("нет файла"));
    add(QStringLiteral("ignore"), QStringLiteral(".gitignore"), m_hasGitignore,
        m_hasGitignore ? QStringLiteral("есть") : QStringLiteral("нет файла"));
    add(QStringLiteral("origin"), QStringLiteral("GitHub origin"), m_originIsGithub,
        m_originIsGithub ? m_slug : (m_hasOrigin ? m_originUrl : QStringLiteral("remote origin нет")));
    add(QStringLiteral("about"), QStringLiteral("Описание на GitHub"),
        m_originIsGithub && !m_description.trimmed().isEmpty(),
        m_originIsGithub ? (m_description.isEmpty() ? QStringLiteral("пусто") : m_description)
                         : QStringLiteral("после публикации"));
    const bool published = m_originIsGithub && m_repo && m_repo->isOpen() && m_repo->hasUpstream();
    add(QStringLiteral("push"), QStringLiteral("Ветка на GitHub"), published,
        published ? QStringLiteral("upstream есть") : QStringLiteral("нужен push -u"));
    return rows;
}

void GithubClient::refreshIfStale(int seconds)
{
    if (!m_refreshedAt.isValid() || m_refreshedAt.secsTo(QDateTime::currentDateTime()) >= seconds)
        refresh();
}

void GithubClient::refreshLocalDress()
{
    m_hasReadme = false;
    m_hasLicense = false;
    m_hasGitignore = false;
    m_hasLfsAttributes = false;
    m_dressPresent.clear();
    const QString root = (m_repo && m_repo->isOpen()) ? m_repo->rootPath() : QString();
    if (root.isEmpty()) {
        emit changed();
        return;
    }
    for (const QString &kind : dressKinds())
        m_dressPresent.insert(kind, dressFilePresent(root, kind));
    m_hasReadme = m_dressPresent.value(QStringLiteral("readme")).toBool();
    m_hasLicense = m_dressPresent.value(QStringLiteral("license")).toBool();
    m_hasGitignore = m_dressPresent.value(QStringLiteral("gitignore")).toBool();
    QFile attributes(root + QStringLiteral("/.gitattributes"));
    if (attributes.open(QIODevice::ReadOnly)) {
        const QByteArray body = attributes.read(256 * 1024);
        m_hasLfsAttributes = body.contains("filter=lfs");
    }
    emit changed();
}

QString GithubClient::takePendingTemplate(const QString &kind)
{
    const QString key = kind.trimmed().toLower();
    if (key.isEmpty() || key != m_pendingTemplateKind)
        return {};
    const QString body = m_pendingTemplateBody;
    m_pendingTemplateKind.clear();
    m_pendingTemplateBody.clear();
    return body;
}

void GithubClient::refresh()
{
    m_available = !QStandardPaths::findExecutable(QStringLiteral("gh")).isEmpty();
    m_authenticated = false;
    m_login.clear();
    m_host.clear();
    m_hasOrigin = false;
    m_originIsGithub = false;
    m_originUrl.clear();
    m_slug.clear();
    m_description.clear();
    m_homepage.clear();
    m_visibility.clear();
    m_htmlUrl.clear();
    m_repoName.clear();
    m_defaultBranch.clear();
    m_topics.clear();
    m_issues = false;
    m_wiki = false;
    m_projects = false;
    m_discussions = false;
    m_deleteBranchOnMerge = false;
    m_isTemplate = false;
    m_policy = {};
    m_pages = {};
    m_emails.clear();
    m_hasReadme = false;
    m_hasLicense = false;
    m_hasGitignore = false;
    m_hasLfsAttributes = false;
    m_dressPresent.clear();

    const QString root = (m_repo && m_repo->isOpen()) ? m_repo->rootPath() : QString();
    if (!root.isEmpty()) {
        for (const QString &kind : dressKinds())
            m_dressPresent.insert(kind, dressFilePresent(root, kind));
        m_hasReadme = m_dressPresent.value(QStringLiteral("readme")).toBool();
        m_hasLicense = m_dressPresent.value(QStringLiteral("license")).toBool();
        m_hasGitignore = m_dressPresent.value(QStringLiteral("gitignore")).toBool();
        QFile attributes(root + QStringLiteral("/.gitattributes"));
        if (attributes.open(QIODevice::ReadOnly)) {
            const QByteArray body = attributes.read(256 * 1024);
            m_hasLfsAttributes = body.contains("filter=lfs");
        }

        GitRunner git(8000);
        const GitRunResult url = git.run(root, {QStringLiteral("remote"), QStringLiteral("get-url"),
                                                 QStringLiteral("origin")});
        if (url.ok) {
            m_hasOrigin = true;
            m_originUrl = QString::fromUtf8(url.stdoutBytes).trimmed();
            const GithubRemote parsed = parseGithubRemote(m_originUrl);
            if (parsed.ok) {
                m_originIsGithub = true;
                m_host = parsed.host;
                m_repoName = parsed.name;
                m_slug = parsed.owner + QLatin1Char('/') + parsed.name;
            }
        }
    }

    if (!m_available) {
        m_statusLine = QStringLiteral("GitHub CLI (gh) не установлен.");
        m_refreshedAt = QDateTime::currentDateTime();
        emit changed();
        return;
    }

    const ProcOut auth = runProgram(QStringLiteral("gh"), {QStringLiteral("auth"), QStringLiteral("status")}, root,
                                    8000);
    const QRegularExpression loggedIn(
        QStringLiteral("Logged in to (\\S+) (?:account|as) (\\S+)"));
    const QRegularExpressionMatch authMatch = loggedIn.match(auth.out + QLatin1Char('\n') + auth.err);
    if (auth.started && auth.code == 0 && authMatch.hasMatch()) {
        m_authenticated = true;
        if (m_host.isEmpty())
            m_host = authMatch.captured(1);
        m_login = authMatch.captured(2);
    }

    if (m_authenticated && m_originIsGithub && !root.isEmpty()) {
        const ProcOut view = runProgram(
            QStringLiteral("gh"),
            {QStringLiteral("repo"), QStringLiteral("view"), QStringLiteral("--json"),
             QStringLiteral("name,description,url,visibility,homepageUrl,repositoryTopics,"
                           "hasIssuesEnabled,hasWikiEnabled,hasProjectsEnabled,hasDiscussionsEnabled,"
                           "deleteBranchOnMerge,isTemplate,defaultBranchRef,isArchived,"
                           "mergeCommitAllowed,squashMergeAllowed,rebaseMergeAllowed")},
            root, 12000);
        const QJsonObject obj = QJsonDocument::fromJson(view.out.toUtf8()).object();
        if (view.code == 0 && !obj.isEmpty()) {
            m_description = obj.value(QStringLiteral("description")).toString();
            m_homepage = obj.value(QStringLiteral("homepageUrl")).toString();
            m_visibility = obj.value(QStringLiteral("visibility")).toString().toLower();
            m_htmlUrl = obj.value(QStringLiteral("url")).toString();
            const QString jsonName = obj.value(QStringLiteral("name")).toString();
            if (!jsonName.isEmpty())
                m_repoName = jsonName;
            m_defaultBranch = obj.value(QStringLiteral("defaultBranchRef")).toObject().value(QStringLiteral("name")).toString();
            m_issues = obj.value(QStringLiteral("hasIssuesEnabled")).toBool();
            m_wiki = obj.value(QStringLiteral("hasWikiEnabled")).toBool();
            m_projects = obj.value(QStringLiteral("hasProjectsEnabled")).toBool();
            m_discussions = obj.value(QStringLiteral("hasDiscussionsEnabled")).toBool();
            m_deleteBranchOnMerge = obj.value(QStringLiteral("deleteBranchOnMerge")).toBool();
            m_isTemplate = obj.value(QStringLiteral("isTemplate")).toBool();
            const QJsonArray topics = obj.value(QStringLiteral("repositoryTopics")).toArray();
            for (const QJsonValue &topic : topics) {
                const QString name = topic.toObject().value(QStringLiteral("name")).toString();
                if (!name.isEmpty())
                    m_topics.append(name);
            }
            githubPolicyFillFromView(&m_policy, obj);
        }

        const ProcOut api = runProgram(QStringLiteral("gh"),
                                       {QStringLiteral("api"), QStringLiteral("repos/%1").arg(m_slug)}, root, 12000);
        const QJsonObject repo = QJsonDocument::fromJson(api.out.toUtf8()).object();
        if (api.code == 0 && (repo.contains(QStringLiteral("full_name")) || repo.contains(QStringLiteral("id")))) {
            GithubPolicy parsed = githubPolicyFromApi(repo);
            if (view.code == 0 && !obj.isEmpty())
                githubPolicyFillFromView(&parsed, obj);
            m_policy = parsed;
        }

        const ProcOut pages = runProgram(QStringLiteral("gh"),
                                        {QStringLiteral("api"), QStringLiteral("repos/%1/pages").arg(m_slug)}, root, 12000);
        const QJsonObject pagesObj = QJsonDocument::fromJson(pages.out.toUtf8()).object();
        const QString pagesErr = pages.out + pages.err;
        if (pages.code == 0 && pagesObj.contains(QStringLiteral("html_url")))
            m_pages = githubPagesFromApi(200, pagesObj);
        else if (pagesErr.contains(QStringLiteral("Not Found")))
            m_pages = githubPagesFromApi(404, {});
    }

    if (!m_authenticated)
        m_statusLine = QStringLiteral("gh установлен, входа нет. В терминале: gh auth login");
    else if (!m_originIsGithub)
        m_statusLine = m_login + QStringLiteral(" · репозиторий ещё не на GitHub");
    else
        m_statusLine = QStringLiteral("%1 · %2%3")
                           .arg(m_login, m_slug, m_visibility.isEmpty() ? QString() : QStringLiteral(" · ") + m_visibility);

    m_refreshedAt = QDateTime::currentDateTime();
    emit changed();
}

QString GithubClient::snapshotText() const
{
    QStringList lines;
    lines << QStringLiteral("gh: %1").arg(m_available ? QStringLiteral("installed") : QStringLiteral("missing"));
    lines << QStringLiteral("auth: %1").arg(m_authenticated ? m_login : QStringLiteral("no"));
    lines << QStringLiteral("origin: %1").arg(m_originUrl.isEmpty() ? QStringLiteral("none") : m_originUrl);
    lines << QStringLiteral("github: %1").arg(m_originIsGithub ? m_slug : QStringLiteral("no"));
    lines << QStringLiteral("visibility: %1").arg(m_visibility.isEmpty() ? QStringLiteral("unknown") : m_visibility);
    lines << QStringLiteral("description: %1")
                 .arg(m_description.trimmed().isEmpty() ? QStringLiteral("(empty)") : m_description.trimmed());
    lines << QStringLiteral("homepage: %1").arg(m_homepage.isEmpty() ? QStringLiteral("(empty)") : m_homepage);
    lines << QStringLiteral("topics: %1").arg(m_topics.isEmpty() ? QStringLiteral("(none)") : m_topics.join(QStringLiteral(", ")));
    QStringList dressBits;
    for (const QString &kind : dressKinds()) {
        const bool present = m_dressPresent.contains(kind) ? m_dressPresent.value(kind).toBool()
                                                           : (kind == QLatin1String("readme")     ? m_hasReadme
                                                              : kind == QLatin1String("license")  ? m_hasLicense
                                                              : kind == QLatin1String("gitignore") ? m_hasGitignore
                                                                                                  : false);
        dressBits << QStringLiteral("%1=%2").arg(kind, present ? QStringLiteral("yes") : QStringLiteral("no"));
    }
    lines << QStringLiteral("dress: %1").arg(dressBits.join(QLatin1Char(' ')));
    lines << QStringLiteral("readme: %1").arg(m_hasReadme ? QStringLiteral("yes") : QStringLiteral("no"));
    lines << QStringLiteral("license: %1").arg(m_hasLicense ? QStringLiteral("yes") : QStringLiteral("no"));
    lines << QStringLiteral("gitignore: %1").arg(m_hasGitignore ? QStringLiteral("yes") : QStringLiteral("no"));
    lines << QStringLiteral("lfsAttributes: %1").arg(m_hasLfsAttributes ? QStringLiteral("yes") : QStringLiteral("no"));
    lines << QStringLiteral("html: %1").arg(m_htmlUrl.isEmpty() ? QStringLiteral("(none)") : m_htmlUrl);
    lines << QStringLiteral("defaultBranch: %1").arg(m_defaultBranch.isEmpty() ? QStringLiteral("(unknown)") : m_defaultBranch);
    const auto tri = [](const GithubFlag &flag) {
        if (!flag.known)
            return QStringLiteral("unknown");
        return flag.value ? QStringLiteral("on") : QStringLiteral("off");
    };
    if (m_pages.known)
        lines << QStringLiteral("pages: %1 branch: %2 path: %3 cname: %4 https: %5")
                     .arg(m_pages.enabled ? QStringLiteral("on") : QStringLiteral("off"),
                          m_pages.branch.isEmpty() ? QStringLiteral("(none)") : m_pages.branch,
                          m_pages.path.isEmpty() ? QStringLiteral("/") : m_pages.path,
                          m_pages.cname.isEmpty() ? QStringLiteral("(none)") : m_pages.cname,
                          m_pages.httpsKnown ? (m_pages.https ? QStringLiteral("on") : QStringLiteral("off"))
                                            : QStringLiteral("unknown"));
    if (m_policy.loaded) {
        lines << QStringLiteral("merge: %1 squash: %2 rebase: %3 auto: %4 updateBranch: %5 fork: %6 signoff: %7 archived: %8")
                     .arg(tri(m_policy.mergeCommit), tri(m_policy.squashMerge), tri(m_policy.rebaseMerge),
                          tri(m_policy.autoMerge), tri(m_policy.updateBranch), tri(m_policy.allowForking),
                          tri(m_policy.webSignoff), tri(m_policy.archived));
        lines << QStringLiteral("pullRequests: %1 prPolicy: %2 scanning: %3 pushProtection: %4 advancedSecurity: %5")
                     .arg(tri(m_policy.pullRequests),
                          m_policy.prPolicyKnown ? m_policy.prPolicy : QStringLiteral("unknown"),
                          tri(m_policy.secretScanning), tri(m_policy.pushProtection), tri(m_policy.advancedSecurity));
    }
    lines << QStringLiteral("issues: %1 wiki: %2 projects: %3 discussions: %4 template: %5 deleteBranchOnMerge: %6")
                 .arg(m_issues ? QStringLiteral("on") : QStringLiteral("off"),
                      m_wiki ? QStringLiteral("on") : QStringLiteral("off"),
                      m_projects ? QStringLiteral("on") : QStringLiteral("off"),
                      m_discussions ? QStringLiteral("on") : QStringLiteral("off"),
                      m_isTemplate ? QStringLiteral("on") : QStringLiteral("off"),
                      m_deleteBranchOnMerge ? QStringLiteral("on") : QStringLiteral("off"));
    lines << QStringLiteral("suggestedName: %1").arg(suggestedName().isEmpty() ? QStringLiteral("(none)") : suggestedName());
    if (m_repo && m_repo->isOpen())
        lines << QStringLiteral("upstream: %1").arg(m_repo->hasUpstream() ? QStringLiteral("yes") : QStringLiteral("no"));
    lines << QStringLiteral("services: issues, pulls, discussions, projects, releases and autolinks are edited on the GitHub tab");
    lines << QStringLiteral("access: collaborators, actions, secrets, rulesets, branch protection and codespaces are edited on the GitHub tab");
    lines << QStringLiteral("identity: profile, emails, keys, organization card, teams and security policy are edited on the GitHub tab");
    lines << QStringLiteral("followup: environment reviewers, project views, banner expiry, selected security attach and org roles are edited on the GitHub tab");
    if (!m_serviceDigest.isEmpty())
        lines << QStringLiteral("serviceDigest:\n%1").arg(m_serviceDigest.left(800));
    return lines.join(QLatin1Char('\n'));
}

GitRunResult GithubClient::applyFileCommand(const QString &commandId, const QVariantMap &args)
{
    GitRunResult result;
    if (!m_repo || !m_repo->isOpen()) {
        result.error = QStringLiteral("Репозиторий не открыт.");
        result.exitCode = 1;
        return result;
    }
    const QString root = m_repo->rootPath();
    QString error;
    bool ok = false;
    QString created;
    if (commandId == QLatin1String("github_readme")) {
        ok = writeScaffoldReadme(root, args.value(QStringLiteral("title")).toString(),
                                 args.value(QStringLiteral("blurb")).toString(), &error);
        created = QStringLiteral("README.md");
    } else if (commandId == QLatin1String("github_license")) {
        QString owner;
        GitRunner git(4000);
        const GitRunResult name = git.run(root, {QStringLiteral("config"), QStringLiteral("user.name")});
        if (name.ok)
            owner = QString::fromUtf8(name.stdoutBytes).trimmed();
        ok = writeScaffoldLicense(root, args.value(QStringLiteral("kind")).toString(), owner,
                                  QDate::currentDate().year(), &error);
        created = QStringLiteral("LICENSE");
    } else {
        result.error = QStringLiteral("Неизвестная команда оформления: %1").arg(commandId);
        result.exitCode = 1;
        return result;
    }
    result.ok = ok;
    result.exitCode = ok ? 0 : 1;
    if (ok) {
        result.stdoutBytes = QStringLiteral("Создан %1").arg(created).toUtf8();
        refreshLocalDress();
    } else {
        result.error = error;
    }
    return result;
}

QString GithubClient::dressText(const QString &kind) const
{
    if (!m_repo || !m_repo->isOpen())
        return {};
    return readDressFile(m_repo->rootPath(), kind);
}

QString GithubClient::dressFileNameFor(const QString &kind) const
{
    const QString root = (m_repo && m_repo->isOpen()) ? m_repo->rootPath() : QString();
    return dressFileName(root, kind);
}

QString GithubClient::readmeTemplate(const QString &title, const QString &blurb) const
{
    const QString heading = title.trimmed().isEmpty() ? suggestedName() : title.trimmed();
    return scaffoldReadmeText(heading, blurb);
}

QString GithubClient::dressTemplate(const QString &kind) const
{
    return dressTemplateText(kind, suggestedName());
}

QString GithubClient::fetchTemplate(const QString &kind, const QString &key)
{
    QString error;
    const QStringList argv = githubTemplateArgv(kind, key, &error);
    if (argv.isEmpty()) {
        setNote(error);
        return {};
    }
    const QString root = (m_repo && m_repo->isOpen()) ? m_repo->rootPath() : QString();
    const ProcOut view = runProgram(QStringLiteral("gh"), argv, root, 12000);
    if (view.code != 0) {
        const QString message = (view.err + view.out).trimmed();
        setNote(message.isEmpty() ? QStringLiteral("Шаблон GitHub не найден.") : message);
        return {};
    }
    m_pendingTemplateKind = kind.trimmed().toLower();
    m_pendingTemplateBody = view.out;
    setNote(QStringLiteral("Шаблон подставлен в черновик. Файл ещё не записан."));
    return view.out;
}

void GithubClient::refreshEmails()
{
    m_emails.clear();
    if (!m_available || !m_authenticated) {
        setNote(QStringLiteral("Список почты доступен после gh auth login."));
        emit changed();
        return;
    }
    const QString root = (m_repo && m_repo->isOpen()) ? m_repo->rootPath() : QDir::homePath();
    const ProcOut out = runProgram(QStringLiteral("gh"),
                                   {QStringLiteral("api"), QStringLiteral("--method"), QStringLiteral("GET"),
                                    QStringLiteral("user/emails")},
                                   root, 15000);
    if (out.code != 0) {
        const QString message = (out.err + out.out).trimmed();
        setNote(message.isEmpty() ? QStringLiteral("Не удалось прочитать почту. Нужен scope user.") : message.left(300));
        emit changed();
        return;
    }
    QString error;
    m_emails = githubEmailLines(out.out.toUtf8(), &error);
    if (!error.isEmpty()) {
        m_emails.clear();
        setNote(error);
    } else {
        setNote(m_emails.isEmpty() ? QStringLiteral("У учётной записи нет адресов.")
                                   : QStringLiteral("Адресов: %1. Сами адреса в снимок советника не попадают.").arg(m_emails.size()));
    }
    emit changed();
}

void GithubClient::refreshWiki()
{
    m_wikiPages.clear();
    if (!m_originIsGithub) {
        setNote(QStringLiteral("Wiki доступна, когда origin указывает на GitHub."));
        return;
    }
    const QString url = githubWikiRemoteUrl(m_originUrl);
    const QString cache = githubWikiCacheDir(m_slug);
    const GitRunResult ready = prepareWikiCheckout(url, cache);
    if (!ready.ok) {
        setNote(ready.error);
        return;
    }
    m_wikiPages = listWikiPages(cache);
    setNote(m_wikiPages.isEmpty() ? QStringLiteral("Wiki пустая. Введите имя страницы и отправьте её.")
                                  : QStringLiteral("Страницы wiki: %1").arg(m_wikiPages.join(QStringLiteral(", "))));
    emit changed();
}

QString GithubClient::wikiPageText(const QString &page) const
{
    return readWikiPage(githubWikiCacheDir(m_slug), page);
}

QString GithubClient::licenseTemplate(const QString &kind) const
{
    QString owner;
    if (m_repo && m_repo->isOpen()) {
        GitRunner git(4000);
        const GitRunResult name = git.run(m_repo->rootPath(), {QStringLiteral("config"), QStringLiteral("user.name")});
        if (name.ok)
            owner = QString::fromUtf8(name.stdoutBytes).trimmed();
    }
    return scaffoldLicenseText(kind, owner, QDate::currentDate().year());
}

GitRunResult GithubClient::saveDressFile(const QString &kind, const QString &body)
{
    GitRunResult result;
    if (!m_repo || !m_repo->isOpen()) {
        result.error = QStringLiteral("Репозиторий не открыт.");
        result.exitCode = 1;
        return result;
    }
    QString error;
    const bool ok = writeDressFile(m_repo->rootPath(), kind, body, &error);
    result.ok = ok;
    result.exitCode = ok ? 0 : 1;
    if (!ok) {
        result.error = error;
        return result;
    }
    const QString name = dressFileName(m_repo->rootPath(), kind);
    result.stdoutBytes = QStringLiteral("Записан %1").arg(name).toUtf8();
    refreshLocalDress();
    m_note = QString::fromUtf8(result.stdoutBytes);
    emit changed();
    return result;
}

QString GithubClient::serviceDigest() const
{
    return m_serviceDigest;
}

QString GithubClient::projectNodeId(const QString &number, QString *error) const
{
    static const QRegularExpression numberRe(QStringLiteral("^[1-9][0-9]{0,8}$"));
    const QString n = number.trimmed();
    if (!numberRe.match(n).hasMatch()) {
        if (error)
            *error = QStringLiteral("Номер проекта — целое число без нуля впереди.");
        return {};
    }
    if (!m_originIsGithub || !m_repo) {
        if (error)
            *error = QStringLiteral("Проект читается, когда origin указывает на GitHub.");
        return {};
    }
    const QString owner = m_login.trimmed().isEmpty() ? QStringLiteral("@me") : m_login.trimmed();
    const ProcOut out = runProgram(QStringLiteral("gh"),
                                   {QStringLiteral("project"), QStringLiteral("view"), n, QStringLiteral("--owner"), owner,
                                    QStringLiteral("--format"), QStringLiteral("json")},
                                   m_repo->rootPath(), 15000);
    if (out.code != 0) {
        const QString message = (out.err + out.out).trimmed();
        if (error) {
            *error = message.isEmpty()
                         ? QStringLiteral("Проект не прочитан. Для проектов нужен scope project: gh auth refresh -s project.")
                         : message;
        }
        return {};
    }
    const QString id = QJsonDocument::fromJson(out.out.toUtf8()).object().value(QStringLiteral("id")).toString().trimmed();
    static const QRegularExpression idRe(QStringLiteral("^PVT_[A-Za-z0-9_]+$"));
    if (!idRe.match(id).hasMatch()) {
        if (error)
            *error = QStringLiteral("В ответе проекта нет id. Для проектов нужен scope project.");
        return {};
    }
    return id;
}

void GithubClient::refreshServices()
{
    if (!m_originIsGithub || !m_authenticated || !m_repo) {
        setNote(QStringLiteral("Списки сервисов доступны, когда origin указывает на GitHub и выполнен gh auth login."));
        return;
    }
    const QString root = m_repo->rootPath();
    const auto brief = [](const QByteArray &json, const auto &one) {
        const QJsonDocument doc = QJsonDocument::fromJson(json);
        QJsonArray arr;
        if (doc.isArray())
            arr = doc.array();
        else if (doc.object().contains(QStringLiteral("projects")))
            arr = doc.object().value(QStringLiteral("projects")).toArray();
        if (arr.isEmpty())
            return QStringLiteral("пусто");
        QStringList parts;
        const int n = qMin(static_cast<int>(arr.size()), 12);
        for (int i = 0; i < n; ++i) {
            if (!arr.at(i).isObject())
                continue;
            const QString line = one(arr.at(i).toObject()).simplified();
            if (!line.isEmpty())
                parts << line.left(80);
        }
        if (parts.isEmpty())
            return QStringLiteral("пусто");
        QString text = parts.join(QStringLiteral(" · "));
        if (arr.size() > n)
            text += QStringLiteral(" · …");
        return text;
    };
    const auto section = [&](const QString &title, const QStringList &args, const auto &one) {
        const ProcOut out = runProgram(QStringLiteral("gh"), args, root, 8000);
        if (out.code != 0) {
            const QString message = (out.err + out.out).trimmed().left(160);
            return title + QStringLiteral(": ") + (message.isEmpty() ? QStringLiteral("нет ответа") : message);
        }
        return title + QStringLiteral(": ") + brief(out.out.toUtf8(), one);
    };
    QStringList lines;
    lines << section(QStringLiteral("issues"),
                     {QStringLiteral("issue"), QStringLiteral("list"), QStringLiteral("-L"), QStringLiteral("12"),
                      QStringLiteral("--state"), QStringLiteral("all"), QStringLiteral("--json"),
                      QStringLiteral("number,title,state")},
                     [](const QJsonObject &row) {
                         return QStringLiteral("#%1 %2 %3")
                             .arg(row.value(QStringLiteral("number")).toInt())
                             .arg(row.value(QStringLiteral("state")).toString(), row.value(QStringLiteral("title")).toString());
                     });
    lines << section(QStringLiteral("pulls"),
                     {QStringLiteral("pr"), QStringLiteral("list"), QStringLiteral("-L"), QStringLiteral("12"),
                      QStringLiteral("--state"), QStringLiteral("all"), QStringLiteral("--json"),
                      QStringLiteral("number,title,isDraft")},
                     [](const QJsonObject &row) {
                         return QStringLiteral("#%1 %2 %3")
                             .arg(row.value(QStringLiteral("number")).toInt())
                             .arg(row.value(QStringLiteral("isDraft")).toBool() ? QStringLiteral("draft")
                                                                                : QStringLiteral("ready"),
                                  row.value(QStringLiteral("title")).toString());
                     });
    lines << section(QStringLiteral("discussions"),
                     {QStringLiteral("discussion"), QStringLiteral("list"), QStringLiteral("-L"), QStringLiteral("12"),
                      QStringLiteral("--state"), QStringLiteral("all"), QStringLiteral("--json"),
                      QStringLiteral("number,title,category")},
                     [](const QJsonObject &row) {
                         const QJsonValue category = row.value(QStringLiteral("category"));
                         const QString name = category.isObject() ? category.toObject().value(QStringLiteral("name")).toString()
                                                                  : category.toString();
                         return QStringLiteral("#%1 %2 %3")
                             .arg(row.value(QStringLiteral("number")).toInt())
                             .arg(name, row.value(QStringLiteral("title")).toString());
                     });
    lines << section(QStringLiteral("labels"),
                     {QStringLiteral("label"), QStringLiteral("list"), QStringLiteral("-L"), QStringLiteral("30"),
                      QStringLiteral("--json"), QStringLiteral("name")},
                     [](const QJsonObject &row) { return row.value(QStringLiteral("name")).toString(); });
    lines << section(QStringLiteral("milestones"),
                     {QStringLiteral("api"), QStringLiteral("repos/%1/milestones").arg(m_slug), QStringLiteral("--method"),
                      QStringLiteral("GET")},
                     [](const QJsonObject &row) {
                         return QStringLiteral("#%1 %2 %3")
                             .arg(row.value(QStringLiteral("number")).toInt())
                             .arg(row.value(QStringLiteral("state")).toString(), row.value(QStringLiteral("title")).toString());
                     });
    lines << section(QStringLiteral("releases"),
                     {QStringLiteral("release"), QStringLiteral("list"), QStringLiteral("-L"), QStringLiteral("12"),
                      QStringLiteral("--json"), QStringLiteral("tagName,name,isDraft")},
                     [](const QJsonObject &row) {
                         return QStringLiteral("%1 %2 %3")
                             .arg(row.value(QStringLiteral("tagName")).toString(),
                                  row.value(QStringLiteral("isDraft")).toBool() ? QStringLiteral("draft")
                                                                                : QStringLiteral("published"),
                                  row.value(QStringLiteral("name")).toString());
                     });
    lines << section(QStringLiteral("autolinks"),
                     {QStringLiteral("repo"), QStringLiteral("autolink"), QStringLiteral("list"), QStringLiteral("--json"),
                      QStringLiteral("id,keyPrefix,urlTemplate")},
                     [](const QJsonObject &row) {
                         return QStringLiteral("#%1 %2")
                             .arg(row.value(QStringLiteral("id")).toInt())
                             .arg(row.value(QStringLiteral("keyPrefix")).toString());
                     });
    lines << section(QStringLiteral("projects"),
                     {QStringLiteral("project"), QStringLiteral("list"), QStringLiteral("--owner"),
                      m_login.trimmed().isEmpty() ? QStringLiteral("@me") : m_login.trimmed(), QStringLiteral("--limit"),
                      QStringLiteral("12"), QStringLiteral("--format"), QStringLiteral("json")},
                     [](const QJsonObject &row) {
                         return QStringLiteral("#%1 %2")
                             .arg(row.value(QStringLiteral("number")).toInt())
                             .arg(row.value(QStringLiteral("title")).toString());
                     });
    m_serviceDigest = lines.join(QLatin1Char('\n'));
    setNote(QStringLiteral("Списки issues, pull requests, обсуждений, релизов и проектов обновлены."));
    emit changed();
}
