#pragma once

#include <QObject>

class VideoDecoderTest : public QObject
{
    Q_OBJECT

private slots:
    void decodesIvf_data();
    void decodesIvf();

    void rejectsInterFrameWithoutKeyFrame();
    void rejectsDecodeWhenNotOpen();
};
