#include "Advisor.h"

#include "GithubDeskActions.h"

#include "CommandCatalog.h"
#include "GitRunner.h"
#include "GithubClient.h"
#include "LlmClient.h"
#include "MentorReply.h"
#include "RepoContext.h"

#include <optional>

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

Advisor::Advisor(RepoContext *repo, CommandCatalog *catalog, LlmClient *llm, QObject *parent)
    : QObject(parent)
    , m_repo(repo)
    , m_catalog(catalog)
    , m_llm(llm)
{
    // m_recommendations is a value member — do not parent to `this`.
    if (m_repo)
        connect(m_repo, &RepoContext::changed, this, &Advisor::refresh);
    if (m_llm) {
        // Only clear stale Offline banners when probe succeeds — never auto-enrich.
        // Enrich runs solely via LLM+ / enrichWithLlm() after an explicit user action.
        connect(m_llm, &LlmClient::statusChanged, this, [this]() {
            if (m_llm && m_llm->reachable())
                clearStaleOfflineError();
        });
    }
    m_enrichCooldown.invalidate();
}

RecommendationModel *Advisor::recommendations()
{
    return &m_recommendations;
}

QString Advisor::lastAdvice() const
{
    return m_lastAdvice;
}

bool Advisor::advising() const
{
    return m_advising;
}

bool Advisor::enriching() const
{
    return m_enriching;
}

QString Advisor::lastError() const
{
    return m_lastError;
}

QString Advisor::askUserPrompt() const
{
    return m_askUserPrompt;
}

QStringList Advisor::askUserOptions() const
{
    return m_askUserOptions;
}

bool Advisor::hasAskUser() const
{
    return !m_askUserPrompt.isEmpty();
}

QVariantList Advisor::suggestedCommands() const
{
    return m_suggestedCommands;
}

QString Advisor::activeTask() const
{
    return m_task;
}

void Advisor::setGithub(GithubClient *github)
{
    m_github = github;
}

void Advisor::remember(const QString &line)
{
    const QString text = line.trimmed();
    if (text.isEmpty())
        return;
    m_transcript << (text.size() > 500 ? text.left(500) + QStringLiteral("…") : text);
    while (m_transcript.size() > 8)
        m_transcript.removeFirst();
}

void Advisor::clearAskUser()
{
    if (m_askUserPrompt.isEmpty() && m_askUserOptions.isEmpty())
        return;
    clearAskUserInternal();
    emit adviceChanged();
}

void Advisor::clearDialogue()
{
    m_task.clear();
    m_transcript.clear();
    m_lastAdvice.clear();
    m_lastError.clear();
    m_suggestedCommands.clear();
    m_lastStudentQuestion.clear();
    m_pendingStudentReply.clear();
    m_pendingDressKind.clear();
    clearAskUserInternal();
    emit adviceChanged();
}

QString Advisor::guessDressKindFromPrompt(const QString &prompt)
{
    const QString p = prompt.toLower();
    if (p.contains(QLatin1String("security")) || p.contains(QStringLiteral("безопасн")))
        return QStringLiteral("security");
    if (p.contains(QLatin1String("funding")) || p.contains(QStringLiteral("поддержк"))
        || p.contains(QStringLiteral("спонсор")))
        return QStringLiteral("funding");
    if (p.contains(QLatin1String("contributing")) || p.contains(QStringLiteral("участи")))
        return QStringLiteral("contributing");
    if (p.contains(QLatin1String("conduct")) || p.contains(QStringLiteral("поведени")))
        return QStringLiteral("conduct");
    if (p.contains(QLatin1String("support")) || p.contains(QStringLiteral("поддержк")))
        return QStringLiteral("support");
    if (p.contains(QLatin1String("codeowners")) || p.contains(QStringLiteral("владельц")))
        return QStringLiteral("codeowners");
    if (p.contains(QLatin1String("gitignore")) || p.contains(QStringLiteral("игнор")))
        return QStringLiteral("gitignore");
    if (p.contains(QLatin1String("readme")))
        return QStringLiteral("readme");
    if (p.contains(QLatin1String("license")) || p.contains(QStringLiteral("лиценз")))
        return QStringLiteral("license");
    if (p.contains(QLatin1String("issue")))
        return QStringLiteral("issue");
    if (p.contains(QLatin1String("pull")) || p.contains(QStringLiteral("pr ")))
        return QStringLiteral("pull");
    return {};
}

void Advisor::clearAskUserInternal()
{
    m_askUserPrompt.clear();
    m_askUserOptions.clear();
}

