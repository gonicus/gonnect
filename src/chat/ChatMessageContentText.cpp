#include "ChatMessageContentText.h"
#include "ChatMessageTransformer.h"
#include "ChatMessage.h"
#include <cmark.h>
#include <QEvent>

ChatMessageContentText::ChatMessageContentText(const QString &text, QObject *parent)
    : QObject{ parent }
{
    setText(text);
}

bool ChatMessageContentText::isSimpleText() const
{
    return m_parts.length() == 1 && !m_parts.first()->isCode();
}

QString ChatMessageContentText::simpleText() const
{
    return m_simpleText;
}

void ChatMessageContentText::setText(const QString &text)
{
    if (m_rawText == text) {
        return;
    }
    m_rawText = text;
    processText();
}

void ChatMessageContentText::processText()
{
    const auto *chatMessageObj = qobject_cast<ChatMessage *>(parent());
    if (!chatMessageObj) {
        return;
    }

    m_simpleText = m_rawText;
    m_htmlText = convertHtmlText(m_rawText);

    // Split into code/pre blocks
    qDeleteAll(m_parts);
    m_parts.clear();

    const QByteArray utf8Data = m_rawText.toUtf8();
    cmark_node *doc =
            cmark_parse_document(utf8Data.constData(), utf8Data.size(), CMARK_OPT_DEFAULT);
    cmark_iter *iter = cmark_iter_new(doc);

    struct CodeRange
    {
        int startLine = 0;
        int endLine = 0;
        QString literal;
        QString fenceInfo;
    };
    QList<CodeRange> codeRanges;

    cmark_event_type ev_type;
    while ((ev_type = cmark_iter_next(iter)) != CMARK_EVENT_DONE) {
        if (ev_type != CMARK_EVENT_ENTER) {
            continue;
        }
        cmark_node *currentNode = cmark_iter_get_node(iter);
        const auto type = cmark_node_get_type(currentNode);
        if (type == CMARK_NODE_CODE_BLOCK) {
            CodeRange range;
            range.startLine = cmark_node_get_start_line(currentNode);
            range.endLine = cmark_node_get_end_line(currentNode);
            range.literal = QString::fromUtf8(cmark_node_get_literal(currentNode));
            range.fenceInfo = QString::fromUtf8(cmark_node_get_fence_info(currentNode));
            codeRanges.append(range);
        } else if (type == CMARK_NODE_HTML_BLOCK) {
            const auto lit = QString::fromUtf8(cmark_node_get_literal(currentNode));
            if (lit.trimmed().startsWith("<pre", Qt::CaseInsensitive)) {
                const auto openTagEnd = lit.indexOf('>');
                const auto closeTagStart = lit.lastIndexOf("</pre", -1, Qt::CaseInsensitive);
                if (openTagEnd != -1 && closeTagStart > openTagEnd) {
                    CodeRange range;
                    range.startLine = cmark_node_get_start_line(currentNode);
                    range.endLine = cmark_node_get_end_line(currentNode);
                    range.literal = lit.mid(openTagEnd + 1, closeTagStart - openTagEnd - 1);
                    codeRanges.append(range);
                }
            }
        }
    }

    cmark_iter_free(iter);
    cmark_node_free(doc);

    // Slice original Markdown, structure like lists and blank lines is preserved.
    const QStringList allLines = m_rawText.split(QStringLiteral("\n"));
    auto appendTextPart = [&](const QString &markdown) {
        const QString buffer = markdown.trimmed();
        if (buffer.isEmpty()) {
            return;
        }
        m_parts.append(
                new ChatMessageContentPart(false, buffer, convertHtmlText(buffer), "", this));
    };
    int cursorLine = 1;
    for (const auto &range : codeRanges) {
        appendTextPart(allLines.sliced(cursorLine - 1, range.startLine - cursorLine)
                               .join(QStringLiteral("\n")));
        m_parts.append(
                new ChatMessageContentPart(true, range.literal, QString(), range.fenceInfo, this));
        cursorLine = range.endLine + 1;
    }
    appendTextPart(allLines.sliced(cursorLine - 1).join(QStringLiteral("\n")));

    Q_EMIT contentChanged();
}

QString ChatMessageContentText::convertHtmlText(const QString &originalText) const
{
    namespace T = ChatMessageTransformer;

    if (const auto *chatMessageObj = qobject_cast<ChatMessage *>(parent())) {
        return T::markdownToHtml(T::highlightMentions(originalText, *chatMessageObj));
    }

    return T::markdownToHtml(originalText);
}
