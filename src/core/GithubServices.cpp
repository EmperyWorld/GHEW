#include "GithubServices.h"

#include <QFileInfo>
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

bool titleOk(const QString &text, int maxLen)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty() || trimmed.size() > maxLen)
        return false;
    if (trimmed.contains(QLatin1Char('\n')) || trimmed.contains(QLatin1Char('\r')) || trimmed.contains(QChar(0)))
        return false;
    return !trimmed.startsWith(QLatin1Char('-'));
}

bool bodyOk(const QString &text, int maxLen)
{
    return text.size() <= maxLen && !text.contains(QChar(0));
}

bool loginOk(const QString &text)
{
    if (text == QLatin1String("@me"))
        return true;
    static const QRegularExpression re(QStringLiteral("^[A-Za-z0-9](?:[A-Za-z0-9-]{0,37}[A-Za-z0-9])?$"));
    return re.match(text).hasMatch();
}

bool labelOk(const QString &text)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty() || trimmed.size() > 50)
        return false;
    if (trimmed.startsWith(QLatin1Char('-')))
        return false;
    return !trimmed.contains(QLatin1Char('\n')) && !trimmed.contains(QLatin1Char('\r'));
}

bool branchOk(const QString &text)
{
    static const QRegularExpression re(QStringLiteral("^[A-Za-z0-9][A-Za-z0-9._/-]{0,119}$"));
    return re.match(text).hasMatch() && !text.contains(QStringLiteral("..")) && !text.contains(QStringLiteral("//"));
}

bool tagOk(const QString &text)
{
    static const QRegularExpression re(QStringLiteral("^[A-Za-z0-9][A-Za-z0-9._-]{0,99}$"));
    return re.match(text.trimmed()).hasMatch();
}

bool ownerOk(const QString &text)
{
    return loginOk(text.trimmed());
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

QString requireOwner(const QString &owner, QString *error)
{
    const QString text = owner.trimmed();
    if (!ownerOk(text)) {
        setError(error, QStringLiteral("Владелец проекта: логин или @me."));
        return {};
    }
    return text;
}

QString colorOrEmpty(const QString &color, QString *error)
{
    const QString original = color.trimmed();
    if (original.isEmpty())
        return {};
    const QString text = original.startsWith(QLatin1Char('#')) ? original.mid(1) : original;
    static const QRegularExpression re(QStringLiteral("^[0-9A-Fa-f]{6}$"));
    if (!re.match(text).hasMatch()) {
        setError(error, QStringLiteral("Цвет метки: 6 шестнадцатеричных знаков, решётка необязательна."));
        return {};
    }
    return text;
}

} // namespace

QStringList githubIssueWriteArgv(const QString &number, const QString &title, const QString &body, QString *error)
{
    resetError(error);
    if (!titleOk(title, 256)) {
        setError(error, QStringLiteral("Заголовок issue: без перевода строки, не с дефиса, до 256 символов."));
        return {};
    }
    if (!bodyOk(body, 65536)) {
        setError(error, QStringLiteral("Текст issue слишком длинный."));
        return {};
    }
    const QString n = number.trimmed();
    if (n.isEmpty()) {
        return {QStringLiteral("issue"), QStringLiteral("create"), QStringLiteral("--title"), title.trimmed(),
                QStringLiteral("--body"), body};
    }
    if (!numberOk(n)) {
        setError(error, QStringLiteral("Номер issue — целое число без нуля впереди."));
        return {};
    }
    return {QStringLiteral("issue"), QStringLiteral("edit"), n, QStringLiteral("--title"), title.trimmed(),
            QStringLiteral("--body"), body};
}