void Advisor::applyMentorReply(const QString &raw)
{
    const MentorReply parsed = parseMentorReply(raw);
    m_lastAdvice = parsed.answer;
    if (parsed.askUser.isValid()) {
        m_askUserPrompt = parsed.askUser.prompt;
        m_askUserOptions = parsed.askUser.options;
    } else {
        clearAskUserInternal();
    }

    QVariantList commands;
    for (const MentorCommand &cmd : parsed.commands) {
        QVariantMap args = cmd.args;
        const bool desk = githubDeskActionKnown(cmd.id);
        if (desk && !githubDeskAcceptAdvisorCommand(cmd.id, &args))
            continue;
        if (!desk && (!m_catalog || !m_catalog->findById(cmd.id)))
            continue;
        bool seen = false;
        for (const QVariant &existing : commands) {
            if (existing.toMap().value(QStringLiteral("commandId")).toString() == cmd.id) {
                seen = true;
                break;
            }
        }
        if (seen)
            continue;
        const std::optional<CommandSpec> spec = (!desk && m_catalog) ? m_catalog->findById(cmd.id) : std::nullopt;
        QVariantMap row;
        row.insert(QStringLiteral("commandId"), cmd.id);
        row.insert(QStringLiteral("title"), desk ? githubDeskActionTitle(cmd.id) : (spec ? spec->title : cmd.id));
        row.insert(QStringLiteral("args"), args);
        commands.append(row);
        if (commands.size() >= 4)
            break;
    }
    m_suggestedCommands = commands;
}

