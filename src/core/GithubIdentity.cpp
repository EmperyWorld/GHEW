#include "GithubIdentity.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

namespace {

void setError(QString *error, const QString &text)
{
    if (error)
        *error = text;
}

void resetError(QString *error)
{
    if (error)
        error->clear();
}

bool loginOk(const QString &text)
{
    static const QRegularExpression re(QStringLiteral("^[A-Za-z0-9](?:[A-Za-z0-9-]{0,37}[A-Za-z0-9])?$"));
    return re.match(text).hasMatch();
}

bool numberOk(const QString &text)
{
    static const QRegularExpression re(QStringLiteral("^[1-9][0-9]{0,8}$"));
    return re.match(text).hasMatch();
}

bool plainOk(const QString &text, int maxLen)
{
    if (text.size() > maxLen || text.contains(QLatin1Char('\n')) || text.contains(QLatin1Char('\r')) || text.contains(QChar(0)))
        return false;
    return true;
}

QString requireOrg(const QString &org, QString *error)
{
    const QString name = org.trimmed();
    if (!loginOk(name)) {
        setError(error, QStringLiteral("Организация: логин GitHub без пробелов."));
        return {};
    }
    return name;
}

QString requireLogin(const QString &login, const QString &label, QString *error)
{
    const QString name = login.trimmed();
    if (!loginOk(name)) {
        setError(error, QStringLiteral("%1: логин GitHub без пробелов.").arg(label));
        return {};
    }
    return name;
}

bool emailOk(const QString &text)
{
    static const QRegularExpression re(QStringLiteral("^[A-Za-z0-9._%+-]+@[A-Za-z0-9.-]+\\.[A-Za-z]{2,}$"));
    return text.size() <= 254 && re.match(text).hasMatch();
}

bool httpsOk(const QString &text)
{
    static const QRegularExpression re(QStringLiteral("^https://[^\\s\"'`]{1,500}$"));
    return re.match(text).hasMatch();
}

QString twitterOf(const QString &text, QString *error)
{
    QString name = text.trimmed();
    if (name.startsWith(QLatin1Char('@')))
        name.remove(0, 1);
    if (name.isEmpty())
        return {};
    static const QRegularExpression re(QStringLiteral("^[A-Za-z0-9_]{1,15}$"));
    if (!re.match(name).hasMatch()) {
        setError(error, QStringLiteral("Twitter: до 15 букв, цифр и подчёркиваний, без адреса."));
        return {};
    }
    return name;
}

bool featureOk(const QString &text)
{
    return text == QLatin1String("enabled") || text == QLatin1String("disabled") || text == QLatin1String("not_set");
}

GithubHiddenCommand jsonCall(const QString &method, const QString &path, const QJsonObject &body)
{
    GithubHiddenCommand command;
    command.argv = {QStringLiteral("api"), QStringLiteral("--method"), method, path, QStringLiteral("--input"),
                    QStringLiteral("-")};
    command.stdinText = QString::fromUtf8(QJsonDocument(body).toJson(QJsonDocument::Compact));
    return command;
}

QStringList api(const QString &method, const QString &path)
{
    return {QStringLiteral("api"), QStringLiteral("--method"), method, path};
}

void putPlain(QJsonObject *body, const QString &key, const QString &value, int maxLen, QString *error, bool *bad)
{
    const QString text = value.trimmed();
    if (text.isEmpty())
        return;
    if (!plainOk(text, maxLen)) {
        setError(error, QStringLiteral("Поле «%1»: без перевода строки, до %2 символов.").arg(key).arg(maxLen));
        *bad = true;
        return;
    }
    body->insert(key, text);
}

bool sshPublic(const QString &line)
{
    if (line.contains(QLatin1Char('\n')) || line.contains(QLatin1Char('\r')) || line.size() > 2000 || line.size() < 20)
        return false;
    if (line.contains(QStringLiteral("PRIVATE KEY"), Qt::CaseInsensitive))
        return false;
    return line.startsWith(QLatin1String("ssh-ed25519 ")) || line.startsWith(QLatin1String("ssh-rsa "))
           || line.startsWith(QLatin1String("ecdsa-sha2-nistp256 "))
           || line.startsWith(QLatin1String("sk-ssh-ed25519@openssh.com "));
}

QJsonArray repoIdsOf(const QString &csv, QString *error, bool required)
{
    QJsonArray ids;
    const QStringList parts = csv.split(QLatin1Char(','), Qt::SkipEmptyParts);
    static const QRegularExpression re(QStringLiteral("^[1-9][0-9]{0,14}$"));
    for (const QString &part : parts) {
        const QString n = part.trimmed();
        if (!re.match(n).hasMatch()) {
            setError(error, QStringLiteral("Номера репозиториев: целые числа через запятую, не больше двадцати."));
            return {};
        }
        ids.append(static_cast<double>(n.toLongLong()));
    }
    if (required && ids.isEmpty()) {
        setError(error, QStringLiteral("Нужен хотя бы один номер репозитория. На все репозитории отсюда не назначается."));
        return {};
    }
    if (ids.size() > 20) {
        setError(error, QStringLiteral("Номеров репозиториев не больше двадцати."));
        return {};
    }
    return ids;
}

QString teamSlug(const QString &text, QString *error)
{
    const QString slug = text.trimmed();
    static const QRegularExpression re(QStringLiteral("^[A-Za-z0-9](?:[A-Za-z0-9-]{0,98}[A-Za-z0-9])?$"));
    if (!re.match(slug).hasMatch() || slug.contains(QStringLiteral("--"))) {
        setError(error, QStringLiteral("Slug команды: буквы, цифры и одиночные дефисы."));
        return {};
    }
    return slug;
}

} // namespace

