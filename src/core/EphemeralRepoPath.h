#pragma once

#include <QString>

/// Smoke/lesson sandboxes that must not appear in the welcome recent list.
bool isEphemeralRepoPath(const QString &path);
