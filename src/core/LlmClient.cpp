#include "LlmClient.h"

#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QAbstractSocket>
#include <QHostAddress>
#include <QSettings>
#include <QUrl>

namespace {

QString jsonContentToString(const QJsonValue &v)
{
    if (v.isString())
        return v.toString();
    if (v.isArray()) {
        QString out;
        for (const QJsonValue &part : v.toArray()) {
            if (part.isString()) {
                out += part.toString();
            } else if (part.isObject()) {
                const QJsonObject o = part.toObject();
                const QString t = o.value(QStringLiteral("text")).toString();
                if (!t.isEmpty())
                    out += t;
                else
                    out += o.value(QStringLiteral("content")).toString();
            }
        }
        return out;
    }
    return {};
}

/// Gemma / llama.cpp often fill reasoning_content while content stays empty (esp. when
/// finish_reason=length during thinking). Prefer final content, then reasoning fields.
/// Gemma 4 on llama.cpp (`--reasoning auto`) spends the whole budget in
/// reasoning_content and leaves content empty (finish_reason=length). The
/// chat template honours enable_thinking=false and then writes the answer
/// into content — that is what the mentor UI must show.
void disableThinking(QJsonObject &payload)
{
    payload.insert(QStringLiteral("chat_template_kwargs"),
                   QJsonObject{{QStringLiteral("enable_thinking"), false}});
}

QString extractAssistantText(const QJsonObject &messageOrDelta)
{
    QString content = jsonContentToString(messageOrDelta.value(QStringLiteral("content"))).trimmed();
    if (!content.isEmpty())
        return content;

    static const char *keys[] = {"reasoning_content", "reasoning", "thinking", "refusal"};
    for (const char *key : keys) {
        const QString alt = jsonContentToString(messageOrDelta.value(QLatin1String(key))).trimmed();
        if (!alt.isEmpty())
            return alt;
    }
    return {};
}

} // namespace

LlmClient::LlmClient(QObject *parent)
    : QObject(parent)
    , m_nam(new QNetworkAccessManager(this))
    , m_statusMessage(tr("LLM не проверен"))
{
    loadFromEnvironment();
}

void LlmClient::loadFromEnvironment()
{
    const QByteArray envBase = qgetenv("GHEW_LLM_BASE_URL").isEmpty()
                                   ? qgetenv("GIT_LAB_LLM_BASE_URL")
                                   : qgetenv("GHEW_LLM_BASE_URL");
    const QByteArray envModel = qgetenv("GHEW_LLM_MODEL").isEmpty()
                                    ? qgetenv("GIT_LAB_LLM_MODEL")
                                    : qgetenv("GHEW_LLM_MODEL");
    const QByteArray envKey = qgetenv("GHEW_LLM_API_KEY").isEmpty()
                                  ? qgetenv("GIT_LAB_LLM_API_KEY")
                                  : qgetenv("GHEW_LLM_API_KEY");
    const QByteArray legacyBase = qgetenv("LLM_BASE_URL");
    const QByteArray legacyModel = qgetenv("LLM_MODEL");
    const QByteArray legacyKey = qgetenv("LLM_API_KEY");

    QSettings settings;
    const QString settingsBase = settings.value(QStringLiteral("llm/baseUrl")).toString();
    const QString settingsModel = settings.value(QStringLiteral("llm/model")).toString();
    const QString settingsKey = settings.value(QStringLiteral("llm/apiKey")).toString();

    QString base = !envBase.isEmpty() ? QString::fromUtf8(envBase)
                   : (!legacyBase.isEmpty() ? QString::fromUtf8(legacyBase)
                      : (!settingsBase.isEmpty() ? settingsBase : QStringLiteral("http://127.0.0.1:8080/v1")));
    QString model = !envModel.isEmpty() ? QString::fromUtf8(envModel)
                    : (!legacyModel.isEmpty() ? QString::fromUtf8(legacyModel)
                       : (!settingsModel.isEmpty() ? settingsModel : QStringLiteral("gemma-4-26b.gguf")));
    QString key = !envKey.isEmpty() ? QString::fromUtf8(envKey)
                  : (!legacyKey.isEmpty() ? QString::fromUtf8(legacyKey) : settingsKey);

    m_baseUrl = normalizeBase(base);
    m_model = model.trimmed();
    m_apiKey = key.trimmed();
    emit configChanged();
}

void LlmClient::persistSettings() const
{
    QSettings settings;
    settings.setValue(QStringLiteral("llm/baseUrl"), m_baseUrl);
    settings.setValue(QStringLiteral("llm/model"), m_model);
    settings.setValue(QStringLiteral("llm/apiKey"), m_apiKey);
}

