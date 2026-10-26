#pragma once

#include <QObject>
#include <QString>
#include <qqmlregistration.h>

namespace MainPageSelectionEnums {
Q_NAMESPACE
QML_NAMED_ELEMENT(MainPageSelection)
Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")

enum class PageType { Base, Call, Chats, Conference, Settings, Emergency };
Q_ENUM_NS(PageType)
}

struct MainPageSelection
{
    Q_GADGET
    QML_VALUE_TYPE(mainPageSelection)
    QML_STRUCTURED_VALUE

    Q_PROPERTY(MainPageSelectionEnums::PageType type MEMBER type)
    Q_PROPERTY(QString id MEMBER id)
    Q_PROPERTY(QObject *attachedData MEMBER attachedData)

public:
    bool operator==(const MainPageSelection &) const = default;

    using PageType = MainPageSelectionEnums::PageType;
    PageType type = PageType::Base;

    QString id;
    QObject *attachedData = nullptr;
};
