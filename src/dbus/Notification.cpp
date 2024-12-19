#include <QDBusArgument>
#include <QVariantMap>
#include <QMetaEnum>
#include <QUuid>
#include <QBuffer>
#include <QPixmap>
#include "Notification.h"
#include "NotificationIcon.h"
#include "NotificationManager.h"

Notification::Notification(const QString &title, const QString &body, Priority priority,
                           QObject *parent)
    : QObject(parent), m_title(title), m_body(body), m_priority(priority)
{
    m_id = QUuid::createUuid().toString(QUuid::WithoutBraces).toUtf8();
}

void Notification::setIcon(const QString &iconUri)
{
    m_iconUri = iconUri;
}

void Notification::setEmblem(const QString &emblemUri)
{
    m_emblemUri = emblemUri;
}

void Notification::setRoundedIcon(bool flag)
{
    m_roundedIcon = flag;
}

void Notification::addButton(const QString &label, const QString &action, const QString &purpose,
                             const QVariantMap &parameters)
{
    QVariantMap buttonDescription = {
        { "label", label },
        { "action", action },
    };

    if (m_version == 2) {
        if (!purpose.isEmpty()) {
            buttonDescription.insert("purpose", purpose);
        }

        if (!parameters.isEmpty()) {
            buttonDescription.insert("target", parameters);
        }
    }

    m_buttons.append(buttonDescription);
}

QVariantMap Notification::toPortalDefinition() const
{
    QVariantMap data = { { "title", m_title }, { "body", m_body }, { "priority", priority() } };

    if (m_version == 2) {
        data.insert("sound", m_sound);

        if (m_displayHint) {
            QStringList hints;

            if (m_displayHint & DisplayHint::persistent) {
                hints.push_back("persistent");
            }
            if (m_displayHint & DisplayHint::transient) {
                hints.push_back("transient");
            } else if (m_displayHint & DisplayHint::tray) {
                hints.push_back("tray");
            }
            if (m_displayHint & DisplayHint::hideOnLockscreen) {
                hints.push_back("hide-on-lockscreen");
            }
            if (m_displayHint & DisplayHint::hideContentOnLockScreen) {
                hints.push_back("hide-content-on-lockscreen");
            }
            if (m_displayHint & DisplayHint::showAsNew) {
                hints.push_back("show-as-new");
            }

            data.insert("display-hint", hints);
        }

        if (!m_category.isEmpty()) {
            data.insert("category", m_category);
        }
    }

    if (!m_defaultAction.isEmpty()) {
        data.insert("default-action", m_defaultAction);
        data.insert("default-action-target", m_defaultParameters);
    }
    if (!m_iconUri.isEmpty()) {
        NotificationIcon icon(m_iconUri, m_emblemUri, m_roundedIcon);
        data.insert("icon", QVariant::fromValue(icon));
    }
    if (!m_buttons.isEmpty()) {
        data.insert("buttons", QVariant::fromValue(m_buttons));
    }
    return data;
}

QString Notification::priority() const
{
    return QMetaEnum::fromType<Priority>().valueToKey(m_priority);
}

Notification::~Notification()
{
    NotificationManager::instance().remove(m_id);
}
