#include <QLoggingCategory>
#include "SIPManager.h"
#include "PreferredIdentity.h"
#include "PreferredIdentityValidator.h"

Q_LOGGING_CATEGORY(lcSIPIdentity, "gonnect.sip.identity")

PreferredIdentity::PreferredIdentity(SIPManager *manager, const QString &id)
    : QObject(manager), m_id(id)
{
    m_settings.beginGroup(id);
    m_displayName = m_settings.value("name").toString();
    m_identity = m_settings.value("identity").toString();
    m_prefix = m_settings.value("prefix").toString();
    m_automatic = m_settings.value("automatic", true).toBool();
    m_enabled = m_settings.value("enabled", true).toBool();

    validate();

    connect(this, &PreferredIdentity::displayNameChanged, this, &PreferredIdentity::validate);
    connect(this, &PreferredIdentity::identityChanged, this, &PreferredIdentity::validate);
    connect(this, &PreferredIdentity::prefixChanged, this, &PreferredIdentity::validate);
    connect(this, &PreferredIdentity::enabledChanged, this, &PreferredIdentity::validate);
    connect(this, &PreferredIdentity::automaticChanged, this, &PreferredIdentity::validate);
}

PreferredIdentity::PreferredIdentity(const PreferredIdentity &other) : QObject()
{
    m_id = other.m_id;
    m_displayName = other.m_displayName;
    m_identity = other.m_identity;
    m_prefix = other.m_prefix;
    m_automatic = other.m_automatic;
    m_enabled = other.m_enabled;
    validate();
}

PreferredIdentity::~PreferredIdentity() { }

void PreferredIdentity::setDisplayName(const QString &value)
{
    if (value != m_displayName) {
        m_displayName = value;
        Q_EMIT displayNameChanged();

        m_settings.setValue("name", m_displayName);
    }
}

void PreferredIdentity::setIdentity(const QString &value)
{
    if (value != m_identity) {
        m_identity = value;
        Q_EMIT identityChanged();

        m_settings.setValue("identity", m_identity);
    }
}

void PreferredIdentity::setPrefix(const QString &value)
{
    if (value != m_prefix) {
        m_prefix = value;
        Q_EMIT prefixChanged();
    }

    m_settings.setValue("prefix", m_prefix);
}

void PreferredIdentity::setEnabled(bool flag)
{
    if (flag != m_enabled) {
        m_enabled = flag;
        Q_EMIT enabledChanged();
    }

    m_settings.setValue("enabled", m_enabled);
}

void PreferredIdentity::setAutomatic(bool flag)
{
    if (flag != m_automatic) {
        m_automatic = flag;
        Q_EMIT automaticChanged();
    }

    m_settings.setValue("automatic", m_automatic);
}

void PreferredIdentity::validate()
{
    const bool isValid = PreferredIdentityValidator::instance().isValid(*this);

    if (isValid != m_isValid) {
        m_isValid = isValid;
        Q_EMIT isValidChanged();
    }
}
