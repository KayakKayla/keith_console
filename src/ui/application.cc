#include "application.h"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "plain_text_surface.h"
#include "terminal/logger.h"
#include "terminal/terminal_bridge.h"

#include <QtQml/qqmlregistration.h>
#include <QDir>

int Application::run(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    const QString logDir = QDir(QCoreApplication::applicationDirPath()).filePath("../logs");
    initLogger(logDir);

    qmlRegisterType<PlainTextSurface>("KeithConsole", 1, 0, "PlainTextSurface");

    QQmlApplicationEngine engine;
    auto *bridge = new TerminalBridge(&engine);
    engine.rootContext()->setContextProperty("terminalBridge", bridge);
    QObject::connect(
        &engine,
        &QQmlApplicationEngine::objectCreationFailed,
        &app,
        []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);
    engine.loadFromModule("keith_console", "Main");

    return app.exec();
}

int main(int argc, char *argv[])
{
    Application application;
    return application.run(argc, argv);
}
