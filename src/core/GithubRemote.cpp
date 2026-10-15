#include "GithubRemote.h"

#include <QRegularExpression>

GithubRemote parseGithubRemote(const QString &url)
{
    QString u = url.trimmed();
    while (u.endsWith(QLatin1Char('/')))
        u.chop(1);
    if (u.endsWith(QLatin1String(".git"), Qt::CaseInsensitive))
        u.chop(4);

    const QRegularExpression https(
        QStringLiteral(R"(^https?://([^/\s]+)/([^/\s]+)/([^/\s]+)$)"));
    const QRegularExpression scp(QStringLiteral(R"(^git@([^:\s]+):([^/\s]+)/([^/\s]+)$)"));
    const QRegularExpression ssh(QStringLiteral(R"(^ssh://(?:git@)?([^/\s]+)/([^/\s]+)/([^/\s]+)$)"));

    QString host;
    QString owner;
    QString name;
    QRegularExpressionMatch match = https.match(u);
    if (!match.hasMatch())
        match = scp.match(u);
    if (!match.hasMatch())
        match = ssh.match(u);
    if (match.hasMatch()) {
        host = match.captured(1);
        owner = match.captured(2);
        name = match.captured(3);
    }

    GithubRemote remote;
    if (host.isEmpty() || !host.contains(QLatin1String("github"), Qt::CaseInsensitive))
        return remote;
    if (owner.isEmpty() || name.isEmpty() || owner.contains(QLatin1Char(' '))
        || name.contains(QLatin1Char(' ')))
        return remote;
    remote.ok = true;
    remote.host = host;
    remote.owner = owner;
    remote.name = name;
    return remote;
}
