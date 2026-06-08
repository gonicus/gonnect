#pragma once

#include <QObject>
#include <QQmlEngine>
#include "PresenceState.h"

class GlobalStateAggregator : public QObject
{
    Q_OBJECT

    Q_PROPERTY(PresenceState::State presenceState MEMBER m_presenceState NOTIFY presenceStateChanged
                       FINAL)
    Q_PROPERTY(QString statusText MEMBER m_statusText NOTIFY statusTextChanged FINAL)

public:
    static GlobalStateAggregator &instance()
    {
        static GlobalStateAggregator *_instance = nullptr;
        if (!_instance) {
            _instance = new GlobalStateAggregator;
        }
        return *_instance;
    };

    PresenceState::State presenceState() const { return m_presenceState; }
    QString statusText() const { return m_statusText; }

private:
    explicit GlobalStateAggregator(QObject *parent = nullptr);

    PresenceState::State m_presenceState = PresenceState::State::Available;
    QString m_statusText;

Q_SIGNALS:
    void presenceStateChanged();
    void statusTextChanged();
};

class GlobalStateAggregatorWrapper
{
    Q_GADGET
    QML_FOREIGN(GlobalStateAggregator)
    QML_NAMED_ELEMENT(GlobalStateAggregator)
    QML_SINGLETON

public:
    static GlobalStateAggregator *create(QQmlEngine *, QJSEngine *)
    {
        return &GlobalStateAggregator::instance();
    }

private:
    GlobalStateAggregatorWrapper() = default;
};
