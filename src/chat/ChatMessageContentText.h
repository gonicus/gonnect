#pragma once

#include <QObject>
#include <qqmlintegration.h>
#include "ChatMessageContentPart.h"

class ChatMessageContentText : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

    Q_PROPERTY(bool isSimpleText READ isSimpleText NOTIFY contentChanged FINAL)
    Q_PROPERTY(QString simpleText READ simpleText NOTIFY contentChanged FINAL)
    Q_PROPERTY(QList<ChatMessageContentPart *> contentParts READ contentParts NOTIFY contentChanged
                       FINAL)

public:
    explicit ChatMessageContentText(const QString &text = "", QObject *parent = nullptr);

    bool isSimpleText() const;
    QString simpleText() const;

    QList<ChatMessageContentPart *> contentParts() const { return m_parts; }
    QString rawText() const;

    void setText(const QString &text);

    void processText();

private:
    QString convertText(const QString &originalText) const;
    QString m_rawText;
    QString m_simpleText;
    QList<ChatMessageContentPart *> m_parts;

Q_SIGNALS:
    void contentChanged();
};
