#include "GithubAccess.h"

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

bool numberOk(const QString &text)
{
    static const QRegularExpression re(QStringLiteral("^[1-9][0-9]{0,8}$"));
    return re.match(text.trimmed()).hasMatch();
}

bool loginOk(const QString &text)
{
    static const QRegularExpression re(QStringLiteral("^[A-Za-z0-9](?:[A-Za-z0-9-]{0,37}[A-Za-z0-9])?$"));
    return re.match(text).hasMatch();
}

bool branchOk(const QString &text)
{
    static const QRegularExpression re(QStringLiteral("^[A-Za-z0-9][A-Za-z0-9._/-]{0,119}$"));
    return re.match(text).hasMatch() && !text.contains(QStringLiteral("..")) && !text.contains(QStringLiteral("//"));
}

bool titleOk(const QString &text, int maxLen)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty() || trimmed.size() > maxLen)
        return false;
    if (trimmed.contains(QLatin1Char('\n')) || trimmed.contains(QLatin1Char('\r')) || trimmed.contains(QChar(0)))
        return false;
    return !trimmed.startsWith(QLatin1Char('-'));
}

bool envNameOk(const QString &text)
{
    static const QRegularExpression re(QStringLiteral("^[A-Za-z0-9][A-Za-z0-9._-]{0,63}$"));
    return re.match(text).hasMatch();
}

bool secretNameOk(const QString &text)
{
    static const QRegularExpression re(QStringLiteral("^[A-Za-z_][A-Za-z0-9_]{0,80}$"));
    return re.match(text.trimmed()).hasMatch();
}

QString requireNumber(const QString &number, QString *error)
{
    const QString n = number.trimmed();
    if (!numberOk(n)) {
        setError(error, QStringLiteral("Номер — целое число без нуля впереди."));
        return {};
    }
    return n;
}

QStringList api(const QString &method, const QString &path)
{
    return {QStringLiteral("api"), QStringLiteral("--method"), method, path};
}

} // namespace

QStringList githubCollaboratorArgv(const QString &login, const QString &permission, bool remove, QString *error)
{
    resetError(error);
    const QString user = login.trimmed();
    if (!loginOk(user)) {
        setError(error, QStringLiteral("Соавтор: логин GitHub без пробелов."));
        return {};
    }
    const QString path = QStringLiteral("repos/{owner}/{repo}/collaborators/%1").arg(user);
    if (remove)
        return api(QStringLiteral("DELETE"), path);
    const QString mode = permission.trimmed();
    if (mode != QLatin1String("pull") && mode != QLatin1String("triage") && mode != QLatin1String("push")
        && mode != QLatin1String("maintain") && mode != QLatin1String("admin")) {
        setError(error, QStringLiteral("Право соавтора: pull, triage, push, maintain или admin."));
        return {};
    }
    QStringList argv = api(QStringLiteral("PUT"), path);
    argv << QStringLiteral("-f") << QStringLiteral("permission=%1").arg(mode);
    return argv;
}

QStringList githubInvitationDeleteArgv(const QString &id, QString *error)
{
    resetError(error);
    const QString n = requireNumber(id, error);
    if (n.isEmpty())
        return {};
    return api(QStringLiteral("DELETE"), QStringLiteral("repos/{owner}/{repo}/invitations/%1").arg(n));
}

QStringList githubInvitationAcceptArgv(const QString &id, QString *error)
{
    resetError(error);
    const QString n = requireNumber(id, error);
    if (n.isEmpty())
        return {};
    return api(QStringLiteral("PATCH"), QStringLiteral("user/repository_invitations/%1").arg(n));
}

