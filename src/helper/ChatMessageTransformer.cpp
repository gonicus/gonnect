#include "ChatMessageTransformer.h"

#ifndef APP_TESTS
#  include "ChatUser.h"
#endif

#include <cmark.h>

#include <QRegularExpression>
#include <QSet>

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
    char *html = cmark_markdown_to_html(utf8Data.constData(), utf8Data.size(),
                                        CMARK_OPT_HARDBREAKS | CMARK_OPT_UNSAFE);
    if (html == nullptr) {
        return {};
    }
    const QString result = sanitizeHtml(QString::fromUtf8(html));
    std::free(html);
    return result;
}

QString sanitizeHtml(const QString &orig)
{
    static const QSet<QString> allowedTags = {
        QStringLiteral("a"),          QStringLiteral("b"),      QStringLiteral("i"),
        QStringLiteral("em"),         QStringLiteral("strong"), QStringLiteral("code"),
        QStringLiteral("pre"),        QStringLiteral("ul"),     QStringLiteral("ol"),
        QStringLiteral("li"),         QStringLiteral("br"),     QStringLiteral("p"),
        QStringLiteral("blockquote"), QStringLiteral("span"),
    };
    static const QRegularExpression tagRe(R"(<(/?)([a-zA-Z0-9]+)([^<>]*)>)");
    static const QRegularExpression hrefRe(R"(href\s*=\s*(\"([^\"]*)\"|'([^']*)'|([^\s\"'>]+)))",
                                           QRegularExpression::CaseInsensitiveOption);
    static const QRegularExpression schemeRe(R"(^\s*(https?|ftp|mailto|chat):)",
                                             QRegularExpression::CaseInsensitiveOption);

    QString result;
    int lastPos = 0;
    auto it = tagRe.globalMatch(orig);

    while (it.hasNext()) {
        auto match = it.next();
        result.append(orig.sliced(lastPos, match.capturedStart() - lastPos));
        lastPos = match.capturedEnd();

        const bool isClose = !match.captured(1).isEmpty();
        const QString tag = match.captured(2).toLower();
        if (!allowedTags.contains(tag)) {
            continue;
        }

        if (isClose) {
            result.append(QStringLiteral("</%1>").arg(tag));
        } else if (tag == QStringLiteral("a")) {
            QString href;
            const auto hrefMatch = hrefRe.match(match.captured(3));
            if (hrefMatch.hasMatch()) {
                href = hrefMatch.captured(2);
                if (href.isEmpty()) {
                    href = hrefMatch.captured(3);
                }
                if (href.isEmpty()) {
                    href = hrefMatch.captured(4);
                }
            }
            if (!href.isEmpty() && schemeRe.match(href).hasMatch()) {
                result.append(QStringLiteral("<a href=\"%1\">").arg(href.toHtmlEscaped()));
            } else {
                result.append(QStringLiteral("<a>"));
            }
        } else if (tag == QStringLiteral("br")) {
            result.append(QStringLiteral("<br />"));
        } else {
            result.append(QStringLiteral("<%1>").arg(tag));
        }
    }

    result.append(orig.sliced(lastPos));
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
