#pragma once

#include <QObject>
#include <QQmlEngine>

#include "Contact.h"

class ContactHelper : public QObject
{
    Q_OBJECT

public:
    static ContactHelper &instance()
    {
        static ContactHelper _instance;
        return _instance;
    }

    Q_INVOKABLE Contact *lookupByChatUser(const ChatUser *chatUser) const;

private:
    explicit ContactHelper(QObject *parent = nullptr);
};

class ContactHelperWrapper
{
    Q_GADGET
    QML_FOREIGN(ContactHelper)
    QML_NAMED_ELEMENT(ContactHelper)
    QML_SINGLETON

public:
    static ContactHelper *create(QQmlEngine *, QJSEngine *)
    {
        QQmlEngine::setObjectOwnership(&ContactHelper::instance(), QQmlEngine::CppOwnership);
        return &ContactHelper::instance();
    }

private:
    ContactHelperWrapper() = default;
};
