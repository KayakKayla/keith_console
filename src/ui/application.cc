#include "application.h"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QQmlApplicationEngine>

int Application::run(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;
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
