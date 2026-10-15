#pragma once

#include <QString>

struct GithubRemote {
    bool ok = false;
    QString host;
    QString owner;
    QString name;
};

/// Accepts the usual GitHub fetch URLs. Other hosts return ok=false.
GithubRemote parseGithubRemote(const QString &url);
