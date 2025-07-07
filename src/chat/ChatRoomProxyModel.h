#pragma once

#include <QSortFilterProxyModel>
#include <QQmlEngine>

class ChatRoomProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_CLASSINFO("DefaultProperty", "sourceModel")

public:
    explicit ChatRoomProxyModel(QObject *parent = nullptr);

protected:
    virtual bool lessThan(const QModelIndex &sourceLeft,
                          const QModelIndex &sourceRight) const override;
};