QStringList githubIssueActionArgv(const QString &number, const QString &action, const QString &extra, QString *error)
{
    resetError(error);
    const QString n = requireNumber(number, error);
    if (n.isEmpty())
        return {};
    const QString act = action.trimmed();
    if (act == QLatin1String("close")) {
        QStringList argv{QStringLiteral("issue"), QStringLiteral("close"), n};
        const QString reason = extra.trimmed();
        if (reason.isEmpty())
            return argv;
        if (reason != QLatin1String("completed") && reason != QLatin1String("not planned")) {
            setError(error, QStringLiteral("Причина закрытия: completed или not planned."));
            return {};
        }
        argv << QStringLiteral("--reason") << reason;
        return argv;
    }
    if (act == QLatin1String("reopen"))
        return {QStringLiteral("issue"), QStringLiteral("reopen"), n};
    if (act == QLatin1String("pin"))
        return {QStringLiteral("issue"), QStringLiteral("pin"), n};
    if (act == QLatin1String("unpin"))
        return {QStringLiteral("issue"), QStringLiteral("unpin"), n};
    if (act == QLatin1String("unlock"))
        return {QStringLiteral("issue"), QStringLiteral("unlock"), n};
    if (act == QLatin1String("lock")) {
        QStringList argv{QStringLiteral("issue"), QStringLiteral("lock"), n};
        const QString reason = extra.trimmed();
        if (reason.isEmpty())
            return argv;
        if (reason != QLatin1String("off_topic") && reason != QLatin1String("resolved") && reason != QLatin1String("spam")
            && reason != QLatin1String("too_heated")) {
            setError(error, QStringLiteral("Причина блокировки: off_topic, resolved, spam или too_heated."));
            return {};
        }
        argv << QStringLiteral("--reason") << reason;
        return argv;
    }
    if (act == QLatin1String("comment")) {
        if (extra.trimmed().isEmpty() || !bodyOk(extra, 65536)) {
            setError(error, QStringLiteral("Комментарий пустой или слишком длинный."));
            return {};
        }
        return {QStringLiteral("issue"), QStringLiteral("comment"), n, QStringLiteral("--body"), extra};
    }
    if (act == QLatin1String("delete"))
        return {QStringLiteral("issue"), QStringLiteral("delete"), n, QStringLiteral("--yes")};
    setError(error, QStringLiteral("Неизвестное действие с issue."));
    return {};
}

QStringList githubIssueLinkArgv(const QString &number, const QString &kind, const QString &value, bool remove,
                                QString *error)
{
    resetError(error);
    const QString n = requireNumber(number, error);
    if (n.isEmpty())
        return {};
    const QString k = kind.trimmed();
    const QString v = value.trimmed();
    if (k == QLatin1String("parent") && remove)
        return {QStringLiteral("issue"), QStringLiteral("edit"), n, QStringLiteral("--remove-parent")};
    if (k == QLatin1String("milestone") && remove)
        return {QStringLiteral("issue"), QStringLiteral("edit"), n, QStringLiteral("--remove-milestone")};

    QString flag;
    if (k == QLatin1String("assignee")) {
        if (!loginOk(v)) {
            setError(error, QStringLiteral("Исполнитель: логин или @me."));
            return {};
        }
        flag = remove ? QStringLiteral("--remove-assignee") : QStringLiteral("--add-assignee");
    } else if (k == QLatin1String("label")) {
        if (!labelOk(v)) {
            setError(error, QStringLiteral("Имя метки: до 50 символов, без перевода строки и не с дефиса."));
            return {};
        }
        flag = remove ? QStringLiteral("--remove-label") : QStringLiteral("--add-label");
    } else if (k == QLatin1String("subissue") || k == QLatin1String("blocked") || k == QLatin1String("blocking")
               || k == QLatin1String("parent")) {
        if (!numberOk(v)) {
            setError(error, QStringLiteral("Нужен номер связанной задачи."));
            return {};
        }
        if (k == QLatin1String("subissue"))
            flag = remove ? QStringLiteral("--remove-sub-issue") : QStringLiteral("--add-sub-issue");
        else if (k == QLatin1String("blocked"))
            flag = remove ? QStringLiteral("--remove-blocked-by") : QStringLiteral("--add-blocked-by");
        else if (k == QLatin1String("blocking"))
            flag = remove ? QStringLiteral("--remove-blocking") : QStringLiteral("--add-blocking");
        else
            flag = QStringLiteral("--parent");
    } else if (k == QLatin1String("milestone")) {
        if (!titleOk(v, 200)) {
            setError(error, QStringLiteral("Имя вехи: без перевода строки, не с дефиса, до 200 символов."));
            return {};
        }
        flag = QStringLiteral("--milestone");
    } else {
        setError(error, QStringLiteral("Неизвестный вид связи issue."));
        return {};
    }
    return {QStringLiteral("issue"), QStringLiteral("edit"), n, flag, v};
}

