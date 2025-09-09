#pragma once

#include <QAbstractListModel>
#include <QQmlEngine>

#include "IConferenceConnector.h"

class ChatModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(IConferenceConnector *iConferenceConnector MEMBER m_iConferenceConnector NOTIFY
                       iConferenceConnectorChanged FINAL)
    Q_PROPERTY(uint realMessagesCount READ realMessagesCount NOTIFY realMessagesCountChanged FINAL)

public:
    enum class Roles {
        FromId = Qt::UserRole + 1,
        NickName,
        Message,
        Timestamp,
        IsPrivateMessage,
        IsOwnMessage,
        IsSystemMessage
    };
    explicit ChatModel(QObject *parent = nullptr);

    virtual QHash<int, QByteArray> roleNames() const override;
    virtual int rowCount(const QModelIndex &parent) const override;
    virtual QVariant data(const QModelIndex &index, int role) const override;

    uint realMessagesCount() const { return m_realMessagesCount; }

private Q_SLOTS:
    void onIConferenceConnectorChanged();
    void updateRealMessagesCount();

private:
    QString addLinkTags(const QString &orig) const;

    IConferenceConnector *m_iConferenceConnector = nullptr;
    QObject *m_iConferenceConnectorContext = nullptr;
    uint m_realMessagesCount = 0;

Q_SIGNALS:
    void iConferenceConnectorChanged();
    void realMessagesCountChanged();
};