GithubHiddenCommand githubProfileCommand(const QString &name, const QString &bio, const QString &blog, const QString &company,
                                         const QString &location, const QString &twitter, bool hireable, QString *error)
{
    resetError(error);
    bool bad = false;
    QJsonObject body;
    body.insert(QStringLiteral("hireable"), hireable);
    putPlain(&body, QStringLiteral("name"), name, 255, error, &bad);
    putPlain(&body, QStringLiteral("bio"), bio, 160, error, &bad);
    putPlain(&body, QStringLiteral("company"), company, 255, error, &bad);
    putPlain(&body, QStringLiteral("location"), location, 255, error, &bad);
    const QString site = blog.trimmed();
    if (!site.isEmpty()) {
        if (!httpsOk(site) && !site.startsWith(QLatin1String("http://"))) {
            setError(error, QStringLiteral("Сайт профиля: http или https без пробелов."));
            bad = true;
        } else if (site.size() > 255 || site.contains(QLatin1Char(' '))) {
            setError(error, QStringLiteral("Сайт профиля: http или https без пробелов."));
            bad = true;
        } else {
            body.insert(QStringLiteral("blog"), site);
        }
    }
    if (!twitter.trimmed().isEmpty()) {
        const QString handle = twitterOf(twitter, error);
        if (handle.isEmpty())
            bad = true;
        else
            body.insert(QStringLiteral("twitter_username"), handle);
    }
    if (bad)
        return {};
    return jsonCall(QStringLiteral("PATCH"), QStringLiteral("user"), body);
}

GithubHiddenCommand githubEmailCommand(const QString &email, const QString &action, QString *error)
{
    resetError(error);
    const QString act = action.trimmed();
    if (act == QLatin1String("public") || act == QLatin1String("private")) {
        QJsonObject body;
        body.insert(QStringLiteral("visibility"), act);
        return jsonCall(QStringLiteral("PATCH"), QStringLiteral("user/email/visibility"), body);
    }
    const QString address = email.trimmed();
    if (!emailOk(address)) {
        setError(error, QStringLiteral("Нужен один адрес почты."));
        return {};
    }
    QJsonObject body;
    body.insert(QStringLiteral("emails"), QJsonArray{address});
    if (act == QLatin1String("add"))
        return jsonCall(QStringLiteral("POST"), QStringLiteral("user/emails"), body);
    if (act == QLatin1String("delete"))
        return jsonCall(QStringLiteral("DELETE"), QStringLiteral("user/emails"), body);
    setError(error, QStringLiteral("Почта: add, delete, public или private."));
    return {};
}

QStringList githubEmailLines(const QByteArray &json, QString *error)
{
    resetError(error);
    QJsonParseError parseError;
    const QJsonDocument doc = QJsonDocument::fromJson(json, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isArray()) {
        setError(error, QStringLiteral("GitHub не вернул список адресов."));
        return {};
    }
    QStringList lines;
    const QJsonArray rows = doc.array();
    for (const QJsonValue &value : rows) {
        if (!value.isObject())
            continue;
        const QJsonObject obj = value.toObject();
        const QString address = obj.value(QStringLiteral("email")).toString().trimmed();
        if (address.isEmpty())
            continue;
        QStringList bits{address};
        if (obj.value(QStringLiteral("primary")).toBool())
            bits << QStringLiteral("основная");
        if (!obj.value(QStringLiteral("verified")).toBool())
            bits << QStringLiteral("не подтверждена");
        bits << (obj.value(QStringLiteral("visibility")).toString() == QLatin1String("public") ? QStringLiteral("видна всем")
                                                                                              : QStringLiteral("скрыта"));
        lines << bits.join(QStringLiteral(" · "));
    }
    return lines;
}

