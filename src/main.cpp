#include "app/AppController.h"
#include "core/GhewBuild.h"

#include <QCoreApplication>
#include <QFileInfo>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickStyle>
#include <QUrl>

#include <KLocalizedString>

#include <cstdio>

namespace {

QString repoPathFromArgs(int argc, char *argv[])
{
    for (int i = 1; i < argc; ++i) {
        const QByteArray arg(argv[i]);
        if (arg.startsWith('-'))
            continue;
        QString path = QString::fromLocal8Bit(arg);
        if (path.startsWith(QStringLiteral("file://")))
            path = QUrl(path).toLocalFile();
        const QFileInfo fi(path);
        if (fi.exists())
            return fi.absoluteFilePath();
        if (!path.isEmpty())
            return QFileInfo(path).absoluteFilePath();
    }
    return {};
}

void setAppIdentity()
{
    QCoreApplication::setOrganizationName(QStringLiteral("EmperyWorld"));
    QCoreApplication::setApplicationName(QStringLiteral("ghew"));
    QCoreApplication::setApplicationVersion(QStringLiteral(GHEW_VERSION));
}

} // namespace

int main(int argc, char *argv[])
{
    if (argc >= 2 && QByteArray(argv[1]) == QByteArrayLiteral("--self-check")) {
        QCoreApplication app(argc, argv);
        setAppIdentity();
        AppController controller;
        const QString report = controller.runSelfCheck();
        std::fprintf(stdout, "%s\n", qPrintable(report));
        return report.contains(QStringLiteral("SELF_CHECK_OK")) ? 0 : 4;
    }
    if (argc >= 2 && QByteArray(argv[1]) == QByteArrayLiteral("--integration-test")) {
        QCoreApplication app(argc, argv);
        setAppIdentity();
        AppController controller;
        const QString report = controller.runIntegrationTest();
        std::fprintf(stdout, "%s\n", qPrintable(report));
        return report.contains(QStringLiteral("INTEGRATION_OK")) ? 0 : 5;
    }
    if (argc >= 3 && QByteArray(argv[1]) == QByteArrayLiteral("--dump-status")) {
        QCoreApplication app(argc, argv);
        setAppIdentity();
        AppController controller;
        const QString json = controller.dumpRepositoryStatus(QString::fromLocal8Bit(argv[2]));
        if (json.isEmpty()) {
            std::fprintf(stderr, "ghew: failed to open repository: %s\n",
                         qPrintable(controller.repo()->lastError()));
            return 3;
        }
        std::fprintf(stdout, "%s\n", qPrintable(json));
        return 0;
    }

    QGuiApplication app(argc, argv);
    setAppIdentity();

    KLocalizedString::setApplicationDomain("ghew");
    QQuickStyle::setStyle(QStringLiteral("org.kde.desktop"));

    AppController controller;
    AppController::setInstance(&controller);

    if (controller.commandCount() <= 0) {
        std::fprintf(stderr, "ghew: command catalog is empty (data/commands missing)\n");
        return 2;
    }

    QQmlApplicationEngine engine;

    const QUrl mainUrl(QStringLiteral("qrc:/qt/qml/Ghew/Main.qml"));
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        [mainUrl]() {
            std::fprintf(stderr, "Failed to create QML object from %s\n", qPrintable(mainUrl.toString()));
            QCoreApplication::exit(1);
        },
        Qt::QueuedConnection);

    engine.load(mainUrl);
    if (engine.rootObjects().isEmpty()) {
        std::fprintf(stderr, "Failed to load %s\n", qPrintable(mainUrl.toString()));
        return 1;
    }

    const QString cliRepo = repoPathFromArgs(argc, argv);
    if (!cliRepo.isEmpty()) {
        if (!controller.openRepository(cliRepo)) {
            std::fprintf(stderr, "ghew: cannot open repository: %s\n%s\n",
                         qPrintable(cliRepo), qPrintable(controller.repo()->lastError()));
        }
    }

    return app.exec();
}
