#pragma once

#include "StatusTypes.h"

#include <QString>
#include <QVector>

struct PorcelainParseResult {
    QVector<StatusFileEntry> files;
    int stagedCount = 0;
    int unstagedCount = 0;
    int untrackedCount = 0;
    int conflictCount = 0;
};

/// Pure parser for `git status --porcelain` lines (unit-testable).
PorcelainParseResult parsePorcelainStatus(const QString &porcelain);

QString classifyPorcelainState(QChar x, QChar y);
QString summarizePorcelainCode(const QString &code, const QString &state);
