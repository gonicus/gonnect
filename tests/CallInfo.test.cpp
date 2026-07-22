#include "CallInfo.test.h"
#include "SIPCallInfo.h"

#include <QTest>

CallInfoTest::CallInfoTest(QObject *parent) : QObject{ parent } { }

void CallInfoTest::testEmpty()
{
    const auto none = SIPCallInfo::parse({});
    QVERIFY(!none.isValid());
    QCOMPARE(none.security(), SIPCallInfo::Security::Unknown);
    QCOMPARE(none.uiState(), SIPCallInfo::UiState::None);

    QVERIFY(!SIPCallInfo::parse({ "" }).isValid());
}

void CallInfoTest::testSecurityValues()
{
    QCOMPARE(SIPCallInfo::parse({ "<urn:x-cisco-remotecc:callinfo>;security=Encrypted" }).security(),
             SIPCallInfo::Security::Encrypted);
    QCOMPARE(SIPCallInfo::parse({ "<urn:x-cisco-remotecc:callinfo>;security=Authenticated" })
                     .security(),
             SIPCallInfo::Security::Authenticated);
    QCOMPARE(SIPCallInfo::parse({ "<urn:x-cisco-remotecc:callinfo>;security=NotAuthenticated" })
                     .security(),
             SIPCallInfo::Security::NotAuthenticated);
}

void CallInfoTest::testSpaceAfterEquals()
{
    // The samples in Cisco OL-25254-01 all put a space behind the "=".
    const auto info = SIPCallInfo::parse(
            { "<urn:x-cisco-remotecc:callinfo>; security= NotAuthenticated; orientation= from; "
              "gci= 1-18004; call-instance= 1" });

    QVERIFY(info.isValid());
    QCOMPARE(info.security(), SIPCallInfo::Security::NotAuthenticated);
}

void CallInfoTest::testUpperCaseUrn()
{
    // The document spells the urn both ways.
    const auto info = SIPCallInfo::parse({ "<urn:X-cisco-remotecc:callinfo>; security=Encrypted" });

    QVERIFY(info.isValid());
    QCOMPARE(info.security(), SIPCallInfo::Security::Encrypted);
}

void CallInfoTest::testIgnoresOtherParameters()
{
    const auto info = SIPCallInfo::parse(
            { "<urn:x-cisco-remotecc:callinfo>; orientation= to; gci= 1-162120; call-instance= 1" });

    QVERIFY(info.isValid());
    QCOMPARE(info.security(), SIPCallInfo::Security::Unknown);
    QCOMPARE(info.uiState(), SIPCallInfo::UiState::None);
}

void CallInfoTest::testIgnoresNonCiscoCallInfo()
{
    const auto info =
            SIPCallInfo::parse({ "<http://www.example.com/photo.jpg>;purpose=icon",
                                 "<http://wwww.example.com/alice/>;purpose=info" });

    QVERIFY(!info.isValid());
    QCOMPARE(info.security(), SIPCallInfo::Security::Unknown);
}

void CallInfoTest::testCommaSeparatedList()
{
    // One header line may carry several entries.
    const auto info = SIPCallInfo::parse({ "<http://www.example.com/photo.jpg>;purpose=icon, "
                                           "<urn:x-cisco-remotecc:callinfo>; security= Encrypted" });

    QVERIFY(info.isValid());
    QCOMPARE(info.security(), SIPCallInfo::Security::Encrypted);
}

void CallInfoTest::testMultipleHeaders()
{
    const auto info = SIPCallInfo::parse({ "<http://www.example.com/photo.jpg>;purpose=icon",
                                           "<urn:x-cisco-remotecc:callinfo>; security= "
                                           "Authenticated" });

    QVERIFY(info.isValid());
    QCOMPARE(info.security(), SIPCallInfo::Security::Authenticated);
}

void CallInfoTest::testUiStateRingout()
{
    const auto info = SIPCallInfo::parse({ "<urn:x-cisco-remotecc:callinfo>; ui-state= ringout" });

    QVERIFY(info.isValid());
    QCOMPARE(info.uiState(), SIPCallInfo::UiState::Ringout);

    const auto both = SIPCallInfo::parse({ "<urn:x-cisco-remotecc:callinfo>; security= "
                                           "NotAuthenticated; orientation= to; gci= 1-162120; "
                                           "ui-state= ringout; call-instance= 1" });
    QCOMPARE(both.security(), SIPCallInfo::Security::NotAuthenticated);
    QCOMPARE(both.uiState(), SIPCallInfo::UiState::Ringout);
}

