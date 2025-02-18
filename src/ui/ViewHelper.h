#pragma once

#include <QObject>
#include <QQmlEngine>

#include "appversion.h"

#include "Application.h"

class Ringer;
class HeadsetDeviceProxy;

class ViewHelper : public QObject
{
    Q_OBJECT

    Q_PROPERTY(QString userConfigPath READ userConfigPath CONSTANT FINAL)
    Q_PROPERTY(bool isDebugRun READ isDebugRun CONSTANT FINAL)
    Q_PROPERTY(bool isPlayingRingTone READ isPlayingRingTone NOTIFY isPlayingRingToneChanged FINAL)

public:
    static ViewHelper &instance()
    {
        static ViewHelper *_instance = nullptr;
        if (!_instance) {
            _instance = new ViewHelper;
        }
        return *_instance;
    }

    Q_INVOKABLE QString appVersion() const { return QString::fromStdString(getVersion()); }

    Q_INVOKABLE bool isSystrayAvailable() const;

    bool isDebugRun() const
    {
        return qobject_cast<Application *>(Application::instance())->isDebugRun();
    }
    Q_INVOKABLE void downloadDebugInformation() const;

    QString userConfigPath() const
    {
        return "file:///" + QStandardPaths::writableLocation(QStandardPaths::ConfigLocation)
                + "/gonnect/99-user.conf";
    }

    Q_INVOKABLE QString secondsToNiceText(int seconds) const;
    Q_INVOKABLE int secondsDelta(const QDateTime &start, const QDateTime &end) const;
    Q_INVOKABLE void copyToClipboard(const QString &str) const;
    Q_INVOKABLE void reloadAddressBook() const;

    /// Returns the contact id for the phone number if such contact exists or empty string
    Q_INVOKABLE QString contactIdByNumber(const QString &phoneNumber) const;

    /// List of file name filters for supported audio files (as used by a file picker dialog)
    Q_INVOKABLE QStringList audioFileSelectors() const;

    Q_INVOKABLE void toggleFavorite(const QString &phoneNumber) const;

    /// Creates initials (two letters) out of the given name
    Q_INVOKABLE QString initials(const QString &name) const;

    Q_INVOKABLE void testPlayRingTone(qreal volume);
    Q_INVOKABLE void stopTestPlayRingTone();
    bool isPlayingRingTone() const { return m_isPlayingRingTone; }

    Q_INVOKABLE void quitApplicationNoConfirm() const;

    Q_INVOKABLE void resetTrayIcon() const;

    Q_INVOKABLE HeadsetDeviceProxy *headsetDeviceProxy() const;

    Q_INVOKABLE QString encryptSecret(const QString &secret) const;

    void requestLdapPassword(const QString &id, const QString &host);
    Q_INVOKABLE void respondLdapPassword(const QString &id, const QString password);

    void requestCardDavPassword(const QString &id, const QString &host);
    Q_INVOKABLE void respondCardDavPassword(const QString &id, const QString password);

public slots:
    Q_INVOKABLE void quitApplication();

private:
    explicit ViewHelper(QObject *parent = nullptr);

    bool m_isPlayingRingTone = false;
    Ringer *m_ringer = nullptr;
    QTimer m_ringerTimer;

signals:
    void activateSearch();
    void showSettingsWindow();
    void showAboutWindow();
    void isPlayingRingToneChanged();
    void showAudioSettings();
    void showQuitConfirm();
    void showEmergency(QString accountId, int callId, QString displayName);
    void hideEmergency();

    void ldapPasswordRequested(QString id, QString host);
    void ldapPasswordResponded(QString id, QString password);

    void cardDavPasswordRequested(QString id, QString host);
    void cardDavPasswordResponded(QString id, QString password);
};

class ViewHelperWrapper
{
    Q_GADGET
    QML_FOREIGN(ViewHelper)
    QML_NAMED_ELEMENT(ViewHelper)
    QML_SINGLETON

public:
    static ViewHelper *create(QQmlEngine *, QJSEngine *) { return &ViewHelper::instance(); }

private:
    ViewHelperWrapper() = default;
};
