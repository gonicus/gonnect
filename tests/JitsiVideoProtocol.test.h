#pragma once

#include <QObject>

class JitsiVideoProtocolTest : public QObject
{
    Q_OBJECT

private slots:
    void parsesFrame();
    void rejectsInvalidFrame_data();
    void rejectsInvalidFrame();
    void parsesCodecs();
    void rejectsMessageOfOtherType();
    void parsesTracks();
    void createsKeyFrameRequest();
    void mapsQualityHeight_data();
    void mapsQualityHeight();
    void createsReceiverConstraints();
};
