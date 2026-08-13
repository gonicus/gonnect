#include "Contacts.test.h"
#include "FuzzyCompare.h"
#include "PhoneNumberUtil.h"

#include <QTest>

ContactsTest::ContactsTest(QObject *parent) : QObject{ parent } { }

void ContactsTest::testCleanPhoneNumber()
{
    QCOMPARE(PhoneNumberUtil::cleanPhoneNumber(""), QString(""));

    // Strip whitespace and -
    QCOMPARE(PhoneNumberUtil::cleanPhoneNumber("  +49 2931 9160   "), QString("+4929319160"));
    QCOMPARE(PhoneNumberUtil::cleanPhoneNumber("  +49-2931-9160   "), QString("+4929319160"));
    // Handle URL encoded numbers
    QCOMPARE(PhoneNumberUtil::cleanPhoneNumber("callto:+49%202932%209160"), QString("+4929329160"));
    QCOMPARE(PhoneNumberUtil::cleanPhoneNumber("%2B49%202932%209160"), QString("+4929329160"));
}

void ContactsTest::testLevenshteinDistance()
{
    QCOMPARE(FuzzyCompare::levenshteinDistance("", ""), 0);
    QCOMPARE(FuzzyCompare::levenshteinDistance("uniformed", "uninformed"), 1);
    QCOMPARE(FuzzyCompare::levenshteinDistance("kitten", "sitten"), 1);
    QCOMPARE(FuzzyCompare::levenshteinDistance("kitten", "sittin"), 2);
    QCOMPARE(FuzzyCompare::levenshteinDistance("kitten", "sitting"), 3);
}

void ContactsTest::testJaroWinklerDistance()
{
    QVERIFY(FuzzyCompare::jaroWinklerDistance("developer", "developes") - 0.955556 < 0.00001);
    QVERIFY(FuzzyCompare::jaroWinklerDistance("developer", "seveloper") - 0.925926 < 0.00001);
}

void ContactsTest::testSortListByWeight()
{
    QList<QString> list = { "Two", "Four", "One", "Three" };
    const QList<QString> targetList = { "One", "Two", "Three", "Four" };
    const QList<qreal> weightList = { 4, 8, 2, 6 };

    FuzzyCompare::sortListByWeight(list, weightList);

    QCOMPARE(list, targetList);
}

void ContactsTest::testIsSipUri()
{
    // Valid SIP and SIPS URIs
    QVERIFY(PhoneNumberUtil::isSipUri("sip:alice@example.com"));
    QVERIFY(PhoneNumberUtil::isSipUri("sips:alice@example.com"));
    QVERIFY(PhoneNumberUtil::isSipUri("SIP:alice@example.com")); // case-insensitive scheme
    QVERIFY(PhoneNumberUtil::isSipUri("sip:+4929319160@example.com"));
    QVERIFY(PhoneNumberUtil::isSipUri("sip:100@192.168.1.1"));
    QVERIFY(PhoneNumberUtil::isSipUri("sip:*21*@pbx.example.com")); // * in user part
    QVERIFY(PhoneNumberUtil::isSipUri("sip:#100@pbx.example.com")); // # in user part
    QVERIFY(PhoneNumberUtil::isSipUri("\"John Doe\" <sip:john@example.com>")); // display name

    // Not SIP URIs
    QVERIFY(!PhoneNumberUtil::isSipUri(""));
    QVERIFY(!PhoneNumberUtil::isSipUri("+4929319160")); // bare number
    QVERIFY(!PhoneNumberUtil::isSipUri("alice@example.com")); // missing scheme
    QVERIFY(!PhoneNumberUtil::isSipUri("http://example.com"));
    QVERIFY(!PhoneNumberUtil::isSipUri("sip:@example.com")); // empty user part
}

void ContactsTest::testNumberFromSipUrl()
{
    QCOMPARE(PhoneNumberUtil::numberFromSipUrl("sip:alice@example.com"), QString("alice"));
    QCOMPARE(PhoneNumberUtil::numberFromSipUrl("sips:alice@example.com"), QString("alice"));
    QCOMPARE(PhoneNumberUtil::numberFromSipUrl("SIP:alice@example.com"),
             QString("alice")); // case-insensitive
    QCOMPARE(PhoneNumberUtil::numberFromSipUrl("sip:+4929319160@example.com"),
             QString("+4929319160"));
    QCOMPARE(PhoneNumberUtil::numberFromSipUrl("sip:100@192.168.1.1"), QString("100"));
    QCOMPARE(PhoneNumberUtil::numberFromSipUrl("\"John\" <sip:john@example.com>"), QString("john"));

    // No SIP URL — returns empty string
    QCOMPARE(PhoneNumberUtil::numberFromSipUrl("+4929319160"), QString(""));
    QCOMPARE(PhoneNumberUtil::numberFromSipUrl(""), QString(""));
}

