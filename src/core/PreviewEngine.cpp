#include "PreviewEngine.h"

#include <QCryptographicHash>
#include <QUuid>

PreviewEngine::PreviewEngine(GitRunner *runner)
    : m_runner(runner)
{
}

PreviewResult PreviewEngine::build(const CommandSpec &spec, const QVariantMap &args, const QString &repoRoot) const
{
    PreviewResult result;
    result.argv = buildArgv(spec, args);
    if (result.argv.isEmpty()) {
        result.error = QStringLiteral("empty argv");
        return result;
    }

    result.program = spec.runner.isEmpty() ? QStringLiteral("git") : spec.runner;
    result.cmdline = result.program + QLatin1Char(' ') + result.argv.join(QLatin1Char(' '));
    result.confirmToken = makeToken(result.argv);

    if (result.program != QLatin1String("git")) {
        QString narrative = QStringLiteral("Будет выполнено в `%1`:\n\n`%2`\n\n").arg(repoRoot, result.cmdline);
        if (result.program == QLatin1String("gh")) {
            narrative += QStringLiteral(
                "Это GitHub CLI от имени текущего входа `gh`. Токен в GHEW не сохраняется.\n");
        } else if (result.program == QLatin1String("file")) {
            narrative += QStringLiteral(
                "Будет создан локальный файл. Существующий README или LICENSE не перезаписывается.\n");
        }
        switch (spec.danger) {
        case DangerLevel::Safe:
            narrative += QStringLiteral("\nКоманда помечена как безопасная.");
            break;
        case DangerLevel::Mutates:
            narrative += QStringLiteral("\nКоманда меняет репозиторий или его карточку на GitHub.");
            break;
        case DangerLevel::Destructive:
            narrative += QStringLiteral("\n**Опасная операция.**");
            break;
        }
        result.narrative = narrative;
        result.ok = true;
        return result;
    }

    QString narrative = QStringLiteral("Будет выполнено в `%1`:\n\n`%2`\n\n")
                            .arg(repoRoot, result.cmdline);
    narrative += strategyNarrative(spec, result.argv, repoRoot);

    switch (spec.danger) {
    case DangerLevel::Safe:
        narrative += QStringLiteral("\n\nКоманда помечена как безопасная (не меняет историю/файлы намеренно).");
        break;
    case DangerLevel::Mutates:
        narrative += QStringLiteral("\n\nКоманда изменит состояние репозитория (индекс, коммиты, ветки и т.п.).");
        break;
    case DangerLevel::Destructive:
        narrative += QStringLiteral("\n\n**Опасная операция:** возможна потеря незакоммиченных данных или переписывание истории.");
        break;
    }

    result.narrative = narrative;
    result.ok = true;
    return result;
}

QString PreviewEngine::runText(const QString &repoRoot, const QStringList &args) const
{
    if (!m_runner) {
        return {};
    }
    const GitRunResult r = m_runner->run(repoRoot, args);
    QString text = QString::fromUtf8(r.stdoutBytes).trimmed();
    if (!r.ok && text.isEmpty()) {
        text = QString::fromUtf8(r.stderrBytes).trimmed();
    }
    return text;
}

