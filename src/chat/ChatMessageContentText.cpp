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

QString ChatMessageContentText::rawText() const
{
    return m_rawText;
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

    m_simpleText = convertText(m_rawText);

    // Split into code/pre blocks
    qDeleteAll(m_parts);
    m_parts.clear();

    const QByteArray utf8Data = m_rawText.toUtf8();
    QString currentTextBuffer;
    cmark_node *doc =
            cmark_parse_document(utf8Data.constData(), utf8Data.size(), CMARK_OPT_DEFAULT);
    cmark_iter *iter = cmark_iter_new(doc);
    cmark_event_type ev_type;

    while ((ev_type = cmark_iter_next(iter)) != CMARK_EVENT_DONE) {
        cmark_node *currentNode = cmark_iter_get_node(iter);
        cmark_node_type type = cmark_node_get_type(currentNode);

        if (ev_type == CMARK_EVENT_ENTER) {
            const auto lit = QString::fromUtf8(cmark_node_get_literal(currentNode));

            if (type == CMARK_NODE_CODE_BLOCK) {

                // Remaining normal text
                if (!currentTextBuffer.isEmpty()) {
                    m_parts.append(new ChatMessageContentPart(
                            false, convertText(currentTextBuffer.trimmed()), "", this));
                    currentTextBuffer.clear();
                }

                // Code block
                m_parts.append(new ChatMessageContentPart(
                        true, lit, QString::fromUtf8(cmark_node_get_fence_info(currentNode)),
                        this));

            } else if (type == CMARK_NODE_HTML_BLOCK
                       && lit.trimmed().startsWith("<pre", Qt::CaseInsensitive)) {

                // <pre>-Block
                // Strip outer tags
                const auto openTagEnd = lit.indexOf('>');
                const auto closeTagStart = lit.lastIndexOf("</pre", -1, Qt::CaseInsensitive);
                if (openTagEnd != -1 && closeTagStart > openTagEnd) {

                    // Remaining normal text
                    if (!currentTextBuffer.isEmpty()) {
                        m_parts.append(new ChatMessageContentPart(
                                false, convertText(currentTextBuffer.trimmed()), "", this));
                        currentTextBuffer.clear();
                    }

                    // Stripped content
                    const auto innerContent =
                            lit.mid(openTagEnd + 1, closeTagStart - openTagEnd - 1);
                    m_parts.append(new ChatMessageContentPart(true, innerContent, "", this));
                } else {
                    // Buffer normal text
                    currentTextBuffer += lit;
                }

            } else if (!lit.isEmpty()) {
                // Buffer normal text
                currentTextBuffer += lit;
            }
        }
    }

    // Remaining buffered text
    if (!currentTextBuffer.isEmpty()) {
        m_parts.append(new ChatMessageContentPart(false, convertText(currentTextBuffer.trimmed()),
                                                  "", this));
        currentTextBuffer.clear();
    }

    cmark_iter_free(iter);
    cmark_node_free(doc);

    Q_EMIT contentChanged();
}

QString ChatMessageContentText::convertText(const QString &originalText) const
{
    if (const auto *chatMessageObj = qobject_cast<ChatMessage *>(parent())) {
        return ChatMessageTransformer::addLinkTags(
                ChatMessageTransformer::highlightMentions(originalText, *chatMessageObj));
    }

    return ChatMessageTransformer::addLinkTags(originalText);
}
