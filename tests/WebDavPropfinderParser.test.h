#pragma once

#include <QObject>

class WebDavPropfindParserTest : public QObject
{
    Q_OBJECT

public:
    explicit WebDavPropfindParserTest(QObject *parent = nullptr);

private Q_SLOTS:
    void testParsesCollectionAndEtag();
    void testSkipsPropertiesFrom404Propstat();
    void testHandlesResourceLevelForbiddenStatus();
    void testParsesMultipleResponses();
    void testParsesNonDavNamespaceProperty();
    void testBuildPropfindBodyProducesValidXmlWithMultipleNamespaces();
    void testParseMultiStatusHandlesEmptyInput();
};
