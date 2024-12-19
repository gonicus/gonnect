#pragma once

#include <QObject>
#include <QQmlEngine>
#include <QSortFilterProxyModel>

class CallsProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_CLASSINFO("DefaultProperty", "sourceModel")

public:
    explicit CallsProxyModel(QObject *parent = nullptr);

protected:
    virtual bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
};
