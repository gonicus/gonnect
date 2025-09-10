#pragma once

#include <QObject>
#include <QQmlEngine>
#include <qqmlintegration.h>

class GlobalMuteState : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")

    Q_PROPERTY(bool isMuted READ isMuted NOTIFY isMutedChanged FINAL)

public:
    static GlobalMuteState &instance()
    {
        static GlobalMuteState *_instance = nullptr;
        if (!_instance) {
            _instance = new GlobalMuteState;
        }
        return *_instance;
    }

    bool isMuted() const { return m_isMuted; }
    void reset();

    Q_INVOKABLE void toggleMute(const QString &tag = "");

private:
    explicit GlobalMuteState(QObject *parent = nullptr);

    bool m_isMuted = false;

Q_SIGNALS:
    void isMutedChanged();
    void isMutedChangedWithTag(bool isMuted, const QString tag);
};

class GlobalMuteStateWrapper
{
    Q_GADGET
    QML_FOREIGN(GlobalMuteState)
    QML_NAMED_ELEMENT(GlobalMuteState)
    QML_SINGLETON

public:
    static GlobalMuteState *create(QQmlEngine *, QJSEngine *)
    {
        return &GlobalMuteState::instance();
    }

private:
    GlobalMuteStateWrapper() = default;
};