QStringList githubWorkflowArgv(const QString &action, const QString &workflow, const QString &ref, const QString &inputKey,
                               const QString &inputValue, QString *error)
{
    resetError(error);
    const QString act = action.trimmed();
    const QString name = workflow.trimmed();
    const bool numeric = numberOk(name);
    static const QRegularExpression fileRe(QStringLiteral("^[A-Za-z0-9][A-Za-z0-9._/-]{0,160}$"));
    const bool file = fileRe.match(name).hasMatch() && !name.contains(QStringLiteral(".."))
                      && !name.contains(QStringLiteral("//"))
                      && (name.endsWith(QLatin1String(".yml")) || name.endsWith(QLatin1String(".yaml")));
    if (!numeric && !file) {
        setError(error, QStringLiteral("Workflow: номер или имя файла .yml или .yaml, без .."));
        return {};
    }
    if (act == QLatin1String("enable") || act == QLatin1String("disable"))
        return {QStringLiteral("workflow"), act, name};
    if (act != QLatin1String("run")) {
        setError(error, QStringLiteral("Действие workflow: enable, disable или run."));
        return {};
    }
    const QString at = ref.trimmed();
    if (!at.isEmpty() && !branchOk(at)) {
        setError(error, QStringLiteral("Ветка запуска: буквы, цифры, точка, подчёркивание, дефис и слэш."));
        return {};
    }
    const QString key = inputKey.trimmed();
    const QString value = inputValue;
    if (key.isEmpty() != value.isEmpty()) {
        setError(error, QStringLiteral("Поле запуска: укажите и имя, и значение, либо оставьте оба пустыми."));
        return {};
    }
    static const QRegularExpression keyRe(QStringLiteral("^[A-Za-z_][A-Za-z0-9_-]{0,40}$"));
    if (!key.isEmpty() && (!keyRe.match(key).hasMatch() || value.contains(QLatin1Char('\n')) || value.contains(QLatin1Char('\r'))
                          || value.size() > 200)) {
        setError(error, QStringLiteral("Поле запуска: имя из букв и цифр, значение до 200 символов без перевода строки."));
        return {};
    }
    QStringList argv{QStringLiteral("workflow"), QStringLiteral("run"), name};
    if (!at.isEmpty())
        argv << QStringLiteral("--ref") << at;
    if (!key.isEmpty())
        argv << QStringLiteral("-f") << QStringLiteral("%1=%2").arg(key, value);
    return argv;
}

QStringList githubRunArgv(const QString &action, const QString &runId, bool failedOnly, QString *error)
{
    resetError(error);
    const QString n = requireNumber(runId, error);
    if (n.isEmpty())
        return {};
    const QString act = action.trimmed();
    if (act == QLatin1String("cancel"))
        return {QStringLiteral("run"), QStringLiteral("cancel"), n};
    if (act == QLatin1String("delete"))
        return {QStringLiteral("run"), QStringLiteral("delete"), n};
    if (act == QLatin1String("rerun")) {
        QStringList argv{QStringLiteral("run"), QStringLiteral("rerun"), n};
        if (failedOnly)
            argv << QStringLiteral("--failed");
        return argv;
    }
    setError(error, QStringLiteral("Действие запуска: cancel, rerun или delete."));
    return {};
}

QStringList githubVariableArgv(const QString &name, const QString &value, const QString &envName, bool remove,
                               QString *error)
{
    resetError(error);
    if (!secretNameOk(name)) {
        setError(error, QStringLiteral("Имя переменной: буква или подчёркивание, затем буквы, цифры и подчёркивание."));
        return {};
    }
    const QString env = envName.trimmed();
    if (!env.isEmpty() && !envNameOk(env)) {
        setError(error, QStringLiteral("Имя окружения: буквы, цифры, точка, подчёркивание и дефис."));
        return {};
    }
    if (remove) {
        QStringList argv{QStringLiteral("variable"), QStringLiteral("delete"), name.trimmed()};
        if (!env.isEmpty())
            argv << QStringLiteral("--env") << env;
        return argv;
    }
    if (value.isEmpty() || value.size() > 4000 || value.contains(QLatin1Char('\n')) || value.contains(QLatin1Char('\r'))
        || value.contains(QChar(0))) {
        setError(error, QStringLiteral("Значение переменной: до 4000 символов и без перевода строки."));
        return {};
    }
    QStringList argv{QStringLiteral("variable"), QStringLiteral("set"), name.trimmed(), QStringLiteral("--body"), value};
    if (!env.isEmpty())
        argv << QStringLiteral("--env") << env;
    return argv;
}

