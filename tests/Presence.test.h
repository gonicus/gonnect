#pragma once

#include <QObject>

class PresenceTest : public QObject
{
    Q_OBJECT
public:
    explicit PresenceTest(QObject *parent = nullptr);

private slots:
    void testEmptyAggregatorIsUnknown();
    void testSingleProviderState();
    void testPriorityAggregation();
    void testRingingWinsOverEverything();
    void testStateTextJoin();
    void testStateTextSkipsEmpty();
    void testStateChangeRecomputes();
    void testProviderDestroyedIsRemoved();
    void testNullptrRegistrationIgnored();
    void testDuplicateRegistrationIgnored();
};