GithubHiddenCommand githubSocialCommand(const QString &url, bool remove, QString *error)
{
    resetError(error);
    const QString link = url.trimmed();
    if (!httpsOk(link)) {
        setError(error, QStringLiteral("Соцаккаунт: полный адрес https без пробелов."));
        return {};
    }
    QJsonObject body;
    body.insert(QStringLiteral("account_urls"), QJsonArray{link});
    return jsonCall(remove ? QStringLiteral("DELETE") : QStringLiteral("POST"), QStringLiteral("user/social_accounts"), body);
}

GithubHiddenCommand githubUserKeyCommand(const QString &kind, const QString &title, const QString &key, const QString &id,
                                         bool remove, QString *error)
{
    resetError(error);
    const QString sort = kind.trimmed();
    QString path;
    if (sort == QLatin1String("ssh"))
        path = QStringLiteral("user/keys");
    else if (sort == QLatin1String("signing"))
        path = QStringLiteral("user/ssh_signing_keys");
    else if (sort == QLatin1String("gpg"))
        path = QStringLiteral("user/gpg_keys");
    else {
        setError(error, QStringLiteral("Вид ключа: ssh, signing или gpg."));
        return {};
    }
    const QString n = id.trimmed();
    if (remove) {
        if (!numberOk(n)) {
            setError(error, QStringLiteral("Для удаления ключа нужен его номер."));
            return {};
        }
        GithubHiddenCommand command;
        command.argv = api(QStringLiteral("DELETE"), path + QLatin1Char('/') + n);
        return command;
    }
    if (key.contains(QStringLiteral("PRIVATE KEY"), Qt::CaseInsensitive)) {
        setError(error, QStringLiteral("Закрытый ключ сюда не принимается."));
        return {};
    }
    if (!plainOk(title.trimmed(), 80) || title.trimmed().isEmpty() || title.trimmed().startsWith(QLatin1Char('-'))) {
        setError(error, QStringLiteral("Название ключа: без перевода строки, не с дефиса, до 80 символов."));
        return {};
    }
    QJsonObject body;
    if (sort == QLatin1String("gpg")) {
        const QString block = key.trimmed();
        if (block.size() > 20000 || !block.contains(QStringLiteral("BEGIN PGP PUBLIC KEY BLOCK")) || block.contains(QChar(0))) {
            setError(error, QStringLiteral("Нужен открытый блок PGP, не длиннее 20000 символов."));
            return {};
        }
        body.insert(QStringLiteral("armored_public_key"), block);
        body.insert(QStringLiteral("name"), title.trimmed());
    } else {
        const QString line = key.trimmed();
        if (!sshPublic(line)) {
            setError(error, QStringLiteral("Нужен однострочный открытый ключ ssh-ed25519, ssh-rsa, ecdsa-sha2-nistp256 или sk-ssh-ed25519."));
            return {};
        }
        body.insert(QStringLiteral("title"), title.trimmed());
        body.insert(QStringLiteral("key"), line);
    }
    return jsonCall(QStringLiteral("POST"), path, body);
}

QStringList githubProfileRepoArgv(const QString &login, QString *error)
{
    resetError(error);
    const QString name = requireLogin(login, QStringLiteral("Логин профиля"), error);
    if (name.isEmpty())
        return {};
    return {QStringLiteral("repo"), QStringLiteral("create"), name, QStringLiteral("--public"), QStringLiteral("--add-readme"),
            QStringLiteral("--description"), QStringLiteral("Profile README")};
}

GithubHiddenCommand githubProfileReadmeCommand(const QString &login, const QString &text, const QString &sha, QString *error)
{
    resetError(error);
    const QString name = requireLogin(login, QStringLiteral("Логин профиля"), error);
    if (name.isEmpty())
        return {};
    if (text.trimmed().isEmpty() || text.size() > 48000 || text.contains(QChar(0))) {
        setError(error, QStringLiteral("Текст README пустой или длиннее 48000 символов."));
        return {};
    }
    const QString current = sha.trimmed();
    if (!current.isEmpty()) {
        static const QRegularExpression hex(QStringLiteral("^[0-9a-fA-F]{40}$"));
        if (!hex.match(current).hasMatch()) {
            setError(error, QStringLiteral("SHA README — 40 шестнадцатеричных знаков. Пустое поле создаёт файл."));
            return {};
        }
    }
    QJsonObject body;
    body.insert(QStringLiteral("message"), QStringLiteral("Обновить README профиля"));
    body.insert(QStringLiteral("content"), QString::fromLatin1(text.toUtf8().toBase64()));
    if (!current.isEmpty())
        body.insert(QStringLiteral("sha"), current);
    return jsonCall(QStringLiteral("PUT"), QStringLiteral("repos/%1/%1/contents/README.md").arg(name), body);
}

