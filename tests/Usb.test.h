#pragma once

#include <QObject>

class UsbTest : public QObject
{
    Q_OBJECT
public:
    explicit UsbTest(QObject *parent = nullptr);

private slots:
    // Synthetic descriptors — bytes fully known, exact assertions
    void testSyntheticHeadsetParseTree();
    void testSyntheticTeamsReportIds();

    // Cross-negative: each parser must ignore descriptors that are not its target
    void testParseIgnoresNonHeadset();
    void testTeamsIgnoresNonTeams();

    // Real captured hardware — capture-independent invariants across all fixtures
    void testRealDescriptors_data();
    void testRealDescriptors();
};
