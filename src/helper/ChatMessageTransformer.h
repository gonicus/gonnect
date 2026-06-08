#pragma once

#include <QString>
#include "ChatMessage.h"

namespace ChatMessageTransformer {

QString addLinkTags(const QString &orig);
QString highlightMentions(const QString &orig, const ChatMessage &message);

} // namespace ChatMessageTransformer
