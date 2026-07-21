#include "CallRouting.test.h"
#include "SIPCallRoutingHop.h"

#include <QTest>

static QString reasonTr(const char *source)
{
    return QCoreApplication::translate("SIPCallRoutingHop", source);
}

CallRoutingTest::CallRoutingTest(QObject *parent) : QObject{ parent } { }

void CallRoutingTest::testEmpty()
{
    QVERIFY(SIPCallRoutingHop::parse({}, {}).isEmpty());
    QVERIFY(SIPCallRoutingHop::parse({ "" }, { "" }).isEmpty());
}

void CallRoutingTest::testSingleDiversionHeader()
{
    const auto hops = SIPCallRoutingHop::parse(
            {}, { "<sip:+4915100@pbx.example.com>;reason=unconditional;counter=1" });

    QCOMPARE(hops.size(), 1);
    QCOMPARE(hops.at(0).uri, QString("sip:+4915100@pbx.example.com"));
    QCOMPARE(hops.at(0).reasonText, reasonTr("redirected"));
    QVERIFY(hops.at(0).diversion);
}

void CallRoutingTest::testMultipleDiversionHeaders()
{
    // Diversion headers are stacked most-recent-first; parse() reverses them so
    // the returned list is in chronological order.
    const auto hops =
            SIPCallRoutingHop::parse({},
                                     { "<sip:+4915300@pbx.example.com>;reason=user-busy",
                                       "<sip:+4915100@pbx.example.com>;reason=unconditional" });

    QCOMPARE(hops.size(), 2);
    QCOMPARE(hops.at(0).uri, QString("sip:+4915100@pbx.example.com"));
    QCOMPARE(hops.at(0).reasonText, reasonTr("redirected"));
    QCOMPARE(hops.at(1).uri, QString("sip:+4915300@pbx.example.com"));
    QCOMPARE(hops.at(1).reasonText, reasonTr("busy"));
}

void CallRoutingTest::testDiversionCommaSeparatedList()
{
    // Regression: multiple hops carried as a single comma-separated Diversion
    // header value must all be parsed, not just the first.
    const auto hops = SIPCallRoutingHop::parse(
            {},
            { "<sip:+4915300@pbx.example.com>;reason=user-busy;counter=1,"
              "<sip:+4915100@pbx.example.com>;reason=unconditional;counter=1" });

    QCOMPARE(hops.size(), 2);
    QCOMPARE(hops.at(0).uri, QString("sip:+4915100@pbx.example.com"));
    QCOMPARE(hops.at(0).reasonText, reasonTr("redirected"));
    QCOMPARE(hops.at(1).uri, QString("sip:+4915300@pbx.example.com"));
    QCOMPARE(hops.at(1).reasonText, reasonTr("busy"));
}

void CallRoutingTest::testDiversionQuotedReason()
{
    // Regression: reason may be a quoted-string; the quotes must be stripped
    // before the reason is translated.
    const auto hops = SIPCallRoutingHop::parse(
            {}, { "<sip:+4915100@pbx.example.com>;reason=\"unconditional\"" });

    QCOMPARE(hops.size(), 1);
    QCOMPARE(hops.at(0).reasonText, reasonTr("redirected"));
}

void CallRoutingTest::testHistoryInfoCommaSeparatedList()
{
    // Regression: RFC 7044 hi-entries in a single comma-separated header value
    // must all be parsed.
    const auto hops =
            SIPCallRoutingHop::parse({ "<sip:+4915100@pbx.example.com>;index=1,"
                                       "<sip:+4915200@pbx.example.com;cause=302>;index=1.1,"
                                       "<sip:+4915300@pbx.example.com;cause=486>;index=1.2" },
                                     {});

    QCOMPARE(hops.size(), 3);

    QCOMPARE(hops.at(0).uri, QString("sip:+4915100@pbx.example.com"));
    QCOMPARE(hops.at(0).index, QString("1"));
    QCOMPARE(hops.at(0).reason, -1);
    QVERIFY(!hops.at(0).diversion);

    QCOMPARE(hops.at(1).uri, QString("sip:+4915200@pbx.example.com;cause=302"));
    QCOMPARE(hops.at(1).index, QString("1.1"));
    QCOMPARE(hops.at(1).reason, 302);
    QCOMPARE(hops.at(1).reasonText, reasonTr("redirected"));

    QCOMPARE(hops.at(2).index, QString("1.2"));
    QCOMPARE(hops.at(2).reason, 486);
    QCOMPARE(hops.at(2).reasonText, reasonTr("busy"));
}

void CallRoutingTest::testHistoryInfoMultipleHeaders()
{
    // Multiple separate History-Info headers (each one value) are concatenated
    // in document order.
    const auto hops =
            SIPCallRoutingHop::parse({ "<sip:+4915100@pbx.example.com>;index=1",
                                       "<sip:+4915200@pbx.example.com;cause=302>;index=1.1",
                                       "<sip:+4915300@pbx.example.com;cause=408>;index=1.2" },
                                     {});

    QCOMPARE(hops.size(), 3);
    QCOMPARE(hops.at(0).uri, QString("sip:+4915100@pbx.example.com"));
    QCOMPARE(hops.at(1).reason, 302);
    QCOMPARE(hops.at(2).reason, 408);
    QCOMPARE(hops.at(2).reasonText, reasonTr("not answered"));
}

void CallRoutingTest::testHistoryInfoPreferredOverDiversion()
{
    // When both header types are present, History-Info (qualified RFC) wins.
    const auto hops =
            SIPCallRoutingHop::parse({ "<sip:+49222@pbx.example.com>;index=1" },
                                     { "<sip:+49111@pbx.example.com>;reason=unconditional" });

    QCOMPARE(hops.size(), 1);
    QCOMPARE(hops.at(0).uri, QString("sip:+49222@pbx.example.com"));
    QVERIFY(!hops.at(0).diversion);
}

void CallRoutingTest::testUriPreservesCase()
{
    // Regression: the URI must keep its original case (previously the whole
    // message was lower-cased before parsing).
    const auto hops = SIPCallRoutingHop::parse({ "<sip:Alice@Example.COM>;index=1" }, {});

    QCOMPARE(hops.size(), 1);
    QCOMPARE(hops.at(0).uri, QString("sip:Alice@Example.COM"));
}

void CallRoutingTest::testEntryWithoutUriSkipped()
{
    const auto hops =
            SIPCallRoutingHop::parse({}, { "sip:noangle@pbx.example.com;reason=unconditional" });

    QVERIFY(hops.isEmpty());
}

void CallRoutingTest::testUriWithCommaNotSplit()
{
    // A comma inside the angle-bracketed URI must not be treated as an entry
    // separator.
    const auto hops =
            SIPCallRoutingHop::parse({ "<sip:+4915100@pbx.example.com;foo=a,b>;index=1" }, {});

    QCOMPARE(hops.size(), 1);
    QCOMPARE(hops.at(0).uri, QString("sip:+4915100@pbx.example.com;foo=a,b"));
}

QTEST_GUILESS_MAIN(CallRoutingTest)
