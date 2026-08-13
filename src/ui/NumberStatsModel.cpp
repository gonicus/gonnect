#include "NumberStatsModel.h"
#include "NumberStats.h"
#include "NumberStat.h"

NumberStatsModel::NumberStatsModel(QObject *parent) : QAbstractListModel{ parent }
{
    auto &numStats = NumberStats::instance();

    connect(&numStats, &NumberStats::numberStatAdded, this, [this](const qsizetype index) {
        beginInsertRows(QModelIndex(), index, index);
        endInsertRows();
    });
}

QHash<int, QByteArray> NumberStatsModel::roleNames() const
{
    return {
        { static_cast<int>(Roles::PhoneNumber), "phoneNumber" },
    };
}

int NumberStatsModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return NumberStats::instance().statsAndFlags().size();
}

QVariant NumberStatsModel::data(const QModelIndex &index, int role) const
{
    Q_UNUSED(role);

    const auto item = NumberStats::instance().statsAndFlags().at(index.row());
    return item->phoneNumber;
}
