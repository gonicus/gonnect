#pragma once

#include <QObject>

class RemotePartyIdTest : public QObject
{
    Q_OBJECT
public:
    explicit RemotePartyIdTest(QObject *parent = nullptr);

private slots:
    void testEmpty();
    void testConnectedLineName();
    void testWithoutDisplayName();
    void testCallbackNumberIsUriParameter();
    void testPrivacyOff();
    void testPrivacyName();
    void testPrivacyUri();
    void testPrivacyFull();
    void testUnknownPrivacyIsRestrictive();
    void testMissingPrivacyIsOff();
    void testScreen();
    void testParty();
    void testMultipleHeaders();
    void testCommaSeparatedList();
    void testCommaInsideDisplayName();
    void testBareAddrSpec();
};
