#include "RemotePartyId.test.h"
#include "SIPRemotePartyId.h"

#include <QTest>

RemotePartyIdTest::RemotePartyIdTest(QObject *parent) : QObject{ parent } { }

void RemotePartyIdTest::testEmpty()
{
    QVERIFY(SIPRemotePartyId::parse({}).isEmpty());
    QVERIFY(SIPRemotePartyId::parse({ "" }).isEmpty());
}

void RemotePartyIdTest::testConnectedLineName()
{
    // Example from the CUCM SIP line messaging guide: we dialled 9728135001 and the PBX tells us
    // it is Bob Jones.
    const auto list = SIPRemotePartyId::parse(
            { "\"Bob Jones\" <sip:9728135001@10.10.10.2>;party=called;screen=yes;privacy=off" });

    QCOMPARE(list.size(), 1);
    const auto &rpid = list.at(0);
    QVERIFY(rpid.isValid());
    QCOMPARE(rpid.rawDisplayName(), QString("Bob Jones"));
    QCOMPARE(rpid.rawNumber(), QString("9728135001"));
    QCOMPARE(rpid.uri(), QString("sip:9728135001@10.10.10.2"));
    QCOMPARE(rpid.party(), SIPRemotePartyId::Party::Called);
    QCOMPARE(rpid.name(), QString("Bob Jones"));
    QCOMPARE(rpid.number(), QString("9728135001"));
}

void RemotePartyIdTest::testWithoutDisplayName()
{
    const auto list = SIPRemotePartyId::parse(
            { "<sip:3100@172.18.202.96>;party=calling;screen=yes;privacy=off" });

    QCOMPARE(list.size(), 1);
    QVERIFY(list.at(0).rawDisplayName().isEmpty());
    QCOMPARE(list.at(0).rawNumber(), QString("3100"));
}

void RemotePartyIdTest::testCallbackNumberIsUriParameter()
{
    // x-cisco-callback-number sits inside the angle brackets, next to the URI - not among the
    // header parameters. The user part keeps the localized form.
    const auto list = SIPRemotePartyId::parse(
            { "<sip:2325757@172.18.202.96;x-cisco-callback-number=92325757>;party=calling;"
              "screen=yes;privacy=off" });

    QCOMPARE(list.size(), 1);
    QCOMPARE(list.at(0).rawNumber(), QString("2325757"));
    QCOMPARE(list.at(0).callbackNumber(), QString("92325757"));
    QCOMPARE(list.at(0).uri(), QString("sip:2325757@172.18.202.96"));
}

void RemotePartyIdTest::testPrivacyOff()
{
    const auto list = SIPRemotePartyId::parse(
            { "\"sip line\" <sip:69005@10.10.10.2>;party=calling;privacy=off;screen=yes" });

    QVERIFY(!list.at(0).isNameRestricted());
    QVERIFY(!list.at(0).isNumberRestricted());
    QCOMPARE(list.at(0).name(), QString("sip line"));
    QCOMPARE(list.at(0).number(), QString("69005"));
}

void RemotePartyIdTest::testPrivacyName()
{
    // The header still carries the real name, only the parameter marks it as restricted.
    const auto list = SIPRemotePartyId::parse({ "\"Anonymous\" <sip:69005@10.10.10.2>;"
                                                "party=calling;id-type=subscriber;privacy=name;"
                                                "screen=yes" });

    const auto &rpid = list.at(0);
    QVERIFY(rpid.isNameRestricted());
    QVERIFY(!rpid.isNumberRestricted());
    QCOMPARE(rpid.name(), QString());
    QCOMPARE(rpid.number(), QString("69005"));
}

void RemotePartyIdTest::testPrivacyUri()
{
    const auto list = SIPRemotePartyId::parse({ "\"sip line\" <sip:69005@10.10.10.2>;"
                                                "party=calling;id-type=subscriber;privacy=uri;"
                                                "screen=yes" });

    const auto &rpid = list.at(0);
    QVERIFY(!rpid.isNameRestricted());
    QVERIFY(rpid.isNumberRestricted());
    QCOMPARE(rpid.name(), QString("sip line"));
    // The real number is still in the header, but must not be shown.
    QCOMPARE(rpid.rawNumber(), QString("69005"));
    QCOMPARE(rpid.number(), QString());
}

