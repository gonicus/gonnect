#pragma once

#include <QAbstractListModel>
#include <QQmlEngine>
#include "PublicChatRoom.h"

class IChatProvider;

class ChatRoomSearchModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool isLoading READ isLoading NOTIFY isLoadingChanged FINAL)
    Q_PROPERTY(bool canLoadMore READ canLoadMore NOTIFY canLoadMoreChanged FINAL)
    Q_PROPERTY(QString searchPhrase READ searchPhrase WRITE setSearchPhrase NOTIFY
                       searchPhraseChanged FINAL)
    Q_PROPERTY(quint32 limit MEMBER m_limit NOTIFY limitChanged FINAL)
    Q_PROPERTY(IChatProvider *chatProvider MEMBER m_chatProvider NOTIFY chatProviderChanged FINAL)

public:
    enum class Roles { Id = Qt::UserRole + 1, Name, Topic, NumberOfJoinedMembers, JoinRule };

    explicit ChatRoomSearchModel(QObject *parent = nullptr);

    virtual QHash<int, QByteArray> roleNames() const override;
    virtual int rowCount(const QModelIndex &parent) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;

    QString searchPhrase() const { return m_searchPhrase; }
    void setSearchPhrase(const QString &phrase);

    bool isLoading() const { return m_isLoading; }
    bool canLoadMore() const { return m_canLoadMore; }

    /// Load the next m_limit results
    Q_INVOKABLE void loadNext();

private Q_SLOTS:
    void onChatProviderChanged();
    void updateModel();

private:
    void setIsLoading(bool value);
    void setCanLoadMore(bool value);

    bool m_isLoading = false;
    bool m_isLoadingNext = false;
    bool m_canLoadMore = false;
    QString m_searchPhrase;
    QString m_searchTag;
    QString m_nextBatchToken;
    quint32 m_limit = 20;
    IChatProvider *m_chatProvider = nullptr;
    QObject *m_chatProviderContext = nullptr;
    QList<QSharedPointer<PublicChatRoom>> m_publicRooms;

Q_SIGNALS:
    void isLoadingChanged();
    void canLoadMoreChanged();
    void chatProviderChanged();
    void searchPhraseChanged();
    void limitChanged();
};
