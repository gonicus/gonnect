#include "Application.h"
#include "StateManager.h"
#include "SearchProvider.h"
#include "NotificationManager.h"
#include "appversion.h"
#include "SIPManager.h"
#include "SIPCallManager.h"
#include "SystemTrayMenu.h"
#include "AddressBookManager.h"
#include "USBDevices.h"

#include <sys/socket.h>
#include <unistd.h>
#include <QLoggingCategory>
#include <QTranslator>
#include <QLibraryInfo>
#include <QQmlEngine>
#include <QQmlContext>
#include <QDesktopServices>

#include "SecretPortal.h"
#include "AppSettings.h"

using namespace std::chrono_literals;

using namespace Qt::Literals::StringLiterals;

Q_LOGGING_CATEGORY(lcApplication, "gonnect.app")

int Application::s_sighupFd[2];
int Application::s_sigtermFd[2];

Application::Application(int &argc, char **argv) : QApplication(argc, argv)
{
    qCCritical(lcApplication) << "Constructing app, version" << getVersion();
    connect(this, &Application::aboutToQuit, this, &Application::shutdown);

    setApplicationName("GOnnect");
    setOrganizationName("gonnect");
    setOrganizationDomain("gonicus.de");
    setDesktopFileName("de.gonicus.gonnect");
    setApplicationVersion(QString::fromStdString(getVersion()));

    initDebugRun();
    AddressBookManager::instance().initAddressBookConfigs();

    setQuitOnLastWindowClosed(false);

    USBDevices::instance().initialize();

    StateManager::instance().setParent(this);
    SearchProvider::instance().setParent(this);

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

    installTranslations();

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
    AddressBookManager::instance().reloadAddressBook();
    SystemTrayMenu::instance(); // Ensure singleton is created

    AppSettings settings;
    bool autostart = settings.value("generic/autostart", false).toBool();

    m_backgroundPortal.RequestBackground(
            autostart, false, [](uint code, const QVariantMap &response) {
                switch (code) {
                case 0:
                    qCDebug(lcApplication)
                            << "autostart is set to" << response.value("autostart").toBool();
                    qCDebug(lcApplication)
                            << "background is set to" << response.value("background").toBool();
                    break;
                case 1:
                    qCWarning(lcApplication) << "autostart request was rejected by portal";
                    break;
                case 2:
                    qCWarning(lcApplication) << "autostart request to portal failed";
                    break;
                }
            });

    auto &sp = SecretPortal::instance();
    if (sp.isValid()) {
        connect(&sp, &SecretPortal::initializedChanged, this, [this]() { initializeSIP(); });

        sp.initialize();
    } else {
        qCWarning(lcApplication) << "no secrets portal available - unable to store secrets";
        initializeSIP();
    }
}

void Application::initializeSIP()
{
    auto &sm = SIPManager::instance();
    sm.initialize();

    auto &cm = SIPCallManager::instance();
    connect(&cm, &SIPCallManager::missedCallsChanged, this,
            [this]() { setBadgeNumber(SIPCallManager::instance().missedCalls()); });

    StateManager::instance().initialize();
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

void Application::fileMessageHandler(QtMsgType type, const QMessageLogContext &context,
                                     const QString &msg)
{
    const QString message = qFormatLogMessage(type, context, msg);

    static FILE *f = fopen(Application::logFilePath().toLatin1(), "a");
    fprintf(f, "%s\n", qPrintable(message));
    fflush(f);

    if (s_originalMessageHandler) {
        s_originalMessageHandler(type, context, msg);
    }
}

void Application::initDebugRun()
{
    AppSettings settings;
    m_isDebugRun = settings.value("generic/nextDebugRun", false).toBool();

    if (m_isDebugRun) {
        settings.setValue("generic/nextDebugRun", false);
        s_originalMessageHandler = qInstallMessageHandler(Application::fileMessageHandler);

        QTimer::singleShot(5 * 60 * 1000, this, []() {
            qCInfo(lcApplication) << "5 minutes are up; debug run will end automatically.";
            StateManager::instance().restart();
        });
    }
}
