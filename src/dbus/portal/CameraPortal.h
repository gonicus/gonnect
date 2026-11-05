#pragma once

#include "AbstractPortal.h"

#define CAMERA_PORTAL_INTERFACE "org.freedesktop.portal.Camera"

class CameraPortal : public AbstractPortal
{
    Q_OBJECT
    Q_DISABLE_COPY(CameraPortal)

    Q_PROPERTY(bool isCameraPresent READ isCameraPresent NOTIFY isCameraPresentChanged)
    Q_PROPERTY(bool hasAccess READ hasAccess NOTIFY hasAccessChanged)

public:
    Q_REQUIRED_RESULT static CameraPortal &instance()
    {
        static CameraPortal *_instance = nullptr;

        if (_instance == nullptr) {
            _instance = new CameraPortal();
        }

        return *_instance;
    }

    ~CameraPortal() = default;

    bool hasAccess() const { return m_hasAccess; }

    /**
     * Check if there is a camera available. Works without permission and parameters.
     */
    bool isCameraPresent() const { return m_isCameraPresent; }

    /**
     * Request to gain access to the camera.
     */
    void AccessCamera(PortalResponse callback);

    /**
     * Open a file descriptor to the PipeWire remote where the camera nodes
     * are available. The file descriptor should be used to create a
     * ``pw_core`` object, by using ``pw_context_connect_fd()``.
     *
     * This method will only succeed if the application already has permission
     * to access camera devices.
     */
    void OpenPipeWireRemote(std::function<void(int fd, const QString &error)> callback);

Q_SIGNALS:
    void isCameraPresentChanged();
    void hasAccessChanged();

private Q_SLOTS:
    void propertiesChanged(const QString &interfaceName, const QVariantMap &changed,
                           const QStringList &invalidated);

private:
    explicit CameraPortal(QObject *parent = nullptr);

    bool m_isCameraPresent = false;
    bool m_hasAccess = false;
};
