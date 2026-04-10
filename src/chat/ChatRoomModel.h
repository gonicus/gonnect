#pragma once

#include <QAbstractListModel>
#include <qqmlintegration.h>

#include "IChatProvider.h"

class ChatRoomModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(IChatProvider *chatProvider MEMBER m_chatProvider NOTIFY chatProviderChanged FINAL)

public:
    ChatRoomModel(QObject *parent = nullptr);

    enum class Roles { RoomId = Qt::UserRole + 1, Name, UnreadCount };

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

private:
    IChatProvider *m_chatProvider = nullptr;
    QObject *m_chatProviderContext = nullptr;

private Q_SLOTS:
    void onChatProviderChanged();

Q_SIGNALS:
    void chatProviderChanged();
};
