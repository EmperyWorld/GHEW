#include "GithubLifecycle.h"

#include "GithubRemote.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QJsonObject>
#include <QRegularExpression>
#include <QStandardPaths>

namespace {

bool validLogin(const QString &login)
{
    const QRegularExpression re(QStringLiteral("^[A-Za-z0-9](?:[A-Za-z0-9-]{0,37}[A-Za-z0-9])?$"));
    return re.match(login).hasMatch();
}

bool validRepoName(const QString &name)
{
    const QRegularExpression re(QStringLiteral("^[A-Za-z0-9._-]{1,100}$"));
    return re.match(name).hasMatch();
}

bool confirmName(const QString &repoName, const QString &typedName, QString *error)
{
    if (repoName.isEmpty()) {
        if (error)
            *error = QStringLiteral("Имя репозитория ещё не прочитано.");
        return false;
    }
    if (typedName != repoName) {
        if (error)
            *error = QStringLiteral("Введите имя репозитория «%1» без владельца.").arg(repoName);
        return false;
    }
    return true;
}

} // namespace

QStringList githubForkArgv(const GithubForkRequest &request, QString *error)
{
    if (error)
        error->clear();
    const QString forkName = request.forkName.trimmed();
    const QString org = request.org.trimmed();
    if (!forkName.isEmpty() && !validRepoName(forkName)) {
        if (error)
            *error = QStringLiteral("Имя форка: буквы, цифры, точка, подчёркивание и дефис.");
        return {};
    }
    if (!org.isEmpty() && !validLogin(org)) {
        if (error)
            *error = QStringLiteral("Организация: буквы, цифры и дефис, без пробелов.");
        return {};
    }
    if (!request.rewireOrigin && request.slug.trimmed().isEmpty()) {
        if (error)
            *error = QStringLiteral("Нет адреса owner/repo для форка.");
        return {};
    }

    QStringList argv{QStringLiteral("repo"), QStringLiteral("fork")};
    if (!request.rewireOrigin)
        argv << request.slug.trimmed();
    argv << QStringLiteral("--clone=false");
    if (request.rewireOrigin)
        argv << QStringLiteral("--remote");
    if (request.defaultBranchOnly)
        argv << QStringLiteral("--default-branch-only");
    if (!forkName.isEmpty())
        argv << QStringLiteral("--fork-name") << forkName;
    if (!org.isEmpty())
        argv << QStringLiteral("--org") << org;
    return argv;
}

QStringList githubDeleteArgv(const QString &slug, const QString &repoName, const QString &typedName, QString *error)
{
    if (error)
        error->clear();
    if (slug.trimmed().isEmpty()) {
        if (error)
            *error = QStringLiteral("Нет адреса owner/repo для удаления.");
        return {};
    }
    if (!confirmName(repoName, typedName, error))
        return {};
    return {QStringLiteral("repo"), QStringLiteral("delete"), slug.trimmed(), QStringLiteral("--yes")};
}

QStringList githubTransferArgv(const QString &currentOwner, const QString &repoName, const QString &typedName,
                               const QString &newOwner, const QString &newName, const QString &teamIds, QString *error)
{
    if (error)
        error->clear();
    if (!confirmName(repoName, typedName, error))
        return {};
    const QString owner = newOwner.trimmed();
    if (!validLogin(owner)) {
        if (error)
            *error = QStringLiteral("Новый владелец: буквы, цифры и дефис, без пробелов.");
        return {};
    }
    if (owner.compare(currentOwner.trimmed(), Qt::CaseInsensitive) == 0) {
        if (error)
            *error = QStringLiteral("Репозиторий уже принадлежит %1.").arg(currentOwner.trimmed());
        return {};
    }
    const QString renamed = newName.trimmed();
    if (!renamed.isEmpty() && !validRepoName(renamed)) {
        if (error)
            *error = QStringLiteral("Новое имя: буквы, цифры, точка, подчёркивание и дефис.");
        return {};
    }

    QStringList ids;
    const QStringList parts = teamIds.split(QRegularExpression(QStringLiteral("[,\\s]+")), Qt::SkipEmptyParts);
    const QRegularExpression idRe(QStringLiteral("^[1-9][0-9]{0,17}$"));
    for (const QString &part : parts) {
        if (!idRe.match(part).hasMatch()) {
            if (error)
                *error = QStringLiteral("Id команды — положительное число: %1").arg(part);
            return {};
        }
        ids.append(part);
    }

    QStringList argv{QStringLiteral("api"), QStringLiteral("--method"), QStringLiteral("POST"),
                     QStringLiteral("repos/{owner}/{repo}/transfer"), QStringLiteral("-f"),
                     QStringLiteral("new_owner=%1").arg(owner)};
    if (!renamed.isEmpty())
        argv << QStringLiteral("-f") << QStringLiteral("new_name=%1").arg(renamed);
    for (const QString &id : ids)
        argv << QStringLiteral("-F") << QStringLiteral("team_ids[]=%1").arg(id);
    return argv;
}

