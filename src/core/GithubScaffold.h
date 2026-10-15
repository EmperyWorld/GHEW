#pragma once

#include <QString>

/// Writes README.md only when the tree has no readme yet.
bool writeScaffoldReadme(const QString &repoRoot, const QString &title, const QString &blurb, QString *error);

/// kind: mit | bsd2 | proprietary. Does not overwrite an existing license file.
bool writeScaffoldLicense(const QString &repoRoot, const QString &kind, const QString &owner, int year,
                          QString *error);

QString scaffoldReadmeText(const QString &title, const QString &blurb);
QString scaffoldLicenseText(const QString &kind, const QString &owner, int year);

/// kind: readme | license | gitignore | contributing | conduct | security | support | funding | codeowners | issue | pull.
QStringList dressKinds();
QString dressFileName(const QString &repoRoot, const QString &kind);
bool dressFilePresent(const QString &repoRoot, const QString &kind);
QString dressTemplateText(const QString &kind, const QString &title);
QString readDressFile(const QString &repoRoot, const QString &kind);
/// Overwrites the dressing file. The path is chosen only from the whitelist above.
bool writeDressFile(const QString &repoRoot, const QString &kind, const QString &body, QString *error);