QString LlmClient::baseUrl() const
{
    return m_baseUrl;
}

void LlmClient::setBaseUrl(const QString &url)
{
    const QString n = normalizeBase(url);
    if (m_baseUrl == n)
        return;
    m_baseUrl = n;
    m_reachable = false;
    m_statusMessage = tr("Адрес изменён — нажмите «Проверить LLM»");
    emit configChanged();
    emit statusChanged();
}

QString LlmClient::model() const
{
    return m_model;
}

void LlmClient::setModel(const QString &model)
{
    if (m_model == model)
        return;
    m_model = model.trimmed();
    emit configChanged();
}

QString LlmClient::apiKey() const
{
    return m_apiKey;
}

void LlmClient::setApiKey(const QString &key)
{
    if (m_apiKey == key)
        return;
    m_apiKey = key.trimmed();
    emit configChanged();
}

bool LlmClient::busy() const
{
    return m_busy;
}

bool LlmClient::reachable() const
{
    return m_reachable;
}

QString LlmClient::statusMessage() const
{
    return m_statusMessage;
}

bool LlmClient::configured() const
{
    return !m_baseUrl.isEmpty();
}

QString LlmClient::streamBuffer() const
{
    return m_streamBuffer;
}

bool LlmClient::streaming() const
{
    return m_streaming;
}

void LlmClient::cancelStream()
{
    if (m_streamReply) {
        m_streamReply->abort();
        m_streamReply = nullptr;
    }
    if (m_streaming) {
        m_streaming = false;
        emit streamingChanged();
    }
}


QString LlmClient::normalizeBase(const QString &url) const
{
    QString u = url.trimmed();
    if (u.isEmpty())
        return {};
    if (!u.contains(QStringLiteral("://")))
        u.prepend(QStringLiteral("http://"));
    while (u.endsWith(QLatin1Char('/')))
        u.chop(1);
    return u;
}

bool LlmClient::urlAllowed(const QString &base) const
{
    const QUrl url(base);
    if (!url.isValid() || url.scheme().isEmpty())
        return false;
    if (url.scheme() != QLatin1String("http") && url.scheme() != QLatin1String("https"))
        return false;
    const QString host = url.host().toLower();
    if (host.isEmpty())
        return false;
    if (host == QLatin1String("localhost") || host == QLatin1String("127.0.0.1") || host == QLatin1String("::1"))
        return true;
    const QHostAddress addr(host);
    if (addr.isNull()) {
        // hostname — allow only .local / private-looking names used with podman; reject public-looking
        // Resolve not performed here; restrict to clearly local names.
        return host.endsWith(QLatin1String(".local")) || host.endsWith(QLatin1String(".lan"));
    }
    if (addr.isLoopback())
        return true;
    // RFC1918
    if (addr.protocol() == QAbstractSocket::IPv4Protocol) {
        const quint32 ip = addr.toIPv4Address();
        const auto inRange = [](quint32 v, quint32 net, quint32 mask) {
            return (v & mask) == net;
        };
        if (inRange(ip, 0x0A000000u, 0xFF000000u)) // 10.0.0.0/8
            return true;
        if (inRange(ip, 0xAC100000u, 0xFFF00000u)) // 172.16.0.0/12
            return true;
        if (inRange(ip, 0xC0A80000u, 0xFFFF0000u)) // 192.168.0.0/16
            return true;
    }
    return false;
}

bool LlmClient::maySendApiKey(const QString &base) const
{
    return urlAllowed(base); // same local-only policy as local-agent
}

void LlmClient::setBusy(bool busy)
{
    if (m_busy == busy)
        return;
    m_busy = busy;
    emit busyChanged();
}

void LlmClient::setStatus(bool reachable, const QString &message)
{
    if (m_reachable == reachable && m_statusMessage == message)
        return;
    m_reachable = reachable;
    m_statusMessage = message;
    emit statusChanged();
}

QNetworkReply *LlmClient::get(const QUrl &url)
{
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    if (!m_apiKey.isEmpty() && maySendApiKey(m_baseUrl))
        req.setRawHeader("Authorization", QByteArray("Bearer ") + m_apiKey.toUtf8());
    return m_nam->get(req);
}

QNetworkReply *LlmClient::postJson(const QUrl &url, const QByteArray &body)
{
    QNetworkRequest req(url);
    req.setHeader(QNetworkRequest::ContentTypeHeader, QStringLiteral("application/json"));
    if (!m_apiKey.isEmpty() && maySendApiKey(m_baseUrl))
        req.setRawHeader("Authorization", QByteArray("Bearer ") + m_apiKey.toUtf8());
    return m_nam->post(req, body);
}

