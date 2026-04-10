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

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
};
