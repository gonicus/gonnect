#pragma once

#include <QAbstractListModel>
#include <QQmlEngine>

struct NumberStat;

class HistoryContactSearchModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_PROPERTY(QString searchText MEMBER m_searchText NOTIFY searchTextChanged FINAL)

public:
    enum class Roles {
        DisplayName = Qt::UserRole + 1,
        Url,
        IsPhoneNumber,
        IsFavorite,
        IsBlocked,
        IsAnonymous,
        IsSipSubscriptable,
        LastCall,
        SearchStringDistance
    };

    explicit HistoryContactSearchModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

private Q_SLOTS:
    void updateModel();
    void onFavoriteChanged(const NumberStat *numStatItem);

private:
    struct Item
    {
        bool isPhoneNumber = false;
        QString url;
        QString displayName;
        qreal searchStringDistance = 0;
        QDateTime lastCall;

        bool operator!=(const Item &other) const;
        bool operator==(const Item &other) const;
    };

    QString m_searchText;
    QList<Item> m_resultList;

Q_SIGNALS:
    void searchTextChanged();
};
