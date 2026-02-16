#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QQmlEngine>

struct NumberStat;

class FavoritesModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

public:
    enum class Roles {
        PhoneNumber = Qt::UserRole + 1,
        ContactId,
        Name,
        Company,
        HasBuddyState,
        HasAvatar,
        AvatarPath,
        IsAnonymous,
        IsBlocked,
        NumberType,
        ContactType
    };

    explicit FavoritesModel(QObject *parent = nullptr);

    QHash<int, QByteArray> roleNames() const override;
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;

private Q_SLOTS:
    void updateModel();

private:
    QList<NumberStat *> m_favorites;
};
