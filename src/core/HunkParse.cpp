#include "HunkParse.h"

ParsedDiff parseUnifiedDiff(const QString &diffText)
{
    ParsedDiff out;
    if (diffText.trimmed().isEmpty()) {
        out.error = QStringLiteral("empty diff");
        return out;
    }

    DiffHunk current;
    bool inHunk = false;
    QString pendingPre;
    QString pendingPost;
    QString pendingPath;

    const auto flush = [&]() {
        if (inHunk && !current.header.isEmpty()) {
            current.filePath = pendingPath;
            current.preImage = pendingPre;
            current.postImage = pendingPost;
            out.hunks.push_back(current);
        }
        current = DiffHunk{};
        inHunk = false;
    };

    const QStringList lines = diffText.split(QLatin1Char('\n'));
    for (const QString &line : lines) {
        if (line.startsWith(QLatin1String("diff --git "))) {
            flush();
            // diff --git a/path b/path
            const QString rest = line.mid(11).trimmed();
            const int sp = rest.indexOf(QLatin1Char(' '));
            QString b = sp >= 0 ? rest.mid(sp + 1) : rest;
            if (b.startsWith(QLatin1String("b/")))
                b = b.mid(2);
            pendingPath = b;
            pendingPre.clear();
            pendingPost.clear();
            continue;
        }
        if (line.startsWith(QLatin1String("--- "))) {
            pendingPre = line;
            QString p = line.mid(4).trimmed();
            if (p.startsWith(QLatin1String("a/")))
                p = p.mid(2);
            if (p != QLatin1String("/dev/null") && !p.isEmpty())
                pendingPath = p;
            continue;
        }
        if (line.startsWith(QLatin1String("+++ "))) {
            pendingPost = line;
            QString p = line.mid(4).trimmed();
            if (p.startsWith(QLatin1String("b/")))
                p = p.mid(2);
            if (p != QLatin1String("/dev/null") && !p.isEmpty())
                pendingPath = p;
            continue;
        }
        if (line.startsWith(QLatin1String("@@ "))) {
            flush();
            current.header = line;
            current.filePath = pendingPath;
            current.preImage = pendingPre;
            current.postImage = pendingPost;
            inHunk = true;
            continue;
        }
        if (inHunk) {
            if (line.startsWith(QLatin1String("diff --git "))) {
                // handled above
            } else if (!line.startsWith(QLatin1String("\\"))) {
                current.lines.append(line);
            }
        }
    }
    flush();

    if (out.hunks.isEmpty())
        out.error = QStringLiteral("no hunks");
    return out;
}

QString buildPatchFromHunks(const QVector<DiffHunk> &hunks, bool selectedOnly)
{
    QStringList out;
    QString lastPath;
    for (const DiffHunk &h : hunks) {
        if (selectedOnly && !h.selected)
            continue;
        if (h.filePath != lastPath) {
            lastPath = h.filePath;
            out << QStringLiteral("diff --git a/%1 b/%1").arg(h.filePath);
            out << (h.preImage.isEmpty() ? QStringLiteral("--- a/%1").arg(h.filePath) : h.preImage);
            out << (h.postImage.isEmpty() ? QStringLiteral("+++ b/%1").arg(h.filePath) : h.postImage);
        }
        out << h.header;
        out += h.lines;
    }
    if (out.isEmpty())
        return {};
    return out.join(QLatin1Char('\n')) + QLatin1Char('\n');
}
