#pragma once

#include <QString>
#include <QStringList>
#include <QByteArray>

struct GitRunResult {
    bool ok = false;
    int exitCode = -1;
    QByteArray stdoutBytes;
    QByteArray stderrBytes;
    QString error;

    QString combinedOutput() const;
};

class GitRunner
{
public:
    explicit GitRunner(int timeoutMs = 30000);

    GitRunResult run(const QString &workingDirectory, const QStringList &args) const;

private:
    int m_timeoutMs;
};
