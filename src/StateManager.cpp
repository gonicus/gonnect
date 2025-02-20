#include <QtDBus/QDBusConnection>

#include "DBusActivationAdapter.h"
#include "DBusActivationInterface.h"
#include "GlobalShortcutPortal.h"
#include "StateManager.h"
#include "SIPCallManager.h"
#include "SIPManager.h"
#include "Application.h"
#include "CallHistory.h"
#include "ScreenSaverInterface.h"

Q_LOGGING_CATEGORY(lcStateHandling, "gonnect.state")

StateManager::StateManager(QObject *parent) : QObject(parent)
{
    m_activationAdapter = new DBusActivationAdapter(this);

    auto con = QDBusConnection::sessionBus();
    if (con.isConnected()) {
        m_isFirstInstance = con.registerObject("/de/gonicus/gonnect", this)
                && con.registerService("de.gonicus.gonnect");
    }

    m_inhibitPortal = new InhibitPortal(this);
    connect(m_inhibitPortal, &InhibitPortal::stateChanged, this,
            &StateManager::sessionStateChanged);

    auto &cm = SIPCallManager::instance();
    connect(&cm, &SIPCallManager::activeCallsChanged, this, [this]() {
        if (SIPCallManager::instance().activeCalls() == 0) {
            m_inhibitPortal->release();
        }
    });

    m_screenSaverInterface = new OrgFreedesktopScreenSaverInterface("org.freedesktop.ScreenSaver",
                                                                    "/org/freedesktop/ScreenSaver",
                                                                    QDBusConnection::sessionBus());
}

bool StateManager::globalShortcutsSupported() const
{
    return m_globalShortcutPortal ? m_globalShortcutPortal->isSupported() : false;
}

QVariantMap StateManager::globalShortcuts() const
{
    QVariantMap res;
    auto gcs = m_globalShortcutPortal->shortcuts();

    for (auto &gc : std::as_const(gcs)) {
        QVariantMap info = { { "description", gc->description },
                             { "trigger", gc->triggerDescription } };
        res.insert(gc->id, info);
    }

    return res;
}

void StateManager::initialize()
{
    m_globalShortcutPortal = new GlobalShortcutPortal(this);
    m_globalShortcutPortal->initialize();

    connect(m_globalShortcutPortal, &GlobalShortcutPortal::initialized, this,
            &StateManager::globalShortcutsSupportedChanged);
    connect(m_globalShortcutPortal, &GlobalShortcutPortal::shortcutsChanged, this,
            &StateManager::globalShortcutsChanged);
    connect(m_globalShortcutPortal, &GlobalShortcutPortal::activated, this,
            [](const QString &action) {
                auto &cm = SIPCallManager::instance();
                if (action == "dial") {
                    qobject_cast<Application *>(Application::instance())->rootWindow()->show();
                } else if (action == "hangup") {
                    cm.endAllCalls();
                } else if (action == "redial") {
                    auto ci = CallHistory::instance().lastOutgoingSipInfo();
                    SIPCallManager::instance().call(ci.sipUrl);
                } else if (action == "toggle-hold") {
                    cm.toggleHold();
                }
            });
}

void StateManager::restart()
{
    qApp->exit(RESTART_CODE);
}

StateManager::~StateManager()
{
    auto con = QDBusConnection::sessionBus();
    if (con.isConnected()) {
        con.unregisterObject("/de/gonicus/gonnect", QDBusConnection::UnregisterTree);
        con.unregisterService("de.gonicus.gonnect");
    }

    if (SIPCallManager::instance().activeCalls() == 0) {
        m_inhibitPortal->release();
    }
}

void StateManager::inhibitScreenSaver()
{
    if (m_screenSaverInterface && !m_screenSaverIsInhibited) {
        QDBusPendingReply<unsigned> reply = m_screenSaverInterface->Inhibit(
                QApplication::applicationName(), tr("Phone calls are active"));
        reply.waitForFinished();

        m_screenSaverCookie = reply.value();
        m_screenSaverIsInhibited = true;
    }
}

void StateManager::releaseScreenSaver()
{
    if (m_screenSaverInterface && m_screenSaverIsInhibited) {
        m_screenSaverInterface->UnInhibit(m_screenSaverCookie);
        m_screenSaverIsInhibited = false;
    }
}

void StateManager::sessionStateChanged(bool, InhibitPortal::InhibitState state)
{
    if (state == InhibitPortal::InhibitState::QUERY_END) {

        // If there are active calls, inhibit session changes that would interrupt
        // the call. That allows (at least for ~60s) for a reaction of the user.
        unsigned activeCalls = SIPCallManager::instance().activeCalls();
        if (activeCalls) {
            qCDebug(lcStateHandling)
                    << "trying to inhibit session due to" << activeCalls << "active calls";
            m_inhibitPortal->inhibit(
                    InhibitPortal::InhibitFlag::IDLE | InhibitPortal::InhibitFlag::LOGOUT
                            | InhibitPortal::InhibitFlag::SUSPEND
                            | InhibitPortal::InhibitFlag::USER_SWITCH,
                    QObject::tr("There are %n active call(s).", "calls", activeCalls),
                    [this](uint code, const QVariantMap &) {
                        if (code != 0) {
                            qCCritical(lcStateHandling)
                                    << "failed to inhibit session: response code" << code;
                        } else {
                            m_inhibitPortal->queryEndResponse();
                        }
                    });
        }
    }
}

void StateManager::sendArguments(const QStringList &args)
{
    auto con = QDBusConnection::sessionBus();
    if (con.isConnected()) {
        QVariantList vlArgs;
        for (auto &arg : std::as_const(args)) {
            vlArgs << arg;
        }

        OrgFreedesktopApplicationInterface actionInterface("de.gonicus.gonnect",
                                                           "/de/gonicus/gonnect", con, this);

        actionInterface.ActivateAction("invoke", vlArgs, {});
    }
}

void StateManager::Activate(const QVariantMap &)
{
    qobject_cast<Application *>(Application::instance())->rootWindow()->show();
}

void StateManager::ActivateAction(const QString &action_name, const QVariantList &parameter,
                                  const QVariantMap &platform_data)
{
    Q_UNUSED(platform_data)

    // Default action for invoke is to try dialing a number
    if (action_name == "invoke") {
        for (auto &v : std::as_const(parameter)) {
            QString value = v.toString();

            if (value == "--show") {
                qobject_cast<Application *>(Application::instance())->rootWindow()->show();
            } else if (value == "--hangup") {
                SIPCallManager::instance().endAllCalls();
            } else {
                SIPCallManager::instance().call(value);
                break;
            }
        }
    } else if (action_name == "Show") {
        qobject_cast<Application *>(Application::instance())->rootWindow()->show();
    } else if (action_name == "Hangup") {
        SIPCallManager::instance().endAllCalls();
    } else if (action_name == "refreshIdentities") {
        SIPManager::instance().initializePreferredIdentities();
    } else {
        qCWarning(lcStateHandling) << "unknown activation action" << action_name;
    }
}

void StateManager::Open(const QStringList &args, const QVariantMap &)
{
    if (args.length()) {
        QVariantList vArgs;
        for (auto &arg : std::as_const(args)) {
            vArgs.push_back(arg);
        }
        ActivateAction("invoke", vArgs, {});
    } else {
        qobject_cast<Application *>(Application::instance())->rootWindow()->show();
    }
}