QStringList githubLabelWriteArgv(const QString &name, const QString &color, const QString &description,
                                 const QString &newName, bool edit, QString *error)
{
    resetError(error);
    if (!labelOk(name)) {
        setError(error, QStringLiteral("Имя метки: до 50 символов, без перевода строки и не с дефиса."));
        return {};
    }
    const QString hex = colorOrEmpty(color, error);
    if (error && !error->isEmpty())
        return {};
    const QString about = description.trimmed();
    if (!about.isEmpty() && !titleOk(about, 200)) {
        setError(error, QStringLiteral("Описание метки: без перевода строки, не с дефиса, до 200 символов."));
        return {};
    }
    if (!edit) {
        QStringList argv{QStringLiteral("label"), QStringLiteral("create"), name.trimmed()};
        if (!hex.isEmpty())
            argv << QStringLiteral("--color") << hex;
        if (!about.isEmpty())
            argv << QStringLiteral("--description") << about;
        return argv;
    }
    const QString renamed = newName.trimmed();
    if (!renamed.isEmpty() && !labelOk(renamed)) {
        setError(error, QStringLiteral("Новое имя метки: до 50 символов, без перевода строки и не с дефиса."));
        return {};
    }
    if (renamed.isEmpty() && hex.isEmpty() && about.isEmpty()) {
        setError(error, QStringLiteral("Нечего менять у метки."));
        return {};
    }
    QStringList argv{QStringLiteral("label"), QStringLiteral("edit"), name.trimmed()};
    if (!renamed.isEmpty())
        argv << QStringLiteral("--name") << renamed;
    if (!hex.isEmpty())
        argv << QStringLiteral("--color") << hex;
    if (!about.isEmpty())
        argv << QStringLiteral("--description") << about;
    return argv;
}

QStringList githubLabelDeleteArgv(const QString &name, QString *error)
{
    resetError(error);
    if (!labelOk(name)) {
        setError(error, QStringLiteral("Имя метки: до 50 символов, без перевода строки и не с дефиса."));
        return {};
    }
    return {QStringLiteral("label"), QStringLiteral("delete"), name.trimmed(), QStringLiteral("--yes")};
}

QStringList githubMilestoneArgv(const QString &number, const QString &title, const QString &state, QString *error)
{
    resetError(error);
    const QString mode = state.trimmed();
    if (mode != QLatin1String("open") && mode != QLatin1String("closed") && mode != QLatin1String("delete")) {
        setError(error, QStringLiteral("Состояние вехи: open, closed или delete."));
        return {};
    }
    const QString n = number.trimmed();
    if (mode == QLatin1String("delete")) {
        if (!numberOk(n)) {
            setError(error, QStringLiteral("Для удаления вехи нужен её номер."));
            return {};
        }
        return {QStringLiteral("api"), QStringLiteral("--method"), QStringLiteral("DELETE"),
                QStringLiteral("repos/{owner}/{repo}/milestones/%1").arg(n)};
    }
    const QString heading = title.trimmed();
    if (!n.isEmpty()) {
        if (!numberOk(n)) {
            setError(error, QStringLiteral("Номер вехи — целое число без нуля впереди."));
            return {};
        }
        if (!heading.isEmpty() && !titleOk(heading, 200)) {
            setError(error, QStringLiteral("Название вехи: без перевода строки, не с дефиса, до 200 символов."));
            return {};
        }
        QStringList argv{QStringLiteral("api"), QStringLiteral("--method"), QStringLiteral("PATCH"),
                         QStringLiteral("repos/{owner}/{repo}/milestones/%1").arg(n),
                         QStringLiteral("-f"), QStringLiteral("state=%1").arg(mode)};
        if (!heading.isEmpty())
            argv << QStringLiteral("-f") << QStringLiteral("title=%1").arg(heading);
        return argv;
    }
    if (!titleOk(heading, 200)) {
        setError(error, QStringLiteral("Название вехи: без перевода строки, не с дефиса, до 200 символов."));
        return {};
    }
    return {QStringLiteral("api"), QStringLiteral("--method"), QStringLiteral("POST"),
            QStringLiteral("repos/{owner}/{repo}/milestones"), QStringLiteral("-f"),
            QStringLiteral("title=%1").arg(heading), QStringLiteral("-f"), QStringLiteral("state=%1").arg(mode)};
}

