#pragma once

#include <QSortFilterProxyModel>
#include <QQmlEngine>
#include <QTimer>
#include "ChatRoomModel.h"

class ChatRoomProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_CLASSINFO("DefaultProperty", "sourceModel")
    Q_CLASSINFO("RegisterEnumClassesUnscoped", "false")

    Q_PROPERTY(
            bool showSectionHeader MEMBER m_showSectionHeader NOTIFY showSectionHeaderChanged FINAL)
    Q_PROPERTY(bool onlyUnread READ onlyUnread WRITE setOnlyUnread NOTIFY onlyUnreadChanged FINAL)
    Q_PROPERTY(bool groupFavorites READ groupFavorites WRITE setGroupFavorites NOTIFY
                       groupFavoritesChanged FINAL)
    Q_PROPERTY(
            QString filterText READ filterText WRITE setFilterText NOTIFY filterTextChanged FINAL)
    Q_PROPERTY(ChatRoomProxyModel::SortStrategy sortStrategy MEMBER m_sortStrategy NOTIFY
                       sortStrategyChanged FINAL)

public:
    enum class SortStrategy { Alphabetical, LatestActivity };
    Q_ENUM(SortStrategy)

    enum class Roles { SectionHeader = static_cast<int>(ChatRoomModel::Roles::LastRole) };

    explicit ChatRoomProxyModel(QObject *parent = nullptr);

    virtual QHash<int, QByteArray> roleNames() const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;

    bool onlyUnread() const { return m_onlyUnread; }
    void setOnlyUnread(bool value);

    bool groupFavorites() const { return m_groupFavorites; }
    void setGroupFavorites(bool value);

    QString filterText() const { return m_filterText; }
    void setFilterText(const QString &filterText);

    ChatRoomProxyModel::SortStrategy sortStrategy() const;
    void setSortStrategy(const ChatRoomProxyModel::SortStrategy strategy);

protected:
    virtual bool lessThan(const QModelIndex &sourceLeft,
                          const QModelIndex &sourceRight) const override;
    virtual bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    QMetaObject::Connection m_dataChangedConnection;
    QObject *m_sourceModelContext = nullptr;
    bool m_showSectionHeader = true;
    bool m_onlyUnread = false;
    bool m_groupFavorites = false;
    QString m_filterText;
    ChatRoomProxyModel::SortStrategy m_sortStrategy =
            ChatRoomProxyModel::SortStrategy::Alphabetical;
    QTimer m_sortDebounceTimer;
    QTimer m_sectionHeaderDebounceTimer;

private Q_SLOTS:
    void onSourceModelChanged();
    void applySort();
    void refreshSectionHeaders();

Q_SIGNALS:
    void showSectionHeaderChanged();
    void onlyUnreadChanged();
    void groupFavoritesChanged();
    void filterTextChanged();
    void sortStrategyChanged();
};
