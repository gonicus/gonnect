#pragma once

#include <QString>

#ifndef APP_TESTS
#  include "ChatMessage.h"
#endif

namespace ChatMessageTransformer {

QString addLinkTags(const QString &orig);

#ifndef APP_TESTS
QString highlightMentions(const QString &orig, const ChatMessage &message);
#endif

} // namespace ChatMessageTransformer
