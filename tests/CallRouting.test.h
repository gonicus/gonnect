#pragma once

#include <QObject>

class CallRoutingTest : public QObject
{
    Q_OBJECT
public:
    explicit CallRoutingTest(QObject *parent = nullptr);

private slots:
    void testEmpty();
    void testSingleDiversionHeader();
    void testMultipleDiversionHeaders();
    void testDiversionCommaSeparatedList();
    void testDiversionQuotedReason();
    void testHistoryInfoCommaSeparatedList();
    void testHistoryInfoMultipleHeaders();
    void testHistoryInfoPreferredOverDiversion();
    void testUriPreservesCase();
    void testEntryWithoutUriSkipped();
    void testUriWithCommaNotSplit();
};