QString PreviewEngine::strategyNarrative(const CommandSpec &spec, const QStringList &argv, const QString &repoRoot) const
{
    const QString strategy = spec.previewStrategy.isEmpty() ? QStringLiteral("generic") : spec.previewStrategy;
    const QString status = runText(repoRoot, {QStringLiteral("status"), QStringLiteral("-sb")});

    if (strategy == QLatin1String("status")) {
        return QStringLiteral("Будет показан статус рабочей копии. Состояние репозитория не изменится.\n\nСейчас:\n```\n%1\n```")
            .arg(status.isEmpty() ? QStringLiteral("(пусто)") : status);
    }

    if (strategy == QLatin1String("diff")) {
        const QString diff = runText(repoRoot, {QStringLiteral("diff"), QStringLiteral("--stat")});
        return QStringLiteral("Будет показан diff незакоммиченных (unstaged) изменений.\n\nСейчас --stat:\n```\n%1\n```")
            .arg(diff.isEmpty() ? QStringLiteral("(нет отличий)") : diff);
    }

    if (strategy == QLatin1String("add")) {
        const QString porcelain = runText(repoRoot, {QStringLiteral("status"), QStringLiteral("--porcelain")});
        return QStringLiteral("В индекс попадут изменения из рабочей копии (см. список ниже).\n\n```\n%1\n```")
            .arg(porcelain.isEmpty() ? QStringLiteral("(нечего добавлять)") : porcelain);
    }

    if (strategy == QLatin1String("commit")) {
        const QString staged = runText(repoRoot, {QStringLiteral("diff"), QStringLiteral("--cached"), QStringLiteral("--stat")});
        QString message;
        const int mIdx = argv.indexOf(QStringLiteral("-m"));
        if (mIdx >= 0 && mIdx + 1 < argv.size()) {
            message = argv.at(mIdx + 1);
        }
        return QStringLiteral("Будет создан коммит%1.\n\nВ индексе сейчас:\n```\n%2\n```")
            .arg(message.isEmpty() ? QString() : QStringLiteral(" с сообщением «%1»").arg(message),
                 staged.isEmpty() ? QStringLiteral("(индекс пуст — commit, скорее всего, не пройдёт)") : staged);
    }

    if (strategy == QLatin1String("branch_switch")) {
        const QString branch = runText(repoRoot, {QStringLiteral("branch"), QStringLiteral("--show-current")});
        QString target;
        for (int i = 0; i < argv.size(); ++i) {
            if ((argv.at(i) == QLatin1String("switch") || argv.at(i) == QLatin1String("checkout"))
                && i + 1 < argv.size()) {
                target = argv.at(i + 1);
                if (target.startsWith(QLatin1Char('-'))) {
                    target.clear();
                    continue;
                }
                break;
            }
        }
        return QStringLiteral("Текущая ветка: `%1`.\nПосле выполнения HEAD будет на: `%2`.\n\nСтатус:\n```\n%3\n```")
            .arg(branch.isEmpty() ? QStringLiteral("(detached)") : branch,
                 target.isEmpty() ? QStringLiteral("?") : target,
                 status);
    }

    if (strategy == QLatin1String("reset")) {
        const QString dirty = runText(repoRoot, {QStringLiteral("status"), QStringLiteral("--porcelain")});
        const bool hard = argv.contains(QStringLiteral("--hard"));
        QString text = QStringLiteral("Будет выполнен reset");
        if (hard) {
            text += QStringLiteral(" **--hard**: рабочее дерево и индекс будут приведены к целевому коммиту; "
                                   "незакоммиченные изменения пропадут.");
        } else if (argv.contains(QStringLiteral("--soft"))) {
            text += QStringLiteral(" --soft: HEAD сдвинется, индекс и рабочее дерево останутся.");
        } else {
            text += QStringLiteral(" (mixed по умолчанию): HEAD и индекс изменятся, рабочее дерево — нет.");
        }
        text += QStringLiteral("\n\nСейчас незакоммичено:\n```\n%1\n```")
                    .arg(dirty.isEmpty() ? QStringLiteral("(чисто)") : dirty);
        return text;
    }

    if (strategy == QLatin1String("push")) {
        const QString ahead = runText(repoRoot,
                                      {QStringLiteral("status"), QStringLiteral("-sb")});
        const QString dry = runText(repoRoot, QStringList(argv) << QStringLiteral("--dry-run"));
        return QStringLiteral("Будет отправлено на remote (сначала dry-run).\n\nВетка/статус:\n```\n%1\n```\n\n"
                              "`--dry-run`:\n```\n%2\n```")
            .arg(ahead.isEmpty() ? QStringLiteral("(нет данных)") : ahead,
                 dry.isEmpty() ? QStringLiteral("(dry-run без вывода или недоступен)") : dry);
    }

    if (strategy == QLatin1String("clean")) {
        QStringList dryArgs = argv;
        if (!dryArgs.contains(QStringLiteral("-n")) && !dryArgs.contains(QStringLiteral("--dry-run"))) {
            dryArgs.append(QStringLiteral("-n"));
        }
        const QString dry = runText(repoRoot, dryArgs);
        return QStringLiteral("Будут удалены неотслеживаемые файлы. Список (dry-run):\n```\n%1\n```")
            .arg(dry.isEmpty() ? QStringLiteral("(нечего удалять)") : dry);
    }

    if (strategy == QLatin1String("merge")) {
        return QStringLiteral("Будет merge/rebase относительно текущей ветки.\n\nСтатус:\n```\n%1\n```\n"
                              "При конфликтах git остановится и попросит разрешить их вручную.")
            .arg(status.isEmpty() ? QStringLiteral("(чисто)") : status);
    }

    if (strategy == QLatin1String("stash")) {
        const QString list = runText(repoRoot, {QStringLiteral("stash"), QStringLiteral("list")});
        return QStringLiteral("Операция со stash.\n\nТекущий список stash:\n```\n%1\n```\n\nСтатус:\n```\n%2\n```")
            .arg(list.isEmpty() ? QStringLiteral("(пусто)") : list, status);
    }

    return QStringLiteral("Текущий статус репозитория:\n```\n%1\n```")
        .arg(status.isEmpty() ? QStringLiteral("(чисто или нет данных)") : status);
}

QStringList PreviewEngine::buildArgv(const CommandSpec &spec, const QVariantMap &args) const
{
    QStringList argv = spec.gitBase;

    for (const CommandParam &param : spec.params) {
        const QVariant value = args.contains(param.name) ? args.value(param.name) : param.defaultValue;

        if (param.type == QLatin1String("bool")) {
            if (value.toBool() && !param.flag.isEmpty()) {
                if (param.flag.startsWith(QLatin1Char('-')) && !param.flag.startsWith(QLatin1String("--"))
                    && param.flag.size() > 2) {
                    for (int i = 1; i < param.flag.size(); ++i) {
                        argv << (QStringLiteral("-") + param.flag.at(i));
                    }
                } else {
                    argv << param.flag;
                }
            }
            continue;
        }

        const QString text = value.toString();
        if (text.isEmpty() && !(param.keepEmpty && args.contains(param.name))) {
            continue;
        }
        if (!param.flag.isEmpty()) {
            argv << param.flag;
        }
        // Multi-path: newline-separated list (UI multi-select).
        if (param.type == QLatin1String("path") && text.contains(QLatin1Char('\n'))) {
            const QStringList parts = text.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
            for (const QString &p : parts)
                argv << p.trimmed();
            continue;
        }
        argv << text;
    }

    return argv;
}

QString PreviewEngine::makeToken(const QStringList &argv) const
{
    const QByteArray payload = argv.join(QLatin1Char('\0')).toUtf8() + QUuid::createUuid().toByteArray();
    return QString::fromLatin1(QCryptographicHash::hash(payload, QCryptographicHash::Sha256).toHex());
}
