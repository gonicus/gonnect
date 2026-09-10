#pragma once

#include <QObject>
#include <qqmlintegration.h>
#include "ChatMessageContentPart.h"

class ChatMessageContentText : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

    Q_PROPERTY(QString rawText READ rawText NOTIFY contentChanged FINAL)
    Q_PROPERTY(bool isSimpleText READ isSimpleText NOTIFY contentChanged FINAL)
    Q_PROPERTY(QString simpleText READ simpleText NOTIFY contentChanged FINAL)
    Q_PROPERTY(QString htmlText READ htmlText NOTIFY contentChanged FINAL)
    Q_PROPERTY(QList<ChatMessageContentPart *> contentParts READ contentParts NOTIFY contentChanged
                       FINAL)

public:
    explicit ChatMessageContentText(const QString &text = "", QObject *parent = nullptr);

    bool isSimpleText() const;
    QString simpleText() const;
    QString htmlText() const { return m_htmlText; }
    QString rawText() const { return m_rawText; }
    QList<ChatMessageContentPart *> contentParts() const { return m_parts; }
    void setText(const QString &text);

    void processText();

private:
    QString convertHtmlText(const QString &originalText) const;
    QString m_rawText;
    QString m_simpleText;
    QString m_htmlText;
    QList<ChatMessageContentPart *> m_parts;

Q_SIGNALS:
    void contentChanged();
};
