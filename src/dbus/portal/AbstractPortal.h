#pragma once

#include <QObject>
#include <QDBusMessage>
#include <QDBusInterface>
#include "PortalSession.h"

#define FREEDESKTOP_DBUS_PORTAL_PATH "/org/freedesktop/portal/desktop"
#define FREEDESKTOP_DBUS_PORTAL_SERVICE "org.freedesktop.portal.Desktop"

typedef std::function<void(uint code, const QVariantMap &response)> PortalResponse;

class PortalResponseTracker : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(PortalResponseTracker)

public:
    explicit PortalResponseTracker(PortalResponse callback, QObject *parent);
    ~PortalResponseTracker() = default;

private:
    PortalResponse m_callback;

private Q_SLOTS:
    void responseReceived(uint state, const QVariantMap &map);
};

class AbstractPortal : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString version READ version CONSTANT)

    Q_DISABLE_COPY(AbstractPortal)

public:
    explicit AbstractPortal(const QString &service, const QString &path, const QString &interface,
                            QObject *parent = nullptr);
    ~AbstractPortal() = default;

    enum ResponseCode { SUCCESS = 0, CANCELED = 1, ERROR = 2 };
    Q_ENUM(ResponseCode)

    bool isValid() { return m_interface->isValid() && !version().isEmpty(); }

    inline QString version() const { return m_interface->property("version").toString(); }

protected:
    QString portalCall(const QString &handle, const QDBusMessage &message, PortalResponse callback);
    QString generateHandleToken() const;
    QString parentId() const;

    QPointer<OrgFreedesktopPortalSessionInterface> m_session;
    QPointer<QDBusInterface> m_interface;

private:
    QString replyPath(const QString &token) const;
};
