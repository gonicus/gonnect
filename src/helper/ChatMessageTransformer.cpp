#include "ChatMessageTransformer.h"
#include "ChatUser.h"

#include <QRegularExpression>

namespace ChatMessageTransformer {

QString addLinkTags(const QString &orig)
{
    static const QRegularExpression re(
            R"((<a\b[^>]*>.*?</a>)|(<a\b[^>]*href\s*=\s*"[^"]*")|(\[[^\]]*\]\([^)]*\))|\b((?:https?://|ftp://|www\.)[^\s<>]+(?<![\s<>\p{P}])))",
            QRegularExpression::CaseInsensitiveOption
                    | QRegularExpression::DotMatchesEverythingOption);

    QString result;
    int lastPos = 0;
    auto it = re.globalMatch(orig);

    while (it.hasNext()) {
        auto match = it.next();

        result.append(orig.sliced(lastPos, match.capturedStart() - lastPos));

        QString fullMatch = match.captured(0);
        QString url = match.captured(4);

        if (!url.isEmpty()) {
            QString href = url;
            if (href.startsWith("www.", Qt::CaseInsensitive)) {
                href.prepend("https://");
            }
            result.append(QString(R"(<a href="%1">%2</a>)").arg(href, url));
        } else {
            result.append(fullMatch);
        }

        lastPos = match.capturedEnd();
    }

    result.append(orig.sliced(lastPos));
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
