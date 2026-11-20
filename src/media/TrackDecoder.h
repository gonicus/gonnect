#pragma once

#include "EncodedFrame.h"
#include "VideoDecoder.h"

#include <QObject>
#include <QSize>
#include <QString>

#include <atomic>
#include <memory>

class FramePump;

/// Decodes frames of single remote track (in the objects thread)
class TrackDecoder : public QObject
{
    Q_OBJECT

public:
    explicit TrackDecoder(const QString &trackId, QObject *parent = nullptr);

    /// Callable from any thread; output may change from frame to frame
    void enqueue(EncodedFrame frame, std::shared_ptr<FramePump> output);

    /// Frames enqueued but not decoded yet
    int queuedFrames() const { return m_queuedFrames.load(); }

Q_SIGNALS:
    void decodeFailed();

private:
    void decode(const EncodedFrame &frame, const std::shared_ptr<FramePump> &output);

    QString m_trackId;
    VideoDecoder m_decoder;
    QSize m_size;
    std::atomic<int> m_queuedFrames{ 0 };
};
