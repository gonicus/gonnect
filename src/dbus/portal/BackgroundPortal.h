#pragma once

#include "AbstractPortal.h"

#define BACKGROUND_PORTAL_INTERFACE "org.freedesktop.portal.Background"

class BackgroundPortal : public AbstractPortal
{
    Q_OBJECT
    Q_DISABLE_COPY(BackgroundPortal)

public:
    explicit BackgroundPortal(QObject *parent = nullptr);
    ~BackgroundPortal() = default;

    /**
     * Request background operation. The callback response contains:
     *
     * background (b)
     * true if the application is allowed to run in the background.
     *
     * autostart (b)
     * true if the application is will be autostarted.
     */
    void RequestBackground(bool autostart, bool useDBusActivation, PortalResponse callback);

    void SetStatus(const QString &message);
};