void CallInfoTest::testRingoutClearedWithoutUiState()
{
    // "This reINVITE or UPDATE will not contain a ui-state value and phone must stop playing
    // ringback in this case." - so a valid header without ui-state means: not ringing.
    const auto info =
            SIPCallInfo::parse({ "<urn:x-cisco-remotecc:callinfo>; security= Encrypted" });

    QVERIFY(info.isValid());
    QCOMPARE(info.uiState(), SIPCallInfo::UiState::None);
}

void CallInfoTest::testUiStateBusy()
{
    // Endpoints advertising X-cisco-callinfo get no 486 for a busy line, CUCM sends a 183
    // Session Progress carrying ui-state= busy instead.
    const auto info = SIPCallInfo::parse({ "<urn:x-cisco-remotecc:callinfo>; security= "
                                           "NotAuthenticated; ui-state= busy; call-instance= 1" });

    QVERIFY(info.isValid());
    QCOMPARE(info.uiState(), SIPCallInfo::UiState::Busy);
    QCOMPARE(info.security(), SIPCallInfo::Security::NotAuthenticated);
}

void CallInfoTest::testUnknownUiStateValue()
{
    const auto info = SIPCallInfo::parse({ "<urn:x-cisco-remotecc:callinfo>; ui-state= bogus" });

    QVERIFY(info.isValid());
    QCOMPARE(info.uiState(), SIPCallInfo::UiState::None);
}

void CallInfoTest::testGci()
{
    // The Global Call Identifier groups all legs of one call and is what the call persistency
    // feature needs to resume a call after a network switch.
    const auto info = SIPCallInfo::parse(
            { "<urn:x-cisco-remotecc:callinfo>; security= NotAuthenticated; orientation= from; "
              "gci= 1-18004; call-instance= 1" });

    QVERIFY(info.isValid());
    QCOMPARE(info.gci(), QString("1-18004"));
}

void CallInfoTest::testGciAbsent()
{
    const auto info = SIPCallInfo::parse({ "<urn:x-cisco-remotecc:callinfo>; security= Encrypted" });

    QVERIFY(info.isValid());
    QVERIFY(info.gci().isEmpty());
}

void CallInfoTest::testPriority()
{
    // Emergency operator call back must reach the user even while another call is active.
    const auto emergency =
            SIPCallInfo::parse({ "<urn:x-cisco-remotecc:callinfo>; priority= emergency" });
    QCOMPARE(emergency.priority(), SIPCallInfo::Priority::Emergency);
    QVERIFY(emergency.priorityOverride());

    // Hold reversion, park reversion and call pickup.
    const auto urgent =
            SIPCallInfo::parse({ "<urn:x-cisco-remotecc:callinfo>; security= Encrypted; "
                                 "priority= urgent; call-instance= 1" });
    QCOMPARE(urgent.priority(), SIPCallInfo::Priority::Urgent);
    QVERIFY(urgent.priorityOverride());
    QCOMPARE(urgent.security(), SIPCallInfo::Security::Encrypted);
}

void CallInfoTest::testPriorityAbsentIsNormal()
{
    // "All other calls are considered as having a priority of 'normal' and a priority
    // parameter is not provided."
    const auto info =
            SIPCallInfo::parse({ "<urn:x-cisco-remotecc:callinfo>; security= Encrypted" });

    QCOMPARE(info.priority(), SIPCallInfo::Priority::Normal);
    QVERIFY(!info.priorityOverride());

    const auto explicitNormal =
            SIPCallInfo::parse({ "<urn:x-cisco-remotecc:callinfo>; priority= normal" });
    QCOMPARE(explicitNormal.priority(), SIPCallInfo::Priority::Normal);
    QVERIFY(!explicitNormal.priorityOverride());
}

void CallInfoTest::testUnknownPriorityValue()
{
    // Must not accidentally override alerting on something we do not understand.
    const auto info = SIPCallInfo::parse({ "<urn:x-cisco-remotecc:callinfo>; priority= bogus" });

    QVERIFY(info.isValid());
    QCOMPARE(info.priority(), SIPCallInfo::Priority::Normal);
    QVERIFY(!info.priorityOverride());
}

void CallInfoTest::testUnknownSecurityValue()
{
    const auto info = SIPCallInfo::parse({ "<urn:x-cisco-remotecc:callinfo>; security= Bogus" });

    QVERIFY(info.isValid());
    QCOMPARE(info.security(), SIPCallInfo::Security::Unknown);
}

void CallInfoTest::testSecurityWithoutValue()
{
    const auto info = SIPCallInfo::parse({ "<urn:x-cisco-remotecc:callinfo>; security=" });

    QVERIFY(info.isValid());
    QCOMPARE(info.security(), SIPCallInfo::Security::Unknown);
}

QTEST_GUILESS_MAIN(CallInfoTest)