QStringList githubPullWriteArgv(const QString &number, const QString &title, const QString &body, const QString &base,
                                const QString &head, bool draft, QString *error)
{
    resetError(error);
    if (!titleOk(title, 256)) {
        setError(error, QStringLiteral("Заголовок pull request: без перевода строки, не с дефиса, до 256 символов."));
        return {};
    }
    if (!bodyOk(body, 65536)) {
        setError(error, QStringLiteral("Текст pull request слишком длинный."));
        return {};
    }
    const QString baseBranch = base.trimmed();
    if (!baseBranch.isEmpty() && !branchOk(baseBranch)) {
        setError(error, QStringLiteral("Базовая ветка: буквы, цифры, точка, подчёркивание, дефис и слэш."));
        return {};
    }
    const QString n = number.trimmed();
    if (n.isEmpty()) {
        const QString headBranch = head.trimmed();
        if (!branchOk(headBranch)) {
            setError(error, QStringLiteral("Ветка pull request обязательна: так gh не предложит push и форк."));
            return {};
        }
        QStringList argv{QStringLiteral("pr"), QStringLiteral("create"), QStringLiteral("--title"), title.trimmed(),
                         QStringLiteral("--body"), body, QStringLiteral("--head"), headBranch};
        if (!baseBranch.isEmpty())
            argv << QStringLiteral("--base") << baseBranch;
        if (draft)
            argv << QStringLiteral("--draft");
        return argv;
    }
    if (!numberOk(n)) {
        setError(error, QStringLiteral("Номер pull request — целое число без нуля впереди."));
        return {};
    }
    QStringList argv{QStringLiteral("pr"), QStringLiteral("edit"), n, QStringLiteral("--title"), title.trimmed(),
                     QStringLiteral("--body"), body};
    if (!baseBranch.isEmpty())
        argv << QStringLiteral("--base") << baseBranch;
    return argv;
}

QStringList githubPullReviewArgv(const QString &number, const QString &action, const QString &body, QString *error)
{
    resetError(error);
    const QString n = requireNumber(number, error);
    if (n.isEmpty())
        return {};
    const QString act = action.trimmed();
    QString flag;
    if (act == QLatin1String("approve"))
        flag = QStringLiteral("--approve");
    else if (act == QLatin1String("comment"))
        flag = QStringLiteral("--comment");
    else if (act == QLatin1String("request"))
        flag = QStringLiteral("--request-changes");
    else {
        setError(error, QStringLiteral("Ревью: approve, comment или request."));
        return {};
    }
    if (act != QLatin1String("approve") && body.trimmed().isEmpty()) {
        setError(error, QStringLiteral("Для замечания нужен текст ревью."));
        return {};
    }
    if (!bodyOk(body, 65536)) {
        setError(error, QStringLiteral("Текст ревью слишком длинный."));
        return {};
    }
    QStringList argv{QStringLiteral("pr"), QStringLiteral("review"), n, flag};
    if (!body.isEmpty())
        argv << QStringLiteral("--body") << body;
    return argv;
}

QStringList githubPullMergeArgv(const QString &number, const QString &method, QString *error)
{
    resetError(error);
    const QString n = requireNumber(number, error);
    if (n.isEmpty())
        return {};
    const QString mode = method.trimmed();
    QString flag;
    if (mode == QLatin1String("merge"))
        flag = QStringLiteral("--merge");
    else if (mode == QLatin1String("squash"))
        flag = QStringLiteral("--squash");
    else if (mode == QLatin1String("rebase"))
        flag = QStringLiteral("--rebase");
    else {
        setError(error, QStringLiteral("Слияние: merge, squash или rebase."));
        return {};
    }
    return {QStringLiteral("pr"), QStringLiteral("merge"), n, flag};
}

QStringList githubPullReadyArgv(const QString &number, bool draft, QString *error)
{
    resetError(error);
    const QString n = requireNumber(number, error);
    if (n.isEmpty())
        return {};
    QStringList argv{QStringLiteral("pr"), QStringLiteral("ready"), n};
    if (draft)
        argv << QStringLiteral("--undo");
    return argv;
}

QStringList githubPullUpdateArgv(const QString &number, bool rebase, QString *error)
{
    resetError(error);
    const QString n = requireNumber(number, error);
    if (n.isEmpty())
        return {};
    QStringList argv{QStringLiteral("pr"), QStringLiteral("update-branch"), n};
    if (rebase)
        argv << QStringLiteral("--rebase");
    return argv;
}

QStringList githubDiscussionWriteArgv(const QString &number, const QString &title, const QString &body,
                                      const QString &category, QString *error)
{
    resetError(error);
    if (!titleOk(title, 256)) {
        setError(error, QStringLiteral("Заголовок обсуждения: без перевода строки, не с дефиса, до 256 символов."));
        return {};
    }
    if (!bodyOk(body, 65536)) {
        setError(error, QStringLiteral("Текст обсуждения слишком длинный."));
        return {};
    }
    const QString n = number.trimmed();
    const QString cat = category.trimmed();
    if (n.isEmpty()) {
        if (!titleOk(cat, 80)) {
            setError(error, QStringLiteral("Категория обсуждения обязательна."));
            return {};
        }
        return {QStringLiteral("discussion"), QStringLiteral("create"), QStringLiteral("--title"), title.trimmed(),
                QStringLiteral("--body"), body, QStringLiteral("--category"), cat};
    }
    if (!numberOk(n)) {
        setError(error, QStringLiteral("Номер обсуждения — целое число без нуля впереди."));
        return {};
    }
    QStringList argv{QStringLiteral("discussion"), QStringLiteral("edit"), n, QStringLiteral("--title"), title.trimmed(),
                     QStringLiteral("--body"), body};
    if (!cat.isEmpty()) {
        if (!titleOk(cat, 80)) {
            setError(error, QStringLiteral("Категория обсуждения: без перевода строки и не с дефиса."));
            return {};
        }
        argv << QStringLiteral("--category") << cat;
    }
    return argv;
}

