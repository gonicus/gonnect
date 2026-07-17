#include "ChatMessageTransformer.test.h"
#include "ChatMessageTransformer.h"

#include <QString>
#include <QTest>

ChatMessageTransformerTest::ChatMessageTransformerTest(QObject *parent) : QObject{ parent } { }

void ChatMessageTransformerTest::testAddLinkTagsPlainUrl()
{
    QCOMPARE(ChatMessageTransformer::addLinkTags("https://example.com"),
             QString(R"(<a href="https://example.com">https://example.com</a>)"));
}
void ChatMessageTransformerTest::testAddLinkTagsWwwUrl()
{
    QCOMPARE(ChatMessageTransformer::addLinkTags("www.example.com"),
             QString(R"(<a href="https://www.example.com">www.example.com</a>)"));
}

void ChatMessageTransformerTest::testAddLinkTagsExistingAnchor()
{
    const QString input = "See <a href=\"https://example.com\">here</a> for details";
    QCOMPARE(ChatMessageTransformer::addLinkTags(input), input);
}

void ChatMessageTransformerTest::testAddLinkTagsNoUrl()
{
    QCOMPARE(ChatMessageTransformer::addLinkTags("Just plain text."), QString("Just plain text."));
}

void ChatMessageTransformerTest::testAddLinkTagsFtpUrl()
{
    QCOMPARE(ChatMessageTransformer::addLinkTags("ftp://files.example.com"),
             QString(R"(<a href="ftp://files.example.com">ftp://files.example.com</a>)"));
}

void ChatMessageTransformerTest::testAddLinkTagsUrlWithPathAndQuery()
{
    QCOMPARE(
            ChatMessageTransformer::addLinkTags(
                    "https://example.com/path/to/page?foo=bar&baz=1#section"),
            QString(R"(<a href="https://example.com/path/to/page?foo=bar&baz=1#section">https://example.com/path/to/page?foo=bar&baz=1#section</a>)"));
}

void ChatMessageTransformerTest::testAddLinkTagsTrailingPunctuation()
{
    QCOMPARE(ChatMessageTransformer::addLinkTags("See https://example.com."),
             QString(R"(See <a href="https://example.com">https://example.com</a>.)"));
}

void ChatMessageTransformerTest::testAddLinkTagsMultipleUrls()
{
    QCOMPARE(
            ChatMessageTransformer::addLinkTags("See https://a.com and https://b.com"),
            QString(R"(See <a href="https://a.com">https://a.com</a> and <a href="https://b.com">https://b.com</a>)"));
}

void ChatMessageTransformerTest::testAddLinkTagsMarkdownLink()
{
    const QString input = "[text](https://example.com)";
    QCOMPARE(ChatMessageTransformer::addLinkTags(input), input);
}

void ChatMessageTransformerTest::testAddLinkTagsMixedContent()
{
    QCOMPARE(
            ChatMessageTransformer::addLinkTags(
                    "Click <a href=\"x\">here</a> or go https://y.com and [md](https://z.com)"),
            QString(R"(Click <a href="x">here</a> or go <a href="https://y.com">https://y.com</a> and [md](https://z.com))"));
}

void ChatMessageTransformerTest::testAddLinkTagsEmptyString()
{
    QCOMPARE(ChatMessageTransformer::addLinkTags(""), QString(""));
}
QTEST_GUILESS_MAIN(ChatMessageTransformerTest)
