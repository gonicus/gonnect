#pragma once

#include <QObject>

class CallInfoTest : public QObject
{
    Q_OBJECT
public:
    explicit CallInfoTest(QObject *parent = nullptr);

private slots:
    void testEmpty();
    void testSecurityValues();
    void testSpaceAfterEquals();
    void testUpperCaseUrn();
    void testIgnoresOtherParameters();
    void testIgnoresNonCiscoCallInfo();
    void testCommaSeparatedList();
    void testMultipleHeaders();
    void testUiStateRingout();
    void testRingoutClearedWithoutUiState();
    void testUiStateBusy();
    void testUnknownUiStateValue();
    void testGci();
    void testGciAbsent();
    void testPriority();
    void testPriorityAbsentIsNormal();
    void testUnknownPriorityValue();
    void testUnknownSecurityValue();
    void testSecurityWithoutValue();
};
