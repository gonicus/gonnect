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

    // 00 at beginning is replaced with +
    QCOMPARE(PhoneNumberUtil::cleanPhoneNumber("0029319160"), QString("+29319160"));

    // 0 at beginning is replaced with +49
    QCOMPARE(PhoneNumberUtil::cleanPhoneNumber("029319160"), QString("+4929319160"));

    // Every number must start with +
    QCOMPARE(PhoneNumberUtil::cleanPhoneNumber("29319160"), QString("+29319160"));

    // Internal numbers (up to 3 digits) must remain unchainged
    QCOMPARE(PhoneNumberUtil::cleanPhoneNumber("  123 "), QString("123"));
    QCOMPARE(PhoneNumberUtil::cleanPhoneNumber("123"), QString("123"));
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

QTEST_GUILESS_MAIN(ContactsTest)
