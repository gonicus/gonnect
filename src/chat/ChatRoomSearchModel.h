#pragma once

#include <QAbstractListModel>
#include <QQmlEngine>
#include "PublicChatRoom.h"

class IChatProvider;

class ChatRoomSearchModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(QString searchPhrase MEMBER m_searchPhrase NOTIFY searchPhraseChanged)
    Q_PROPERTY(IChatProvider *chatProvider MEMBER m_chatProvider NOTIFY chatProviderChanged)

public:
    enum class Roles { Id = Qt::UserRole + 1, Name, Topic, NumberOfJoinedMembers, JoinRule };

    explicit ChatRoomSearchModel(QObject *parent = nullptr);

    virtual QHash<int, QByteArray> roleNames() const override;
    virtual int rowCount(const QModelIndex &parent) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;

private Q_SLOTS:
    void onChatProviderChanged();

private:
    QString m_searchPhrase;
    QString m_lastSearchId;
    IChatProvider *m_chatProvider = nullptr;
    QObject *m_chatProviderContext = nullptr;
    QList<QSharedPointer<PublicChatRoom>> m_publicRooms;

Q_SIGNALS:
    void chatProviderChanged();
    void searchPhraseChanged();
};
