#pragma once

#include "LibAvPointers.h"

#include <QAbstractVideoBuffer>
#include <QVideoFrameFormat>

/// Exposes decoded YUV 4:2:0 AVFrame to Qt w/o copying planes.
class PlanarVideoBuffer : public QAbstractVideoBuffer
{
public:
    explicit PlanarVideoBuffer(AVFramePtr frame);

    MapData map(QVideoFrame::MapMode mode) override;
    QVideoFrameFormat format() const override;

private:
    AVFramePtr m_frame;
    QVideoFrameFormat m_format;
};