QStringList githubDiscussionCommentArgv(const QString &target, const QString &body, bool remove, QString *error)
{
    resetError(error);
    const QString id = target.trimmed();
    if (remove) {
        static const QRegularExpression node(QStringLiteral("^DC_[A-Za-z0-9]+$"));
        const bool url = id.startsWith(QStringLiteral("https://github.com/")) && id.contains(QStringLiteral("discussioncomment-"));
        if (!node.match(id).hasMatch() && !url) {
            setError(error, QStringLiteral("Удаление комментария: id вида DC_… или ссылка на комментарий."));
            return {};
        }
        return {QStringLiteral("discussion"), QStringLiteral("comment"), id, QStringLiteral("--delete"),
                QStringLiteral("--yes")};
    }
    if (!numberOk(id)) {
        setError(error, QStringLiteral("Номер обсуждения — целое число без нуля впереди."));
        return {};
    }
    if (body.trimmed().isEmpty() || !bodyOk(body, 65536)) {
        setError(error, QStringLiteral("Комментарий обсуждения пустой или слишком длинный."));
        return {};
    }
    return {QStringLiteral("discussion"), QStringLiteral("comment"), id, QStringLiteral("--body"), body};
}

QStringList githubReleaseCreateArgv(const QString &tag, const QString &title, const QString &notes, bool draft,
                                    bool prerelease, const QString &target, QString *error)
{
    resetError(error);
    if (!tagOk(tag)) {
        setError(error, QStringLiteral("Тег релиза: буквы, цифры, точка, подчёркивание и дефис."));
        return {};
    }
    if (!title.trimmed().isEmpty() && !titleOk(title, 256)) {
        setError(error, QStringLiteral("Заголовок релиза: без перевода строки, не с дефиса, до 256 символов."));
        return {};
    }
    if (!bodyOk(notes, 65536)) {
        setError(error, QStringLiteral("Текст релиза слишком длинный."));
        return {};
    }
    const QString at = target.trimmed();
    if (!at.isEmpty() && !branchOk(at)) {
        setError(error, QStringLiteral("Цель релиза: ветка или полный SHA."));
        return {};
    }
    QStringList argv{QStringLiteral("release"), QStringLiteral("create"), tag.trimmed(), QStringLiteral("--title"),
                     title.trimmed().isEmpty() ? tag.trimmed() : title.trimmed(), QStringLiteral("--notes"), notes};
    if (draft)
        argv << QStringLiteral("--draft");
    if (prerelease)
        argv << QStringLiteral("--prerelease");
    if (!at.isEmpty())
        argv << QStringLiteral("--target") << at;
    return argv;
}

QStringList githubReleaseEditArgv(const QString &tag, const QString &title, const QString &notes, bool draft,
                                  bool prerelease, const QString &target, QString *error)
{
    resetError(error);
    if (!tagOk(tag)) {
        setError(error, QStringLiteral("Тег релиза: буквы, цифры, точка, подчёркивание и дефис."));
        return {};
    }
    if (!title.trimmed().isEmpty() && !titleOk(title, 256)) {
        setError(error, QStringLiteral("Заголовок релиза: без перевода строки, не с дефиса, до 256 символов."));
        return {};
    }
    if (!bodyOk(notes, 65536)) {
        setError(error, QStringLiteral("Текст релиза слишком длинный."));
        return {};
    }
    const QString at = target.trimmed();
    if (!at.isEmpty() && !branchOk(at)) {
        setError(error, QStringLiteral("Цель релиза: ветка или полный SHA."));
        return {};
    }
    QStringList argv{QStringLiteral("release"), QStringLiteral("edit"), tag.trimmed(), QStringLiteral("--title"),
                     title.trimmed().isEmpty() ? tag.trimmed() : title.trimmed(), QStringLiteral("--notes"), notes,
                     QStringLiteral("--draft=%1").arg(draft ? QStringLiteral("true") : QStringLiteral("false")),
                     QStringLiteral("--prerelease=%1").arg(prerelease ? QStringLiteral("true") : QStringLiteral("false"))};
    if (!at.isEmpty())
        argv << QStringLiteral("--target") << at;
    return argv;
}

