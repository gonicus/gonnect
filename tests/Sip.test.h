#pragma once

#include <QObject>

class SipTest : public QObject
{
    Q_OBJECT
public:
    explicit SipTest(QObject *parent = nullptr);

private slots:
    void testIsIdentityNumberValid();
    void testIsDisplayNameValid();
    void testIsPrefixValid();
};
