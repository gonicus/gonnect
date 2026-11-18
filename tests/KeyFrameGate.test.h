#pragma once

#include <QObject>

class KeyFrameGateTest : public QObject
{
    Q_OBJECT

private slots:
    void dropsUntilKeyFrame();
    void codecChangeRequiresKeyFrame();
    void requiredKeyFrameDropsInterFrames();
    void throttlesKeyFrameRequests();
};
