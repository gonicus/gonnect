#pragma once

#include <QObject>
#include "PresenceState.h"

class IPresenceStateProvider : public QObject
{
    Q_OBJECT

public:
    explicit IPresenceStateProvider(QObject *parent = nullptr);

    PresenceState::State presenceState() const { return m_presenceState; }
    QString stateText() const { return m_stateText; }

protected:
    void setPresenceState(PresenceState::State state);
    void setStateText(const QString &text);

private:
    PresenceState::State m_presenceState = PresenceState::State::Unknown;
    QString m_stateText;

Q_SIGNALS:
    void presenceStateChanged();
    void stateTextChanged();
};
