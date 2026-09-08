#include "ChatMessageTransformer.h"

#ifndef APP_TESTS
#  include "ChatUser.h"
#endif

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

QString fixNewLines(const QString &orig)
{
    QString out;
    out.reserve(orig.size() * 2);
    const int n = orig.size();
    int i = 0;
    while (i < n) {
        if (orig.at(i) != QLatin1Char('\n')) {
            out.append(orig.at(i++));
            continue;
        }
        int j = i;
        while (j < n && orig.at(j) == QLatin1Char('\n')) {
            ++j;
        }
        const int run = j - i;
        if (run == 1) {
            const bool esc = i > 0 && orig.at(i - 1) == QLatin1Char('\\');
            out.append(esc ? QStringLiteral("\n") : QStringLiteral("\\\n"));
        } else {
            out.append(QStringLiteral("\\\n"));
            for (int r = 1; r < run; ++r) {
                out.append(QChar(0x2060));
                out.append(QStringLiteral("  \n")); // <-- statt "\\\n"
            }
        }
        i = j;
    }
    if (out.endsWith(QStringLiteral("\\\n"))) {
        out.chop(2);
        out.append('\n');
    }
    if (out.endsWith(QStringLiteral("  \n"))) {
        out.chop(3);
        out.append('\n');
    }
    return out;
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
