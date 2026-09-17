#pragma once

#include <QObject>

class ChatMessageTransformerTest : public QObject
{
    Q_OBJECT

public:
    explicit ChatMessageTransformerTest(QObject *parent = nullptr);

private slots:
    void testLinkifyBareUrlsPlainUrl();
    void testLinkifyBareUrlsWwwUrl();
    void testLinkifyBareUrlsExistingAnchor();
    void testLinkifyBareUrlsNoUrl();
    void testLinkifyBareUrlsFtpUrl();
    void testLinkifyBareUrlsUrlWithPathAndQuery();
    void testLinkifyBareUrlsTrailingPunctuation();
    void testLinkifyBareUrlsMultipleUrls();
    void testLinkifyBareUrlsMarkdownLink();
    void testLinkifyBareUrlsMixedContent();
    void testLinkifyBareUrlsEmptyString();
    void testLinkifyBareUrlsMarkdownLinkParensInUrl();
    void testLinkifyBareUrlsPlainUrlWithParens();
    void testLinkifyBareUrlsCodeSpan();
    void testLinkifyBareUrlsBareDomainWithText();

    void testMarkdownToHtmlParagraphs();
    void testMarkdownToHtmlHardBreak();
    void testMarkdownToHtmlList();
    void testMarkdownToHtmlListFromIssue();
    void testMarkdownToHtmlLink();
    void testMarkdownToHtmlCodeSpan();
};
