#include "ChatMessageTransformer.h"
#include "ChatUser.h"

#include <QRegularExpression>

namespace ChatMessageTransformer {

QString addLinkTags(const QString &orig)
{
    static const QRegularExpression re("^\\S+\\.\\S{2,}[^.]$",
                                       QRegularExpression::CaseInsensitiveOption);

    auto split = orig.split(' ');

    QMutableListIterator it(split);
    while (it.hasNext()) {
        auto s = it.next();

        const auto match = re.match(s);
        if (match.hasMatch()) {
            it.setValue(QString("<a href=\"%1\">%1</a>").arg(s));
        }
    }

    return split.join(' ');
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

        // Use word boundaries (\b) to prevent name splitting.
        // escape(name) prevents regex injections.
        const QRegularExpression regex(QString(R"(\b%1\b)").arg(QRegularExpression::escape(name)));
        const QString replacement = QString("[%1](chat://%2)").arg(name, user->id());
        str.replace(regex, replacement);
    }

    return str;
}

} // namespace ChatMessageTransformer
