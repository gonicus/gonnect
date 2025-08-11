#include "NumberStatsModel.h"
#include "NumberStats.h"
#include "NumberStat.h"

NumberStatsModel::NumberStatsModel(QObject *parent) : QAbstractListModel{ parent }
{

    auto &numStats = NumberStats::instance();

    connect(&numStats, &NumberStats::countChanged, this, [this](const qsizetype index) {
        const auto modelIndex = createIndex(index, 0);
        emit dataChanged(modelIndex, modelIndex, { static_cast<int>(Roles::Count) });
    });

    connect(&numStats, &NumberStats::numberStatAdded, this, [this](const qsizetype index) {
        beginInsertRows(QModelIndex(), index, index);
        endInsertRows();
    });
}

QHash<int, QByteArray> NumberStatsModel::roleNames() const
{
    return {
        { static_cast<int>(Roles::PhoneNumber), "phoneNumber" },
        { static_cast<int>(Roles::Count), "count" },
    };
}

int NumberStatsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return NumberStats::instance().statsAndFlags().size();
}

QVariant NumberStatsModel::data(const QModelIndex &index, int role) const
{

    const auto item = NumberStats::instance().statsAndFlags().at(index.row());

    switch (role) {
    case static_cast<int>(Roles::Count):
        return item->callCount;
    }

    return item->phoneNumber;
}
