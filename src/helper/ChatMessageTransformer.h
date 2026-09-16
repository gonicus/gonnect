#pragma once

#include <QString>

#ifndef APP_TESTS
#  include "ChatMessage.h"
#endif

namespace ChatMessageTransformer {

// Converts bare URLs (https://, http://, ftp://, www.) into Markdown links,
// so cmark renders them as anchors. Skips inline code, existing Markdown
// links and raw HTML tags.
QString linkifyBareUrls(const QString &orig);

// Renders Markdown to an HTML fragment via cmark. Single newlines become
// line breaks, no manual backslash escaping is applied.
QString markdownToHtml(const QString &orig);
QString sanitizeHtml(const QString &orig);

#ifndef APP_TESTS
QString highlightMentions(const QString &orig, const ChatMessage &message);
#endif

} // namespace ChatMessageTransformer
