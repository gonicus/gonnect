#include "ContactSourceInfoModel.h"
#include "AddressBook.h"

ContactSourceInfoModel::ContactSourceInfoModel(QObject *parent) : QAbstractListModel{ parent }
{
    connect(&AddressBook::instance(), &AddressBook::contactSourceInfosChanged, this, [this]() {
        beginResetModel();
        endResetModel();
    });
}

QHash<int, QByteArray> ContactSourceInfoModel::roleNames() const
{
    return {
        { static_cast<int>(Roles::Priority), "priority" },
        { static_cast<int>(Roles::DisplayName), "displayName" },
    };
}

int ContactSourceInfoModel::rowCount(const QModelIndex &) const
{
    return AddressBook::instance().sortedSourceInfos().size();
}

QVariant ContactSourceInfoModel::data(const QModelIndex &index, int role) const
{
    const auto &info = AddressBook::instance().sortedSourceInfos().at(index.row());

    switch (role) {
    case static_cast<int>(Roles::Priority):
        return info.prio;

    case static_cast<int>(Roles::DisplayName):
    default:
        return info.displayName;
    }
}
