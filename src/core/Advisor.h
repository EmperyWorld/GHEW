#pragma once

#include "RecommendationModel.h"

#include <QElapsedTimer>
#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <QStringList>
#include <QVariantList>

class CommandCatalog;
class GithubClient;
class LlmClient;
class RepoContext;

class Advisor : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Advisor is owned by App")

    Q_PROPERTY(RecommendationModel *recommendations READ recommendations CONSTANT)
    Q_PROPERTY(QString lastAdvice READ lastAdvice NOTIFY adviceChanged)
    Q_PROPERTY(bool advising READ advising NOTIFY advisingChanged)
    Q_PROPERTY(bool enriching READ enriching NOTIFY enrichingChanged)
    Q_PROPERTY(QString lastError READ lastError NOTIFY adviceChanged)
    Q_PROPERTY(QString askUserPrompt READ askUserPrompt NOTIFY adviceChanged)
    Q_PROPERTY(QStringList askUserOptions READ askUserOptions NOTIFY adviceChanged)
    Q_PROPERTY(bool hasAskUser READ hasAskUser NOTIFY adviceChanged)
    Q_PROPERTY(QVariantList suggestedCommands READ suggestedCommands NOTIFY adviceChanged)
    Q_PROPERTY(QString activeTask READ activeTask NOTIFY adviceChanged)

public:
    Advisor(RepoContext *repo, CommandCatalog *catalog, LlmClient *llm, QObject *parent = nullptr);

    RecommendationModel *recommendations();
    QString lastAdvice() const;
    bool advising() const;
    bool enriching() const;
    QString lastError() const;
    QString askUserPrompt() const;
    QStringList askUserOptions() const;
    bool hasAskUser() const;
    QVariantList suggestedCommands() const;
    QString activeTask() const;

    void setGithub(GithubClient *github);

    Q_INVOKABLE void refresh();
    /// Start a new mentor task (clears prior transcript).
    Q_INVOKABLE void askAdvice(const QString &question);
    /// Continue the current task — free-text answer to ask_user, or a follow-up message.
    Q_INVOKABLE void replyMentor(const QString &text);
    /// Same task, new snapshot, after a command the advisor itself launched.
    void continueAfterCommand(const QString &commandId, const QString &cmdline, int exitCode,
                              const QString &outputTail, const QString &delta);
    /// Continue mentor dialogue after the user picks an ask_user option chip.
    Q_INVOKABLE void answerAskUser(const QString &option);
    /// Apply a canned mentor reply (tests / offline fixtures) without calling the LLM.
    Q_INVOKABLE void applyMentorReplyText(const QString &raw);
    /// Consume free-text the student just typed (for gh_file body when RUN omitted it).
    QString takePendingStudentReply();
    QString takePendingDressKind();
    QString pendingDressKind() const;
    /// Manual enrich (LLM+). Never called automatically on startup/probe.
    Q_INVOKABLE void enrichWithLlm();
    Q_INVOKABLE void clearAskUser();
    /// Drop active task, ask_user, and suggested RUN buttons — start fresh.
    Q_INVOKABLE void clearDialogue();

signals:
    void adviceChanged();
    void advisingChanged();
    void enrichingChanged();
    void selectCommandRequested(const QString &commandId);
    void runCommandRequested(const QString &commandId);

private:
    QVector<Recommendation> buildRuleRecommendations() const;
    QString repoSnapshot() const;
    QString statusFingerprint() const;
    void parseLlmRecommendations(const QString &text);
    void runEnrich(bool force);
    void clearStaleOfflineError();
    void clearAskUserInternal();
    void applyMentorReply(const QString &raw);
    void remember(const QString &line);
    void startTurn(const QString &userExtra, bool newTask);

    RepoContext *m_repo = nullptr;
    GithubClient *m_github = nullptr;
    CommandCatalog *m_catalog = nullptr;
    LlmClient *m_llm = nullptr;
    RecommendationModel m_recommendations;
    QString m_lastAdvice;
    QString m_lastError;
    QString m_askUserPrompt;
    QStringList m_askUserOptions;
    QVariantList m_suggestedCommands;
    QString m_lastStudentQuestion;
    QString m_task;
    QStringList m_transcript;
    /// Free-text answer waiting to become a dress-file body / config payload.
    QString m_pendingStudentReply;
    QString m_pendingDressKind;
    bool m_advising = false;
    bool m_enriching = false;
    QString m_lastEnrichFingerprint;
    QElapsedTimer m_enrichCooldown;

    static QString guessDressKindFromPrompt(const QString &prompt);
};
