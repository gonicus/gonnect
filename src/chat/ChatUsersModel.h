#pragma once

#include <QAbstractListModel>
#include <QQmlEngine>
#include "IChatProvider.h"

class ChatUsersModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(IChatProvider *chatProvider MEMBER m_chatProvider NOTIFY chatProviderChanged FINAL)
public:
    enum class Roles { Id = Qt::UserRole + 1, Name, AvatarPath, HasPresenceState, PresenceState };

    explicit ChatUsersModel(QObject *parent = nullptr);

    virtual QHash<int, QByteArray> roleNames() const override;
    virtual int rowCount(const QModelIndex &parent) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;

private:
    IChatProvider *m_chatProvider = nullptr;
    QObject *m_chatProviderContext = nullptr;

Q_SIGNALS:
    void chatProviderChanged();
};