GithubHiddenCommand githubSecretCommand(const QString &name, const QString &value, const QString &envName,
                                        const QString &app, bool remove, QString *error)
{
    resetError(error);
    GithubHiddenCommand command;
    if (!secretNameOk(name)) {
        setError(error, QStringLiteral("Имя секрета: буква или подчёркивание, затем буквы, цифры и подчёркивание."));
        return command;
    }
    const QString env = envName.trimmed();
    if (!env.isEmpty() && !envNameOk(env)) {
        setError(error, QStringLiteral("Имя окружения: буквы, цифры, точка, подчёркивание и дефис."));
        return command;
    }
    const QString application = app.trimmed().isEmpty() ? QStringLiteral("actions") : app.trimmed();
    if (application != QLatin1String("actions") && application != QLatin1String("dependabot")
        && application != QLatin1String("codespaces")) {
        setError(error, QStringLiteral("Секрет: actions, dependabot или codespaces."));
        return command;
    }
    if (remove) {
        command.argv = {QStringLiteral("secret"), QStringLiteral("delete"), name.trimmed(), QStringLiteral("--app"),
                        application};
        if (!env.isEmpty())
            command.argv << QStringLiteral("--env") << env;
        return command;
    }
    if (value.isEmpty() || value.size() > 8192 || value.contains(QChar(0))) {
        setError(error, QStringLiteral("Значение секрета пустое или длиннее 8192 символов."));
        return command;
    }
    command.argv = {QStringLiteral("secret"), QStringLiteral("set"), name.trimmed(), QStringLiteral("--app"), application};
    if (!env.isEmpty())
        command.argv << QStringLiteral("--env") << env;
    command.stdinText = value;
    return command;
}

QStringList githubEnvironmentArgv(const QString &name, int waitMinutes, bool protectedBranches, const QString &reviewers,
                                  bool remove, QString *error)
{
    resetError(error);
    if (!envNameOk(name.trimmed())) {
        setError(error, QStringLiteral("Имя окружения: буквы, цифры, точка, подчёркивание и дефис."));
        return {};
    }
    const QString path = QStringLiteral("repos/{owner}/{repo}/environments/%1").arg(name.trimmed());
    if (remove)
        return api(QStringLiteral("DELETE"), path);
    if (waitMinutes < 0 || waitMinutes > 43200) {
        setError(error, QStringLiteral("Ожидание окружения: от 0 до 43200 минут."));
        return {};
    }
    QStringList argv = api(QStringLiteral("PUT"), path);
    argv << QStringLiteral("-F") << QStringLiteral("wait_timer=%1").arg(waitMinutes);
    if (protectedBranches) {
        argv << QStringLiteral("-F") << QStringLiteral("deployment_branch_policy[protected_branches]=true");
        argv << QStringLiteral("-F") << QStringLiteral("deployment_branch_policy[custom_branch_policies]=false");
    } else {
        argv << QStringLiteral("-F") << QStringLiteral("deployment_branch_policy=null");
    }
    const QStringList people = reviewers.split(QLatin1Char(','), Qt::SkipEmptyParts);
    if (people.size() > 6) {
        setError(error, QStringLiteral("Ревьюеров окружения не больше шести."));
        return {};
    }
    static const QRegularExpression reviewerRe(QStringLiteral("^(User|Team):([1-9][0-9]{0,14})$"));
    for (const QString &part : people) {
        const QRegularExpressionMatch match = reviewerRe.match(part.trimmed());
        if (!match.hasMatch()) {
            setError(error, QStringLiteral("Ревьюер: User:номер или Team:номер, через запятую."));
            return {};
        }
        argv << QStringLiteral("-f") << QStringLiteral("reviewers[][type]=%1").arg(match.captured(1));
        argv << QStringLiteral("-F") << QStringLiteral("reviewers[][id]=%1").arg(match.captured(2));
    }
    return argv;
}

