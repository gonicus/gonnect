#include "ChatMessageTransformer.h"

#ifndef APP_TESTS
#  include "ChatUser.h"
#endif

#include <cmark.h>

#include <QRegularExpression>

#include <cstdlib>

namespace ChatMessageTransformer {

QString linkifyBareUrls(const QString &orig)
{
    static const QRegularExpression re(
            R"((`[^`]*?`)|(\[[^\]]*\]\([^)]*\))|(<[^<>]*>)|\b((?:https?://|ftp://|www\.)[^\s<>]+(?<![\s<>\p{P}])))",
            QRegularExpression::CaseInsensitiveOption
                    | QRegularExpression::DotMatchesEverythingOption);

    QString result;
    int lastPos = 0;
    auto it = re.globalMatch(orig);

    while (it.hasNext()) {
        auto match = it.next();

        result.append(orig.sliced(lastPos, match.capturedStart() - lastPos));

        const QString fullMatch = match.captured(0);
        const QString url = match.captured(4);

        if (!url.isEmpty()) {
            QString href = url;
            if (href.startsWith(QStringLiteral("www."), Qt::CaseInsensitive)) {
                href.prepend(QStringLiteral("https://"));
            }
            result.append(QStringLiteral("[%1](%2)").arg(url, href));
        } else {
            result.append(fullMatch);
        }

        lastPos = match.capturedEnd();
    }

    result.append(orig.sliced(lastPos));
    return result;
}

QString markdownToHtml(const QString &orig)
{
    const QByteArray utf8Data = linkifyBareUrls(orig).toUtf8();
    char *html =
            cmark_markdown_to_html(utf8Data.constData(), utf8Data.size(), CMARK_OPT_HARDBREAKS);
    if (html == nullptr) {
        return {};
    }
    const QString result = QString::fromUtf8(html);
    std::free(html);
    return result;
}

#ifndef APP_TESTS
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
#endif

} // namespace ChatMessageTransformer
