#pragma once

#include <QObject>
#include <qqmlintegration.h>
#include "ChatMessageContentPart.h"

class ChatMessageContentText : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

    Q_PROPERTY(QString simpleText READ simpleText NOTIFY contentChanged FINAL)
    Q_PROPERTY(QList<ChatMessageContentPart *> contentParts READ contentParts NOTIFY contentChanged
                       FINAL)

public:
    explicit ChatMessageContentText(const QString &text = "", QObject *parent = nullptr);

    bool isSimpleText() const;
    QString simpleText() const;
    QList<ChatMessageContentPart *> contentParts() const { return m_parts; }
    void setText(const QString &text);

private:
    QString m_rawText;
    QList<ChatMessageContentPart *> m_parts;

Q_SIGNALS:
    void contentChanged();
};
