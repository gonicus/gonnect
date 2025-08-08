#pragma once

#include <QObject>
#include <QTimer>
#include <QMediaDevices>
#include <QCameraDevice>
#include <QQmlEngine>
#include <qqmlintegration.h>

#include "AppSettings.h"

class VideoManager : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QList<QCameraDevice> devices READ devices NOTIFY devicesChanged FINAL)
    Q_PROPERTY(
            QString selectedDeviceId MEMBER m_selectedDeviceId NOTIFY selectedDeviceIdChanged FINAL)

public:
    static VideoManager &instance()
    {
        static VideoManager *_instance = nullptr;
        if (!_instance) {
            _instance = new VideoManager;
        }
        return *_instance;
    };

    QList<QCameraDevice> devices() const { return m_devices; }
    QCameraDevice selectedDevice() const;

public slots:
    void updateDevices();

private slots:
    void updateProfile();

private:
    explicit VideoManager(QObject *parent = nullptr);
    bool isDeviceAvailable(const QString &deviceId) const;
    QCameraDevice deviceById(const QString &deviceId) const;

    QTimer m_debounceUpdateTimer;
    QMediaDevices m_mediaDevices;
    QList<QCameraDevice> m_devices;
    QString m_selectedDeviceId;
    std::unique_ptr<AppSettings> m_settings = nullptr;
    unsigned m_currentProfile = 0;

signals:
    void devicesChanged();
    void selectedDeviceIdChanged();
};

class VideoManagerWrapper
{
    Q_GADGET
    QML_FOREIGN(VideoManager)
    QML_NAMED_ELEMENT(VideoManager)
    QML_SINGLETON

public:
    static VideoManager *create(QQmlEngine *, QJSEngine *) { return &VideoManager::instance(); }

private:
    VideoManagerWrapper() = default;
};