QStringList githubLfsArgv(bool enable)
{
    return {QStringLiteral("api"), QStringLiteral("--method"), enable ? QStringLiteral("PUT") : QStringLiteral("DELETE"),
            QStringLiteral("repos/{owner}/{repo}/lfs")};
}

QStringList githubTemplateArgv(const QString &kind, const QString &key, QString *error)
{
    if (error)
        error->clear();
    const QString name = key.trimmed();
    const QRegularExpression re(QStringLiteral("^[A-Za-z0-9][A-Za-z0-9.+_-]{0,79}$"));
    if (!re.match(name).hasMatch()) {
        if (error)
            *error = QStringLiteral("Имя шаблона: буквы, цифры, точка, плюс, подчёркивание и дефис.");
        return {};
    }
    const QString which = kind.trimmed().toLower();
    if (which == QLatin1String("gitignore"))
        return {QStringLiteral("repo"), QStringLiteral("gitignore"), QStringLiteral("view"), name};
    if (which == QLatin1String("license"))
        return {QStringLiteral("repo"), QStringLiteral("license"), QStringLiteral("view"), name};
    if (error)
        *error = QStringLiteral("Каталог GitHub есть для лицензии и .gitignore.");
    return {};
}

GithubPagesState githubPagesFromApi(int httpCode, const QJsonObject &body)
{
    GithubPagesState state;
    if (httpCode == 404) {
        state.known = true;
        state.enabled = false;
        state.path = QStringLiteral("/");
        return state;
    }
    if (httpCode != 200 || !body.contains(QStringLiteral("html_url")))
        return state;
    state.known = true;
    state.enabled = true;
    state.htmlUrl = body.value(QStringLiteral("html_url")).toString();
    state.cname = body.value(QStringLiteral("cname")).toString();
    if (body.contains(QStringLiteral("https_enforced")) && body.value(QStringLiteral("https_enforced")).isBool()) {
        state.httpsKnown = true;
        state.https = body.value(QStringLiteral("https_enforced")).toBool();
    }
    state.buildType = body.value(QStringLiteral("build_type")).toString();
    const QJsonObject source = body.value(QStringLiteral("source")).toObject();
    state.branch = source.value(QStringLiteral("branch")).toString();
    state.path = source.value(QStringLiteral("path")).toString();
    if (state.path.isEmpty())
        state.path = QStringLiteral("/");
    return state;
}

QStringList githubPagesArgv(const GithubPagesState &want, const GithubPagesState &have, QString *error)
{
    if (error)
        error->clear();
    if (!have.known || !want.known) {
        if (error)
            *error = QStringLiteral("Состояние GitHub Pages ещё не прочитано.");
        return {};
    }
    if (!want.enabled && !have.enabled) {
        if (error)
            *error = QStringLiteral("Сайт GitHub Pages ещё не создан.");
        return {};
    }
    if (have.enabled && !want.enabled) {
        return {QStringLiteral("api"), QStringLiteral("--method"), QStringLiteral("DELETE"),
                QStringLiteral("repos/{owner}/{repo}/pages")};
    }
    if (want.path != QLatin1String("/") && want.path != QLatin1String("/docs")) {
        if (error)
            *error = QStringLiteral("Каталог Pages: / или /docs.");
        return {};
    }
    const QRegularExpression branchRe(QStringLiteral("^[A-Za-z0-9._/-]{1,250}$"));
    if (!branchRe.match(want.branch).hasMatch() || want.branch.contains(QLatin1String(".."))) {
        if (error)
            *error = QStringLiteral("Укажите ветку, из которой публикуется сайт.");
        return {};
    }
    if (!have.enabled) {
        if (!want.cname.trimmed().isEmpty() || want.https) {
            if (error)
                *error = QStringLiteral("Сначала создайте сайт веткой и каталогом. Домен и HTTPS записываются следующим сохранением.");
            return {};
        }
        return {QStringLiteral("api"),
                QStringLiteral("--method"),
                QStringLiteral("POST"),
                QStringLiteral("repos/{owner}/{repo}/pages"),
                QStringLiteral("-f"),
                QStringLiteral("source[branch]=%1").arg(want.branch),
                QStringLiteral("-f"),
                QStringLiteral("source[path]=%1").arg(want.path)};
    }
    QStringList argv{QStringLiteral("api"), QStringLiteral("--method"), QStringLiteral("PUT"),
                     QStringLiteral("repos/{owner}/{repo}/pages")};
    bool changed = false;
    if (want.branch != have.branch || want.path != have.path) {
        argv << QStringLiteral("-f") << QStringLiteral("source[branch]=%1").arg(want.branch) << QStringLiteral("-f")
             << QStringLiteral("source[path]=%1").arg(want.path);
        changed = true;
    }
    if (want.cname != have.cname) {
        argv << QStringLiteral("-F")
             << (want.cname.trimmed().isEmpty() ? QStringLiteral("cname=null")
                                                : QStringLiteral("cname=%1").arg(want.cname.trimmed()));
        changed = true;
    }
    if (want.httpsKnown && have.httpsKnown && want.https != have.https) {
        argv << QStringLiteral("-F")
             << QStringLiteral("https_enforced=%1").arg(want.https ? QStringLiteral("true") : QStringLiteral("false"));
        changed = true;
    }
    if (!changed) {
        if (error)
            *error = QStringLiteral("GitHub Pages без изменений.");
        return {};
    }
    return argv;
}

