#pragma once

#include <QObject>

#ifdef Q_OS_LINUX
#  include <pulse/pulseaudio.h>
#endif

#include <QMediaDevices>
#include <QTimer>
#include <QtQml/qqml.h>
#include <QtQml/qqmlregistration.h>

#include "AppSettings.h"
#include "SIPAudioDevice.h"

class AudioPort;
class QAudioOutput;
class QAudioInput;

class AudioManager : public QObject
{
    Q_OBJECT
    Q_DISABLE_COPY(AudioManager)

    Q_PROPERTY(QList<SIPAudioDevice *> devices READ devices NOTIFY devicesChanged FINAL)
    Q_PROPERTY(QString playbackDeviceId READ playbackDeviceId WRITE setPlaybackDeviceId NOTIFY
                       playbackDeviceIdChanged FINAL)
    Q_PROPERTY(QString captureDeviceId READ captureDeviceId WRITE setCaptureDeviceId NOTIFY
                       captureDeviceIdChanged FINAL)
    Q_PROPERTY(QString ringDeviceId READ ringDeviceId WRITE setRingDeviceId NOTIFY
                       ringDeviceIdChanged FINAL)
    Q_PROPERTY(unsigned currentProfile READ currentProfile NOTIFY currentProfileChanged FINAL)

    Q_PROPERTY(qreal captureAudioVolume READ captureAudioVolume WRITE setCaptureAudioVolume NOTIFY
                       captureAudioVolumeChanged FINAL)
    Q_PROPERTY(qreal playbackAudioVolume READ playbackAudioVolume WRITE setPlaybackAudioVolume
                       NOTIFY playbackAudioVolumeChanged FINAL)
    Q_PROPERTY(bool hasCaptureAudioLevel READ hasCaptureAudioLevel NOTIFY
                       hasCaptureAudioLevelChanged FINAL)
    Q_PROPERTY(bool isAudioCaptureMuted MEMBER m_isAudioCaptureMuted NOTIFY
                       isAudioCaptureMutedChanged FINAL)

public:
    Q_REQUIRED_RESULT static AudioManager &instance()
    {
        static AudioManager *_instance = nullptr;

        if (_instance == nullptr) {
            _instance = new AudioManager();
        }

        return *_instance;
    }

    QList<SIPAudioDevice *> devices() const { return m_devices; }

    void initialize();

#ifdef Q_OS_LINUX
    static void paSubscriptionEventCallback(pa_context *context, pa_subscription_event_type_t type,
                                            uint32_t index, void *userdata);
    static void paContextStateCallback(pa_context *context, void *userdata);
    void paMuteInputByName(const QString &name, bool state);
    void paGetInputMuteState(pa_context *context);
    static void paInputMuteStateCallback(pa_context *context, const pa_source_info *source, int end,
                                         void *userdata);
#endif

    pj::AudioMedia &getPlaybackDevMedia() const;
    pj::AudioMedia &getCaptureDevMedia() const;

    void setPlaybackDeviceId(const QString &id);
    QString playbackDeviceId() const { return m_playbackHash; }

    void setCaptureDeviceId(const QString &id);
    QString captureDeviceId() const { return m_captureHash; }

    void setRingDeviceId(const QString &id);
    QString ringDeviceId() const { return m_ringHash; }

    void setExternalRinger(bool flag);
    bool externalRinger() const { return m_externalRinger; }

    QAudioOutput *getQtAudioOutputForHash(const QString &id);
    QAudioInput *getQtAudioInputForHash(const QString &id);

    bool hasCaptureAudioLevel() const;

    /// The set volume of the audio capture device, between 0.0 and 1.0
    qreal captureAudioVolume() const;
    void setCaptureAudioVolume(qreal volume);
    qreal playbackAudioVolume() const;
    void setPlaybackAudioVolume(qreal volume);

    bool isAudioCaptureMuted() const { return m_isAudioCaptureMuted; }

    unsigned currentProfile() const { return m_currentAudioProfile; }

    ~AudioManager();

Q_SIGNALS:
    void noMatchingAudioProfile();
    void matchingAudioProfile();
    void currentProfileChanged();
    void devicesChanged();
    void playbackDeviceIdChanged();
    void captureDeviceIdChanged();
    void ringDeviceIdChanged();

    void captureAudioVolumeChanged();
    void playbackAudioVolumeChanged();
    void hasCaptureAudioLevelChanged();
    void isAudioCaptureMutedChanged();
    void externalRingerChanged();

private Q_SLOTS:
#ifdef Q_OS_LINUX
    void paMainloopIterate();
#endif

private:
    bool noSyncSystemMute() { return m_settings.value("generic/useHeadset", true).toBool(); }

    AudioManager(QObject *parent = nullptr);
    void refreshAudioDevices();
    void doProfileElection();
    bool isDeviceAvailable(const QString &hash);

    bool m_isAudioCaptureMuted = false;

    QString m_playbackHash;
    QString m_captureHash;
    QString m_ringHash;

    AudioPort *m_playbackAudioPort = nullptr;
    AudioPort *m_captureAudioPort = nullptr;

    unsigned m_captureDeviceId = 0;
    unsigned m_currentAudioProfile = 0;

    QTimer m_updateDebouncer;
    QList<SIPAudioDevice *> m_devices;

    QMediaDevices *m_mediaDevices = nullptr;

    AppSettings m_settings;

    bool m_externalRinger = false;

#ifdef Q_OS_LINUX
    pa_mainloop *m_paMainloop = nullptr;
    QTimer m_paMainloopTimer;
    pa_context *m_paContext = nullptr;
#endif
};

class AudioManagerWrapper
{
    Q_GADGET
    QML_FOREIGN(AudioManager)
    QML_NAMED_ELEMENT(AudioManager)
    QML_SINGLETON

public:
    static AudioManager *create(QQmlEngine *, QJSEngine *) { return &AudioManager::instance(); }

private:
    AudioManagerWrapper() = default;
};
