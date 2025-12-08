#include "DateEventsModel.h"
#include "DateEventManager.h"
#include "DateEvent.h"

DateEventsModel::DateEventsModel(QObject *parent) : QAbstractListModel{ parent }
{
    auto &manager = DateEventManager::instance();

    connect(&manager, &DateEventManager::dateEventsCleared, this, [this]() {
        beginResetModel();
        endResetModel();
    });

    connect(&manager, &DateEventManager::dateEventAdded, this, [this](qsizetype, DateEvent *) {
        beginResetModel();
        endResetModel();
    });

    connect(&manager, &DateEventManager::dateEventModified, this, [this]() {
        beginResetModel();
        endResetModel();
    });

    connect(&manager, &DateEventManager::dateEventRemoved, this, [this](qsizetype) {
        beginResetModel();
        endResetModel();
    });
}

QHash<int, QByteArray> DateEventsModel::roleNames() const
{
    return {
        { static_cast<int>(Roles::DateTime), "dateTime" },
        { static_cast<int>(Roles::Date), "date" },
        { static_cast<int>(Roles::EndDateTime), "endDateTime" },
        { static_cast<int>(Roles::Summary), "summary" },
        { static_cast<int>(Roles::RoomName), "roomName" },
        { static_cast<int>(Roles::IsJitsiMeeting), "isJitsiMeeting" },
        { static_cast<int>(Roles::IsOtherLink), "isOtherLink" },
    };
}

int DateEventsModel::rowCount(const QModelIndex &) const
{
    return DateEventManager::instance().dateEvents().size();
}

QVariant DateEventsModel::data(const QModelIndex &index, int role) const
{
    const auto dateEvent = q_check_ptr(DateEventManager::instance().dateEvents().at(index.row()));

    switch (role) {
    case static_cast<int>(Roles::DateTime):
        return dateEvent->start();

    case static_cast<int>(Roles::Date):
        return dateEvent->start().date();

    case static_cast<int>(Roles::EndDateTime):
        return dateEvent->end();

    case static_cast<int>(Roles::RoomName):
        return dateEvent->roomName();

    case static_cast<int>(Roles::IsJitsiMeeting):
        return dateEvent->isJitsiMeeting();

    case static_cast<int>(Roles::IsOtherLink):
        return dateEvent->isOtherLink();

    case static_cast<int>(Roles::Summary):
    default:
        return dateEvent->summary();
    }
}
