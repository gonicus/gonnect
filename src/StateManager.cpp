#include <QtDBus/QDBusConnection>

#ifdef Q_OS_LINUX
#  include "DBusActivationAdapter.h"
#  include "DBusActivationInterface.h"
#  include "GOnnectDBusAPI.h"
#endif

#include "GlobalShortcuts.h"
#include "StateManager.h"
#include "SIPCallManager.h"
#include "SIPManager.h"
#include "Application.h"
#include "CallHistory.h"
#include "GlobalCallState.h"
#include "ExternalMediaManager.h"

Q_LOGGING_CATEGORY(lcStateHandling, "gonnect.state")

StateManager::StateManager(QObject *parent) : QObject(parent)
{
#ifdef Q_OS_LINUX
    m_activationAdapter = new DBusActivationAdapter(this);
    m_apiEndpoint = new GOnnectDBusAPI(this);

    auto con = QDBusConnection::sessionBus();
    if (con.isConnected()) {
        m_isFirstInstance =
                con.registerObject(FLATPAK_APP_PATH, this) && con.registerService(FLATPAK_APP_ID);
    }
#endif

    m_inhibitHelper = &InhibitHelper::instance();
    connect(m_inhibitHelper, &InhibitHelper::stateChanged, this,
            &StateManager::sessionStateChanged);

    auto &cm = SIPCallManager::instance();
    connect(&cm, &SIPCallManager::activeCallsChanged, this, [this]() {
        if (SIPCallManager::instance().activeCalls() == 0) {
            m_inhibitHelper->release();
        }
    });

    connect(&GlobalCallState::instance(), &GlobalCallState::globalCallStateChanged, this,
            &StateManager::updateInhibitState);
}

bool StateManager::globalShortcutsSupported() const
{
    return GlobalShortcuts::instance().isSupported();
}

QVariantMap StateManager::globalShortcuts() const
{
    QVariantMap res;
    auto gcs = GlobalShortcuts::instance().shortcuts();

    for (auto &gc : std::as_const(gcs)) {
        QVariantMap info = { { "description", gc->description },
                             { "trigger", gc->triggerDescription } };
        res.insert(gc->id, info);
    }

    return res;
}

void StateManager::setUiEditMode(bool option)
{
    if (m_uiEditMode != option) {
        m_uiEditMode = option;

        Q_EMIT uiEditModeChanged();
    }
};

void StateManager::setUiDirtyState(bool option)
{
    if (m_uiDirtyState != option) {
        m_uiDirtyState = option;

        Q_EMIT uiDirtyStateChanged();
    }
};

void StateManager::setUiSaveState(bool option)
{
    if (m_uiSaveState != option) {
        m_uiSaveState = option;

        Q_EMIT uiSaveStateChanged();
    }
};

void StateManager::initialize()
{
    auto &globalShortcuts = GlobalShortcuts::instance();

    QList<Shortcut> shortcuts = {
        { "dial",
          { { "description", tr("Show dial window and focus search field") },
            { "preferred_trigger", "CTRL+ALT+K" } } },
        { "hangup",
          { { "description", tr("End all calls") }, { "preferred_trigger", "CTRL+ALT+E" } } },
        { "redial",
          { { "description", tr("Redial last outgoing call") },
            { "preferred_trigger", "CTRL+ALT+R" } } },
        { "toggle-hold",
          { { "description", tr("Toggle hold") }, { "preferred_trigger", "CTRL+ALT+M" } } },
    };

    globalShortcuts.setShortcuts(shortcuts);

    connect(&globalShortcuts, &GlobalShortcuts::initialized, this,
            &StateManager::globalShortcutsSupportedChanged);
    connect(&globalShortcuts, &GlobalShortcuts::shortcutsChanged, this,
            &StateManager::globalShortcutsChanged);
    connect(&globalShortcuts, &GlobalShortcuts::activated, this, [](const QString &action) {
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
#ifdef Q_OS_LINUX
    auto con = QDBusConnection::sessionBus();
    if (con.isConnected()) {
        con.unregisterObject(FLATPAK_APP_PATH, QDBusConnection::UnregisterTree);
        con.unregisterService(FLATPAK_APP_ID);
    }
#endif

    if (SIPCallManager::instance().activeCalls() == 0) {
        m_inhibitHelper->release();
    }
}

void StateManager::inhibitScreenSaver()
{
    if (m_inhibitHelper) {
        m_inhibitHelper->inhibitScreenSaver(QApplication::applicationName(),
                                            tr("Phone calls are active"));
    }
}

void StateManager::releaseScreenSaver()
{
    if (m_inhibitHelper) {
        m_inhibitHelper->releaseScreenSaver();
    }
}

void StateManager::sessionStateChanged(bool, InhibitHelper::InhibitState state)
{
    if (state == InhibitHelper::InhibitState::QUERY_END) {

        // If there are active calls, inhibit session changes that would interrupt
        // the call. That allows (at least for ~60s) for a reaction of the user.
        unsigned activeCalls = SIPCallManager::instance().activeCalls();
        if (activeCalls) {
            qCDebug(lcStateHandling)
                    << "trying to inhibit session due to" << activeCalls << "active calls";
            m_inhibitHelper->inhibit(
                    InhibitHelper::InhibitFlag::IDLE | InhibitHelper::InhibitFlag::LOGOUT
                            | InhibitHelper::InhibitFlag::SUSPEND
                            | InhibitHelper::InhibitFlag::USER_SWITCH,
                    QObject::tr("There are %n active call(s).", "calls", activeCalls));
        }
    }
}

void StateManager::sendArguments(const QStringList &args)
{
#ifdef Q_OS_LINUX
    auto con = QDBusConnection::sessionBus();
    if (con.isConnected()) {
        QVariantList vlArgs;
        for (auto &arg : std::as_const(args)) {
            vlArgs << arg;
        }

        OrgFreedesktopApplicationInterface actionInterface(FLATPAK_APP_ID, FLATPAK_APP_PATH, con,
                                                           this);

        actionInterface.ActivateAction("invoke", vlArgs, {});
    }
#else
    Q_UNUSED(args)
#endif
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

void StateManager::updateInhibitState()
{
    typedef ICallState::State State;

    const auto state = GlobalCallState::instance().globalCallState();
    const auto changeMask = m_oldCallState ^ state;

    m_oldCallState = state;
    if (changeMask & State::RingingIncoming) {
        if (state & State::RingingIncoming) {
            qCInfo(lcStateHandling) << "ringing - pausing external media";
            ExternalMediaManager::instance().pause();
        }
    }

    if (changeMask & State::RingingOutgoing) {
        if (state & State::RingingOutgoing) {
            qCInfo(lcStateHandling) << "ringing - pausing external media";
            ExternalMediaManager::instance().pause();
        }
    }

    if (changeMask & State::CallActive) {
        if (state & State::CallActive) {
            qCInfo(lcStateHandling) << "call active - inhibit screensaver, pausing external media";
            inhibitScreenSaver();
            ExternalMediaManager::instance().pause();
        } else {
            qCInfo(lcStateHandling) << "call ended - release screensaver, resuming external media";
            releaseScreenSaver();
            ExternalMediaManager::instance().resume();
        }
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
