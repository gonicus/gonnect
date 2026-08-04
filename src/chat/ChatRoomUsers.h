#pragma once

#include <QAbstractListModel>
#include <QQmlEngine>
#include "IChatRoom.h"

class ChatRoomUsers : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(IChatRoom *chatRoom MEMBER m_chatRoom NOTIFY chatRoomChanged FINAL)

public:
    enum class Roles { Id = Qt::UserRole + 1, ComputedName, AvatarPath };
    explicit ChatRoomUsers(QObject *parent = nullptr);

protected:
    virtual QHash<int, QByteArray> roleNames() const override;
    virtual int rowCount(const QModelIndex &parent) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;

private:
    IChatRoom *m_chatRoom = nullptr;

Q_SIGNALS:
    void chatRoomChanged();
};
