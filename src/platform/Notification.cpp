#include <QVariantMap>
#include <QMetaEnum>
#include <QUuid>
#include <QBuffer>
#include <QPixmap>
#include "Notification.h"
#include "NotificationIcon.h"
#include "NotificationManager.h"

/// Number of chars that shall be displayed in the notification body.
#define GONNECT_NOTIFICATION_MAX_MESSAGE_LENGTH 200

Notification::Notification(const QString &title, const QString &body, Priority priority,
                           QObject *parent)
    : QObject(parent), m_title(title), m_body(body), m_priority(priority)
{
    m_id = QUuid::createUuid().toString(QUuid::WithoutBraces).toUtf8();

    if (m_body.length() > GONNECT_NOTIFICATION_MAX_MESSAGE_LENGTH) {
        m_body.truncate(GONNECT_NOTIFICATION_MAX_MESSAGE_LENGTH - 3);
        m_body += "...";
    }
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

bool Notification::hasThemedIcon() const
{
    NotificationIcon icon(m_iconUri, m_emblemUri, m_roundedIcon);
    return !icon.imageName().isEmpty();
}

QByteArray Notification::iconData() const
{
    NotificationIcon icon(m_iconUri, m_emblemUri, m_roundedIcon);
    return icon.data();
}

QString Notification::priority() const
{
    return QMetaEnum::fromType<Priority>().valueToKey(m_priority);
}

Notification::~Notification()
{
    NotificationManager::instance().remove(m_id);
}
