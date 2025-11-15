#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QtWebEngineQuick>
#include "Application.h"

#ifdef Q_OS_LINUX
#  include <QDBusConnection>
#  include <QDBusMetaType>
#  include <signal.h>
#endif

using namespace Qt::Literals::StringLiterals;

static int setup_unix_signal_handlers()
{
#ifdef Q_OS_LINUX
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
#endif

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

    QtWebEngineQuick::initialize();

#ifdef WITH_DBUS
    qDBusRegisterMetaType<QList<QVariantMap>>();
    qDBusRegisterMetaType<QPair<QString, QVariantMap>>();
    qDBusRegisterMetaType<QList<QPair<QString, QVariantMap>>>();
#endif

    qputenv("QT_QUICK_FLICKABLE_WHEEL_DECELERATION", "7000"); // Workaround bad scrolling
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS",
            "--use-fake-ui-for-media-stream"); // Workaround for QTBUG-134637

    int exitCode = 0;

    Application app(argc, argv);
    if (!app.isFirstInstance()) {
        qInfo() << "another instance is running - sending args and exit";
        app.sendArguments();
        return 2;
    }

    app.setWindowIcon(QIcon(":/icons/gonnect.svg"));

    // Fonts
    const QStringList fontPaths = { ":/font/NotoColorEmoji-Regular.ttf" };

    for (const QString &fontPath : fontPaths) {

        QFileInfo fontFileInfo(fontPath);
        fontFileInfo.makeAbsolute();
        qInfo().noquote().nospace()
                << "Trying to load font from file " << fontFileInfo.absoluteFilePath()
                << " (exists: " << fontFileInfo.exists()
                << ", readable: " << fontFileInfo.isReadable() << ")";

        const int fontId = QFontDatabase::addApplicationFont(fontPath);
        if (fontId >= 0) {
            const auto fontFamily = QFontDatabase::applicationFontFamilies(fontId).at(0);
            QFontDatabase::addApplicationEmojiFontFamily(fontFamily);
            qInfo() << "Loaded font" << fontFamily;
        } else {
            qWarning() << "Custom font could not be loaded";
        }
    }

    QQmlApplicationEngine engine;
    QObject::connect(
            &engine, &QQmlApplicationEngine::objectCreationFailed, &app,
            []() { QCoreApplication::exit(-1); }, Qt::QueuedConnection);
    engine.loadFromModule("base", "Main");

    const auto &objs = engine.rootObjects();
    const auto &itemObjs = objs.first();

    QQuickWindow *mainWindow = itemObjs->findChild<QQuickWindow *>("gonnectWindow");
    if (mainWindow) {
        app.setRootWindow(mainWindow);
    }

    exitCode = app.exec();
    if (exitCode == RESTART_CODE) {
        QProcess::startDetached(argv[0]);
    }

    return exitCode;
}
