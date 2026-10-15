#include "GitRunner.h"

#include <QProcess>

QString GitRunResult::combinedOutput() const
{
    QString out = QString::fromUtf8(stdoutBytes);
    const QString err = QString::fromUtf8(stderrBytes);
    if (!err.isEmpty()) {
        if (!out.isEmpty() && !out.endsWith(QLatin1Char('\n'))) {
            out += QLatin1Char('\n');
        }
        out += err;
    }
    return out;
}

GitRunner::GitRunner(int timeoutMs)
    : m_timeoutMs(timeoutMs)
{
}

GitRunResult GitRunner::run(const QString &workingDirectory, const QStringList &args) const
{
    GitRunResult result;

    if (workingDirectory.isEmpty()) {
        result.error = QStringLiteral("working directory is empty");
        return result;
    }
    if (args.isEmpty()) {
        result.error = QStringLiteral("git argv is empty");
        return result;
    }

    QProcess process;
    process.setWorkingDirectory(workingDirectory);
    process.setProcessChannelMode(QProcess::SeparateChannels);
    process.start(QStringLiteral("git"), args);

    if (!process.waitForStarted(5000)) {
        result.error = QStringLiteral("failed to start git: %1").arg(process.errorString());
        return result;
    }

    if (!process.waitForFinished(m_timeoutMs)) {
        process.kill();
        process.waitForFinished(2000);
        result.error = QStringLiteral("git timed out after %1 ms").arg(m_timeoutMs);
        return result;
    }

    result.stdoutBytes = process.readAllStandardOutput();
    result.stderrBytes = process.readAllStandardError();
    result.exitCode = process.exitCode();
    result.ok = process.exitStatus() == QProcess::NormalExit && result.exitCode == 0;
    if (!result.ok && result.error.isEmpty()) {
        result.error = QString::fromUtf8(result.stderrBytes).trimmed();
        if (result.error.isEmpty()) {
            result.error = QStringLiteral("git exited with code %1").arg(result.exitCode);
        }
    }
    return result;
}
