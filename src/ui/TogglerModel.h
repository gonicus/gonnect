#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QQmlEngine>

class TogglerModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

public:
    enum class Roles { Id = Qt::UserRole + 1, Name, Description, IsActive, IsBusy, Display };

    explicit TogglerModel(QObject *parent = nullptr);

    virtual QHash<int, QByteArray> roleNames() const override;
    virtual int rowCount(const QModelIndex &parent) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;
};