QStringList githubCacheDeleteArgv(const QString &keyOrId, QString *error)
{
    resetError(error);
    const QString key = keyOrId.trimmed();
    if (key == QLatin1String("--all") || key == QLatin1String("*") || key.isEmpty()) {
        setError(error, QStringLiteral("Укажите id или ключ одного кэша. Удаление всех кэшей отсюда не отправляется."));
        return {};
    }
    static const QRegularExpression keyRe(QStringLiteral("^[A-Za-z0-9][A-Za-z0-9._/-]{0,120}$"));
    if (!numberOk(key) && (!keyRe.match(key).hasMatch() || key.contains(QStringLiteral("..")))) {
        setError(error, QStringLiteral("Ключ кэша: буквы, цифры, точка, подчёркивание, дефис и слэш."));
        return {};
    }
    return {QStringLiteral("cache"), QStringLiteral("delete"), key};
}

QStringList githubAlertArgv(const QString &kind, const QString &number, const QString &state, const QString &reason,
                            QString *error)
{
    resetError(error);
    const QString n = requireNumber(number, error);
    if (n.isEmpty())
        return {};
    const QString which = kind.trimmed();
    const QString mode = state.trimmed();
    const QString why = reason.trimmed();
    QString path;
    QString reasonField;
    QStringList allowed;
    QString openState;
    QString closedState;
    if (which == QLatin1String("dependabot")) {
        path = QStringLiteral("repos/{owner}/{repo}/dependabot/alerts/%1").arg(n);
        reasonField = QStringLiteral("dismissed_reason");
        allowed = {QStringLiteral("fix_started"), QStringLiteral("inaccurate"), QStringLiteral("no_bandwidth"),
                   QStringLiteral("not_used"), QStringLiteral("tolerable_risk")};
        openState = QStringLiteral("open");
        closedState = QStringLiteral("dismissed");
    } else if (which == QLatin1String("code")) {
        path = QStringLiteral("repos/{owner}/{repo}/code-scanning/alerts/%1").arg(n);
        reasonField = QStringLiteral("dismissed_reason");
        allowed = {QStringLiteral("false positive"), QStringLiteral("won't fix"), QStringLiteral("used in tests"),
                   QStringLiteral("mitigated")};
        openState = QStringLiteral("open");
        closedState = QStringLiteral("dismissed");
    } else if (which == QLatin1String("secret")) {
        path = QStringLiteral("repos/{owner}/{repo}/secret-scanning/alerts/%1").arg(n);
        reasonField = QStringLiteral("resolution");
        allowed = {QStringLiteral("false_positive"), QStringLiteral("wont_fix"), QStringLiteral("revoked"),
                   QStringLiteral("used_in_tests")};
        openState = QStringLiteral("open");
        closedState = QStringLiteral("resolved");
    } else {
        setError(error, QStringLiteral("Оповещение: dependabot, code или secret."));
        return {};
    }
    QStringList argv = api(QStringLiteral("PATCH"), path);
    if (mode == openState) {
        argv << QStringLiteral("-f") << QStringLiteral("state=%1").arg(openState);
        return argv;
    }
    if (mode != closedState || !allowed.contains(why)) {
        setError(error, QStringLiteral("Для закрытия оповещения выберите состояние и причину из списка."));
        return {};
    }
    argv << QStringLiteral("-f") << QStringLiteral("state=%1").arg(closedState);
    argv << QStringLiteral("-f") << QStringLiteral("%1=%2").arg(reasonField, why);
    return argv;
}

