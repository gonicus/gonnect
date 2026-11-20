#include "KeyFrameGate.test.h"
#include "KeyFrameGate.h"

#include <QTest>

using namespace std::chrono_literals;

namespace {

EncodedFrame frame(VideoCodec codec, bool isKeyFrame)
{
    EncodedFrame encodedFrame;
    encodedFrame.codec = codec;
    encodedFrame.isKeyFrame = isKeyFrame;
    return encodedFrame;
}

} // namespace

void KeyFrameGateTest::dropsUntilKeyFrame()
{
    KeyFrameGate gate;

    QVERIFY(!gate.accept(frame(VideoCodec::VP8, false)));
    QVERIFY(gate.accept(frame(VideoCodec::VP8, true)));
    QVERIFY(gate.accept(frame(VideoCodec::VP8, false)));
    QVERIFY(!gate.isWaitingForKeyFrame());
}

void KeyFrameGateTest::codecChangeRequiresKeyFrame()
{
    KeyFrameGate gate;
    QVERIFY(gate.accept(frame(VideoCodec::VP8, true)));

    QVERIFY(!gate.accept(frame(VideoCodec::VP9, false)));
    QVERIFY(gate.isWaitingForKeyFrame());
    QVERIFY(gate.accept(frame(VideoCodec::VP9, true)));
}

void KeyFrameGateTest::requiredKeyFrameDropsInterFrames()
{
    KeyFrameGate gate;
    QVERIFY(gate.accept(frame(VideoCodec::AV1, true)));

    gate.requireKeyFrame();

    QVERIFY(!gate.accept(frame(VideoCodec::AV1, false)));
    QVERIFY(gate.accept(frame(VideoCodec::AV1, true)));
}

void KeyFrameGateTest::throttlesKeyFrameRequests()
{
    KeyFrameGate gate;
    const KeyFrameGate::Clock::time_point start;

    QVERIFY(gate.shouldRequestKeyFrame(start));
    QVERIFY(!gate.shouldRequestKeyFrame(start + 100ms));
    QVERIFY(gate.shouldRequestKeyFrame(start + KeyFrameGate::RequestInterval));

    // No requests once a key frame arrived
    QVERIFY(gate.accept(frame(VideoCodec::VP8, true)));
    QVERIFY(!gate.shouldRequestKeyFrame(start + 10s));
}

QTEST_GUILESS_MAIN(KeyFrameGateTest)
