#include <QLoggingCategory>
#include <QRegularExpression>
#include "SIPAccountManager.h"
#include "SIPCallManager.h"
#include "SIPBuddy.h"
#include "Toggler.h"
#include "ErrorBus.h"

Q_LOGGING_CATEGORY(lcToggler, "gonnect.sip.toggler")

using namespace std::chrono_literals;

Toggler::Toggler(const QString &id, QObject *parent) : QObject(parent), Account(), m_id(id)
{
    m_timeoutTimer.setSingleShot(true);
    m_timeoutTimer.setInterval(5s);

    connect(&m_timeoutTimer, &QTimer::timeout, this, [this]() {
        m_busy = false;

        Q_EMIT ErrorBus::instance().error(tr("Failed to toggle the state of %1.").arg(m_id));
        Q_EMIT busyChanged();
        Q_EMIT activeChanged();
    });
}

bool Toggler::initialize()
{
    m_settings.beginGroup(m_id);

    QString ac = m_settings.value("account", "").toString();
    if (ac.isEmpty()) {
        qCCritical(lcToggler) << "ignoring toggler" << m_id << "without account reference";
        return false;
    }

    m_account = SIPAccountManager::instance().getAccount(ac);
    if (!m_account) {
        qCCritical(lcToggler) << "ignoring toggler" << m_id << "with invalid account reference";
        return false;
    }

    m_label = m_settings.value("label", "").toString();
    if (m_label.isEmpty()) {
        qCCritical(lcToggler) << "ignoring toggler" << m_id << "without label";
        return false;
    }

    m_description = m_settings.value("description", "").toString();

    m_subscribe = m_settings.value("subscribe", "").toString();
    m_toggle = m_settings.value("toggle", "").toString();
    if (m_toggle.isEmpty() || m_subscribe.isEmpty()) {
        qCCritical(lcToggler) << "ignoring toggler" << m_id << "without toggle/subscribe";
        return false;
    }

    m_display = 0;
    QStringList displayOptions = m_settings.value("display", "").toStringList();
    for (auto &option : std::as_const(displayOptions)) {
        if (option == "menu") {
            m_display |= DISPLAY::MENU;
        } else if (option == "tray") {
            m_display |= DISPLAY::TRAY;
        } else if (option == "statusbar") {
            m_display |= DISPLAY::STATUS;
        } else if (option == "settings-phoning") {
            m_display |= DISPLAY::CFG_PHONING;
        } else {
            qCCritical(lcToggler) << "ignoring unknown toggler" << m_id
                                  << "display option:" << option;
        }
    }

    m_buddy = new SIPBuddy(m_account, m_subscribe);
    connect(m_buddy, &SIPBuddy::statusChanged, this, [this]() {
        m_timeoutTimer.stop();

        m_busy = false;
        Q_EMIT busyChanged();

        Q_EMIT activeChanged();
    });
    m_buddy->initialize();

    m_settings.endGroup();
    return true;
}

void Toggler::setActive(bool value)
{
    if (!m_busy && m_buddy) {
        bool active = m_buddy->status() == SIPBuddyState::BUSY;
        if (active != value) {
            m_timeoutTimer.start();

            m_busy = true;
            Q_EMIT busyChanged();

            SIPCallManager::instance().call(m_toggle, true);
        }
    }
}

Toggler::~Toggler() { }
