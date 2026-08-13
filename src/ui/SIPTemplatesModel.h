#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QQmlEngine>

class SIPTemplatesModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

public:
    enum class Roles { Id = Qt::UserRole + 1, Name };

    explicit SIPTemplatesModel(QObject *parent = nullptr);

    Q_INVOKABLE bool hasFields(const QString &id) const;

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
};
