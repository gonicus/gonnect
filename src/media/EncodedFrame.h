#pragma once

#include "VideoCodec.h"

#include <QByteArray>
#include <QSize>

/// Encoded video frame (as in WebRTC)
struct EncodedFrame
{
    VideoCodec codec = VideoCodec::VP8;
    bool isKeyFrame = false;
    quint32 rtpTimestamp = 0;
    QSize size; // For keyframes only
    QByteArray payload;
};
