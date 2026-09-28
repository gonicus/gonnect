#include "Usb.test.h"
#include "ReportDescriptorParser.h"
#include "ReportDescriptorStructs.h"
#include "ReportDescriptorEnums.h"

#include <QTest>
#include <QFile>

#ifndef TEST_DATA_DIR
#  define TEST_DATA_DIR "."
#endif

namespace {

QByteArray loadDescriptor(const QString &relativePath)
{
    QFile file(QString::fromUtf8(TEST_DATA_DIR) + "/" + relativePath);
    if (!file.open(QIODevice::ReadOnly)) {
        return {};
    }
    return file.readAll();
}

} // namespace

UsbTest::UsbTest(QObject *parent) : QObject{ parent } { }

void UsbTest::testSyntheticHeadsetParseTree()
{
    const QByteArray data = loadDescriptor("hid/synthetic-telephony-headset.bin");
    QVERIFY2(!data.isEmpty(), "fixture hid/synthetic-telephony-headset.bin missing");

    ReportDescriptorParser parser;
    const auto appColl = parser.parse(data);

    QVERIFY(appColl);
    QCOMPARE(appColl->rawPageId, quint32(0x0B)); // Telephony
    QCOMPARE(appColl->rawUsageId, quint32(0x05)); // Headset

    // Two reports: input controls (id 1), LED outputs (id 2)
    QCOMPARE(appColl->reports.size(), qsizetype(2));
    QCOMPARE(appColl->reports.at(0)->id, quint32(1));
    QCOMPARE(appColl->reports.at(1)->id, quint32(2));

    // Hook switch: first input bit of report 1
    const auto hook = appColl->findUsage(UsageId::Telephony_HookSwitch);
    QVERIFY(hook.isValid());
    QCOMPARE(hook.report->id, quint32(1));
    QCOMPARE(hook.usage->type, UsageType::Input);
    QCOMPARE(hook.usage->size, quint32(1));
    QCOMPARE(hook.bitPositionInReport, qsizetype(0));

    // Phone mute: second input bit
    const auto mute = appColl->findUsage(UsageId::Telephony_PhoneMute);
    QVERIFY(mute.isValid());
    QCOMPARE(mute.report->id, quint32(1));
    QCOMPARE(mute.bitPositionInReport, qsizetype(1));

    // Report 1 = 2 control bits + 6 padding bits = one byte
    QCOMPARE(appColl->reports.at(0)->bitFieldLength(), qsizetype(8));

    // LEDs live in report 2 as outputs, in declaration order
    const auto ledOffHook = appColl->findUsage(UsageId::LED_OffHook);
    QVERIFY(ledOffHook.isValid());
    QCOMPARE(ledOffHook.report->id, quint32(2));
    QCOMPARE(ledOffHook.usage->type, UsageType::Output);
    QCOMPARE(ledOffHook.bitPositionInReport, qsizetype(0));

    QCOMPARE(appColl->findUsage(UsageId::LED_Mute).bitPositionInReport, qsizetype(1));
    QCOMPARE(appColl->findUsage(UsageId::LED_Ring).bitPositionInReport, qsizetype(2));
}

void UsbTest::testSyntheticTeamsReportIds()
{
    const QByteArray data = loadDescriptor("hid/synthetic-teams-vendor.bin");
    QVERIFY2(!data.isEmpty(), "fixture hid/synthetic-teams-vendor.bin missing");

    ReportDescriptorParser parser;
    const auto info = parser.parseTeamsReportIDs(data);

    QCOMPARE(info.size(), 3);
    QCOMPARE(info.value(UsageId::Teams_VendorExtension), quint16(5));
    QCOMPARE(info.value(UsageId::Teams_DisplayControl), quint16(6));
    QCOMPARE(info.value(UsageId::Teams_Button), quint16(7));
}

void UsbTest::testParseIgnoresNonHeadset()
{
    // The Teams vendor descriptor is not a Telephony/Headset application collection,
    // so parse() must not report one.
    const QByteArray data = loadDescriptor("hid/synthetic-teams-vendor.bin");
    QVERIFY2(!data.isEmpty(), "fixture hid/synthetic-teams-vendor.bin missing");

    ReportDescriptorParser parser;
    QVERIFY(!parser.parse(data));
}

