#include "CallsProxyModel.h"
#include "CallsModel.h"
#include "SIPCallManager.h"

CallsProxyModel::CallsProxyModel(QObject *parent) : QSortFilterProxyModel{ parent }
{

    connect(this, &CallsProxyModel::onlyEstablishedCallsChanged, this,
            [this]() { beginFilterChange(); endFilterChange(); });
    connect(this, &CallsProxyModel::hideIncomingSecondaryCallOnBusyChanged, this,
            [this]() { beginFilterChange(); endFilterChange(); });
    connect(&SIPCallManager::instance(), &SIPCallManager::establishedCallsCountChanged, this,
            [this]() { beginFilterChange(); endFilterChange(); });
}

bool CallsProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    const auto model = sourceModel();
    if (!model) {
        return false;
    }

    const auto index = model->index(sourceRow, 0, sourceParent);
    const bool isEstablished =
            model->data(index, static_cast<int>(CallsModel::Roles::IsEstablished)).toBool();

    if (!isEstablished) {
        if (m_onlyEstablishedCalls) {
            return false;
        }

        if (m_hideIncomingSecondaryCallOnBusy
            && SIPCallManager::instance().beBusyOnNextIncomingCall()) {
            return false;
        }
    }

    return !model->data(index, static_cast<int>(CallsModel::Roles::IsBlocked)).toBool();
}
