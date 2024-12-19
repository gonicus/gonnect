#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QSortFilterProxyModel>

class NumberStatsProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_CLASSINFO("DefaultProperty", "sourceModel")

public:
    explicit NumberStatsProxyModel(QObject *parent = nullptr);

protected:
    virtual bool lessThan(const QModelIndex &sourceLeft,
                          const QModelIndex &sourceRight) const override;
};
