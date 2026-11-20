#pragma once

#include <QByteArray>
#include <QVideoFrame>

/// Serializer for camera frames of gonnectCamera.js.
///
/// Frame layout (all multi-byte values are little endian):
///
///   0          4          8          12
///   +----------+----------+----------+---------------------------------+
///   | format   | width    | height   | planes (no row padding)         |
///   | uint32   | uint32   | uint32   |                                 |
///   +----------+----------+----------+---------------------------------+
///   '-------- header (12 bytes) -----''---- pixel data (see below) -----'
///
/// There is no padding between rows or between planes: each row is exactly
/// width × bytes-per-pixel bytes long, and the planes follow each other directly.
///
/// I420 (planar YUV 4:2:0): three planes, in this order:
///
///   +---------------------------+------------------+------------------+
///   | Y                         | U                | V                |
///   | width × height            | cw × ch          | cw × ch          |
///   +---------------------------+------------------+------------------+
///
///   cw = (width + 1) / 2     (half the width, rounded up)
///   ch = (height + 1) / 2    (half the height, rounded up)
///   pixel data size = width × height + 2 × cw × ch
///
/// RGBX: a single plane, 4 bytes per pixel:
///
///   +----------------------------------------------------------------+
///   | RGBX                                                           |
///   | width × height × 4 bytes                                       |
///   +----------------------------------------------------------------+
class CameraFrameSerializer
{
public:
    enum class FrameFormat : quint32 { I420 = 0, RGBX = 1 };

    static constexpr qsizetype HeaderSize = 3 * sizeof(quint32);

    static QByteArray serialize(const QVideoFrame &frame);

    CameraFrameSerializer() = delete;
};
