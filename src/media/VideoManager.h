#pragma once

#include <QObject>
#include <QTimer>
#include <QMediaDevices>
#include <QCameraDevice>
#include <QQmlEngine>
#include <qqmlintegration.h>

#include "AppSettings.h"
#include "VideoDevice.h"

#ifdef Q_OS_FLATPAK
#  include "linux/PipeWireCameraDevices.h"
#endif

class VideoManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QList<VideoDevice> devices READ devices NOTIFY devicesChanged FINAL)
    Q_PROPERTY(
            QString selectedDeviceId MEMBER m_selectedDeviceId NOTIFY selectedDeviceIdChanged FINAL)
    Q_PROPERTY(VideoManager::CameraAccessState cameraAccessState READ cameraAccessState NOTIFY
                       cameraAccessStateChanged FINAL)

public:
    enum class CameraAccessState {
        Unknown,
        Pending,
        Granted, // Either specifically granted or system has no restriction
        Denied
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

    Q_INVOKABLE void acquireCameras();

public Q_SLOTS:
    void updateDevices();

private Q_SLOTS:
    void updateProfile();

private:
    explicit VideoManager(QObject *parent = nullptr);
    bool isDeviceAvailable(const QString &deviceId) const;
    VideoDevice deviceById(const QString &deviceId) const;
    QList<VideoDevice> devicesFromMediaDevices() const;
    void setCameraAccessState(CameraAccessState value);

    QTimer m_debounceUpdateTimer;
    QMediaDevices m_mediaDevices;

    QList<VideoDevice> m_devices;
#ifdef Q_OS_FLATPAK
    PipeWireCameraDevices m_pipewireCameras;
#endif

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