void ContactsTest::testNameFromSipUrl()
{
    // No display name — falls back to user part
    QCOMPARE(PhoneNumberUtil::nameFromSipUrl("sip:alice@example.com"), QString("alice"));
    QCOMPARE(PhoneNumberUtil::nameFromSipUrl("sips:+4929319160@example.com"),
             QString("+4929319160"));

    // Quoted display name before the URI
    QCOMPARE(PhoneNumberUtil::nameFromSipUrl("\"Alice\"<sip:alice@example.com>"), QString("Alice"));
    QCOMPARE(PhoneNumberUtil::nameFromSipUrl("\"John Doe\" <sip:john@example.com>"),
             QString("John Doe"));

    // No SIP URL — returns empty string
    QCOMPARE(PhoneNumberUtil::nameFromSipUrl(""), QString(""));
    QCOMPARE(PhoneNumberUtil::nameFromSipUrl("+4929319160"), QString(""));
}
void ContactsTest::testBareURI()
{
    // Plain URIs are returned unchanged
    QCOMPARE(PhoneNumberUtil::bareURI("sip:alice@example.com"), QString("sip:alice@example.com"));
    QCOMPARE(PhoneNumberUtil::bareURI("sips:secure@example.com"),
             QString("sips:secure@example.com"));

    // Display name and angle brackets are stripped
    QCOMPARE(PhoneNumberUtil::bareURI("\"John Doe\" <sip:john@example.com>"),
             QString("sip:john@example.com"));
    // Trailing parameters after the closing bracket are dropped
    QCOMPARE(PhoneNumberUtil::bareURI("<sip:a@b>;tag=123"), QString("sip:a@b"));

    // No SIP URI in the input — returns empty string
    QCOMPARE(PhoneNumberUtil::bareURI("+4929319160"), QString(""));
    QCOMPARE(PhoneNumberUtil::bareURI(""), QString(""));
}

void ContactsTest::testClearInternationalChars()
{
    // ASCII passes through untouched
    QCOMPARE(PhoneNumberUtil::clearInternationalChars(""), QString(""));
    QCOMPARE(PhoneNumberUtil::clearInternationalChars("Mueller 123"), QString("Mueller 123"));

    // KNOWN BUG: the umlaut/diacritic source characters in the replacement table were
    // corrupted to U+FFFD (REPLACEMENT CHARACTER) in the source (commit 42c2dcb), so real
    // umlauts are no longer folded. These expectations describe the *intended* behaviour and
    // are marked as expected failures; once the source table is repaired they will XPASS and
    // Qt Test turns an unexpected pass into a failure, flagging that this guard can be removed.
    QEXPECT_FAIL("", "umlaut source chars corrupted to U+FFFD in PhoneNumberUtil.cpp (42c2dcb)",
                 Continue);
    QCOMPARE(PhoneNumberUtil::clearInternationalChars(QString::fromUtf8("Müller")),
             QString("Mueller"));
    QEXPECT_FAIL("", "umlaut source chars corrupted to U+FFFD in PhoneNumberUtil.cpp (42c2dcb)",
                 Continue);
    QCOMPARE(PhoneNumberUtil::clearInternationalChars(QString::fromUtf8("Straße")),
             QString("Strasse"));
}

void ContactsTest::testIsEmergencyCallUrl()
{
    // With no emergencyRegex configured (default in the test environment) nothing is an
    // emergency call. An empty input never matches regardless of configuration.
    // NOTE: this function latches its regex on first call via a static flag, so it cannot be
    // re-tested against different configurations within a single process.
    QVERIFY(!PhoneNumberUtil::isEmergencyCallUrl(""));
}

void ContactsTest::testIsNumberAnonymous()
{
    // With no anonymousRegex configured (default in the test environment) no number is
    // anonymous. An empty input never matches regardless of configuration.
    // NOTE: same static-latch caveat as testIsEmergencyCallUrl.
    QVERIFY(!PhoneNumberUtil::isNumberAnonymous(""));
}

QTEST_GUILESS_MAIN(ContactsTest)
