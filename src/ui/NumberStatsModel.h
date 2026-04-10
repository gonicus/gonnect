#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QQmlEngine>

class NumberStatsModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

public:
    enum class Roles { PhoneNumber = Qt::UserRole + 1, Count };

    explicit NumberStatsModel(QObject *parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
};
