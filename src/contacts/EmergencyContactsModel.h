#pragma once

#include <QAbstractListModel>
#include <QQmlEngine>

class EmergencyContactsModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
public:
    enum class Roles { Index = Qt::UserRole + 1, Number, DisplayName };

    explicit EmergencyContactsModel(QObject *parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
};
