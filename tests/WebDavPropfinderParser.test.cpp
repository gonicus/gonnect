#include "WebDavPropfinderParser.test.h"
#include "WebDavPropfindParser.h"

#include <QTest>
#include <QXmlStreamReader>

WebDavPropfindParserTest::WebDavPropfindParserTest(QObject *parent) : QObject{ parent } { }

void WebDavPropfindParserTest::testParsesCollectionAndEtag()
{
    const QByteArray xml = R"(<?xml version="1.0"?>
          <D:multistatus xmlns:D="DAV:">
            <D:response>
              <D:href>/calendars/user/home/</D:href>
              <D:propstat>
                <D:prop>
                  <D:resourcetype><D:collection/></D:resourcetype>
                  <D:getetag>"abc123"</D:getetag>
                </D:prop>
                <D:status>HTTP/1.1 200 OK</D:status>
              </D:propstat>
            </D:response>
          </D:multistatus>)";

    WebDavPropfindParser parser;
    const auto items = parser.parseMultiStatus(xml);

    QCOMPARE(items.size(), 1);
    QCOMPARE(items.first().href, QString("/calendars/user/home/"));
    QVERIFY(items.first().isCollection());
    QCOMPARE(items.first().property("DAV:", "getetag").text, QString("\"abc123\""));
    QCOMPARE(items.first().httpStatus, 200);
}

void WebDavPropfindParserTest::testSkipsPropertiesFrom404Propstat()
{
    const QByteArray xml = R"(<?xml version="1.0"?>
          <D:multistatus xmlns:D="DAV:">
            <D:response>
              <D:href>/addressbooks/user/contacts/card1.vcf</D:href>
              <D:propstat>
                <D:prop><D:getetag>"etag-1"</D:getetag></D:prop>
                <D:status>HTTP/1.1 200 OK</D:status>
              </D:propstat>
              <D:propstat>
                <D:prop><D:displayname/></D:prop>
                <D:status>HTTP/1.1 404 Not Found</D:status>
              </D:propstat>
            </D:response>
          </D:multistatus>)";

    WebDavPropfindParser parser;
    const auto items = parser.parseMultiStatus(xml);

    QCOMPARE(items.size(), 1);
    QCOMPARE(items.first().property("DAV:", "getetag").text, QString("\"etag-1\""));
    QVERIFY(!items.first().properties.contains("DAV:|displayname"));
}

void WebDavPropfindParserTest::testHandlesResourceLevelForbiddenStatus()
{
    const QByteArray xml = R"(<?xml version="1.0"?>
          <D:multistatus xmlns:D="DAV:">
            <D:response>
              <D:href>/calendars/other-user/private/</D:href>
              <D:status>HTTP/1.1 403 Forbidden</D:status>
            </D:response>
          </D:multistatus>)";

    WebDavPropfindParser parser;
    const auto items = parser.parseMultiStatus(xml);

    QCOMPARE(items.size(), 1);
    QCOMPARE(items.first().httpStatus, 403);
    QVERIFY(items.first().properties.isEmpty());
}

void WebDavPropfindParserTest::testParsesMultipleResponses()
{
    const QByteArray xml = R"(<?xml version="1.0"?>
          <D:multistatus xmlns:D="DAV:">
            <D:response><D:href>/calendars/user/home/</D:href>
              <D:propstat><D:prop><D:resourcetype><D:collection/></D:resourcetype></D:prop>
                <D:status>HTTP/1.1 200 OK</D:status></D:propstat></D:response>
            <D:response><D:href>/calendars/user/home/event1.ics</D:href>
              <D:propstat><D:prop><D:getetag>"e1"</D:getetag></D:prop>
                <D:status>HTTP/1.1 200 OK</D:status></D:propstat></D:response>
            <D:response><D:href>/calendars/user/home/event2.ics</D:href>
              <D:propstat><D:prop><D:getetag>"e2"</D:getetag></D:prop>
                <D:status>HTTP/1.1 200 OK</D:status></D:propstat></D:response>
          </D:multistatus>)";

    WebDavPropfindParser parser;
    const auto items = parser.parseMultiStatus(xml);

    QCOMPARE(items.size(), 3);
    QVERIFY(items.at(0).isCollection());
    QVERIFY(!items.at(1).isCollection());
    QCOMPARE(items.at(1).property("DAV:", "getetag").text, QString("\"e1\""));
    QCOMPARE(items.at(2).property("DAV:", "getetag").text, QString("\"e2\""));
}