void LlmClient::probe()
{
    if (m_baseUrl.isEmpty()) {
        setStatus(false, tr("Не задан GHEW_LLM_BASE_URL / LLM_BASE_URL"));
        return;
    }
    if (!urlAllowed(m_baseUrl)) {
        setStatus(false, tr("LLM URL запрещён (только localhost / LAN)"));
        return;
    }

    ++m_inflight;
    setBusy(true);
    const QUrl url(m_baseUrl + QStringLiteral("/models"));
    QNetworkReply *reply = get(url);
    connect(reply, &QNetworkReply::finished, this, [this, reply]() {
        reply->deleteLater();
        --m_inflight;
        if (m_inflight <= 0) {
            m_inflight = 0;
            setBusy(false);
        }

        if (reply->error() != QNetworkReply::NoError) {
            setStatus(false, tr("LLM недоступен: %1").arg(reply->errorString()));
            return;
        }
        const QByteArray raw = reply->readAll();
        const QJsonDocument doc = QJsonDocument::fromJson(raw);
        if (!doc.isObject()) {
            setStatus(false, tr("Некорректный ответ /models"));
            return;
        }
        const QJsonArray data = doc.object().value(QStringLiteral("data")).toArray();
        QStringList ids;
        for (const QJsonValue &v : data) {
            if (v.isObject())
                ids << v.toObject().value(QStringLiteral("id")).toString();
            else if (v.isString())
                ids << v.toString();
        }
        if (ids.isEmpty()) {
            setStatus(false, tr("/models пуст"));
            return;
        }
        // Prefer gemma*26* like local-agent pick_gemma_26
        QString chosen;
        for (const QString &id : ids) {
            const QString low = id.toLower();
            if (low.contains(QLatin1String("gemma")) && low.contains(QLatin1String("26"))) {
                chosen = id;
                break;
            }
        }
        if (chosen.isEmpty())
            chosen = ids.first();
        if (m_model.isEmpty() || !ids.contains(m_model))
            setModel(chosen);
        setStatus(true, tr("LLM онлайн · %1").arg(m_model));
    });
}

void LlmClient::complete(const QString &systemPrompt, const QString &userPrompt, int maxTokens,
                         CompleteCallback callback)
{
    if (!callback)
        return;
    if (m_baseUrl.isEmpty() || m_model.isEmpty()) {
        callback(false, tr("LLM не настроен"));
        return;
    }
    if (!urlAllowed(m_baseUrl)) {
        callback(false, tr("LLM URL запрещён (только localhost / LAN)"));
        return;
    }

    QJsonObject payload;
    payload.insert(QStringLiteral("model"), m_model);
    payload.insert(QStringLiteral("stream"), false);
    payload.insert(QStringLiteral("temperature"), 0.3);
    if (maxTokens > 0)
        payload.insert(QStringLiteral("max_tokens"), maxTokens);
    disableThinking(payload);

    QJsonArray messages;
    if (!systemPrompt.isEmpty()) {
        messages.append(QJsonObject{{QStringLiteral("role"), QStringLiteral("system")},
                                    {QStringLiteral("content"), systemPrompt}});
    }
    messages.append(QJsonObject{{QStringLiteral("role"), QStringLiteral("user")},
                                {QStringLiteral("content"), userPrompt}});
    payload.insert(QStringLiteral("messages"), messages);

    ++m_inflight;
    setBusy(true);
    QNetworkReply *reply = postJson(QUrl(m_baseUrl + QStringLiteral("/chat/completions")),
                                    QJsonDocument(payload).toJson(QJsonDocument::Compact));
    connect(reply, &QNetworkReply::finished, this, [this, reply, callback]() {
        reply->deleteLater();
        --m_inflight;
        if (m_inflight <= 0) {
            m_inflight = 0;
            setBusy(false);
        }

        if (reply->error() != QNetworkReply::NoError) {
            callback(false, tr("Ошибка LLM: %1").arg(reply->errorString()));
            return;
        }
        const QJsonDocument doc = QJsonDocument::fromJson(reply->readAll());
        if (!doc.isObject()) {
            callback(false, tr("Некорректный JSON от chat/completions"));
            return;
        }
        const QJsonObject root = doc.object();
        const QJsonArray choices = root.value(QStringLiteral("choices")).toArray();
        if (choices.isEmpty()) {
            callback(false, tr("Пустой choices от LLM"));
            return;
        }
        const QJsonObject choice = choices.at(0).toObject();
        const QJsonObject message = choice.value(QStringLiteral("message")).toObject();
        QString content = extractAssistantText(message);
        // Rare: some servers put text on the choice itself.
        if (content.isEmpty())
            content = extractAssistantText(choice);
        if (content.isEmpty()) {
            const QString reason = choice.value(QStringLiteral("finish_reason")).toString();
            callback(false,
                     tr("Пустой ответ модели%1")
                         .arg(reason.isEmpty() ? QString()
                                               : tr(" (finish_reason=%1; увеличьте max_tokens)").arg(reason)));
            return;
        }
        callback(true, content);
    });
}

