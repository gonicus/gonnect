#pragma once

#include <QAbstractListModel>
#include <QObject>
#include <QQmlEngine>

struct NumberStat;

class HistoryModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(int limit MEMBER m_limit NOTIFY limitChanged FINAL)

public:
    enum class Roles {
        Id = Qt::UserRole + 1,
        ContactId,
        Day,
        Time,
        Account,
        ContactName,
        Company,
        Location,
        HasAvatar,
        AvatarPath,
        RemoteUrl,
        RemotePhoneNumber,
        DurationSeconds,
        WasEstablished,
        IsAnonymous,
        IsFavorite,
        IsBlocked,
        Type,
        HasBuddyState
    };

    explicit HistoryModel(QObject *parent = nullptr);

    virtual int rowCount(const QModelIndex &parent) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;
    virtual QHash<int, QByteArray> roleNames() const override;

private Q_SLOTS:
    void handleFavoriteToggle(const NumberStat *item);
    void resetModel();

private:
    int m_limit = -1;

Q_SIGNALS:
    void limitChanged();
};