GithubHiddenCommand githubOrgCardCommand(const QString &org, const QString &name, const QString &description, const QString &blog,
                                         const QString &email, const QString &twitter, const QString &location,
                                         const QString &company, QString *error)
{
    resetError(error);
    const QString owner = requireOrg(org, error);
    if (owner.isEmpty())
        return {};
    bool bad = false;
    QJsonObject body;
    putPlain(&body, QStringLiteral("name"), name, 255, error, &bad);
    putPlain(&body, QStringLiteral("description"), description, 160, error, &bad);
    putPlain(&body, QStringLiteral("company"), company, 255, error, &bad);
    putPlain(&body, QStringLiteral("location"), location, 255, error, &bad);
    const QString site = blog.trimmed();
    if (!site.isEmpty()) {
        if ((!httpsOk(site) && !site.startsWith(QLatin1String("http://"))) || site.size() > 255 || site.contains(QLatin1Char(' '))) {
            setError(error, QStringLiteral("Сайт организации: http или https без пробелов."));
            bad = true;
        } else {
            body.insert(QStringLiteral("blog"), site);
        }
    }
    const QString mail = email.trimmed();
    if (!mail.isEmpty()) {
        if (!emailOk(mail)) {
            setError(error, QStringLiteral("Публичная почта организации не похожа на адрес."));
            bad = true;
        } else {
            body.insert(QStringLiteral("email"), mail);
        }
    }
    if (!twitter.trimmed().isEmpty()) {
        const QString handle = twitterOf(twitter, error);
        if (handle.isEmpty())
            bad = true;
        else
            body.insert(QStringLiteral("twitter_username"), handle);
    }
    if (bad)
        return {};
    if (body.isEmpty()) {
        setError(error, QStringLiteral("Заполните хотя бы одно поле карточки. Пустые поля уже записанное не затирают."));
        return {};
    }
    return jsonCall(QStringLiteral("PATCH"), QStringLiteral("orgs/%1").arg(owner), body);
}

GithubHiddenCommand githubOrgPolicyCommand(const QString &org, const QString &permission, bool publicRepos, bool privateRepos,
                                           bool internalRepos, bool pages, bool publicPages, bool privatePages,
                                           bool forkPrivate, QString *error)
{
    resetError(error);
    const QString owner = requireOrg(org, error);
    if (owner.isEmpty())
        return {};
    const QString mode = permission.trimmed();
    if (mode != QLatin1String("read") && mode != QLatin1String("write") && mode != QLatin1String("admin")
        && mode != QLatin1String("none")) {
        setError(error, QStringLiteral("Право по умолчанию: read, write, admin или none."));
        return {};
    }
    QJsonObject body;
    body.insert(QStringLiteral("default_repository_permission"), mode);
    body.insert(QStringLiteral("members_can_create_public_repositories"), publicRepos);
    body.insert(QStringLiteral("members_can_create_private_repositories"), privateRepos);
    body.insert(QStringLiteral("members_can_create_internal_repositories"), internalRepos);
    body.insert(QStringLiteral("members_can_create_pages"), pages);
    body.insert(QStringLiteral("members_can_create_public_pages"), publicPages);
    body.insert(QStringLiteral("members_can_create_private_pages"), privatePages);
    body.insert(QStringLiteral("members_can_fork_private_repositories"), forkPrivate);
    return jsonCall(QStringLiteral("PATCH"), QStringLiteral("orgs/%1").arg(owner), body);
}

GithubHiddenCommand githubOrgTeamCommand(const QString &org, const QString &name, const QString &description,
                                         const QString &privacy, const QString &slug, bool remove, QString *error)
{
    resetError(error);
    const QString owner = requireOrg(org, error);
    if (owner.isEmpty())
        return {};
    if (remove) {
        const QString id = teamSlug(slug, error);
        if (id.isEmpty())
            return {};
        GithubHiddenCommand command;
        command.argv = api(QStringLiteral("DELETE"), QStringLiteral("orgs/%1/teams/%2").arg(owner, id));
        return command;
    }
    const QString title = name.trimmed();
    if (title.isEmpty() || !plainOk(title, 100) || title.contains(QLatin1Char('/')) || title.startsWith(QLatin1Char('-'))) {
        setError(error, QStringLiteral("Имя команды: без перевода строки и слэша, до 100 символов."));
        return {};
    }
    const QString about = description.trimmed();
    if (!plainOk(about, 255)) {
        setError(error, QStringLiteral("Описание команды: без перевода строки, до 255 символов."));
        return {};
    }
    const QString mode = privacy.trimmed();
    if (mode != QLatin1String("closed") && mode != QLatin1String("secret")) {
        setError(error, QStringLiteral("Видимость команды: closed или secret."));
        return {};
    }
    QJsonObject body;
    body.insert(QStringLiteral("name"), title);
    body.insert(QStringLiteral("privacy"), mode);
    if (!about.isEmpty())
        body.insert(QStringLiteral("description"), about);
    return jsonCall(QStringLiteral("POST"), QStringLiteral("orgs/%1/teams").arg(owner), body);
}