void RemotePartyIdTest::testPrivacyFull()
{
    const auto list = SIPRemotePartyId::parse({ "\"sip line\" <sip:69005@10.10.10.2>;"
                                                "party=calling;id-type=subscriber;privacy=full;"
                                                "screen=yes" });

    const auto &rpid = list.at(0);
    QVERIFY(rpid.isNameRestricted());
    QVERIFY(rpid.isNumberRestricted());
    QCOMPARE(rpid.name(), QString());
    QCOMPARE(rpid.number(), QString());
    // ... while the raw values survive for diagnostics.
    QCOMPARE(rpid.rawDisplayName(), QString("sip line"));
    QCOMPARE(rpid.rawNumber(), QString("69005"));
}

void RemotePartyIdTest::testUnknownPrivacyIsRestrictive()
{
    // A value we do not understand must not be treated as "show everything".
    const auto list =
            SIPRemotePartyId::parse({ "\"Bob\" <sip:1000@host>;party=calling;privacy=bogus" });

    QCOMPARE(list.at(0).privacy(), SIPRemotePartyId::Privacy::Full);
    QCOMPARE(list.at(0).name(), QString());
    QCOMPARE(list.at(0).number(), QString());
}

void RemotePartyIdTest::testMissingPrivacyIsOff()
{
    const auto list = SIPRemotePartyId::parse({ "\"Bob\" <sip:1000@host>;party=calling" });

    QCOMPARE(list.at(0).privacy(), SIPRemotePartyId::Privacy::Off);
    QCOMPARE(list.at(0).name(), QString("Bob"));
}

void RemotePartyIdTest::testScreen()
{
    QVERIFY(SIPRemotePartyId::parse({ "<sip:1000@host>;screen=yes" }).at(0).isScreened());
    QVERIFY(!SIPRemotePartyId::parse({ "<sip:1000@host>;screen=no" }).at(0).isScreened());
    QVERIFY(!SIPRemotePartyId::parse({ "<sip:1000@host>" }).at(0).isScreened());
}

void RemotePartyIdTest::testParty()
{
    QCOMPARE(SIPRemotePartyId::parse({ "<sip:1@h>;party=calling" }).at(0).party(),
             SIPRemotePartyId::Party::Calling);
    QCOMPARE(SIPRemotePartyId::parse({ "<sip:1@h>;party=called" }).at(0).party(),
             SIPRemotePartyId::Party::Called);
    QCOMPARE(SIPRemotePartyId::parse({ "<sip:1@h>" }).at(0).party(),
             SIPRemotePartyId::Party::Unknown);
}

void RemotePartyIdTest::testMultipleHeaders()
{
    const auto list = SIPRemotePartyId::parse(
            { "\"Alice\" <sip:1000@host>;party=calling", "\"Bob\" <sip:2000@host>;party=called" });

    QCOMPARE(list.size(), 2);
    QCOMPARE(list.at(0).party(), SIPRemotePartyId::Party::Calling);
    QCOMPARE(list.at(1).party(), SIPRemotePartyId::Party::Called);
}

void RemotePartyIdTest::testCommaSeparatedList()
{
    const auto list = SIPRemotePartyId::parse(
            { "\"Alice\" <sip:1000@host>;party=calling, \"Bob\" <sip:2000@host>;party=called" });

    QCOMPARE(list.size(), 2);
    QCOMPARE(list.at(0).rawNumber(), QString("1000"));
    QCOMPARE(list.at(1).rawNumber(), QString("2000"));
}

void RemotePartyIdTest::testCommaInsideDisplayName()
{
    // A comma in the display name must not split the entry - the same class of bug that once
    // turned a display name into post dial DTMF digits.
    const auto list =
            SIPRemotePartyId::parse({ "\"Foobar, Dr., M.\" <sip:1000@host>;party=calling" });

    QCOMPARE(list.size(), 1);
    QCOMPARE(list.at(0).rawDisplayName(), QString("Foobar, Dr., M."));
    QCOMPARE(list.at(0).rawNumber(), QString("1000"));
}

void RemotePartyIdTest::testBareAddrSpec()
{
    const auto list = SIPRemotePartyId::parse({ "sip:1000@host;party=calling;privacy=off" });

    QCOMPARE(list.size(), 1);
    QCOMPARE(list.at(0).rawNumber(), QString("1000"));
    QCOMPARE(list.at(0).party(), SIPRemotePartyId::Party::Calling);
}

QTEST_GUILESS_MAIN(RemotePartyIdTest)
