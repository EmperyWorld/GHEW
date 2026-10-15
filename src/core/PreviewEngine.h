#pragma once

#include "CommandSpec.h"
#include "GitRunner.h"

#include <QString>
#include <QStringList>
#include <QVariantMap>

struct PreviewResult {
    bool ok = false;
    QString error;
    QString program = QStringLiteral("git");
    QString cmdline;
    QString narrative;
    QString confirmToken;
    QStringList argv;
};

class PreviewEngine
{
public:
    explicit PreviewEngine(GitRunner *runner);

    PreviewResult build(const CommandSpec &spec, const QVariantMap &args, const QString &repoRoot) const;

private:
    QStringList buildArgv(const CommandSpec &spec, const QVariantMap &args) const;
    QString makeToken(const QStringList &argv) const;
    QString runText(const QString &repoRoot, const QStringList &args) const;
    QString strategyNarrative(const CommandSpec &spec, const QStringList &argv, const QString &repoRoot) const;

    GitRunner *m_runner = nullptr;
};
