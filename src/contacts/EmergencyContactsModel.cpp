#include "EmergencyContactsModel.h"
#include "EmergencyContact.h"
#include "GlobalInfo.h"

EmergencyContactsModel::EmergencyContactsModel(QObject *parent) : QAbstractListModel{ parent } { }

QHash<int, QByteArray> EmergencyContactsModel::roleNames() const
{
    return {
        { static_cast<int>(Roles::Index), "index" },
        { static_cast<int>(Roles::Number), "number" },
        { static_cast<int>(Roles::DisplayName), "displayName" },
    };
}

int EmergencyContactsModel::rowCount(const QModelIndex &) const
{
    return GlobalInfo::instance().emergencyContacts().size();
}

QVariant EmergencyContactsModel::data(const QModelIndex &index, int role) const
{
    const auto contact = GlobalInfo::instance().emergencyContacts().at(index.row());

    switch (role) {
    case static_cast<int>(Roles::Index):
        return contact->index();

    case static_cast<int>(Roles::Number):
        return contact->number();

    case static_cast<int>(Roles::DisplayName):
    default:
        return contact->displayName();
    }
}
