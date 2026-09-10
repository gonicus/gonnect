#pragma once

#include <QObject>

class ContactsTest : public QObject
{
    Q_OBJECT
public:
    explicit ContactsTest(QObject *parent = nullptr);

private slots:
    void testCleanPhoneNumber();

    void testLevenshteinDistance();
    void testJaroWinklerDistance();
    void testSortListByWeight();

    void testIsSipUri();
    void testNumberFromSipUrl();
    void testNameFromSipUrl();
    void testBareURI();
    void testClearInternationalChars();
    void testIsEmergencyCallUrl();
    void testIsNumberAnonymous();
};
