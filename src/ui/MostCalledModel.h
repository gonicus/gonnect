#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QQmlEngine>

class MostCalledModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

public:
    enum class Roles { Name = Qt::UserRole + 1, PhoneNumber, HasBuddyState, NumberType };

    explicit MostCalledModel(QObject *parent = nullptr);

    virtual QHash<int, QByteArray> roleNames() const override;
    virtual int rowCount(const QModelIndex &parent) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;

private Q_SLOTS:
    void updateModel();

private:
    QStringList m_phoneNumbers;
};
