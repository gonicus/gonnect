#pragma once

#include <QObject>
#include <qqmlintegration.h>

class ICallState : public QObject
{
    Q_OBJECT
    Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")
    QML_ELEMENT
    QML_UNCREATABLE("")

    Q_PROPERTY(ICallState::States callState READ callState NOTIFY callStateChanged FINAL)

public:
    enum class State {
        Idle = 0,
        RingingIncoming = 1 << 0,
        RingingOutgoing = 1 << 1,
        CallActive = 1 << 2,
        AudioActive = 1 << 3,
        VideoActive = 1 << 4,
        OnHold = 1 << 5,
        SharingScreen = 1 << 6,
        Migrating = 1 << 7,
        KnockingIncoming = 1 << 8
    };
    Q_ENUM(State)
    Q_DECLARE_FLAGS(States, State)
    Q_FLAG(States)

    static QString callStateToString(const ICallState::State &state);
    static QList<QByteArray> statesAsStrings(const ICallState::States &states);

    QString uuid() const { return m_id; }

    ICallState(QObject *parent = nullptr);

    const ICallState::States callState() const { return m_callState; }

    void toggleHold();

protected:
    void setCallState(const ICallState::States &callState);
    void addCallState(const ICallState::States &states);
    void removeCallState(const ICallState::States &states);

    virtual void toggleHoldImpl() = 0;

private:
    States m_callState = State::Idle;
    QString m_id;

signals:
    void callStateChanged();
};

Q_DECLARE_OPERATORS_FOR_FLAGS(ICallState::States)
