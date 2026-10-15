#include "core/LlmClient.h"

#include <QCoreApplication>
#include <QEventLoop>
#include <QRegularExpression>
#include <QTimer>

#include <cstdio>

#define CHECK(cond)                                                                                                    \
    do {                                                                                                               \
        if (!(cond)) {                                                                                                 \
            std::fprintf(stderr, "FAIL %s:%d: %s\n", __FILE__, __LINE__, #cond);                                       \
            return 1;                                                                                                  \
        }                                                                                                              \
    } while (0)

/// Hits the llama.cpp already running on this machine (127.0.0.1:8080).
/// Not a mock: probe /v1/models, then a real chat completion and a real SSE stream.
int testLlmLive()
{
    int argc = 1;
    char arg0[] = "ghew-tests";
    char *argv[] = {arg0, nullptr};
    QCoreApplication app(argc, argv);

    const QByteArray env = qgetenv("GHEW_LLM_BASE_URL");
    const QString base = env.isEmpty() ? QStringLiteral("http://127.0.0.1:8080/v1")
                                       : QString::fromUtf8(env);

    LlmClient client;
    client.setBaseUrl(base);
    // Force probe to adopt the id the server actually advertises.
    client.setModel(QString());

    {
        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);
        QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        QObject::connect(&client, &LlmClient::statusChanged, &loop, &QEventLoop::quit);
        timer.start(20000);
        client.probe();
        loop.exec();
    }
    CHECK(client.reachable());
    CHECK(!client.model().isEmpty());
    std::fprintf(stderr, "LLM_LIVE model=%s\n", qPrintable(client.model()));

    bool ok = false;
    QString text;
    {
        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);
        QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start(90000);
        client.complete(QStringLiteral("Ответь по-русски одним коротким предложением. Без JSON."),
                        QStringLiteral("Что показывает git status?"), 120,
                        [&](bool o, const QString &t) {
                            ok = o;
                            text = t;
                            loop.quit();
                        });
        loop.exec();
    }
    CHECK(ok);
    CHECK(text.size() > 12);
    CHECK(QRegularExpression(QStringLiteral("[А-Яа-яЁё]")).match(text).hasMatch());
    CHECK(!text.contains(QStringLiteral("Self-Correction")));
    CHECK(!text.trimmed().startsWith(QStringLiteral("*")));
    std::fprintf(stderr, "LLM_LIVE complete: %s\n", qPrintable(text.left(240)));

    bool streamOk = false;
    QString streamed;
    {
        QEventLoop loop;
        QTimer timer;
        timer.setSingleShot(true);
        QObject::connect(&timer, &QTimer::timeout, &loop, &QEventLoop::quit);
        timer.start(90000);
        client.completeStream(QStringLiteral("Ответь по-русски одним коротким предложением."),
                              QStringLiteral("Зачем нужен git add?"), 80, {},
                              [&](bool o, const QString &t) {
                                  streamOk = o;
                                  streamed = t;
                                  loop.quit();
                              },
                              true);
        loop.exec();
    }
    CHECK(streamOk);
    CHECK(streamed.size() > 8);
    CHECK(QRegularExpression(QStringLiteral("[А-Яа-яЁё]")).match(streamed).hasMatch());
    CHECK(streamed == client.streamBuffer());
    std::fprintf(stderr, "LLM_LIVE stream: %s\n", qPrintable(streamed.left(240)));

    std::printf("LLM_LIVE_OK\n");
    return 0;
}
