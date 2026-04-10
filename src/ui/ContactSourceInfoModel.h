#pragma once

#include <QAbstractListModel>
#include <QQmlEngine>

class ContactSourceInfoModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

public:
    enum class Roles { Priority = Qt::UserRole + 1, DisplayName };
    explicit ContactSourceInfoModel(QObject *parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
};
