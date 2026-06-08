#pragma once

#include <QSortFilterProxyModel>
#include <QObject>
#include <QQmlEngine>

class FavoritesProxyModel : public QSortFilterProxyModel
{
    Q_OBJECT
    QML_ELEMENT
    Q_CLASSINFO("DefaultProperty", "sourceModel")

    Q_PROPERTY(bool showJitsi MEMBER m_showJitsi NOTIFY showJitsiChanged FINAL)
    Q_PROPERTY(bool showChatRooms MEMBER m_showChatRooms NOTIFY showChatRoomsChanged FINAL)

public:
    explicit FavoritesProxyModel(QObject *parent = nullptr);

protected:
    virtual bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;
    virtual bool lessThan(const QModelIndex &sourceLeft,
                          const QModelIndex &sourceRight) const override;

private:
    bool m_showJitsi = true;
    bool m_showChatRooms = true;

Q_SIGNALS:
    void showJitsiChanged();
    void showChatRoomsChanged();
};
