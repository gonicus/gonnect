#pragma once

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavutil/frame.h>
#include <libswscale/swscale.h>
}

#include <memory>

struct AVCodecContextDeleter
{
    void operator()(AVCodecContext *context) const { avcodec_free_context(&context); }
};
struct AVFrameDeleter
{
    void operator()(AVFrame *frame) const { av_frame_free(&frame); }
};
struct AVPacketDeleter
{
    void operator()(AVPacket *packet) const { av_packet_free(&packet); }
};
struct SwsContextDeleter
{
    void operator()(SwsContext *context) const { sws_freeContext(context); }
};

using AVCodecContextPtr = std::unique_ptr<AVCodecContext, AVCodecContextDeleter>;
using AVFramePtr = std::unique_ptr<AVFrame, AVFrameDeleter>;
using AVPacketPtr = std::unique_ptr<AVPacket, AVPacketDeleter>;
using SwsContextPtr = std::unique_ptr<SwsContext, SwsContextDeleter>;
