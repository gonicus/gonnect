#pragma once

#include <QObject>

class ChatMessageTransformerTest : public QObject
{
    Q_OBJECT

public:
    explicit ChatMessageTransformerTest(QObject *parent = nullptr);

private slots:
    void testAddLinkTagsPlainUrl();
    void testAddLinkTagsWwwUrl();
    void testAddLinkTagsExistingAnchor();
    void testAddLinkTagsNoUrl();
    void testAddLinkTagsFtpUrl();
    void testAddLinkTagsUrlWithPathAndQuery();
    void testAddLinkTagsTrailingPunctuation();
    void testAddLinkTagsMultipleUrls();
    void testAddLinkTagsMarkdownLink();
    void testAddLinkTagsMixedContent();
    void testAddLinkTagsEmptyString();
    void testAddLinkTagsMarkdownLinkParensInUrl();
    void testAddLinkTagsPlainUrlWithParens();

    void testFixNewLines();
};
