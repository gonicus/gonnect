#pragma once

#include <QObject>
#include <QQmlEngine>

class IConferenceConnector;

class VideoCallHelper : public QObject
{
    Q_OBJECT

    Q_PROPERTY(
            bool hasActiveVideoCall READ hasActiveVideoCall NOTIFY hasActiveVideoCallChanged FINAL)
    Q_PROPERTY(QString activeVideoCallUrl READ activeVideoCallUrl NOTIFY activeVideoCallUrlChanged
                       FINAL)

public:
    static VideoCallHelper &instance()
    {
        static VideoCallHelper _instance;
        return _instance;
    }

    Q_INVOKABLE void joinOrStartConfernece(const QString &url) const;
    Q_INVOKABLE bool hasMatchingConferenceConnector(const QString &url) const;
    Q_INVOKABLE IConferenceConnector *matchingConferenceConnector(const QString &url) const;

private:
    explicit VideoCallHelper(QObject *parent = nullptr);

    QUrl normalizeUrl(const QUrl &url) const;
    bool isPrefix(const QUrl &url, const QUrl &possiblePrefix) const;
    QString remainderPath(const QUrl &url, const QUrl &prefix) const;

    bool hasActiveVideoCall() const { return !m_activeVideoCallUrl.isEmpty(); }
    QString activeVideoCallUrl() const { return m_activeVideoCallUrl; }

    QString m_activeVideoCallUrl;

private Q_SLOTS:
    void updateActiveVideoCall();

Q_SIGNALS:
    void hasActiveVideoCallChanged();
    void activeVideoCallUrlChanged();
};

class VideoCallHelperWrapper
{
    Q_GADGET
    QML_FOREIGN(VideoCallHelper)
    QML_NAMED_ELEMENT(VideoCallHelper)
    QML_SINGLETON

public:
    static VideoCallHelper *create(QQmlEngine *, QJSEngine *)
    {
        QQmlEngine::setObjectOwnership(&VideoCallHelper::instance(), QQmlEngine::CppOwnership);
        return &VideoCallHelper::instance();
    }

private:
    VideoCallHelperWrapper() = default;
};
