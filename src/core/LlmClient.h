#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QString>
#include <functional>

class QNetworkAccessManager;
class QNetworkReply;

/// OpenAI-compatible client (llama.cpp /v1), same contract as local-agent/llm_client.py.
class LlmClient : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("LlmClient is owned by App")

    Q_PROPERTY(QString baseUrl READ baseUrl WRITE setBaseUrl NOTIFY configChanged)
    Q_PROPERTY(QString model READ model WRITE setModel NOTIFY configChanged)
    Q_PROPERTY(QString apiKey READ apiKey WRITE setApiKey NOTIFY configChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(bool reachable READ reachable NOTIFY statusChanged)
    Q_PROPERTY(QString statusMessage READ statusMessage NOTIFY statusChanged)
    Q_PROPERTY(bool configured READ configured NOTIFY configChanged)
    Q_PROPERTY(QString streamBuffer READ streamBuffer NOTIFY streamBufferChanged)
    Q_PROPERTY(bool streaming READ streaming NOTIFY streamingChanged)

public:
    using CompleteCallback = std::function<void(bool ok, const QString &textOrError)>;
    using StreamCallback = std::function<void(const QString &chunk)>;

    explicit LlmClient(QObject *parent = nullptr);

    QString baseUrl() const;
    void setBaseUrl(const QString &url);
    QString model() const;
    void setModel(const QString &model);
    QString apiKey() const;
    void setApiKey(const QString &key);
    bool busy() const;
    bool reachable() const;
    QString statusMessage() const;
    bool configured() const;
    QString streamBuffer() const;
    bool streaming() const;

    void loadFromEnvironment();
    void persistSettings() const;
    Q_INVOKABLE void probe();
    Q_INVOKABLE void cancelStream();
    void complete(const QString &systemPrompt, const QString &userPrompt, int maxTokens,
                  CompleteCallback callback);
    /// @param contentOnly when true, ignore reasoning_content/reasoning deltas (mentor UI).
    void completeStream(const QString &systemPrompt, const QString &userPrompt, int maxTokens,
                        StreamCallback onChunk, CompleteCallback onDone, bool contentOnly = false);

signals:
    void configChanged();
    void busyChanged();
    void statusChanged();
    void streamBufferChanged();
    void streamingChanged();
    void streamChunk(const QString &chunk);

private:
    QString normalizeBase(const QString &url) const;
    bool urlAllowed(const QString &base) const;
    bool maySendApiKey(const QString &base) const;
    QNetworkReply *get(const QUrl &url);
    QNetworkReply *postJson(const QUrl &url, const QByteArray &body);
    void setBusy(bool busy);
    void setStatus(bool reachable, const QString &message);

    QNetworkAccessManager *m_nam = nullptr;
    QString m_baseUrl;
    QString m_model;
    QString m_apiKey;
    bool m_busy = false;
    bool m_reachable = false;
    QString m_statusMessage;
    int m_inflight = 0;
    QString m_streamBuffer;
    bool m_streaming = false;
    QNetworkReply *m_streamReply = nullptr;
    QByteArray m_sseCarry;
};
