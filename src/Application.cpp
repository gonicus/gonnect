#include "Application.h"
#include "StateManager.h"
#include "SearchProvider.h"
#include "UISettings.h"
#include "NotificationManager.h"
#include "appversion.h"
#include "SIPManager.h"
#include "SIPCallManager.h"
#include "SystemTrayMenu.h"
#include "AddressBookManager.h"
#include "USBDevices.h"
#include "Credentials.h"
#include "AppSettings.h"
#include "DateEventFeederManager.h"
#include "BackgroundManager.h"
#include <QLoggingCategory>
#include <QTranslator>
#include <QLibraryInfo>
#include <QQmlEngine>
#include <QQmlContext>
#include <QDesktopServices>
#include <QtWebEngineQuick>
#include <iostream>

#ifdef Q_OS_DARWIN
#   define LOGFAULT_USE_OS_LOG
#endif
#include "logfault/logfault.h"

#ifdef Q_OS_WINDOWS
#   include "windows/WindowsEventLogHandler.h"
#endif

#ifdef Q_OS_LINUX
#  include <sys/socket.h>
#  include <unistd.h>
#endif

using namespace std::chrono_literals;

using namespace Qt::Literals::StringLiterals;

Q_LOGGING_CATEGORY(lcApplication, "gonnect.app")

#ifdef Q_OS_LINUX
int Application::s_sighupFd[2];
int Application::s_sigtermFd[2];
#endif

Application::Application(int &argc, char **argv) : QApplication(argc, argv)
{
    qCCritical(lcApplication) << "Constructing app, version" << getVersion();
    connect(this, &Application::aboutToQuit, this, &Application::shutdown);

    setApplicationName("GOnnect");
    setOrganizationName("gonnect");
    setOrganizationDomain("gonicus.de");
#ifdef Q_OS_LINUX
    setDesktopFileName(FLATPAK_APP_ID);
#endif
    setApplicationVersion(QString::fromStdString(getVersion()));

    installTranslations();

    initLogging();

    AddressBookManager::instance().initAddressBookConfigs();
    DateEventFeederManager::instance().initFeederConfigs();

    setQuitOnLastWindowClosed(false);

    USBDevices::instance().initialize();

    StateManager::instance().setParent(this);
    SearchProvider::instance().setParent(this);
    UISettings::instance().setParent(this);

#ifdef Q_OS_LINUX
    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, s_sighupFd)) {
        qFatal("Couldn't create HUP socketpair");
    }

    if (::socketpair(AF_UNIX, SOCK_STREAM, 0, s_sigtermFd)) {
        qFatal("Couldn't create TERM socketpair");
    }

    m_hupNotifier = new QSocketNotifier(s_sighupFd[1], QSocketNotifier::Read, this);
    connect(m_hupNotifier, SIGNAL(activated(QSocketDescriptor)), this, SLOT(handleSigHup()));
    m_termNotifier = new QSocketNotifier(s_sigtermFd[1], QSocketNotifier::Read, this);
    connect(m_termNotifier, SIGNAL(activated(QSocketDescriptor)), this, SLOT(handleSigTerm()));
#endif

    // Take care for running "initialize" after exec() is called
    QTimer::singleShot(0, this, &Application::initialize);
}