QVector<Recommendation> Advisor::buildRuleRecommendations() const
{
    QVector<Recommendation> out;
    if (!m_repo || !m_repo->isOpen() || !m_catalog)
        return out;

    auto add = [&](const QString &id, int priority, const QString &reason) {
        const auto spec = m_catalog->findById(id);
        if (!spec)
            return;
        for (const Recommendation &r : out) {
            if (r.commandId == id)
                return;
        }
        Recommendation rec;
        rec.commandId = id;
        rec.title = spec->title;
        rec.reason = reason;
        rec.source = QStringLiteral("rule");
        rec.priority = priority;
        out.push_back(rec);
    };

    const int staged = m_repo->stagedCount();
    const int unstaged = m_repo->unstagedCount();
    const int untracked = m_repo->untrackedCount();
    const int dirty = m_repo->dirtyCount();
    const int ahead = m_repo->ahead();
    const int behind = m_repo->behind();
    const bool hasUp = m_repo->hasUpstream();

    const int conflictCount = m_repo->conflictCount();
    const bool hasConflict = conflictCount > 0;
    const QString op = m_repo->operationKind();

    if (hasConflict || !op.isEmpty()) {
        if (hasConflict) {
            add(QStringLiteral("status"), 100,
                tr("Конфликт: %1 файл(ов) — сначала посмотрите статус.").arg(conflictCount));
            add(QStringLiteral("diff"), 95, tr("Просмотрите конфликтные участки перед продолжением."));
            add(QStringLiteral("add_path"), 90, tr("После ручного разрешения добавьте файл в индекс."));
            add(QStringLiteral("checkout_ours"), 60, tr("В крайнем случае возьмите версию текущей ветки (--ours)."));
            add(QStringLiteral("checkout_theirs"), 58, tr("Или версию вливаемой ветки (--theirs) — осторожно."));
        }
        if (op == QLatin1String("merge")) {
            add(QStringLiteral("merge_continue"), 88,
                hasConflict ? tr("После разрешения конфликтов завершите merge --continue.")
                            : tr("Незавершённый merge — можно продолжить merge --continue."));
            add(QStringLiteral("merge_abort"), 55, tr("Безопасно отменить незавершённый merge."));
        } else if (op == QLatin1String("rebase")) {
            add(QStringLiteral("rebase_continue"), 88,
                hasConflict ? tr("После разрешения конфликтов продолжите rebase --continue.")
                            : tr("Незавершённый rebase — продолжите rebase --continue."));
            add(QStringLiteral("rebase_abort"), 55, tr("Безопасно отменить незавершённый rebase."));
        } else if (op == QLatin1String("cherry-pick")) {
            add(QStringLiteral("cherry_pick_continue"), 88,
                hasConflict ? tr("После разрешения конфликтов продолжите cherry-pick --continue.")
                            : tr("Незавершённый cherry-pick — продолжите --continue."));
            add(QStringLiteral("cherry_pick_abort"), 55, tr("Безопасно отменить незавершённый cherry-pick."));
        } else if (op == QLatin1String("revert")) {
            add(QStringLiteral("revert_continue"), 88,
                hasConflict ? tr("После разрешения конфликтов продолжите revert --continue.")
                            : tr("Незавершённый revert — продолжите --continue."));
            add(QStringLiteral("revert_abort"), 55, tr("Безопасно отменить незавершённый revert."));
        } else if (hasConflict) {
            add(QStringLiteral("merge_continue"), 70, tr("Если это merge — завершите через merge --continue."));
            add(QStringLiteral("rebase_continue"), 69, tr("Если это rebase — продолжите через rebase --continue."));
            add(QStringLiteral("merge_abort"), 50, tr("Безопасно отменить незавершённый merge."));
            add(QStringLiteral("rebase_abort"), 49, tr("Безопасно отменить незавершённый rebase."));
        }
    }

    if (staged > 0) {
        add(QStringLiteral("diff_staged"), 95, tr("В индексе %1 файл(ов) — проверьте перед коммитом.").arg(staged));
        add(QStringLiteral("commit"), 92, tr("Есть изменения в индексе — логичный следующий шаг: commit."));
    }

    if (unstaged > 0) {
        add(QStringLiteral("diff"), 88, tr("Есть незакоммиченные правки вне индекса (%1).").arg(unstaged));
        add(QStringLiteral("add_all"), 85, tr("Добавьте изменения в индекс (add), затем commit."));
        add(QStringLiteral("restore_worktree"), 40, tr("Если правки не нужны — можно откатить рабочее дерево."));
    }

    if (untracked > 0) {
        add(QStringLiteral("status"), 80, tr("Есть неотслеживаемые файлы (%1).").arg(untracked));
        add(QStringLiteral("add_path"), 78, tr("Добавьте нужные новые файлы точечно."));
        add(QStringLiteral("clean_dry"), 35, tr("Посмотрите, что удалит clean (сначала dry-run)."));
    }

    if (hasUp && ahead > 0 && behind > 0) {
        add(QStringLiteral("log_graph"), 98,
            tr("Ветка разошлась с upstream (↑%1 ↓%2) — сверьте историю.").arg(ahead).arg(behind));
        add(QStringLiteral("pull_rebase"), 90,
            tr("Частый путь: pull --rebase, затем push (линейная история)."));
        add(QStringLiteral("pull"), 82, tr("Или обычный pull (merge), если rebase нежелателен."));
        add(QStringLiteral("push_force_with_lease"), 40,
            tr("Force-with-lease — только если уже переписали локальную историю осознанно."));
    } else if (hasUp && behind > 0) {
        add(QStringLiteral("fetch"), 86, tr("Ветка позади upstream на %1 коммит(ов) — сначала fetch.").arg(behind));
        add(QStringLiteral("pull_rebase"), 85, tr("Подтяните изменения через pull --rebase (без merge-коммита)."));
        add(QStringLiteral("pull"), 83, tr("Или обычный pull (merge), если так принято в команде."));
        add(QStringLiteral("log_graph"), 60, tr("Сверьте историю локально и на remote."));
    } else if (hasUp && ahead > 0) {
        add(QStringLiteral("log_oneline"), 82, tr("Локально на %1 коммит(ов) больше — просмотрите перед push.").arg(ahead));
        add(QStringLiteral("push"), 80, tr("Отправьте локальные коммиты на upstream."));
    }

    if (!hasUp) {
        add(QStringLiteral("remote_v"), 55, tr("Upstream не настроен — проверьте remote."));
        add(QStringLiteral("branch_list"), 50, tr("Посмотрите локальные ветки и их tracking."));
        if (m_repo->remotes()->rowCount() > 0) {
            const QString branch = m_repo->branch();
            if (!branch.isEmpty()) {
                add(QStringLiteral("branch_set_upstream"), 76,
                    tr("Привяжите ветку к %1/%2 (track).")
                        .arg(m_repo->remotes()->nameAt(0), branch));
            }
            if (m_repo->localCommits() > 0) {
                add(QStringLiteral("push"), 78,
                    tr("Есть локальные коммиты без upstream — push -u origin <ветка>."));
            }
        }
    }

    if (m_repo->remotes()->rowCount() > 0 && dirty == 0 && !hasConflict && op.isEmpty()) {
        add(QStringLiteral("fetch_prune"), 30,
            tr("Дерево чистое — можно fetch --prune и убрать устаревшие remote-ветки."));
    }

    if (dirty == 0 && !hasConflict) {
        add(QStringLiteral("status"), 45, tr("Рабочее дерево чистое — можно осмотреть статус/историю."));
        add(QStringLiteral("log_oneline"), 44, tr("Просмотрите недавние коммиты."));
        add(QStringLiteral("log_graph"), 42, tr("Граф веток помогает понять структуру истории."));
    }

    // Local commit identity — without it commit and license scaffolds fail.
    {
        GitRunner git(4000);
        const GitRunResult name = git.run(m_repo->rootPath(), {QStringLiteral("config"), QStringLiteral("user.name")});
        const GitRunResult email = git.run(m_repo->rootPath(), {QStringLiteral("config"), QStringLiteral("user.email")});
        const bool hasName = name.ok && !QString::fromUtf8(name.stdoutBytes).trimmed().isEmpty();
        const bool hasEmail = email.ok && !QString::fromUtf8(email.stdoutBytes).trimmed().isEmpty();
        if (!hasName || !hasEmail) {
            add(QStringLiteral("config_set_user"), 99,
                tr("Не заданы локальные user.name / user.email — коммиты и LICENSE не пройдут."));
        }
    }

    if (m_github && m_repo->localCommits() > 0) {
        if (!m_github->hasReadme()) {
            add(QStringLiteral("github_readme"), 34,
                tr("Нет README — на GitHub репозиторий без него выглядит пустым."));
        }
        if (!m_github->hasLicense()) {
            add(QStringLiteral("github_license"), 33,
                tr("Нет файла лицензии. Вид выбирается явно: mit, bsd2 или proprietary."));
        }
        if (m_github->available() && m_github->authenticated() && !m_github->hasOrigin()) {
            add(QStringLiteral("github_create"), 84,
                tr("Локальные коммиты ещё не на GitHub. Создайте приватный репозиторий и отправьте ветку."));
        }
        if (m_github->originIsGithub() && !hasUp) {
            add(QStringLiteral("github_push"), 83,
                tr("Репозиторий на GitHub есть, upstream не привязан — опубликуйте текущую ветку."));
        }
        if (m_github->originIsGithub() && m_github->description().trimmed().isEmpty()) {
            add(QStringLiteral("github_describe"), 36, tr("У репозитория на GitHub пустое описание."));
        }
    }

    if (out.isEmpty())
        add(QStringLiteral("status"), 10, tr("Начните с git status."));

    return out;
}