void UsbTest::testTeamsIgnoresNonTeams()
{
    // The plain headset descriptor has no vendor (0xFF99) page, so no Teams report ids.
    const QByteArray data = loadDescriptor("hid/synthetic-telephony-headset.bin");
    QVERIFY2(!data.isEmpty(), "fixture hid/synthetic-telephony-headset.bin missing");

    ReportDescriptorParser parser;
    QVERIFY(parser.parseTeamsReportIDs(data).isEmpty());
}

void UsbTest::testRealDescriptors_data()
{
    QTest::addColumn<QString>("file");
    QTest::addColumn<bool>(
            "expectHeadset"); // has a top-level Telephony/Headset (0x0B/0x05) collection
    QTest::addColumn<bool>("hasTeamsPage"); // has a vendor (0xFF99) usage page at all

    // Classification is derived from the descriptor's marker bytes (05 0B 09 05 A1 01 for the
    // headset application collection, 06 99 FF for the Teams vendor page) — independent of the
    // parser's internal computation, so these rows are not circular.
    QTest::newRow("Logitech G535 Wireless Gaming Headset") << "046d-0ac4.bin" << false << false;
    QTest::newRow("Plantronics Poly BT700") << "047f-02e6.bin" << true << true;
    QTest::newRow("Plantronics Poly Voyager Base CD") << "047f-02ec.bin" << true << false;
    QTest::newRow("Plantronics Blackwire C5220") << "047f-c053.bin" << true << true;
    // Composite device whose Telephony/Headset collection is the 4th (last) top-level collection
    // — parse() must still find it; the device-level enumeration fix lives in USBDevices.
    QTest::newRow("Jabra EVOLVE LINK MS") << "0b0e-0307.bin" << true << false;
    QTest::newRow("Jabra BIZ 2400 II") << "0b0e-2453.bin" << true << false;
    QTest::newRow("Jabra SPEAK 810") << "0b0e-2456.bin" << true << false;
    QTest::newRow("Sennheiser SC 660 ANC USB") << "1395-0082.bin" << true << false;
    QTest::newRow("Sennheiser EPOS IMPACT D1") << "1395-009c.bin" << true << true;
    QTest::newRow("Sennheiser EPOS IMPACT DW") << "1395-00a2.bin" << true << false;
    QTest::newRow("Sennheiser EPOS IMPACT D") << "1395-00a9.bin" << true << false;
    QTest::newRow("Yealink WH66") << "6993-b02b.bin" << true << true;
}

void UsbTest::testRealDescriptors()
{
    QFETCH(QString, file);
    QFETCH(bool, expectHeadset);
    QFETCH(bool, hasTeamsPage);

    const QByteArray data = loadDescriptor("hid/" + file);
    QVERIFY2(!data.isEmpty(), qPrintable("fixture hid/" + file + " missing"));

    // Note: 1395-009c/00a2/00a9 nest an application collection inside another. hid-rp's
    // parse_items() (run via an unused `Parser` pass) threw ex_collection_nested_application on
    // these; the production caller (USBDevices) caught it and silently dropped the device, so
    // these real headsets were never recognized. That dead pass has been removed, so they now
    // parse cleanly — these rows guard against a regression. (The throw crashed only this test,
    // which has no try/catch of its own.)
    ReportDescriptorParser parser;
    const auto appColl = parser.parse(data);

    if (expectHeadset) {
        QVERIFY2(appColl != nullptr, qPrintable(file + ": expected a headset collection"));
        QCOMPARE(appColl->rawPageId, quint32(0x0B));
        QCOMPARE(appColl->rawUsageId, quint32(0x05));
    } else {
        QVERIFY2(appColl == nullptr, qPrintable(file + ": must not report a headset collection"));
    }

    // Airtight invariant: parseTeamsReportIDs gates everything on the 0xFF99 usage page, so a
    // descriptor without that page can never yield Teams report ids. (Presence of the page does
    // not guarantee a non-empty map — the UC-Display usage must be 0x01 — so that direction is
    // only asserted for the verified Yealink fixture below.)
    if (!hasTeamsPage) {
        QVERIFY2(parser.parseTeamsReportIDs(data).isEmpty(),
                 qPrintable(file + ": no 0xFF99 page but Teams ids returned"));
    }
}

QTEST_GUILESS_MAIN(UsbTest)