QString githubWikiRemoteUrl(const QString &originUrl)
{
    const GithubRemote parsed = parseGithubRemote(originUrl);
    if (!parsed.ok)
        return {};
    const QString trimmed = originUrl.trimmed();
    if (trimmed.startsWith(QLatin1String("ssh://")))
        return QStringLiteral("ssh://git@%1/%2/%3.wiki.git").arg(parsed.host, parsed.owner, parsed.name);
    if (trimmed.startsWith(QLatin1String("git@")))
        return QStringLiteral("git@%1:%2/%3.wiki.git").arg(parsed.host, parsed.owner, parsed.name);
    return QStringLiteral("https://%1/%2/%3.wiki.git").arg(parsed.host, parsed.owner, parsed.name);
}

bool validWikiPageName(const QString &name)
{
    const QRegularExpression re(QStringLiteral("^[A-Za-z0-9][A-Za-z0-9_-]{0,80}$"));
    return re.match(name.trimmed()).hasMatch();
}

namespace {

GitRunResult failed(const QString &message)
{
    GitRunResult result;
    result.ok = false;
    result.exitCode = 1;
    result.error = message;
    return result;
}

GitRunResult runGit(const QString &cwd, const QStringList &args)
{
    GitRunner git(20000);
    return git.run(cwd, args);
}

bool ensureWikiCheckout(const QString &wikiUrl, const QString &cacheDir, QString *error)
{
    const GitRunResult remote = runGit(QDir::tempPath(), {QStringLiteral("ls-remote"), wikiUrl});
    if (!remote.ok) {
        if (error)
            *error = QStringLiteral("Wiki недоступна. Включите Wiki в настройках репозитория.\n%1")
                         .arg(QString::fromUtf8(remote.stderrBytes).trimmed());
        return false;
    }
    const bool empty = QString::fromUtf8(remote.stdoutBytes).trimmed().isEmpty();
    QDir().mkpath(QFileInfo(cacheDir).absolutePath());
    if (empty) {
        if (!QFileInfo::exists(cacheDir + QStringLiteral("/.git"))) {
            QDir(cacheDir).removeRecursively();
            if (!QDir().mkpath(cacheDir)) {
                if (error)
                    *error = QStringLiteral("Не удалось создать каталог wiki.");
                return false;
            }
            const GitRunResult init = runGit(cacheDir, {QStringLiteral("init"), QStringLiteral("-b"), QStringLiteral("master")});
            if (!init.ok) {
                if (error)
                    *error = QStringLiteral("Не удалось подготовить пустую wiki.");
                return false;
            }
            const GitRunResult add = runGit(cacheDir, {QStringLiteral("remote"), QStringLiteral("add"), QStringLiteral("origin"), wikiUrl});
            if (!add.ok) {
                if (error)
                    *error = QStringLiteral("Не удалось добавить адрес wiki.");
                return false;
            }
        }
        return true;
    }
    if (!QFileInfo::exists(cacheDir + QStringLiteral("/.git"))) {
        QDir(cacheDir).removeRecursively();
        const GitRunResult clone = runGit(QFileInfo(cacheDir).absolutePath(),
                                          {QStringLiteral("clone"), QStringLiteral("--depth"), QStringLiteral("1"), wikiUrl,
                                           QFileInfo(cacheDir).fileName()});
        if (!clone.ok) {
            if (error)
                *error = QStringLiteral("Не удалось клонировать wiki.\n%1").arg(QString::fromUtf8(clone.stderrBytes).trimmed());
            return false;
        }
        return true;
    }
    const GitRunResult pull = runGit(cacheDir, {QStringLiteral("pull"), QStringLiteral("--ff-only")});
    if (!pull.ok) {
        if (error)
            *error = QStringLiteral("Локальная wiki разошлась с GitHub. Отправка отменена.\n%1")
                         .arg(QString::fromUtf8(pull.stderrBytes).trimmed());
        return false;
    }
    return true;
}

} // namespace

