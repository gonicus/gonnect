#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QQmlEngine>

struct NumberStat;
class IChatRoom;
class IChatProvider;

class FavoritesModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

public:
    struct FavRoom
    {
        IChatProvider *provider = nullptr;
        IChatRoom *room = nullptr;
    };

    enum class Roles {
        PhoneNumber = Qt::UserRole + 1,
        Name,
        Company,
        HasBuddyState,
        HasAvatar,
        AvatarPath,
        NumberType,
        ContactType,
        ChatProvider
    };

    explicit FavoritesModel(QObject *parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;

private Q_SLOTS:
    void updateModel();

private:
    QList<NumberStat *> m_favorites;
    QList<FavRoom> m_favoriteChatRooms;

    QObject *m_chatProviderContext = nullptr;
};
