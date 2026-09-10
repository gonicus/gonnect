#include "ActivitiesProxyModel.h"
#include "ActivitiesModel.h"

ActivitiesProxyModel::ActivitiesProxyModel(QObject *parent) : QSortFilterProxyModel{ parent }
{
    connect(this, &ActivitiesProxyModel::filterTextChanged, this,
            &ActivitiesProxyModel::updateIsFiltering);
    connect(this, &ActivitiesProxyModel::mediumFilterChanged, this,
            &ActivitiesProxyModel::updateIsFiltering);

    connect(this, &ActivitiesProxyModel::filterTextChanged, this, [this]() {
        beginFilterChange();
        endFilterChange();
    });
    connect(this, &ActivitiesProxyModel::mediumFilterChanged, this, [this]() {
        beginFilterChange();
        endFilterChange();
    });
}

bool ActivitiesProxyModel::filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const
{
    const auto model = qobject_cast<ActivitiesModel *>(sourceModel());
    if (!model) {
        return false;
    }

    const auto index = model->index(sourceRow, 0, sourceParent);
    if (!index.isValid()) {
        return false;
    }

    using Roles = ActivitiesModel::Roles;

    // Medium filter
    if (m_mediumFilter != MediumFilter::ALL) {
        const bool isSIP = model->data(index, static_cast<int>(Roles::IsSIPCall)).toBool();
        if (m_mediumFilter == MediumFilter::SIPCALL && !isSIP) {
            return false;
        }

        const bool isJitsi = model->data(index, static_cast<int>(Roles::IsJitsiMeetCall)).toBool();
        if (m_mediumFilter == MediumFilter::JITSIMEET && !isJitsi) {
            return false;
        }

        const bool isChat = model->data(index, static_cast<int>(Roles::IsChatMessage)).toBool();
        if (m_mediumFilter == MediumFilter::CHAT && !isChat) {
            return false;
        }
    }

    // Filter text
    const auto filterText = m_filterText.trimmed();
    if (!filterText.isEmpty()) {
        const auto title = model->data(index, static_cast<int>(Roles::Title)).toString();
        if (!title.isEmpty() && title.contains(filterText, Qt::CaseInsensitive)) {
            return true;
        }

        const auto text = model->data(index, static_cast<int>(Roles::Text)).toString();
        if (!text.isEmpty() && text.contains(filterText, Qt::CaseInsensitive)) {
            return true;
        }

        return false;
    }

    return true;
}

void ActivitiesProxyModel::updateIsFiltering()
{
    const bool isFiltering =
            m_mediumFilter != MediumFilter::ALL || !m_filterText.trimmed().isEmpty();

    if (m_isFiltering != isFiltering) {
        m_isFiltering = isFiltering;
        Q_EMIT isFilteringChanged();
    }
}
