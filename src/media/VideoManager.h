#pragma once

#include <QObject>
#include <QTimer>
#include <QQmlEngine>
#include <qqmlintegration.h>

#include "AppSettings.h"
#include "VideoDevice.h"
#include "CameraSource.h"

#include <memory>

class VideoManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QList<VideoDevice> devices READ devices NOTIFY devicesChanged FINAL)
    Q_PROPERTY(
            QString selectedDeviceId MEMBER m_selectedDeviceId NOTIFY selectedDeviceIdChanged FINAL)
    Q_PROPERTY(VideoManager::CameraAccessState cameraAccessState READ cameraAccessState NOTIFY
                       cameraAccessStateChanged FINAL)
    Q_PROPERTY(bool isCameraAccessDenied READ isCameraAccessDenied NOTIFY cameraAccessStateChanged
                       FINAL)

public:
    enum class CameraAccessState {
        Unknown,
        Pending,
        Granted, // Either specifically granted or system has no restrictions at all
        Denied,
        DeniedRestartRequired // App must be restarted to apply correct state
    };
    Q_ENUM(CameraAccessState)

    static VideoManager &instance()
    {
        static VideoManager *_instance = nullptr;
        if (!_instance) {
            _instance = new VideoManager;
        }
        return *_instance;
    };

    QList<VideoDevice> devices() const { return m_devices; }
    VideoDevice selectedDevice() const;
    VideoManager::CameraAccessState cameraAccessState() const { return m_cameraAccessState; }
    bool isCameraAccessDenied() const;

    Q_INVOKABLE void acquireCameras();

public Q_SLOTS:
    void updateDevices();

private Q_SLOTS:
    void updateProfile();

private:
    explicit VideoManager(QObject *parent = nullptr);
    void setCameraSource(std::unique_ptr<CameraSource> source);
    bool isDeviceAvailable(const QString &deviceId) const;
    VideoDevice deviceById(const QString &deviceId) const;
    void setCameraAccessState(CameraAccessState value);

    QTimer m_debounceUpdateTimer;
    std::unique_ptr<CameraSource> m_cameraSource;

    QList<VideoDevice> m_devices;

    QString m_selectedDeviceId;
    std::unique_ptr<AppSettings> m_settings = nullptr;
    unsigned m_currentProfile = 0;

    bool m_portalPending = false;
    CameraAccessState m_cameraAccessState = CameraAccessState::Unknown;

Q_SIGNALS:
    void devicesChanged();
    void selectedDeviceIdChanged();
    void cameraAccessStateChanged();
};

class VideoManagerWrapper
{
    Q_GADGET
    QML_FOREIGN(VideoManager)
    QML_NAMED_ELEMENT(VideoManager)
    QML_SINGLETON

public:
    static VideoManager *create(QQmlEngine *, QJSEngine *)
    {
        QQmlEngine::setObjectOwnership(&VideoManager::instance(), QQmlEngine::CppOwnership);
        return &VideoManager::instance();
    }

private:
    VideoManagerWrapper() = default;
};
