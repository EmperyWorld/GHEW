#include "EphemeralRepoPath.h"

#include <QDir>
#include <QFileInfo>
#include <QStandardPaths>

bool isEphemeralRepoPath(const QString &path)
{
    const QString cleaned = QDir::cleanPath(path);
    if (cleaned.isEmpty())
        return true;

    static const char *markers[] = {
        "/git-lab-conflict-",
        "/git-lab-init-",
        "/git-lab-lessons/",
        "/ghew-conflict-",
        "/ghew-init-",
        "/ghew-lessons/",
    };
    for (const char *m : markers) {
        if (cleaned.contains(QLatin1String(m)))
            return true;
    }

    const QFileInfo fi(cleaned);
    const QString base = fi.fileName();
    if (base.startsWith(QLatin1String("git-lab-conflict-"))
        || base.startsWith(QLatin1String("git-lab-init-"))
        || base.startsWith(QLatin1String("ghew-conflict-"))
        || base.startsWith(QLatin1String("ghew-init-"))
        || base.startsWith(QLatin1String("lesson-"))) {
        const QString tmp = QDir::cleanPath(
            QStandardPaths::writableLocation(QStandardPaths::TempLocation));
        const QString cache = QDir::cleanPath(
            QStandardPaths::writableLocation(QStandardPaths::CacheLocation));
        if ((!tmp.isEmpty() && cleaned.startsWith(tmp))
            || (!cache.isEmpty() && cleaned.startsWith(cache)))
            return true;
    }

    if (cleaned.startsWith(QLatin1String("/tmp/git-lab-"))
        || cleaned.startsWith(QLatin1String("/tmp/ghew-")))
        return true;

    return false;
}
