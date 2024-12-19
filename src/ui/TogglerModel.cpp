#include "TogglerModel.h"
#include "TogglerManager.h"
#include "Toggler.h"

TogglerModel::TogglerModel(QObject *parent) : QAbstractListModel{ parent }
{
    connect(&TogglerManager::instance(), &TogglerManager::togglerChanged, this, [this]() {
        beginResetModel();
        endResetModel();
    });

    connect(&TogglerManager::instance(), &TogglerManager::togglerActiveChanged, this,
            [this](Toggler *toggler, bool) {
                const auto idx = TogglerManager::instance().toggler().indexOf(toggler);
                if (idx >= 0) {
                    const auto modelIndex = createIndex(idx, 0);
                    emit dataChanged(modelIndex, modelIndex, { static_cast<int>(Roles::IsActive) });
                }
            });
    connect(&TogglerManager::instance(), &TogglerManager::togglerBusyChanged, this,
            [this](Toggler *toggler, bool) {
                const auto idx = TogglerManager::instance().toggler().indexOf(toggler);
                if (idx >= 0) {
                    const auto modelIndex = createIndex(idx, 0);
                    emit dataChanged(modelIndex, modelIndex, { static_cast<int>(Roles::IsBusy) });
                }
            });
}

QHash<int, QByteArray> TogglerModel::roleNames() const
{
    return {
        { static_cast<int>(Roles::Id), "id" },
        { static_cast<int>(Roles::Name), "name" },
        { static_cast<int>(Roles::Description), "description" },
        { static_cast<int>(Roles::IsActive), "isActive" },
        { static_cast<int>(Roles::IsBusy), "isBusy" },
        { static_cast<int>(Roles::Display), "display" },
    };
}

int TogglerModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return TogglerManager::instance().toggler().size();
}

QVariant TogglerModel::data(const QModelIndex &index, int role) const
{
    const auto toggler = TogglerManager::instance().toggler().at(index.row());

    switch (role) {
    case static_cast<int>(Roles::Id):
        return toggler->id();

    case static_cast<int>(Roles::Description):
        return toggler->description();

    case static_cast<int>(Roles::IsActive):
        return toggler->isActive();

    case static_cast<int>(Roles::IsBusy):
        return toggler->isBusy();

    case static_cast<int>(Roles::Display):
        return toggler->display();

    case static_cast<int>(Roles::Name):
    default:
        return toggler->name();
    }
}