QString Advisor::repoSnapshot() const
{
    if (!m_repo || !m_repo->isOpen())
        return tr("Репозиторий не открыт.");
    QString snapshot = m_repo->dumpStatusJson();
    {
        GitRunner git(4000);
        const GitRunResult name = git.run(m_repo->rootPath(), {QStringLiteral("config"), QStringLiteral("user.name")});
        const GitRunResult email = git.run(m_repo->rootPath(), {QStringLiteral("config"), QStringLiteral("user.email")});
        const QString n = name.ok ? QString::fromUtf8(name.stdoutBytes).trimmed() : QString();
        const QString e = email.ok ? QString::fromUtf8(email.stdoutBytes).trimmed() : QString();
        snapshot += QStringLiteral("\n\ngitIdentity:\n  user.name: %1\n  user.email: %2\n")
                        .arg(n.isEmpty() ? QStringLiteral("(missing)") : n,
                             e.isEmpty() ? QStringLiteral("(missing)") : e);
    }
    if (m_github)
        snapshot += QStringLiteral("\n\nGitHub:\n") + m_github->snapshotText();
    if (!m_pendingStudentReply.isEmpty()) {
        snapshot += QStringLiteral("\n\npendingStudentReply:\n%1\n").arg(m_pendingStudentReply.left(2000));
        if (!m_pendingDressKind.isEmpty())
            snapshot += QStringLiteral("pendingDressKind: %1\n").arg(m_pendingDressKind);
    }
    return snapshot;
}

void Advisor::refresh()
{
    if (!m_repo || !m_repo->isOpen()) {
        m_recommendations.clear();
        return;
    }
    m_recommendations.setItems(buildRuleRecommendations());
    // Rules only. LLM enrich is manual (кнопка LLM+) — иначе при открытии репо
    // probe→statusChanged запускал бы chat/completions без запроса пользователя.
}

QString Advisor::statusFingerprint() const
{
    if (!m_repo || !m_repo->isOpen())
        return {};
    return QStringLiteral("%1|%2|%3|%4|%5|%6|%7|%8|%9")
        .arg(m_repo->branch(), m_repo->operationKind())
        .arg(m_repo->stagedCount())
        .arg(m_repo->unstagedCount())
        .arg(m_repo->untrackedCount())
        .arg(m_repo->conflictCount())
        .arg(m_repo->ahead())
        .arg(m_repo->behind())
        .arg(m_repo->lastCommitSubject());
}

void Advisor::clearStaleOfflineError()
{
    if (m_lastError.isEmpty())
        return;
    if (m_lastError.startsWith(QLatin1String("Offline"))
        || m_lastError.contains(QStringLiteral("LLM недоступен"))) {
        m_lastError.clear();
        emit adviceChanged();
    }
}

void Advisor::enrichWithLlm()
{
    runEnrich(true);
}

void Advisor::parseLlmRecommendations(const QString &text)
{
    if (!m_catalog)
        return;

    QString payload = text.trimmed();
    static const QRegularExpression fence(QStringLiteral("```(?:json)?\\s*([\\s\\S]*?)```"));
    const auto m = fence.match(payload);
    if (m.hasMatch())
        payload = m.captured(1).trimmed();

    const int start = payload.indexOf(QLatin1Char('['));
    const int end = payload.lastIndexOf(QLatin1Char(']'));
    if (start >= 0 && end > start)
        payload = payload.mid(start, end - start + 1);

    const QJsonDocument doc = QJsonDocument::fromJson(payload.toUtf8());
    if (!doc.isArray()) {
        m_lastError = tr("LLM вернул рекомендации не в формате JSON-массива");
        emit adviceChanged();
        return;
    }

    QVector<Recommendation> llm;
    int prio = 70;
    for (const QJsonValue &v : doc.array()) {
        if (!v.isObject())
            continue;
        const QJsonObject o = v.toObject();
        const QString id = o.value(QStringLiteral("commandId")).toString().trimmed();
        if (id.isEmpty())
            continue;
        const auto spec = m_catalog->findById(id);
        if (!spec)
            continue;
        Recommendation r;
        r.commandId = id;
        r.title = spec->title;
        r.reason = o.value(QStringLiteral("reason")).toString().trimmed();
        if (r.reason.isEmpty())
            r.reason = tr("Рекомендация модели");
        r.source = QStringLiteral("llm");
        r.priority = prio--;
        llm.push_back(r);
    }
    if (!llm.isEmpty())
        m_recommendations.mergeLlmItems(llm);
}

