#include <QQmlApplicationEngine>
#include <QDBusConnection>
#include <QGuiApplication>
#include <QQuickWindow>
#include <QDBusMetaType>
#include <signal.h>
#include "AccountPortal.h"
#include "Application.h"

static int setup_unix_signal_handlers()
{
    struct sigaction hup, term;

    hup.sa_handler = Application::hupSignalHandler;
    sigemptyset(&hup.sa_mask);
    hup.sa_flags = 0;
    hup.sa_flags |= SA_RESTART;

    if (sigaction(SIGHUP, &hup, 0)) {
        return 1;
    }

    term.sa_handler = Application::termSignalHandler;
    sigemptyset(&term.sa_mask);
    term.sa_flags = 0;
    term.sa_flags |= SA_RESTART;

    if (sigaction(SIGTERM, &term, 0)) {
        return 2;
    }

    return 0;
}

int main(int argc, char *argv[])
{
    setup_unix_signal_handlers();

    qDBusRegisterMetaType<QPair<QString, QVariantMap>>();
    qDBusRegisterMetaType<QList<QPair<QString, QVariantMap>>>();

    if (!QDBusConnection::sessionBus().isConnected()) {
        qWarning("Cannot connect to the D-Bus session bus.\n"
                 "Please check your system settings and try again.\n");
        return 1;
    }

    Application app(argc, argv);
    if (!app.isFirstInstance()) {
        qCritical() << "Another instance is running - sending args and exit";
        app.sendArguments();
        return 2;
    }

    QQmlApplicationEngine engine;
    QObject::connect(
            &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
            []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    engine.loadFromModule("base", "Main");

    QQuickWindow *win = qobject_cast<QQuickWindow *>(engine.rootObjects().first());
    app.setRootWindow(win);

    return app.exec();
}
