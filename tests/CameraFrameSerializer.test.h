#pragma once

#include <QObject>

class CameraFrameSerializerTest : public QObject
{
    Q_OBJECT

private slots:
    void serializesI420WithoutPadding_data();
    void serializesI420WithoutPadding();

    void serializesOtherFormatsAsRgbx();
    void rejectsInvalidFrame();
};