void Advisor::runEnrich(bool force)
{
    if (!m_llm || !m_repo || !m_repo->isOpen() || m_enriching)
        return;

    if (!m_llm->reachable()) {
        if (force)
            m_llm->probe();
        return;
    }

    const QString fp = statusFingerprint();
    constexpr qint64 kCooldownMs = 60000;
    if (!force && m_enrichCooldown.isValid() && m_enrichCooldown.elapsed() < kCooldownMs
        && fp == m_lastEnrichFingerprint) {
        return;
    }

    m_enriching = true;
    emit enrichingChanged();
    m_lastError.clear();
    emit adviceChanged();

    const QStringList allowed = m_catalog->allIds();
    const QString system = tr(
        "Ты — преподаватель Git в приложении GHEW. "
        "По снимку репозитория предложи 3–5 следующих учебных действий. "
        "Финальный ответ — ТОЛЬКО JSON-массив объектов "
        "{\"commandId\":\"...\",\"reason\":\"...на русском...\"} "
        "в обычном content (не только в reasoning). "
        "commandId строго из списка: %1. "
        "Не выдумывай команды. Без markdown и пояснений вне JSON.")
                               .arg(allowed.join(QLatin1Char(',')));
    const QString user = tr("Снимок репозитория (JSON):\n%1").arg(repoSnapshot());

    // Gemma spends many tokens on reasoning_content; keep headroom for final JSON.
    m_llm->complete(system, user, 1600, [this, fp](bool ok, const QString &text) {
        m_enriching = false;
        emit enrichingChanged();
        if (!ok) {
            if (m_llm && m_llm->reachable())
                m_lastError = tr("Ошибка обогащения LLM: %1").arg(text);
            else
                m_lastError = tr("Offline: LLM недоступен — показаны только rule-based рекомендации. (%1)")
                                  .arg(text);
            emit adviceChanged();
            return;
        }
        m_lastEnrichFingerprint = fp;
        m_enrichCooldown.restart();
        m_lastError.clear();
        parseLlmRecommendations(text);
        emit adviceChanged();
    });
}

void Advisor::answerAskUser(const QString &option)
{
    replyMentor(option);
}

void Advisor::replyMentor(const QString &text)
{
    const QString opt = text.trimmed();
    if (opt.isEmpty() || m_advising)
        return;

    // Chip that is a bare catalog id — run it (no args). Prefer free text for dress/config.
    if (hasAskUser() && m_catalog && m_catalog->findById(opt) && m_askUserOptions.contains(opt)) {
        clearAskUserInternal();
        emit adviceChanged();
        emit runCommandRequested(opt);
        return;
    }

    if (hasAskUser()) {
        const QString priorAsk = m_askUserPrompt;
        clearAskUserInternal();
        if (m_task.isEmpty())
            m_task = priorAsk.isEmpty() ? opt : priorAsk;
        remember(tr("Студент ответил: %1").arg(opt));
        m_pendingStudentReply = opt;
        m_pendingDressKind = guessDressKindFromPrompt(priorAsk);

        // If the ask was about a dress file, offer RUN immediately with the student's text —
        // do not wait for the LLM to copy it into a body (demo-quality failure mode).
        if (!m_pendingDressKind.isEmpty()) {
            QVariantMap args;
            args.insert(QStringLiteral("kind"), m_pendingDressKind);
            args.insert(QStringLiteral("body"), opt);
            QVariantMap row;
            row.insert(QStringLiteral("commandId"), QStringLiteral("gh_file"));
            row.insert(QStringLiteral("title"), githubDeskActionTitle(QStringLiteral("gh_file")));
            row.insert(QStringLiteral("args"), args);
            m_suggestedCommands = {row};
            m_lastAdvice = tr("Принял текст для `%1`. Нажмите «Выполнить», чтобы записать файл.\n\n"
                              "Вопрос был: %2")
                               .arg(m_pendingDressKind, priorAsk);
            emit adviceChanged();
            return;
        }

        // Identity from free text: "Name <email@x>" or two lines.
        if (priorAsk.contains(QStringLiteral("user.name"), Qt::CaseInsensitive)
            || priorAsk.contains(QStringLiteral("email"), Qt::CaseInsensitive)
            || priorAsk.contains(QStringLiteral("автор"), Qt::CaseInsensitive)
            || priorAsk.contains(QStringLiteral("идентит"), Qt::CaseInsensitive)) {
            QString name;
            QString email;
            static const QRegularExpression angled(QStringLiteral("^\\s*(.+?)\\s*<([^>]+@[^>]+)>\\s*$"));
            const auto m = angled.match(opt);
            if (m.hasMatch()) {
                name = m.captured(1).trimmed();
                email = m.captured(2).trimmed();
            } else {
                const QStringList lines = opt.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
                if (lines.size() >= 2) {
                    name = lines.at(0).trimmed();
                    email = lines.at(1).trimmed();
                }
            }
            if (!name.isEmpty() && email.contains(QLatin1Char('@'))) {
                QVariantMap args;
                args.insert(QStringLiteral("name"), name);
                args.insert(QStringLiteral("email"), email);
                QVariantMap row;
                row.insert(QStringLiteral("commandId"), QStringLiteral("config_set_user"));
                row.insert(QStringLiteral("title"),
                           m_catalog && m_catalog->findById(QStringLiteral("config_set_user"))
                               ? m_catalog->findById(QStringLiteral("config_set_user"))->title
                               : QStringLiteral("config_set_user"));
                row.insert(QStringLiteral("args"), args);
                m_suggestedCommands = {row};
                m_lastAdvice = tr("Принял автора %1 <%2>. Нажмите «Выполнить», чтобы записать локальный config.")
                                   .arg(name, email);
                emit adviceChanged();
                return;
            }
        }

        emit adviceChanged();
        const QString follow = priorAsk.isEmpty()
                                   ? tr("Ответ студента: %1. Продолжай ту же задачу. "
                                        "Если нужно записать файл — добавь RUN с gh_file и полным body "
                                        "(текст ответа уже в сообщении).")
                                         .arg(opt)
                                   : tr("Наставник спросил: «%1». Студент ответил своим текстом: %2. "
                                        "Продолжай ту же задачу. Для файла оформления — RUN gh_file "
                                        "с kind и body = текст студента целиком.")
                                         .arg(priorAsk, opt);
        startTurn(follow, false);
        return;
    }

    if (!m_task.isEmpty()) {
        remember(tr("Студент: %1").arg(opt));
        m_pendingStudentReply = opt;
        startTurn(tr("Сообщение студента по текущей задаче «%1»:\n%2\n"
                     "Продолжай ту же задачу. Не начинай новую. "
                     "Если нужна запись файла — RUN gh_file с body из этого сообщения.")
                      .arg(m_task, opt),
                  false);
        return;
    }

    askAdvice(opt);
}