GithubHiddenCommand githubOrgTeamMemberCommand(const QString &org, const QString &slug, const QString &login,
                                               const QString &role, bool remove, QString *error)
{
    resetError(error);
    const QString owner = requireOrg(org, error);
    if (owner.isEmpty())
        return {};
    const QString id = teamSlug(slug, error);
    const QString user = requireLogin(login, QStringLiteral("Участник"), error);
    if (id.isEmpty() || user.isEmpty())
        return {};
    const QString path = QStringLiteral("orgs/%1/teams/%2/memberships/%3").arg(owner, id, user);
    if (remove) {
        GithubHiddenCommand command;
        command.argv = api(QStringLiteral("DELETE"), path);
        return command;
    }
    const QString mode = role.trimmed();
    if (mode != QLatin1String("member") && mode != QLatin1String("maintainer")) {
        setError(error, QStringLiteral("Роль в команде: member или maintainer."));
        return {};
    }
    QJsonObject body;
    body.insert(QStringLiteral("role"), mode);
    return jsonCall(QStringLiteral("PUT"), path, body);
}

GithubHiddenCommand githubOrgPeopleCommand(const QString &org, const QString &login, const QString &email,
                                           const QString &invitationId, const QString &role, const QString &action,
                                           QString *error)
{
    resetError(error);
    const QString owner = requireOrg(org, error);
    if (owner.isEmpty())
        return {};
    const QString act = action.trimmed();
    if (act == QLatin1String("invite")) {
        const QString address = email.trimmed();
        if (!emailOk(address)) {
            setError(error, QStringLiteral("Приглашение: нужен адрес почты."));
            return {};
        }
        const QString mode = role.trimmed().isEmpty() ? QStringLiteral("direct_member") : role.trimmed();
        if (mode != QLatin1String("direct_member") && mode != QLatin1String("admin")) {
            setError(error, QStringLiteral("Роль приглашения: direct_member или admin."));
            return {};
        }
        QJsonObject body;
        body.insert(QStringLiteral("email"), address);
        body.insert(QStringLiteral("role"), mode);
        return jsonCall(QStringLiteral("POST"), QStringLiteral("orgs/%1/invitations").arg(owner), body);
    }
    if (act == QLatin1String("cancel")) {
        const QString n = invitationId.trimmed();
        if (!numberOk(n)) {
            setError(error, QStringLiteral("Для отзыва приглашения нужен его номер."));
            return {};
        }
        GithubHiddenCommand command;
        command.argv = api(QStringLiteral("DELETE"), QStringLiteral("orgs/%1/invitations/%2").arg(owner, n));
        return command;
    }
    const QString user = requireLogin(login, QStringLiteral("Участник"), error);
    if (user.isEmpty())
        return {};
    if (act == QLatin1String("remove-member")) {
        GithubHiddenCommand command;
        command.argv = api(QStringLiteral("DELETE"), QStringLiteral("orgs/%1/members/%2").arg(owner, user));
        return command;
    }
    if (act == QLatin1String("remove-outside")) {
        GithubHiddenCommand command;
        command.argv = api(QStringLiteral("DELETE"), QStringLiteral("orgs/%1/outside_collaborators/%2").arg(owner, user));
        return command;
    }
    setError(error, QStringLiteral("Действие: invite, cancel, remove-member или remove-outside."));
    return {};
}

