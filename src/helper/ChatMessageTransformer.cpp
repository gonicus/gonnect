#include "ChatMessageTransformer.h"
#include "ChatUser.h"

#include <QRegularExpression>

namespace ChatMessageTransformer {

QString addLinkTags(const QString &orig)
{
    static const QRegularExpression reWithGroup(
            R"(\b((?:https?://|ftp://|www\.)[^\s<>]+(?<![\s<>\p{P}])))",
            QRegularExpression::CaseInsensitiveOption);

    QString result = orig;
    result.replace(reWithGroup, R"(<a href="\1">\1</a>)");

    static const QRegularExpression wwwRe(R"(<a href="www\.)",
                                          QRegularExpression::CaseInsensitiveOption);
    result.replace(wwwRe, R"(<a href="http://www.)");

    return result;
}

QString highlightMentions(const QString &orig, const ChatMessage &message)
{
    const auto mentions = message.mentionedUsers();
    if (mentions.isEmpty()) {
        return orig;
    }

    QString str(orig);

    for (const auto *user : mentions) {
        const auto name = user->computedName();
        if (name.isEmpty()) {
            continue;
        }

        qsizetype pos = 0;
        while ((pos = str.indexOf(name, pos)) != -1) {

            const bool boundaryBefore = (pos == 0 || !str.at(pos - 1).isLetterOrNumber());
            const int afterPos = pos + name.length();
            const bool boundaryAfter =
                    (afterPos >= str.length()) || !str.at(afterPos).isLetterOrNumber();

            if (boundaryBefore && boundaryAfter) {
                const QString replacement = QString("[%1](chat://%2)").arg(name, user->id());
                str.replace(pos, name.length(), replacement);

                pos += replacement.length();
            } else {
                pos += name.length();
            }
        }
    }

    return str;
}

} // namespace ChatMessageTransformer
