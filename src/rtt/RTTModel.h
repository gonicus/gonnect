#pragma once

#include <QAbstractListModel>
#include <QVector>
#include <QQmlEngine>

#include "RTTMessage.h"

class RTTModel : public QAbstractListModel
{
    Q_OBJECT
    QML_ELEMENT

public:
    enum class Roles { Timestamp = Qt::UserRole + 1, Message, IsMe, IsFinished };

    explicit RTTModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    QHash<int, QByteArray> roleNames() const override;

    Q_INVOKABLE void reset();

    Q_INVOKABLE void addMessage(qint64 timestamp, const QString &text, bool isMe);
    Q_INVOKABLE void updateMessage(const QString &message, bool isMe, bool isFinished = false);

private:
    QList<RTTMessage> m_messages;
};