GitRunResult prepareWikiCheckout(const QString &wikiUrl, const QString &cacheDir)
{
    QString error;
    if (!ensureWikiCheckout(wikiUrl, cacheDir, &error))
        return failed(error);
    GitRunResult ok;
    ok.ok = true;
    ok.exitCode = 0;
    return ok;
}

QString githubWikiCacheDir(const QString &slug)
{
    const QStringList parts = slug.split(QLatin1Char('/'));
    if (parts.size() != 2 || parts.at(0).contains(QLatin1String("..")) || parts.at(1).contains(QLatin1String("..")))
        return {};
    const QString base = QStandardPaths::writableLocation(QStandardPaths::CacheLocation);
    if (base.isEmpty())
        return {};
    return base + QStringLiteral("/wiki/") + parts.at(0) + QLatin1Char('/') + parts.at(1);
}

QStringList listWikiPages(const QString &cacheDir)
{
    QStringList names;
    const QStringList files = QDir(cacheDir).entryList({QStringLiteral("*.md")}, QDir::Files, QDir::Name);
    for (QString name : files) {
        name.chop(3);
        if (validWikiPageName(name))
            names.append(name);
    }
    return names;
}

QString readWikiPage(const QString &cacheDir, const QString &page)
{
    if (!validWikiPageName(page))
        return {};
    QFile file(cacheDir + QLatin1Char('/') + page.trimmed() + QStringLiteral(".md"));
    if (!file.open(QIODevice::ReadOnly))
        return {};
    return QString::fromUtf8(file.readAll());
}

GitRunResult publishWikiPage(const QString &authorRepo, const QString &wikiUrl, const QString &cacheDir,
                             const QString &page, const QString &body)
{
    if (!validWikiPageName(page))
        return failed(QStringLiteral("Имя страницы wiki: буквы, цифры, подчёркивание и дефис."));
    if (wikiUrl.trimmed().isEmpty() || cacheDir.trimmed().isEmpty())
        return failed(QStringLiteral("Нет адреса wiki."));
    if (body.size() > 1000000)
        return failed(QStringLiteral("Текст страницы длиннее 1 МБ."));
    const GitRunResult userName = runGit(authorRepo, {QStringLiteral("config"), QStringLiteral("user.name")});
    const GitRunResult userEmail = runGit(authorRepo, {QStringLiteral("config"), QStringLiteral("user.email")});
    const QString author = QString::fromUtf8(userName.stdoutBytes).trimmed();
    const QString email = QString::fromUtf8(userEmail.stdoutBytes).trimmed();
    if (!userName.ok || !userEmail.ok || author.isEmpty() || email.isEmpty())
        return failed(QStringLiteral("В репозитории не заданы user.name и user.email."));

    QString error;
    if (!ensureWikiCheckout(wikiUrl, cacheDir, &error))
        return failed(error);

    const QString fileName = page.trimmed() + QStringLiteral(".md");
    QFile file(cacheDir + QLatin1Char('/') + fileName);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return failed(QStringLiteral("Не удалось записать страницу wiki."));
    file.write(body.toUtf8());
    if (!body.endsWith(QLatin1Char('\n')))
        file.write("\n");
    file.close();

    const GitRunResult added = runGit(cacheDir, {QStringLiteral("add"), QStringLiteral("--"), fileName});
    if (!added.ok)
        return failed(QStringLiteral("Не удалось добавить страницу wiki в индекс."));
    const GitRunResult quiet = runGit(cacheDir, {QStringLiteral("diff"), QStringLiteral("--cached"), QStringLiteral("--quiet")});
    if (!quiet.ok) {
        const GitRunResult commit = runGit(cacheDir, {QStringLiteral("-c"), QStringLiteral("user.name=%1").arg(author),
                                                      QStringLiteral("-c"), QStringLiteral("user.email=%1").arg(email),
                                                      QStringLiteral("commit"), QStringLiteral("-m"),
                                                      QStringLiteral("Страница %1").arg(page.trimmed())});
        if (!commit.ok)
            return failed(QStringLiteral("Не удалось закоммитить страницу wiki.\n%1")
                              .arg(QString::fromUtf8(commit.stderrBytes).trimmed()));
    }
    const GitRunResult pushed = runGit(cacheDir, {QStringLiteral("push"), QStringLiteral("-u"), QStringLiteral("origin"),
                                                  QStringLiteral("HEAD")});
    if (!pushed.ok)
        return failed(QStringLiteral("Не удалось отправить wiki.\n%1").arg(QString::fromUtf8(pushed.stderrBytes).trimmed()));
    GitRunResult ok;
    ok.ok = true;
    ok.exitCode = 0;
    ok.stdoutBytes = QStringLiteral("Страница %1 отправлена в wiki.").arg(page.trimmed()).toUtf8();
    return ok;
}
