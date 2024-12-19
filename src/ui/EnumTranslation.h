#pragma once

#include <QObject>
#include <QQmlEngine>
#include <pjsip/sip_msg.h>
#include "Contact.h"
#include "CallHistoryItem.h"

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
