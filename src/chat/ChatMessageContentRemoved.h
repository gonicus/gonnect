#pragma once

#include <QObject>
#include <QQmlEngine>

class ChatMessageContentRemoved : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

    Q_PROPERTY(QString reason READ reason NOTIFY reasonChanged FINAL)

public:
    explicit ChatMessageContentRemoved(QObject *parent = nullptr);

    QString reason() const { return m_reason; }
    void setReason(const QString &reason);

private:
    QString m_reason;

Q_SIGNALS:
    void reasonChanged();
};