GithubHiddenCommand githubRulesetCommand(const QString &id, const QString &name, const QString &includeRef,
                                         const QString &enforcement, const QString &rulesCsv, QString *error)
{
    resetError(error);
    GithubHiddenCommand command;
    if (!titleOk(name, 80)) {
        setError(error, QStringLiteral("Имя ruleset: без перевода строки, не с дефиса, до 80 символов."));
        return command;
    }
    const QString mode = enforcement.trimmed();
    if (mode != QLatin1String("active") && mode != QLatin1String("disabled") && mode != QLatin1String("evaluate")) {
        setError(error, QStringLiteral("Режим ruleset: active, disabled или evaluate."));
        return command;
    }
    static const QRegularExpression refRe(QStringLiteral("^(~DEFAULT_BRANCH|~ALL|refs/heads/[A-Za-z0-9._/-]{1,120})$"));
    const QString ref = includeRef.trimmed();
    if (!refRe.match(ref).hasMatch() || ref.contains(QStringLiteral(".."))) {
        setError(error, QStringLiteral("Ветка ruleset: refs/heads/имя, ~DEFAULT_BRANCH или ~ALL."));
        return command;
    }
    const QStringList allowed{QStringLiteral("deletion"), QStringLiteral("non_fast_forward"),
                              QStringLiteral("required_linear_history")};
    QJsonArray rules;
    const QStringList parts = rulesCsv.split(QLatin1Char(','), Qt::SkipEmptyParts);
    for (const QString &part : parts) {
        const QString type = part.trimmed();
        if (!allowed.contains(type) || rules.size() >= 3) {
            setError(error, QStringLiteral("Правила ruleset: deletion, non_fast_forward и required_linear_history."));
            return command;
        }
        QJsonObject rule;
        rule.insert(QStringLiteral("type"), type);
        rules.append(rule);
    }
    if (rules.isEmpty()) {
        setError(error, QStringLiteral("Выберите хотя бы одно правило ruleset."));
        return command;
    }
    QJsonObject conditions;
    QJsonObject refName;
    refName.insert(QStringLiteral("include"), QJsonArray{ref});
    refName.insert(QStringLiteral("exclude"), QJsonArray{});
    conditions.insert(QStringLiteral("ref_name"), refName);
    QJsonObject body;
    body.insert(QStringLiteral("name"), name.trimmed());
    body.insert(QStringLiteral("target"), QStringLiteral("branch"));
    body.insert(QStringLiteral("enforcement"), mode);
    body.insert(QStringLiteral("conditions"), conditions);
    body.insert(QStringLiteral("rules"), rules);
    const QString n = id.trimmed();
    QString path = QStringLiteral("repos/{owner}/{repo}/rulesets");
    QString method = QStringLiteral("POST");
    if (!n.isEmpty()) {
        if (!numberOk(n)) {
            setError(error, QStringLiteral("Номер ruleset — целое число без нуля впереди."));
            return command;
        }
        path += QLatin1Char('/') + n;
        method = QStringLiteral("PUT");
    }
    command.argv = api(method, path);
    command.argv << QStringLiteral("--input") << QStringLiteral("-");
    command.stdinText = QString::fromUtf8(QJsonDocument(body).toJson(QJsonDocument::Compact));
    return command;
}

QStringList githubBranchProtectionArgv(const QString &branch, int reviews, bool enforceAdmins, bool blockForce,
                                       bool blockDelete, bool remove, QString *error)
{
    resetError(error);
    if (!branchOk(branch.trimmed())) {
        setError(error, QStringLiteral("Имя ветки защиты: буквы, цифры, точка, подчёркивание, дефис и слэш."));
        return {};
    }
    const QString path = QStringLiteral("repos/{owner}/{repo}/branches/%1/protection").arg(branch.trimmed());
    if (remove)
        return api(QStringLiteral("DELETE"), path);
    if (reviews < 0 || reviews > 6) {
        setError(error, QStringLiteral("Число ревью: от 0 до 6. Ноль снимает требование."));
        return {};
    }
    QStringList argv = api(QStringLiteral("PUT"), path);
    argv << QStringLiteral("-F") << QStringLiteral("required_status_checks=null");
    argv << QStringLiteral("-F") << QStringLiteral("enforce_admins=%1").arg(enforceAdmins ? QStringLiteral("true") : QStringLiteral("false"));
    argv << QStringLiteral("-F") << QStringLiteral("restrictions=null");
    argv << QStringLiteral("-F") << QStringLiteral("allow_force_pushes=%1").arg(blockForce ? QStringLiteral("false") : QStringLiteral("true"));
    argv << QStringLiteral("-F") << QStringLiteral("allow_deletions=%1").arg(blockDelete ? QStringLiteral("false") : QStringLiteral("true"));
    argv << QStringLiteral("-F") << QStringLiteral("required_linear_history=false");
    if (reviews == 0) {
        argv << QStringLiteral("-F") << QStringLiteral("required_pull_request_reviews=null");
    } else {
        argv << QStringLiteral("-F")
             << QStringLiteral("required_pull_request_reviews[required_approving_review_count]=%1").arg(reviews);
        argv << QStringLiteral("-F") << QStringLiteral("required_pull_request_reviews[dismiss_stale_reviews]=false");
        argv << QStringLiteral("-F") << QStringLiteral("required_pull_request_reviews[require_code_owner_reviews]=false");
    }
    return argv;
}

