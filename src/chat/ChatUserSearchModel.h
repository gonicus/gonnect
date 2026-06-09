#pragma once

#include <QAbstractListModel>
#include <QQmlEngine>

#include "IChatProvider.h"

class ChatUser;

class ChatUserSearchModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")

    Q_PROPERTY(QString searchPhrase MEMBER m_searchPhrase NOTIFY searchPhraseChanged)
    Q_PROPERTY(IChatProvider *chatProvider MEMBER m_chatProvider NOTIFY chatProviderChanged)

public:
    enum class Roles { Id = Qt::UserRole + 1, Name, AvatarPath, HasPresenceState, PresenceState };

    explicit ChatUserSearchModel(QObject *parent = nullptr);

    virtual QHash<int, QByteArray> roleNames() const override;
    virtual int rowCount(const QModelIndex &parent) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;

private Q_SLOTS:
    void onChatProviderChanged();

private:
    void updateModel(const QList<ChatUser *> &userList);

    QString m_searchPhrase;
    IChatProvider *m_chatProvider = nullptr;
    QObject *m_chatProviderContext = nullptr;
    QString m_searchId;

    QList<ChatUser *> m_model;

Q_SIGNALS:
    void searchPhraseChanged();
    void chatProviderChanged();
};
