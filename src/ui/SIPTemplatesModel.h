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

    virtual QHash<int, QByteArray> roleNames() const override;
    virtual int rowCount(const QModelIndex &parent) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;
};
