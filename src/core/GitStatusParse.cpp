#include "GitStatusParse.h"

#include <QObject>

QString classifyPorcelainState(QChar x, QChar y)
{
    if (x == QLatin1Char('U') || y == QLatin1Char('U') || (x == QLatin1Char('A') && y == QLatin1Char('A'))
        || (x == QLatin1Char('D') && y == QLatin1Char('D'))) {
        return QStringLiteral("conflict");
    }
    if (x == QLatin1Char('?') && y == QLatin1Char('?'))
        return QStringLiteral("untracked");
    const bool staged = x != QLatin1Char(' ') && x != QLatin1Char('?');
    const bool unstaged = y != QLatin1Char(' ') && y != QLatin1Char('?');
    if (staged && unstaged)
        return QStringLiteral("mixed");
    if (staged)
        return QStringLiteral("staged");
    if (unstaged)
        return QStringLiteral("unstaged");
    return QStringLiteral("clean");
}

QString summarizePorcelainCode(const QString &code, const QString &state)
{
    if (state == QLatin1String("untracked"))
        return QObject::tr("новый");
    if (state == QLatin1String("conflict"))
        return QObject::tr("конфликт");
    if (code.size() < 2)
        return state;

    const QChar x = code.at(0);
    const QChar y = code.at(1);
    QStringList parts;
    auto map = [](QChar c) -> QString {
        switch (c.toLatin1()) {
        case 'M':
            return QObject::tr("изменён");
        case 'A':
            return QObject::tr("добавлен");
        case 'D':
            return QObject::tr("удалён");
        case 'R':
            return QObject::tr("переименован");
        case 'C':
            return QObject::tr("скопирован");
        case 'T':
            return QObject::tr("тип");
        default:
            return {};
        }
    };
    if (x != QLatin1Char(' ') && x != QLatin1Char('?')) {
        const QString s = map(x);
        if (!s.isEmpty())
            parts << QObject::tr("индекс: %1").arg(s);
    }
    if (y != QLatin1Char(' ') && y != QLatin1Char('?')) {
        const QString s = map(y);
        if (!s.isEmpty())
            parts << QObject::tr("рабочая: %1").arg(s);
    }
    if (parts.isEmpty())
        return state;
    return parts.join(QStringLiteral(" · "));
}

PorcelainParseResult parsePorcelainStatus(const QString &porcelain)
{
    PorcelainParseResult result;
    const QStringList lines = porcelain.split(QLatin1Char('\n'), Qt::SkipEmptyParts);
    for (const QString &line : lines) {
        if (line.size() < 3)
            continue;
        const QString code = line.left(2);
        QString path = line.mid(3);
        const int arrow = path.indexOf(QStringLiteral(" -> "));
        if (arrow >= 0)
            path = path.mid(arrow + 4);

        const QChar x = code.at(0);
        const QChar y = code.at(1);
        StatusFileEntry e;
        e.path = path;
        e.code = code;
        e.state = classifyPorcelainState(x, y);
        e.summary = summarizePorcelainCode(code, e.state);
        result.files.push_back(e);

        if (e.state == QLatin1String("conflict"))
            ++result.conflictCount;
        if (e.state == QLatin1String("untracked"))
            ++result.untrackedCount;
        else {
            if (x != QLatin1Char(' ') && x != QLatin1Char('?'))
                ++result.stagedCount;
            if (y != QLatin1Char(' ') && y != QLatin1Char('?'))
                ++result.unstagedCount;
        }
    }
    return result;
}