QStringList githubReleaseDeleteArgv(const QString &tag, bool cleanupTag, QString *error)
{
    resetError(error);
    if (!tagOk(tag)) {
        setError(error, QStringLiteral("Тег релиза: буквы, цифры, точка, подчёркивание и дефис."));
        return {};
    }
    QStringList argv{QStringLiteral("release"), QStringLiteral("delete"), tag.trimmed(), QStringLiteral("--yes")};
    if (cleanupTag)
        argv << QStringLiteral("--cleanup-tag");
    return argv;
}

QStringList githubReleaseUploadArgv(const QString &tag, const QString &filePath, QString *error)
{
    resetError(error);
    if (!tagOk(tag)) {
        setError(error, QStringLiteral("Тег релиза: буквы, цифры, точка, подчёркивание и дефис."));
        return {};
    }
    if (filePath.contains(QLatin1Char('\n')) || filePath.contains(QLatin1Char('\r'))) {
        setError(error, QStringLiteral("Путь к файлу релиза не должен содержать перевод строки."));
        return {};
    }
    const QFileInfo info(filePath);
    if (!info.exists() || !info.isFile() || !info.isReadable()) {
        setError(error, QStringLiteral("Файл релиза не найден или его нельзя прочитать."));
        return {};
    }
    constexpr qint64 kMax = 512LL * 1024LL * 1024LL;
    if (info.size() > kMax) {
        setError(error, QStringLiteral("Файл релиза больше 512 МБ."));
        return {};
    }
    const QString canonical = info.canonicalFilePath();
    if (canonical.isEmpty() || canonical.startsWith(QLatin1Char('-'))) {
        setError(error, QStringLiteral("Не удалось разобрать путь к файлу релиза."));
        return {};
    }
    return {QStringLiteral("release"), QStringLiteral("upload"), tag.trimmed(), canonical};
}

QStringList githubAutolinkCreateArgv(const QString &prefix, const QString &urlTemplate, bool numeric, QString *error)
{
    resetError(error);
    static const QRegularExpression key(QStringLiteral("^[A-Za-z][A-Za-z0-9-]{0,39}$"));
    const QString prefixText = prefix.trimmed();
    if (!key.match(prefixText).hasMatch()) {
        setError(error, QStringLiteral("Префикс автоссылки: буквы, цифры и дефис, до 40 символов."));
        return {};
    }
    const QString url = urlTemplate.trimmed();
    static const QRegularExpression http(QStringLiteral("^https://[^\\s\"'`]{1,500}$"));
    if (!http.match(url).hasMatch() || !url.contains(QStringLiteral("<num>"))) {
        setError(error, QStringLiteral("Шаблон автоссылки: https-адрес со вставкой <num>."));
        return {};
    }
    QStringList argv{QStringLiteral("repo"), QStringLiteral("autolink"), QStringLiteral("create"), prefixText, url};
    if (numeric)
        argv << QStringLiteral("--numeric");
    return argv;
}

QStringList githubAutolinkDeleteArgv(const QString &id, QString *error)
{
    resetError(error);
    const QString n = requireNumber(id, error);
    if (n.isEmpty())
        return {};
    return {QStringLiteral("repo"), QStringLiteral("autolink"), QStringLiteral("delete"), n, QStringLiteral("--yes")};
}

QStringList githubProjectCreateArgv(const QString &owner, const QString &title, QString *error)
{
    resetError(error);
    const QString who = requireOwner(owner, error);
    if (who.isEmpty())
        return {};
    if (!titleOk(title, 80)) {
        setError(error, QStringLiteral("Название проекта: без перевода строки, не с дефиса, до 80 символов."));
        return {};
    }
    return {QStringLiteral("project"), QStringLiteral("create"), QStringLiteral("--owner"), who,
            QStringLiteral("--title"), title.trimmed()};
}

