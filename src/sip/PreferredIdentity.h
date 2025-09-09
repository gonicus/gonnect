#pragma once
#include <QObject>
#include <QtQml/qqml.h>
#include <QtQml/qqmlregistration.h>
#include "AppSettings.h"

class SIPManager;

class PreferredIdentity : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("Object is managed by SIPAccount")

    Q_PROPERTY(bool isValid READ isValid NOTIFY isValidChanged FINAL)

    Q_PROPERTY(QString id READ id CONSTANT FINAL)
    Q_PROPERTY(QString displayName READ displayName WRITE setDisplayName NOTIFY displayNameChanged
                       FINAL)
    Q_PROPERTY(QString identity READ identity WRITE setIdentity NOTIFY identityChanged FINAL)
    Q_PROPERTY(QString prefix READ prefix WRITE setPrefix NOTIFY prefixChanged FINAL)
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged FINAL)
    Q_PROPERTY(bool automatic READ automatic WRITE setAutomatic NOTIFY automaticChanged FINAL)

public:
    explicit PreferredIdentity(SIPManager *manager, const QString &id);
    PreferredIdentity(const PreferredIdentity &other);
    virtual ~PreferredIdentity();

    QString id() const { return m_id; }

    void setDisplayName(const QString &value);
    QString displayName() const { return m_displayName; }

    void setIdentity(const QString &value);
    QString identity() const { return m_identity; }

    void setPrefix(const QString &value);
    QString prefix() const { return m_prefix; }

    void setEnabled(bool flag);
    bool enabled() const { return m_enabled; }

    void setAutomatic(bool flag);
    bool automatic() const { return m_automatic; }

    bool isValid() const { return m_isValid; }

Q_SIGNALS:
    void displayNameChanged();
    void identityChanged();
    void prefixChanged();
    void enabledChanged();
    void automaticChanged();
    void isValidChanged();

private Q_SLOTS:
    void validate();

private:
    AppSettings m_settings;

    QString m_displayName;
    QString m_prefix;
    QString m_identity;

    QString m_id;

    bool m_isValid = false;
    bool m_enabled = true;
    bool m_automatic = true;
};