QStringList githubDeployKeyArgv(const QString &title, const QString &key, bool readOnly, QString *error)
{
    resetError(error);
    if (!titleOk(title, 80)) {
        setError(error, QStringLiteral("Название ключа: без перевода строки, не с дефиса, до 80 символов."));
        return {};
    }
    const QString line = key.trimmed();
    if (line.contains(QLatin1Char('\n')) || line.contains(QLatin1Char('\r')) || line.size() > 2000
        || line.contains(QStringLiteral("PRIVATE KEY"), Qt::CaseInsensitive)) {
        setError(error, QStringLiteral("Нужен однострочный открытый ключ. Закрытый ключ сюда не принимается."));
        return {};
    }
    const bool known = line.startsWith(QLatin1String("ssh-ed25519 ")) || line.startsWith(QLatin1String("ssh-rsa "))
                       || line.startsWith(QLatin1String("ecdsa-sha2-nistp256 "))
                       || line.startsWith(QLatin1String("sk-ssh-ed25519@openssh.com "));
    if (!known) {
        setError(error, QStringLiteral("Ключ должен начинаться с ssh-ed25519, ssh-rsa, ecdsa-sha2-nistp256 или sk-ssh-ed25519."));
        return {};
    }
    QStringList argv = api(QStringLiteral("POST"), QStringLiteral("repos/{owner}/{repo}/keys"));
    argv << QStringLiteral("-f") << QStringLiteral("title=%1").arg(title.trimmed());
    argv << QStringLiteral("-f") << QStringLiteral("key=%1").arg(line);
    argv << QStringLiteral("-F") << QStringLiteral("read_only=%1").arg(readOnly ? QStringLiteral("true") : QStringLiteral("false"));
    return argv;
}

QStringList githubDeployKeyDeleteArgv(const QString &id, QString *error)
{
    resetError(error);
    const QString n = requireNumber(id, error);
    if (n.isEmpty())
        return {};
    return api(QStringLiteral("DELETE"), QStringLiteral("repos/{owner}/{repo}/keys/%1").arg(n));
}

