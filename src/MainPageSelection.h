#pragma once

#include <QObject>
#include <QString>
#include <qqmlregistration.h>

struct MainPageSelection
{
    Q_GADGET
    QML_NAMED_ELEMENT(MainPageSelection)
    QML_VALUE_TYPE(mainPageSelection)
    QML_STRUCTURED_VALUE
    Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")

    Q_PROPERTY(MainPageSelection::PageType type MEMBER type)
    Q_PROPERTY(QString id MEMBER id)
    Q_PROPERTY(QObject *attachedData MEMBER attachedData)

public:
    enum class PageType { Base, Call, Chats, Conference, Settings };
    Q_ENUM(PageType)

    bool operator==(const MainPageSelection &) const = default;

    PageType type = PageType::Base;
    QString id;
    QObject *attachedData = nullptr;
};
