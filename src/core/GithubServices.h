#pragma once

#include <QString>
#include <QStringList>

/// Argv builders for repository services. They do not run `gh`.
/// Bodies may contain newlines. Titles, names, and ids must not look like flags.

QStringList githubIssueWriteArgv(const QString &number, const QString &title, const QString &body, QString *error);
QStringList githubIssueActionArgv(const QString &number, const QString &action, const QString &extra, QString *error);
QStringList githubIssueLinkArgv(const QString &number, const QString &kind, const QString &value, bool remove,
                                QString *error);

QStringList githubLabelWriteArgv(const QString &name, const QString &color, const QString &description,
                                 const QString &newName, bool edit, QString *error);
QStringList githubLabelDeleteArgv(const QString &name, QString *error);

QStringList githubMilestoneArgv(const QString &number, const QString &title, const QString &state, QString *error);

QStringList githubPullWriteArgv(const QString &number, const QString &title, const QString &body, const QString &base,
                                const QString &head, bool draft, QString *error);
QStringList githubPullReviewArgv(const QString &number, const QString &action, const QString &body, QString *error);
QStringList githubPullMergeArgv(const QString &number, const QString &method, QString *error);
QStringList githubPullReadyArgv(const QString &number, bool draft, QString *error);
QStringList githubPullUpdateArgv(const QString &number, bool rebase, QString *error);

QStringList githubDiscussionWriteArgv(const QString &number, const QString &title, const QString &body,
                                      const QString &category, QString *error);
QStringList githubDiscussionCommentArgv(const QString &target, const QString &body, bool remove, QString *error);

QStringList githubReleaseCreateArgv(const QString &tag, const QString &title, const QString &notes, bool draft,
                                    bool prerelease, const QString &target, QString *error);
QStringList githubReleaseEditArgv(const QString &tag, const QString &title, const QString &notes, bool draft,
                                  bool prerelease, const QString &target, QString *error);
QStringList githubReleaseDeleteArgv(const QString &tag, bool cleanupTag, QString *error);
QStringList githubReleaseUploadArgv(const QString &tag, const QString &filePath, QString *error);

QStringList githubAutolinkCreateArgv(const QString &prefix, const QString &urlTemplate, bool numeric, QString *error);
QStringList githubAutolinkDeleteArgv(const QString &id, QString *error);

QStringList githubProjectCreateArgv(const QString &owner, const QString &title, QString *error);
QStringList githubProjectFieldArgv(const QString &number, const QString &owner, const QString &name,
                                   const QString &dataType, const QString &options, QString *error);
QStringList githubProjectItemArgv(const QString &number, const QString &owner, const QString &title,
                                  const QString &body, QString *error);
QStringList githubProjectAddArgv(const QString &number, const QString &owner, const QString &url, QString *error);
QStringList githubProjectEditArgv(const QString &number, const QString &owner, const QString &url, const QString &field,
                                  const QString &value, QString *error);
QStringList githubProjectLinkArgv(const QString &number, const QString &owner, QString *error);
QStringList githubProjectViewArgv(const QString &projectId, const QString &name, const QString &layout, QString *error);
QStringList githubProjectViewEditArgv(const QString &projectId, const QString &viewId, const QString &name, bool remove,
                                      QString *error);
