#pragma once

#include <QObject>
#include <QQmlEngine>

class ChatMessageContentPart : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("")

    Q_PROPERTY(bool isCode READ isCode CONSTANT FINAL)
    Q_PROPERTY(QString text READ text CONSTANT FINAL)
    Q_PROPERTY(QString htmlText READ htmlText CONSTANT FINAL)
    Q_PROPERTY(QString fenceInfo READ fenceInfo CONSTANT FINAL)

public:
    explicit ChatMessageContentPart(QObject *parent);
    ChatMessageContentPart(bool isCode, const QString &text, const QString &htmlText,
                           const QString &fenceInfo, QObject *parent);

    bool isCode() const { return m_isCode; }
    QString text() const { return m_text; }
    QString htmlText() const { return m_htmlText; }
    QString fenceInfo() const { return m_fenceInfo; }

private:
    bool m_isCode = false;
    QString m_text;
    QString m_htmlText;
    QString m_fenceInfo;
};

QDebug operator<<(QDebug dbg, const ChatMessageContentPart &contentPart);
