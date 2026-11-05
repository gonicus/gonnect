#include "Sip.test.h"
#include "PreferredIdentityValidator.h"

#include <QTest>

SipTest::SipTest(QObject *parent) : QObject{ parent } { }

void SipTest::testIsIdentityNumberValid()
{
    const auto &validator = PreferredIdentityValidator::instance();

    // Must be a leading '+' followed by one or more digits
    QVERIFY(validator.isIdentityNumberValid("+49123456"));
    QVERIFY(validator.isIdentityNumberValid("+1"));

    // Missing '+', empty, or bare '+' are invalid
    QVERIFY(!validator.isIdentityNumberValid("49123456"));
    QVERIFY(!validator.isIdentityNumberValid(""));
    QVERIFY(!validator.isIdentityNumberValid("+"));

    // Non-digit content anywhere is invalid (whitespace, letters, doubled '+')
    QVERIFY(!validator.isIdentityNumberValid("+49 123"));
    QVERIFY(!validator.isIdentityNumberValid("+49a"));
    QVERIFY(!validator.isIdentityNumberValid("++49"));
    QVERIFY(!validator.isIdentityNumberValid("+49-123"));
}

void SipTest::testIsDisplayNameValid()
{
    const auto &validator = PreferredIdentityValidator::instance();

    // Any non-empty string is a valid display name
    QVERIFY(validator.isDisplayNameValid("Alice"));
    QVERIFY(validator.isDisplayNameValid(" "));

    QVERIFY(!validator.isDisplayNameValid(""));
}

void SipTest::testIsPrefixValid()
{
    const auto &validator = PreferredIdentityValidator::instance();

    // The prefix is currently unconstrained — anything, including empty, is accepted
    QVERIFY(validator.isPrefixValid(""));
    QVERIFY(validator.isPrefixValid("0"));
    QVERIFY(validator.isPrefixValid("anything"));
}

QTEST_GUILESS_MAIN(SipTest)
