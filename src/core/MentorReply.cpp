#include "MentorReply.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QRegularExpression>

namespace {

bool looksLikeMetaLine(const QString &line)
{
    const QString t = line.trimmed();
    if (t.isEmpty())
        return false;

    static const QRegularExpression metaRe(
        QStringLiteral(
            "^(?:"
            "Self-Correction\\s*:|"
            "Final\\s*Polish\\s*:|"
            "User\\s*\\(student\\)\\s*says|"
            "Current State of the Repo|"
            "Next Logical Steps|"
            "Observations\\s*:|"
            "Context\\s*:|"
            "Acknowledge the state|"
            "Option\\s*\\d\\s*:|"
            "Keep it calm|"
            "Brief\\s*\\(max|"
            "Use the provided IDs|"
            "Don't ask for dangerous|"
            "Max\\s*12\\s*sentences|"
            "Russian\\s*\\?|"
            "IDs used\\s*\\?|"
            "Explain why\\s*\\?|"
            "No dangerous commands|"
            "No made-up state"
            ")"),
        QRegularExpression::CaseInsensitiveOption);
    if (metaRe.match(t).hasMatch())
        return true;

    // Bullet + English checklist / draft
    static const QRegularExpression bulletMeta(
        QStringLiteral(
            "^[•\\-\\*]\\s*(?:"
            "Max\\s*12|Russian\\s*\\?|IDs used|Explain why|No dangerous|No made-up|"
            "Acknowledge|Option\\s*\\d|Keep it calm|Brief\\s*\\(|Use the provided|"
            "Don't ask|Self-Correction|Final\\s*Polish"
            ")"),
        QRegularExpression::CaseInsensitiveOption);
    if (bulletMeta.match(t).hasMatch())
        return true;

    // "…? Yes." checklist answers
    static const QRegularExpression yesCheck(
        QStringLiteral("\\?\\s*Yes(?:\\s*\\([^)]*\\))?\\.?\\s*$"),
        QRegularExpression::CaseInsensitiveOption);
    if (yesCheck.match(t).hasMatch())
        return true;

    // Tool status noise: "diff - ok."
    static const QRegularExpression toolOk(
        QStringLiteral("^[a-z_][a-z0-9_]*\\s*-\\s*ok\\.?\\s*$"),
        QRegularExpression::CaseInsensitiveOption);
    if (toolOk.match(t).hasMatch())
        return true;

    return false;
}

QVector<MentorCommand> takeRunBlock(QString *text)
{
    QVector<MentorCommand> commands;
    if (!text)
        return commands;
    static const QRegularExpression block(
        QStringLiteral("<<<RUN>>>\\s*([\\s\\S]*?)\\s*<<<END_RUN>>>"),
        QRegularExpression::CaseInsensitiveOption);
    const auto m = block.match(*text);
    if (!m.hasMatch())
        return commands;
    const QString payload = m.captured(1).trimmed();

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(payload.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError) {
        // Keep the broken block visible so the student sees the mentor failed to emit a button.
        return commands;
    }
    text->remove(m.capturedStart(), m.capturedLength());

    const auto appendObject = [&](const QJsonObject &o) {
        const QString id = o.value(QStringLiteral("id")).toString().trimmed();
        if (id.isEmpty())
            return;
        MentorCommand cmd;
        cmd.id = id;
        const QJsonObject args = o.value(QStringLiteral("args")).toObject();
        for (auto it = args.begin(); it != args.end(); ++it)
            cmd.args.insert(it.key(), it.value().toVariant());
        commands.push_back(cmd);
    };

    if (doc.isObject()) {
        const QJsonObject root = doc.object();
        if (root.contains(QStringLiteral("commands"))) {
            const QJsonArray arr = root.value(QStringLiteral("commands")).toArray();
            for (const QJsonValue &v : arr) {
                if (v.isObject())
                    appendObject(v.toObject());
            }
        } else {
            appendObject(root);
        }
    }
    return commands;
}

void collectBacktickCommands(const QString &text, QVector<MentorCommand> *commands)
{
    if (!commands)
        return;
    static const QRegularExpression idRe(QStringLiteral("`([a-z][a-z0-9_]*)`"));
    auto it = idRe.globalMatch(text);
    while (it.hasNext()) {
        const QString id = it.next().captured(1);
        bool exists = false;
        for (const MentorCommand &c : *commands) {
            if (c.id == id) {
                exists = true;
                break;
            }
        }
        if (exists)
            continue;
        MentorCommand cmd;
        cmd.id = id;
        commands->push_back(cmd);
        if (commands->size() >= 4)
            return;
    }
}

QString stripAskUserBlock(QString text, MentorAskUser *out)
{
    static const QRegularExpression block(
        QStringLiteral(
            "<<<ASK_USER>>>\\s*([\\s\\S]*?)\\s*<<<END_ASK_USER>>>"),
        QRegularExpression::CaseInsensitiveOption);
    const auto m = block.match(text);
    if (!m.hasMatch())
        return text;

    const QString payload = m.captured(1).trimmed();
    text.remove(m.capturedStart(), m.capturedLength());

    if (!out)
        return text;

    QJsonParseError err;
    const QJsonDocument doc = QJsonDocument::fromJson(payload.toUtf8(), &err);
    if (err.error != QJsonParseError::NoError || !doc.isObject())
        return text;

    const QJsonObject o = doc.object();
    out->prompt = o.value(QStringLiteral("prompt")).toString().trimmed();
    const QJsonArray arr = o.value(QStringLiteral("options")).toArray();
    for (const QJsonValue &v : arr) {
        const QString opt = v.toString().trimmed();
        if (!opt.isEmpty())
            out->options << opt;
    }
    // Cap options for UI
    while (out->options.size() > 6)
        out->options.removeLast();
    return text;
}

QString takeAfterFinalPolish(const QString &raw)
{
    static const QRegularExpression marker(
        QStringLiteral("Final\\s*Polish\\s*:\\s*"),
        QRegularExpression::CaseInsensitiveOption);
    int last = -1;
    int end = -1;
    auto it = marker.globalMatch(raw);
    while (it.hasNext()) {
        const auto m = it.next();
        last = m.capturedStart();
        end = m.capturedEnd();
    }
    if (last < 0)
        return raw;
    return raw.mid(end);
}

QString filterMetaLines(const QString &text)
{
    const QStringList lines = text.split(QLatin1Char('\n'));
    QStringList kept;
    kept.reserve(lines.size());
    for (const QString &line : lines) {
        if (looksLikeMetaLine(line))
            continue;
        kept << line;
    }
    // Collapse excess blank lines
    QStringList compact;
    bool prevBlank = true;
    for (const QString &line : kept) {
        const bool blank = line.trimmed().isEmpty();
        if (blank && prevBlank)
            continue;
        compact << line;
        prevBlank = blank;
    }
    while (!compact.isEmpty() && compact.first().trimmed().isEmpty())
        compact.removeFirst();
    while (!compact.isEmpty() && compact.last().trimmed().isEmpty())
        compact.removeLast();
    return compact.join(QLatin1Char('\n'));
}

QString dedupeRepeatedBody(const QString &text)
{
    const QString t = text.trimmed();
    if (t.size() < 40)
        return t;

    // Exact half-repeat (A + A)
    const int half = t.size() / 2;
    for (int split = half; split >= half - 120 && split > 16; --split) {
        if (t.at(split) != QLatin1Char('\n') && split != half)
            continue;
        const QString a = t.left(split).trimmed();
        const QString b = t.mid(split).trimmed();
        if (!a.isEmpty() && a == b)
            return a;
    }

    // Paragraph duplicate: last paragraph equals an earlier one of similar size
    const QStringList paras = t.split(QRegularExpression(QStringLiteral("\\n\\s*\\n")));
    if (paras.size() >= 2) {
        QStringList unique;
        for (const QString &p : paras) {
            const QString pt = p.trimmed();
            if (pt.isEmpty())
                continue;
            if (!unique.isEmpty() && unique.last() == pt)
                continue;
            unique << pt;
        }
        if (unique.size() < paras.size())
            return unique.join(QStringLiteral("\n\n"));
    }
    return t;
}

void synthesizeAskUserFromAnswer(const QString &answer, MentorAskUser *out)
{
    if (!out || out->isValid())
        return;

    static const QRegularExpression numbered(
        QStringLiteral("^\\s*(\\d+)[.)]\\s+(.+)$"));
    QStringList options;
    QString lastQuestion;
    for (const QString &line : answer.split(QLatin1Char('\n'))) {
        const QString t = line.trimmed();
        const auto m = numbered.match(t);
        if (m.hasMatch()) {
            QString label = m.captured(2).trimmed();
            // Prefer short label: text before em-dash / parenthetical id
            const int cut = label.indexOf(QRegularExpression(QStringLiteral("\\s[—–-]\\s|\\s\\(`")));
            if (cut > 8)
                label = label.left(cut).trimmed();
            if (label.size() > 72)
                label = label.left(69) + QStringLiteral("…");
            if (!label.isEmpty())
                options << label;
        }
        if (t.endsWith(QLatin1Char('?')) && t.size() >= 8)
            lastQuestion = t;
    }
    if (options.size() >= 2 && !lastQuestion.isEmpty()) {
        out->prompt = lastQuestion;
        out->options = options.mid(0, 6);
    }
}

} // namespace

