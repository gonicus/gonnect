#include "Helpers.test.h"
#include "TextFormatHelper.h"
#include "SecretGenerator.h"

#include <QTest>
#include <QLocale>

HelpersTest::HelpersTest(QObject *parent) : QObject{ parent } { }

void HelpersTest::initTestCase()
{
    // formatFileSize renders numbers via QLocale(). Pin a deterministic locale so the expected
    // strings do not depend on the machine's locale (decimal separator, digit grouping).
    QLocale locale(QLocale::C);
    locale.setNumberOptions(QLocale::OmitGroupSeparator);
    QLocale::setDefault(locale);
}

void HelpersTest::testFormatFileSize()
{
    const auto &helper = TextFormatHelper::instance();

    // Zero and negatives collapse to "0 B"
    QCOMPARE(helper.formatFileSize(0), QString("0 B"));
    QCOMPARE(helper.formatFileSize(-4096), QString("0 B"));

    // Below 1 KiB: bytes, no decimals
    QCOMPARE(helper.formatFileSize(1), QString("1 B"));
    QCOMPARE(helper.formatFileSize(512), QString("512 B"));
    QCOMPARE(helper.formatFileSize(1023), QString("1023 B"));

    // Unit boundaries: two decimals from KiB upwards
    QCOMPARE(helper.formatFileSize(1024), QString("1.00 KB"));
    QCOMPARE(helper.formatFileSize(1536), QString("1.50 KB"));
    QCOMPARE(helper.formatFileSize(1024LL * 1024), QString("1.00 MB"));
    QCOMPARE(helper.formatFileSize(1024LL * 1024 * 1024), QString("1.00 GB"));
}

void HelpersTest::testFormatFileSizeClampsToPetabyte()
{
    const auto &helper = TextFormatHelper::instance();

    // 1024^5 == 1 PiB, the largest known unit
    QCOMPARE(helper.formatFileSize(1024LL * 1024 * 1024 * 1024 * 1024), QString("1.00 PB"));

    // 1024^6 would be an exabyte; the index is clamped to PB and the value scaled accordingly
    QCOMPARE(helper.formatFileSize(1024LL * 1024 * 1024 * 1024 * 1024 * 1024),
             QString("1024.00 PB"));
}

void HelpersTest::testGenerateSecretLength()
{
    QCOMPARE(SecretGenerator::generateSecret(0).length(), 0);
    QCOMPARE(SecretGenerator::generateSecret(16).length(), 16);
    QCOMPARE(SecretGenerator::generateSecret(200).length(), 200);
    // Default argument
    QCOMPARE(SecretGenerator::generateSecret().length(), 16);
}

void HelpersTest::testGenerateSecretCharset()
{
    static const QString allowed =
            "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";

    const QString secret = SecretGenerator::generateSecret(255);
    for (const QChar c : secret) {
        QVERIFY2(allowed.contains(c),
                 qPrintable(QString("Unexpected character '%1' in generated secret").arg(c)));
    }
}

void HelpersTest::testGenerateSecretIsRandom()
{
    // Two independent 32-char secrets colliding is astronomically unlikely (62^32); a match
    // would indicate the generator is not actually random.
    const QString a = SecretGenerator::generateSecret(32);
    const QString b = SecretGenerator::generateSecret(32);
    QVERIFY(a != b);
}

QTEST_GUILESS_MAIN(HelpersTest)
