#pragma once

#include <QObject>
#include <QQmlEngine>
#include <pjsip/sip_msg.h>
#include "Contact.h"
#include "CallHistoryItem.h"
#include "CrossSigningSecret.h"
#include "ChatRoomProxyModel.h"
#include "ChatMessageContentUserStateChange.h"

class EnumTranslation : public QObject
{
    Q_OBJECT

public:
    static EnumTranslation &instance()
    {
        static EnumTranslation *_instance = nullptr;
        if (!_instance) {
            _instance = new EnumTranslation;
        }
        return *_instance;
    }

    QString sipStatusCode(const pjsip_status_code statusCode) const;
    Q_INVOKABLE QString sipStatusCode(const int statusCode) const;

    Q_INVOKABLE QString numberType(const Contact::NumberType numberType) const;
    Q_INVOKABLE QString callType(const CallHistoryItem::Type callType) const;
    Q_INVOKABLE QString
    crossSigningMethod(const CrossSigningSecret::CrossSigningMethod method) const;

    Q_INVOKABLE QString chatRoomSortStrategy(const ChatRoomProxyModel::SortStrategy strategy) const;

    Q_INVOKABLE QString userStateChange(const ChatMessageContentUserStateChange::State state,
                                        const QString &name) const;

    Q_INVOKABLE QString presenceState(const PresenceState::State state) const;
    Q_INVOKABLE QColor presenceStateColor(const PresenceState::State state) const;

private:
    explicit EnumTranslation(QObject *parent = nullptr);
};

class EnumTranslationWrapper
{
    Q_GADGET
    QML_FOREIGN(EnumTranslation)
    QML_NAMED_ELEMENT(EnumTranslation)
    QML_SINGLETON

public:
    static EnumTranslation *create(QQmlEngine *, QJSEngine *)
    {
        return &EnumTranslation::instance();
    }

private:
    EnumTranslationWrapper() = default;
};