GithubHiddenCommand githubOrgRoleCommand(const QString &org, const QString &roleId, const QString &name,
                                         const QString &description, const QString &baseRole, const QString &permissions,
                                         bool remove, QString *error)
{
    resetError(error);
    const QString owner = requireOrg(org, error);
    if (owner.isEmpty())
        return {};
    const QString n = roleId.trimmed();
    if (remove) {
        if (!numberOk(n)) {
            setError(error, QStringLiteral("Для удаления роли нужен её номер."));
            return {};
        }
        GithubHiddenCommand command;
        command.argv = api(QStringLiteral("DELETE"), QStringLiteral("orgs/%1/custom-repository-roles/%2").arg(owner, n));
        return command;
    }
    const QString title = name.trimmed();
    if (title.isEmpty() || !plainOk(title, 100)) {
        setError(error, QStringLiteral("Имя роли: без перевода строки, до 100 символов."));
        return {};
    }
    const QString about = description.trimmed();
    if (!plainOk(about, 300)) {
        setError(error, QStringLiteral("Описание роли: без перевода строки, до 300 символов."));
        return {};
    }
    const QString base = baseRole.trimmed();
    if (base != QLatin1String("read") && base != QLatin1String("triage") && base != QLatin1String("write")
        && base != QLatin1String("maintain")) {
        setError(error, QStringLiteral("Базовая роль: read, triage, write или maintain."));
        return {};
    }
    const QStringList allowed{QStringLiteral("add_label"),     QStringLiteral("remove_label"),      QStringLiteral("add_assignee"),
                              QStringLiteral("close_issue"),   QStringLiteral("reopen_issue"),      QStringLiteral("close_pull_request"),
                              QStringLiteral("reopen_pull_request"), QStringLiteral("request_pr_review"),
                              QStringLiteral("set_milestone"), QStringLiteral("manage_wiki")};
    QJsonArray perms;
    const QStringList parts = permissions.split(QLatin1Char(','), Qt::SkipEmptyParts);
    for (const QString &part : parts) {
        const QString item = part.trimmed();
        if (!allowed.contains(item) || perms.contains(item)) {
            setError(error, QStringLiteral("Право роли из списка: add_label, remove_label, add_assignee, close_issue, manage_wiki и соседние."));
            return {};
        }
        perms.append(item);
    }
    if (perms.isEmpty()) {
        setError(error, QStringLiteral("Выберите хотя бы одно дополнительное право роли."));
        return {};
    }
    QJsonObject body;
    body.insert(QStringLiteral("name"), title);
    body.insert(QStringLiteral("base_role"), base);
    body.insert(QStringLiteral("permissions"), perms);
    if (!about.isEmpty())
        body.insert(QStringLiteral("description"), about);
    if (n.isEmpty())
        return jsonCall(QStringLiteral("POST"), QStringLiteral("orgs/%1/custom-repository-roles").arg(owner), body);
    if (!numberOk(n)) {
        setError(error, QStringLiteral("Номер роли — целое число без нуля впереди."));
        return {};
    }
    return jsonCall(QStringLiteral("PATCH"), QStringLiteral("orgs/%1/custom-repository-roles/%2").arg(owner, n), body);
}

GithubHiddenCommand githubOrgPropertyCommand(const QString &org, const QString &name, const QString &valueType,
                                             const QString &description, const QString &allowedCsv, bool remove,
                                             QString *error)
{
    resetError(error);
    const QString owner = requireOrg(org, error);
    if (owner.isEmpty())
        return {};
    static const QRegularExpression nameRe(QStringLiteral("^[a-z][a-z0-9_]{0,39}$"));
    const QString property = name.trimmed();
    if (!nameRe.match(property).hasMatch()) {
        setError(error, QStringLiteral("Имя свойства: строчные буквы, цифры и подчёркивание."));
        return {};
    }
    const QString path = QStringLiteral("orgs/%1/properties/schema/%2").arg(owner, property);
    if (remove) {
        GithubHiddenCommand command;
        command.argv = api(QStringLiteral("DELETE"), path);
        return command;
    }
    const QString kind = valueType.trimmed();
    if (kind != QLatin1String("string") && kind != QLatin1String("true_false") && kind != QLatin1String("url")
        && kind != QLatin1String("single_select")) {
        setError(error, QStringLiteral("Тип свойства: string, true_false, url или single_select."));
        return {};
    }
    const QString about = description.trimmed();
    if (!plainOk(about, 160)) {
        setError(error, QStringLiteral("Описание свойства: без перевода строки, до 160 символов."));
        return {};
    }
    QJsonArray allowed;
    if (kind == QLatin1String("single_select")) {
        static const QRegularExpression token(QStringLiteral("^[A-Za-z0-9][A-Za-z0-9 ._-]{0,40}$"));
        const QStringList parts = allowedCsv.split(QLatin1Char(','), Qt::SkipEmptyParts);
        for (const QString &part : parts) {
            const QString item = part.trimmed();
            if (!token.match(item).hasMatch()) {
                setError(error, QStringLiteral("Значения списка: через запятую, без пустых и слишком длинных."));
                return {};
            }
            allowed.append(item);
        }
        if (allowed.isEmpty() || allowed.size() > 20) {
            setError(error, QStringLiteral("Для списка нужно от 1 до 20 значений."));
            return {};
        }
    } else if (!allowedCsv.trimmed().isEmpty()) {
        setError(error, QStringLiteral("Список значений нужен только для single_select."));
        return {};
    }
    QJsonObject body;
    body.insert(QStringLiteral("value_type"), kind);
    body.insert(QStringLiteral("required"), false);
    body.insert(QStringLiteral("values_editable_by"), QStringLiteral("org_actors"));
    if (!about.isEmpty())
        body.insert(QStringLiteral("description"), about);
    if (!allowed.isEmpty())
        body.insert(QStringLiteral("allowed_values"), allowed);
    return jsonCall(QStringLiteral("PUT"), path, body);
}