void Advisor::applyMentorReplyText(const QString &raw)
{
    applyMentorReply(raw);
    emit adviceChanged();
}

QString Advisor::takePendingStudentReply()
{
    const QString body = m_pendingStudentReply;
    m_pendingStudentReply.clear();
    return body;
}

QString Advisor::takePendingDressKind()
{
    const QString kind = m_pendingDressKind;
    m_pendingDressKind.clear();
    return kind;
}

QString Advisor::pendingDressKind() const
{
    return m_pendingDressKind;
}

void Advisor::continueAfterCommand(const QString &commandId, const QString &cmdline, int exitCode,
                                   const QString &outputTail, const QString &delta)
{
    if (m_advising || !m_llm || !m_repo || !m_repo->isOpen())
        return;
    if (m_task.isEmpty())
        m_task = tr("Разобрать дерево и довести следующий шаг");
    const QString tail = outputTail.trimmed();
    remember(tr("Выполнено %1 (`%2`), exit %3. %4%5")
                 .arg(commandId, cmdline)
                 .arg(exitCode)
                 .arg(delta, tail.isEmpty() ? QString() : tr("\nВывод: %1").arg(tail.left(400))));
    startTurn(tr("Команда уже выполнена приложением. Задача не изменилась: «%1».\n"
                 "Не проси студента заново описать задачу и не проси вручную повторить эту команду.\n"
                 "По новому снимку, включая блок GitHub, скажи, что изменилось, и один следующий шаг. "
                 "Если шаг — команда каталога (в том числе github_*), добавь блок RUN.")
                  .arg(m_task),
              false);
}

void Advisor::askAdvice(const QString &question)
{
    startTurn(question.trimmed(), true);
}

