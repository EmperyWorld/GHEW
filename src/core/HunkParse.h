#pragma once

#include <QString>
#include <QStringList>
#include <QVector>

struct DiffHunk {
    QString filePath;
    QString header;      // @@ -a,b +c,d @@ …
    QStringList lines;   // body including leading space/+/-
    QString preImage;    // --- a/…
    QString postImage;   // +++ b/…
    bool selected = true;
};

struct ParsedDiff {
    QVector<DiffHunk> hunks;
    QString error;
};

/// Parse unified `git diff` output into selectable hunks.
ParsedDiff parseUnifiedDiff(const QString &diffText);

/// Build a patch suitable for `git apply --cached` from selected hunks.
QString buildPatchFromHunks(const QVector<DiffHunk> &hunks, bool selectedOnly = true);
