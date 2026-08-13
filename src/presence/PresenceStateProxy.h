#pragma once

#include <QObject>
#include <QQmlEngine>

#include "PresenceState.h"
#include "Contact.h"

class PresenceStateProxy : public QObject
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString contactId MEMBER m_contactId NOTIFY contactIdChanged FINAL)
    Q_PROPERTY(
            PresenceState::State presenceState READ presenceState NOTIFY presenceStateChanged FINAL)
    Q_PROPERTY(QString stateText READ stateText NOTIFY stateTextChanged FINAL)

public:
    explicit PresenceStateProxy(QObject *parent = nullptr);
    ~PresenceStateProxy();

    PresenceState::State presenceState() const;
    QString stateText() const;

private:
    QString m_contactId;
    Contact *m_contact = nullptr;
    PresenceStateAggregator *m_aggregator = nullptr;
    QObject *m_context = nullptr;

private Q_SLOTS:
    void updateStateObject();

Q_SIGNALS:
    void contactIdChanged();
    void presenceStateChanged();
    void stateTextChanged();
};