void WebDavPropfindParserTest::testParsesNonDavNamespaceProperty()
{
    const QByteArray xml = R"(<?xml version="1.0"?>
          <D:multistatus xmlns:D="DAV:" xmlns:C="urn:ietf:params:xml:ns:caldav">
            <D:response>
              <D:href>/principals/users/admin/</D:href>
              <D:propstat>
                <D:prop>
                  <C:calendar-home-set><D:href>/calendars/admin/</D:href></C:calendar-home-set>
                </D:prop>
                <D:status>HTTP/1.1 200 OK</D:status>
              </D:propstat>
            </D:response>
          </D:multistatus>)";

    WebDavPropfindParser parser;
    const auto items = parser.parseMultiStatus(xml);

    QCOMPARE(items.size(), 1);
    const auto homeSet =
            items.first().property("urn:ietf:params:xml:ns:caldav", "calendar-home-set");

    bool foundHrefChild = false;
    for (const auto &child : homeSet.childElements) {
        if (child.namespaceUri == "DAV:" && child.localName == "href") {
            foundHrefChild = true;
        }
    }
    QVERIFY(foundHrefChild);
}

void WebDavPropfindParserTest::testParsesComponentAttributesForCalendarFiltering()
{
    const QByteArray xml = R"(<?xml version="1.0"?>
            <D:multistatus xmlns:D="DAV:" xmlns:C="urn:ietf:params:xml:ns:caldav">
              <D:response>
                <D:href>/caldav/user/tasks/</D:href>
                <D:propstat>
                  <D:prop>
                    <D:resourcetype><D:collection/><C:calendar/></D:resourcetype>
                    <C:supported-calendar-component-set>
                      <C:comp name="VTODO"/>
                      <C:comp name="VTIMEZONE"/>
                    </C:supported-calendar-component-set>
                  </D:prop>
                  <D:status>HTTP/1.1 200 OK</D:status>
                </D:propstat>
              </D:response>
              <D:response>
                <D:href>/caldav/user/calendar/</D:href>
                <D:propstat>
                  <D:prop>
                    <D:resourcetype><D:collection/><C:calendar/></D:resourcetype>
                    <C:supported-calendar-component-set>
                      <C:comp name="VEVENT"/>
                      <C:comp name="VTIMEZONE"/>
                    </C:supported-calendar-component-set>
                  </D:prop>
                  <D:status>HTTP/1.1 200 OK</D:status>
                </D:propstat>
              </D:response>
            </D:multistatus>)";

    WebDavPropfindParser parser;
    const auto items = parser.parseMultiStatus(xml);

    QCOMPARE(items.size(), 2);

    const auto &tasks = items.at(0);
    QVERIFY(tasks.isCalendarCollection());
    QVERIFY(tasks.supportsComponent("VTODO"));
    QVERIFY(!tasks.supportsComponent("VEVENT"));

    const auto &calendar = items.at(1);
    QVERIFY(calendar.isCalendarCollection());
    QVERIFY(calendar.supportsComponent("VEVENT"));
    QVERIFY(!calendar.supportsComponent("VTODO"));
}

void WebDavPropfindParserTest::testBuildPropfindBodyProducesValidXmlWithMultipleNamespaces()
{
    WebDavPropfindParser parser;
    const auto body = parser.buildPropfindBody(
            { { "D", "DAV:", "resourcetype" },
              { "D", "DAV:", "getetag" },
              { "C", "urn:ietf:params:xml:ns:caldav", "calendar-home-set" } });

    QXmlStreamReader reader(body);
    while (!reader.atEnd()) {
        reader.readNext();
    }
    QVERIFY2(!reader.hasError(), qPrintable(reader.errorString()));
}

void WebDavPropfindParserTest::testParseMultiStatusHandlesEmptyInput()
{
    WebDavPropfindParser parser;
    QVERIFY(parser.parseMultiStatus(QByteArray()).isEmpty());
    QVERIFY(parser.parseMultiStatus("not xml at all").isEmpty());
}

QTEST_GUILESS_MAIN(WebDavPropfindParserTest)