GithubHiddenCommand githubWebhookCommand(const QString &id, const QString &url, const QString &events,
                                         const QString &secret, bool active, bool remove, QString *error)
{
    resetError(error);
    GithubHiddenCommand command;
    const QString n = id.trimmed();
    if (remove) {
        if (!numberOk(n)) {
            setError(error, QStringLiteral("Для удаления вебхука нужен его номер."));
            return command;
        }
        command.argv = api(QStringLiteral("DELETE"), QStringLiteral("repos/{owner}/{repo}/hooks/%1").arg(n));
        return command;
    }
    const QString hookUrl = url.trimmed();
    static const QRegularExpression http(QStringLiteral("^https://[^\\s\"'`]{1,500}$"));
    if (!http.match(hookUrl).hasMatch()) {
        setError(error, QStringLiteral("Адрес вебхука: https без пробелов."));
        return command;
    }
    if (secret.size() > 512 || secret.contains(QChar(0))) {
        setError(error, QStringLiteral("Секрет вебхука длиннее 512 символов."));
        return command;
    }
    const QStringList allowed{QStringLiteral("push"), QStringLiteral("pull_request"), QStringLiteral("issues"),
                              QStringLiteral("release"), QStringLiteral("workflow_run")};
    QJsonArray eventList;
    const QStringList parts = events.split(QLatin1Char(','), Qt::SkipEmptyParts);
    for (const QString &part : parts) {
        const QString event = part.trimmed();
        if (!allowed.contains(event)) {
            setError(error, QStringLiteral("События вебхука: push, pull_request, issues, release, workflow_run."));
            return command;
        }
        eventList.append(event);
    }
    if (eventList.isEmpty()) {
        setError(error, QStringLiteral("Выберите хотя бы одно событие вебхука."));
        return command;
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
    QString path = QStringLiteral("repos/{owner}/{repo}/hooks");
    QString method = QStringLiteral("POST");
    if (!n.isEmpty()) {
        if (!numberOk(n)) {
            setError(error, QStringLiteral("Номер вебхука — целое число без нуля впереди."));
            return command;
        }
        path += QLatin1Char('/') + n;
        method = QStringLiteral("PATCH");
    }
    command.argv = api(method, path);
    command.argv << QStringLiteral("--input") << QStringLiteral("-");
    command.stdinText = QString::fromUtf8(QJsonDocument(body).toJson(QJsonDocument::Compact));
    return command;
}

QStringList githubAdvisoryArgv(const QString &ghsa, const QString &summary, const QString &description,
                               const QString &severity, const QString &ecosystem, const QString &packageName,
                               const QString &versionRange, QString *error)
{
    resetError(error);
    if (!titleOk(summary, 256)) {
        setError(error, QStringLiteral("Краткое описание совета: без перевода строки, не с дефиса, до 256 символов."));
        return {};
    }
    if (description.trimmed().isEmpty() || description.size() > 8000 || description.contains(QChar(0))) {
        setError(error, QStringLiteral("Текст совета пустой или длиннее 8000 символов."));
        return {};
    }
    const QString level = severity.trimmed();
    if (level != QLatin1String("low") && level != QLatin1String("medium") && level != QLatin1String("high")
        && level != QLatin1String("critical")) {
        setError(error, QStringLiteral("Серьёзность: low, medium, high или critical."));
        return {};
    }
    const QString id = ghsa.trimmed();
    static const QRegularExpression ghsaRe(QStringLiteral("^GHSA-[a-z0-9]{4}-[a-z0-9]{4}-[a-z0-9]{4}$"));
    if (!id.isEmpty()) {
        if (!ghsaRe.match(id).hasMatch()) {
            setError(error, QStringLiteral("Номер совета имеет вид GHSA-xxxx-xxxx-xxxx."));
            return {};
        }
        QStringList argv = api(QStringLiteral("PATCH"), QStringLiteral("repos/{owner}/{repo}/security-advisories/%1").arg(id));
        argv << QStringLiteral("-f") << QStringLiteral("summary=%1").arg(summary.trimmed());
        argv << QStringLiteral("-f") << QStringLiteral("description=%1").arg(description);
        argv << QStringLiteral("-f") << QStringLiteral("severity=%1").arg(level);
        return argv;
    }
    const QStringList ecosystems{QStringLiteral("npm"), QStringLiteral("pip"), QStringLiteral("maven"),
                                 QStringLiteral("nuget"), QStringLiteral("rubygems"), QStringLiteral("composer"),
                                 QStringLiteral("go"), QStringLiteral("rust"), QStringLiteral("pub"),
                                 QStringLiteral("docker"), QStringLiteral("swift"), QStringLiteral("actions")};
    if (!ecosystems.contains(ecosystem.trimmed()) || !titleOk(packageName, 100)) {
        setError(error, QStringLiteral("Для черновика нужны экосистема из списка и имя пакета."));
        return {};
    }
    const QString range = versionRange.trimmed();
    static const QRegularExpression rangeRe(QStringLiteral("^[A-Za-z0-9.<>=~^* ,-]{1,80}$"));
    if (!rangeRe.match(range).hasMatch() || range.startsWith(QLatin1Char('-'))) {
        setError(error, QStringLiteral("Диапазон версий: до 80 символов, например < 1.0.0."));
        return {};
    }
    QStringList argv = api(QStringLiteral("POST"), QStringLiteral("repos/{owner}/{repo}/security-advisories"));
    argv << QStringLiteral("-f") << QStringLiteral("summary=%1").arg(summary.trimmed());
    argv << QStringLiteral("-f") << QStringLiteral("description=%1").arg(description);
    argv << QStringLiteral("-f") << QStringLiteral("severity=%1").arg(level);
    argv << QStringLiteral("-f") << QStringLiteral("vulnerabilities[][package][ecosystem]=%1").arg(ecosystem.trimmed());
    argv << QStringLiteral("-f") << QStringLiteral("vulnerabilities[][package][name]=%1").arg(packageName.trimmed());
    argv << QStringLiteral("-f") << QStringLiteral("vulnerabilities[][vulnerable_version_range]=%1").arg(range);
    return argv;
}

QStringList githubInteractionArgv(const QString &limit, bool clear, QString *error)
{
    resetError(error);
    if (clear)
        return api(QStringLiteral("DELETE"), QStringLiteral("repos/{owner}/{repo}/interaction-limits"));
    const QString mode = limit.trimmed();
    if (mode != QLatin1String("existing_users") && mode != QLatin1String("contributors_only")
        && mode != QLatin1String("collaborators_only")) {
        setError(error, QStringLiteral("Ограничение комментариев: existing_users, contributors_only или collaborators_only."));
        return {};
    }
    QStringList argv = api(QStringLiteral("PUT"), QStringLiteral("repos/{owner}/{repo}/interaction-limits"));
    argv << QStringLiteral("-f") << QStringLiteral("limit=%1").arg(mode);
    return argv;
}

QStringList githubCodespaceCreateArgv(const QString &slug, const QString &branch, const QString &displayName,
                                      QString *error)
{
    resetError(error);
    static const QRegularExpression slugRe(QStringLiteral("^[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+$"));
    if (!slugRe.match(slug.trimmed()).hasMatch()) {
        setError(error, QStringLiteral("Не удалось определить owner/repo для codespace."));
        return {};
    }
    if (!branchOk(branch.trimmed())) {
        setError(error, QStringLiteral("Ветка codespace обязательна, чтобы gh не спрашивал её сам."));
        return {};
    }
    if (!titleOk(displayName, 48)) {
        setError(error, QStringLiteral("Имя codespace: без перевода строки, не с дефиса, до 48 символов."));
        return {};
    }
    return {QStringLiteral("codespace"), QStringLiteral("create"), QStringLiteral("--repo"), slug.trimmed(),
            QStringLiteral("--branch"), branch.trimmed(), QStringLiteral("--display-name"), displayName.trimmed(),
            QStringLiteral("--default-permissions")};
}

QStringList githubCodespaceEditArgv(const QString &name, const QString &displayName, QString *error)
{
    resetError(error);
    static const QRegularExpression nameRe(QStringLiteral("^[A-Za-z0-9][A-Za-z0-9-]{0,80}$"));
    if (!nameRe.match(name.trimmed()).hasMatch()) {
        setError(error, QStringLiteral("Имя существующего codespace: буквы, цифры и дефис."));
        return {};
    }
    if (!titleOk(displayName, 48)) {
        setError(error, QStringLiteral("Новое имя codespace: без перевода строки, не с дефиса, до 48 символов."));
        return {};
    }
    return {QStringLiteral("codespace"), QStringLiteral("edit"), QStringLiteral("--codespace"), name.trimmed(),
            QStringLiteral("--display-name"), displayName.trimmed()};
}

QStringList githubCodespaceDeleteArgv(const QString &name, QString *error)
{
    resetError(error);
    static const QRegularExpression nameRe(QStringLiteral("^[A-Za-z0-9][A-Za-z0-9-]{0,80}$"));
    if (!nameRe.match(name.trimmed()).hasMatch()) {
        setError(error, QStringLiteral("Имя codespace: буквы, цифры и дефис. Удаление всех codespace отсюда не отправляется."));
        return {};
    }
    return api(QStringLiteral("DELETE"), QStringLiteral("user/codespaces/%1").arg(name.trimmed()));
}