void Advisor::startTurn(const QString &userExtra, bool newTask)
{
    const QString q = userExtra.trimmed();
    if (q.isEmpty()) {
        m_lastError = tr("Введите вопрос.");
        emit adviceChanged();
        return;
    }
    if (!m_llm) {
        m_lastError = tr("LLM-клиент не создан.");
        emit adviceChanged();
        return;
    }
    if (!m_repo || !m_repo->isOpen()) {
        m_lastError = tr("Сначала откройте репозиторий.");
        emit adviceChanged();
        return;
    }
    if (m_advising)
        return;

    if (!m_llm->reachable())
        m_llm->probe();
    if (m_github) {
        m_github->refreshLocalDress();
        m_github->refreshIfStale(45);
    }

    if (newTask) {
        m_task = q;
        m_transcript.clear();
        remember(tr("Задача: %1").arg(m_task));
        m_lastAdvice.clear();
    } else if (m_task.isEmpty()) {
        m_task = q;
        remember(tr("Задача: %1").arg(m_task));
    }
    m_lastStudentQuestion = newTask ? q : m_task;
    m_advising = true;
    emit advisingChanged();
    m_lastError.clear();
    if (!newTask)
        m_lastAdvice = tr("Смотрю, что изменилось по задаче «%1»…").arg(m_task);
    clearAskUserInternal();
    m_suggestedCommands.clear();
    emit adviceChanged();

    QStringList catalogHints;
    for (const Recommendation &r : m_recommendations.items())
        catalogHints << QStringLiteral("%1 — %2").arg(r.commandId, r.title);

    const QString system = tr(
        "Ты — спокойный наставник по Git для русскоязычного студента.\n"
        "ЖЁСТКИЕ ПРАВИЛА ВЫВОДА:\n"
        "1) В content пиши ТОЛЬКО финальный ответ студенту на русском (до ~8 предложений).\n"
        "2) ЗАПРЕЩЕНО писать в content: Self-Correction, Final Polish, чеклисты, "
        "английские черновики, «Max 12 sentences? Yes», статусы вида «diff - ok».\n"
        "3) Reasoning держи в reasoning_content, не копируй его в content.\n"
        "4) Опирайся на снимок репозитория; не выдумывай состояние.\n"
        "5) Команды называй id из каталога или gh_* и кратко объясняй зачем. Id каталога: %1.\n"
        "6) Опасные команды — только с явным предупреждением.\n"
        "7) Если нужен ответ или выбор студента — НЕ заканчивай обычным вопросом в тексте. "
        "Вместо этого в конце content добавь ровно один блок инструмента ask_user:\n"
        "<<<ASK_USER>>>\n"
        "{\"prompt\":\"Краткий вопрос на русском\",\"options\":[\"вариант1\",\"вариант2\"]}\n"
        "<<<END_ASK_USER>>>\n"
        "options необязательны: студент может ответить своим текстом. Если даёте options — 2–5 коротких "
        "вариантов на русском. Если вариант — команда каталога, пишите её id (например diff).\n"
        "8) Если следующий шаг — конкретная команда каталога, НЕ ограничивайтесь фразой "
        "«используй commit». В конце content добавьте блок, чтобы кнопка её выполнила:\n"
        "<<<RUN>>>\n"
        "{\"commands\":[{\"id\":\"commit\",\"args\":{\"message\":\"кратко по-русски\"}}]}\n"
        "<<<END_RUN>>>\n"
        "id — из каталога или из списка правок GitHub ниже. Для commit всегда заполните args.message. "
        "Опасные id тоже указывайте — студент подтвердит их отдельно. "
        "Кнопка gh_* вызывает ту же функцию, что и вкладка GitHub, с тем же превью.\n"
        "Правки GitHub:\n%2\n"
        "Значение секрета, секрет вебхука и закрытый ключ в RUN не клади: их вводят на вкладке. "
        "Удаление секрета — gh_secret_delete без value. "
        "gh_issue_act.action: close, reopen, comment, pin, unpin, lock, unlock, delete. "
        "gh_issue_link.kind: assignee, subissue, blocked, blocking, parent. "
        "gh_pull_review.action: approve, comment, request. gh_pull_merge.method: merge, squash, rebase. "
        "gh_email.action: add, delete, public, private. gh_org_people.action: invite, cancel, remove-member, remove-outside. "
        "gh_workflow.action: enable, disable, run. gh_run.action: cancel, rerun, delete. "
        "gh_file.kind и gh_template.kind: readme, license, gitignore, contributing, conduct, security, support, funding, codeowners, issue, pull. "
        "Из RUN gh_template сразу пишет файл на диск; на вкладке GitHub шаблон только в черновик. "
        "gh_file пишет body; из RUN body обязателен (текст студента). "
        "config_set_user пишет локальные user.name и user.email (args.name, args.email) — "
        "предлагай его, если в снимке gitIdentity missing. "
        "После записи файла следующий шаг — add и commit. В снимке смотри dress.*: yes/no — не утверждай, "
        "что файл не применился, если там yes. "
        "Номера issue, PR и релизов бери из gh_services, не выдумывай. "
        "Не предлагай удалить репозиторий, слить pull request, удалить релиз или issue, пока студент сам не попросил. "
        "Не предлагай --all, scope all, смену логина организации и публикацию совета.\n"
        "9) Веди одну задачу, пока студент сам не напишет новую. После выполненной команды "
        "смотри новый снимок и называй следующий шаг. Не проси пересказать задачу и не проси "
        "«посмотри статус», если снимок уже в сообщении.\n"
        "10) Оформление GitHub — часть той же задачи. Блок GitHub в снимке — источник правды. "
        "github_create только если origin ещё нет, и только приватный, пока студент явно не попросил публичный. "
        "В args передайте name и, если есть, description. "
        "github_license требует args.kind: mit, bsd2 или proprietary — не выбирай вид сам, спроси ask_user. "
        "После github_readme или github_license следующий шаг — add и commit, затем github_push. "
        "github_describe, github_topic, github_topic_remove, github_visibility и github_rename — только когда origin уже на GitHub. "
        "public и github_rename — только если студент явно попросил. "
        "Текст README, LICENSE и .gitignore и остальные файлы оформления — gh_file; не затирай уже существующий файл, пока студент сам не попросил. "
        "Архив, удаление, передача, форк, LFS, Pages и отправка wiki идут через gh_* и останавливаются на превью. "
        "Issues, pull requests, обсуждения, projects, релизы и автоссылки — тоже gh_*. "
        "Слияние pull request, обновление его ветки, удаление issue, метки, вехи, релиза, автоссылки и комментария обсуждения, а также создание и правка релиза требуют превью. "
        "Создание pull request передаёт head и не пушит ветку само. "
        "Соавторы, Actions, секреты, переменные, окружения, кэш, оповещения, ruleset, защита веток, deploy key, вебхуки, советы безопасности и codespaces — gh_*. "
        "Секрет и секрет вебхука не называй и не проси повторить: программа не пишет их в превью и журнал. "
        "Смена доступа, запуск и удаление workflow, удаление секрета, ruleset, защита ветки, ключи, вебхуки, черновик совета и создание или удаление codespace требуют превью. "
        "Не предлагай удалить все кэши или все codespace. "
        "Профиль и организация — gh_profile, gh_email, gh_emails, gh_social, gh_user_key, gh_profile_repo, gh_profile_readme и gh_org_*. "
        "Список почты кнопка gh_emails показывает на карточке; сами адреса в снимок не попадают. "
        "Входящее приглашение — gh_invitation_accept по одному номеру и требует превью. "
        "Закрытый ключ не принимается. "
        "Удаление почты, ключей, участника, команды, роли, вебхука, политика организации, security manager, immutable releases и README профиля требуют превью. "
        "Не предлагай удалить организацию, сменить её логин или назначить конфигурацию безопасности на все репозитории. "
        "Срок баннера, ревьюеры окружения, переименование и удаление вида проекта, назначение существующей роли организации и immutable releases selected — соответствующие gh_*. "
        "Конфигурация безопасности назначается только на перечисленные номера репозиториев. "
        "Форк по умолчанию не переименовывает origin. Переключение origin на форк, удаление, передачу и отправку wiki называй только если студент явно попросил. "
        "На вкладке GitHub шаблон только заполняет черновик, пока студент не нажмёт запись. "
        "Не проси затирать уже существующий файл, пока студент сам не попросил.")
                               .arg(m_catalog->allIds().join(QLatin1Char(',')), githubDeskActionCatalog());

    const QString user = tr("Задача:\n%1\n\nХод:\n%2\n\nСнимок репозитория:\n%3\n\n"
                            "Текущие рекомендации UI:\n%4\n\nСейчас:\n%5")
                             .arg(m_task,
                                  m_transcript.isEmpty() ? tr("(пока пусто)") : m_transcript.join(QStringLiteral("\n")),
                                  repoSnapshot(),
                                  catalogHints.isEmpty() ? tr("(нет)") : catalogHints.join(QLatin1Char('\n')),
                                  q);

    // contentOnly: do not stream reasoning_content into the mentor pane.
    m_llm->completeStream(
        system, user, 1200,
        [this](const QString &) {
            // Live preview: show only sanitized text (hides CoT while model dumps drafts).
            const MentorReply preview = parseMentorReply(m_llm->streamBuffer());
            if (!preview.answer.isEmpty()) {
                m_lastAdvice = preview.answer;
                m_lastError.clear();
                emit adviceChanged();
            }
        },
        [this, q](bool ok, const QString &text) {
            m_advising = false;
            emit advisingChanged();
            if (!ok) {
                clearAskUserInternal();
                const bool offline = !m_llm || !m_llm->reachable();
                QStringList lines;
                if (offline)
                    lines << tr("**LLM недоступен** — ответ по правилам (offline).");
                else
                    lines << tr("**Ошибка запроса к LLM** — ниже запасной ответ по правилам.");
                lines << tr("Вопрос: %1").arg(q);
                lines << QString();
                lines << tr("По текущему статусу репозитория:");
                const auto items = m_recommendations.items();
                if (items.isEmpty()) {
                    lines << tr("- Откройте вкладку «Советы» после refresh, или выполните status.");
                } else {
                    int n = 0;
                    for (const Recommendation &r : items) {
                        if (n++ >= 5)
                            break;
                        lines << tr("- **%1** (`%2`): %3").arg(r.title, r.commandId, r.reason);
                        if (m_catalog) {
                            if (const auto spec = m_catalog->findById(r.commandId)) {
                                if (!spec->explanation.isEmpty())
                                    lines << tr("  Почему: %1").arg(spec->explanation.left(220));
                            }
                        }
                    }
                }
                lines << QString();
                if (offline) {
                    lines << tr("Запустите local-agent / llama.cpp на %1 и нажмите «Проверить LLM».")
                                 .arg(m_llm ? m_llm->baseUrl() : QStringLiteral("http://127.0.0.1:8080/v1"));
                    m_lastError = tr("Offline: %1").arg(text);
                } else {
                    m_lastError = tr("Ошибка LLM: %1").arg(text);
                }
                // Attach RUN for the top safe/mutates recommendations so offline is not demo-only prose.
                QJsonArray cmds;
                int added = 0;
                for (const Recommendation &r : items) {
                    if (added >= 2)
                        break;
                    if (r.commandId == QLatin1String("push_force_with_lease")
                        || r.commandId == QLatin1String("checkout_ours")
                        || r.commandId == QLatin1String("checkout_theirs"))
                        continue;
                    QJsonObject cmd;
                    cmd.insert(QStringLiteral("id"), r.commandId);
                    QJsonObject args;
                    if (r.commandId == QLatin1String("commit") || r.commandId == QLatin1String("commit_amend"))
                        args.insert(QStringLiteral("message"), tr("chore: шаг по статусу"));
                    cmd.insert(QStringLiteral("args"), args);
                    cmds.append(cmd);
                    ++added;
                }
                if (!cmds.isEmpty()) {
                    QJsonObject root;
                    root.insert(QStringLiteral("commands"), cmds);
                    lines << QStringLiteral("<<<RUN>>>");
                    lines << QString::fromUtf8(QJsonDocument(root).toJson(QJsonDocument::Compact));
                    lines << QStringLiteral("<<<END_RUN>>>");
                }
                applyMentorReply(lines.join(QLatin1Char('\n')));
            } else {
                applyMentorReply(text);
                m_lastError.clear();
                if (m_lastAdvice.isEmpty()) {
                    m_lastAdvice = tr("Модель вернула черновик без финального ответа. "
                                      "Нажмите «Спросить» ещё раз или сократите вопрос.");
                }
            }
            remember(tr("Наставник: %1").arg(m_lastAdvice.left(400)));
            emit adviceChanged();
        },
        true);
}
