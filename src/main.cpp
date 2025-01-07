#include <QQmlApplicationEngine>
#include <QDBusConnection>
#include <QQuickWindow>
#include <QDBusMetaType>

#include <signal.h>
#include "Application.h"
#include "ErrorBus.h"

using namespace Qt::Literals::StringLiterals;

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
    qSetMessagePattern(
            "\033[32m%{time h:mm:ss.zzz}%{if-category}\033[32m %{category}:%{endif} "
            "%{if-debug}\033[34m%{function}%{endif}%{if-warning}\033[31m%{backtrace "
            "depth=3}%{endif}%{if-critical}\033[31m%{backtrace "
            "depth=3}%{endif}%{if-fatal}\033[31m%{backtrace depth=3}%{endif}\033[0m %{message}");
    setup_unix_signal_handlers();

    if (!QDBusConnection::sessionBus().isConnected()) {
        qWarning("Cannot connect to the D-Bus session bus.\n"
                 "Please check your system settings and try again.\n");
        return 1;
    }

    qDBusRegisterMetaType<QList<QVariantMap>>();
    qDBusRegisterMetaType<QPair<QString, QVariantMap>>();
    qDBusRegisterMetaType<QList<QPair<QString, QVariantMap>>>();

    qputenv("QT_QUICK_FLICKABLE_WHEEL_DECELERATION", "7000"); // Workaround bad scrolling
    qputenv("XDG_CURRENT_DESKTOP", "gnome"); // Workaround for QTBUG-126179

    int exitCode = 0;

    Application app(argc, argv);
    if (!app.isFirstInstance()) {
        qInfo() << "another instance is running - sending args and exit";
        return 2;
    }

    QQmlApplicationEngine engine;
    QObject::connect(
            &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
            []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    engine.loadFromModule("base", "Main");

    const auto &objs = engine.rootObjects();
    const auto &itemObjs = objs.first();

    QQuickWindow *dialWindow = itemObjs->findChild<QQuickWindow *>("dialWindow");
    if (dialWindow) {
        app.setRootWindow(dialWindow);
    }

    exitCode = app.exec();
    if (exitCode == RESTART_CODE) {
        QProcess::startDetached(argv[0]);
    }

    return exitCode;
}
