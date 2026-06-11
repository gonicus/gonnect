#pragma once

#include "Contact.h"
#include "NumberStats.h"

#include <QAbstractListModel>
#include <QObject>
#include <QQmlEngine>

struct NumberStat;
class IChatRoom;
class IChatProvider;

struct FavoriteEntry
{
    Contact *contact = nullptr;
    IChatProvider *chatProvider = nullptr;
    IChatRoom *chatRoom = nullptr;

    struct Addr
    {
        NumberStats::ContactType contactType = NumberStats::ContactType::PhoneNumber;
        Contact::NumberType numberType = Contact::NumberType::Unknown;
        QString addr;
    };

    std::vector<std::unique_ptr<Addr>> addrs;

    QString name() const;
};

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
        Name = Qt::UserRole + 1,
        Company,
        HasBuddyState,
        HasAvatar,
        AvatarPath,
        ChatProvider,
        ChatRoom,
        Addresses,
        SubscribableNumber,
    };

    explicit FavoritesModel(QObject *parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;

private Q_SLOTS:
    void scheduleModelUpdate();
    void updateModel();

private:
    void sortInnerModel();
    void addChatProviderSignals(IChatProvider &provider);

    std::vector<std::unique_ptr<FavoriteEntry>> m_favorites;
    QHash<Contact *, FavoriteEntry *> m_favoriteContactLookup;
    QTimer m_modelUpdateTimer;

    QObject *m_chatProviderContext = nullptr;
    bool m_isUpdating = false;
};