MentorReply parseMentorReply(const QString &raw)
{
    MentorReply out;
    if (raw.trimmed().isEmpty())
        return out;

    MentorAskUser ask;
    QString body = raw;
    QVector<MentorCommand> commands = takeRunBlock(&body);
    body = stripAskUserBlock(body, &ask);
    body = takeAfterFinalPolish(body);
    body = filterMetaLines(body);
    body = dedupeRepeatedBody(body);

    // If still looks like pure English draft with no Cyrillic, try original after polish only
    static const QRegularExpression cyrillic(QStringLiteral("[А-Яа-яЁё]"));
    if (!cyrillic.match(body).hasMatch()) {
        QString fallback = takeAfterFinalPolish(raw);
        fallback = stripAskUserBlock(fallback, nullptr);
        fallback = filterMetaLines(fallback);
        fallback = dedupeRepeatedBody(fallback);
        if (cyrillic.match(fallback).hasMatch())
            body = fallback;
    }

    synthesizeAskUserFromAnswer(body, &ask);

    // Drop trailing bare question from answer when it becomes the ask_user prompt
    if (ask.isValid()) {
        QStringList lines = body.split(QLatin1Char('\n'));
        while (!lines.isEmpty() && lines.last().trimmed().isEmpty())
            lines.removeLast();
        if (!lines.isEmpty() && lines.last().trimmed() == ask.prompt)
            lines.removeLast();
        body = lines.join(QLatin1Char('\n')).trimmed();
    }

    out.answer = body.trimmed();
    out.askUser = ask;
    collectBacktickCommands(out.answer, &commands);
    while (commands.size() > 4)
        commands.removeLast();
    out.commands = commands;
    return out;
}
