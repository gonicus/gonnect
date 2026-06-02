#pragma once

#include <QSortFilterProxyModel>
#include <QQmlEngine>

class ChatRoomProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_CLASSINFO("DefaultProperty", "sourceModel")
    Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")

    Q_PROPERTY(bool onlyUnread MEMBER m_onlyUnread NOTIFY onlyUnreadChanged FINAL)
    Q_PROPERTY(bool groupFavorites MEMBER m_groupFavorites NOTIFY groupFavoritesChanged FINAL)
    Q_PROPERTY(QString filterText MEMBER m_filterText NOTIFY filterTextChanged FINAL)
    Q_PROPERTY(ChatRoomProxyModel::SortStrategy sortStrategy MEMBER m_sortStrategy NOTIFY
                       sortStrategyChanged FINAL)

public:
    enum class SortStrategy { Alphabetical, LatestActivity };
    Q_ENUM(SortStrategy)

    explicit ChatRoomProxyModel(QObject *parent = nullptr);

    ChatRoomProxyModel::SortStrategy sortStrategy() const;
    void setSortStrategy(const ChatRoomProxyModel::SortStrategy strategy);

protected:
    virtual bool lessThan(const QModelIndex &sourceLeft,
                          const QModelIndex &sourceRight) const override;
    virtual bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    QObject *m_sourceModelContext = nullptr;
    bool m_onlyUnread = false;
    bool m_groupFavorites = false;
    QString m_filterText;
    ChatRoomProxyModel::SortStrategy m_sortStrategy =
            ChatRoomProxyModel::SortStrategy::Alphabetical;

private Q_SLOTS:
    void onSourceModelChanged();

Q_SIGNALS:
    void onlyUnreadChanged();
    void groupFavoritesChanged();
    void filterTextChanged();
    void sortStrategyChanged();
};