QStringList githubProjectFieldArgv(const QString &number, const QString &owner, const QString &name,
                                   const QString &dataType, const QString &options, QString *error)
{
    resetError(error);
    const QString n = requireNumber(number, error);
    if (n.isEmpty())
        return {};
    const QString who = requireOwner(owner, error);
    if (who.isEmpty())
        return {};
    if (!titleOk(name, 40)) {
        setError(error, QStringLiteral("Имя поля: без перевода строки, не с дефиса, до 40 символов."));
        return {};
    }
    const QString type = dataType.trimmed().toUpper();
    if (type != QLatin1String("TEXT") && type != QLatin1String("SINGLE_SELECT") && type != QLatin1String("DATE")
        && type != QLatin1String("NUMBER")) {
        setError(error, QStringLiteral("Тип поля: TEXT, SINGLE_SELECT, DATE или NUMBER."));
        return {};
    }
    const QString rawOptions = options.trimmed();
    if (type != QLatin1String("SINGLE_SELECT") && !rawOptions.isEmpty()) {
        setError(error, QStringLiteral("Список вариантов только у поля со списком."));
        return {};
    }
    QString joined;
    if (type == QLatin1String("SINGLE_SELECT")) {
        const QStringList parts = rawOptions.split(QLatin1Char(','), Qt::SkipEmptyParts);
        QStringList clean;
        for (const QString &part : parts) {
            const QString item = part.trimmed();
            if (!titleOk(item, 40)) {
                setError(error, QStringLiteral("Вариант списка: без запятой внутри, не с дефиса, до 40 символов."));
                return {};
            }
            clean << item;
        }
        if (clean.isEmpty() || clean.size() > 20) {
            setError(error, QStringLiteral("У списка нужно от 1 до 20 вариантов через запятую."));
            return {};
        }
        joined = clean.join(QLatin1Char(','));
    }
    QStringList argv{QStringLiteral("project"), QStringLiteral("field-create"), n, QStringLiteral("--owner"), who,
                     QStringLiteral("--name"), name.trimmed(), QStringLiteral("--data-type"), type};
    if (!joined.isEmpty())
        argv << QStringLiteral("--single-select-options") << joined;
    return argv;
}

QStringList githubProjectItemArgv(const QString &number, const QString &owner, const QString &title, const QString &body,
                                  QString *error)
{
    resetError(error);
    const QString n = requireNumber(number, error);
    if (n.isEmpty())
        return {};
    const QString who = requireOwner(owner, error);
    if (who.isEmpty())
        return {};
    if (!titleOk(title, 256)) {
        setError(error, QStringLiteral("Заголовок карточки: без перевода строки, не с дефиса, до 256 символов."));
        return {};
    }
    if (!bodyOk(body, 65536)) {
        setError(error, QStringLiteral("Текст карточки слишком длинный."));
        return {};
    }
    return {QStringLiteral("project"), QStringLiteral("item-create"), n, QStringLiteral("--owner"), who,
            QStringLiteral("--title"), title.trimmed(), QStringLiteral("--body"), body};
}

QStringList githubProjectAddArgv(const QString &number, const QString &owner, const QString &url, QString *error)
{
    resetError(error);
    const QString n = requireNumber(number, error);
    if (n.isEmpty())
        return {};
    const QString who = requireOwner(owner, error);
    if (who.isEmpty())
        return {};
    static const QRegularExpression re(
        QStringLiteral("^https://github\\.com/[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+/(issues|pull)/[1-9][0-9]{0,8}$"));
    if (!re.match(url.trimmed()).hasMatch()) {
        setError(error, QStringLiteral("Ссылка карточки: https://github.com/владелец/репозиторий/issues/номер или /pull/номер."));
        return {};
    }
    return {QStringLiteral("project"), QStringLiteral("item-add"), n, QStringLiteral("--owner"), who,
            QStringLiteral("--url"), url.trimmed()};
}

QStringList githubProjectEditArgv(const QString &number, const QString &owner, const QString &url, const QString &field,
                                  const QString &value, QString *error)
{
    resetError(error);
    const QString n = requireNumber(number, error);
    if (n.isEmpty())
        return {};
    const QString who = requireOwner(owner, error);
    if (who.isEmpty())
        return {};
    static const QRegularExpression re(
        QStringLiteral("^https://github\\.com/[A-Za-z0-9_.-]+/[A-Za-z0-9_.-]+/(issues|pull)/[1-9][0-9]{0,8}$"));
    if (!re.match(url.trimmed()).hasMatch()) {
        setError(error, QStringLiteral("Ссылка карточки: https://github.com/владелец/репозиторий/issues/номер или /pull/номер."));
        return {};
    }
    if (!titleOk(field, 40)) {
        setError(error, QStringLiteral("Имя поля карточки: без перевода строки, не с дефиса, до 40 символов."));
        return {};
    }
    if (!titleOk(value, 200)) {
        setError(error, QStringLiteral("Значение поля: без перевода строки, не с дефиса, до 200 символов."));
        return {};
    }
    return {QStringLiteral("project"), QStringLiteral("item-edit"), n, QStringLiteral("--owner"), who,
            QStringLiteral("--url"), url.trimmed(), QStringLiteral("--field"), field.trimmed(),
            QStringLiteral("--value"), value.trimmed()};
}

