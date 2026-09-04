#include "ChatMessageTransformer.test.h"
#include "ChatMessageTransformer.h"

#include <QString>
#include <QTest>

ChatMessageTransformerTest::ChatMessageTransformerTest(QObject *parent) : QObject{ parent } { }

namespace T = ChatMessageTransformer;

void ChatMessageTransformerTest::testAddLinkTagsPlainUrl()
{
    QCOMPARE(T::addLinkTags("https://example.com"),
             QString(R"(<a href="https://example.com">https://example.com</a>)"));
}
void ChatMessageTransformerTest::testAddLinkTagsWwwUrl()
{
    QCOMPARE(T::addLinkTags("www.example.com"),
             QString(R"(<a href="https://www.example.com">www.example.com</a>)"));
}

void ChatMessageTransformerTest::testAddLinkTagsExistingAnchor()
{
    const QString input = "See <a href=\"https://example.com\">here</a> for details";
    QCOMPARE(T::addLinkTags(input), input);
}

void ChatMessageTransformerTest::testAddLinkTagsNoUrl()
{
    QCOMPARE(T::addLinkTags("Just plain text."), QString("Just plain text."));
}

void ChatMessageTransformerTest::testAddLinkTagsFtpUrl()
{
    QCOMPARE(T::addLinkTags("ftp://files.example.com"),
             QString(R"(<a href="ftp://files.example.com">ftp://files.example.com</a>)"));
}

void ChatMessageTransformerTest::testAddLinkTagsUrlWithPathAndQuery()
{
    QCOMPARE(
            T::addLinkTags("https://example.com/path/to/page?foo=bar&baz=1#section"),
            QString(R"(<a href="https://example.com/path/to/page?foo=bar&baz=1#section">https://example.com/path/to/page?foo=bar&baz=1#section</a>)"));
}

void ChatMessageTransformerTest::testAddLinkTagsTrailingPunctuation()
{
    QCOMPARE(T::addLinkTags("See https://example.com."),
             QString(R"(See <a href="https://example.com">https://example.com</a>.)"));
}

void ChatMessageTransformerTest::testAddLinkTagsMultipleUrls()
{
    QCOMPARE(
            T::addLinkTags("See https://a.com and https://b.com"),
            QString(R"(See <a href="https://a.com">https://a.com</a> and <a href="https://b.com">https://b.com</a>)"));
}

void ChatMessageTransformerTest::testAddLinkTagsMarkdownLink()
{
    const QString input = "[text](https://example.com)";
    QCOMPARE(T::addLinkTags(input), input);
}

void ChatMessageTransformerTest::testAddLinkTagsMixedContent()
{
    QCOMPARE(
            T::addLinkTags(
                    "Click <a href=\"x\">here</a> or go https://y.com and [md](https://z.com)"),
            QString(R"(Click <a href="x">here</a> or go <a href="https://y.com">https://y.com</a> and [md](https://z.com))"));
}

void ChatMessageTransformerTest::testAddLinkTagsEmptyString()
{
    QCOMPARE(T::addLinkTags(""), QString(""));
}

void ChatMessageTransformerTest::testAddLinkTagsMarkdownLinkParensInUrl()
{
    const QString input = QStringLiteral("[text](https://en.wikipedia.org/wiki/Foo_(bar))");
    QCOMPARE(T::addLinkTags(input), input);
}

void ChatMessageTransformerTest::testAddLinkTagsPlainUrlWithParens()
{
    const QString input = "https://en.wikipedia.org/wiki/Foo_(bar)";
    const QString expected =
            QStringLiteral("<a "
                           "href=\"https://en.wikipedia.org/wiki/Foo_(bar\">https://"
                           "en.wikipedia.org/wiki/Foo_(bar</a>)");
    QCOMPARE(T::addLinkTags(input), expected);
}

void ChatMessageTransformerTest::testFixNewLines()
{
    QCOMPARE(T::fixNewLines(""), "");

    // No newlines
    QCOMPARE(T::fixNewLines("Hallo World"), "Hallo World");

    // Single newlines -> \\\n (line break)
    QCOMPARE(T::fixNewLines("Line 1\nLine 2"), "Line 1\\\nLine 2");
    QCOMPARE(T::fixNewLines("Text\n"), "Text\\\n");
    QCOMPARE(T::fixNewLines("\n"), "\\\n");
    QCOMPARE(T::fixNewLines("\nA"), "\\\nA");
    QCOMPARE(T::fixNewLines("A\nB\nC"), "A\\\nB\\\nC");

    // Multiple consecutive newlines -> \\\n\u2060\\\n (paragraph break via two line breaks)
    QCOMPARE(T::fixNewLines("A\n\nB"), QStringLiteral("A\\\n\u2060\\\nB"));
    QCOMPARE(T::fixNewLines("\n\n"), QStringLiteral("\\\n\u2060\\\n"));

    // Triple consecutive newlines
    QCOMPARE(T::fixNewLines("A\n\n\nB"), QStringLiteral("A\\\n\u2060\\\n\u2060\\\nB"));
    QCOMPARE(T::fixNewLines("\n\n\n"), QStringLiteral("\\\n\u2060\\\n\u2060\\\n"));

    // Mixed single and double newlines
    QCOMPARE(T::fixNewLines("A\nB\n\nC"), QStringLiteral("A\\\nB\\\n\u2060\\\nC"));
    QCOMPARE(T::fixNewLines("A\n\nB\nC"), QStringLiteral("A\\\n\u2060\\\nB\\\nC"));

    // Idempotency
    QCOMPARE(T::fixNewLines("A\\\nB"), "A\\\nB");
}

QTEST_GUILESS_MAIN(ChatMessageTransformerTest)
