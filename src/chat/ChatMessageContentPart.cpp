#include "ChatMessageContentPart.h"

ChatMessageContentPart::ChatMessageContentPart(QObject *parent) : QObject{ parent } { }

ChatMessageContentPart::ChatMessageContentPart(bool isCode, const QString &text,
                                               const QString &fenceInfo, QObject *parent)
    : QObject{ parent }, m_isCode{ isCode }, m_text{ text.trimmed() }, m_fenceInfo{ fenceInfo }
{
}

QDebug operator<<(QDebug dbg, const ChatMessageContentPart &contentPart)
{
    QDebugStateSaver saver(dbg);
    dbg.nospace() << "ChatMessageContentPart(isCode=" << contentPart.isCode()
                  << ", text=" << contentPart.text().left(20) << ")";
    return dbg;
}