void LlmClient::completeStream(const QString &systemPrompt, const QString &userPrompt, int maxTokens,
                               StreamCallback onChunk, CompleteCallback onDone, bool contentOnly)
{
    if (!onDone)
        return;
    if (m_baseUrl.isEmpty() || m_model.isEmpty()) {
        onDone(false, tr("LLM не настроен"));
        return;
    }
    if (!urlAllowed(m_baseUrl)) {
        onDone(false, tr("LLM URL запрещён (только localhost / LAN)"));
        return;
    }

    cancelStream();
    m_streamBuffer.clear();
    m_sseCarry.clear();
    emit streamBufferChanged();

    QJsonObject payload;
    payload.insert(QStringLiteral("model"), m_model);
    payload.insert(QStringLiteral("stream"), true);
    payload.insert(QStringLiteral("temperature"), 0.3);
    if (maxTokens > 0)
        payload.insert(QStringLiteral("max_tokens"), maxTokens);
    disableThinking(payload);

    QJsonArray messages;
    if (!systemPrompt.isEmpty()) {
        messages.append(QJsonObject{{QStringLiteral("role"), QStringLiteral("system")},
                                    {QStringLiteral("content"), systemPrompt}});
    }
    messages.append(QJsonObject{{QStringLiteral("role"), QStringLiteral("user")},
                                {QStringLiteral("content"), userPrompt}});
    payload.insert(QStringLiteral("messages"), messages);

    ++m_inflight;
    setBusy(true);
    m_streaming = true;
    emit streamingChanged();

    QNetworkReply *reply = postJson(QUrl(m_baseUrl + QStringLiteral("/chat/completions")),
                                    QJsonDocument(payload).toJson(QJsonDocument::Compact));
    m_streamReply = reply;

    connect(reply, &QNetworkReply::readyRead, this, [this, reply, onChunk, contentOnly]() {
        m_sseCarry.append(reply->readAll());
        while (true) {
            const int nl = m_sseCarry.indexOf('\n');
            if (nl < 0)
                break;
            QByteArray line = m_sseCarry.left(nl).trimmed();
            m_sseCarry.remove(0, nl + 1);
            if (line.isEmpty() || !line.startsWith("data:"))
                continue;
            QByteArray data = line.mid(5).trimmed();
            if (data == "[DONE]")
                continue;
            const QJsonDocument doc = QJsonDocument::fromJson(data);
            if (!doc.isObject())
                continue;
            const QJsonArray choices = doc.object().value(QStringLiteral("choices")).toArray();
            if (choices.isEmpty())
                continue;
            const QJsonObject delta = choices.at(0).toObject().value(QStringLiteral("delta")).toObject();
            // Prefer visible content; optionally skip reasoning (mentor must not show CoT).
            QString piece = jsonContentToString(delta.value(QStringLiteral("content")));
            if (piece.isEmpty() && !contentOnly) {
                piece = jsonContentToString(delta.value(QStringLiteral("reasoning_content")));
                if (piece.isEmpty())
                    piece = jsonContentToString(delta.value(QStringLiteral("reasoning")));
            }
            if (piece.isEmpty())
                continue;
            m_streamBuffer += piece;
            emit streamBufferChanged();
            emit streamChunk(piece);
            if (onChunk)
                onChunk(piece);
        }
    });

    connect(reply, &QNetworkReply::finished, this, [this, reply, onDone]() {
        if (m_streamReply == reply)
            m_streamReply = nullptr;
        reply->deleteLater();
        --m_inflight;
        if (m_inflight <= 0) {
            m_inflight = 0;
            setBusy(false);
        }
        const bool wasStreaming = m_streaming;
        m_streaming = false;
        if (wasStreaming)
            emit streamingChanged();

        if (reply->error() != QNetworkReply::NoError && m_streamBuffer.isEmpty()) {
            onDone(false, tr("Ошибка LLM stream: %1").arg(reply->errorString()));
            return;
        }
        if (m_streamBuffer.trimmed().isEmpty()) {
            onDone(false, tr("Пустой stream-ответ модели"));
            return;
        }
        onDone(true, m_streamBuffer);
    });
}
