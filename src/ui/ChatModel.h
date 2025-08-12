#pragma once

#include <QAbstractListModel>
#include <QQmlEngine>

#include "JitsiConnector.h"

class ChatModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(JitsiConnector *jitsiConnector MEMBER m_jitsiConnector NOTIFY jitsiConnectorChanged
                       FINAL)
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

private slots:
    void onJitsiConnectorChanged();
    void updateRealMessagesCount();

private:
    QString addLinkTags(const QString &orig) const;

    JitsiConnector *m_jitsiConnector = nullptr;
    QObject *m_jitsiConnectorContext = nullptr;
    uint m_realMessagesCount = 0;

signals:
    void jitsiConnectorChanged();
    void realMessagesCountChanged();
};