QStringList githubProjectLinkArgv(const QString &number, const QString &owner, QString *error)
{
    resetError(error);
    const QString n = requireNumber(number, error);
    if (n.isEmpty())
        return {};
    const QString who = requireOwner(owner, error);
    if (who.isEmpty())
        return {};
    return {QStringLiteral("project"), QStringLiteral("link"), n, QStringLiteral("--owner"), who};
}

QStringList githubProjectViewArgv(const QString &projectId, const QString &name, const QString &layout, QString *error)
{
    resetError(error);
    static const QRegularExpression idRe(QStringLiteral("^PVT_[A-Za-z0-9_]+$"));
    if (!idRe.match(projectId.trimmed()).hasMatch()) {
        setError(error, QStringLiteral("Не удалось прочитать id проекта."));
        return {};
    }
    if (!titleOk(name, 80)) {
        setError(error, QStringLiteral("Название вида: без перевода строки, не с дефиса, до 80 символов."));
        return {};
    }
    const QString mode = layout.trimmed();
    if (mode != QLatin1String("BOARD_LAYOUT") && mode != QLatin1String("TABLE_LAYOUT")
        && mode != QLatin1String("ROADMAP_LAYOUT")) {
        setError(error, QStringLiteral("Вид: доска, таблица или дорожная карта."));
        return {};
    }
    const QString query = QStringLiteral(
        "mutation($projectId:ID!,$name:String!,$layout:ProjectV2ViewLayout!){"
        "createProjectV2View(input:{projectId:$projectId,name:$name,layout:$layout}){projectV2View{id name}}}");
    return {QStringLiteral("api"), QStringLiteral("graphql"), QStringLiteral("-f"),
            QStringLiteral("query=%1").arg(query), QStringLiteral("-f"),
            QStringLiteral("projectId=%1").arg(projectId.trimmed()), QStringLiteral("-f"),
            QStringLiteral("name=%1").arg(name.trimmed()), QStringLiteral("-f"),
            QStringLiteral("layout=%1").arg(mode)};
}

QStringList githubProjectViewEditArgv(const QString &projectId, const QString &viewId, const QString &name, bool remove,
                                      QString *error)
{
    resetError(error);
    static const QRegularExpression projectRe(QStringLiteral("^PVT_[A-Za-z0-9_]+$"));
    static const QRegularExpression viewRe(QStringLiteral("^PVTV_[A-Za-z0-9_]+$"));
    if (!projectRe.match(projectId.trimmed()).hasMatch()) {
        setError(error, QStringLiteral("Не удалось прочитать id проекта."));
        return {};
    }
    if (!viewRe.match(viewId.trimmed()).hasMatch()) {
        setError(error, QStringLiteral("Id вида начинается с PVTV_."));
        return {};
    }
    if (remove) {
        const QString query = QStringLiteral(
            "mutation($projectId:ID!,$viewId:ID!){"
            "deleteProjectV2View(input:{projectId:$projectId,viewId:$viewId}){projectV2{id}}}");
        return {QStringLiteral("api"), QStringLiteral("graphql"), QStringLiteral("-f"),
                QStringLiteral("query=%1").arg(query), QStringLiteral("-f"),
                QStringLiteral("projectId=%1").arg(projectId.trimmed()), QStringLiteral("-f"),
                QStringLiteral("viewId=%1").arg(viewId.trimmed())};
    }
    if (!titleOk(name, 80)) {
        setError(error, QStringLiteral("Название вида: без перевода строки, не с дефиса, до 80 символов."));
        return {};
    }
    const QString query = QStringLiteral(
        "mutation($projectId:ID!,$viewId:ID!,$name:String!){"
        "updateProjectV2View(input:{projectId:$projectId,viewId:$viewId,name:$name}){projectV2View{id name}}}");
    return {QStringLiteral("api"), QStringLiteral("graphql"), QStringLiteral("-f"),
            QStringLiteral("query=%1").arg(query), QStringLiteral("-f"),
            QStringLiteral("projectId=%1").arg(projectId.trimmed()), QStringLiteral("-f"),
            QStringLiteral("viewId=%1").arg(viewId.trimmed()), QStringLiteral("-f"),
            QStringLiteral("name=%1").arg(name.trimmed())};
}