GithubHiddenCommand githubOrgHookCommand(const QString &org, const QString &id, const QString &url, const QString &events,
                                         const QString &secret, bool active, bool remove, QString *error)
{
    resetError(error);
    const QString owner = requireOrg(org, error);
    if (owner.isEmpty())
        return {};
    const QString n = id.trimmed();
    if (remove) {
        if (!numberOk(n)) {
            setError(error, QStringLiteral("Для удаления вебхука нужен его номер."));
            return {};
        }
        GithubHiddenCommand command;
        command.argv = api(QStringLiteral("DELETE"), QStringLiteral("orgs/%1/hooks/%2").arg(owner, n));
        return command;
    }
    const QString hookUrl = url.trimmed();
    if (!httpsOk(hookUrl)) {
        setError(error, QStringLiteral("Адрес вебхука: https без пробелов."));
        return {};
    }
    if (secret.size() > 512 || secret.contains(QChar(0))) {
        setError(error, QStringLiteral("Секрет вебхука длиннее 512 символов."));
        return {};
    }
    const QStringList allowed{QStringLiteral("push"), QStringLiteral("pull_request"), QStringLiteral("issues"),
                              QStringLiteral("release"), QStringLiteral("workflow_run")};
    QJsonArray eventList;
    const QStringList parts = events.split(QLatin1Char(','), Qt::SkipEmptyParts);
    for (const QString &part : parts) {
        const QString event = part.trimmed();
        if (!allowed.contains(event)) {
            setError(error, QStringLiteral("События вебхука: push, pull_request, issues, release, workflow_run."));
            return {};
        }
        eventList.append(event);
    }
    if (eventList.isEmpty()) {
        setError(error, QStringLiteral("Выберите хотя бы одно событие вебхука."));
        return {};
    }
    QJsonObject config;
    config.insert(QStringLiteral("url"), hookUrl);
    config.insert(QStringLiteral("content_type"), QStringLiteral("json"));
    config.insert(QStringLiteral("insecure_ssl"), QStringLiteral("0"));
    if (!secret.isEmpty())
        config.insert(QStringLiteral("secret"), secret);
    QJsonObject body;
    body.insert(QStringLiteral("name"), QStringLiteral("web"));
    body.insert(QStringLiteral("active"), active);
    body.insert(QStringLiteral("events"), eventList);
    body.insert(QStringLiteral("config"), config);
    QString path = QStringLiteral("orgs/%1/hooks").arg(owner);
    QString method = QStringLiteral("POST");
    if (!n.isEmpty()) {
        if (!numberOk(n)) {
            setError(error, QStringLiteral("Номер вебхука — целое число без нуля впереди."));
            return {};
        }
        path += QLatin1Char('/') + n;
        method = QStringLiteral("PATCH");
    }
    return jsonCall(method, path, body);
}

GithubHiddenCommand githubOrgBannerCommand(const QString &org, const QString &text, const QString &expiresAt,
                                           bool dismissible, bool remove, QString *error)
{
    resetError(error);
    const QString owner = requireOrg(org, error);
    if (owner.isEmpty())
        return {};
    if (remove) {
        GithubHiddenCommand command;
        command.argv = api(QStringLiteral("DELETE"), QStringLiteral("orgs/%1/announcement").arg(owner));
        return command;
    }
    const QString banner = text.trimmed();
    if (banner.isEmpty() || banner.size() > 1024 || banner.contains(QChar(0))) {
        setError(error, QStringLiteral("Текст баннера пустой или длиннее 1024 символов."));
        return {};
    }
    const QString when = expiresAt.trimmed();
    if (!when.isEmpty()) {
        static const QRegularExpression stamp(QStringLiteral("^[0-9]{4}-[0-9]{2}-[0-9]{2}T[0-9]{2}:[0-9]{2}:[0-9]{2}Z$"));
        if (!stamp.match(when).hasMatch()) {
            setError(error, QStringLiteral("Срок баннера: YYYY-MM-DDTHH:MM:SSZ. Пустое поле — без срока."));
            return {};
        }
    }
    QJsonObject body;
    body.insert(QStringLiteral("announcement"), banner);
    body.insert(QStringLiteral("user_dismissible"), dismissible);
    if (!when.isEmpty())
        body.insert(QStringLiteral("expires_at"), when);
    return jsonCall(QStringLiteral("PATCH"), QStringLiteral("orgs/%1/announcement").arg(owner), body);
}

