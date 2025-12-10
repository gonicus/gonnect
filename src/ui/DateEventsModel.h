#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QQmlEngine>

class DateEventsModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

public:
    enum class Roles {
        DateTime = Qt::UserRole + 1,
        Date,
        EndDateTime,
        Summary,
        Location,
        Link,
        IsJitsiMeeting,
        IsOtherLink
    };

    explicit DateEventsModel(QObject *parent = nullptr);

    virtual QHash<int, QByteArray> roleNames() const override;
    virtual int rowCount(const QModelIndex &parent) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;
};
