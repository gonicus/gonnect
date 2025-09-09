#include <QLoggingCategory>
#include <QTimer>

#include "SIPBuddy.h"
#include "SIPAccount.h"
#include "SIPManager.h"
#include "SIPCallManager.h"
#include "Notification.h"
#include "NotificationManager.h"
#include "PhoneNumberUtil.h"
#include "AvatarManager.h"

Q_LOGGING_CATEGORY(lcSIPBuddy, "gonnect.sip.buddy")

using std::chrono::seconds;
using namespace std::chrono_literals;

SIPBuddy::SIPBuddy(SIPAccount *account, const QString &uri)
    : QObject(account), pj::Buddy(), m_uri(uri), m_account(account)
{
}

bool SIPBuddy::initialize()
{
    pj::BuddyConfig cfg;

    cfg.uri = m_uri.toStdString();

    try {
        create(*m_account, cfg);
        subscribePresence(true);
    } catch (pj::Error &err) {
        qCCritical(lcSIPBuddy) << "failed to create buddy" << m_uri << ":" << err.info(false);
        return false;
    }

    return true;
}

SIPBuddy::~SIPBuddy() { }

void SIPBuddy::onBuddyState()
{
    pj::BuddyInfo bi = getInfo();

    SIPBuddyState::STATUS status = SIPBuddyState::STATUS::UNKNOWN;
    QString statusText = QString::fromStdString(bi.presStatus.statusText);

    // Currently hard coded to what our asterisk is sending us
    if (statusText == "Ready") {
        status = SIPBuddyState::STATUS::READY;
    } else if (statusText == "Ringing") {
        status = SIPBuddyState::STATUS::RINGING;
    } else if (statusText == "On the phone") {
        status = SIPBuddyState::STATUS::BUSY;
    } else if (statusText == "Unavailable") {
        status = SIPBuddyState::STATUS::UNAVAILABLE;
    }

    if (m_subscribeToStatus && status == SIPBuddyState::STATUS::READY) {
        notifyOnceWhenBuddyAvailable();
        m_subscribeToStatus = false;
        m_subscribeTimeout.stop();
    }

    if (status != m_status) {
        m_status = status;
        m_statusText = statusText;
        Q_EMIT statusChanged(m_status);

        qCInfo(lcSIPBuddy) << "Buddy" << m_uri << "changed state to:" << m_status;
        Q_EMIT SIPManager::instance().buddyStateChanged(m_uri, status);
    }
}

void SIPBuddy::subscribeToBuddyStatus()
{
    m_subscribeToStatus = true;

    m_subscribeTimeout.singleShot(6h, this, [this]() { m_subscribeToStatus = false; });
}

void SIPBuddy::notifyOnceWhenBuddyAvailable()
{
    // Create notification text
    const auto contactInfo = PhoneNumberUtil::instance().contactInfoBySipUrl(m_uri);
    const Contact *c = contactInfo.contact;
    QStringList bodyParts;

    const QString title =
            tr("%1 is now available")
                    .arg((c && !c->name().isEmpty()) ? c->name() : contactInfo.phoneNumber);
    const QString number = contactInfo.phoneNumber;

    if (c && !c->company().isEmpty()) {
        bodyParts.append(c->company());
    }

    auto countries = contactInfo.countries;
    if (!contactInfo.city.isEmpty()) {
        countries.push_front(contactInfo.city);
    }
    if (countries.size()) {
        bodyParts.append(countries.join(", "));
    }

    // Create notification object
    auto n = new Notification(title, bodyParts.join("\n"), Notification::Priority::normal, this);

    auto &am = AvatarManager::instance();
    QString avatar = c ? am.avatarPathFor(c->id()) : "";

    if (avatar.isEmpty()) {
        n->setIcon("help-about-symbolic");
    } else {
        n->setIcon(avatar);
        n->setRoundedIcon(true);
    }

    n->setDisplayHint(Notification::tray | Notification::hideContentOnLockScreen);
    n->setDefaultAction("call");

    QString ref = NotificationManager::instance().add(n);

    QMetaObject::Connection notificationConnection =
            connect(n, &Notification::actionInvoked, this, [number, ref](QString, QVariantList) {
                SIPCallManager::instance().call(number);

                NotificationManager::instance().remove(ref);
            });
}
