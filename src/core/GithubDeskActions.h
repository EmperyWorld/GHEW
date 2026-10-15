#pragma once

#include <QString>
#include <QVariantMap>

/// Advisor RUN ids for the GitHub cards. These are not catalog commands.
bool githubDeskActionKnown(const QString &id);
QString githubDeskActionTitle(const QString &id);
QString githubDeskActionCatalog();
int githubDeskActionCount();

/// Drop a RUN the advisor must not carry: secret values, webhook secrets, private keys.
/// Returns false when the button should not be shown.
bool githubDeskAcceptAdvisorCommand(const QString &id, QVariantMap *args);
