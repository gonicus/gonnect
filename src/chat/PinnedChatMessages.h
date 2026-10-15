#pragma once

#include <QAbstractListModel>
#include <QQmlEngine>

#include "IChatRoom.h"

class PinnedChatMessages : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(IChatRoom *chatRoom MEMBER m_chatRoom NOTIFY chatRoomChanged FINAL)

public:
    enum class Roles { EventId = Qt::UserRole + 1, NickName, Content };

    explicit PinnedChatMessages(QObject *parent = nullptr);

    virtual QHash<int, QByteArray> roleNames() const override;
    virtual int rowCount(const QModelIndex &parent) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;

private:
    IChatRoom *m_chatRoom = nullptr;
    QObject *m_chatRoomContext = nullptr;

private Q_SLOTS:
    void onChatRoomUpdated();

Q_SIGNALS:
    void chatRoomChanged();
};
