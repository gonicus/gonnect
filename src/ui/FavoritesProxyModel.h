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

    virtual QVariant data(const QModelIndex &index, int role) const override;

protected:
    virtual bool filterAcceptsRow(int sourceRow, const QModelIndex &sourceParent) const override;

private:
    bool isJitsiAddr(const QVariantMap &addr) const;
    bool isChatAddr(const QVariantMap &addr) const;

    bool m_showJitsi = true;
    bool m_showChatRooms = true;

Q_SIGNALS:
    void showJitsiChanged();
    void showChatRoomsChanged();
};
