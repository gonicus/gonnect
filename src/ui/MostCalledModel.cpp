#include "MostCalledModel.h"
#include "NumberStats.h"
#include "PhoneNumberUtil.h"

MostCalledModel::MostCalledModel(QObject *parent) : QAbstractListModel{ parent }
{
    auto &numStats = NumberStats::instance();

    connect(&numStats, &NumberStats::numberStatAdded, this, &MostCalledModel::updateModel);
    connect(&numStats, &NumberStats::countChanged, this, &MostCalledModel::updateModel);

    updateModel();
}

void MostCalledModel::updateModel()
{
    beginResetModel();
    m_phoneNumbers = NumberStats::instance().mostCalled(5, false);
    endResetModel();
}

QHash<int, QByteArray> MostCalledModel::roleNames() const
{
    return {
        { static_cast<int>(Roles::Name), "name" },
        { static_cast<int>(Roles::PhoneNumber), "phoneNumber" },
        { static_cast<int>(Roles::HasBuddyState), "hasBuddyState" },
        { static_cast<int>(Roles::NumberType), "numberType" },
    };
}

int MostCalledModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return m_phoneNumbers.size();
}

QVariant MostCalledModel::data(const QModelIndex &index, int role) const
{
    const auto phoneNumber = m_phoneNumbers.at(index.row());
    const auto contactInfo = PhoneNumberUtil::instance().contactInfoBySipUrl(phoneNumber);

    switch (role) {
    case static_cast<int>(Roles::HasBuddyState):
        return contactInfo.isSipSubscriptable;

    case static_cast<int>(Roles::NumberType):
        return static_cast<int>(contactInfo.numberType);

    case static_cast<int>(Roles::PhoneNumber):
        return phoneNumber;
    }

    return contactInfo.contact ? contactInfo.contact->name() : "";
}
