#pragma once

#include <QObject>

class PipeWireFrameConverterTest : public QObject
{
    Q_OBJECT

private slots:
    void convertsYuvFormats_data();
    void convertsYuvFormats();

    void convertsToLimitedRange_data();
    void convertsToLimitedRange();

    void convertsMjpeg_data();
    void convertsMjpeg();

    void rejectsUnsupportedFormat();
    void rejectsBufferWithoutFormat();
    void rejectsShortChunk();
    void rejectsCorruptedChunk();
    void rejectsBrokenMjpeg();
};