void Application::installTranslations()
{
    if (m_baseTranslator.load(QLocale::system(), u"qtbase"_s, u"_"_s,
                              QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
        installTranslator(&m_baseTranslator);
    }

    if (m_declarativeTranslator.load(QLocale::system(), u"qtdeclarative"_s, u"_"_s,
                                     QLibraryInfo::path(QLibraryInfo::TranslationsPath))) {
        installTranslator(&m_declarativeTranslator);
    }

    if (m_appTranslator.load(QLocale(), "gonnect"_L1, "_"_L1, ":/i18n"_L1)) {
        installTranslator(&m_appTranslator);
    }
}

void Application::initialize()
{
    StateManager::instance().initialize();

    AddressBookManager::instance().reloadAddressBook();
    DateEventFeederManager::instance().reload();
    SystemTrayMenu::instance(); // Ensure singleton is created

    AppSettings settings;
    bool autostart = settings.value("generic/autostart", false).toBool();

    BackgroundManager::instance().request(autostart);

    auto &cds = Credentials::instance();
    connect(&cds, &Credentials::initializedChanged, this, [this]() { initializeSIP(); });
    cds.initialize();
}

void Application::initializeSIP()
{
    auto &sm = SIPManager::instance();
    sm.initialize();

    auto &cm = SIPCallManager::instance();
    connect(&cm, &SIPCallManager::missedCallsChanged, this, [this]() {
        auto count = SIPCallManager::instance().missedCalls();
        setBadgeNumber(count);
        SystemTrayMenu::instance().setBadgeNumber(count);
    });

    m_initialized = true;
}

bool Application::isFirstInstance() const
{
    return StateManager::instance().isFirstInstance();
}

void Application::sendArguments() const
{
    StateManager::instance().sendArguments(arguments().sliced(1));
}

#ifdef Q_OS_LINUX
void Application::hupSignalHandler(int)
{
    char a = 1;
    (void)!::write(s_sighupFd[0], &a, sizeof(a));
}

void Application::termSignalHandler(int)
{
    char a = 1;
    (void)!::write(s_sigtermFd[0], &a, sizeof(a));
}

void Application::handleSigTerm()
{
    m_termNotifier->setEnabled(false);
    char tmp;
    (void)!::read(s_sigtermFd[1], &tmp, sizeof(tmp));

    quit();
}

void Application::handleSigHup()
{
    m_hupNotifier->setEnabled(false);
    char tmp;
    (void)!::read(s_sighupFd[1], &tmp, sizeof(tmp));

    quit();
}
#endif // Q_OS_LINUX

Application::~Application()
{
    qCDebug(lcApplication) << "Destructing app";
}

void Application::shutdown()
{
    NotificationManager::instance().shutdown();
    USBDevices::instance().shutdown();

    if (m_initialized) {
        SIPManager::instance().shutdown();
    }
}

QString Application::logFilePath()
{
    static QString path =
            QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) + "/" + logFileName();
    return path;
}

QString Application::logFileName()
{
    static QString fileName =
            QDateTime::currentDateTime().toString("yyyy-MM-dd---hh-mm-ss") + "---gonnect-log.txt";
    return fileName;
}

QtMessageHandler s_originalMessageHandler = nullptr;

void Application::initLogging()
{
    bool replaceMessageHandler = false;

    AppSettings settings;
    m_isDebugRun = settings.value("generic/nextDebugRun", false).toBool();
#ifndef Q_OS_LINUX
    replaceMessageHandler = true;
#else
    replaceMessageHandler = m_isDebugRun;
#endif

    if (replaceMessageHandler) {
        s_originalMessageHandler = qInstallMessageHandler(Application::logQtMessages);
    }

    if (m_isDebugRun) {
        settings.setValue("generic/nextDebugRun", false);
        auto filePath = Application::logFilePath().toStdString();
        logfault::LogManager::Instance().AddHandler(std::make_unique<logfault::StreamHandler>(filePath, logfault::LogLevel::TRACE, false));

        QTimer::singleShot(5 * 60 * 1000, this, []() {
            qCInfo(lcApplication) << "5 minutes are up; debug run will end automatically.";
            StateManager::instance().restart();
        });
    }

#ifdef Q_OS_WINDOWS
    std::unique_ptr<logfault::Handler> eventhandler{new WindowsEventLogHandler("GOnnect", logfault::LogLevel::WARN)};
    logfault::LogManager::Instance().AddHandler(std::move(eventhandler));
    s_originalMessageHandler = nullptr;
#endif // Q_OS_WINDOWS

#ifdef Q_OS_DARWIN
    logfault::LogManager::Instance().AddHandler(std::make_unique<logfault::OsLogHandler>(
        "oslog",
        logfault::LogLevel::WARN,
        logfault::OsLogHandler::Options{"de.gonicus.gonnect"}));
#endif // Q_OS_DARWIN
}

void Application::logQtMessages(QtMsgType type, const QMessageLogContext &context, const QString &rawMsg)
{
    auto msg =  rawMsg.toUtf8().constData();

    switch (type) {
    case QtDebugMsg:
        LFLOG_DEBUG << msg;
        break;
    case QtInfoMsg:
        LFLOG_INFO << msg;
        break;
    case QtWarningMsg: {
        // remove spam messages
        static const QRegularExpression filter{"is neither a default constructible QObject"
                                               "|Cannot anchor to an item that isn't a parent or sibling"
                                               "|Detected anchors on an item that is managed by a layout"};
        if (filter.match(msg).hasMatch()) {
            LFLOG_TRACE << msg;
            break;
        }
        LFLOG_WARN << msg;
        } break;
    case QtCriticalMsg:
        LFLOG_ERROR << msg;
        break;
    case QtFatalMsg:
        LFLOG_ERROR << "[**FATAL**] " << msg;
        exit(-1);
        break;
    }

    if (s_originalMessageHandler) {
        s_originalMessageHandler(type, context, rawMsg);
    }
}