GithubHiddenCommand githubOrgSecurityConfigCommand(const QString &org, const QString &name, const QString &dependabot,
                                                   const QString &secretScanning, const QString &pushProtection,
                                                   const QString &configId, bool remove, QString *error)
{
    resetError(error);
    const QString owner = requireOrg(org, error);
    if (owner.isEmpty())
        return {};
    const QString n = configId.trimmed();
    if (remove) {
        if (!numberOk(n)) {
            setError(error, QStringLiteral("Для удаления конфигурации нужен её номер."));
            return {};
        }
        GithubHiddenCommand command;
        command.argv = api(QStringLiteral("DELETE"), QStringLiteral("orgs/%1/code-security/configurations/%2").arg(owner, n));
        return command;
    }
    const QString title = name.trimmed();
    if (title.isEmpty() || !plainOk(title, 100)) {
        setError(error, QStringLiteral("Имя конфигурации: без перевода строки, до 100 символов."));
        return {};
    }
    const QString alerts = dependabot.trimmed();
    const QString secrets = secretScanning.trimmed();
    const QString push = pushProtection.trimmed();
    if (!featureOk(alerts) || !featureOk(secrets) || !featureOk(push)) {
        setError(error, QStringLiteral("Состояние функции: enabled, disabled или not_set."));
        return {};
    }
    QJsonObject body;
    body.insert(QStringLiteral("name"), title);
    body.insert(QStringLiteral("dependency_graph"), QStringLiteral("not_set"));
    body.insert(QStringLiteral("dependabot_alerts"), alerts);
    body.insert(QStringLiteral("secret_scanning"), secrets);
    body.insert(QStringLiteral("secret_scanning_push_protection"), push);
    return jsonCall(QStringLiteral("POST"), QStringLiteral("orgs/%1/code-security/configurations").arg(owner), body);
}

QStringList githubOrgSecurityManagerArgv(const QString &org, const QString &teamSlugText, bool remove, QString *error)
{
    resetError(error);
    const QString owner = requireOrg(org, error);
    if (owner.isEmpty())
        return {};
    const QString id = teamSlug(teamSlugText, error);
    if (id.isEmpty())
        return {};
    return api(remove ? QStringLiteral("DELETE") : QStringLiteral("PUT"),
               QStringLiteral("orgs/%1/security-managers/teams/%2").arg(owner, id));
}

GithubHiddenCommand githubOrgImmutableCommand(const QString &org, const QString &policy, const QString &repoIds, QString *error)
{
    resetError(error);
    const QString owner = requireOrg(org, error);
    if (owner.isEmpty())
        return {};
    const QString mode = policy.trimmed();
    if (mode != QLatin1String("all") && mode != QLatin1String("none") && mode != QLatin1String("selected")) {
        setError(error, QStringLiteral("Immutable releases: all, none или selected."));
        return {};
    }
    QJsonObject body;
    body.insert(QStringLiteral("enforced_repositories"), mode);
    if (mode == QLatin1String("selected")) {
        const QJsonArray ids = repoIdsOf(repoIds, error, true);
        if (ids.isEmpty())
            return {};
        body.insert(QStringLiteral("selected_repository_ids"), ids);
    } else if (!repoIds.trimmed().isEmpty()) {
        setError(error, QStringLiteral("Список номеров нужен только для selected."));
        return {};
    }
    return jsonCall(QStringLiteral("PUT"), QStringLiteral("orgs/%1/settings/immutable-releases").arg(owner), body);
}

QStringList githubOrgRoleAssignArgv(const QString &org, const QString &login, const QString &roleId, bool remove,
                                    QString *error)
{
    resetError(error);
    const QString owner = requireOrg(org, error);
    const QString user = requireLogin(login, QStringLiteral("Пользователь"), error);
    if (owner.isEmpty() || user.isEmpty())
        return {};
    const QString n = roleId.trimmed();
    if (!numberOk(n)) {
        setError(error, QStringLiteral("Номер роли организации — целое число без нуля впереди."));
        return {};
    }
    return api(remove ? QStringLiteral("DELETE") : QStringLiteral("PUT"),
               QStringLiteral("orgs/%1/organization-roles/users/%2/%3").arg(owner, user, n));
}

GithubHiddenCommand githubOrgSecurityAttachCommand(const QString &org, const QString &configId, const QString &repoIds,
                                                   QString *error)
{
    resetError(error);
    const QString owner = requireOrg(org, error);
    if (owner.isEmpty())
        return {};
    const QString n = configId.trimmed();
    if (!numberOk(n)) {
        setError(error, QStringLiteral("Для назначения нужен номер конфигурации."));
        return {};
    }
    const QJsonArray ids = repoIdsOf(repoIds, error, true);
    if (ids.isEmpty())
        return {};
    QJsonObject body;
    body.insert(QStringLiteral("scope"), QStringLiteral("selected"));
    body.insert(QStringLiteral("selected_repository_ids"), ids);
    return jsonCall(QStringLiteral("POST"),
                    QStringLiteral("orgs/%1/code-security/configurations/%2/attach").arg(owner, n), body);
}
