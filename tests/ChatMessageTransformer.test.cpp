#include "ChatMessageTransformer.test.h"
#include "ChatMessageTransformer.h"

#include <QString>
#include <QTest>

ChatMessageTransformerTest::ChatMessageTransformerTest(QObject *parent) : QObject{ parent } { }

namespace T = ChatMessageTransformer;

void ChatMessageTransformerTest::testLinkifyBareUrlsPlainUrl()
{
    QCOMPARE(T::linkifyBareUrls("https://example.com"),
             QString("[https://example.com](https://example.com)"));
}

void ChatMessageTransformerTest::testLinkifyBareUrlsWwwUrl()
{
    QCOMPARE(T::linkifyBareUrls("www.example.com"),
             QString("[www.example.com](https://www.example.com)"));
}

void ChatMessageTransformerTest::testLinkifyBareUrlsExistingAnchor()
{
    const QString input = "See <a href=\"https://example.com\">here</a> for details";
    QCOMPARE(T::linkifyBareUrls(input), input);
}

void ChatMessageTransformerTest::testLinkifyBareUrlsNoUrl()
{
    QCOMPARE(T::linkifyBareUrls("Just plain text."), QString("Just plain text."));
}

void ChatMessageTransformerTest::testLinkifyBareUrlsFtpUrl()
{
    QCOMPARE(T::linkifyBareUrls("ftp://files.example.com"),
             QString("[ftp://files.example.com](ftp://files.example.com)"));
}

void ChatMessageTransformerTest::testLinkifyBareUrlsUrlWithPathAndQuery()
{
    QCOMPARE(T::linkifyBareUrls("https://example.com/path/to/page?foo=bar&baz=1#section"),
             QString("[https://example.com/path/to/page?foo=bar&baz=1#section](https://"
                     "example.com/path/to/page?foo=bar&baz=1#section)"));
}

void ChatMessageTransformerTest::testLinkifyBareUrlsTrailingPunctuation()
{
    QCOMPARE(T::linkifyBareUrls("See https://example.com."),
             QString("See [https://example.com](https://example.com)."));
}

void ChatMessageTransformerTest::testLinkifyBareUrlsMultipleUrls()
{
    QCOMPARE(T::linkifyBareUrls("See https://a.com and https://b.com"),
             QString("See [https://a.com](https://a.com) and [https://b.com](https://b.com)"));
}

void ChatMessageTransformerTest::testLinkifyBareUrlsMarkdownLink()
{
    const QString input = "[text](https://example.com)";
    QCOMPARE(T::linkifyBareUrls(input), input);
}

void ChatMessageTransformerTest::testLinkifyBareUrlsMixedContent()
{
    QCOMPARE(T::linkifyBareUrls(
                     "Click <a href=\"x\">here</a> or go https://y.com and [md](https://z.com)"),
             QString("Click <a href=\"x\">here</a> or go [https://y.com](https://y.com) and "
                     "[md](https://z.com)"));
}

void ChatMessageTransformerTest::testLinkifyBareUrlsEmptyString()
{
    QCOMPARE(T::linkifyBareUrls(""), QString(""));
}

void ChatMessageTransformerTest::testLinkifyBareUrlsMarkdownLinkParensInUrl()
{
    const QString input = QStringLiteral("[text](https://en.wikipedia.org/wiki/Foo_(bar))");
    QCOMPARE(T::linkifyBareUrls(input), input);
}

void ChatMessageTransformerTest::testLinkifyBareUrlsPlainUrlWithParens()
{
    QCOMPARE(T::linkifyBareUrls("https://en.wikipedia.org/wiki/Foo_(bar)"),
             QStringLiteral("[https://en.wikipedia.org/wiki/Foo_(bar](https://en.wikipedia.org/"
                            "wiki/Foo_(bar))"));
}

void ChatMessageTransformerTest::testLinkifyBareUrlsCodeSpan()
{
    const QString input = QStringLiteral("`https://a.com`");
    QCOMPARE(T::linkifyBareUrls(input), input);
}

void ChatMessageTransformerTest::testLinkifyBareUrlsBareDomainWithText()
{
    const QString input = QStringLiteral("meet.irgeneineurl.de Punkt 12");
    QCOMPARE(T::linkifyBareUrls(input), input);
}

void ChatMessageTransformerTest::testMarkdownToHtmlParagraphs()
{
    QCOMPARE(T::markdownToHtml(""), QString(""));
    QCOMPARE(T::markdownToHtml("Hallo World"), QString("<p>Hallo World</p>\n"));
    QCOMPARE(T::markdownToHtml("A\n\nB"), QString("<p>A</p>\n<p>B</p>\n"));
}

void ChatMessageTransformerTest::testMarkdownToHtmlHardBreak()
{
    QCOMPARE(T::markdownToHtml("Line 1\nLine 2"), QString("<p>Line 1<br />\nLine 2</p>\n"));
}

void ChatMessageTransformerTest::testMarkdownToHtmlList()
{
    const QString output = T::markdownToHtml("* a\n* b");
    QVERIFY(!output.contains(QChar('\\')));
    QCOMPARE(output.count(QStringLiteral("<li>")), 2);
    QVERIFY(output.contains(QStringLiteral("<ul>")));
}

void ChatMessageTransformerTest::testMarkdownToHtmlListFromIssue()
{
    const QString input = QStringLiteral("Nicht-Bulletin 1:\n"
                                         "\n"
                                         "* Punkt 1\n"
                                         "    * Punkt 1.1\n"
                                         "    * Punkt 1.2\n"
                                         "* Punkt 2\n"
                                         "* Punkt 3\n"
                                         "* Punkt 4\n"
                                         "* Punkt 5\n"
                                         "    * Punkt 5.1\n"
                                         "* Punkt 6\n"
                                         "* Punkt 7\n"
                                         "* Punkt 8\n"
                                         "* Punkt 9\n"
                                         "    * Punkt 9.1\n"
                                         "\n"
                                         "Nicht-Bulletin 2:\n"
                                         "\n"
                                         "* Punkt 10\n"
                                         "* Punkt 11\n"
                                         "    * Punkt 11.1\n"
                                         "* meet.irgeneineurl.de Punkt 12\n"
                                         "* Punkt 13\n"
                                         "* Punkt 14");
    const QString output = T::markdownToHtml(input);
    QVERIFY(!output.contains(QChar('\\')));
    QCOMPARE(output.count(QStringLiteral("<li>")), 19);
    QVERIFY(output.contains(QStringLiteral("<p>Nicht-Bulletin 1:</p>")));
    QVERIFY(output.contains(QStringLiteral("<p>Nicht-Bulletin 2:</p>")));
    QVERIFY(output.contains(QStringLiteral("meet.irgeneineurl.de Punkt 12")));
}

void ChatMessageTransformerTest::testMarkdownToHtmlLink()
{
    const QString output = T::markdownToHtml("See https://example.com.");
    QVERIFY(output.contains(
            QStringLiteral("<a href=\"https://example.com\">https://example.com</a>")));
}

void ChatMessageTransformerTest::testMarkdownToHtmlCodeSpan()
{
    const QString output = T::markdownToHtml("`https://a.com`");
    QVERIFY(output.contains(QStringLiteral("<code>https://a.com</code>")));
    QVERIFY(!output.contains(QStringLiteral("<a href")));
}

QTEST_GUILESS_MAIN(ChatMessageTransformerTest)
